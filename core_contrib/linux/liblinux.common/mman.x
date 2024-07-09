/*	BSDI mman.x,v 1.5 2001/06/23 18:18:44 donn Exp	*/

/*
 * Mmap()-related transforms.
 */

include "types.xh"
include "mman.xh"


/*
 * Tricky: we assume that a call to mmap() with MAP_GROWSDOWN
 * is creating a thread stack, so we cheat and create
 * a full thread stack instead of the partial one that was requested.
 */

%{

#include <sys/param.h>

/* From the Linux threads package...  */
#define	INITIAL_STACK_SIZE	(16 * 1024)
#define	STACK_SIZE		(2 * 1024 * 1024 - CLBYTES)
%}

/* This will produce an ignorable warning from transform.  */
void *mmap(const void *addr, size_t nbytes, int prot, LINUX_MAP_GROWSDOWN,
    int fd, off_t o)
{
#if 0
	if (addr == NULL || nbytes != INITIAL_STACK_SIZE)
		abort();
#endif

	addr = (void *)
	    ((((unsigned long)addr & ~CLOFSET) + nbytes) - STACK_SIZE);
	nbytes = STACK_SIZE;

	return ((void *)__bsdi_syscall(SYS_mmap, addr, nbytes, prot,
	    MAP_PRIVATE|MAP_ANON|MAP_FIXED, fd, 0, o));
}

/*
 * Here is the generic mmap() call, tweaked for 64-bit structure alignment.
 * This will produce an ignorable warning from transform.
 */
void *mmap(const void *addr, size_t nbytes, int prot, map_t flags,
    int fd, off_t o)
{
	return ((void *)__bsdi_syscall(SYS_mmap, addr, nbytes, prot, flags,
	    fd, 0, o));
}

int mprotect(const void *buf, size_t len, int prot);

int munmap(const void *buf, size_t len);

/* We currently ignore msync flags... */
int msync(const void *addr, size_t len, int flags);

int mlock(const void *addr, size_t len);

int munlock(const void *addr, size_t len);

/*
 * Linux malloc() uses mremap(), but it 'fails softly'
 * if mremap() doesn't work.
 */
int mremap(const void *addr, size_t old_len, size_t new_len, int may_move) = ENOSYS;

/*
 * XXX mlockall()?
 * XXX munlockall()?
 */
