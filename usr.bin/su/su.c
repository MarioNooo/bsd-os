/*	BSDI	su.c,v 2.16 2000/03/22 18:06:10 donn Exp	*/

/*
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
static char sccsid[] = "@(#)su.c	8.3 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <login_cap.h>
#include <db.h>
#include <libpasswd.h>

#define	ARGSTR	"-fKlm"
#define	OARGSTR	"a:c:"

int	chshell __P((char *));
char   *ontty __P((void));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char **environ;
	enum { UNSET, YES, NO } iscsh;
	struct passwd *mpwd, *pwd;
	struct group *gr;
	uid_t ruid;
	login_cap_t *lc;
	int asme, asthem, authok, ch, fastlogin, prio;
	char *class, *style, *p, *q, **g;
	char *user, *shell, *cleanenv, **nargv, **np, *fullname;

	iscsh = UNSET;
	class = style = NULL;
	asme = asthem = fastlogin = 0;
	while ((ch = getopt(argc, argv, ARGSTR OARGSTR)) != EOF)
		switch (ch) {
		case 'a':
			if (style)
				usage();
			style = optarg;
			break;
		case 'c':
			if (class)
				usage();
			class = optarg;
			break;
		case 'f':
			fastlogin = 1;
			break;
		case 'K':
			if (style)
				usage();
			style = LOGIN_DEFSTYLE;
			break;
		case '-':
		case 'l':
			asme = 0;
			asthem = 1;
			break;
		case 'm':
			asme = 1;
			asthem = 0;
			break;
		case '?':
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (errno)
		prio = 0;
	(void)setpriority(PRIO_PROCESS, 0, -2);
	openlog("su", LOG_CONS, 0);

	/* get current login name and shell */
	ruid = getuid();
	user = getlogin();
	if (user == NULL || (mpwd = getpwnam(user)) == NULL ||
	    mpwd->pw_uid != ruid)
		mpwd = getpwuid(ruid);
	if (mpwd == NULL)
		errx(1, "who are you?");
	if ((mpwd = pw_copy(mpwd)) == NULL)
		err(1, "duping passwd entry");

	/* get target login information, default to root */
	user = *argv ? *argv++ : "root";
	if ((pwd = getpwnam(user)) == NULL)
		errx(1, "unknown login %s", user);
	if ((pwd = pw_copy(pwd)) == NULL)
		err(1, "duping passwd entry");

	if (ruid && class)
		errx(1, "-c only available for super user");

	/* force the usage of specified class */
	if (class)
		pwd->pw_class = class;
	if ((lc = login_getclass(pwd->pw_class)) == NULL)
		errx(1, "could not retrieve requested class information");

	if (asme)
		if (mpwd->pw_shell && *mpwd->pw_shell)
			shell = mpwd->pw_shell;
		else {
			shell = _PATH_BSHELL;
			iscsh = NO;
		}

	if (ruid) {
		/*
		 * If we are trying to become root and the default style
		 * is being used, don't bother to look it up (we might be
		 * be su'ing up to fix /etc/login.conf)
		 */
		if ((pwd->pw_uid || !style || strcmp(style, LOGIN_DEFSTYLE)) &&
		    (style = login_getstyle(lc, style, "auth-su")) == NULL)
			errx(1, "invalid authentication type");
		if (pwd->pw_uid || strcmp(user, "root") != 0)
			fullname = user;
		else {
			if ((fullname =
			    malloc(strlen(mpwd->pw_name) + 6)) == NULL)
				err(1, NULL);
			(void)sprintf(fullname, "%s.root", mpwd->pw_name);
		}
		/*
		 * Let the authentication program know if they are in
		 * group wheel or not (if trying to become super user)
		 */
		if (pwd->pw_uid == 0 && mpwd->pw_gid != 0 &&
		    (gr = getgrgid((gid_t)0))) {
			for (g = gr->gr_mem; *g != NULL; ++g)
				if (strcmp(mpwd->pw_name, *g) == 0) {
					auth_setopt("wheel", "yes");
					break;
				}
			if (*g == NULL)
				auth_setopt("wheel", "no");
		}
		authok = auth_check(fullname, lc->lc_class, style, 0, NULL);

		if (authok == 0) {
			if ((p = auth_value("errormsg")) != NULL)
				fprintf(stderr, "%s\n", p);
			fprintf(stderr, "Sorry\n");
			syslog(LOG_AUTH|LOG_WARNING,
			    "BAD SU %s to %s%s", mpwd->pw_name, user, ontty());
			exit(1);
		}
	}

	if (asme) {
		/* if asme and non-standard target shell, must be root */
		if (!chshell(pwd->pw_shell) && ruid)
			errx(1, "%s:  permission denied", pwd->pw_shell);
	} else if (pwd->pw_shell && *pwd->pw_shell) {
		shell = pwd->pw_shell;
		iscsh = UNSET;
	} else {
		shell = _PATH_BSHELL;
		iscsh = NO;
	}

	/* if we're forking a csh, we want to slightly muck the args */
	if (iscsh == UNSET) {
		if (p = strrchr(shell, '/'))
			++p;
		else
			p = shell;
		iscsh = strcmp(p, "csh") ? NO : YES;
	}


	if (!asme) {
		if (asthem) {
			p = getenv("TERM");
			cleanenv = NULL;
			environ = &cleanenv;
			if (setusercontext(lc,
			    pwd, pwd->pw_uid, LOGIN_SETPATH))
				err(1, "setting user context");
			if (p != NULL)
				(void)setenv("TERM", p, 1);
			if (chdir(pwd->pw_dir) < 0)
				err(1, "%s", pwd->pw_dir);
		} else if (pwd->pw_uid == 0)
			if (setusercontext(lc,
			    pwd, pwd->pw_uid, LOGIN_SETPATH|LOGIN_SETUMASK))
				err(1, "setting path");
		if (asthem || pwd->pw_uid) {
			(void)setenv("USER", pwd->pw_name, 1);
			(void)setenv("LOGNAME", pwd->pw_name, 1);
		}
		(void)setenv("HOME", pwd->pw_dir, 1);
		(void)setenv("SHELL", shell, 1);
	}
	auth_env();

	if ((nargv = malloc((argc + 4) * sizeof (char *))) == NULL)
		err(1, NULL);

	np = nargv;

	/* csh strips the first character... */
	*np++ = asthem ? "-su" : iscsh == YES ? "_su" : "su";

	if (iscsh == YES) {
		if (fastlogin)
			*np++ = "-f";
		if (asme)
			*np++ = "-m";
	}

	while (*np++ = *argv++)
		;

	if (ruid != 0)
		syslog(LOG_NOTICE|LOG_AUTH, "%s to %s%s",
		    mpwd->pw_name, user, ontty());

	(void)setpriority(PRIO_PROCESS, 0, prio);
	if (setusercontext(lc, pwd, pwd->pw_uid,
	    (asthem ? (LOGIN_SETPRIORITY | LOGIN_SETUMASK) : 0) |
	    LOGIN_SETRESOURCES | LOGIN_SETGROUP | LOGIN_SETUSER))
		err(1, "setting user context");
	if (pwd->pw_uid && auth_approve(lc, pwd->pw_name, "su") <= 0)
		err(1, "approval failure");

	execv(shell, nargv);
	err(1, "%s", shell);
}

int
chshell(sh)
	char *sh;
{
	char *cp;

	while ((cp = getusershell()) != NULL)
		if (strcmp(cp, sh) == 0)
			return (1);
	return (0);
}

char *
ontty()
{
	static char buf[MAXPATHLEN + 4];
	char *p;

	buf[0] = 0;
	if (p = ttyname(STDERR_FILENO))
		snprintf(buf, sizeof(buf), " on %s", p);
	return (buf);
}

void
usage()
{
	(void)fprintf(stderr,
    "usage: su [%s] [-a auth-type] [-c login-class] [login [argument ...]]\n",
	    ARGSTR);
	exit(1);
}
