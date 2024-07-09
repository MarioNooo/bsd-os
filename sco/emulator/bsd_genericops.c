/*
 * Copyright (c) 1994,1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bsd_genericops.c,v 2.4 1996/08/20 17:39:21 donn Exp
 */

/*
 * Support for BSD generic ops
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"


/*
 * We emulate a couple of syscalls whose stubs use ECX,
 * which we covet for a return EIP.
 *
 * Note that the vfork() implementation will screw up if we switch
 * back to the version that shares memory between parent and child...
 */

pid_t
bsd_vfork()
{
	pid_t pid = vfork();

	if (pid > 0)
		/* parent: EAX = child pid, EDX = 0 */
		*program_edx = 0;
	else if (pid == 0) {
		/* child: EAX = parent pid (ignored), EDX = 1 */
		*program_edx = 1;
	}

	return (pid);
}

int
bsd_break(newbreak)
	const char *newbreak;
{

	return (syscall(SYS_break, newbreak));
}

long
bsd_otruncate(p, o)
	const char *p;
	long o;
{

	return ((long)truncate(p, (off_t)o));
}

/*
 * Most of these entries are not currently used,
 * courtesy of the speed hack in emulate_glue.s.
 */
struct genericops bsd_generictab = {
	access,
	acct,
	0,	/* adjtime, */
	0,
	0,	/* async_daemon, */
	bsd_break,
	sco_chdir,
	chflags,
	chmod,
	chown,
	chroot,
	creat,
	0,
	0,
	sco_execve,
	0,
	0,
	0,
	0,	/* getdescriptor, */
	0,	/* getdtablesize, */
	0,	/* getegid, */
	0,	/* geteuid, */
	getfh,
	0,	/* getfsstat, */
	0,	/* getgid, */
	0,	/* getgroups, */
	0,	/* gethostid, */
	0,	/* gethostname, */
	0,	/* getitimer, */
	0,	/* getkerninfo, */
	0,	/* getlogin, */
	0,	/* getpagesize, */
	0,	/* getpgrp, */
	0,	/* getpid, */
	0,	/* getppid, */
	0,	/* getpriority, */
	0,	/* getrlimit, */
	0,	/* getrusage, */
	0,	/* gettimeofday, */
	0,	/* getuid, */
	0,	/* kill, */
	0,	/* killpg, */
	ktrace,
	link,
	lstat,
	0,	/* madvise, */
	0,	/* mincore, */
	mkdir,
	mkfifo,
	mknod,
	mount,
	0,	/* mprotect, */
	0,
	0,	/* msync, */
	0,	/* munmap, */
	0,
	0,
	commit_open,
	bsd_otruncate,
	pathconf,
	0,
	0,
	0,	/* pipe, */
	0,
	0,
	0,	/* profil, */
	sco_protctl,
	0,	/* ptrace, */
	quotactl,
	readlink,
	0,	/* reboot, */
	rename,
	revoke,
	rmdir,
	0,	/* sbrk, */
	0,	/* mount, */
	0,	/* scoinfo */
	0,	/* commit_select, */
	0, 	/* commit_sem_lock, */
	0,	/* sem_wakeup, */
	0,
	0,	/* setdescriptor, */
	0,	/* setegid, */
	0,	/* seteuid, */
	0,	/* setgid, */
	0,	/* setgroups, */
	0,	/* sethostid, */
	0,	/* sethostname, */
	0,	/* setitimer, */
	0,	/* setlogin, */
	0,	/* setpgid, */
	0,	/* setpriority, */
	0,	/* setregid, */
	0,	/* setreuid, */
	0,	/* setrlimit, */
	0,	/* setsid, */
	0,	/* settimeofday, */
	0,	/* setuid, */
	0,
	0,	/* commit_sigaction, */
	0,
	0,	/* sigpending, */
	0,	/* commit_sigprocmask, */
	0,	/* commit_sigreturn, */
	0,	/* commit_sigstack, */
	0,	/* commit_sigsuspend, */
	0,	/* socket, */
	0,	/* sstk, */
	stat,
	(int (*) __P((const char *, struct statfs *, int, int)))statfs,
	0,
	swapon,
	symlink,
	0,	/* sync, */
	0,	/* sysconf, */
	0,
	0,
	0,	/* time, */
	0,
	truncate,
	0,
	0,
	0,	/* umask, */
	0,
	0,
	unlink,
	unmount,
	0,
	utimes,
	bsd_vfork,
	0,	/* commit_wait4, */
	0,
};

/*
 * BSD file ops are currently generic.
 */

int
bsd_fchdir(f)
	int f;
{
	int r;

	if ((r = fchdir(f)) == -1)
		return (-1);

	set_curdir(0);
	return (r);
}

struct fdops bsdfileops = {
	0, /* accept */
	0, /* bind */
	0, /* close */
	0, /* connect */
	0, /* dup */
	0, /* dup2 */
	bsd_fchdir,
	0, /* fchflags */
	0, /* fchmod */
	0, /* fchown */
	0, /* fcntl */
	0, /* flock */
	0, /* fpathconf */
	0, /* fstat */
	0, /* fstatfs */
	0, /* fsync */
	0, /* ftruncate */
	0, /* getdents */
	0, /* getdirentries */
	0, /* getmsg */
	0, /* getpeername */
	0, /* getsockname */
	0, /* getsockopt */
	0, /* gtty */
	0, /* init */
	0, /* ioctl */
	0, /* listen */
	0, /* lseek */
	0, /* mmap */
	0, /* nfssvc */
	0, /* putmsg */
	0, /* read */
	0, /* readv */
	0, /* recv */
	0, /* recvfrom */
	0, /* recvmsg */
	0, /* send */
	0, /* sendmsg */
	0, /* sendto */
	0, /* setsockopt */
	0, /* shutdown */
	0, /* socketpair */
	0, /* stty */
	0, /* write */
	0, /* writev */
};
