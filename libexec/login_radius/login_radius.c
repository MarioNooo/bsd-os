/*-
 * Copyright (c) 1996, 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_radius.c,v 1.4 1998/04/14 00:39:02 prb Exp
 */
/*
 * Copyright(c) 1996 by tfm associates.
 * All rights reserved.
 *
 * tfm associates
 * P.O. Box 1244
 * Eugene OR 97440-1244 USA
 *
 * This contains unpublished proprietary source code of tfm associates.
 * The copyright notice above does not evidence any 
 * actual or intended publication of such source code.
 * 
 * A license is granted to Berkeley Software Design, Inc. by
 * tfm associates to modify and/or redistribute this software under the
 * terms and conditions of the software License Agreement provided with this
 * distribution. The Berkeley Software Design Inc. software License
 * Agreement specifies the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <login_cap.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int cleanstring(char *);
int raddauth(char *, char *, char *, char *, char *, char **);

void
main(int argc, char **argv)
{
	FILE *back;
	char challenge[1024];
	char *class, *service, *style, *username, *password, *emsg;
	int c, n;

	back = NULL;

	password = class = service = NULL;

	/*
	 * Usage: login_xxx [-s service] user.instance [class]
	 */
	while ((c = getopt(argc, argv, "ds:v:")) != EOF)
		switch(c) {
		case 'd':
			back = stdout;
			break;
		case 'v':
			break;
		case 's':       /* service */
			service = optarg;
			if (strcmp(service, "login") != 0 &&
			    strcmp(service, "challenge") != 0 &&
			    strcmp(service, "response") != 0)
				err(1, "%s not supported", service);
			break;
		default:
			syslog(LOG_ERR, "usage error");
			exit(1);
		}

	if (service == NULL)
		service = LOGIN_DEFSERVICE;

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

	if (style = strrchr(argv[0], '/'))
		++style;
	else
		style = argv[0];

	if (strncmp(style, "login_", 6) == 0)
		style += 6;

	if (!cleanstring(style))
		errx(1, "style contains non-printables");
	if (!cleanstring(username))
		errx(1, "username contains non-printables");
	if (service && !cleanstring(service))
		errx(1, "service contains non-printables");
	if (class && !cleanstring(class))
		errx(1, "class contains non-printables");

	/*
	 * Filedes 3 is the back channel, where we send back results.
	 */

        if (back == NULL && (back = fdopen(3, "a")) == NULL)  {
                syslog(LOG_ERR, "reopening back channel");
                exit(1);
        }

	memset(challenge, 0, sizeof(challenge));

	if (strcmp(service, "response") == 0) {
		c = -1;
		n = 0;
		while (++c < sizeof(challenge) &&
		    read(3, &challenge[c], 1) == 1) {
			if (challenge[c] == '\0' && ++n == 2)
				break;
			if (challenge[c] == '\0' && n == 1)
				password = challenge + c + 1;
		}
		if (n < 2) {
			syslog(LOG_ERR, "protocol error on back channel");
			exit(1);
		}
	}

	emsg = NULL;

	c = raddauth(username, class, style, 
	    strcmp(service, "login") ? challenge : NULL, password, &emsg);

	if (c == 0) {
		if (challenge == NULL || *challenge == '\0')
			fprintf(back, BI_AUTH "\n");
		else {
			fprintf(back, BI_VALUE " challenge %s\n",
			    auth_mkvalue(challenge));
			fprintf(back, BI_CHALLENGE "\n");
		}
		exit(0);
	}
	if (emsg && strcmp(service, "login") == 0)
		fprintf(stderr, "%s\n", emsg);
	else if (emsg)
		fprintf(back, "value errormsg %s\n", auth_mkvalue(emsg));
	if (strcmp(service, "challenge") == 0) {
		fprintf(back, BI_SILENT "\n");
		exit (0);
	}
	fprintf(back, BI_REJECT "\n");
	exit(1);
}

static int
cleanstring(char *s)
{
	while (*s && isgraph(*s) && *s != '\\')
		++s;
	return(*s == '\0' ? 1 : 0);
}
