/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_passwd.c,v 1.13 2001/10/03 17:29:55 polk Exp
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <login_cap.h>
#include <bsd_auth.h>

FILE *back;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *class, *instance, *p, *salt, *username, *wheel;
	char response[1024];
	int c, mode, lastchance;
	struct passwd *pwd;
	struct rlimit rl;

	class = NULL;
	username = NULL;
	wheel = NULL;
	mode = 0;
	p = NULL;
	lastchance = 0;

	rl.rlim_cur = 0;
	rl.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rl);

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

	back = NULL;
	while ((c = getopt(argc, argv, "dv:s:")) != EOF)
		switch(c) {
		case 'd':
			back = stdout;
			break;
		case 'v':
			if (strncmp(optarg, "wheel=", 6) == 0)
				wheel = optarg + 6;
			else if (strncmp(optarg, "lastchance=", 10) == 0)
				lastchance = strcmp(optarg + 10, "yes") == 0;
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
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}

	/*
	 * .root instances in passwd is just the root account.
	 * all other instances will fail.
	 * make a special check to see if there really is an
	 * account named user.root.
	 */
	if ((pwd = getpwnam(username)) == NULL) {
		instance = strchr(username, '.');
		if (instance && strcmp(instance+1, "root") == 0) {
			*instance++ = 0;
			if ((pwd = getpwnam(username)) == NULL || pwd->pw_uid)
				username = instance;
		}
		pwd = getpwnam(username);
	}

	if (back == NULL && (back = fdopen(3, "r+")) == NULL) {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}
	if (wheel != NULL && strcmp(wheel, "yes") != 0) {
		fprintf(back, BI_VALUE " errormsg %s\n",
		    auth_mkvalue("you are not in group wheel"));
		fprintf(back, BI_REJECT "\n");
		exit(1);
	}

	if (pwd && *pwd->pw_passwd == '\0') {
		fprintf(back, BI_AUTH "\n");
		exit(0);
	}

	if (mode == 1) {
		fprintf(back, BI_SILENT "\n");
		exit(0);
	}

	if (pwd)
		salt = pwd->pw_passwd;
	else
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
			exit(1);
		}
	} else
		p = getpass("Password:");

	salt = crypt(p, salt);
	memset(p, 0, strlen(p));
	if (!pwd || strcmp(salt, pwd->pw_passwd) != 0)
		exit(1);

	c = login_check_expire(back, pwd, class, lastchance);

	if (c == 0)
		fprintf(back, BI_AUTH "\n");
	exit(c);
}
