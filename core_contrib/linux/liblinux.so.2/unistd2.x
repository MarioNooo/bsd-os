/*	BSDI unistd2.x,v 1.2 2001/01/23 04:05:57 donn Exp	*/

/*
 * Support for glibc-only functions in (or related to) unistd.h.
 */

include "types.xh"

%{
#include <sys/stat.h>
%}

/* Change ownership but don't follow links.  We ignore links.  */
int lchown(char *file, uid_t owner, gid_t group)
{
	struct stat s;

	if (__bsdi_syscall(SYS_lstat, file, &s) == 0 && S_ISLNK(s.st_mode))
		return (0);
	return (__bsdi_syscall(SYS_chown, owner, group));
}

/*
 * Glibc will emulate these functions if we return ENOSYS for them.
 */
ssize_t __kernel_getcwd(char *buf, size_t size) = ENOSYS;

ssize_t __kernel_pread(int f, void *buf, size_t size, unsigned long olow,
    unsigned long ohigh) = ENOSYS;

ssize_t __kernel_pwrite(int f, const void *buf, size_t size,
    unsigned long olow, unsigned long ohigh) = ENOSYS;
