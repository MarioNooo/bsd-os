/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_genericops.c,v 2.6 1996/08/20 17:39:25 donn Exp
 */

/*
 * Support for generic ops
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"

/* compat routines */
extern int creat __P((const char *, mode_t));
extern int ftime __P((struct timeb *));

int
sco_mount(devname, filename, flag)
	const char *devname;
	const char *filename;
	int flag;
{

	errx(1, "unsupported mount system call");
}

int
sco_umount(filename)
	const char *filename;
{

	errx(1, "unsupported umount system call");
}

int
sco_eaccess(path, mode)
	const char *path;
	int mode;
{

	/* hack: just use iBCS2 access() */
	return (access(path, mode));
}

int
sco_stime(timep)
	const time_t *timep;
{

	errx(1, "unsupported stime system call");
}

int
sco_uadmin(a, b, c)
	int a, b, c;
{

	errx(1, "unsupported uadmin system call");
}

/* by inspection */
#define	GETFSIND	1
#define	GETFSTYP	2
#define	GETNFSTYP	3

#define	FSTYPSZ		16

/*
 * XXX BSD compatibility!  This interface is now obsolete,
 * but from laziness, I've grabbed the relevant definitions
 * from the old <sys/mount.h> file and inserted them here.
 */

/*
 * File system types.
 */
#define	MOUNT_NONE	0
#define	MOUNT_UFS	1	/* Fast Filesystem */
#define	MOUNT_NFS	2	/* Sun-compatible Network Filesystem */
#define	MOUNT_MFS	3	/* Memory-based Filesystem */
#define	MOUNT_MSDOS	4 	/* MS-DOS FAT-12/FAT-16 filesystem */
#define	MOUNT_LFS	5	/* Log-based Filesystem */
#define	MOUNT_LOFS	6	/* Loopback Filesystem */
#define	MOUNT_FDESC	7	/* File Descriptor Filesystem */
#define	MOUNT_PORTAL	8	/* Portal Filesystem */
#define	MOUNT_NULL	9	/* Minimal Filesystem Layer */
#define	MOUNT_UMAP	10	/* User/Group Identifer Remapping Filesystem */
#define	MOUNT_KERNFS	11	/* Kernel Information Filesystem */
#define	MOUNT_PROCFS	12	/* /proc Filesystem */
#define	MOUNT_AFS	13	/* Andrew Filesystem */
#define	MOUNT_CD9660	14	/* ISO9660 (aka CDROM) Filesystem */
#define	MOUNT_ISO9660	MOUNT_CD9660
#define	MOUNT_UNION	15	/* Union (translucent) Filesystem */
#define	MOUNT_MAXTYPE	15

#define INITMOUNTNAMES { \
	"none",		/*  0 MOUNT_NONE */ \
	"ufs",		/*  1 MOUNT_UFS */ \
	"nfs",		/*  2 MOUNT_NFS */ \
	"mfs",		/*  3 MOUNT_MFS */ \
	"msdos",	/*  4 MOUNT_MSDOS */ \
	"lfs",		/*  5 MOUNT_LFS */ \
	"lofs",		/*  6 MOUNT_LOFS */ \
	"fdesc",	/*  7 MOUNT_FDESC */ \
	"portal",	/*  8 MOUNT_PORTAL */ \
	"null",		/*  9 MOUNT_NULL */ \
	"umap",		/* 10 MOUNT_UMAP */ \
	"kernfs",	/* 11 MOUNT_KERNFS */ \
	"procfs",	/* 12 MOUNT_PROCFS */ \
	"afs",		/* 13 MOUNT_AFS */ \
	"cd9660",	/* 14 MOUNT_CD9660 */ \
	"union",	/* 15 MOUNT_UNION */ \
	0,		/* 16 MOUNT_SPARE */ \
}

static char *mount_names[] = INITMOUNTNAMES;

int
sco_sysfs(cookie, arg1, arg2)
	int cookie, arg1, arg2;
{
	int i;
	char *s, *t;

	switch (cookie) {

	case GETFSIND:
		s = (char *)arg1;
		for (i = 1; i <= MOUNT_MAXTYPE; ++i)
			if (strcasecmp(s, mount_names[i]) == 0)
				break;
		if (i > MOUNT_MAXTYPE) {
			errno = EINVAL;
			return (-1);
		}
		return (i);

	case GETFSTYP:
		if (arg1 <= 0 || arg1 > MOUNT_MAXTYPE) {
			errno = EINVAL;
			return (-1);
		}
		s = mount_names[arg1];
		t = (char *)arg2;
		do {
			if (islower(*s))
				*t++ = toupper(*s);
			else
				*t++ = *s;
		} while (*s++);
		return (0);

	case GETNFSTYP:
		return (MOUNT_MAXTYPE);

	default:
		errno = EINVAL;
		return (-1);
	}
}

long
sco_nap(n)
	long n;
{
	struct timeval tv;

	tv.tv_sec = n / 1000;
	tv.tv_usec = (n % 1000) * 1000;

	if (select(0, NULL, NULL, NULL, &tv) == -1)
		return (-1);
	return (n);
}

int
sco_pause()
{
	sigset_t s;

	sigemptyset(&s);
	return (commit_sigsuspend(&s));
}

struct genericops generictab = {
	access,
	acct,
	0,
	alarm,
	0,
	sco_break,
	sco_chdir,
	0,
	chmod,
	chown,
	chroot,
	sco_creat,
	sco_eaccess,
	sco_execv,
	sco_execve,
	_exit,
	sco_fork,
	ftime,
	0,
	0,
	0,
	0,
	0,
	0,
	sco_getgid,
	sco_getgroups,
	0,
	0,
	getitimer,
	0,
	0,
	0,
	0,
	sco_getpid,
	0,
	0,
	0,
	0,
	0,
	sco_getuid,
	kill,
	0,
	0,
	link,
	lstat,
	0,
	0,
	mkdir,
	0,
	sco_mknod,
	0,
	0,
	sco_msg,
	0,
	0,
	sco_nap,
	nice,
	sco_open,
	0,
	pathconf,
	sco_pause,
	sco_pgrpctl,
	sco_pipe,
	sco_plock,
	sco_poll,
	profil,
	sco_protctl,
	sco_ptrace,
	0,
	readlink,
	0,
	rename,
	0,
	rmdir,
	0,
	sco_mount,
	scoinfo,
	commit_select,
	0,
	0,
	sco_sem,
	0,
	0,
	0,
	setgid,
	sco_setgroups,
	0,
	0,		/* sethostname */
	setitimer,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	setuid,
	sco_shm,
	sco_sigaction,
	sco_signal,
	sigpending,
	commit_sigprocmask,
	0,
	0,
	commit_sigsuspend,
	0,
	0,
	stat,
	(int (*) __P((const char *, struct statfs *, int, int)))statfs,
	sco_stime,
	0,
	symlink,
	sync,
	sysconf,
	sco_sysfs,
	sco_sysi86,
	time,
	times,
	0,
	sco_uadmin,
	sco_ulimit,
	umask,
	sco_umount,
	sco_uname,
	unlink,
	0,
	utime,
	0,
	0,
	0,
	sco_waitpid,
};

struct genericops *generic = &generictab;
