/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)docmd.c	8.1 (Berkeley) 6/9/93";
#endif /* not lint */

#include "defs.h"
#include <setjmp.h>
#include <netdb.h>

FILE	*lfp;			/* log file for recording files updated */
struct	subcmd *subcmds;	/* list of sub-commands for current cmd */
jmp_buf	env;

static int	 makeconn __P((char *));
static int	 okname __P((char *));
static void	 closeconn __P((void));
static void	 cmptime __P((char *));
static void	 doarrow __P((char **,
		    struct namelist *, char *, struct subcmd *));
static void	 dodcolon __P((char **,
		    struct namelist *, char *, struct subcmd *));
static void	 notify __P((char *, char *, struct namelist *, time_t));
static void	 rcmptime __P((struct stat *));

/*
 * Do the commands in cmds (initialized by yyparse).
 */
void
docmds(dhosts, argc, argv)
	char **dhosts;
	int argc;
	char **argv;
{
	register struct cmd *c;
	register struct namelist *f;
	register char **cpp;
	extern struct cmd *cmds;

	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);

	for (c = cmds; c != NULL; c = c->c_next) {
		if (dhosts != NULL && *dhosts != NULL) {
			for (cpp = dhosts; *cpp; cpp++)
				if (strcmp(c->c_name, *cpp) == 0)
					goto fndhost;
			continue;
		}
	fndhost:
		if (argc) {
			for (cpp = argv; *cpp; cpp++) {
				if (c->c_label != NULL &&
				    strcmp(c->c_label, *cpp) == 0) {
					cpp = NULL;
					goto found;
				}
				for (f = c->c_files; f != NULL; f = f->n_next)
					if (strcmp(f->n_name, *cpp) == 0)
						goto found;
			}
			continue;
		} else
			cpp = NULL;
	found:
		switch (c->c_type) {
		case ARROW:
			doarrow(cpp, c->c_files, c->c_name, c->c_cmds);
			break;
		case DCOLON:
			dodcolon(cpp, c->c_files, c->c_name, c->c_cmds);
			break;
		default:
			fatal("illegal command type %d\n", c->c_type);
		}
	}
	closeconn();
}

/*
 * Process commands for sending files to other machines.
 */
static void
doarrow(filev, files, rhost, cmds)
	char **filev;
	struct namelist *files;
	char *rhost;
	struct subcmd *cmds;
{
	register struct namelist *f;
	register struct subcmd *sc;
	register char **cpp;
	int n, ddir, opts = options;

	if (debug)
		printf("doarrow(%p, %s, %p)\n", files, rhost, cmds);

	if (files == NULL) {
		error("no files to be updated\n");
		return;
	}

	subcmds = cmds;
	ddir = files->n_next != NULL;	/* destination is a directory */
	if (nflag)
		printf("updating host %s\n", rhost);
	else {
		if (setjmp(env))
			goto done;
		signal(SIGPIPE, lostconn);
		if (!makeconn(rhost))
			return;
		if ((lfp = fopen(tempfile, "w")) == NULL) {
			fatal("cannot open %s\n", tempfile);
			exit(1);
		}
	}
	for (f = files; f != NULL; f = f->n_next) {
		if (filev) {
			for (cpp = filev; *cpp; cpp++)
				if (strcmp(f->n_name, *cpp) == 0)
					goto found;
			if (!nflag) {
				(void) fclose(lfp);
				lfp = NULL;
			}
			continue;
		}
	found:
		n = 0;
		for (sc = cmds; sc != NULL; sc = sc->sc_next) {
			if (sc->sc_type != INSTALL)
				continue;
			n++;
			install(f->n_name, sc->sc_name,
				sc->sc_name == NULL ? 0 : ddir, sc->sc_options);
			opts = sc->sc_options;
		}
		if (n == 0)
			install(f->n_name, NULL, 0, options);
	}
done:
	if (!nflag) {
		(void) signal(SIGPIPE, cleanup);
		if (lfp != NULL) {
			(void) fclose(lfp);
			lfp = NULL;
		}
	}
	for (sc = cmds; sc != NULL; sc = sc->sc_next)
		if (sc->sc_type == NOTIFY)
			notify(tempfile, rhost, sc->sc_args, 0);
	if (!nflag) {
		register struct linkbuf *ihp;

		(void) unlink(tempfile);
		for (; (ihp = ihead) != NULL; ihead = ihp->nextp, free(ihp)) {
			if ((opts & IGNLNKS) || ihead->count == 0)
				continue;
			log(lfp, "%s: Warning: missing links\n",
				ihead->pathname);
		}
	}
}

/*
 * Create a connection to the rdist server on the machine rhost.
 */
static int
makeconn(remote)
	char *remote;
{
	int cpid, n, rc, sv[2];
	char *cp, **ap, **argv, *rsh_path;
	char *ruser, *rhost;
	char buf[MAXPATHLEN];
	static char *cur_host = NULL;

	rc = 0;
	argv = NULL;
	rsh_path = NULL;

	if (debug)
		printf("makeconn(%s)\n", remote);

	/* check for a cached connection */
	if (cur_host != NULL && remr >= 0) {
		if (strcasecmp(cur_host, remote) == 0)
			return(1);
		closeconn();
	}
	cur_host = remote;

	/* Parse user@host syntax */
	ruser = NULL;
	if ((rhost = strchr(remote, '@')) != NULL) {
		*rhost = 0;
		if ((ruser = strdup(remote)) == NULL) {
			warn("strdup");
			goto escape;
		}
		*rhost++ = '@';
		if (*ruser == 0)
			ruser = user;
		else if (!okname(ruser))
			goto escape;
	} else {
		ruser = user;
		rhost = remote;
	}

	if (!qflag)
		printf("updating host %s\n", rhost);

	fflush(stdout);

	/* Build an argument list for rsh */

	/* Get a writable copy of the rsh path */
	if ((rsh_path = strdup(path_rsh)) == NULL) {
		warn("strdup");
		goto escape;
	}

	/* Get a rough estimate of how many vectors we need */
	n = 10 + Kflag + ((kflag != NULL) * 2) + Nflag + xflag + qflag;
	for (cp = path_rsh; *cp != '\0'; cp++)
		if (isspace(*cp))
			n++;

	if ((ap = argv = malloc(n * sizeof(*argv))) == NULL) {
		warn("malloc");
		goto escape;
	}

	/* First parse the rsh command into vectors */
	cp = rsh_path;
	for (ap = argv + 1; (*ap = strsep(&cp, " \t")) != NULL;)
		if (**ap != '\0')
			++ap;

	/* Add kerberos related options */
	if (Kflag)
		*ap++ = "-K";
	if (kflag != NULL) {
		*ap++ = "-k";
		*ap++ = kflag;
	}
	if (xflag)
		*ap++ = "-x";
	if (Nflag)
		*ap++ = "-N";

	/* Add user, host and the command to run */
	*ap++ = "-l";
	*ap++ = ruser;
	*ap++ = rhost;
	*ap++ = _PATH_RDIST;
	*ap++ = "-Server";
	if (qflag)
		*ap++ = "-q";
	*ap++ = NULL;

	if (ap - argv > n)
		errx(1, "argument vector size error");

	/* Fix argv[0] to be the full path and argv[1] to be the command */
	ap = argv;
	*ap++ = argv[1];
	if ((cp = strchr(*ap, '/')) != NULL)
		*ap = cp + 1;

	if (debug) {
		printf("makeconn: command:");
		for (ap = argv; *ap != NULL; ap++) {
			printf("%s ", *ap);
		}
		printf("\n");
	}

	/* get a socketpair we'll use for stdin and stdout. */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		warn("socketpair");
		goto escape;
	}

	cpid = fork();
	if (cpid < 0) {
		warn("fork");
		goto escape;
	}
	if (cpid == 0) {
		/* child. use sv[1] to be stdin/stdout, and close sv[0]. */
		(void)close(sv[0]);
		if (dup2(sv[1], STDIN_FILENO) < 0 || 
		    dup2(STDIN_FILENO, STDOUT_FILENO) < 0)
			_exit(255);
		/* fork again to lose parent. */
		cpid = fork();
		if (cpid < 0)
			_exit(255);
		if (cpid > 0)
			_exit(0);
		/* in grandchild here. */
		execvp(argv[0], &argv[1]);
		_exit(255);
	}

	/* parent. close sv[1], return sv[0]. */
	(void)close(sv[1]);
	/* reap child. */
	(void)wait(NULL);

	remr = remw = sv[0];

	cp = buf;
	if ((n = read(remr, cp, 1)) != 1) {
		if (n < 0)
			warn("read failed");
		(void)close(remr);
		(void)close(remw);
		remr = remw = -1;
		goto escape;
#ifdef notdef
		lostconn(0);
#endif
	}
	if (*cp == 'V') {
		do {
			if (read(remr, cp, 1) != 1)
				lostconn(0);
		} while (*cp++ != '\n' && cp < buf + sizeof(buf));
		*--cp = '\0';
		cp = buf;
		n = 0;
		while (*cp >= '0' && *cp <= '9')
			n = (n * 10) + (*cp++ - '0');
		if (*cp == '\0' && n == VERSION) {
			rc = 1;
			goto escape;
		}
		error("connection failed: version numbers don't match (local %d, remote %d)\n", VERSION, n);
	} else {
		char ebuf[1024];

		*ebuf = *cp;
		cp = ebuf + 1;
		do {
			if (read(remr, cp, 1) != 1)
				break;
		} while (*cp++ != '\n' && cp < ebuf + sizeof(ebuf));
		*--cp = '\0';
		if (*ebuf)
			error("got unexpected input: %s\n", ebuf);
		else
		     error("connection failed: version numbers don't match\n");

	}
	closeconn();

 escape:
	if (ruser != NULL && ruser != user)
		free(ruser);
	if (argv != NULL)
		free(argv);
	if (rsh_path != NULL)
		free(rsh_path);
	return(rc);
}

/*
 * Signal end of previous connection.
 */
static void
closeconn()
{
	if (debug)
		printf("closeconn()\n");

	if (remr >= 0) {
		(void) write(remw, "\2\n", 2);
		(void) close(remw);
		(void) close(remr);
		remr = remw = -1;
	}
}

void
lostconn(signo)
	int signo;
{
	if (iamremote)
		cleanup(0);
	(void) close(remr);
	(void) close(remw);
	remr = remw = -1;
	log(lfp, "rdist: lost connection\n");
	longjmp(env, 1);
}

static int
okname(name)
	register char *name;
{
	register char *cp = name;
	register int c;

	do {
		c = *cp;
		if (c & 0200)
			goto bad;
		if (!isalpha(c) && !isdigit(c) && c != '_' && c != '-')
			goto bad;
		cp++;
	} while (*cp);
	return(1);
bad:
	error("invalid user name %s\n", name);
	return(0);
}

time_t	lastmod;
FILE	*tfp;

/*
 * Process commands for comparing files to time stamp files.
 */
static void
dodcolon(filev, files, stamp, cmds)
	char **filev;
	struct namelist *files;
	char *stamp;
	struct subcmd *cmds;
{
	register struct subcmd *sc;
	register struct namelist *f;
	register char **cpp;
	struct timeval tv[2];
	struct timezone tz;
	struct stat stb;

	if (debug)
		printf("dodcolon()\n");

	if (files == NULL) {
		error("no files to be updated\n");
		return;
	}
	if (stat(stamp, &stb) < 0) {
		error("%s: %s\n", stamp, strerror(errno));
		return;
	}
	if (debug)
		printf("%s: %ld\n", stamp, stb.st_mtime);

	subcmds = cmds;
	lastmod = stb.st_mtime;
	if (nflag || (options & VERIFY))
		tfp = NULL;
	else {
		if ((tfp = fopen(tempfile, "w")) == NULL) {
			error("%s: %s\n", stamp, strerror(errno));
			return;
		}
		(void) gettimeofday(&tv[0], &tz);
		tv[1] = tv[0];
		(void) utimes(stamp, tv);
	}

	for (f = files; f != NULL; f = f->n_next) {
		if (filev) {
			for (cpp = filev; *cpp; cpp++)
				if (strcmp(f->n_name, *cpp) == 0)
					goto found;
			continue;
		}
	found:
		tp = NULL;
		cmptime(f->n_name);
	}

	if (tfp != NULL)
		(void) fclose(tfp);
	for (sc = cmds; sc != NULL; sc = sc->sc_next)
		if (sc->sc_type == NOTIFY)
			notify(tempfile, NULL, sc->sc_args, lastmod);
	if (!nflag && !(options & VERIFY))
		(void) unlink(tempfile);
}

/*
 * Compare the mtime of file to the list of time stamps.
 */
static void
cmptime(name)
	char *name;
{
	struct stat stb;

	if (debug)
		printf("cmptime(%s)\n", name);

	if (except(name))
		return;

	if (nflag) {
		printf("comparing dates: %s\n", name);
		return;
	}

	/*
	 * first time cmptime() is called?
	 */
	if (tp == NULL) {
		if (exptilde(target, name) == NULL)
			return;
		tp = name = target;
		while (*tp)
			tp++;
	}
	if (access(name, 4) < 0 || stat(name, &stb) < 0) {
		error("%s: %s\n", name, strerror(errno));
		return;
	}

	switch (stb.st_mode & S_IFMT) {
	case S_IFREG:
		break;

	case S_IFDIR:
		rcmptime(&stb);
		return;

	default:
		error("%s: not a plain file\n", name);
		return;
	}

	if (stb.st_mtime > lastmod)
		log(tfp, "new: %s\n", name);
}

static void
rcmptime(st)
	struct stat *st;
{
	register DIR *d;
	register struct direct *dp;
	register char *cp;
	char *otp;
	int len;

	if (debug)
		printf("rcmptime(%p)\n", st);

	if ((d = opendir(target)) == NULL) {
		error("%s: %s\n", target, strerror(errno));
		return;
	}
	otp = tp;
	len = tp - target;
	while ((dp = readdir(d)) != NULL) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (len + 1 + strlen(dp->d_name) >= sizeof(target) - 1) {
			error("%s/%s: Name too long\n", target, dp->d_name);
			continue;
		}
		tp = otp;
		*tp++ = '/';
		cp = dp->d_name;
		while ((*tp++ = *cp++) != '\0')
			;
		tp--;
		cmptime(target);
	}
	closedir(d);
	tp = otp;
	*tp = '\0';
}

/*
 * Notify the list of people the changes that were made.
 * rhost == NULL if we are mailing a list of changes compared to at time
 * stamp file.
 */
static void
notify(file, rhost, to, lmod)
	char *file, *rhost;
	register struct namelist *to;
	time_t lmod;
{
	struct stat stb;
	FILE *pf;
	int fd, len;
	char buf[MAXPATHLEN + 50];

	if ((options & VERIFY) || to == NULL)
		return;
	if (!qflag) {
		printf("notify ");
		if (rhost)
			printf("@%s ", rhost);
		prnames(to);
	}
	if (nflag)
		return;

	if ((fd = open(file, 0)) < 0) {
		error("%s: %s\n", file, strerror(errno));
		return;
	}
	if (fstat(fd, &stb) < 0) {
		error("%s: %s\n", file, strerror(errno));
		(void) close(fd);
		return;
	}
	if (stb.st_size == 0) {
		(void) close(fd);
		return;
	}
	/*
	 * Create a pipe to mailling program.
	 */
	(void)snprintf(buf, sizeof(buf), "%s -oi -t", _PATH_SENDMAIL);
	pf = popen(buf, "w");
	if (pf == NULL) {
		error("notify: \"%s\" failed\n", _PATH_SENDMAIL);
		(void) close(fd);
		return;
	}
	/*
	 * Output the proper header information.
	 */
	fprintf(pf, "From: rdist (Remote distribution program)\n");
	fprintf(pf, "To:");
	if (!any('@', to->n_name) && rhost != NULL)
		fprintf(pf, " %s@%s", to->n_name, rhost);
	else
		fprintf(pf, " %s", to->n_name);
	to = to->n_next;
	while (to != NULL) {
		if (!any('@', to->n_name) && rhost != NULL)
			fprintf(pf, ", %s@%s", to->n_name, rhost);
		else
			fprintf(pf, ", %s", to->n_name);
		to = to->n_next;
	}
	putc('\n', pf);
	if (rhost != NULL)
		fprintf(pf, "Subject: files updated by rdist from %s to %s\n",
			host, rhost);
	else
		fprintf(pf, "Subject: files updated after %s\n", ctime(&lmod));
	putc('\n', pf);

	while ((len = read(fd, buf, sizeof(buf))) > 0)
		(void) fwrite(buf, 1, len, pf);
	(void) close(fd);
	(void) pclose(pf);
}

/*
 * Return true if name is in the list.
 */
int
inlist(list, file)
	struct namelist *list;
	char *file;
{
	register struct namelist *nl;

	for (nl = list; nl != NULL; nl = nl->n_next)
		if (!strcmp(file, nl->n_name))
			return(1);
	return(0);
}

/*
 * Return TRUE if file is in the exception list.
 */
int
except(file)
	char *file;
{
	register struct	subcmd *sc;
	register struct	namelist *nl;

	if (debug)
		printf("except(%s)\n", file);

	for (sc = subcmds; sc != NULL; sc = sc->sc_next) {
		switch (sc->sc_type) {
		case EXCEPT:
			for (nl = sc->sc_args; nl != NULL; nl = nl->n_next)
				if (!strcmp(file, nl->n_name))
					return(1);
			break;

		case PATTERN:
			for (nl = sc->sc_args; nl != NULL; nl = nl->n_next) {
		        	int err;
				char ebuf[1024];

				err = regexec(nl->n_preg, file, 0, NULL, 0);
				switch (err) {
				case 0:
					return 1;

				case REG_NOMATCH:
					break;

				default:
					(void) regerror(err, nl->n_preg, ebuf, sizeof ebuf);
					error("except: %s\n", ebuf);
					break;
				}
			}
			break;

		default:
			break;
		}
	}
	return(0);
}

char *
colon(cp)
	register char *cp;
{

	while (*cp) {
		if (*cp == ':')
			return(cp);
		if (*cp == '/')
			return(0);
		cp++;
	}
	return(0);
}
