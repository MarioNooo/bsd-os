/*	BSDI mman64.x,v 1.1 2000/02/12 18:12:27 donn Exp	*/

/*
 * Mmap() with a 64-bit offset.
 */

include "types.xh"
include "mman.xh"

/* We lie and give the return value as 'int' to avoid errno problems.  */
int mmap64(const void *addr, size_t nbytes, int prot, map_t flags, int fd,
    off64_t o)
{
	return (__bsdi_qsyscall(SYS_mmap, addr, nbytes, prot, flags, fd, 0, o));
}
