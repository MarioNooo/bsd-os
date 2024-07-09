/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_stat.c,v 2.1 1995/02/03 15:15:35 polk Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <err.h>	/* XXX */

#include "emulate.h"
#include "sco.h"

/* from iBCS2 p 6-51 */
struct sco_stat {
	short		sst_dev;
	unsigned short	sst_ino;
	unsigned short	sst_mode;
	short		sst_nlink;
	unsigned short	sst_uid;
	unsigned short	sst_gid;
	short		sst_rdev;
	long		sst_size;
	long		sst_atime;
	long		sst_mtime;
	long		sst_ctime;
};

void
stat_out(bs, oss)
	const struct stat *bs;
	struct stat *oss;
{
	struct sco_stat *ss = (struct sco_stat *)oss;

	/*
	 * Several of these fields simply get truncated...  Too bad.
	 * The file type bits turn out to match.
	 */
	ss->sst_dev = bs->st_dev;
	ss->sst_ino = bs->st_ino;
	ss->sst_mode = bs->st_mode;
	ss->sst_nlink = bs->st_nlink;
	ss->sst_uid = bs->st_uid;
	ss->sst_gid = bs->st_gid;
	ss->sst_rdev = bs->st_rdev;
	ss->sst_size = bs->st_size;
	ss->sst_atime = bs->st_atime;
	ss->sst_mtime = bs->st_mtime;
	ss->sst_ctime = bs->st_ctime;
}
