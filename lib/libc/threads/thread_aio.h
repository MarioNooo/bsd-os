/*-
 * Copyright (c) 1996, 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_aio.h,v 1.4 2000/08/24 15:54:29 jch Exp
 */

/*
 * Private thread definitions for the thread kernel.
 */

#ifndef _THREAD_AIO_H_
#define _THREAD_AIO_H_

/*
 * File descriptor table structure.  We maintain a table of file
 * descriptors that is sized to the current RLIMIT_NOFILE on demand.
 * Each entry in the table represents a file descriptor.  File
 * descriptors entries point to a per-object entry that contains the
 * flags (which are shared between dup'd file descriptors).
 *
 * A file descriptor that is not in use will have a NULL pointer in the
 * table.  A file descriptor that has been closed and is pending
 * notification of threads with pending I/O will have a NULL pointer
 * to the per-object entry.
 */

typedef struct {
	u_int		aiofo_refcount;		/* Number of references (via dup) */
	u_int		aiofo_oflags;		/* Flags used in open. */
} thread_aio_object_t;

typedef struct {
	struct pthread_queue	aiofd_ractive;	/* Threads blocked on read */
	struct pthread_queue	aiofd_wactive;	/* Threads blocked on write */
	struct pthread_queue	aiofd_close;	/* Thread blocked closing file */
	struct pthread_queue	aiofd_closewait; /* Thread waiting for slot */
	thread_aio_object_t	*aiofd_object;	/* Per-object data (shared for duped fds) */
	short			aiofd_rcount;	/* # threads waiting on read */
	short			aiofd_wcount;	/* # threads waiting on write */
	short			aiofd_ecount;	/* # threads waiting on exceptions */
	u_char			aiofd_fdflags;	/* kernel's per-fd flags */
} thread_aio_desc_t;

#define	FD_READ		0x0001		/* Block on the read queue */
#define	FD_WRITE	0x0002		/* Block on the write queue */

#define	THREAD_AIO(fd, dir, ret, call, action) {				\
	thread_aio_desc_t *Xdp;							\
	u_int Xfd = (fd);							\
	if (Xfd >= _thread_aio_table_size ||					\
	    (Xdp = _thread_aio_table[(fd)]) == NULL ||				\
	    Xdp->aiofd_object == NULL) {					\
	    ret = -1; errno = EBADF;						\
	    goto Xaction;							\
	}									\
	while (((ret) = (call)) == -1) {					\
		if ((errno == EWOULDBLOCK || errno == EAGAIN) &&		\
		    (Xdp->aiofd_fdflags & FD_NONBLOCK) == 0 &&			\
		    (Xdp->aiofd_object->aiofo_oflags & O_NONBLOCK) == 0 &&	\
		    _thread_aio_suspend((fd), (dir)) == 0)			\
			continue;						\
	    Xaction:								\
		action;								\
	}									\
}

extern  thread_aio_desc_t **_thread_aio_table;
extern	int _thread_aio_dtablesize;
extern	int _thread_aio_table_size;
extern 	struct pthread_queue _thread_selectq;

__BEGIN_DECLS
void 	_thread_aio_entry_destroy __P((u_int));
int 	_thread_aio_entry_dup __P((int, int));
int 	_thread_aio_entry_init __P((u_int));
void 	_thread_aio_poll __P((int));
int	_thread_aio_suspend __P((int, int));
int 	_thread_aio_table_init __P((void));
__END_DECLS

#endif  
