/*	BSDI select.x,v 1.5 2003/09/23 17:46:34 donn Exp	*/

/*
 * Select.h syscalls -- currently just select().
 */

%{
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
%}

int select(int nd, struct fd_set *is, struct fd_set *os, struct fd_set *es,
    const struct timeval *tv);

int __kernel__newselect(int nd, struct fd_set *is, struct fd_set *os,
    struct fd_set *es, const struct timeval *tv) = select;

int __kernel_select(void **arg) {
	return (__bsdi_syscall(SYS_select, (int)arg[0],
	    (struct fd_set *)arg[1], (struct fd_set *)arg[2],
	    (struct fd_set *)arg[3], (const struct timeval *)arg[4]));
}

int pselect(int nd, struct fd_set *is, struct fd_set *os, struct fd_set *es,
    const struct timespec *ts, const struct sigset *ss);

/*
 * Glibc has poll() emulation code that it uses if a poll() syscall
 * is not available, but it's buggy.  We replace it with our own code here.
 * Like the glibc poll() emulation, this simplified version does not
 * support a number of poll() features, such as POLLHUP.
 */

%{
struct pollfd {
	int		fd;
	short		events;
	short		revents;
};

#define	POLLIN		0x01
#define	POLLPRI		0x02
#define	POLLOUT		0x04
#define	POLLERR		0x08
#define	POLLHUP		0x10
#define	POLLNVAL	0x20

#define	INFTIM		(-1)
%}

int poll(struct pollfd *pfp, unsigned long n, int to)
{
	struct timeval tv;
	struct timeval *tvp = NULL;
	fd_set *rp = NULL;
	fd_set *wp = NULL;
	fd_set *ep = NULL;
	unsigned long i;
	int error;
	static int dtablesize = FD_SETSIZE;
	int maxfd = -1;
	int fd_setsize;
	int selected = 0;
	short summary = 0;

	for (i = 0; i < n; ++i) {
		summary |= pfp[i].events;
		if (pfp[i].fd > maxfd)
			maxfd = pfp[i].fd;
	}

	if (maxfd >= dtablesize) {
		dtablesize = __bsdi_syscall(SYS_getdtablesize);
		if (maxfd >= dtablesize)
			maxfd = dtablesize - 1;
	}
	fd_setsize = roundup(maxfd + 1, NFDBITS);

	if (summary & POLLIN) {
		rp = alloca(fd_setsize / NBBY);
		FD_NZERO(fd_setsize, rp);
	}
	if (summary & POLLOUT) {
		wp = alloca(fd_setsize / NBBY);
		FD_NZERO(fd_setsize, wp);
	}
	if (summary & POLLPRI) {
		ep = alloca(fd_setsize / NBBY);
		FD_NZERO(fd_setsize, ep);
	}

	if (to != INFTIM) {
		tv.tv_sec = (unsigned)to / 1000;
		tv.tv_usec = ((unsigned)to % 1000) * 1000;
		tvp = &tv;
	}

	for (i = 0; i < n; ++i) {
		pfp[i].revents = 0;
		if (pfp[i].fd < 0)
			/* SVr4 API p 144 documents this bizarreness */
			continue;
		if (pfp[i].fd >= fd_setsize) {
			pfp[i].revents |= POLLNVAL;
			continue;
		}
		if (pfp[i].events & POLLIN)
			FD_SET(pfp[i].fd, rp);
		if (pfp[i].events & POLLOUT)
			FD_SET(pfp[i].fd, wp);
		if (pfp[i].events & POLLPRI)
			FD_SET(pfp[i].fd, ep);
	}

	error = __bsdi_syscall(SYS_select, maxfd + 1, rp, wp, ep, tvp);
	if (__bsdi_error(error) && __bsdi_error_val(error) == EBADF) {
		for (i = 0; i < n; ++i) {
			error = __bsdi_syscall(SYS_fcntl, pfp[i].fd, F_GETFL);
			if (!__bsdi_error(error) ||
			    __bsdi_error_val(error) != EBADF)
				continue;
			pfp[i].revents |= POLLNVAL;
			if (pfp[i].events & POLLIN)
				FD_CLR(pfp[i].fd, rp);
			if (pfp[i].events & POLLOUT)
				FD_CLR(pfp[i].fd, wp);
			if (pfp[i].events & POLLPRI)
				FD_CLR(pfp[i].fd, ep);
		}
		error = __bsdi_syscall(SYS_select, maxfd + 1, rp, wp, ep, tvp);
	}
	if (__bsdi_error(error))
		return (error);

	for (i = 0; i < n; ++i) {
		if (pfp[i].fd < 0)
			continue;
		if (pfp[i].events & POLLIN && FD_ISSET(pfp[i].fd, rp))
			pfp[i].revents |= POLLIN;
		if (pfp[i].events & POLLOUT && FD_ISSET(pfp[i].fd, wp))
			pfp[i].revents |= POLLOUT;
		if (pfp[i].events & POLLPRI && FD_ISSET(pfp[i].fd, ep))
			pfp[i].revents |= POLLPRI;
		if (pfp[i].revents)
			++selected;
	}

	return (selected);
}
