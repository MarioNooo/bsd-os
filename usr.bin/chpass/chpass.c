/*	BSDI chpass.c,v 2.6 1998/09/10 19:40:30 torek Exp	*/

/*-
 * Copyright (c) 1988, 1993, 1994
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
"@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)chpass.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <db.h>

#include <libpasswd.h>

#include "chpass.h"
#include "pathnames.h"

char *progname = "chpass";
char tempname[] = "/tmp/chpass.XXXXXX";
uid_t uid;

void	baduser __P((void));
void	onlyone __P((void));
void	usage __P((void));
int	delete __P((char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	enum { NEWSH, LOADENTRY, EDITENTRY, DELETENTRY } op;
	struct passwd *opw, *pw, lpw;
	int ch, tfd;
	char *arg, *e;

	op = EDITENTRY;
	while ((ch = getopt(argc, argv, "a:d:s:")) != EOF)
		switch (ch) {
		case 'a':
			if (op != EDITENTRY)
				onlyone();
			op = LOADENTRY;
			arg = optarg;
			break;
		case 'd':
			if (op != EDITENTRY)
				onlyone();
			op = DELETENTRY;
			arg = optarg;
			break;
		case 's':
			if (op != EDITENTRY)
				onlyone();
			op = NEWSH;
			arg = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	uid = getuid();

	opw = 0;
	if (op == EDITENTRY || op == NEWSH) {
		switch(argc) {
		case 0:
			if ((pw = getpwuid(uid)) == NULL)
				errx(1, "unknown user: uid %u", uid);
			break;
		case 1:
			if ((pw = getpwnam(*argv)) == NULL)
				errx(1, "unknown user: %s", *argv);
			if (uid && uid != pw->pw_uid)
				baduser();
			break;
		default:
			usage();
		}
		opw = pw_copy(pw);
		pw = pw_copy(pw);
		if (opw == NULL || pw == NULL)
			errx(1, "out of memory");
    	}

	if (op == NEWSH) {
		/* protect p_shell -- it thinks NULL is /bin/sh */
		if (!arg[0])
			usage();
		if (p_shell(arg, pw, NULL))
			err(1, NULL);
	}

	if (op == LOADENTRY) {
		if (uid)
			baduser();
		pw = &lpw;
		if ((e = pw_split(arg, pw, 1)) != NULL) {
			err(1, "%s", e);
			exit(1);
		}
		if ((opw = getpwnam(pw->pw_name)) != NULL &&
		    (opw = pw_copy(opw)) == NULL)
			err(1, NULL);
	}

	if (op == DELETENTRY) {
		if (uid)
			baduser();
		if ((pw = getpwnam(arg)) == NULL)
			errx(1, "unknown user: %s", arg);
		endpwent();
		exit (delete(pw->pw_name));
	}

	if (op == EDITENTRY) {
		tfd = mkstemp(tempname);
		if (tfd < 0)
			err(1, "%s", tempname);
		display(tfd, tempname, pw);
		edit(tempname, pw);
		(void)unlink(tempname);
	}
	endpwent();			/* Close up the database */

	e = pwd_update(opw, pw, PW_WARNROOT);
	if (e)
		errx(1, "%s", e);
	exit(0);
}

void
baduser()
{

	errx(1, "%s", strerror(EACCES));
}

void
onlyone()
{
	errx(1, "only one of the -a, -d and -s options may be specified");
}

void
usage()
{

	(void)fprintf(stderr,
	    "usage: chpass [-a list] [-d account] [-s shell] [user]\n");
	exit(1);
}


struct pwinfo pw;
char tmppath[MAXPATHLEN] = { 0 };

void
cleanup(sig)
	int sig;
{

	pw_abort(&pw);
	pw_unlock(&pw);
	if (tmppath[0])
		unlink(tmppath);

	/*
	 * XXX:
	 * This shoud be using strsignal(3), but BSD/OS doesn't have
	 * it yet.
	 */
	errx(1, "signal: %s: no changes made",
	    (u_int)sig >= NSIG ?  "(unknown)" : sys_siglist[sig]);
}

int
delete(account)
	char *account;
{
	struct sigaction act;
	FILE *fp, *tp;
	sigset_t sigs;
	size_t alen, len;
	int tfd, pfd;
	char *p;

    	pw_init(&pw, PW_WARNROOT | PW_MAKEOLD);

    	act.sa_handler = cleanup;
    	act.sa_flags = 0;
	pw_sigs(&sigs, &act);

    	pw_unlimit();
    	if ((pfd = pw_lock(&pw, O_NONBLOCK)) < 0) {
	    	if (errno == EWOULDBLOCK)
			errx(1, "password file already locked");
		err(1, "locking password file");
    	}

	tfd = pwd_tmp(&pw, tmppath, MAXPATHLEN, &p);
	if (p)
		errx(1, "%s", p);
	(void)fchmod(tfd, PW_PERM_SECURE);

	if ((fp = fdopen(pfd, "r")) == NULL) {
		pw_abort(&pw);
		unlink(tmppath);
		err(1, "%s", _PATH_MASTERPASSWD);
	}
	if ((tp = fdopen(tfd, "w")) == NULL) {
		pw_abort(&pw);
		unlink(tmppath);
		err(1, "%s", tmppath);
	}

	alen = strlen(account);
	while ((p = fgetln(fp, &len)) != NULL)
		if (len <= alen ||
		    p[alen] != ':' || memcmp(p, account, alen) != 0)
			if (fwrite(p, sizeof(*p), len, tp) == EOF) {
				pw_abort(&pw);
				unlink(tmppath);
				err(1, "%s", _PATH_MASTERPASSWD);
			}

	if (ferror(fp)) {
		pw_abort(&pw);
		unlink(tmppath);
		err(1, "%s", tmppath);
	}
	fclose(tp);

	if ((p = pwd_rebuild(&pw, tmppath, 2048)) != NULL) {
		warnx("%s", p);
		warnx("no changes made");
		pw_abort(&pw);
		pw_unlock(&pw);
		unlink(tmppath);
		exit(0);
	}
	(void)sigprocmask(SIG_BLOCK, &sigs, NULL);
    	if ((p = pwd_install(&pw)) != NULL) {
		warnx("%s", p);
		err(1, "failed to install %s", tmppath);
	}
	return (0);
}
