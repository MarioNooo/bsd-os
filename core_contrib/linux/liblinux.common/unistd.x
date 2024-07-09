/*	BSDI unistd.x,v 1.6 2003/09/23 17:46:34 donn Exp	*/

/*
 * Transformations for functions in unistd.h.
 */

include "types.xh"

%{
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <limits.h>
#include <time.h>

#include "liblinux.h"
%}

/*
 * POSIX interfaces.
 * We list them alphabetically.
 * Note that the usual libc5 versus glibc naming issues apply here,
 * and also as usual, we use the more elaborate libc5 versions.
 */

int _exit(int status) = exit;

/* The kernel syscall name doesn't have the extra underscore.  */
int __kernel_exit(int status);

int access(const char *path, int mode);

/* Stolen directly from the BSD implementation.  */
unsigned int alarm(unsigned int secs)
{
	struct itimerval it, oitv;
	struct itimerval *itp = &it;
	int r;

	timerclear(&itp->it_interval);
	itp->it_value.tv_sec = secs;
	itp->it_value.tv_usec = 0;
	if (__bsdi_error(r =
	    __bsdi_syscall(SYS_setitimer, ITIMER_REAL, itp, &oitv)))
		return (r);
	if (oitv.it_value.tv_usec)
		oitv.it_value.tv_sec++;
	return (oitv.it_value.tv_sec);
}

int chdir(const char *path);

int chown(const char *path, uid_t uid, gid_t gid);

int close(int fd);

int dup(int fd);

int dup2(int old, int new);

int execve(const char *path, char **argv, char **envp);

/* The fork() syscall returns two parameters, so it's special.  */
pid_t fork() { return (__bsdi_fork()); }

gid_t getegid();

uid_t geteuid();

gid_t getgid();

int getgroups(int ngroups, gid_t *gidset)
{
#ifdef LIBC5
	gid_t *bsdi_gidset;
	int i;
	int r;

	if (ngroups < 0)
		return (EINVAL);
	if (ngroups == 0)
		bsdi_gidset = NULL;
	else {
		if (ngroups > NGROUPS_MAX)
			ngroups = NGROUPS_MAX;
		bsdi_gidset = alloca(ngroups * sizeof (gid_t));
	}

	if (__bsdi_error(r =
	    __bsdi_syscall(SYS_getgroups, ngroups, bsdi_gidset)))
		return (r);
	if (ngroups != 0)
		for (i = 0; i < r; ++i)
			gidset[i] = bsdi_gidset[i];
	return (r);
#else
	return (__bsdi_syscall(SYS_getgroups, ngroups, gidset));
#endif
}

int __kernel_getgroups(unsigned int ngroups, kernel_gid_t *gidset) {
	gid_t *bsdi_gidset;
	int i;
	int r;

	if (ngroups < 0)
		return (EINVAL);
	if (ngroups == 0)
		bsdi_gidset = NULL;
	else {
		if (ngroups > NGROUPS_MAX)
			ngroups = NGROUPS_MAX;
		bsdi_gidset = alloca(ngroups * sizeof (gid_t));
	}

	if (__bsdi_error(r =
	    __bsdi_syscall(SYS_getgroups, ngroups, bsdi_gidset)))
		return (r);
	if (ngroups != 0)
		for (i = 0; i < r; ++i)
			gidset[i] = bsdi_gidset[i];
	return (r);
}

/* XXX BSD no longer has an equivalent to getpgid(pid).  */
cookie int linux_pgidpid_t {
	PID_CURRENT		0;
};
pid_t getpgid(PID_CURRENT) { return (__bsdi_syscall(SYS_getpgrp)); }
pid_t getpgid(linux_pgidpid_t p) = EINVAL;

pid_t getpgrp();

pid_t getpid();

pid_t getppid();

uid_t getuid();

int link(const char *path_old, const char *path_new);

/* The bizarre Linux version of BSD lseek().  */
linux_loff_t llseek(int fd, linux_loff_t offset, int whence)
{
	return (__bsdi_qsyscall(SYS_lseek, fd, 0, offset, whence));
}

/*
 * The even-more-bizarre kernel version of llseek() swaps the offset words,
 * adds a result parameter and inserts an underscore in front of the name.
 */
int __kernel__llseek(int fd, unsigned long o_high, unsigned long o_low,
    linux_loff_t *result, int whence) {
	off_t r;

	r = __bsdi_qsyscall(SYS_lseek, fd, 0, ((off_t)o_high << 32) + o_low,
	    whence);
	if (__bsdi_error_ll(r))
		return (r);
	*result = r;
	return (0);
}

/* Lseek() with 32-bit off_t types.  */
off_t lseek(int fd, off_t offset, int whence)
{
	return (__bsdi_qsyscall(SYS_lseek, fd, 0, offset, whence));
}

int pause()
{
	sigset_t ss;

	sigemptyset(&ss);
	return (__bsdi_syscall(SYS_sigsuspend, &ss));
}

int pipe(int *p) { return (__bsdi_pipe(p)); }

ssize_t read(int fd, void *buf, size_t nbytes);

int rmdir(const char *path);

int setgid(gid_t gid);

int setpgid(pid_t pid, pid_t pgid);

int setpgrp(pid_t pid, pid_t pgid) { return setpgid(pid, pgid); }

pid_t setsid();

int setuid(uid_t uid);

int unlink(const char *path);

ssize_t write(int fd, const void *buf, size_t nbytes);

/*
 * Extensions to POSIX.
 */

int acct(const char *path);

int chroot(const char *path);

int fchdir(int fd);

int fchown(int fd, uid_t uid, gid_t gid);

int fsync(int fd);

int ftruncate(int fd, off_t nbytes)
{
	return (__bsdi_qsyscall(SYS_ftruncate, fd, 0, nbytes));
}

/* This library call under Linux uses static defines, so we replace it.  */
int getpagesize()
{
	size_t size;
	int mib[2];
	int value;
	int r;

	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	size = sizeof (value);
	r = __bsdi_syscall(SYS___sysctl, mib, 2, &value, &size, NULL, 0);
	if (__bsdi_error(r))
		return (r);
	return (value);
}

/* Modified from the BSD nice(2) library routine.  */
int nice(int incr)
{
	int prio;

	/* The return value includes the LINUX_PZERO 'feature'. */
	prio = __bsdi_getpriority(PRIO_PROCESS, 0);
	if (__bsdi_error(prio))
		return (prio);
	return (__bsdi_syscall(SYS_setpriority, PRIO_PROCESS, 0,
	    (prio - LINUX_PZERO) + incr));
}

int readlink(const char *path, char *buf, int nbytes);

cookie int reboot_t {
	RB_AUTOBOOT		0x01234567;
	RB_HALT			0xcdef0123;
	LINUX_RB_ENABLE_CAD	0x89abcdef;
	LINUX_RB_DISABLE_CAD	0;
};
int reboot(RB_AUTOBOOT);
int reboot(RB_HALT);
int reboot(LINUX_RB_ENABLE_CAD) = 0;	/* ignore it for now */
int reboot(LINUX_RB_DISABLE_CAD) = 0;	/* ignore it for now */
int reboot(reboot_t r) = EINVAL;

int setgroups(size_t ngroups, const gid_t *gidset)
{
#ifdef LIBC5
	gid_t *bsdi_gidset;
	int i;
	int r;

	if (ngroups > NGROUPS_MAX)
		return (EINVAL);
	bsdi_gidset = ngroups == 0 ? NULL :
	    alloca(ngroups * sizeof (gid_t));

	for (i = 0; i < ngroups; ++i)
		bsdi_gidset[i] = gidset[i];
	return (__bsdi_syscall(SYS_setgroups, ngroups, bsdi_gidset));
#else
	return (__bsdi_syscall(SYS_setgroups, ngroups, gidset));
#endif
}

int __kernel_setgroups(size_t ngroups, const kernel_gid_t *gidset) {
	gid_t *bsdi_gidset;
	int i;
	int r;

	if (ngroups > NGROUPS_MAX)
		return (EINVAL);
	bsdi_gidset = ngroups == 0 ? NULL :
	    alloca(ngroups * sizeof (gid_t));

	for (i = 0; i < ngroups; ++i)
		bsdi_gidset[i] = gidset[i];
	return (__bsdi_syscall(SYS_setgroups, ngroups, bsdi_gidset));
}

/* Taken from the BSD sethostname(2) routine.  */
int sethostname(const char *name, int length_name)
{
	int mib[2];
	int r;

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	r = __bsdi_syscall(SYS___sysctl, mib, 2, NULL, NULL, (void *)name,
	    length_name);
	if (__bsdi_error(r))
		return (r);
	return (0);
}

/*
 * People using this interface usually want to swap between
 * real and effective [UG]IDs, which we can't do, so we just ignore
 * requests to change the real [UG]ID through this interface.
 */
int setregid(gid_t rgid, gid_t egid)
{
	if (egid == -1)
		return (0);
	return (__bsdi_syscall(SYS_setegid, egid));
}

int setreuid(uid_t ruid, uid_t euid)
{
	if (euid == -1)
		return (0);
	return (__bsdi_syscall(SYS_seteuid, euid));
}

int swapoff() = EPERM;

int swapon(const char *path);

int symlink(const char *from, const char *to);

int sync();

int truncate(const char *path, off_t nbytes)
{
	return (__bsdi_qsyscall(SYS_truncate, path, 0, nbytes));
}
