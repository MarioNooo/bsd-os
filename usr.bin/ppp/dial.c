/*-
 * Copyright (c) 1993,1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI dial.c,v 2.18 1999/06/25 23:14:26 chrisk Exp
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/termios.h>
#include <net/if.h>
#include <net/if_dl.h>

#include <err.h>
#include <libdialer.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

#include "ppp.h"
#include "pbuf.h"
#include "ppp_proto.h"
#include "ppp_var.h"

static int daemon_fd = -1;
extern int
cfsetterm(struct termios *, const char *, char **);

extern	uid_t	suid;
extern	gid_t	sgid;

char *
Dial(session_t *S)
{
	int n, r;
	static char buf[2048];
	char *b, *p;
	int sv[2];
	char *pn = NULL;
	cap_t *req = NULL;

	if ((S->flags & F_DIRECT) && S->proxy == NULL) {
		if (S->device == NULL)
			err(1, "no device specified for direct connection");
		if ((S->fd = open(S->device, O_RDWR | O_EXCL)) < 0) {
			snprintf(buf, sizeof(buf),
			    "%s: %s", S->device, strerror(errno));
			return(buf);
		}
		set_lparms(S);
		return(0);
	}

	S->nextdst = S->destinations;

	while (S->nextdst != NULL && *S->nextdst != NULL) {
		pn = *(S->nextdst++);
		if (pn && cap_add(&req, RESOURCE_NUMBER, pn, CM_UNION))
			return(strerror(errno));
	}

	if ((S->flags & F_DIRECT) && cap_add(&req, "direct", NULL, CM_NORMAL))
		return(strerror(errno));

	if (S->speed > 0) {
		snprintf(buf, sizeof(buf), "%d", S->speed);
		if (cap_add(&req, RESOURCE_DTE, buf, CM_NORMAL))
			return(strerror(errno));
	}

	if ((p = S->device) != NULL) {
		if (S->proxy == NULL && (p = strrchr(S->device, '/')))
			++p;
		else
			p = S->device;
		if (cap_add(&req, RESOURCE_LINE, p, CM_NORMAL))
			return(strerror(errno));
	}

	if (S->proxy) {
		strncpy(buf, S->proxy, sizeof(buf));
		if (debug & D_XDEB)
			strncat(buf, " -x", sizeof(buf));
		strncat(buf, " ", sizeof(buf));
		if (S->sysfile) {
			strncat(buf, " -s ", sizeof(buf));
			strncat(buf, S->sysfile, sizeof(buf));
		}
		strncat(buf, S->sysname, sizeof(buf));
		if (cap_add(&req, "proxy", buf, CM_NORMAL))
			return(strerror(errno));
		/*
		 * Stty modes are only used for direct connections
		 */
		if (S->sttymodes && (S->flags & F_DIRECT))
			if (cap_add(&req, RESOURCE_STTYMODES, S->sttymodes,
			    CM_NORMAL))
				return(strerror(errno));
	}

	if ((debug & D_XDEB) && cap_add(&req, "verbose", NULL, CM_NORMAL))
		return(strerror(errno));

        if (daemon_fd >= 0)
                disconnect(S);

	if ((daemon_fd = connect_daemon(req, suid, sgid)) < 0) {
		cap_free(req);
		return(strerror(errno));
	}

	if (debug & D_XDEB) {
		cap_dump(req, buf, sizeof(buf));
		uprintf("Sending request %s\n", buf);
	}

	cap_free(req);

	for (;;) {
		b = buf - 1;
		r = 0;
		while (++b < buf + sizeof(buf)-1 &&
		    (r = read(daemon_fd, b, 1)) == 1)
			if (*b == '\n')
				break;
		*b = 0;
		if (strncmp(buf, "error:", 6) == 0)
			return(buf+6);
		else if (strcmp(buf, "daemon") == 0) {
			n = 2;
			if (recvfds(daemon_fd, sv, &n) < 0)
				return(strerror(errno));
			close(daemon_fd);
			daemon_fd = sv[1];
			S->fd = sv[0];
			set_lparms(S);
			return(0);
		} else if (strcmp(buf, "fd") == 0) {
			if ((S->fd = recvfd(daemon_fd)) < 0)
				return(strerror(errno));
			set_lparms(S);
			return(0);
		} else if (strncmp(buf, "message:", 8) == 0) {
			if (debug & D_XDEB)
				uprintf("%s\n", buf + 8);
		} else if (r <= 0) {
			if (debug & D_XDEB)
				uprintf("failed\n");
			return("lost connection to gettyd(8)");
		}
	}
}

void
disconnect(session_t *s)
{
	if (s->fd >= 0) {
		if (s->oldld >= 0) {
			(void) ioctl(s->fd, TIOCSETD, &s->oldld);
			s->oldld = -1;
		}
		close(s->fd);
		s->fd = -1;
	}
	if (daemon_fd >= 0) {
		close(daemon_fd);
		daemon_fd = -1;
	}
}

void
set_lparms(session_t *s)
{
	struct termios t;
	char *fail;

	/*
	 * Get old line discipline
	 */
	if (s->oldld < 0) {
		s->oldld = 0;      /* default... */
		(void) ioctl(s->fd, TIOCGETD, &s->oldld);
	}

	(void) tcgetattr(s->fd, &t);
	t.c_iflag &= IXON | IXOFF;
	t.c_iflag |= IGNBRK | IGNPAR;
	t.c_oflag = 0;
	t.c_cflag &= CCTS_OFLOW | CRTS_IFLOW | NOCLOCAL;
	t.c_cflag |= CS8 | CREAD | HUPCL;
	t.c_lflag = 0;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if ((s->flags & F_DIRECT) && s->speed > 0)
		cfsetspeed(&t, s->speed);
	if (s->sttymodes && (cfsetterm(&t, s->sttymodes, &fail) != 0))
		fprintf(stderr, "cfsetterm: %s: Bad value\n", fail);
	(void) tcsetattr(s->fd, TCSANOW, &t);
	dprintf(D_MAIN, "called set_lparms\n");
}
