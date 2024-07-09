/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_statfs.c,v 2.1 1995/02/03 15:15:39 polk Exp
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <err.h>	/* XXX */
#include <string.h>

#include "emulate.h"
#include "sco.h"

/* from iBCS2 p 6-53 */
struct sco_statfs {
	short	f_fstyp;
	long	f_bsize;
	long	f_frsize;
	long	f_blocks;
	long	f_bfree;
	long	f_files;
	long	f_ffree;
	char	f_fname[6];
	char	f_fpack[6];
};

void
statfs_out(fs, osfs)
	const struct statfs *fs;
	struct statfs *osfs;
{
	struct sco_statfs *sfs = (struct sco_statfs *)osfs;

	sfs->f_fstyp = fs->f_type;	/* XXX */
	sfs->f_bsize = fs->f_bsize;
	sfs->f_frsize = 1024;		/* XXX */
	sfs->f_blocks = fs->f_blocks;
	sfs->f_bfree = fs->f_bfree;
	sfs->f_files = fs->f_files;
	sfs->f_ffree = fs->f_ffree;
	bzero(sfs->f_fname, sizeof (sfs->f_fname));
	bzero(sfs->f_fpack, sizeof (sfs->f_fpack));
}
