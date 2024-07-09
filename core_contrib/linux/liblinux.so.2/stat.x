/*	BSDI stat.x,v 1.4 2000/12/08 03:47:36 donn Exp	*/

/*
 * Transformation rules for <sys/stat.h> syscalls.
 */

include "types.xh"
include "stat.xh"

/*
 * Linux's kernel types don't match its user types.
 * The GNU C library performs transformations from the kernel types
 * to the user types.  We interpose the GNU C library stubs rather
 * than the Linux kernel stubs, so that we don't transform twice.
 */

int __xstat(_STAT_VER_LINUX_OLD, const char *name, struct old_stat *buf)
{
	return (__bsdi_syscall(SYS_stat, name, buf));
}
int __xstat(_STAT_VER_LINUX, const char *name, struct stat *buf)
{
	return (__bsdi_syscall(SYS_stat, name, buf));
}
int __xstat(linux_stat_ver_t v, const char *name, struct stat *buf) = EINVAL;

int __fxstat(_STAT_VER_LINUX_OLD, int fd, struct old_stat *buf)
{
	return (__bsdi_syscall(SYS_fstat, fd, buf));
}
int __fxstat(_STAT_VER_LINUX, int fd, struct stat *buf)
{
	return (__bsdi_syscall(SYS_fstat, fd, buf));
}
int __fxstat(linux_stat_ver_t v, int fd, struct stat *buf) = EINVAL;

int __lxstat(_STAT_VER_LINUX_OLD, const char *name, struct old_stat *buf)
{
	return (__bsdi_syscall(SYS_lstat, name, buf));
}
int __lxstat(_STAT_VER_LINUX, const char *name, struct stat *buf)
{
	return (__bsdi_syscall(SYS_lstat, name, buf));
}
int __lxstat(linux_stat_ver_t v, const char *name, struct stat *buf) = EINVAL;

/*
 * When we handle an inline syscall,
 * we DO have to implement the kernel version.
 */
int __kernel_stat(const char *name, struct old_stat *buf);
int __kernel_fstat(int fd, struct old_stat *buf);
int __kernel_lstat(const char *name, struct old_stat *buf);

/*
 * The other stat.h calls...
 */

int chmod(const char *path, mode_t mode);

int fchmod(int fd, mode_t mode);

int mkdir(const char *path, mode_t mode);

unsigned short umask(mode_t m);
