/*	BSDI stat.x,v 1.2 2000/12/08 04:24:01 donn Exp	*/

/*
 * Transformation rules for <sys/stat.h> syscalls.
 */

include "types.xh"
include "stat.xh"

int stat(const char *name, struct stat *buf);

int fstat(int fd, struct stat *buf);

int lstat(const char *name, struct stat *buf);

/*
 * The _x*stat() functions were introduced in libc5,
 * but they didn't really do anything interesting.
 * The only difference with respect to glibc is that
 * libc5 supports only the _STAT_VER_LINUX_OLD version.
 */

int _xstat(_STAT_VER_LINUX_OLD, const char *name, struct stat *buf)
{
	return (__bsdi_syscall(SYS_stat, name, buf));
}
int _xstat(linux_stat_ver_t v, const char *name, struct stat *buf) = EINVAL;

int _fxstat(_STAT_VER_LINUX_OLD, int fd, struct stat *buf)
{
	return (__bsdi_syscall(SYS_fstat, fd, buf));
}
int _fxstat(linux_stat_ver_t v, int fd, struct stat *buf) = EINVAL;

int _lxstat(_STAT_VER_LINUX_OLD, const char *name, struct stat *buf)
{
	return (__bsdi_syscall(SYS_lstat, name, buf));
}
int _lxstat(linux_stat_ver_t v, const char *name, struct stat *buf) = EINVAL;

/*
 * The other stat.h calls...
 */

int chmod(const char *path, mode_t mode);

int fchmod(int fd, mode_t mode);

int mkdir(const char *path, mode_t mode);

unsigned short umask(mode_t m);
