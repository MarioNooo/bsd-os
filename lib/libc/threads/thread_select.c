/*-
 * Copyright (c) 1996, 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_select.c,v 1.22 2003/08/15 19:15:21 polk Exp
 */

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Select.  This version goes to special pains to deal with 
 * more than FD_SETSIZE bits in the select masks.  Also note
 * that while FD_SET, FD_CLR, and FD_ISSET macros work ok
 * the FD_COPY and FD_ZERO macros do not since they assume
 * a fixed size fd_set.
 */

#define	RDBITS	0
#define	WRBITS	1
#define	EXBITS	2

#define	allocbits(nbits, name, x) {					\
	if ((name) != NULL) { 						\
		if (nbits > FD_SETSIZE) {				\
			if ((bp[(x)] = FD_ALLOC(nbits)) == NULL)	\
				abort();				\
		} else							\
			bp[(x)] = &bits[x];				\
	} else 								\
		bp[(x)] = NULL;						\
}

#define freebits(name, x)						\
	if (((name) != NULL) && (bp[x] != &bits[x])) 			\
		free(bp[x]);

#define	getbits(nbits, name, x) 					\
	if ((name) != NULL)  						\
		FD_NCOPY(nbits, (name), bp[(x)]);

#define	putbits(nbits, name, x) 					\
	if ((name) != NULL)  						\
		FD_NCOPY(nbits, bp[(x)], (name));


int
_thread_sys_select(numfds, rfds, wfds, efds, timeout)
	int numfds;
	fd_set *rfds;
	fd_set *wfds;
	fd_set *efds;
	struct timeval *timeout;
{
	struct timespec ts, *tsp;

	TR("_thread_sys_select", numfds, timeout);

#ifdef DEBUG
	if (timeout)
		TR("_thread_sys_select_timeout", timeout->tv_sec,
		   timeout->tv_usec);
	if (rfds)
		TR("_thread_sys_select_rfds", rfds, *rfds);
	if (wfds)
		TR("_thread_sys_select_wfds", wfds, *wfds);
	if (efds)
		TR("_thread_sys_select_efds", efds, *efds);
#endif

	/* Convert timeval to a POSIX timespec. */
	if (timeout == NULL) 
		tsp = NULL;
	else {
		TIMEVAL_TO_TIMESPEC(timeout, &ts);
		tsp = &ts;
	}

	return _thread_sys_pselect(numfds, rfds, wfds, efds, tsp, NULL);
}

int 
_thread_sys_pselect(numfds, rfds, wfds, efds, timeout, sigmask)
	int numfds;
	fd_set *rfds;
	fd_set *wfds;
       	fd_set *efds;
	struct timespec *timeout;
	const sigset_t *sigmask;
{
	struct pthread * selfp;
	thread_aio_desc_t *dp;
	fd_set bits[3];
	fd_set *bp[3];
	struct timeval poll;
	struct timespec ts, *tsp;
	sigset_t oset;
	int badfd, nfds, erc, i;

	selfp = _thread_selfp();

	TR("_thread_sys_pselect", numfds, timeout);

#ifdef DEBUG
	if (timeout)
		TR("_thread_sys_pselect_timeout", timeout->tv_sec,
		   timeout->tv_nsec);
	if (rfds)
		TR("_thread_sys_pselect_rfds", rfds, *rfds);
	if (wfds)
		TR("_thread_sys_pselect_wfds", wfds, *wfds);
	if (efds)
		TR("_thread_sys_pselect_efds", efds, *efds);
#endif

	if (timeout != NULL) {
		if (timespecinvalid(timeout)) {
			errno = EINVAL;
			return (-1);
		}
		ts = *timeout;
		tsp = &ts;
	} else
		tsp = NULL;

	_thread_kern_enter();
	if (sigmask != NULL)
		_thread_sigmask(SIG_SETMASK, sigmask, &oset);

	/* Handle simple timer cases (no fds to poll) */
	if (numfds == 0) {
		errno = _thread_kern_block(NULL, PS_SELECT_WAIT, tsp);
		TR("_thread_sys_pselect_blkret1", errno, 0);
		if (errno == ETHREAD_CANCELED)
			_thread_cancel();
		if ((errno == ETIMEDOUT) || (errno == 0))
			nfds = 0;
		else
			nfds = -1;
		goto Return;
	}

	if (numfds < 0) {
		errno = EINVAL;
		nfds = -1;
		goto Return;
	}
	if (numfds > _thread_aio_table_size)
		numfds = _thread_aio_table_size;

	timerclear(&poll);
	erc = 0;

	/* Mark I/O pending on any fds we reference */
	badfd = 0;
	for (i = 0; i < numfds; i++) {
		int bad;

		dp = _thread_aio_table[i];
		bad = dp == NULL || dp->aiofd_object == NULL;
		if ((rfds != NULL) && FD_ISSET(i, rfds)) {
			if (bad) {
				TR("_thread_sys_pselect_badrfd", numfds, i);
				badfd++;
				break;
			}
			dp->aiofd_rcount++;
		}
		if ((wfds != NULL) && FD_ISSET(i, wfds)) {
			if (bad) {
				TR("_thread_sys_pselect_badwfd", numfds, i);
				badfd++;
				break;
			}
			dp->aiofd_wcount++;
		}
		if ((efds != NULL) && FD_ISSET(i, efds)) {
			if (bad) {
				TR("_thread_sys_pselect_badefd", numfds, i);
				badfd++;
				break;
			}
			dp->aiofd_ecount++;
		}
	}

	/* If we have a bad FD we need to fix refcounts */
	if (badfd) {
		/* Limit scan to those already set */
		numfds = i + 1;
		/* 
		 * Not possible for close to have happened since we
		 * are in the kernel, no need to check.
		 */
		badfd = 0;

		errno = EBADF;
		nfds = -1;
		goto ResetBits;
	}

	allocbits(numfds, rfds, RDBITS);
	allocbits(numfds, wfds, WRBITS);
	allocbits(numfds, efds, EXBITS);

	while (1) {
		getbits(numfds, rfds, RDBITS);
		getbits(numfds, wfds, WRBITS);
		getbits(numfds, efds, EXBITS);

		nfds = _syscall_sys_select(numfds, bp[0], bp[1], bp[2], &poll);
		TR("_thread_sys_pselect_selret1", numfds, nfds);

		/* Return if we got something or if this was just a poll */
		if (erc == ETIMEDOUT || nfds == -1 || nfds > 0 ||
		    (tsp != NULL && !timespecisset(tsp))) {

			for (i = 0; i < numfds; i++) {
				dp = _thread_aio_table[i];
				if (rfds != NULL && FD_ISSET(i, rfds))
					dp->aiofd_rcount--;
				if (wfds != NULL && FD_ISSET(i, wfds))
					dp->aiofd_wcount--;
				if (efds != NULL && FD_ISSET(i, efds))
					dp->aiofd_ecount--;
			}

			if (nfds >= 0 || erc == ETIMEDOUT) {
				putbits(numfds, rfds, RDBITS);
				putbits(numfds, wfds, WRBITS);
				putbits(numfds, efds, EXBITS);
			}

			freebits(rfds, RDBITS);
			freebits(wfds, WRBITS);
			freebits(efds, EXBITS);

			goto Return;
		}

		/* 
		 * Block waiting for any I/O to come ready. If timeval
		 * was passed in NULL tsp will also be null which means
		 * we'll block forever.  Thats exactly what we want for
		 * select. If we do get a timeout we'll force one more
		 * poll to see if we get anything before we return.
		 */
		erc = _thread_kern_block(&_thread_selectq, PS_SELECT_WAIT, tsp);
		TR("_thread_sys_pselect_blkret2", erc, 0);

		/* check for descriptors that have gone bad */
		badfd = 0;
		for (i = 0; i < numfds; i++) {
			int bad;

			dp = _thread_aio_table[i];
			bad = dp == NULL || dp->aiofd_object == NULL;
			if (rfds != NULL && FD_ISSET(i, rfds) && bad) {
				TR("_thread_sys_pselect_badrfd", numfds, i);
				badfd++;
				break;
			}
			if (wfds != NULL && FD_ISSET(i, wfds) && bad) {
				TR("_thread_sys_pselect_badwfd", numfds, i);
				badfd++;
				break;
			}
			if (efds != NULL && FD_ISSET(i, efds) && bad) {
				TR("_thread_sys_pselect_badefd", numfds, i);
				badfd++;
				break;
			}
		}

		if (badfd) {
			/* Reset all the I/O active flags */
			errno = EBADF;
			nfds = -1;
			break;
		}

		/* We've defered the cancel test so we can clean up. */
		if (errno == ETHREAD_CANCELED)
			break;

		if (errno == EINTR) {
			nfds = -1;
			break;
		}

		/* Copy the timeleft back into our timer */
		if (errno != ETIMEDOUT && tsp != NULL)
			*tsp = selfp->timer;
	}

 ResetBits:
	/* Reset I/O active bits on all FDs we've touched */
	for (i = 0; i < numfds; i++) {
		if ((dp = _thread_aio_table[i]) == NULL)
			continue;
		if (rfds != NULL && FD_ISSET(i, rfds))
			dp->aiofd_rcount--;
		if (wfds != NULL && FD_ISSET(i, wfds))
			dp->aiofd_wcount--;
		if (efds != NULL && FD_ISSET(i, efds))
			dp->aiofd_ecount--;
		if (badfd) {
			/* 
			 * If last reference to a closed fd
			 * notify closing thread
			 */
			if (dp != NULL &&
			    dp->aiofd_object == NULL &&
			    dp->aiofd_rcount == 0 &&
			    dp->aiofd_wcount == 0 &&
			    dp->aiofd_ecount == 0) {
				THREAD_EVT_OBJ_USER_0_IF(
				    dp->aiofd_close.q_head,
				    EVT_LOG_PTRACE_THREAD,
				    EVENT_THREAD_SELECT_CWUP);
				while (_thread_kern_wakeup(&dp->aiofd_close))
					continue;
			}
		}
	}

	/* The erc is set independently of errno */
	if (erc == ETHREAD_CANCELED)
		_thread_cancel();

 Return:
	if (sigmask != NULL)
		_thread_sigmask(SIG_SETMASK, &oset, NULL);
	_thread_kern_exit();
	return (nfds);
}
