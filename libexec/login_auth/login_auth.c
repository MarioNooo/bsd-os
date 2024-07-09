/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI login_auth.c,v 1.5 1998/12/01 17:11:37 tks Exp
 */
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "include.h"

static void process(char *);
static void echoon(void);
static void echooff(void);

static int dflag;

void
main(int argc, char **argv)
{
	int c, r, vc;
	char *class, *service, *style, *username;
	char *p;
	char buf[256];
	char ibuf[64];
	fd_set rset;
	void *a;
	char **v;

	class = service = NULL;
	dflag = 0;
	vc = 0;

	if ((v = malloc(sizeof(char *) * (argc / 2))) == NULL)
		err(1, NULL);

	while ((c = getopt(argc, argv, "ds:t:T:v:")) != EOF) {
		switch(c) {
		case 'd':
			dflag = 1;
			break;
		case 's':       /* service */
			service = optarg;
			break;
		case 'T':
			traceall = 1;
			/* FALL THRU to 't' */
		case 't':
			if (trace) {
				syslog(LOG_ERR, "trace file already open");
				exit(1);
			}
			if (getuid())
				errx(1, "-t and -T are limited to super user");
                        if ((trace = fopen(optarg, "a")) == NULL)
                                err(1, "%s", optarg);
			break;
		case 'v':
			if (strlen(optarg) > sizeof(ibuf))
				errx(1, "-v option too long");
			v[vc++] = optarg;
			break;
		default:
			syslog(LOG_ERR, "usage error");
			exit(1);
		}
	}
	v[vc] = 0;

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

	if ((style = strrchr(argv[0], '/')) != NULL)
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

	if ((a = init_auth(username, class)) == NULL)
		err(1, "connecting to authentication server");
	atexit(echoon);

	if (snprintf(buf, sizeof(buf),
    	    "STYLE: %s\n"
	    "USER: %s\n"
	    "%s%s%s%s%s%s",
	    style, username,
	    service ? "SERVICE: " : "",
	    service ? service : "",
	    service ? "\n" : "",
	    class ? "CLASS: " : "",
	    class ? class : "",
	    class ? "\n" : "") >= sizeof(buf))
		errx(1, "username/style/service/class too long");

    	if (send_auth(a, buf) < 0)
		err(1, "sending data to authentication server");
	for (vc = 0; v[vc]; ++vc) {
		p = buf;
		p += snprintf(buf, sizeof(buf), "VARG: ");
		encodestring(p, v[vc], strlen(v[vc]));
		if (send_auth(a, buf) < 0)
			err(1, "sending data to authentication server");
	}
	snprintf(buf, sizeof(buf), "START:\n");
    	if (send_auth(a, buf) < 0)
		err(1, "sending data to authentication server");

	for (;;) {
		FD_ZERO(&rset);
		FD_SET(0, &rset);
		if (dflag == 0)
			FD_SET(3, &rset);
		FD_SET(fd_auth(a), &rset);

		switch (select(FD_SETSIZE, &rset, 0, 0, 0)) {
		case -1:
			if (errno == EINTR)
				continue;
			err(1, "waiting for input");
		case 0:
			break;
		default:
			if (FD_ISSET(0, &rset)) {
				r = read(0, ibuf, sizeof(ibuf));
				if (r < 0)
					err(1, NULL);
				p = buf;
				p += sprintf(p, "FD0: ");
				encodestring(p, ibuf, r);
				if (send_auth(a, buf) < 0)
					err(1, "sending data to authentication server");
			}
			if (dflag == 0 && FD_ISSET(3, &rset)) {
				r = read(3, ibuf, sizeof(ibuf));
				if (r < 0)
					err(1, NULL);
				p = buf;
				p += sprintf(p, "FD3: ");
				encodestring(p, ibuf, r);
				if (send_auth(a, buf) < 0)
					err(1, "sending data to authentication server");
			}
			if (FD_ISSET(fd_auth(a), &rset)) {
				do {
					buf[0] = '\0';
					r = read_auth(a, buf, sizeof(buf));
					if (r < 0)
						err(1, "reading data from authentication server");
					if (r > 0 && buf[0] != '\0')
						process(buf);
				} while (read_auth(a, NULL, 0));
			}
			break;
		}
	}
}

static void
process(char *cmd)
{
	char *data;
	int r;

	r = 0;
	if ((data = strchr(cmd, ' ')) != NULL) {
		*data++ = '\0';
		if ((r = decodestring(data)) < 0)
			errx(1, "garbage from authentication server");
	}

	if (strcmp(cmd, "FD1:") == 0) {
		if (r)
			write(1, data, r);
	} else if (strcmp(cmd, "FD2:") == 0) {
		if (r)
			write(2, data, r);
	} else if (strcmp(cmd, "FD3:") == 0) {
		if (r)
			write(dflag ? 1 : 3, data, r);
	} else if (strcmp(cmd, "EXIT:") == 0)
		exit(data ? atoi(data) : 1);
	else if (strcmp(cmd, "ECHO:") == 0) {
		if (data && strcmp(data, "OFF") == 0)
			echooff();
		else
			echoon();
	} else if (cleanstring(cmd))
		warnx("unknown command from authentication server: %s", cmd);
	else
		errx(1, "garbage from authentication server: %s", cmd);
}

static void
echoon()
{
	struct termios t;
	tcgetattr(0, &t);
	t.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &t);
}

static void
echooff()
{
	struct termios t;
	tcgetattr(0, &t);
	t.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &t);
}
