/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_rchpass.c,v 1.2 1996/09/15 20:11:12 bostic Exp
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <login_cap.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "extern.h"

static FILE *back;

static void _auth_spool(char *fmt, ...);

int
main(int argc, char *argv[])
{
	struct rlimit rl;
    	int c;
    	char *class, *p, *password, *salt, *username;
	char localhost[MAXHOSTNAMELEN];

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

	if ((back = fdopen(3, "a")) == NULL)  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}

	class = username = NULL;
	switch (argc - optind) {
	case 2:
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		password = rpasswd_get(username);
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}

	if (password != NULL && *password == '\0') {
		syslog(LOG_ERR, "%s attempting to add password", username);
		_auth_spool(BI_SILENT "\n");
		exit(0);
	}

	if ((salt = password) == NULL)
		salt = "xx";

	(void)setpriority(PRIO_PROCESS, 0, -4);

	p = getpass("Password:");

	salt = crypt(p, salt);
	memset(p, 0, strlen(p));
	if (password == NULL || strcmp(salt, password) != 0)
		exit(1);
	rpasswd(username);
	_auth_spool(BI_SILENT "\n");
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
