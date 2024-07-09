/*-
 * Copyright (c) 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_token.c,v 1.2 1996/09/04 05:33:05 prb Exp
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <login_cap.h>

#include "token.h"

char	*getpass __P((char *));
char	*getepass __P((char *));
void	rip __P((char *));

main(argc, argv)
	int argc;
	char **argv;
{
	FILE *back = NULL;
	char *class = 0;
	char *username = 0;
	char challenge[1024];
	char *pp;
	int c;
	int mode = 0;
	char *instance;
	char *uname;		/* copy of username including instance	*/
	struct rlimit cds;

	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog(NULL, LOG_ODELAY, LOG_AUTH);

	cds.rlim_cur = 0;
	cds.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &cds) < 0)
		syslog(LOG_ERR, "couldn't set core dump size to 0: %m");

	if (token_init(argv[0]) < 0) {
		syslog(LOG_ERR, "unknown token type");
		errx(1, "unknown token type");
	}

	while ((c = getopt(argc, argv, "ds:v:")) != EOF)
		switch(c) {
		case 'd':		/* to remain undocumented */
			back = stdout;
			break;
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
		class = argv[optind + 1];
	case 1:
		username = argv[optind];
		break;
	default:
		syslog(LOG_ERR, "usage error");
		exit(1);
	}


	if (back == NULL && (back = fdopen(3, "r+")) == NULL)  {
		syslog(LOG_ERR, "reopening back channel");
		exit(1);
	}
	if (mode == 2) {
		mode = 0;
		c = -1;
		while (++c < sizeof(challenge) &&
		    read(3, &challenge[c], 1) == 1) {
			if (challenge[c] == '\0' && ++mode == 2)
				break;
			if (challenge[c] == '\0' && mode == 1)
				pp = challenge + c + 1;
		}
		if (mode < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			exit(1);
		}
	} else {
		tokenchallenge(username, challenge, sizeof(challenge),
		    tt->proper);
		if (mode == 1) {
			fprintf(back, BI_VALUE " challenge %s\n",
			    auth_mkvalue(challenge));
			fprintf(back, BI_CHALLENGE "\n");
			exit(0);
		}
		printf("%s", challenge);

		pp = getpass("");
		rip(pp);
		if (strlen(pp) == 0) {
			char buf[64];
			snprintf(buf, sizeof(buf), "%s Response [echo on]: ",
			    tt->proper);
			pp = getepass(buf);
			rip(pp);
		}
	}

	uname = strdup(username);

	if (instance = strchr(username, '.'))
		*instance++ = 0;

	if (tokenverify(uname, challenge, pp) == 0) {
		fprintf(back, BI_AUTH "\n");
		if (instance && (!strcmp(instance, "root")))
			fprintf(back, BI_ROOTOKAY "\n");
		fprintf(back, BI_SECURE "\n");
		exit(0);
	}
	fprintf(back, BI_REJECT "\n");
	exit(1);
}

/*
 * Strip trailing cr/lf from a line of text
 */

void
rip(buf)
char *buf;
{
        char *cp;

        if((cp = strchr(buf,'\r')) != NULL)
                *cp = '\0';

        if((cp = strchr(buf,'\n')) != NULL)
                *cp = '\0';
}
