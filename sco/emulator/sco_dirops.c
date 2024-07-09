/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_dirops.c,v 2.2 1996/03/05 20:19:13 donn Exp
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"

/*
 * File ops for directories.
 */

extern struct fdops dirops;

struct fddir {
	struct	fdbase d_base;
	enum	{ UNKNOWN, DIRECT, DIRENT } d_type;
	caddr_t	d_buf;		/* to cache directory entries */
};

static void
dir_init(f, filename, flags)
	int f;
	const char *filename;
	int flags;
{
	struct fddir *fddp;

	fd_register(f);
	if ((fddp = malloc(sizeof (struct fddir))) == 0)
		err(1, "can't initialize regular file");
	fddp->d_base.f_ops = &dirops;
	fddp->d_type = UNKNOWN;
	fddp->d_buf = malloc(NBPG);
	fdtab[f] = &fddp->d_base;
}

struct sco_direct {
	unsigned short	d_ino;
	char		d_name[14];
};

ssize_t
dir_read(f, buf, len)
	int f;
	void *buf;
	size_t len;
{
	struct fddir *fddp = (struct fddir *)fdtab[f];
	struct sco_direct *sdp = (struct sco_direct *)buf;
	struct sco_direct *end_sdp = &sdp[len / sizeof (*sdp)];
	char *dbuf = fddp->d_buf;
	struct dirent *dp = (struct dirent *)dbuf;
	struct dirent *end_dp;
	long base;
	int n;

	sig_state = SIG_POSTPONE;

	if (fddp->d_type == DIRENT)
		errx(1, "read on directory after previous readdir");
	for (end_dp = dp; sdp < end_sdp; ++sdp) {
		if (dp >= end_dp) {
			if ((n = getdirentries(f, dbuf, NBPG, &base)) <= 0)
				break;
			dp = (struct dirent *)dbuf;
			end_dp = (struct dirent *)(dbuf + n);
		}
		sdp->d_ino = dp->d_fileno;
		bcopy(dp->d_name, sdp->d_name,
		    MIN(dp->d_namlen + 1, sizeof (sdp->d_name)));
		dp = (struct dirent *)((char *)dp + dp->d_reclen);
	}
	if (dp < end_dp)
		lseek(f, base + ((char *)dp - dbuf), SEEK_SET);
	fddp->d_type = DIRECT;
	return ((char *)sdp - (char *)buf);
}

int
dir_close(f)
	int f;
{

	sig_state = SIG_POSTPONE;

	if (close(f) == -1)
		return (-1);
	free(((struct fddir *)fdtab[f])->d_buf);
	free(fdtab[f]);
	fdtab[f] = 0;
	return (0);
}

off_t
dir_lseek(f, o, c)
	int f, c;
	off_t o;
{
	struct fddir *fddp = (struct fddir *)fdtab[f];
	int zero = o == 0 && c == SEEK_SET;

	if (fddp->d_type == DIRECT && !zero)
		errx(1, "seek on directory after read");
	if (zero)
		fddp->d_type = UNKNOWN;
	return (lseek(f, o, c));
}

int
dir_dup(f)
	int f;
{

	errx(1, "attempt to dup a directory descriptor");
}

int
dir_fcntl(f, c, arg)
	int f, c, arg;
{
	struct flock fl;
	int a = arg;
	int r;

	if (c == F_SETLKW) {
		flock_in((struct flock *)arg, &fl);
		return (commit_fcntl(f, c, (int)&fl));
	}

	sig_state = SIG_POSTPONE;

	switch (c) {

	case F_GETLK:
	case F_SETLK:
		flock_in((struct flock *)arg, &fl);
		a = (int)&fl;
		break;

	case F_SETFL:
		a = open_in(arg);
		if (a & X_O_NDELAY)
			/* best effort */
			a = (a &~ X_O_NDELAY) | O_NONBLOCK;
		break;

	case F_DUPFD:
		errx(1, "attempt to dup a directory descriptor");
		break;

	/* case F_CHKFL? */
	}

	if ((r = fcntl(f, c, a)) == -1)
		return (-1);

	switch (c) {

	case F_GETLK:
		flock_out(&fl, (struct flock *)arg);
		break;

	case F_GETFL:
		r = open_out(r);
		break;
	}

	return (r);
}

/*
 * From iBCS2 p 6-31.
 * 'Tis a pity that this dirent is bigger than the BSD dirent...
 */
struct sco_dirent {
	long		d_ino;
	long		d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};
/*
 * The actual size of a SCO dirent, given the length of the entry name.
 * Name length plus 1 nul byte plus 10 bytes overhead, round at 4 bytes...
 */
#define	sco_dirent_reclen(namlen)	(((namlen) + 1 + 10 + 3) & ~3)

int
dir_getdents(f, xsdp, len)
	int f;
	struct dirent *xsdp;
	size_t len;
{
	struct fddir *fddp = (struct fddir *)fdtab[f];
	char *buf = (char *)xsdp;
	struct sco_dirent *sdp = (struct sco_dirent *)buf;
	struct sco_dirent *end_sdp = (struct sco_dirent *)(buf + len);
	struct sco_dirent *next_sdp;
	char *dbuf = fddp->d_buf;
	struct dirent *dp = (struct dirent *)dbuf;
	struct dirent *end_dp;
	long base;
	int n = 0, reclen;

	if (fddp->d_type == DIRECT)
		errx(1, "readdir on directory after previous read");

	for (end_dp = dp; sdp < end_sdp; sdp = next_sdp) {
		if (dp >= end_dp) {
			if ((n = getdirentries(f, dbuf, NBPG, &base)) <= 0)
				break;
			dp = (struct dirent *)dbuf;
			end_dp = (struct dirent *)(dbuf + n);
		}
		if (dp->d_fileno == 0) {
			dp = (struct dirent *)((char *)dp + dp->d_reclen);
			next_sdp = sdp;
			continue;
		}
		reclen = sco_dirent_reclen(dp->d_namlen);
		next_sdp = (struct sco_dirent *)((char *)sdp + reclen);
		if (next_sdp > end_sdp)
			break;
		/* We truncate the inode number to 16 bits...  */
		sdp->d_ino = (unsigned short)dp->d_fileno;
		sdp->d_off = base + ((char *)dp - dbuf);
		sdp->d_reclen = reclen;
		bcopy(dp->d_name, sdp->d_name, dp->d_namlen + 1);
		dp = (struct dirent *)((char *)dp + dp->d_reclen);
	}
	if (dp < end_dp)
		lseek(f, base + ((char *)dp - dbuf), SEEK_SET);
	if ((char *)sdp == buf && n > 0) {
		/* program's buffer was too small */
		errno = EINVAL;
		return (-1);
	}
	fddp->d_type = DIRENT;
	return ((char *)sdp - buf);
}

struct fdops dirops = {
	0,
	0,
	dir_close,
	0,
	dir_dup,
	0,
	0,
	0,
	0,
	0,
	dir_fcntl,
	0,
	fpathconf,
	fstat,
	/* XXX handle statfs buffer length? */
	(int (*) __P((int, struct statfs *, int, int)))fstatfs,
	0,
	0,
	dir_getdents,
	0,
	enostr,
	0,
	0,
	0,
	enotty,
	dir_init,
	enotty,
	0,
	dir_lseek,
	0,
	0,
	enostr,
	dir_read,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	enotty,
	(ssize_t (*)())eisdir,
	0,
};
