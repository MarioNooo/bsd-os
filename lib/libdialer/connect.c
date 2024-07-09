/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI connect.c,v 1.7 1998/04/13 23:27:44 chrisk Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <libdialer.h>
#include <login_cap.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
connect_daemon(cap_t *cap, uid_t suid, gid_t sgid)
{
    	char buf[2048];
	cap_t *lc, *pc;
	struct sockaddr_un sun;
	int fd;
	int sv[2];

	lc = NULL;
	pc = cap;

	while (pc && strcmp(pc->name, "proxy")) {
		lc = pc;
		pc = pc->next;
	}

	if (pc && pc->values && pc->values[0]) {
		char *proxy = pc->values[0];
		char *p;
		argv_t *avt;

		if ((avt = start_argv(NULL)) == NULL)
			err(1, NULL);

		while (isspace(*proxy))
			++proxy;
		if (*proxy == '\0')
			errx(1, "empty proxy entry");

		while (*proxy) {
			p = proxy;
			while (*p && !isspace(*p))
				++p;
			if (*p)
				*p++ = '\0';
			if (add_argv(avt, proxy) == NULL)
				err(1, NULL);
			proxy = p;
		}

		if (pc->values[1]) {
			for (fd = 0; pc->values[fd]; ++fd)
				pc->values[fd] = pc->values[fd+1];
		} else if (lc) {
			lc->next = pc->next;
			pc->next = 0;
			cap_free(pc);
		} else
			*cap = *(cap->next);

		if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) < 0)
			err(1, NULL);

		switch (fork()) {
		case -1:
			err(1, NULL);
		case 0:
			/*
			 * If we are running as root or not as the user,
			 * nuke the environment and swith to the "daemon"
			 * class.  This set the environment to just the PATH.
			 */
			if (suid == 0 || geteuid() != getuid()) {
				extern char **environ;
				*environ = NULL;
				setclasscontext("daemon", LOGIN_SETALL);
			}
			/*
			 * If asked, set the uid/gid
			 */
			if (suid != -1)
				setuid(suid);
			if (sgid != -1)
				setgid(sgid);
			close(sv[0]);
			if (sv[1] != 0) {
				dup2(sv[1], 0);
				close(sv[1]);
			}
			proxy = avt->argv[0];
			if ((p = strrchr(proxy, '/')) != NULL)
				avt->argv[0] = p + 1;
			execv(proxy,  avt->argv);
			err(1, "%s", proxy);
		default:
			close(sv[1]);
			break;
		}
		fd = sv[0];
	} else {
		if ((fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
			err(1, "socket");

		sun.sun_family = PF_LOCAL;
		strcpy(sun.sun_path, _PATH_DIALER);

		if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
			err(1, "connect");
	}
	strcpy(buf, "request:");
	if (cap_dump(cap, buf + 8, sizeof(buf) - 8) == NULL)
		errx(1, "out of space");
    	write(fd, buf, strlen(buf));
    	return (fd);
}

int
connect_modem(int *daemon_fd, char **reason)
{
	static char buf[2048], *b;
	int n, r;
	int sv[2];

	*reason = 0;
	r = -1;
	for (;;) {
		b = buf - 1;
		while (++b < buf + sizeof(buf)-1 &&
		    (r = read(*daemon_fd, b, 1)) == 1)
			if (*b == '\n')
				break;
		*b = 0;
		if (strncmp(buf, "error:", 6) == 0) {
			*reason = buf+6;
			return (-1);
		}
		else if (strcmp(buf, "daemon") == 0) {
			n = 2;
			if (recvfds(*daemon_fd, sv, &n) < 0) {
				*reason = strerror(errno);
				return (-1);
			}
			close(*daemon_fd);
			*daemon_fd = sv[1];
			return (sv[0]);
		} else if (strcmp(buf, "fd") == 0) {
			if ((sv[0] = recvfd(*daemon_fd)) < 0) {
				*reason = strerror(errno);
				return (-1);
			}
			return (sv[0]);
		} else if (strncmp(buf, "message:", 8) == 0) {
			*reason = buf + 8;
			return (0);	/* special case */
		} else if (r <= 0) {
			*reason = "lost connection to gettyd(8)";
			return (-1);
		}
	}
}
