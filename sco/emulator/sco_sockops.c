/*
 * Copyright (c) 1993,1994,1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_sockops.c,v 2.5 1997/03/18 15:28:19 donn Exp
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <net/if.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_errno.h"
#include "sco_sig_state.h"

/*
 * File ops for sockets.
 *
 * Socket ops are apparently handled under SVr3
 * by opening a 'clone' device named /dev/socksys
 * and using ioctl() to pass a code for a BSD syscall
 * plus up to six arguments.
 *
 * XXX the sockaddr conversion code assumes old-style sockaddrs
 * XXX the sockaddr conversion code is highly byte-order dependent
 */

/* by observation of running programs */
#define	SCO_SOCK_IOCTL		0x801c4942
#define	SCO_SIOCATMARK		0x40045305
#define	SCO_SIOCGIFADDR		0xc020490c
#define	SCO_SIOCGIFFLAGS	0xc0204910
#define	SCO_SIOCGIFCONF		0xc0084911
#define	SCO_SIOCGIFBRDADDR	0xc0204920
#define	SCO_SIOCGPGRP		0x40045307
#define	SCO_SIOCSPGRP		0x80045306
#define	SCO_FIOASYNC		0x8004530a
#define	SCO_FIONREAD		0x40045308
#define	SCO_FIONBIO		0x80045309

struct sockop {
	int	code;
	int	arg[6];
};

extern struct fdops sockops;

#define	SCO_ACCEPT	 1
#define	SCO_BIND	 2
#define	SCO_CONNECT	 3
#define	SCO_GETPEERNAME	 4
#define	SCO_GETSOCKNAME	 5
#define	SCO_GETSOCKOPT 	 6
#define	SCO_LISTEN	 7
#define	SCO_RECV	 8
#define	SCO_RECVFROM	 9
#define	SCO_SEND	10
#define	SCO_SENDTO	11
#define	SCO_SETSOCKOPT	12
#define	SCO_SHUTDOWN	13
#define	SCO_SOCKET	14
/* #define	SCO_SELECT	15 */
#define	SCO_GETDOMAIN	16
#define	SCO_SETDOMAIN	17
#define	SCO_ADJTIME	18
#define	SCO_SETREUID	19
#define	SCO_SETREGID	20
#define	SCO_GETTIMEOFDAY 21
#define	SCO_SETTIMEOFDAY 22
/* #define	SCO_GETITIMER	23 */
/* #define	SCO_SETITIMER	24 */

extern struct fdops regops;
extern int reg_close __P((int));
extern int reg_dup __P((int));
extern int reg_fcntl __P((int, int, int));

static void
sock_init(f, filename, flags)
	int f;
	const char *filename;
	int flags;
{

	fd_register(f);
	if ((fdtab[f] = malloc(sizeof (struct fdbase))) == 0)
		err(1, "can't initialize regular file");
	fdtab[f]->f_ops = &sockops;
}

static ssize_t
sock_read(f, b, n)
	int f;
	void *b;
	size_t n;
{
	ssize_t r = commit_read(f, b, n);

	if (r == -1 && errno == EAGAIN)
		errno = errno_in(SCO_EWOULDBLOCK);

	return (r);
}

static ssize_t
sock_write(f, b, n)
	int f;
	const void *b;
	size_t n;
{
	ssize_t r = commit_write(f, b, n);

	if (r == -1 && errno == EAGAIN)
		errno = errno_in(SCO_EWOULDBLOCK);

	return (r);
}

static int
sock_in(ss, s, len)
	struct sockaddr *ss;
	struct sockaddr *s;
	int len;
{

	if (ss == 0 || len == 0) {
		errno = EINVAL;
		return (-1);
	}

	bcopy(ss, s, len);
	s->sa_family = ss->sa_len;
	s->sa_len = len;

	return (0);
}

#if 0
static void
sock_out(s, ss)
	struct sockaddr *s, *ss;
{
	int len;

	if (s == 0)
		return;

	len = s->sa_len;
	s->sa_len = s->sa_family;
	s->sa_family = 0;
	bcopy(s, ss, len);
	free(s);
}
#endif

static int
sco_accept(f, ss, alen)
	int f;
	struct sockaddr *ss;
	int *alen;
{
	int r;

	if ((r = commit_accept(f, ss, alen)) != -1) {
		sock_init(r, "", O_RDWR);

		/* convert to old-style sockaddr */
		ss->sa_len = ss->sa_family;
		ss->sa_family = 0;
	}
	return (r);
}

static int
sco_bind(f, ss, len)
	int f;
	const struct sockaddr *ss;
	int len;
{
	struct sockaddr *s;
	int r;

	if ((s = malloc(len)) == 0)
		err(1, "sco_bind");

	if (sock_in(ss, s, len) == -1) {
		free(s);
		return (-1);
	}

	r = bind(f, s, len);
	free(s);
	return (r);
}

static int
sco_connect(f, ss, len)
	int f;
	const struct sockaddr *ss;
	int len;
{
	struct sockaddr *s;
	int r;

	if ((s = malloc(len)) == 0)
		err(1, "sco_connect");

	if (sock_in(ss, s, len) == -1) {
		free(s);
		return (-1);
	}

	r = commit_connect(f, s, len);
	free(s);
	return (r);
}

static int
sco_getpeername(f, ss, alen)
	int f;
	struct sockaddr *ss;
	int *alen;
{
	int r;

	if ((r = getpeername(f, ss, alen)) != -1) {
		ss->sa_len = ss->sa_family;
		ss->sa_family = 0;
	}
	return (r);
}

static int
sco_getsockname(f, ss, alen)
	int f;
	struct sockaddr *ss;
	int *alen;
{
	int r;

	if ((r = getsockname(f, ss, alen)) != -1) {
		ss->sa_len = ss->sa_family;
		ss->sa_family = 0;
	}
	return (r);
}

static int
sco_recvfrom(f, m, mlen, flags, ss, alen)
	int f, mlen, flags, *alen;
	void *m;
	struct sockaddr *ss;
{
	int r;

	if ((r = commit_recvfrom(f, m, mlen, flags, ss, alen)) != -1 && ss) {
		ss->sa_len = ss->sa_family;
		ss->sa_family = 0;
	}
	return (r);
}

static int
sco_sendto(f, m, mlen, flags, ss, len)
	int f, mlen, flags, len;
	const void *m;
	const struct sockaddr *ss;
{
	struct sockaddr *s;
	int r;

	if (ss == 0)
		return (sendto(f, m, mlen, flags, 0, len));

	if ((s = malloc(len)) == 0)
		err(1, "sco_sendto");

	if (sock_in(ss, s, len) == -1) {
		free(s);
		return (-1);
	}

	r = sendto(f, m, mlen, flags, s, len);
	free(s);
	return (r);
}

static int
sco_setsockopt(f, level, optname, optval, optlen)
	int f;
	int level;
	int optname;
	const void *optval;
	int optlen;
{
	int r = 0;

	if (optname > SO_OOBINLINE && optname < 0x1000)
		/*
		 * XXX SCO defines but doesn't document two more sockopts
		 * XXX in this range, SO_ORDREL (0x200) and
		 * XXX SO_IMASOCKET (0x400).  We return success.
		 * XXX The Progress DB server has been observed to try
		 * XXX option value 0x1000, but it didn't seem to care
		 * XXX that this didn't work, so we leave it alone.
		 */
		return (0);
	if (optname == SO_REUSEADDR)
		/*
		 * The Progress DB server appears to expect that
		 * SO_REUSEADDR has the Stanford semantics that
		 * imply SO_REUSEPORT as well.
		 */
		r = setsockopt(f, level, SO_REUSEPORT, optval, optlen);
	if (r == 0)
		r = setsockopt(f, level, optname, optval, optlen);
	return (r);
}

static int
sco_getdomain(domain, len)
	char *domain;
	int len;
{
	char host[MAXHOSTNAMELEN];
	char *cp;

	if (gethostname(host, MAXHOSTNAMELEN) == -1)
		return (-1);
	if ((cp = index(host, '.')) == 0) {
		*domain = '\0';
		return (0);
	}
	++cp;
	strncpy(domain, cp, len);
	return (0);
}

static int
sco_setdomain(domain)
	char *domain;
{

	warnx("sco_setdomain: attempt to set domain to '%s' denied", domain);
	return (0);
}

static int
sco_gifconf(f, sifc)
	int f;
	struct ifconf *sifc;
{
	struct ifconf bifc;
	struct ifreq *sifr, *bifr, *slim, *blim;
	int r;
	int save_errno;

	/*
	 * XXX Since BSD ifreq's are sometimes bigger than SCO ifreq's
	 * but never smaller, the following code may lose
	 * if there are big ifreq's returned.
	 */
	bifc.ifc_len = sifc->ifc_len;
	if ((bifc.ifc_buf = (caddr_t)malloc(bifc.ifc_len)) == 0)
		errx(1, "sco_gifconf malloc");

	if ((r = ioctl(f, SIOCGIFCONF, &bifc)) == -1) {
		save_errno = errno;
		free(bifc.ifc_buf);
		errno = save_errno;
		return (-1);
	}

	blim = (struct ifreq *)(bifc.ifc_buf + bifc.ifc_len);
	slim = &sifc->ifc_req[sifc->ifc_len / sizeof (*sifr)];

	/* convert and truncate BSD ifreq array */
	for (bifr = bifc.ifc_req, sifr = sifc->ifc_req;
	    bifr < blim && sifr < slim;
	    bifr = (struct ifreq *)((caddr_t)bifr + sizeof (bifr->ifr_name) +
		MAX(bifr->ifr_addr.sa_len, sizeof (bifr->ifr_addr)))) {
		if (bifr->ifr_addr.sa_family == AF_LINK)
			continue;
		*sifr = *bifr;
		sifr->ifr_addr.sa_len = sifr->ifr_addr.sa_family;
		sifr->ifr_addr.sa_family = 0;
		++sifr;
	}

	free(bifc.ifc_buf);
	sifc->ifc_len = (caddr_t)sifr - sifc->ifc_buf;
	return (r);
}

static int
sock_ioctl(f, c, a)
	int f;
	unsigned long c;
	int a;
{
	const struct sockop *so;
	struct ifreq *ifr;
	int r;

	if (c != SCO_SOCK_IOCTL)
		sig_state = SIG_POSTPONE;

	switch (c) {
	case SCO_FIOASYNC:
		return (ioctl(f, FIOASYNC, a));

	case SCO_FIONREAD:
		return (ioctl(f, FIONREAD, a));

	case SCO_FIONBIO:
		return (ioctl(f, FIONBIO, a));

	case SCO_SIOCGIFCONF:
		return (sco_gifconf(f, (struct ifconf *)a));

	case SCO_SIOCGIFADDR:
		ifr = (struct ifreq *)a;
		if ((r = ioctl(f, SIOCGIFADDR, ifr)) == -1)
			return (-1);
		ifr->ifr_addr.sa_len = ifr->ifr_addr.sa_family;
		ifr->ifr_addr.sa_family = 0;
		return (r);

	case SCO_SIOCGIFFLAGS:
		return (ioctl(f, SIOCGIFFLAGS, (struct ifreq *)a));

	case SCO_SIOCGIFBRDADDR:
		ifr = (struct ifreq *)a;
		if ((r = ioctl(f, SIOCGIFBRDADDR, ifr)) == -1)
			return (-1);
		ifr->ifr_addr.sa_len = ifr->ifr_addr.sa_family;
		ifr->ifr_addr.sa_family = 0;
		return (r);

	case SCO_SIOCATMARK:
		return (ioctl(f, SIOCATMARK, a));

	case SCO_SIOCGPGRP:
		return (ioctl(f, SIOCGPGRP, a));

	case SCO_SIOCSPGRP:
		return (ioctl(f, SIOCSPGRP, a));

	case SCO_SOCK_IOCTL:
		so = (const struct sockop *)a;

		switch (so->code) {
		case SCO_ACCEPT:
		case SCO_CONNECT:
		case SCO_RECV:
		case SCO_RECVFROM:
			break;
		default:
			sig_state = SIG_POSTPONE;
		}

		switch (so->code) {
		case SCO_ACCEPT:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL accept(%#x, %#x, %#x)",
				    so->arg[0], so->arg[1], so->arg[2]);
#endif
			return (sco_accept(so->arg[0], (struct sockaddr *)
			    so->arg[1], (int *)so->arg[2]));
		case SCO_BIND:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL bind(%#x, %#x, %#x)", so->arg[0],
				    so->arg[1], so->arg[2]);
#endif
			return (sco_bind(so->arg[0], (const struct sockaddr *)
			    so->arg[1], so->arg[2]));
		case SCO_CONNECT:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL connect(%#x, %#x, %#x)",
				    so->arg[0], so->arg[1], so->arg[2]);
#endif
			return (sco_connect(so->arg[0],
			    (const struct sockaddr *) so->arg[1], so->arg[2]));
		case SCO_GETPEERNAME:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL getpeername(%#x, %#x, %#x)", so->arg[0],
				    so->arg[1], so->arg[2]);
#endif
			return (sco_getpeername(so->arg[0], (struct sockaddr *)
			    so->arg[1], (int *)so->arg[2]));
		case SCO_GETSOCKNAME:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL getsockname(%#x, %#x, %#x)", so->arg[0],
				    so->arg[1], so->arg[2]);
#endif
			return (sco_getsockname(so->arg[0], (struct sockaddr *)
			    so->arg[1], (int *)so->arg[2]));
		case SCO_GETSOCKOPT:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
		warnx("=> CALL getsockopt(%#x, %#x, %#x, %#x, %#x)", so->arg[0],
		    so->arg[1], so->arg[2], so->arg[3], so->arg[4], so->arg[5]);
#endif
			return (getsockopt(so->arg[0], so->arg[1], so->arg[2],
			    (void *)so->arg[3], (int *)so->arg[4]));
		case SCO_LISTEN:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL listen(%#x, %#x)", so->arg[0],
				    so->arg[1]);
#endif
			return (listen(so->arg[0], so->arg[1]));
		case SCO_RECV:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL recv(%#x, %#x, %#x, %#x)",
				    so->arg[0], so->arg[1], so->arg[2],
				    so->arg[3]);
#endif
			return (commit_recvfrom(so->arg[0], (void *)so->arg[1],
			    so->arg[2], so->arg[3], 0, 0));
		case SCO_RECVFROM:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
		warnx("=> CALL recvfrom(%#x, %#x, %#x, %#x, %#x, %#x)",
		    so->arg[0], so->arg[1], so->arg[2],
		    so->arg[3], so->arg[4], so->arg[5]);
#endif
			return (sco_recvfrom(so->arg[0], (void *)so->arg[1],
			    so->arg[2], so->arg[3], (struct sockaddr *)
			    so->arg[4], (int *)so->arg[5]));
		case SCO_SEND:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL send(%#x, %#x, %#x, %#x)",
				    so->arg[0], so->arg[1], so->arg[2],
				    so->arg[3]);
#endif
			return (send(so->arg[0], (const void *)so->arg[1],
			    so->arg[2], so->arg[3]));
		case SCO_SENDTO:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
		warnx("=> CALL sendto(%#x, %#x, %#x, %#x, %#x, %#x)",
		    so->arg[0], so->arg[1], so->arg[2],
		    so->arg[3], so->arg[4], so->arg[5]);
#endif
			return (sco_sendto(so->arg[0], (const void *)so->arg[1],
			    so->arg[2], so->arg[3], (const struct sockaddr *)
			    so->arg[4], so->arg[5]));
		case SCO_SETSOCKOPT:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
		warnx("=> CALL setsockopt(%#x, %#x, %#x, %#x, %#x)", so->arg[0],
		    so->arg[1], so->arg[2], so->arg[3], so->arg[4], so->arg[5]);
#endif
			return (sco_setsockopt(so->arg[0], so->arg[1],
			    so->arg[2], (const void *)so->arg[3], so->arg[4]));
		case SCO_SHUTDOWN:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL shutdown(%#x, %#x)",
				    so->arg[0], so->arg[1]);
#endif
			return (shutdown(so->arg[0], so->arg[1]));
		case SCO_SOCKET:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL socket(%#x, %#x, %#x)",
				    so->arg[0], so->arg[1], so->arg[2]);
#endif
			if ((r = socket(so->arg[0], so->arg[1], so->arg[2]))
			     != -1)
				sock_init(r, "", O_RDWR);
			return (r);
		case SCO_GETDOMAIN:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL getdomain(%#x, %#x)",
				    so->arg[0], so->arg[1]);
#endif
			return (sco_getdomain((char *)so->arg[0], so->arg[1]));
		case SCO_SETDOMAIN:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL setdomain(%#x)", so->arg[0]);
#endif
			return (sco_setdomain((char *)so->arg[0]));
		case SCO_ADJTIME:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL adjtime(%#x, %#x)", so->arg[0],
				    so->arg[1]);
#endif
			return (adjtime((struct timeval *)so->arg[0],
			    (struct timeval *)so->arg[1]));
		case SCO_SETREUID:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL setreuid(%#x, %#x)", so->arg[0],
				    so->arg[1]);
#endif
			return (setreuid(so->arg[0], so->arg[1]));
		case SCO_SETREGID:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL setregid(%#x, %#x)", so->arg[0],
				    so->arg[1]);
#endif
			return (setregid(so->arg[0], so->arg[1]));
		case SCO_GETTIMEOFDAY:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL gettimeofday(%#x, %#x)",
				    so->arg[0], so->arg[1]);
#endif
			return (gettimeofday((struct timeval *)so->arg[0],
			    (struct timezone *)so->arg[1]));
		case SCO_SETTIMEOFDAY:
#ifdef DEBUG
			if (debug & DEBUG_SYSCALLS)
				warnx("=> CALL settimeofday(%#x, %#x)",
				    so->arg[0], so->arg[1]);
#endif
			return (settimeofday((const struct timeval *)so->arg[0],
			    (const struct timezone *)so->arg[1]));
		}
		errx(1, "unsupported socksys syscall code (%d)", so->code);
		break;
	}

	if (c & IOC_DIRMASK)
		warnx("unsupported socksys ioctl (%#x)", c);
	errno = EINVAL;
	return (-1);
}

/*
 * Alas, since we don't generate the socket calls mechanically,
 * there isn't any reason to use the socket calls in the fdops structure!
 */
struct fdops sockops = {
	0,
	0,
	reg_close,
	0,
	reg_dup,
	0,
	0,
	0,
	0,
	0,
	reg_fcntl,
	0,
	fpathconf,
	fstat,
	/* XXX handle statfs buffer length? */
	(int (*) __P((int, struct statfs *, int, int)))fstatfs,
	0,
	0,
	enotdir,
	0,
	enostr,
	0,
	0,
	0,
	enotty,
	sock_init,
	sock_ioctl,
	0,
	lseek,
	0,
	0,
	enostr,
	sock_read,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	enotty,
	sock_write,
	0,
};
