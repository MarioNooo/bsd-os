/*-
 * Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
 * The Wind River Systems Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI tornado_pax.h,v 2.4 2002/03/22 16:52:01 benoitp Exp
 */

/*
 * tornado_pax.h --
 *
 *	Definitions Tornado specific
 */

#ifndef	_TORNADO_PAX
#define	_TORNADO_PAX

/* Definitions to build on cygwin */

#ifdef	__CYGWIN__

#ifdef BSD_TARGET
#include <bsdos/bsdosLib.h>

#define	dev_t		bsdos_dev_t
#define	off_t		bsdos_off_t

#define	makedev 	bsdos_makedev
#define	major		bsdos_major
#define	minor		bsdos_minor
#endif	/* BSD_TARGET */

#define getpwuid(uid)	NULL
#define getgrgid(gid)	NULL
#define getpwnam(name)	NULL
#define getgrnam(name)	NULL

#ifdef	UNIXFILE
#include <unixfileLib.h>

#define	stat		unix_stat
#define	st_atime	st_atimespec.tv_sec
#define	st_ctime	st_ctimespec.tv_sec
#define	st_mtime	st_mtimespec.tv_sec
#define	fstat		unix_fstat
#define	lstat		unix_lstat

#define	uid_t		unix_uid_t
#define	gid_t		unix_gid_t

void unixFileEndFunc (void);

#define main	main(int argc, char **argv) \
{ \
	int ret; \
	if (unixFileEnvInit("WIND_UNIXFILE_STATS", "WIND_UNIXFILE_REPS", \
			     UF_COMMENT_BAD_ENT|\
			     UF_VERBOSE_ERR|\
			     UF_CREATE_USTAT|\
			     UF_LOCK_USTAT) != OK) \
		return (1); \
	atexit(unixFileEndFunc); \
	ret = _unix_main(argc, argv); \
	unixFileAllFinalize(); \
	return ret; \
	} \
void \
unixFileEndFunc(void) \
{ \
	unixFileAllFinalize(); \
} \
int _unix_main

#if !defined(UF_EXCLUDE_API)
#define open		unix_open
#define close		unix_close
#define read		unix_read
#define write		unix_write
#define lseek		unix_lseek
#define unlink		unix_unlink
#endif

#define readlink	unix_readlink

#define link		unix_link
#define mkdir		unix_mkdir
#define mknod		unix_mknod
#define mkfifo		unix_mkfifo
#define symlink		unix_symlink
#define rmdir		unix_rmdir
#define utimes		unix_utimes
#define chown		unix_chown
#define chmod		unix_chmod
#define fstat		unix_fstat

#endif	/* UNIXFILE */

#endif	/* __CYGWIN__ */

#endif	/* _TORNADO_PAX */
