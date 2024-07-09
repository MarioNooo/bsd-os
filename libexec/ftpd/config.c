/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      BSDI config.c,v 1.5 1999/01/12 15:27:16 prb Exp
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <syslog.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
#include "ftpd.h"
#include "pathnames.h"

typedef enum {
	EMAIL,
	HOSTNAME,
	PATHNAME,
	FILENAME,
	DIRNAME,
	USERNAME,
	ONOFF,
	NUMBER,
	NUMBER8,
	STRING,
	INCOMING,
	REGEXPS
} arg_t;

#define	O(x)	((char *)&(((struct ftphost *)0)->x) - (char *)0)
#define	R(o,t)	(*(t *)(((char *)ch) + (o)->result))

void freehosts(struct ftphost *fh);

struct ftpdir *ftpdirs;
struct ftphost *thishost, *defhost;

#define	GD	0x01		/* Guest disabled for this command */
#define	GE	0x02		/* Guest enabled for this command */
#define	CE	0x04		/* Command implemented */
#define	CD	0x08		/* Command not implemented */

#define	G	1		/* Field allowed in <Guest> block */
#define	U	2		/* Field allowed outside of <Guest> block */

static struct fieldtab {
	char	*field;
	int	flags;
	arg_t	type;
	int	result;
	int	bit;
} fieldtab[] = {
    { "AllowAnonymous",	       U, ONOFF,	O(flags), 	ALLOW_ANON, },
    { "AnonymousDir",	       U, DIRNAME,	O(anondir), },
    { "AnonymousOnly",	       U, ONOFF, 	O(flags),	ANON_ONLY, },
    { "AnonymousUser",	       U, USERNAME,	O(anonuser), },
    { "BannedUserList",	       U, PATHNAME,	O(banlist), },
    { "BuiltinLS",	     G|U, ONOFF,	O(flags),	BUILTIN_LS, },
    { "ChrootUserList",	       U, PATHNAME,	O(chrootlist), },
    { "Debug",		     G|U, ONOFF,	O(flags),	DEBUG, },
    { "ExtraLogging",	     G|U, ONOFF,	O(flags),	ELOGGING, },
    { "GroupFile",	       U, PATHNAME,	O(groupfile), },
    { "Incoming",	     G  , INCOMING,	},
    { "KeepAlive",	     G|U, ONOFF,	O(flags), 	KEEPALIVE},
    { "LogFormat",	       U, STRING,	O(logfmt), },
    { "Logging",	     G|U, ONOFF,	O(flags),	LOGGING, },
    { "LoginMessage",	       U, PATHNAME,	O(loginmsg), },
    { "MaxTimeout",	       U, NUMBER, 	O(maxtimeout), },
    { "MaxUsers",	     G  , NUMBER, 	O(maxusers), },
    { "MessageFile",	     G|U, FILENAME, 	O(msgfile), },
    { "PathFilter",   	     G  , REGEXPS,	O(pathfilter), },
    { "PermittedUserList",     U, PATHNAME,	O(permitlist), },
    { "Proxy",		     G|U, ONOFF,	O(flags),	PROXY_OK, },
    { "RestrictedDataPorts", G|U, ONOFF,	O(flags),	RESTRICTEDPORTS, },
    { "ServerName",	       U, HOSTNAME,	O(hostname), },
    { "StatFile",	       U, PATHNAME,	O(statfile), },
    { "Stats",		     G|U, ONOFF,	O(flags),	STATS, },
    { "Timeout",	       U, NUMBER,	O(timeout), },
    { "Umask",		       U, NUMBER8,	O(umask), },
    { "UseHighPorts",	     G|U, ONOFF,	O(flags),	HIGHPORTS, },
    { "UseRFC931",	       U, ONOFF,	O(flags),	USERFC931, },
    { "VirtualOnly",	       U, ONOFF,	O(flags), 	VIRTUALONLY, },
    { "WelcomeMessage",	       U, PATHNAME,	O(welcome), },
    { NULL, },
};


int ftphostsz = 0;

void
eprintf(char *fmt, ...)
{
        va_list ap;

	if (show_error == 0)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
config_defhost()
{
	char hostname[MAXHOSTNAMELEN];

	ftphostsz = sizeof(struct ftphost) - 4 + ncmdtab + nsitetab;

	if (gethostname(hostname, sizeof(hostname)) < 0)
		hostname[0] = '\0';

	if ((defhost = malloc(ftphostsz)) == NULL)
		fatal("internal memory allocation failure");

	memset(defhost, 0, ftphostsz);
        if ((defhost->hostname = strdup(hostname)) == NULL)
		fatal("internal memory allocation failure");
        defhost->chrootlist = _PATH_FTPCHROOT;
        defhost->banlist = _PATH_FTPUSERS;
        defhost->anonuser = "ftp";
        defhost->loginmsg = _PATH_FTPLOGINMESG;
        defhost->statfile = _PATH_FTPDSTATFILE;
        defhost->welcome  = _PATH_FTPWELCOME;
        defhost->msgfile  = ".message";
        defhost->hostname = sgetsave(hostname);
	defhost->logfmt = "%{time} %{duration} %{remote} %{size} %{path} %{type} %{action} %{direction} %{session} %{user} %{authtype} %{authuser}";

        defhost->flags = HIGHPORTS | RESTRICTEDPORTS | ALLOW_ANON | BUILTIN_LS;
        defhost->flags |= defhost->flags << GUESTSHIFT;
        defhost->flags |= PROXY_OK;	/* only for non-guest sessions */
        defhost->maxtimeout = 7200; /* limit idle time to 2 hours */
        defhost->timeout = 900; /* timeout after 15 minutes of inactivity */
        defhost->umask = CMASK;
	defhost->sitecmds = &defhost->cmds[ncmdtab];
	thishost = defhost;	/* for main() */
}

int
addrequal(struct sockaddr *s1, struct sockaddr *s2)
{
	if (s1->sa_family == s2->sa_family) {
		switch (s1->sa_family) {
		case AF_INET:
			if (memcmp(&SIN(s1)->sin_addr, &SIN(s2)->sin_addr,
			    sizeof(SIN(s2)->sin_addr)) == 0)
				return (1);
			break;
#if INET6
		case AF_INET6:
			if (memcmp(&SIN6(s1)->sin6_addr, &SIN6(s2)->sin6_addr,
			    sizeof(SIN6(s2)->sin6_addr)) == 0)
				return (1);
			break;
#endif
		}
	}
	return (0);
}

void
select_host(struct sockaddr *a)
{
	int tabi;

	for (thishost = defhost->next; thishost; thishost = thishost->next) {
		if (addrequal(SA(&thishost->addr), a))
			break;
	}

	if (thishost == NULL)
		thishost = defhost;


	for (tabi = 0; tabi < ncmdtab; ++tabi)
		if (thishost->cmds[tabi] & CD)
			cmdtab[tabi].implemented = 0;
		else if (thishost->cmds[tabi] & GD)
			cmdtab[tabi].guestokay = 0;
		else if (thishost->cmds[tabi] & GE)
			cmdtab[tabi].guestokay = 1;

	for (tabi = 0; tabi < nsitetab; ++tabi)
		if (thishost->sitecmds[tabi] & CD)
			sitetab[tabi].implemented = 0;
		else if (thishost->sitecmds[tabi] & GD)
			sitetab[tabi].guestokay = 0;
		else if (thishost->sitecmds[tabi] & GE)
			sitetab[tabi].guestokay = 1;
}

int
config_parse(char *file)
{
	FILE *fp;
	char *line, *sline, *arg, *p, *path;
	size_t len;
	struct ftphost *vh, *ch, *lvh, dummyhost, *oldhosts;
	int isguest, lineno, tabi;
	struct tab *tab;
	struct fieldtab *ftab;
	int errors;
	u_long number, number2;
	struct passwd *pw;
	struct group *gr;
	struct ftpdir *fd;
	struct pidlist *pl;
	struct addrinfo req, *ai;

	if ((fp = fopen(file, "r")) == NULL)
		return (-1);

	if (defhost->flags & PARSED) {
		oldhosts = defhost;
		config_defhost();
	} else
		oldhosts = NULL;

	defhost->flags |= PARSED;

	vh = NULL;
	lvh = defhost;
	ch = defhost;
	isguest = 0;
	lineno = 0;
	errors = 0;

	while ((line = fgetln(fp, &len)) != NULL) {
		++lineno;
		if (line[len-1] == '\n')
			--len;
		line[len] = '\0';
		while (isspace(*line)) {
			++line;
			--len;
		}
		if ((p = strchr(line, '#')) == NULL)
			p = line + len;
		while (p > line && isspace(p[-1]))
			--p;
		*p = '\0';
		if (*line == '\0')
			continue;

		/*
		 * Look for the start or end of a block
		 * Anything after the first > is illegal
		 */
		if (*line == '<') {
			if ((p = strchr(++line, '>')) == NULL || p[1]) {
				if (p == NULL) {
					eprintf("no closing ``>''\n");
				} else {
					eprintf("garbage after ``>''\n");
				}
				goto syntax;
			}
			while (p > line && isspace(p[-1]))
				--p;
			*p = '\0';
			if (*line == '/') {
				if (isguest) {
					if (strcasecmp(line, "/guest") != 0) {
						eprintf("expected ``</Guest>''\n");
						goto syntax;
					}
					isguest = 0;
				} else if (vh) {
					if (strcasecmp(line,
					    "/virtualhost") != 0) {
						eprintf("expected ``</VirtualHost>''\n");
						goto syntax;
					}
					vh = NULL;
					ch = defhost;
				} else {
					eprintf("unexpected ``<%s>''\n", line);
					goto syntax;
				}
				continue;
			}
			if (strcasecmp(line, "guest") == 0) {
				if (isguest) {
					eprintf("cannot nest <Guest>\n");
					goto syntax;
				}
				isguest = 1;
			} else if (strncasecmp(line, "virtualhost", 11) == 0 &&
			    isspace(line[11])) {
				if (vh) {
					eprintf("cannot nest <VirtualHost>\n");
					goto syntax;
				}
				if (isguest) {
					eprintf("cannot nest <VirtualHost> in <Guest>\n");
					goto syntax;
				}
				line += 11;
				while (isspace(*line))
					++line;
				for (p = line; *p && !isspace(*p); ++p)
					;
				if (*p) {
					eprintf("unexpected arguments for <VirtualHost ...>\n");
					goto syntax;
				}
				memset(&req, 0, sizeof(struct addrinfo));

				req.ai_socktype = SOCK_STREAM;
				req.ai_flags |= AI_CANONNAME;

				if (getaddrinfo(line, NULL, &req, &ai) != 0) {
					eprintf("%s: cannot resolve hostname\n",
					    line);
					ch = &dummyhost;
					vh = ch;
					continue;
				}
				for (vh = defhost->next; vh; vh = vh->next) {
					if (memcmp(&vh->addr, ai->ai_addr,
					    ai->ai_addrlen) == 0)
						break;
				}

				if (vh == NULL) {
					if ((vh = malloc(ftphostsz)) == NULL)
						fatal("internal memory allocation failure");
					memcpy(vh, defhost, ftphostsz);
					vh->sitecmds = &vh->cmds[ncmdtab];
					lvh->next = vh;
					lvh = vh;
					vh->next = NULL;
					memcpy(&vh->addr, ai->ai_addr,
					    ai->ai_addrlen);
				} else
					eprintf("%s: multiple VirtualHost blocks at line %d\n", line, lineno);
				freeaddrinfo(ai);
				ch = vh;
				ch->hostname = sgetsave(line);
				ch->flags |= VIRTUAL;
			} else {
				eprintf("unexpected ``<%s>''\n", line);
				goto syntax;
			}
			continue;
		}
		if (lvh != defhost && vh == NULL) {
			eprintf("cannot have global defintions following virtual host definitions\n");
			goto syntax;
		}
		sline = line;

#define	NEXTARG do { arg = strsep(&sline, " \t"); } while (arg && *arg == '\0')

#define	GET_ONE_ARG	NEXTARG; \
			if (arg == NULL) { \
				eprintf("missing argument\n"); \
				goto syntax; \
			} \
			p = arg; \
			NEXTARG; \
			if (arg) { \
				eprintf("too many arguments\n"); \
				goto syntax; \
			} \
			arg = p;

		NEXTARG;

		if (arg != line)
			errx(1, "%s:%d: assertion failed", __FILE__, __LINE__);

		/*
		 * Figure out which command tab to look into
		 * There are two, the normal commands and the SITE commands
		 */
		if (strncmp(line, "SITE-", 5) == 0) {
			line += 5;
			tab = sitetab;
			tabi = ncmdtab;
		} else {
			tab = cmdtab;
			tabi = 0;
		}

		while (tab->name) {
			if (strcmp(line, tab->name) == 0)
				break;
			++tabi;
			++tab;
		}
		if (tab->name) {
			GET_ONE_ARG;
			if (strcasecmp(arg, "On") == 0) {
				if (tab->implemented == 0) {
					eprintf("%s: not implemented\n",
						    tab->name);
					continue;
				}
				tab->implemented = 1;
				ch->cmds[tabi] |= CE;
				ch->cmds[tabi] &= ~CD;
				if (isguest) {
					ch->cmds[tabi] |= GE;
					ch->cmds[tabi] &= ~GD;
				}
			} else if (strcasecmp(arg, "Off") == 0) {
				if (isguest) {
					ch->cmds[tabi] &= ~GE;
					ch->cmds[tabi] |= GD;
					continue;
				}
				if (tab->implemented == 0)
					continue;
				ch->cmds[tabi] &= ~CE;
				ch->cmds[tabi] |= CD;
			} else {
				eprintf("Expected ``On'' or ``Off''\n");
				goto syntax;
			}
			continue;
		}

		/* Figure out if we stripped SITE- and put it back on */

		if (sitetab > cmdtab) {
			if (tab > sitetab)
				line -= 5;
		} else {
			if (tab < cmdtab)
				line -= 5;
		}
		for (ftab = fieldtab; ftab->field; ++ftab)
			if (strcasecmp(line, ftab->field) == 0)
				break;
		if (ftab->field == NULL) {
			eprintf("%s: unknown field\n", line);
			goto syntax;
		}
		if (isguest && (ftab->flags & G) == 0) {
			eprintf("%s: not a guest setting\n", line);
			goto syntax;
		} else if (!isguest && (ftab->flags & U) == 0) {
			eprintf("%s: only a guest setting\n", line);
			goto syntax;
		}
		switch (ftab->type) {
		case STRING:
		case INCOMING:
		case REGEXPS:
			break;
		default:
			GET_ONE_ARG;
			break;
		}
		switch (ftab->type) {
		case EMAIL:
			number = strcspn(arg, ":/");
			if (arg[number]) {
				eprintf("email addresses may not contain ``:'' or ``/''\n");
				
				goto syntax;
			}
			if ((p = strchr(arg, '@')) == NULL) {
				eprintf("email address must have an ``@''\n");
				goto syntax;
			}
			if (strchr(p + 1, '@') != NULL) {
					printf(
"email address must have only one ``@''\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(arg);
			break;

		case HOSTNAME:
			number = strcspn(arg, ":/@");
			if (arg[number]) {
				eprintf("hostnames may not contain ``:'', ``/'', or ``@''\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(arg);
			break;

		case FILENAME:
			if (strchr(arg, '/')) {
				eprintf("filename cannot have a ``/''\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(arg);
			break;

		case PATHNAME:
		case DIRNAME:
			if (*arg != '/') {
				eprintf("pathname cannot be relative\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(arg);
			break;

		case USERNAME:
			number = strcspn(arg, "@:/");
			if (arg[number]) {
				eprintf("usernames may not contain ``:'', ``/'', or ``@''\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(arg);
			break;

		case ONOFF:
			if (ftab->result == O(flags)) {
				number = ftab->bit | (ftab->bit << GUESTSHIFT);
				if (isguest)
					number &= ~ftab->bit;
			} else
				number = ftab->bit;

			if (strcasecmp(arg, "On") == 0)
				R(ftab, u_long) |= number;
			else if (strcasecmp(arg, "Off") == 0)
				R(ftab, u_long) &= ~number;
			else {
				eprintf("Expected ``On'' or ``Off''\n");
				goto syntax;
			}
			break;

		case NUMBER:
			errno = 0;
			number = strtoul(arg, &p, 0);
			if (p == arg || *p != '\0' ||
			    (number == ULONG_MAX && errno == ERANGE)) {
				eprintf("expected a numeric value\n");
				goto syntax;
			}
			R(ftab, u_long) = number;
			break;

		case NUMBER8:
			errno = 0;
			number = strtoul(arg, &p, 8);
			if (p == arg || *p != '\0' ||
			    (number == ULONG_MAX && errno == ERANGE)) {
				eprintf("expected an octal value\n");
				goto syntax;
			}
			R(ftab, u_long) = number;
			break;

		case INCOMING:
			/*
			 * Incoming directory user group mode
			 */

			if (!isguest) {
				eprintf("Incoming directives are only valid for guest settings\n");
				goto syntax;
			}
			NEXTARG;
			if (!arg)
				goto syntax;
			
			if (*arg != '/') {
				eprintf("pathname cannot be relative\n");
				goto syntax;
			}
			path = arg;

			NEXTARG;
			if (!arg)
				goto syntax;
			if ((pw = getpwnam(arg)) == NULL) {
				eprintf("%s: no such user\n", arg);
				break;
			}
			NEXTARG;
			if (!arg)
				goto syntax;
			if ((gr = getgrnam(arg)) == NULL) {
				eprintf("%s: no such group\n", arg);
				break;
			}
			NEXTARG;
			if (!arg)
				goto syntax;
			errno = 0;
			number = strtoul(arg, &p, 8);
			if (p == arg || *p != '\0' ||
			    (number == ULONG_MAX && errno == ERANGE)) {
				eprintf("expected an octal value\n");
				goto syntax;
			}
			if (number > 0777) {
				eprintf("invalid mode\n");
				goto syntax;
			}
			NEXTARG;
			if (arg) {
				errno = 0;
				number2 = strtoul(arg, &p, 8);
				if (p == arg || *p != '\0' ||
				    (number2 == ULONG_MAX && errno == ERANGE)) {
					eprintf("expected an octal value\n");
					goto syntax;
				}
				if (number2 > 0777) {
					eprintf("invalid mode\n");
					goto syntax;
				}
				NEXTARG;
			} else
				number2 = 0;
			if (arg) {
				eprintf("too many arguments\n");
				goto syntax;
			}
			if ((fd = malloc(sizeof(*fd))) == NULL)
				fatal("internal memory allocation failure");
			fd->path = sgetsave(path);
			fd->dev = 0;
			fd->ino = 0;
			fd->uid = pw->pw_uid;
			fd->gid = gr->gr_gid;
			fd->mode = number;
			fd->dmode = number2;
			fd->next = ch->ftpdirs;
			ch->ftpdirs = fd;
			break;

		case STRING:
			while (isspace(*sline))
				++sline;
			if (*sline == '\\')
				++sline;
			if (*sline == '\0') {
				eprintf("invalid string\n");
				goto syntax;
			}
			R(ftab, char *) = sgetsave(sline);
			break;

		case REGEXPS:
			NEXTARG;
			while (arg) {
				char ebuf[128];

				struct regexps *re;
				if ((re = malloc(sizeof(*re))) == NULL)
                                	fatal("internal memory allocation failure");
re->pattern = sgetsave(arg);
				number = regcomp(&re->preg, arg,
				    REG_EXTENDED|REG_NOSUB);
				if (number) {
       					regerror(number, &re->preg, ebuf,
					    sizeof(ebuf));
					ebuf[sizeof(ebuf)-1] = '\0';
					eprintf("%s\n", ebuf);
					goto syntax;
				}
				if (R(ftab, struct regexps *) == NULL) {
					re->next = NULL;
					R(ftab, struct regexps *) = re;
				} else {
					re->next = R(ftab, struct regexps *)->next;
					R(ftab, struct regexps *)->next = re;
				}
				NEXTARG;
			}
			break;

		default:
			errx(1, "INTERNAL ERROR-Invalid field type");
		}
		continue;
syntax:
		eprintf("%s: syntax error on line %d\n", file, lineno);
		++errors;
	}
	if (isguest) {
		eprintf("expected ``</Guest>''\n");
		++errors;
	}
	if (vh) {
		eprintf("expected ``</VirtualHost>''\n");
		++errors;
	}
	if (oldhosts) {
		if (errors) {
			freehosts(defhost);
			defhost = oldhosts;
		} else {
			defhost->sessions = oldhosts->sessions;
			for (vh = defhost->next; vh; vh = vh->next) {
				for (lvh = oldhosts->next; lvh; lvh = lvh->next)
					if (memcmp(&vh->addr, &lvh->addr,
					    sizeof(vh->addr)) == 0) {
						vh->sessions = lvh->sessions;
						break;
					}
				for (pl = pidlist; pl; pl = pl->next)
					if (pl->host == vh)
						pl->host = lvh;
			}
			freehosts(oldhosts);
		}
	}
	return (errors);
}

void
freehosts(struct ftphost *fh)
{
	while (fh) {
		struct ftphost *n = fh->next;
		if (fh->chrootlist)
			free(fh->chrootlist);
		if (fh->banlist)
			free(fh->banlist);
		if (fh->permitlist)
			free(fh->permitlist);
		if (fh->groupfile)
			free(fh->groupfile);
		if (fh->anondir)
			free(fh->anondir);
		if (fh->anonuser)
			free(fh->anonuser);
		if (fh->hostname)
			free(fh->hostname);
		if (fh->loginmsg)
			free(fh->loginmsg);
		if (fh->statfile)
			free(fh->statfile);
		if (fh->welcome)
			free(fh->welcome);
		if (fh->logfmt)
			free(fh->logfmt);
		if (fh->tmplogfmt)
			free(fh->tmplogfmt);
		if (fh->msgfile)
			free(fh->msgfile);
		while (fh->pathfilter) {
			struct regexps *p = fh->pathfilter->next;
			regfree(&fh->pathfilter->preg);
			fh->pathfilter = p;
			free(p);
		}
		while (fh->ftpdirs) {
			struct ftpdir *p = fh->ftpdirs->next;
			if (fh->ftpdirs->path)
				free(fh->ftpdirs->path);
			free(fh->ftpdirs);
			fh->ftpdirs = p;
		}
		fh = n;
	}
}
