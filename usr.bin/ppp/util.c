/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI util.c,v 2.6 1997/11/06 20:43:45 chrisk Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <net/if.h>
#include <net/if_dl.h>

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

static void iprintf(FILE *, char *, va_list);

typedef struct event_t {
	struct event_t	*next;
	void		(*func)();
	void		*arg;
	struct timeval	when;
} event_t;

static event_t *events = 0;
int debug = 0;
extern int dialin;
extern session_t *current_session;


FILE	*logfile = stderr;

void
dprintf(int level, char *fmt, ...)
{
	va_list ap;

	if ((debug & level) != level)
		return;

	va_start(ap, fmt);
	iprintf(logfile, fmt, ap);
	va_end(ap);
}

void
uprintf(char *fmt, ...)
{
	va_list ap;
	FILE *fp = dialin ? logfile : stdout;

	if ((debug & D_XDEB) == 0)
		return;

	va_start(ap, fmt);
	iprintf(fp, fmt, ap);
	va_end(ap);
}

static void
iprintf(FILE *fp, char *fmt, va_list ap)
{
	static char buffer[1024];
	static char *bp = buffer;
	char buf[64];
	char *op, *p;
	time_t now;

	bp += vsnprintf((op = bp), buffer + sizeof(buffer) - bp, fmt, ap);

	if (logfile == stderr || logfile == stdout) {
		fputs(buffer, fp);
		fflush(fp);
		bp = buffer;
	} else {
		while ((p = strchr(op, '\n')) != NULL) {
			time(&now);
			strftime(buf, sizeof(buf), "%b %d %T ", localtime(&now));
			fprintf(fp, "%s%s:%s: %.*s\n", buf,
			    current_session->sysname,
			    current_session->ifname,
			    p - buffer, buffer);
			fflush(fp);
			if (*++p) {
				memmove(buffer, p, strlen(p) + 1);
				bp = buffer + strlen(buffer);
				op = buffer;
			} else {
				op = bp = buffer;
				buffer[0] = '\0';
			}
		}
	}
}

void
timeout(func, ppp, tv)
	void (*func)();
	ppp_t *ppp;
	struct timeval *tv;
{
	event_t *f;
	event_t *e = malloc(sizeof(event_t));

	if (e == 0)
		err(1, "timeout");
	untimeout(func, ppp);		/* only one of them! */
	gettimeofday(&e->when, 0);
	tv_add(&e->when, tv);
	e->arg = ppp;
	e->func = func;

	if (events == NULL || tv_cmp(&e->when, &events->when) < 0) {
		e->next = events;
		events = e;
		return;
	}
	for (f = events; f->next && tv_cmp(&e->when, &f->next->when) >= 0;)
		f = f->next;
	e->next = f->next;
	f->next = e;
}

void
untimeout(func, ppp)
	void (*func)();
	ppp_t *ppp;
{
	event_t *e;
	event_t *f;

	while ((e = events) && (!func || e->func == func) && e->arg == ppp) {
		events = e->next;
		free(e);
	}
	while (e && (f = e->next)) {
		if ((!func || f->func == func) && f->arg == ppp) {
			e->next = f->next;
			free(f);
		} else
			e = f;
	}
}

int
tv_cmp(struct timeval *t1, struct timeval *t2)
{
	if (t1->tv_sec < t2->tv_sec)
		return(-1);
	if (t1->tv_sec > t2->tv_sec)
		return(1);
	if (t1->tv_usec < t2->tv_usec)
		return(-1);
	if (t1->tv_usec > t2->tv_usec)
		return(1);
	return(0);
}

void
tv_add(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	while (t1->tv_usec > 1000000) {
		t1->tv_usec -= 1000000;
		t1->tv_sec++;
	}
	while (t1->tv_usec < 0) {
		t1->tv_usec += 1000000;
		t1->tv_sec--;
	}
}

void
tv_delta(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec = t2->tv_sec - t1->tv_sec;
	t1->tv_usec = t2->tv_usec - t1->tv_usec;

	while (t1->tv_usec > 1000000) {
		t1->tv_usec -= 1000000;
		t1->tv_sec++;
	}

	while (t1->tv_usec < 0) {
		t1->tv_usec += 1000000;
		t1->tv_sec--;
	}
	if (t1->tv_sec < 0) {
		t1->tv_sec = 0;
		t1->tv_usec = 0;
	}
}

int
selread(int fd, pbuf_t *m, pbuf_t *c, int block)
{
	struct timeval now;
	fd_set rset;
	struct msghdr mh;
	struct iovec iov;

	m->len = 0;
	c->len = 0;

	for (;;) {
		gettimeofday(&now, 0);

		if (events && tv_cmp(&now, &events->when) >= 0) {
			event_t *e = events;
			events = events->next;
			(*e->func)(e->arg);
			free(e);
			return(0);
		}

		if (events)
			tv_delta(&now, &events->when);

		if (!block) {
			now.tv_usec = 0;
			now.tv_sec = 0;
		}

		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		switch (select(fd + 1, &rset, 0, 0,
		    (!block || events) ? &now : 0)) {
		case 0:
			break;
		case -1:
			switch(errno) {
			case EBADF:
				err(1, "Invalid file descriptor (%d)\n", fd);
				break;
			case EINTR:
				/* Try again */
				break;
			case EINVAL:
				err(1, "Invalid time: %d.%06d\n",
				    now.tv_sec, now.tv_usec);
			default:
				err(1, "select");
			}
			break;
		default:
			mh.msg_name = 0;
			mh.msg_namelen = 0;
			mh.msg_iov = &iov;
			mh.msg_iovlen = 1;
			mh.msg_control = (caddr_t)c->data;
			mh.msg_controllen = c->end - c->data;
			mh.msg_flags = 0;
			iov.iov_base = (caddr_t)m->data;
			iov.iov_len = m->end - m->data;
			if ((m->len = recvmsg(fd, &mh, 0)) < 0)
				return(-1);
			c->len = mh.msg_controllen;
			return(1);
		}
	}
}

int
process_packet(ppp_t *ppp)
{
	pbuf_t pb;
	pbuf_t cb;
	u_char *p;
	ppp_header_t *ph;

	pbuf_init(&pb);
	pbuf_init(&cb);

	while (selread(ppp->ppp_fd, &pb, &cb, 1) < 0) {
		pbuf_init(&pb);
		pbuf_init(&cb);
		switch (errno) {
		case EINTR:
			errno = 0;
			continue;
		default:
			err(1, "process_packet");
		}
	}

	if (pb.len && (debug & D_TRC)) {
		dprintf(D_TRC, "Packet of %d bytes:\n", pb.len);
		ph = (ppp_header_t *)pb.data;
		if (ntohs(ph->phdr_type) == PPP_PAP && pb.data[4] == 1) {
			dprintf(D_TRC, "\t(PAP Request suppressed)\n");
			goto recvit;
		}
		for (p = pb.data; p < pb.data + pb.len;) {
			if (((p - (u_char *)pb.data) & 0xf) == 0)
				dprintf(D_TRC, "\t");
			dprintf(D_TRC, "%02x", *p++);
			if ((p - (u_char *)pb.data) & 0xf)
				dprintf(D_TRC, " ");
			else
				dprintf(D_TRC, "\n");
		}
		if ((p - pb.data) & 0xf)
			dprintf(D_TRC, "\n");
	}
recvit:
	if (pb.len)
		ppp_cp_in(ppp, &pb);

	if (cb.len && (debug & D_TRC)) {
		dprintf(D_TRC, "Control Packet of %d bytes:\n", cb.len);
		for (p = cb.data; p < cb.data + cb.len;) {
			if (((p - (u_char *)cb.data) & 0xf) == 0)
				dprintf(D_TRC, "\t");
			dprintf(D_TRC, "%02x", *p++);
			if ((p - (u_char *)cb.data) & 0xf)
				dprintf(D_TRC, " ");
			else
				dprintf(D_TRC, "\n");
		}
		if ((p - cb.data) & 0xf)
			dprintf(D_TRC, "\n");
	}
	if (cb.len)
		ppp_cmsg(ppp, &cb);

	return (pb.len || cb.len);
}

void
sendpacket(ppp, m)
	ppp_t *ppp;
	pbuf_t *m;
{
	int r;
	u_char *p;
	ppp_header_t *ph = (ppp_header_t *)(m->data);

	if (ppp->ppp_fd < 0)
		return;

	if (debug & D_TRC) {
		dprintf(D_TRC, "Sending packet of %d bytes:\n", m->len);
		if (ntohs(ph->phdr_type) == PPP_PAP && m->data[4] == 1) {
			dprintf(D_TRC, "\t(PAP Request suppressed)\n");
			goto sendit;
		}
		for (p = m->data; p < m->data + m->len;) {
			if (((p - (u_char *)m->data) & 0xf) == 0)
				dprintf(D_TRC, "\t");
			dprintf(D_TRC, "%02x", *p++);
			if ((p - (u_char *)m->data) & 0xf)
				dprintf(D_TRC, " ");
			else
				dprintf(D_TRC, "\n");
		}
		if ((p - m->data) & 0xf)
			dprintf(D_TRC, "\n");
	}
sendit:

	r = sendto(ppp->ppp_fd, m->data, m->len, MSG_DONTROUTE,
		(struct sockaddr *)&ppp->ppp_dl, sizeof(ppp->ppp_dl));

	if (r < 0)
		warn("sendto");
	else if (r != m->len)
		warnx("short write (%d != %d)\n", r, m->len);
}
