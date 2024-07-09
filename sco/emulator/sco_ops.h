/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_ops.h,v 2.4 1996/08/20 17:39:27 donn Exp
 */

/*
 * The structure of the emulator operator tables
 * (virtual function tables).
 * Both SCO and BSD ops are represented.
 */

struct dirent;
struct fhandle;
struct iovec;
struct itimerval;
struct msghdr;
struct pollfd;
struct rlimit;
struct rusage;
struct scoutsname;
struct semaphore;
struct sgttyb;
struct sigaction;
struct sockaddr;
struct stat;
struct statfs;
struct strbuf;
struct timeb;
struct timeval;
struct timezone;
struct tms;
struct utimbuf;

struct fdops {
	int (*accept) __P((int, struct sockaddr *, int *));
	int (*bind) __P((int, const struct sockaddr *, int));
	int (*close) __P((int));
	int (*connect) __P((int, const struct sockaddr *, int));
	int (*dup) __P((int));
	int (*dup2) __P((int, int));
	int (*fchdir) __P((int));
	int (*fchflags) __P((int, unsigned long));
	int (*fchmod) __P((int, mode_t));
	int (*fchown) __P((int, int, int));
	int (*fcntl) __P((int, int, int));
	int (*flock) __P((int, int));
	long (*fpathconf) __P((int, int));
	int (*fstat) __P((int, struct stat *));
	int (*fstatfs) __P((int, struct statfs *, int, int));
	int (*fsync) __P((int));
	int (*ftruncate) __P((int, off_t));
	int (*getdents) __P((int, struct dirent *, size_t));
	int (*getdirentries) __P((int, char *, unsigned, long *));
	int (*getmsg) __P((int, struct strbuf *, struct strbuf *, int *));
	int (*getpeername) __P((int, struct sockaddr *, int *));
	int (*getsockname) __P((int, struct sockaddr *, int *));
	int (*getsockopt) __P((int, int, int, void *, int *));
	int (*gtty) __P((int, struct sgttyb *));
	void (*init) __P((int, const char *, int));
	int (*ioctl) __P((int, unsigned long, int));
	int (*listen) __P((int, int));
	off_t (*lseek) __P((int, off_t, int));
	caddr_t (*mmap) __P((caddr_t, size_t, int, int, int, off_t));
	int (*nfssvc) __P((int, char *, int, char *, int));
	int (*putmsg)
		__P((int, const struct strbuf *, const struct strbuf *, int));
	ssize_t (*read) __P((int, void *, size_t));
	int (*readv) __P((int, const struct iovec *, int));
	int (*recv) __P((int, void *, int, int));
	int (*recvfrom) __P((int, void *, int, int, struct sockaddr *, int *));
	int (*recvmsg) __P((int, struct msghdr *, int));
	int (*send) __P((int, const void *, int, int));
	int (*sendmsg) __P((int, const struct msghdr *, int));
	int (*sendto) __P((int, const void *, int, int,
		const struct sockaddr *, int));
	int (*setsockopt) __P((int, int, int, const void *, int));
	int (*shutdown) __P((int, int));
	int (*socketpair) __P((int, int, int, int *));
	int (*stty) __P((int, const struct sgttyb *));
	ssize_t (*write) __P((int, const void *, size_t));
	int (*writev) __P((int, const struct iovec *, int));
};

/* XXX prototype omitted in Lite */
extern int quotactl __P((const char *, int, int, void *));

struct genericops {
	int (*access) __P((const char *, int));
	int (*acct) __P((const char *));
	int (*adjtime) __P((const struct timeval *, struct timeval *));
	unsigned (*alarm) __P((unsigned));
	int (*async_daemon) __P((void));
	int (*brk) __P((const char *));
	int (*chdir) __P((const char *));
	int (*chflags) __P((const char *, unsigned long));
	int (*chmod) __P((const char *, mode_t));
	int (*chown) __P((const char *, uid_t, gid_t));
	int (*chroot) __P((const char *));
	int (*creat) __P((const char *, mode_t));
	int (*eaccess) __P((const char *, int));
	int (*execv) __P((const char *, char * const *));
	int (*execve) __P((const char *, char * const *, char * const *));
	void (*exit) __P((int));
	pid_t (*fork) __P((void));
	int (*ftime) __P((struct timeb *));
	int (*getdescriptor) __P((int, int));
	int (*getdtablesize) __P((void));
	gid_t (*getegid) __P((void));
	uid_t (*geteuid) __P((void));
	int (*getfh) __P((const char *, struct fhandle *));
	int (*getfsstat) __P((struct statfs *, long, int));
	gid_t (*getgid) __P((void));
	int (*getgroups) __P((int, gid_t *));
	long (*gethostid) __P((void));
	int (*gethostname) __P((char *, int));
	int (*getitimer) __P((int, struct itimerval *));
	int (*getkerninfo) __P((int, int, int, int));
	int (*getlogin) __P((char *, u_int));
	int (*getpagesize) __P((void));
	pid_t (*getpgrp) __P((void));
	pid_t (*getpid) __P((void));
	pid_t (*getppid) __P((void));
	int (*getpriority) __P((int, int));
	int (*getrlimit) __P((int, struct rlimit *));
	int (*getrusage) __P((int, struct rusage *));
	int (*gettimeofday) __P((struct timeval *, struct timezone *));
	uid_t (*getuid) __P((void));
	int (*kill) __P((pid_t, int));
	int (*killpg) __P((pid_t, int));
	int (*ktrace) __P((const char *, int, int, pid_t));
	int (*link) __P((const char *, const char *));
	int (*lstat) __P((const char *, struct stat *));
	int (*madvise) __P((int, int, int));
	int (*mincore) __P((int, int, int));
	int (*mkdir) __P((const char *, mode_t));
	int (*mkfifo) __P((const char *, mode_t));
	int (*mknod) __P((const char *, mode_t, dev_t));
	int (*mount) __P((const char *, const char *, int, void *));
	int (*mprotect) __P((caddr_t, int, int));
	int (*msgop) __P((int, int, int, int, int, int));
	int (*msync) __P((caddr_t, int));
	int (*munmap) __P((caddr_t, int));
	long (*nap) __P((long));
	int (*nice) __P((int));
	int (*open) __P((const char *, int, mode_t));
	long (*otruncate) __P((const char *, long));
	long (*pathconf) __P((const char *, int));
	int (*pause) __P((void));
	int (*pgrpctl) __P((int, int, pid_t, pid_t));
	int (*pipe) __P((void));
	int (*plock) __P((int));
	int (*poll) __P((struct pollfd *, unsigned long, int));
	int (*profil) __P((char *, int, int, int));
	int (*protctl) __P((int, int, int));
	int (*ptrace) __P((int, pid_t, caddr_t, int));
	int (*quotactl) __P((const char *, int, int, void *));
	int (*readlink) __P((const char *, char *, int));
	int (*reboot) __P((int));
	int (*rename) __P((const char *, const char *));
	int (*revoke) __P((const char *));
	int (*rmdir) __P((const char *));
	char *(*sbrk) __P((int));
	int (*sco_mount) __P((const char *, const char *, int));
	int (*scoinfo) __P((struct scoutsname *));
	int (*select)
		__P((int, fd_set *, fd_set *, fd_set *, struct timeval *));
	int (*sem_lock) __P((struct semaphore *));
	int (*sem_wakeup) __P((struct semaphore *));
	int (*semop) __P((int, int, int, int, int));
	int (*setdescriptor) __P((int, int));
	int (*setegid) __P((gid_t));
	int (*seteuid) __P((uid_t));
	int (*setgid) __P((gid_t));
	int (*setgroups) __P((int, const gid_t *));
	void (*sethostid) __P((long));
	int (*sethostname) __P((const char *, int));
	int (*setitimer)
		__P((int, const struct itimerval *, struct itimerval *));
	int (*setlogin) __P((const char *));
	int (*setpgid) __P((pid_t, pid_t));
	int (*setpriority) __P((int, int, int));
	int (*setregid) __P((int, int));
	int (*setreuid) __P((int, int));
	int (*setrlimit) __P((int, const struct rlimit *));
	pid_t (*setsid) __P((void));
	int (*settimeofday)
		__P((const struct timeval *, const struct timezone *));
	int (*setuid) __P((uid_t));
	int (*shmop) __P((int, int, int, int));
	int (*sigaction)
		__P((int, const struct sigaction *, struct sigaction *));
	sig_t (*signal) __P((int, sig_t));
	int (*sigpending) __P((sigset_t *));
	int (*sigprocmask) __P((int, const sigset_t *, sigset_t *));
	int (*sigreturn) __P((struct sigcontext *));
	int (*sigstack) __P((const struct sigstack *, struct sigstack *));
	int (*sigsuspend) __P((const sigset_t *));
	int (*socket) __P((int, int, int));
	int (*sstk) __P((int));
	int (*stat) __P((const char *, struct stat *));
	/* BSD statfs() has only the first two arguments */
	int (*statfs) __P((const char *, struct statfs *, int, int));
	int (*stime) __P((const time_t *));
	int (*swapon) __P((const char *));
	int (*symlink) __P((const char *, const char *));
	void (*sync) __P((void));
	long (*sysconf) __P((int));
	int (*sysfs) __P((int, int, int));
	int (*sysi86) __P((int, int, int, int));
	time_t (*time) __P((time_t *));
	clock_t (*times) __P((struct tms *));
	int (*truncate) __P((const char *, off_t));
	int (*uadmin) __P((int, int, int));
	int (*ulimit) __P((int, int));
	mode_t (*umask) __P((mode_t));
	int (*umount) __P((const char *));
	int (*uname) __P((void *, int, int));
	int (*unlink) __P((const char *));
	int (*unmount) __P((const char *, int));
	int (*utime) __P((const char *, const struct utimbuf *));
	int (*utimes) __P((const char *, const struct timeval *));
	pid_t (*vfork) __P((void));
	pid_t (*wait4) __P((pid_t, int *, int, struct rusage *));
	pid_t (*waitpid) __P((int, int *, int));
};

struct fdbase {
	struct fdops *f_ops;
};

extern struct fdbase **fdtab;
extern int max_fdtab;
extern struct genericops *generic;
