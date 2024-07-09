/*-
 * Copyright (c) 1995,1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_lchpass.c,v 1.4 1997/08/08 18:58:23 prb Exp
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>
#include <login_cap.h>

FILE *back;

static void _auth_spool(char *fmt, ...);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct passwd *pwd;
	char passbuf[_PASSWORD_LEN+1];
	char localhost[MAXHOSTNAMELEN];
    	char *class = 0;
    	char *username = 0;
    	char *instance;
    	char *fullname;
	char *salt;
	char *p;
    	int c;
	struct rlimit rl;

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");

    	while ((c = getopt(argc, argv, "v:s:")) != EOF)
		switch(c) {
		case 'v':
			break;
		case 's':	/* service */
			if (strcmp(optarg, "login") != 0) {
				syslog(LOG_ERR, "%s: invalid service", optarg);
				exit(1);
			}
			break;
		default:
			syslog(LOG_ERR, "usage error");
			exit(1);
		}

	switch(argc - optind) {
	case 2:
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}

	fullname = strdup(username);
	if (fullname == NULL) {
		syslog(LOG_ERR, "could not strdup(%s)", username);
		exit(1);
	}

	pwd = getpwnam(fullname);

	if (!(back = fdopen(3, "a")))  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}

	if (pwd && *pwd->pw_passwd == '\0') {
		syslog(LOG_ERR, "%s attempting to add password", fullname);
#ifdef	BI_SILENT
		_auth_spool(BI_SILENT "\n");
#endif
		exit(0);
	}

	if (pwd)
		salt = pwd->pw_passwd;
	else
		salt = "xx";

	(void)setpriority(PRIO_PROCESS, 0, -4);

	printf("Changing local password for %s.\n", pwd->pw_name);
	p = getpass("Old Password:");

	salt = crypt(p, salt);
	memset(p, 0, strlen(p));
	if (!pwd || strcmp(salt, pwd->pw_passwd) != 0)
		exit(1);
	local_passwd(pwd->pw_name, 1);
#ifdef	BI_SILENT
	_auth_spool(BI_SILENT "\n");
#else
	fprintf(stderr, "Please ignore the following \"Login incorrect\" message\n");
#endif
	exit(0);
}

static void
_auth_spool(char *fmt, ...)
{
	va_list ap;

	if (back) {
		va_start(ap, fmt);

		vfprintf(back, fmt, ap);

		va_end(ap);
	}
}
