/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_poll.c,v 2.2 1997/04/25 16:01:23 donn Exp
 */

/*
 * Support for poll() syscall emulation.
 * We emulate what we can, and hope not to see strange stuff.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"
#include "sco_sig_state.h"

/* from iBCS2 p 6-45 */

struct pollfd {
	int		fd;
	short		events;
	short		revents;
};

#define	SCO_POLLIN	0x01
#define	SCO_POLLPRI	0x02
#define	SCO_POLLOUT	0x04
#define	SCO_POLLERR	0x08
#define	SCO_POLLHUP	0x10
#define	SCO_POLLNVAL	0x20

/* from SVr4 API p 144 */
#define	SCO_INFTIM	(-1)

int
sco_poll(pfp, n, to)
	struct pollfd *pfp;
	unsigned long n;
	int to;
{
	struct timeval tv;
	fd_set r, w, e;
	int maxfd = -1;
	int bits;
	int i, selected = 0;

	FD_ZERO(&r);
	FD_ZERO(&w);
	FD_ZERO(&e);

	if (to != SCO_INFTIM) {
		tv.tv_sec = (unsigned)to / 1000;
		tv.tv_usec = ((unsigned)to % 1000) * 1000;
	}

	for (i = 0; i < n; ++i) {
		pfp[i].revents = 0;
		if (pfp[i].fd < 0)
			/* SVr4 API p 144 documents this bizarreness */
			continue;
		if (pfp[i].fd > maxfd)
			maxfd = pfp[i].fd;
		if (ioctl(pfp[i].fd, TIOCMGET, &bits) == -1) {
			if (errno == EBADF)
				pfp[i].revents |= SCO_POLLNVAL;
			/* otherwise, probably harmless */
		} else if ((bits & TIOCM_CAR) == 0)
			pfp[i].revents |= SCO_POLLHUP;
		if (pfp[i].events & SCO_POLLIN)
			FD_SET(pfp[i].fd, &r);
		if (pfp[i].events & SCO_POLLOUT)
			FD_SET(pfp[i].fd, &w);
		if (pfp[i].events & SCO_POLLPRI)
			FD_SET(pfp[i].fd, &e);
	}

	if (commit_select(maxfd + 1, &r, &w, &e, to == SCO_INFTIM ? 0 : &tv) ==
	    -1)
		return (-1);

	for (i = 0; i < n; ++i) {
		if (pfp[i].fd < 0)
			continue;
		if (pfp[i].events & SCO_POLLIN && FD_ISSET(pfp[i].fd, &r))
			pfp[i].revents |= SCO_POLLIN;
		if (pfp[i].events & SCO_POLLOUT && FD_ISSET(pfp[i].fd, &w))
			pfp[i].revents |= SCO_POLLOUT;
		if (pfp[i].events & SCO_POLLPRI && FD_ISSET(pfp[i].fd, &e))
			pfp[i].revents |= SCO_POLLPRI;
		if (pfp[i].revents)
			++selected;
	}

	return (selected);
}
