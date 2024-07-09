/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_compat.c,v 2.2 1997/10/31 03:14:04 donn Exp
 */

#include "nsl_compat.h"
#include <sys/stat.h>

extern void *(*sco__calloc)(size_t cnt, size_t size);
extern void *(*ibc__calloc)(size_t cnt, size_t size);
extern void (*sco__free)(void *ptr);
extern void (*ibc__free)(void *ptr);

char *__progname = "libnsl";

ts_t *
ts_get(fd)
	int fd;
{
	ts_t *tsp;
	struct stat stat;
	int type;
	int len;
	for (tsp = ts_list; tsp != NULL; tsp = tsp->ts_next)
		if (tsp->ts_fd == fd)
			return (tsp);

	if (fstat(fd, &stat) || (stat.st_mode & S_IFSOCK) == 0) {
		t_errno = TBADF;
		return (NULL);
	}
	len = sizeof(int);
	if (getsockopt (fd, SOL_SOCKET, SO_TYPE, &type, &len)) {
		 t_errno = TSYSERR;
		 tli_maperrno();
		 return (NULL);
	}

	t_errno = TBADF;
	return (NULL);
}

#if 0
static char malbuf[40000];
static int malpnt;
#endif

void *
memalloc(size) 
	size_t	size;
{
	void *p;

	char buffer[100];

	if (sco__calloc)
		p = (*sco__calloc)(1, size);
	else
		p = (*ibc__calloc)(1, size);
#if 0
	p = &malbuf[malpnt];
	malpnt += size;
	bzero (p, size);

#endif

	if (p != NULL)
		return (p);
	/*
	 * fix up error when alloc fails
	 */
	 t_errno = TSYSERR;
	 errno = ENOMEM;
	 tli_maperrno();
	 return (0);
}


void
memfree(ptr)
	void *ptr;
{
	if (sco__free != NULL)
		(*sco__free)(ptr);
	else
		(*ibc__free)(ptr);
	return;
}

void
sock_init(sp, family)
	struct sockaddr *sp;
	int family;
{
	bzero(sp, sizeof(*sp));
	sp->sa_family = family;
	sp->sa_len = sizeof(*sp);
}

int
sock_in(np, sp, len, required)
	netbuf_t *np;
	struct sockaddr *sp;
	int len, required;
{

	if (np->len == 0) {
		if (required) {
			t_errno = TNOADDR;
			return (-1);
		}
		return (0);
	}

	if (np->len > len) {
		t_errno = TBADADDR;
		return (-1);
	}
	if (np->len != len)
		bzero(sp, len);
	bcopy(np->buf, sp, np->len);
	/*
	 * allow family to be in network byte order
	 */
	if (!sp->sa_family)
		sp->sa_family = sp->sa_len;
	sp->sa_len = np->len;
	return (0);
}

int
sock_out(np, sp)
	netbuf_t *np;
	struct sockaddr *sp;
{
	int len;

	if (np->maxlen == 0)
		return (0);

	len = sp->sa_len;
	if (len > np->maxlen) {
		t_errno = TBUFOVFLW;
		return (-1);
	}
	sp->sa_len = sp->sa_family;
	sp->sa_family = 0;
	bcopy(sp, np->buf, len);
	return (0);
}

/*
 * Stub out the rest of NSL routines.
 */
#define	STUB(name) \
	int \
	name(void) \
	{ \
\
		t_errno = TSYSERR; \
		return (-1); \
	}

STUB(_t_checkfd)
STUB(_t_aligned_copy)
STUB(_t_max)
STUB(_t_putback)
STUB(_t_is_event)
STUB(_t_is_ok)
STUB(_t_do_ioctl)
STUB(_t_alloc_bufs)
STUB(_t_setsize)
STUB(_null_tiptr)
STUB(_snd_conn_req)
STUB(_rcv_conn_con)
STUB(_alloc_buf)
