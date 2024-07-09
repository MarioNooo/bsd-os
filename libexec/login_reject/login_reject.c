/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_reject.c,v 1.5 1996/08/22 20:43:11 prb Exp
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <login_cap.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

FILE *back;

static void _auth_spool(char *fmt, ...);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct passwd *pwd;
	char passbuf[1];
    	int c;
	struct rlimit rl;
	int mode = 0;

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

    	while ((c = getopt(argc, argv, "v:s:")) != EOF)
		switch(c) {
		case 'v':
			break;
		case 's':	/* service */
			if (strcmp(optarg, "login") == 0)
				mode = 0;
			else if (strcmp(optarg, "challenge") == 0)
				mode = 1;
			else if (strcmp(optarg, "response") == 0)
				mode = 2;
			else {
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
	case 1:
		break;
	default:
		syslog(LOG_ERR, "usage error for");
		exit(1);
	}

	if (!(back = fdopen(3, "r+")))  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}
	if (mode == 1) {
		_auth_spool(BI_SILENT "\n");
		exit(0);
	}

	if (mode == 2) {
		mode = 0;
		c = -1;
		while (read(3, passbuf, 1) == 1) {
			if (passbuf[0] == '\0' && ++mode == 2)
				break;
		}
		if (mode < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			exit(1);
		}
	} else
		getpass("Password:");


	crypt("password", "xx");
	_auth_spool(BI_REJECT "\n");
	exit(1);
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
