/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_rpasswd.c,v 1.4 1996/11/19 06:16:51 prb Exp
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
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

FILE *back;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct rlimit rl;
    	int c, mode;
    	char *p, *password, *salt, *username, *wheel;
	char response[1024];

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login_rpasswd", LOG_ODELAY, LOG_AUTH);

	mode = 0;
	wheel = NULL;
    	while ((c = getopt(argc, argv, "v:s:")) != EOF)
		switch (c) {
		case 'v':
			if (strncmp(optarg, "wheel=", 6) == 0)
				wheel = optarg + 6;
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

	if ((back = fdopen(3, "r+")) == NULL) {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}
	if (wheel != NULL && strcmp(wheel, "yes") != 0) {
		fprintf(back, BI_VALUE " errormsg %s\n",
		    auth_mkvalue("you are not in group wheel"));
		fputs(BI_REJECT "\n", back);
		exit(1);
	}

    	username = password = NULL;
	switch (argc - optind) {
	case 2:
	case 1:
		username = argv[optind];
		password = rpasswd_get(username);
		break;
	default:
		syslog(LOG_ERR, "usage error");
		fputs(BI_REJECT "\n", back);
		exit(1);
	}

	if (password && *password == '\0') {
		fputs(BI_AUTH "\n", back);
		exit(0);
	}

	if (mode == 1) {
		fputs(BI_SILENT "\n", back);
		exit(0);
	}

	if ((salt = password) == NULL)
		salt = "xx";

	(void)setpriority(PRIO_PROCESS, 0, -4);

	if (mode == 2) {
		mode = 0;
		c = -1;
		while (++c < sizeof(response) &&
		    read(3, &response[c], 1) == 1) {
			if (response[c] == '\0' && ++mode == 2)
				break;
			if (response[c] == '\0' && mode == 1)
				p = response + c + 1;
		}
		if (mode < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			fputs(BI_REJECT "\n", back);
			exit(1);
		}
	} else
		p = getpass("Password:");

	salt = crypt(p, salt);
	memset(p, 0, strlen(p));
	if (password == NULL || strcmp(salt, password) != 0) {
		fputs(BI_REJECT "\n", back);
		exit(1);
	}
	fputs(BI_AUTH "\n", back);
	exit(0);
}
