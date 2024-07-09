/*	BSDI man.c,v 2.6 2002/04/23 11:29:52 benoitp Exp	*/

/*
 * Copyright (c) 1987, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1987, 1993, 1994, 1995\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)man.c	8.17 (Berkeley) 1/31/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "pathnames.h"

int f_all, f_where;

static void	 build_page __P((char *, char **));
static void	 cat __P((char *));
static char	*check_pager __P((char *));
static int	 cleanup __P((void));
static void	 how __P((char *));
static void	 jump __P((char **, char *, char *));
static int	 manual __P((char *, TAG *, glob_t *));
static void	 onsig __P((int));
static void	 usage __P((void));
static void	 add_sections __P((char *, TAG *, TAG *, int));
#ifdef __CYGWIN__
static char	*convertposix __P((char *));
#else
#define convertposix(name)	name
#endif

static char	*machine;

#define	ATTAIL		0x00		/* Insert at tail of list */
#define	ATHEAD		0x01		/* Insert at head of list */
#define	ADDSLASH	0x02		/* Append "missing" slash */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	TAG *defp, *defnewp;
    	TAG *subp = NULL;
	TAG *tree;
	ENTRY *e_defp;
	ENTRY *e_tree;
	ENTRY *e_subp;
	glob_t pg;
	size_t len;
	int ch, f_cat, f_how, found;
    	char *secname = NULL;
	char **ap, *cmd, *p, *p_add, *p_path, *pager;
	char *conffile;

	f_cat = f_how = 0;
	conffile = p_add = p_path = NULL;
	while ((ch = getopt(argc, argv, "-aC:cfhkM:m:P:s:w")) != EOF)
		switch (ch) {
		case 'a':
			f_all = 1;
			break;
		case 'C':
			conffile = convertposix(optarg);
			break;
		case 'c':
		case '-':		/* Deprecated. */
			f_cat = 1;
			break;
		case 'h':
			f_how = 1;
			break;
		case 'm':
			p_add = convertposix(optarg);
			break;
		case 'M':
		case 'P':		/* Backward compatibility. */
			p_path = convertposix(optarg);
			break;
		case 's':
			secname = optarg;
			break;
		case 'w':
			f_all = f_where = 1;
			break;
		/*
		 * The -f and -k options are backward compatible,
		 * undocumented ways of calling whatis(1) and apropos(1).
		 */
		case 'f':
			jump(argv, "-f", "whatis");
			/* NOTREACHED */
		case 'k':
			jump(argv, "-k", "apropos");
			/* NOTREACHED */
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!*argv)
		usage();

	if (!f_cat && !f_how && !f_where)
		if (!isatty(1))
			f_cat = 1;
		else if ((pager = getenv("PAGER")) != NULL)
			pager = check_pager(pager);
		else
			pager = _PATH_PAGER;

	/* Read the configuration file. */
	config(conffile);

	/* Get the machine type. */
#ifdef TORNADO
	if ((machine = getenv("ARCH")) == NULL)
		machine = "foo";	/* don't know the arch of the target */
#else
	if ((machine = getenv("MACHINE")) == NULL)
		machine = MACHINE;
#endif

	/* If there's no _default list, create an empty one. */
	if ((defp = getlist("_default")) == NULL)
		defp = addlist("_default");

	/*
	 * If a section name was not specified by the -s option,
	 * and there are at least 2 arguments and the first argument
	 * starts with a digit, check to see if it matches any sections.
	 * If so, treat it as a section and not as a manual page name.
    	 * If no section was specified, use the _subdir list as the "section".
	 */
    	if (!secname && argv[1] && isdigit(argv[0][0]) &&
	    (subp = getlist(secname = *argv)))
		++argv;
    	else if (secname && !(subp = getlist(secname)))
		errx(1, "%s: no such section", secname);

    	if (!subp)
		subp = getlist("_subdir");

	e_subp = subp == NULL ?  NULL : subp->list.tqh_first;
	for (; e_subp != NULL; e_subp = e_subp->q.tqe_next)
		if (e_subp->s[0] == '/')
			errx(1, "%s: invalid section", secname);

	/*
	 * 1: If the user specified a MANPATH variable, or set the -M
	 *    option, we replace the _default list with the user's list,
	 *    appending the entries in the section list and the machine.
	 */
	if (p_path == NULL)
		p_path = convertposix(getenv("MANPATH"));
	if (p_path != NULL) {
		while ((e_defp = defp->list.tqh_first) != NULL) {
			free(e_defp->s);
			TAILQ_REMOVE(&defp->list, e_defp, q);
		}
		for (p = strtok(p_path, ":");
		    p != NULL; p = strtok(NULL, ":"))
			if (strchr(p, '/') == NULL && (tree = getlist(p)))
				for (e_tree = tree->list.tqh_first;
				    e_tree != NULL;
				    e_tree = e_tree->q.tqe_next)
					add_sections(e_tree->s, subp, defp,
					    ATTAIL);
			else
				add_sections(p, subp, defp, ATTAIL|ADDSLASH);
	}

	/*
	 * 2: If the user did not specify MANPATH or -M rewrite
	 *    the _default list to include the section list and the machine.
	 */
	if (p_path == NULL) {
		defnewp = addlist("_default_new");
		e_defp =
		    defp->list.tqh_first == NULL ? NULL : defp->list.tqh_first;
		for (; e_defp != NULL; e_defp = e_defp->q.tqe_next)
			add_sections(e_defp->s, subp, defnewp, ATTAIL);
		defp = getlist("_default");
		while ((e_defp = defp->list.tqh_first) != NULL) {
			free(e_defp->s);
			TAILQ_REMOVE(&defp->list, e_defp, q);
		}
		free(defp->s);
		TAILQ_REMOVE(&head, defp, q);
		defnewp = getlist("_default_new");
		free(defnewp->s);
		defnewp->s = "_default";
		defp = defnewp;
	}

	/*
	 * 3: If the user set the -m option, insert the user's list before
	 *    whatever list we have, again appending the section list and
	 *    the machine.
	 */
	if (p_add != NULL)
		for (p = strtok(p_add, ":"); p != NULL; p = strtok(NULL, ":"))
			if (strchr(p, '/') == NULL && (tree = getlist(p)))
				for (e_tree = tree->list.tqh_first;
				    e_tree != NULL;
				    e_tree = e_tree->q.tqe_next)
					add_sections(e_tree->s, subp, defp,
					    ATHEAD);
			else
				add_sections(p, subp, defp, ATHEAD|ADDSLASH);

	/*
	 * 4: Search for the files.  Set up an interrupt handler, so the
	 *    temporary files go away.
	 */
	(void)signal(SIGINT, onsig);
	(void)signal(SIGHUP, onsig);

	memset(&pg, 0, sizeof(pg));
	for (found = 0; *argv; ++argv)
		if (manual(*argv, defp, &pg))
			found = 1;

	/* 5: If nothing found, we're done. */
	if (!found) {
		(void)cleanup();
		exit (1);
	}

	/* 6: If it's simple, display it fast. */
	if (f_cat) {
		for (ap = pg.gl_pathv; *ap != NULL; ++ap) {
			if (**ap == '\0')
				continue;
			cat(*ap);
		}
		exit (cleanup());
	}
	if (f_how) {
		for (ap = pg.gl_pathv; *ap != NULL; ++ap) {
			if (**ap == '\0')
				continue;
			how(*ap);
		}
		exit(cleanup());
	}
	if (f_where) {
		for (ap = pg.gl_pathv; *ap != NULL; ++ap) {
			if (**ap == '\0')
				continue;
			(void)printf("%s\n", *ap);
		}
		exit(cleanup());
	}
		
	/*
	 * 7: We display things in a single command; build a list of things
	 *    to display.
	 */
	for (ap = pg.gl_pathv, len = strlen(pager) + 1; *ap != NULL; ++ap) {
		if (**ap == '\0')
			continue;
		len += strlen(*ap) + 1;
	}
	if ((cmd = malloc(len)) == NULL) {
		warn(NULL);
		(void)cleanup();
		exit(1);
	}
	p = cmd;
	len = strlen(pager);
	memmove(p, pager, len);
	p += len;
	*p++ = ' ';
	for (ap = pg.gl_pathv; *ap != NULL; ++ap) {
		if (**ap == '\0')
			continue;
		len = strlen(*ap);
		memmove(p, *ap, len);
		p += len;
		*p++ = ' ';
	}
	*p = '\0';

	/* Use system(3) in case someone's pager is "pager arg1 arg2". */
	(void)system(cmd);

	exit(cleanup());
}

void
add_sections(p, subp, defp, flag)
	char *p;
	TAG *subp, *defp;
	int flag;
{
	ENTRY *e_subp;
	ENTRY *ep;
	char *slashp, buf[MAXPATHLEN * 2];

	slashp = p[strlen(p) - 1] == '/' ? "" : "/";

	/*
	 * If the entry does not end in a slash and we are not
	 * adding slashes, don't append subdirectories.
    	 */
	if (*slashp && !(flag & ADDSLASH)) {
		(void)snprintf(buf, sizeof(buf), "%s{/%s,}", p, machine);
		if ((ep = malloc(sizeof(ENTRY))) == NULL ||
		    (ep->s = strdup(buf)) == NULL)
			err(1, NULL);
		if (flag & ATHEAD) {
			TAILQ_INSERT_HEAD(&defp->list, ep, q);
		} else {
			TAILQ_INSERT_TAIL(&defp->list, ep, q);
		}
		return;
    	}

	e_subp = subp == NULL ?  NULL : subp->list.tqh_first;

	for (; e_subp != NULL; e_subp = e_subp->q.tqe_next) {
		if (e_subp->s[0] == '/')
			continue;

		(void)snprintf(buf, sizeof(buf), "%s%s%s{/%s,}",
		    p, slashp, e_subp->s, machine);
		if ((ep = malloc(sizeof(ENTRY))) == NULL ||
		    (ep->s = strdup(buf)) == NULL)
			err(1, NULL);
		if (flag & ATHEAD) {
			TAILQ_INSERT_HEAD(&defp->list, ep, q);
		} else {
			TAILQ_INSERT_TAIL(&defp->list, ep, q);
		}
	}
}

/*
 * manual --
 *	Search the manuals for the pages.
 */
static int
manual(arg, tag, pg)
	char *arg;
	TAG *tag;
	glob_t *pg;
{
	ENTRY *ep, *e_sufp, *e_tag;
	TAG *missp, *sufp;
	int anyfound, cnt, found;
	char *p, *t, buf[MAXPATHLEN], page[MAXPATHLEN];

#define	ismagic(c) \
	((c) == '*' || (c) == '?' || (c) == '[' || (c) == '\\')

	/* Escape any magic characters in the man page name. */
	for (p = arg, t = page;;) {
		if (ismagic(*p))
			*t++ = '\\';
		if ((*t++ = *p++) == '\0')
			break;
	}

	anyfound = 0;
	buf[0] = '*';

	/* For each element in the list... */
	e_tag = tag == NULL ? NULL : tag->list.tqh_first;
	for (; e_tag != NULL; e_tag = e_tag->q.tqe_next) {
		(void)snprintf(buf, sizeof(buf), "%s/%s.*", e_tag->s, page);
		if (glob(buf,
		    GLOB_APPEND | GLOB_BRACE | GLOB_NOSORT | GLOB_QUOTE,
		    NULL, pg)) {
			warn("globbing");
			(void)cleanup();
			exit(1);
		}
		if (pg->gl_matchc == 0)
			continue;

		/* Find out if it's really a man page. */
		for (cnt = pg->gl_pathc - pg->gl_matchc;
		    cnt < pg->gl_pathc; ++cnt) {

			/*
			 * Try the _suffix key words first.
			 *
			 * XXX
			 * Older versions of man.conf didn't have the suffix
			 * key words, it was assumed that everything was a .0.
			 * We just test for .0 first, it's fast and probably
			 * going to hit.
			 */
			(void)snprintf(buf, sizeof(buf), "*/%s.0", page);
			if (!fnmatch(buf, pg->gl_pathv[cnt], 0))
				goto next;

			e_sufp = (sufp = getlist("_suffix")) == NULL ?
			    NULL : sufp->list.tqh_first;
			for (found = 0;
			    e_sufp != NULL; e_sufp = e_sufp->q.tqe_next) {
				(void)snprintf(buf,
				     sizeof(buf), "*/%s%s", page, e_sufp->s);
				if (!fnmatch(buf, pg->gl_pathv[cnt], 0)) {
					found = 1;
					break;
				}
			}
			if (found)
				goto next;

			/* Try the _build key words next. */
			e_sufp = (sufp = getlist("_build")) == NULL ?
			    NULL : sufp->list.tqh_first;
			for (found = 0;
			    e_sufp != NULL; e_sufp = e_sufp->q.tqe_next) {
				for (p = e_sufp->s;
				    *p != '\0' && !isspace(*p); ++p);
				if (*p == '\0')
					continue;
				*p = '\0';
				(void)snprintf(buf,
				     sizeof(buf), "*/%s%s", page, e_sufp->s);
				if (!fnmatch(buf, pg->gl_pathv[cnt], 0)) {
					if (!f_where)
						build_page(p + 1,
						    &pg->gl_pathv[cnt]);
					*p = ' ';
					found = 1;
					break;
				}
				*p = ' ';
			}
			if (found) {
next:				anyfound = 1;
				if (!f_all) {
					/* Delete any other matches. */
					while (++cnt< pg->gl_pathc)
						pg->gl_pathv[cnt] = "";
					break;
				}
				continue;
			}

			/* It's not a man page, forget about it. */
			pg->gl_pathv[cnt] = "";
		}

		if (anyfound && !f_all)
			break;
	}

	/* If not found, enter onto the missing list. */
	if (!anyfound) {
		if ((missp = getlist("_missing")) == NULL)
			missp = addlist("_missing");
		if ((ep = malloc(sizeof(ENTRY))) == NULL ||
		    (ep->s = strdup(arg)) == NULL) {
			warn(NULL);
			(void)cleanup();
			exit(1);
		}
		TAILQ_INSERT_TAIL(&missp->list, ep, q);
	}
	return (anyfound);
}

/* 
 * build_page --
 *	Build a man page for display.
 */
static void
build_page(fmt, pathp)
	char *fmt, **pathp;
{
	static int warned;
	ENTRY *ep;
	TAG *intmpp;
	int fd;
	char buf[MAXPATHLEN], cmd[MAXPATHLEN], tpath[sizeof(_PATH_TMP)];

	/* Let the user know this may take awhile. */
	if (!warned) {
		warned = 1;
		warnx("Formatting manual page...");
	}

	/* Add a remove-when-done list. */
	if ((intmpp = getlist("_intmp")) == NULL)
		intmpp = addlist("_intmp");

	/* Move to the printf(3) format string. */
	for (; *fmt && isspace(*fmt); ++fmt);

	/*
	 * Get a temporary file and build a version of the file
	 * to display.  Replace the old file name with the new one.
	 */
	(void)strcpy(tpath, _PATH_TMP);
	if ((fd = mkstemp(tpath)) == -1) {
		warn("%s", tpath);
		(void)cleanup();
		exit(1);
	}
	(void)snprintf(buf, sizeof(buf), "%s > %s", fmt, tpath);
	(void)snprintf(cmd, sizeof(cmd), buf, *pathp);
	(void)system(cmd);
	(void)close(fd);
	if ((*pathp = strdup(tpath)) == NULL) {
		warn(NULL);
		(void)cleanup();
		exit(1);
	}

	/* Link the built file into the remove-when-done list. */
	if ((ep = malloc(sizeof(ENTRY))) == NULL) {
		warn(NULL);
		(void)cleanup();
		exit(1);
	}
	ep->s = *pathp;
	TAILQ_INSERT_TAIL(&intmpp->list, ep, q);
}

/*
 * how --
 *	display how information
 */
static void
how(fname)
	char *fname;
{
	FILE *fp;

	int lcnt, print;
	char *p, buf[256];

	if (!(fp = fopen(fname, "r"))) {
		warn("%s", fname);
		(void)cleanup();
		exit (1);
	}
#define	S1	"SYNOPSIS"
#define	S2	"S\bSY\bYN\bNO\bOP\bPS\bSI\bIS\bS"
#define	D1	"DESCRIPTION"
#define	D2	"D\bDE\bES\bSC\bCR\bRI\bIP\bPT\bTI\bIO\bON\bN"
	for (lcnt = print = 0; fgets(buf, sizeof(buf), fp);) {
		if (!strncmp(buf, S1, sizeof(S1) - 1) ||
		    !strncmp(buf, S2, sizeof(S2) - 1)) {
			print = 1;
			continue;
		} else if (!strncmp(buf, D1, sizeof(D1) - 1) ||
		    !strncmp(buf, D2, sizeof(D2) - 1))
			return;
		if (!print)
			continue;
		if (*buf == '\n')
			++lcnt;
		else {
			for(; lcnt; --lcnt)
				(void)putchar('\n');
			for (p = buf; isspace(*p); ++p);
			(void)fputs(p, stdout);
		}
	}
	(void)fclose(fp);
}

/*
 * cat --
 *	cat out the file
 */
static void
cat(fname)
	char *fname;
{
	int fd, n;
	char buf[2048];

	if ((fd = open(fname, O_RDONLY, 0)) < 0) {
		warn("%s", fname);
		(void)cleanup();
		exit(1);
	}
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		if (write(STDOUT_FILENO, buf, n) != n) {
			warn("write");
			(void)cleanup();
			exit (1);
		}
	if (n == -1) {
		warn("read");
		(void)cleanup();
		exit(1);
	}
	(void)close(fd);
}

/*
 * check_pager --
 *	check the user supplied page information
 */
static char *
check_pager(name)
	char *name;
{
	char *p, *save;

	name = convertposix(name);

	/*
	 * if the user uses "more", we make it "more -s"; watch out for
	 * PAGER = "mypager /usr/ucb/more"
	 */
	for (p = name; *p && !isspace(*p); ++p);
	for (; p > name && *p != '/'; --p);
	if (p != name)
		++p;

	/* make sure it's "more", not "morex" */
	if (!strncmp(p, "more", 4) && (!p[4] || isspace(p[4]))){
		save = name;
		/* allocate space to add the "-s" */
		if (!(name =
		    malloc((u_int)(strlen(save) + sizeof("-s") + 1))))
			err(1, NULL);
		(void)sprintf(name, "%s %s", save, "-s");
	}
	return(name);
}

/*
 * jump --
 *	strip out flag argument and jump
 */
static void
jump(argv, flag, name)
	char **argv, *flag, *name;
{
	char **arg;

	argv[0] = name;
	for (arg = argv + 1; *arg; ++arg)
		if (!strcmp(*arg, flag))
			break;
	for (; *arg; ++arg)
		arg[0] = arg[1];
	execvp(name, argv);
	(void)fprintf(stderr, "%s: Command not found.\n", name);
	exit(1);
}

/* 
 * onsig --
 *	If signaled, delete the temporary files.
 */
static void
onsig(signo)
	int signo;
{
	(void)cleanup();

	(void)signal(signo, SIG_DFL);
	(void)kill(getpid(), signo);

	/* NOTREACHED */
	exit (1);
}

/*
 * cleanup --
 *	Clean up temporary files, show any error messages.
 */
static int
cleanup()
{
	TAG *intmpp, *missp;
	ENTRY *ep;
	int rval;

	rval = 0;
	ep = (missp = getlist("_missing")) == NULL ?
	    NULL : missp->list.tqh_first;
	if (ep != NULL)
		for (; ep != NULL; ep = ep->q.tqe_next) {
			warnx("no entry for %s in the manual.", ep->s);
			rval = 1;
		}

	ep = (intmpp = getlist("_intmp")) == NULL ?
	    NULL : intmpp->list.tqh_first;
	for (; ep != NULL; ep = ep->q.tqe_next)
		(void)unlink(ep->s);
	return (rval);
}

#ifdef __CYGWIN__
/* 
 * Convert a possible win32 path to a posix path. The size of <posixname>
 * must be big enough to hold the resulting posix pathname.
 */

static char *
convertposix(name)
	char *name;
{
	char buf[MAXPATHLEN*2];	/* should be enough */
	char *posixname;

	if (name == NULL)
		return NULL;
	
	cygwin_conv_to_posix_path(name, buf);
	
	posixname = malloc(strlen(buf) + 1);
	if (posixname == NULL)
	    err (1, NULL);

	strcpy (posixname, buf);
	return posixname;
}
#endif

/*
 * usage --
 *	print usage message and die
 */
static void
usage()
{
	(void)fprintf(stderr,
    "usage: man [-achw] [-C file] [-M path] [-m path] [[-s] section] title ...\n");
	exit(1);
}
