/*	BSDI mknod.x,v 1.2 2000/02/12 18:09:17 donn Exp	*/

/*
 * This gets done a little differently in glibc than in libc5,
 * so we split the implementation out from unistd.x.
 */

include "types.xh"
include "stat.xh"

cookie int linux_mknod_ver_t {
	_MKNOD_VER_LINUX	1;
};
int __xmknod(_MKNOD_VER_LINUX, const char *path, mode_t mode, const dev_t *pdev)
{
	dev_t dev = pdev == NULL ? 0 : dev_t_in(*pdev);

	if (S_ISFIFO(mode))
		return (__bsdi_syscall(SYS_mkfifo, path, mode));
	return (__bsdi_syscall(SYS_mknod, path, mode, dev));
}
int __xmknod(linux_mknod_ver_t v, const char *path, mode_t mode,
    const dev_t *pdev) = EINVAL;

int __kernel_mknod(const char *path, mode_t mode, kernel_dev_t dev) {
	if (S_ISFIFO(mode))
		return (__bsdi_syscall(SYS_mkfifo, path, mode));
	return (__bsdi_syscall(SYS_mknod, path, mode, dev));
}
