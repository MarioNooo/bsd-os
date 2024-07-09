/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/* THIS FILE IS MAINLY JUST A PLACEHOLDER (see also null.c) */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "diskdefect.h"
#include "bbops.h"

struct bbscsi {
	int	bb_ncur;	/* number of used entries in bb_cur */
	daddr_t	*bb_cur;	/* current bad sectors */
	int	bb_nnew;	/* number of used entries in bb_new */
	daddr_t	*bb_new;	/* new bad sectors to be added */
};

int	scsi_attach __P((struct ddstate *, int));
void	scsi_detach __P((struct ddstate *));
int	scsi_clear __P((struct ddstate *, int));
int	scsi_load __P((struct ddstate *, enum loadhow, char *));
int	scsi_setaux __P((struct ddstate *, char *));
void	scsi_scanlimit __P((struct ddstate *, daddr_t *, daddr_t *));
int	scsi_addbfi __P((struct ddstate *, int, int, int));
int	scsi_addsn __P((struct ddstate *, daddr_t));
int	scsi_print __P((struct ddstate *, enum printhow, int));
int	scsi_store __P((struct ddstate *));

struct bbops scsi_ops = {
	"scsi", "SCSI bad block replacement",
	scsi_attach, scsi_detach, scsi_clear, scsi_load,
	scsi_setaux, scsi_scanlimit, scsi_addbfi, scsi_addsn,
	scsi_print, scsi_store
};

static int sncompare __P((const void *, const void *));

/*
 * See whether to use SCSI bad block replacement, or force it.
 */
int
scsi_attach(ds, force)
	struct ddstate *ds;
	int force;
{
	struct bbscsi *bb;

	switch (ds->d_dl.d_type) {

	case DTYPE_SCSI:
		break;

	default:
		if (!force)
			return (NOT_AUTO);
		break;
	}

	/*
	 * Looks good, take it.
	 */
	bb = emalloc(sizeof *bb);
	bb->bb_ncur = 0;		/* XXX */
	bb->bb_cur = NULL;		/* XXX */
	bb->bb_nnew = 0;
	bb->bb_new = NULL;
	ds->d_badinfo = bb;

	warnx("note: SCSI bad block replacement not yet implemented.");
	return (0);
}

/*
 * Tear down state.  Nothing special here, just free the space we allocated.
 */
void
scsi_detach(ds)
	struct ddstate *ds;
{

	free(ds->d_badinfo);
}

/*
 * Clear loaded info.  Not sure if we will ever use this...
 */
int
scsi_clear(ds, which)
	struct ddstate *ds;
	int which;
{
	struct bbscsi *bb = ds->d_badinfo;

	if (which & CLEAR_OLD) {
		free(bb->bb_cur);
		bb->bb_ncur = 0;
	}
	if (which & CLEAR_NEW) {
		free(bb->bb_new);
		bb->bb_nnew = 0;
	}
	return (0);
}

/*
 * Load existing bad block info.
 */
int
scsi_load(ds, how, userarg)
	struct ddstate *ds;
	enum loadhow how;
	char *userarg;
{
	struct bbscsi *bb;

	switch (how) {

	case LOAD_DEFAULT:
		break;

	case LOAD_CUR:			/* XXX for now */
		break;

	case LOAD_MFR:
	case LOAD_USER:
		return (EOPNOTSUPP);

	default:
		panic("scsi_load");
	}

	/*
	 * bb->bb_ncur = n;
	 * ds->d_bflags |= fixed ? DB_LOADED | DB_MODIFIED : DB_LOADED;
	 */
	return (0);
}

/*
 * Adjust auto-scan limits.  Nothing to do for SCSI.
 */
void
scsi_scanlimit(ds, amin, amax)
	struct ddstate *ds;
	daddr_t *amin, *amax;
{
}

/*
 * Set auxiliary info (none here).
 */
int
scsi_setaux(ds, aux)
	struct ddstate *ds;
	char *aux;
{

	return (0);
}

/*
 * Add a <cyl,trk,bfi> triple as a bad spot.
 */
int
scsi_addbfi(ds, cyl, trk, bfi)
	struct ddstate *ds;
	int cyl, trk, bfi;
{

	/*
	 * XXX - need to find out how to do this, then do it!
	 */
	return (EOPNOTSUPP);
}

/*
 * Add an absolute sector number as a bad spot.
 */
int
scsi_addsn(ds, sn)
	struct ddstate *ds;
	daddr_t sn;
{
	struct bbscsi *bb = ds->d_badinfo;
	daddr_t *mem;

	mem = realloc(bb->bb_new, (bb->bb_nnew + 1) * sizeof *mem);
	if (mem == NULL)
		return (errno);
	bb->bb_new = mem;
	mem[bb->bb_nnew++] = sn;
	return (0);
}

/*
 * Print the old and/or new tables.
 */
int
scsi_print(ds, how, which)
	struct ddstate *ds;
	enum printhow how;
	int which;
{
	struct bbscsi *bb = ds->d_badinfo;
	int i, cyl, trk, sec;
	daddr_t sn, lastsn, *snp;

	switch (how) {

	case PRINT_DEFAULT:
	case PRINT_CTS:
	case PRINT_SN:
		break;

	case PRINT_BFI:
		return (EOPNOTSUPP);

	default:
		panic("scsi_print how");
	}
	if (which & PRINT_OLD)
		printf("(no on-disk information in this format yet)\n");
	if (which & PRINT_NEW) {
		snp = bb->bb_new;
		qsort(snp, bb->bb_nnew, sizeof *snp, sncompare);
		printf("bad blocks for %s:\n", ds->d_name);
		lastsn = -1;
		for (i = bb->bb_nnew; --i >= 0;) {
			sn = *snp++;
			if (sn == lastsn)
				continue;
			lastsn = sn;
			printf("Bad Block: ");
			switch (how) {

			case PRINT_SN:
				printf("sn=%ld\n", (long)sn);
				break;

			case PRINT_DEFAULT:
				printf("sn=%ld, ", (long)sn);
				/* FALLTHROUGH */

			default:
				cyl = CYL(ds, sn);
				trk = TRK(ds, sn);
				sec = SEC(ds, sn);
				printf("cyl=%d, trk=%d, sec=%d\n",
				    cyl, trk, sec);
				break;
			}
		}
	}
	return (0);
}

/*
 * Commit the new changes (if any).
 *
 * If we fail here, we are pretty much dead, but...
 */
int
scsi_store(ds)
	struct ddstate *ds;
{
	struct bbscsi *bb = ds->d_badinfo;

	if (bb->bb_nnew == 0)
		return (0);
	return (EOPNOTSUPP);
}

static int
sncompare(l0, r0)
	const void *l0, *r0;
{
	daddr_t l, r;

	l = *(const daddr_t *)l0;
	r = *(const daddr_t *)r0;
	return (l < r ? -1 : l > r);
}
