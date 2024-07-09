/*	BSDI dirfix.h,v 1.2 2001/06/24 16:51:26 donn Exp	*/

/*
 * Tweak the symbols used by the BSD dirent functions so that
 * they can be used in the Linux emulation library.
 */

#define	closedir	__bsdi_closedir
#define	_dd_hash	__bsdi_dd_hash
#define	opendir		__bsdi_opendir
#define	__opendir2	__bsdi_opendir2
#define	readdir		__bsdi_readdir
#define	rewinddir	__bsdi_rewinddir
#define	seekdir		__bsdi_seekdir
#define	telldir		__bsdi_telldir

#define	free		__libc_free
#define	malloc		__libc_malloc
#define	realloc		__libc_realloc

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"

#define	__bsdi_set_errno(v)	({ \
	int __v = (v); \
	__bsdi_error(__v) ? \
		errno = __bsdi_error_val(__v), -1 : __v; \
})

#define	__bsdi_set_errno_ll(v)	({ \
	long long __v = (v); \
	__bsdi_error_ll(__v) ? \
		errno = __bsdi_error_val(__v), -1LL : __v; \
})

/*
 * Since these calls are inlined in the libc5 dirent routines,
 * we don't have to make them interposable.
 */
#define	close(fd) \
	__bsdi_set_errno(__bsdi_syscall(SYS_close, (fd)))
#define	fcntl(fd, cmd, flag) \
	__bsdi_set_errno(__bsdi_syscall(SYS_fcntl, (fd), (cmd), (flag)))
#define	fstat(fd, sb) \
	__bsdi_set_errno(__bsdi_syscall(SYS_fstat, (fd), (sb)))
#define	fstatfs(fd, buf) \
	__bsdi_set_errno(__bsdi_syscall(SYS_fstatfs, (fd), (buf)))
#define	getdirentries(fd, buf, nbytes, basep) \
	__bsdi_set_errno(__bsdi_syscall(SYS_getdirentries, (fd), (buf), \
	    (nbytes), (basep)))
#define	lseek(fd, offset, whence) \
	__bsdi_set_errno_ll(__bsdi_qsyscall(SYS_lseek, (fd), 0, (offset), \
	    (whence)))
#define	open(name, flags) \
	__bsdi_set_errno(__bsdi_syscall(SYS_open, (name), (flags)))

#define	_thread_qlock_init(vpp)		0
#define	_thread_qlock_lock(vpp)		0
#define	_thread_qlock_unlock(vpp)	0
#define	_thread_qlock_free(vpp)		0
#define	_threads_initialized		0

/* XXX Port mergesort()?  */
#define	mergesort	qsort
