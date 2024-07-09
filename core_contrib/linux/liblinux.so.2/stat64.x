/*	BSDI stat64.x,v 1.1 2000/02/12 18:12:28 donn Exp	*/

/*
 * Transformation rules for <sys/stat.h> syscalls.
 */

include "types.xh"
include "stat64.xh"


/*
 * Linux's kernel types don't match its user types.
 * The GNU C library performs transformations from the kernel types
 * to the user types.  We interpose the GNU C library stubs rather
 * than the Linux kernel stubs, so that we don't transform twice.
 */

int __xstat64(_STAT_VER_LINUX, const char *name, struct stat64 *buf)
{
	return (__bsdi_syscall(SYS_stat, name, buf));
}
int __xstat64(linux_stat_ver_t v, const char *name, struct stat64 *buf) =
    EINVAL;

int __fxstat64(_STAT_VER_LINUX, int fd, struct stat64 *buf)
{
	return (__bsdi_syscall(SYS_fstat, fd, buf));
}
int __fxstat64(linux_stat_ver_t v, int fd, struct stat64 *buf) = EINVAL;

int __lxstat64(_STAT_VER_LINUX, const char *name, struct stat64 *buf)
{
	return (__bsdi_syscall(SYS_lstat, name, buf));
}
int __lxstat64(linux_stat_ver_t v, const char *name, struct stat64 *buf) =
    EINVAL;
