/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_aio.c,v 1.23 2003/08/15 19:15:21 polk Exp
 */

#include <sys/types.h>
#include <time.h>
#include <sys/resource.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "thread_private.h"
#include "thread_trace.h"

/*
 * Thread Support for Polled Asynchronous I/O.
 *
 * These routines provide the basis for the upper level 
 * support for non-blocking I/O.  They all assume that the
 * thread kernel monitor lock is held upon call.
 */

struct pthread_queue _thread_selectq;
thread_aio_desc_t **_thread_aio_table;
int _thread_aio_table_size;		/* Size we have allocated */
int _thread_aio_dtablesize;		/* Current RLIMIT_NOFILE */
static fd_set *_thread_mrp, *_thread_mwp, *_thread_mep;

int _thread_aio_table_init()
{
	int fd, ret;
	struct rlimit flimit;

	TR("_thread_aio_table_init", 0, 0);

	if (_syscall_sys_getrlimit(RLIMIT_NOFILE, &flimit) != 0)
		return (errno);
	_thread_aio_table_size = _thread_aio_dtablesize = flimit.rlim_cur;

	/* Allocate memory for the file descriptor table. */
	_thread_aio_table = calloc(_thread_aio_table_size,
	    sizeof(*_thread_aio_table));
	if (_thread_aio_table == NULL) {
		_thread_aio_table_size = 0;
		return (ENOMEM);
	}

	/*
 	 * Enter a loop to initialise the file descriptor
 	 * table:  This can be done after the program has
	 * run for a bit so we need to try to get the fcntl
 	 * flags and initialize already opened file 
 	 * descriptors.
	 */

	/* First pass, init the table and get the current flags. */
 	for (fd = 0; fd < _thread_aio_table_size; fd++)
		if ((ret = _thread_aio_entry_init(fd)) != 0 &&
		    ret != EBADF)
			return (ret);

	_thread_queue_init(&_thread_selectq);

	return (0);
}

/*
 * Initialize if necessary a file table slot.  Allocate memory for the
 * entry if not already done.  Assumes the monitor lock is held.
 */
int
_thread_aio_entry_init(fd)
	u_int fd;
{
	thread_aio_desc_t *dp;
	void *tspace;
	int flags;

	TR("_thread_aio_entry_init", fd, 0);

	/* Set the per-fd non-blocking flag if necessary */
	if ((flags = _syscall_sys_fcntl(fd, F_GETFD, 0)) == -1)
		return(errno);
	if ((flags & FD_NONBLOCK) == 0 &&
	    _syscall_sys_fcntl(fd, F_SETFD, flags | FD_NONBLOCK) == -1)
		return (errno);

	/* Do we need to resize the table? */
	if (fd >= _thread_aio_table_size) {
		if (fd >= _thread_aio_dtablesize)
			return (EBADF);

		/*
		 * Need to get more space for the aio table.
		 * _thread_aio_dtablesize is the max size.
		 * _thread_aio_table_size is the working size.
		 */
		tspace = realloc(_thread_aio_table,
		    _thread_aio_dtablesize * sizeof(*_thread_aio_table));
		if (tspace == NULL)
			return (ENOMEM);
		_thread_aio_table = tspace;

		/* Zero the new portion of the table */
		memset(_thread_aio_table + _thread_aio_table_size, 0, 
		    (_thread_aio_dtablesize - _thread_aio_table_size) * 
		    sizeof(* _thread_aio_table));

		/* Free FD masks, they will be reallocated when needed */
		if (_thread_mrp != NULL) {
			free(_thread_mrp);
			_thread_mrp = NULL;
		}
		if (_thread_mwp != NULL) {
			free(_thread_mwp);
			_thread_mwp = NULL;
		}
		if (_thread_mep != NULL) {
			free(_thread_mep);
			_thread_mep = NULL;
		}

		/* Remember the new size */
		_thread_aio_table_size = _thread_aio_dtablesize;
	}

	/* Make sure this entry is available for use */
	while ((dp = _thread_aio_table[fd]) != NULL) {
		/* Should be closed */
		if (dp->aiofd_object == NULL)
			abort();
		/* If I/O was in progress wait for cleanup */
		_thread_kern_block(&dp->aiofd_closewait, PS_CLOSE_WAIT, NULL);
	}
		
	/* Allocate and initialize a new entry */
	if ((dp = calloc(1, sizeof(*dp))) == NULL)
		return (ENOMEM);
	dp->aiofd_fdflags = flags;
	_thread_queue_init(&dp->aiofd_ractive);
	_thread_queue_init(&dp->aiofd_wactive);

	/* Allocate per-object information and grab the flags */
	if ((dp->aiofd_object = calloc(1, sizeof(*dp->aiofd_object))) == NULL) {
		free(dp);
		return (ENOMEM);
	}
	dp->aiofd_object->aiofo_refcount++;
	if ((flags = _syscall_sys_fcntl(fd, F_GETFL, 0)) != -1)
		dp->aiofd_object->aiofo_oflags = flags;

	/* And update table */
	_thread_aio_table[fd] = dp;

	return (0);
}

/*
 * Update the table to reflect changes resulting from a dup(2),
 * dup2(2), or fcntl(F_DUPFD).  We will block waiting for pending I/O
 * on the old descriptor if necessary.
 */
int
_thread_aio_entry_dup(dupfd, newfd)
	int dupfd;
	int newfd;
{
	thread_aio_desc_t *dup, *new;
	int flags;

	TR("_thread_aio_entry_dup", dupfd, newfd);

	if ((dup = _thread_aio_table[dupfd]) == NULL)
		abort();

	if ((new = _thread_aio_table[newfd]) != NULL) {
		/* If open, free old descriptor */
		if (new->aiofd_object != NULL)
			_thread_aio_entry_destroy(newfd);
		/* Wait for cleanup to complete */
		while ((new = _thread_aio_table[newfd]) != NULL)
			_thread_kern_block(&new->aiofd_closewait, PS_CLOSE_WAIT,
			    NULL);
	}

	/* Allocate and initialize a new entry */
	if ((new = calloc(1, sizeof(*new))) == NULL)
		return (ENOMEM);
	_thread_aio_table[newfd] = new;
	_thread_queue_init(&new->aiofd_ractive);
	_thread_queue_init(&new->aiofd_wactive);

	/* Get new flags and set non-blocking */
	flags = _syscall_sys_fcntl(newfd, F_GETFD, 0);
	_syscall_sys_fcntl(newfd, F_SETFD, flags | FD_NONBLOCK);

	/* Reference the per-object information from dupfd */
	new->aiofd_object = dup->aiofd_object;
	new->aiofd_object->aiofo_refcount++;
	/* Refresh the flags */
	if ((flags = _syscall_sys_fcntl(newfd, F_GETFL, 0)) != -1)
		new->aiofd_object->aiofo_oflags = flags;

	return (0);
}

/*
 * Clean up an entry due to close() or dup().  Must only be called
 * with a valid entry.
 */
void
_thread_aio_entry_destroy(fd)
	u_int fd;
{
	thread_aio_desc_t *dp;

	TR("_thread_aio_entry_destroy", fd, 0);

	/* Make sure the entry is valid */
	if ((dp = _thread_aio_table[fd]) == NULL || dp->aiofd_object == NULL)
		abort();

	/* Unlink from per-object info and free if last reference */
	if (--dp->aiofd_object->aiofo_refcount <= 0)
		free(dp->aiofd_object);
	dp->aiofd_object = NULL;

	/* Wait for all the outstanding references to clear */
	if (dp->aiofd_rcount != 0 || dp->aiofd_wcount != 0 ||
	    dp->aiofd_ecount != 0) {

		THREAD_EVT_OBJ_USER_1_IF((dp->aiofd_wactive.q_head ||
		    dp->aiofd_ractive.q_head || _thread_selectq.q_head),
		    EVT_LOG_PTRACE_THREAD, EVENT_THREAD_AIO_ENTRY_DESTROY_WUP,
		    fd);

		/* If there were threads blocked wake them up */
		while (_thread_kern_wakeup(&dp->aiofd_wactive))
			continue;
		while (_thread_kern_wakeup(&dp->aiofd_ractive))
			continue;
		/*
		 * Wake up all the threads queued in a select so they can check their
		 * fds in case one or more have been closed.
		 */ 
		while (_thread_kern_wakeup(&_thread_selectq))
			continue;

		/* Wait for all references to be released */
		while (dp->aiofd_rcount != 0 || dp->aiofd_wcount != 0 ||
		    dp->aiofd_ecount != 0)
			_thread_kern_block(&dp->aiofd_close, PS_CLOSE_WAIT, 
			    NULL);

		THREAD_EVT_OBJ_USER_1_IF((dp->aiofd_closewait.q_head),
		    EVT_LOG_PTRACE_THREAD, EVENT_THREAD_AIO_ENTRY_DESTROY_CWUP,
		    fd);
		/* Wake up any threads that want this slot */
		while (_thread_kern_wakeup(&dp->aiofd_closewait))
			continue;
	}

	/* And release the memory */
	free(dp);
	_thread_aio_table[fd] = NULL;
}

/*
 * Poll for the state of I/O.  Should always be called with the
 * thread kernel locked as it will manipulate the run queue if
 * threads are ready for I/O.
 */
void
_thread_aio_poll(canblock)
	int canblock;
{
	int start, nbits, erc;
	struct timeval *tvp, tv;
	fd_set *rp, *wp, *ep;
	fd_set inbits, outbits, excbits;

 	TR("_thread_aio_poll", _thread_selfp(), canblock);
 	TR("_thread_aio_poll_kern_lock", _thread_kern_owner,
	    _thread_selfp()->state);

	rp = wp = ep = NULL;
	tvp = &tv;
	nbits = 0;
	for (start = 0; start < _thread_aio_table_size; start++) {
		thread_aio_desc_t *dp;

		if ((dp = _thread_aio_table[start]) == NULL
		    || dp->aiofd_object == NULL)
			continue;

#define	aio_setbit(bit, xp, fxp, mxp) \
	do { \
		if ((xp) == NULL) { \
			if (_thread_aio_table_size <= FD_SETSIZE) { \
				(xp) = (fxp); \
				FD_ZERO(xp); \
			} else { \
				if ((mxp) == NULL) \
					(mxp) = FD_ALLOC(_thread_aio_table_size); \
				else \
					FD_NZERO(_thread_aio_table_size, mxp); \
				if ((mxp) == NULL) \
					goto out; \
				(xp) = (mxp); \
			} \
		} \
		FD_SET(bit, xp); \
	} while (0)

		if (dp->aiofd_rcount > 0) {
			aio_setbit(start, rp, &inbits, _thread_mrp);
		}
		if (dp->aiofd_wcount > 0) {
			aio_setbit(start, wp, &outbits, _thread_mwp);
		}
		if (dp->aiofd_ecount > 0 || dp->aiofd_rcount > 0 ||
		    dp->aiofd_wcount > 0) {
			aio_setbit(start, ep, &excbits, _thread_mep);
			nbits = start;
		}
	}

	if ((rp == NULL) && (wp == NULL) && (ep == NULL)) {
		if (canblock == 0)
			goto out;
		else
			nbits = 0;
	} else
		nbits++;		/* Adjust for zero based bitstrings */

 	TR("_thread_aio_poll_nbits", nbits, 0);

#ifdef DEBUG
	if (rp)
		TR("_thread_aio_poll_rp", rp, 0);
	if (wp)
		TR("_thread_aio_poll_wp", wp, 0);
	if (ep)
		TR("_thread_aio_poll_ep", ep, 0);
#endif

	erc = 0;
	if (canblock == 0)
		timerclear(&tv);
	else {
		tvp->tv_sec = 1;
		tvp->tv_usec = 0;
	}
	
	erc = _syscall_sys_select(nbits, rp, wp, ep, tvp);

	TR("_thread_aio_poll_erc", erc, 0);

	if (erc <= 0)
		goto out;

	/* 
	 * I/O Pending.  Wakeup any "select" threads and signal
	 * any active file descriptors.
	 */
	THREAD_EVT_OBJ_USER_0_IF((_thread_selectq.q_head),
	    EVT_LOG_PTRACE_THREAD, EVENT_THREAD_AIO_POLL_WUP);
	while (_thread_kern_wakeup(&_thread_selectq))
		continue;

	for (start = 0; start < nbits; start++) {
		thread_aio_desc_t *dp;
		int wake_read, wake_write;

		if ((dp = _thread_aio_table[start]) == NULL
		    || dp->aiofd_object == NULL)
			continue;

		wake_read = wake_write = 0;
		if (rp != NULL && FD_ISSET(start, rp) != 0)
			wake_read++;
		if (wp != NULL && FD_ISSET(start, wp) != 0)
			wake_write++;
		if (ep != NULL && FD_ISSET(start, ep) != 0)
			wake_read = wake_write = 1;

		THREAD_EVT_OBJ_USER_0_IF(
		    ((wake_read && dp->aiofd_ractive.q_head) ||
		    (wake_write && dp->aiofd_wactive.q_head)),
		    EVT_LOG_PTRACE_THREAD, EVENT_THREAD_AIO_POLL_WUP);
		if (wake_read)
			while (_thread_kern_wakeup(&dp->aiofd_ractive))
				continue;
		if (wake_write)
			while (_thread_kern_wakeup(&dp->aiofd_wactive))
				continue;
	}

out:
 	TR("_thread_aio_poll_end", _thread_selfp(), erc);
}

/*
 * Suspend the calling thread on an I/O operation.  
 * Should be called with the kernel monitor locked.  
 * Re-aquires the monitor lock after return from block.
 */

int
_thread_aio_suspend(fd, flags)
	int fd;
	int flags;
{
	thread_aio_desc_t *dp;
	struct pthread_queue *queue;
	int canned;
	int erc;

	TR("_thread_aio_suspend", fd, flags);

	if ((dp = _thread_aio_table[fd]) == NULL)
		abort();
	if ((flags & FD_READ) != 0) {
		dp->aiofd_rcount++;
		queue = &dp->aiofd_ractive;
	} else if ((flags & FD_WRITE) != 0) {
		dp->aiofd_wcount++;
		queue = &dp->aiofd_wactive;
	} else {
		_thread_kern_exit();
		return (EINVAL);
	}

	erc = _thread_kern_block(queue, PS_IO_WAIT, NULL);
	canned = (erc == ETHREAD_CANCELED);

	TR("_thread_aio_suspend_blkret1", erc, 0);

	if ((flags & FD_READ) != 0) 
		dp->aiofd_rcount--;
	else if ((flags & FD_WRITE) != 0)
		dp->aiofd_wcount--;

	/* If someone is waiting for us to finish with this fd, notify them */
	if (dp->aiofd_object == NULL) {
		erc = EBADF;
		if (dp->aiofd_rcount == 0 && dp->aiofd_wcount == 0 &&
		    dp->aiofd_ecount == 0) {
			THREAD_EVT_OBJ_USER_1_IF((dp->aiofd_close.q_head),
			    EVT_LOG_PTRACE_THREAD,
			    EVENT_THREAD_AIO_SUSPEND_CWUP, fd);
			while (_thread_kern_wakeup(&dp->aiofd_close))
				continue;
		}
	}

	if (canned)
		_thread_cancel();
	else if (erc != 0)
		errno = erc;
		
	return (erc);
}
