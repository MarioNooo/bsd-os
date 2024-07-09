/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login.c,v 2.36 2003/06/18 16:49:55 polk Exp
 */
/*-
 * Copyright (c) 1980, 1987, 1988, 1991, 1993, 1994
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
"@(#) Copyright (c) 1980, 1987, 1988, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)login.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */

/*
 * login [ name ]
 * login -h hostname	(for telnetd, etc.)
 * login -f name	(for pre-authenticated login: datakit, xterm, etc.)
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libdialer.h>
#include <login_cap.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <tzfile.h>
#include <unistd.h>
#include <utmp.h>
#include <bsd_auth.h>

#include "pathnames.h"
#include "extern.h"

void	 badlogin __P((char *));
void	 dolastlog __P((int));
void	 getloginname __P((void));
char	*getttyauth __P((char *));
void	 motd __P((void));
void	 printbanner __P((char *, char *));
void	 quickexit __P((int));
int	 rootterm __P((char *));
void	 sigint __P((int));
void	 sleepexit __P((int));
char	*stypeof __P((char *));
void	 timedout __P((int));
void	 hungup __P((int));

extern void login __P((struct utmp *));
extern int isppp __P((char));
extern int check_license __P((char *, int));

#define	TTYGRPNAME	"tty"		/* name of group to own ttys */

/*
 * This bounds the time given to login.  Not a define so it can
 * be patched on machines where it's too small.
 */
u_int	timeout = 300;

struct	passwd *pwd;
int	failures;
char	*envinit[1], *hostname, *username, *tty;

auth_session_t	*as = NULL;
login_cap_t	*lc = NULL;
char		*style;
char		*PP = "ppp_direct";
char		*PL = _PATH_PPP;
int		needbanner = 1;
ttydesc_t	*td;

#define	MAGIC_PPP	"!}!}\"}"

#ifndef	INET6
#define	INET6	1
#endif

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char **environ;
	struct group *gr;
	struct hostent *hp;
	struct in_addr in;
#if	INET6
	struct in6_addr in6;
#endif
	struct rlimit cds, scds;
	struct stat st;
	struct timeval tp;
	struct utmp utmp;
	uid_t uid;
	int ask, authok, ch, cnt, fflag, pflag, quietlog, rootlogin, lastchance;
	int needto, rechdir;
	char *domain, *p, *ttyn;
	char *copyright, *fqdn, *fullname, *instance, *lipaddr, *script;
	char *ripaddr, *shell, *style, *type;
	char localhost[MAXHOSTNAMELEN];
	char tbuf[MAXPATHLEN + 2];
	cap_t *c;
	quad_t expire, warning;

	(void)signal(SIGALRM, timedout);
	if (argc > 1) {
		needto = 0;
		(void)alarm(timeout);
	} else
		needto = 1;

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

	fqdn = lipaddr = ripaddr = fullname = NULL;

    	if (getrlimit(RLIMIT_CORE, &scds) < 0) {
		syslog(LOG_ERR, "couldn't get core dump size: %m");
		scds.rlim_cur = QUAD_MIN;
		scds.rlim_max = QUAD_MIN;
	}
	cds.rlim_cur = 0;
	cds.rlim_max = 0;
    	if (setrlimit(RLIMIT_CORE, &cds) < 0) {
		syslog(LOG_ERR, "couldn't set core dump size to 0: %m");
		scds.rlim_cur = QUAD_MIN;
		scds.rlim_max = QUAD_MIN;
	}

	/*
	 * If preserving the environment, delete ENV.  The problem is that
	 * if we exec a POSIX 1003.2 shell script as the user's shell (e.g.,
	 * false or nologin), the ENV commands will be executed giving the
	 * user a chance to execute arbitrary commands on the host.  Delete
	 * BASH_ENV, because people replace /bin/sh with bash.  Delete IFS
	 * on general principles, in case the shell script is badly written.
	 */
	unsetenv("BASH_ENV");
	unsetenv("ENV");
	unsetenv("IFS");

	/*
	 * -p is used by getty to tell login not to destroy the environment
	 * -f is used to skip a second login authentication
	 * -h is used by other servers to pass the name of the remote
	 *    host to login so that it may be placed in utmp and wtmp
	 */
	domain = NULL;
	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");
	else
		domain = strchr(localhost, '.');

	if ((as = auth_open()) == NULL) {
		syslog(LOG_ERR, "%m");
		err(1, NULL);
	}

	fflag = pflag = 0;
	uid = getuid();
	while ((ch = getopt(argc, argv, "Bfh:L:pR:")) != EOF)
		switch (ch) {
		case 'B':
			needbanner = 0;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
			if (uid) {
				warnx("-h option: %s", strerror(EPERM));
				quickexit(1);
			}
			if ((fqdn = strdup(optarg)) == NULL) {
				warn(NULL);
				quickexit(1);
			}
			auth_setoption(as, "fqdn", fqdn);
			if (domain && (p = strchr(optarg, '.')) &&
			    strcasecmp(p, domain) == 0)
				*p = 0;
			hostname = optarg;
			auth_setoption(as, "hostname", hostname);
			break;
		case 'L':
			if (uid) {
				warnx("-L option: %s", strerror(EPERM));
				quickexit(1);
			}
			if (lipaddr) {
				warnx("duplicate -L option");
				quickexit(1);
			}
			lipaddr = optarg;

			if (inet_aton(lipaddr, &in) &&
			    (hp = gethostbyaddr((char *)&in, 4, AF_INET)))
				strncpy(localhost, hp->h_name, MAXHOSTNAMELEN);
			else
#if	INET6
			if (inet_pton(AF_INET6, lipaddr, &in6) &&
			    (hp = gethostbyaddr((char *)&in6, 16, AF_INET6)))
				strncpy(localhost, hp->h_name, MAXHOSTNAMELEN);
			else
#endif
				strncpy(localhost, lipaddr, MAXHOSTNAMELEN);
			localhost[sizeof(localhost)-1] = 0;
			auth_setoption(as, "local_addr", lipaddr);
			break;
		case 'p':
			pflag = 1;
			break;
		case 'R':
			if (uid) {
				warnx("-R option: %s", strerror(EPERM));
				quickexit(1);
			}
			if (ripaddr) {
				warnx("duplicate -R option");
				quickexit(1);
			}
			ripaddr = optarg;
			auth_setoption(as, "remote_addr", ripaddr);
			break;
		case '?':
		default:
			if (!uid)
				syslog(LOG_ERR, "invalid flag %c", ch);
			fprintf(stderr,
"usage: login [-fp] [-h hostname] [-L lipaddr] [-R ripaddr] [username]");
			quickexit(1);
		}
	argc -= optind;
	argv += optind;

	if (*argv) {
		username = *argv;
		ask = 0;
	} else
		ask = 1;

	ttyn = ttyname(STDIN_FILENO);
	if (ttyn == NULL || *ttyn == '\0') {
		(void)snprintf(tbuf, sizeof(tbuf), "%s??", _PATH_TTY);
		ttyn = tbuf;
	}
	if ((tty = strrchr(ttyn, '/')) == NULL)
		tty = ttyn;
	else
		++tty;

	td = getttydescbyname(tty);

	if (td) {
		if ((c = cap_look(td->ty_cap, "ppp")) && c->value != NULL)
			PP = c->value;
		if ((c = cap_look(td->ty_cap, "pppname")) && c->value != NULL)
			PL = c->value;
	}

	/*
	 * Classify the attempt.
	 * By default we use the value in the ttys file.
	 * If there is a classify script we run that as
	 *
	 *	classify [-f] [username]
	 */
	if (type = getttyauth(tty))
		auth_setoption(as, "auth_type", type);

	/* get the default login class */
	if ((lc = login_getclass(0)) == NULL) {	/* get the default class */ 
		warnx("Failure to retrieve default class");
		quickexit(1);
	}
	if ((script = login_getcapstr(lc, "classify", NULL, NULL)) != NULL) {
		unsetenv("AUTH_TYPE");
		unsetenv("REMOTE_NAME");
		if (script[0] != '/') {
			syslog(LOG_ERR, "Invalid classify script: %s", script);
			warnx("Classification failure");
			quickexit(1);
		}
		shell = strrchr(script, '/') + 1;
		auth_setstate(as, AUTH_OKAY);
		auth_call(as, script, shell, 
		    fflag ? "-f" : username, fflag ? username : 0, 0);
		if (!(auth_getstate(as) & AUTH_ALLOW))
			quickexit(1);
		auth_setenv(as);
		if ((p = getenv("AUTH_TYPE")) != NULL &&
		    strncmp(p, "auth-", 5) == 0)
			type = p;
		if ((p = getenv("REMOTE_NAME")) != NULL)
			hostname = p;
		/*
		 * we may have changed some values, reset them
		 */
		auth_clroptions(as);
		if (type)
			auth_setoption(as, "auth_type", type);
		if (fqdn)
			auth_setoption(as, "fqdn", fqdn);
		if (hostname)
			auth_setoption(as, "hostname", hostname);
		if (lipaddr)
			auth_setoption(as, "local_addr", lipaddr);
		if (ripaddr)
			auth_setoption(as, "remote_addr", ripaddr);
	}

	/*
	 * We also need to check if we were called with the "magic" PPP
	 * user name.
	 */
	if (!ask && strncmp(username, MAGIC_PPP, sizeof(MAGIC_PPP)-1) == 0) {
		auth_clean(as);
		auth_close(as);
		setlogin(PP);
		execl(PL, "ppp", 0);
		err(1, PL);
	}

	printbanner(tty, localhost);

	/*
	 * Request the things like the approval script print things
	 * to stdout (in particular, the nologins files)
	 */
	auth_setitem(as, AUTHV_INTERACTIVE, "True");
  

	for (cnt = 0;; ask = 1) {
		/*
		 * Clean up our current authentication session.
		 * Options are not cleared so we need to clear any
		 * we might set below.
		 */
		auth_clean(as);
		auth_clroption(as, "style");
		auth_clroption(as, "lastchance");

		lastchance = 0;

		if (ask) {
			fflag = 0;
			getloginname();
		}
		if (needto) {
			needto = 0;
			alarm(timeout);
		}
    	    	if ((style = strchr(username, ':')) != NULL)
			*style++ = '\0';
		if (fullname)
			free(fullname);
		if (auth_setitem(as, AUTHV_NAME, username) < 0 ||
		    (fullname = strdup(username)) == NULL) {
			syslog(LOG_ERR, "%m");
			warn(NULL);
			quickexit(1);
		}
		rootlogin = 0;
		if ((instance = strchr(username, '.')) != NULL) {
			if (strncmp(instance, ".root", 5) == 0)
				rootlogin = 1;
			*instance++ = '\0';
		} else
			instance = "";

		if (strlen(username) > UT_NAMESIZE)
			username[UT_NAMESIZE] = '\0';

		/*
		 * Note if trying multiple user names; log failures for
		 * previous user name, but don't bother logging one failure
		 * for nonexistent name (mistyped username).
		 */
		if (failures && strcmp(tbuf, username)) {
			if (failures > (pwd ? 0 : 1))
				badlogin(tbuf);
			failures = 0;
		}
		(void)strcpy(tbuf, username);

		if ((pwd = getpwnam(username)) != NULL &&
		    auth_setpwd(as, pwd) < 0) {
			syslog(LOG_ERR, "%m");
			warn(NULL);
			quickexit(1);
		}

		lc = login_getclass(pwd ? pwd->pw_class : NULL);

		if (!lc)
			goto failed;

		style = login_getstyle(lc, style, type);

		if (!style)
			goto failed;

		/*
		 * Turn off the fflag if we have an an invalid user
		 * or we are not root and we are trying to change uids.
		 */
		if (!pwd || (uid && uid != pwd->pw_uid))
			fflag = 0;

		/*
		 * If we do not have the force flag authenticate the user
		 */
		if (fflag)
			authok = AUTH_SECURE;
		else {
			lastchance =
			    login_getcaptime(lc, "password-dead", 0, 0) != 0;
			if (lastchance)
				auth_setoption(as, "lastchance", "yes");
			/*
			 * Once we start asking for a password
			 *  we want to log a failure on a hup.
			 */
			signal(SIGHUP, hungup);
			auth_verify(as, style, NULL, lc->lc_class, NULL);
			authok = auth_getstate(as);
			/*
			 * If their password expired and it has not been
			 * too long since then, give the user one last
			 * chance to change their password
			 */
			if ((authok & AUTH_PWEXPIRED) && lastchance) {
				authok = AUTH_OKAY;
			} else
				lastchance = 0;
			if ((authok & AUTH_ALLOW) == 0)
				goto failed;
			if (auth_setoption(as, "style", style) < 0) {
				syslog(LOG_ERR, "%m");
				warn(NULL);
				quickexit(1);
			}
		}
		/*
		 * explicitly reject users without password file entries
		 */
		if (pwd == 0)
			goto failed;

		if (pwd && pwd->pw_uid == 0)
			rootlogin = 1;

		authok &= AUTH_SECURE;

		/*
		 * If trying to log in as root on an insecure terminal,
		 * refuse the login attempt unless the authentication
		 * style explicitly says a root login is okay.
		 */
		if (authok == 0 && pwd && rootlogin && !rootterm(tty)) {
			warnx("%s login refused on this terminal.",
			    fullname);
			if (hostname)
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED FROM %s ON TTY %s",
				    fullname, hostname, tty);
			else
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED ON TTY %s",
				     fullname, tty);
			continue;
		}
		if (fflag) {
			type = 0;
			style = "forced";
		}
		break;

failed:
		if (authok & AUTH_SILENT)
			quickexit(0);
		if (as == NULL || (p = auth_getvalue(as, "errormsg")) == NULL)
			p = "Login incorrect";
		(void)printf("%s\n", p);
		failures++;
		/* we allow 10 tries, but after 3 we start backing off */
		if (++cnt > 3) {
			if (cnt >= 10) {
				badlogin(username);
				sleepexit(1);
			}
			sleep((u_int)((cnt - 3) * 5));
		}
	}

	/* committed to login -- turn off timeout */
	(void)alarm((u_int)0);

	endpwent();

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;


	/* Destroy the environment unless its preservation was requested. */
	if (!pflag)
		environ = envinit;
	(void)setenv("HOME", pwd->pw_dir, 1);
	(void)setenv("LOGNAME", pwd->pw_name, 1);
	(void)setenv("SHELL", pwd->pw_shell, 1);
	(void)setenv("TERM", stypeof(tty), 0);
	(void)setenv("USER", pwd->pw_name, 1);

	ttysetdev(ttyname(0), pwd->pw_uid, pwd->pw_gid);

	if (setusercontext(lc, pwd, pwd->pw_uid, LOGIN_SETPATH)) {
		warn("setting user context");
		quickexit(1);
	}
	auth_setenv(as);

	/* if user not super-user, check for disabled logins */
	if (!rootlogin)
		auth_checknologin(lc);

	rechdir = chdir(pwd->pw_dir) < 0;

	if (rechdir) {
		if (login_getcapbool(lc, "requirehome", 0)) {
			(void)printf("No home directory %s!\n", pwd->pw_dir);
			quickexit(1);
		}
		if (chdir("/"))
			quickexit(0);
	}

	if (!(quietlog = login_getcapbool(lc, "hushlogin", 0)))
		quietlog = access(_PATH_HUSHLOGIN, F_OK) == 0;

	if ((p = auth_getvalue(as, "warnmsg")) != NULL)
		(void)printf("WARNING: %s\n\n", p);

	expire = auth_check_expire(as);
	if (expire < 0) {
		(void)printf("Sorry -- your account has expired.\n");
		quickexit(1);
	} else if (expire > 0 && !quietlog) {
		warning = login_getcaptime(lc, "expire-warn",
		    2 * DAYSPERWEEK * SECSPERDAY, 2 * DAYSPERWEEK * SECSPERDAY);
		if (expire < warning)
			(void)printf("Warning: your account expires on %s",
			    ctime(&pwd->pw_expire));
	}

	/* Nothing else left to fail -- really log in. */
	memset((void *)&utmp, 0, sizeof(utmp));
	utmp.ut_time = time((time_t *)NULL);
	(void)strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
	if (hostname)
		(void)strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
	(void)strncpy(utmp.ut_line, tty, sizeof(utmp.ut_line));
	login(&utmp);

	dolastlog(quietlog);

	(void)chown(ttyn, pwd->pw_uid,
	    (gr = getgrnam(TTYGRPNAME)) ? gr->gr_gid : pwd->pw_gid);

	if (tty[sizeof("tty")-1] == 'd')
		syslog(LOG_INFO, "DIALUP %s, %s", tty, pwd->pw_name);

	/* If fflag is on, assume caller/authenticator has logged root login. */
	if (rootlogin && fflag == 0)
		if (hostname)
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s FROM %s",
			    username, tty, hostname);
		else
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s", username, tty);

	if (!quietlog) {
		(void)printf("Copyright 2001, 2002, 2003 Wind River Systems, Inc.\n");
		(void)printf("%s\n\t%s\n",
	    "Copyright 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001",
		    "Berkeley Software Design, Inc.");
		(void)printf("%s\n\t%s %s\n\n",
	    "Copyright (c) 1980, 1983, 1986, 1988, 1990, 1991, 1993, 1994",
		    "The Regents of the University of California. ",
		    "All rights reserved.");
		if ((copyright =
		    login_getcapstr(lc, "copyright", NULL, NULL)) != NULL)
			auth_cat(copyright);
		motd();
		(void)snprintf(tbuf,
		    sizeof(tbuf), "%s/%s", _PATH_MAILDIR, pwd->pw_name);
		if (stat(tbuf, &st) == 0 && st.st_size != 0)
			(void)printf("You have %smail.\n",
			    (st.st_mtime > st.st_atime) ? "new " : "");
	}

	(void)signal(SIGALRM, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGHUP, SIG_DFL);
	(void)signal(SIGTSTP, SIG_DFL);

	shell = login_getcapstr(lc, "shell", pwd->pw_shell, pwd->pw_shell);
    	if (strlen(shell) > MAXPATHLEN) {
		syslog(LOG_ERR, "shell path too long");
		warnx("invalid shell");
		quickexit(1);
	}

	tbuf[0] = '-';
	(void)strcpy(tbuf + 1, (p = strrchr(shell, '/')) ? p + 1 : shell);

	if ((scds.rlim_cur != QUAD_MIN || scds.rlim_max != QUAD_MIN) &&
    	    setrlimit(RLIMIT_CORE, &scds) < 0)
		syslog(LOG_ERR, "couldn't reset core dump size to 0: %m");

	if (lastchance)
		(void)printf("WARNING: Your password has expired.  You must change your password, now!\n");

	if (setusercontext(lc, pwd,
	    rootlogin ? 0 : pwd->pw_uid, LOGIN_SETALL & ~LOGIN_SETPATH)) {
		warn("setting user context"); 
		quickexit(1);
	}

	if (rechdir && chdir(pwd->pw_dir) < 0) {
		(void)printf("No home directory %s!\n", pwd->pw_dir);
		(void)printf("Logging in with home = \"/\".\n");
		(void)setenv("HOME", "/", 1);
	}

	if (auth_approval(as, lc, NULL, "login") == 0) {
		if (auth_getstate(as) & AUTH_EXPIRED)
			(void)printf("Sorry -- your account has expired.\n");
		else
			(void)printf("approval failure\n");
		quickexit(1);
	}

	if (check_license(pwd->pw_name, quietlog)) {
		syslog(LOG_ERR, "%s: exceeded user limit", pwd->pw_name);
		printf("\nWARNING: this session exceeds the licensed user limit!\n\n");
		if (login_getcapbool(lc, "harduserlimit", 0))
			quickexit(1);
	}

	/*
	 * The last thing we do is discard all of the open file descriptors.
	 * Last because the C library may have some open.
	 *
	 * XXX
	 * Assume that stdin, stdout and stderr are 0, 1 and 2, and that
	 * STDERR_FILENO is 2.
	 */
	for (cnt = getdtablesize(); cnt > STDERR_FILENO; cnt--)
		(void)close(cnt);

	/*
	 * Close the authentication session, make sure it is marked
	 * as okay so no files are removed.
	 */
	auth_setstate(as, AUTH_OKAY);
	auth_close(as);
	as = NULL;
	execlp(shell, tbuf, 0);
	warn("%s", shell);
	quickexit(1);
}

/*
 * Allow for a '.' and 16 characters for any instance as well as
 * space for a ':' and 16 charcters defining the authentication type.
 */
#define	NBUFSIZ		(UT_NAMESIZE + 1 + 16 + 1 + 16)

void
getloginname()
{
	struct termios told, tnew;
	static char nbuf[NBUFSIZ];
	char ch;
	int r;
	char *p;

	isppp(0);	/* force isppp to reset itself */

	/*
	 * Sigh.  Because of the need to watch for PPP LCP sequences
	 * we need to turn off input processing and do it ourself.
	 */
	tcgetattr(0, &told);
	tnew = told;
    	tnew.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(0, TCSANOW, &tnew);
	for (;;) {
		(void)printf("login: ");
		fflush(stdout);
		for (p = nbuf; (r = read(STDIN_FILENO, &ch, 1)) == 1 &&
		    ch != told.c_cc[VEOL] && ch != told.c_cc[VEOL2] &&
		    ch != '\r' && ch != '\n';) {
			if (isppp(ch)) {
				setlogin(PP);
				execl(PL, "ppp", NULL);
				tcsetattr(0, TCSANOW, &told);
				warn("%s", PL);
				quickexit(1);
			} else if (ch == told.c_cc[VEOF])
				break;
			else if (ch == told.c_cc[VINTR])
				kill(0, SIGINT);
			else if (ch == told.c_cc[VQUIT])
				kill(0, SIGQUIT);
			else if (ch == told.c_cc[VSUSP])
				kill(0, SIGTSTP);
			else if (ch == told.c_cc[VERASE]) {
				if (p > nbuf) {
					p--;
					if (cfgetospeed(&told) >= B1200)
						printf("\b \b");
					else
						printf("%c", ch);
				}
				fflush(stdout);
			} else if (ch == told.c_cc[VKILL]) {
				printf("%c\r", ch);
				if (cfgetospeed(&told) < B1200)
					printf("\n");
				/* this is the way they do it down under ... */
				else if (p > nbuf)
					printf("                                     \r");
				p = nbuf;
				(void)printf("login: ");
				fflush(stdout);
			} else {
				printf("%c", ch);
				fflush(stdout);
				if (p < nbuf + (NBUFSIZ - 1))
					*p++ = ch;
			}
		}
		if (r <= 0)
			quickexit(0);

		printf("\r\n");
		fflush(stdout);
		if (ch != told.c_cc[VEOL] && ch != told.c_cc[VEOL2] &&
		    ch != '\r' && ch != '\n') {
			badlogin(username);
			tcsetattr(0, TCSANOW, &told);
			quickexit(0);
		}
		if (p > nbuf)
			if (nbuf[0] == '-')
				warnx("login names may not start with '-'.");
			else {
				*p = '\0';
				username = nbuf;
				break;
			}
	}
	tcsetattr(0, TCSANOW, &told);
}

void
printbanner(ttyn, hn)
	char *ttyn, *hn;
{
	cap_t *c;
	char *slash, *banner = "\r\n\r\nWind River Systems, Inc. BSD/OS (%h) (%t)\r\n\r\n";
	char tbuf[8192];
	int fd, nchars;
	time_t t;

	if (needbanner == 0)
		return;
	needbanner = 0;

	if (td != NULL && (c = cap_look(td->ty_cap, "banner")) != NULL &&
	    c->value != NULL)
		banner = c->value;

	while (*banner) {
		if (*banner != '%') {
			putc(*banner++, stdout);
			continue;
		}
		switch (*++banner) {

		case 't':
			slash = strrchr(ttyn, '/');
			if (slash == (char *) 0)
				fputs(ttyn, stdout);
			else
				fputs(&slash[1], stdout);
			break;

		case 'h':
			fputs(hn, stdout);
			break;

		case 'd': {
			static char fmt[] = "%l:% %p on %A, %d %B %Y";

			fmt[4] = 'M';           /* I *hate* SCCS... */
			(void)time(&t);
			(void)strftime(tbuf, sizeof(tbuf), fmt, localtime(&t));
			fputs(tbuf, stdout);
			break;
		}

		case '%':
			putc('%', stdout);
			break;
		}
		banner++;
	}

	if (td != NULL && (c = cap_look(td->ty_cap, "issue")) != NULL &&
	    c->value != NULL &&
	    (fd = open(c->value, O_RDONLY, 0)) >= 0) {
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
		(void)close(fd);
	}
}

int
rootterm(ttyn)
	char *ttyn;
{
	return (td && td->ty_status & TD_SECURE);
}

char *
getttyauth(ttyn)
	char *ttyn;
{
	cap_t *c;
	char *auth, *p, *e;

	if (td == NULL)
		return (NULL);
	
	if ((c = cap_look(td->ty_cap, "auth")) == NULL ||
	    c->value == NULL || c->value[0] == '\0')
		return (NULL);

	p = c->value;
	for (e = p; *e && *e != ':' && *e != ' ' && *e != '\t'; ++e)
		;
	if (*e || e == p) {
		syslog(LOG_NOTICE, "%s: invalid auth string", p);
		return (NULL);
	}
	auth = malloc(sizeof(char) * ((e - p) + 1 + 5));
	if (auth == NULL) {
		syslog(LOG_ERR, "%m");
		warn(NULL);
		quickexit(1);
	}
	strcpy(auth, "auth-");
	strcat(auth, p);
	return (auth);
}

jmp_buf motdinterrupt;

void
motd()
{
	int fd, nchars;
	sig_t oldint;
	char tbuf[8192];
    	char *motd;

    	motd = login_getcapstr(lc, "welcome", _PATH_MOTDFILE, _PATH_MOTDFILE);

	if ((fd = open(motd, O_RDONLY, 0)) < 0)
		return;

	oldint = signal(SIGINT, sigint);
	if (setjmp(motdinterrupt) == 0)
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
	(void)signal(SIGINT, oldint);
	(void)close(fd);
}

/* ARGSUSED */
void
sigint(signo)
	int signo;
{

	longjmp(motdinterrupt, 1);
}

/* ARGSUSED */
void
timedout(signo)
	int signo;
{
	badlogin(username);
	warnx("Login timed out after %d seconds", timeout);
	sleepexit(0);
}

/* ARGSUSED */
void
hungup(signo)
	int signo;
{
	badlogin(username);
	sleepexit(0);
}

void
dolastlog(quiet)
	int quiet;
{
	struct lastlog ll;
	int fd;

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0) {
		(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		if (!quiet) {
			if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
			    ll.ll_time != 0) {
				(void)printf("Last login: %.*s ",
				    24-5, (char *)ctime(&ll.ll_time));
				if (*ll.ll_host != '\0')
					(void)printf("from %.*s\n",
					    (int)sizeof(ll.ll_host),
					    ll.ll_host);
				else
					(void)printf("on %.*s\n",
					    (int)sizeof(ll.ll_line),
					    ll.ll_line);
			}
			(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		}
		memset((void *)&ll, 0, sizeof(ll));
		(void)time(&ll.ll_time);
		(void)strncpy(ll.ll_line, tty, sizeof(ll.ll_line));
		if (hostname)
			(void)strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		(void)write(fd, (char *)&ll, sizeof(ll));
		(void)close(fd);
	}
}

void
badlogin(name)
	char *name;
{

	if (failures == 0)
		return;
	if (hostname) {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s FROM %s",
		    failures, failures > 1 ? "S" : "", hostname);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s FROM %s, %s",
		    failures, failures > 1 ? "S" : "", hostname, name);
	} else {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s ON %s",
		    failures, failures > 1 ? "S" : "", tty);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s ON %s, %s",
		    failures, failures > 1 ? "S" : "", tty, name);
	}
}

#undef	UNKNOWN
#define	UNKNOWN	"su"

char *
stypeof(ttyid)
	char *ttyid;
{
	cap_t *c;
	char *defterm;

	defterm = login_getcapstr(lc, "term", UNKNOWN, UNKNOWN);

	if (ttyid == NULL || td == NULL ||
	    (c = cap_look(td->ty_cap, "term")) == NULL || c->value == NULL)
		return(defterm);
	return(c->value);
}

void
sleepexit(eval)
	int eval;
{
	auth_clean(as);
	auth_close(as);
	(void)sleep(5);
	exit(eval);
}

void
quickexit(eval)
	int eval;
{
	if (as) {
		auth_clean(as);
		auth_close(as);
	}
	exit(eval);
}
