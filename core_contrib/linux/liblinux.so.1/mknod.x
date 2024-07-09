/*	BSDI mknod.x,v 1.1.1.1 1998/12/23 19:57:20 prb Exp	*/

/*
 * Mknod()-related functions.  This was split out from unistd.x
 * because libc5 and glibc do things slightly differently.
 */

include "types.xh"
include "stat.xh"

cookie int linux_mknod_ver_t {
	_MKNOD_VER_LINUX	1;
};
int _xmknod(_MKNOD_VER_LINUX, const char *path, mode_t mode, const dev_t *pdev)
{
	dev_t dev = pdev == NULL ? 0 : dev_t_in(*pdev);

	if (S_ISFIFO(mode))
		return (__bsdi_syscall(SYS_mkfifo, path, mode));
	return (__bsdi_syscall(SYS_mknod, path, mode, dev));
}
int _xmknod(linux_mknod_ver_t v, const char *path, mode_t mode,
    const dev_t *pdev) = EINVAL;

int mknod(const char *path, mode_t mode, dev_t dev)
{
	if (S_ISFIFO(mode))
		return (__bsdi_syscall(SYS_mkfifo, path, mode));
	return (__bsdi_syscall(SYS_mknod, path, mode, dev));
}
