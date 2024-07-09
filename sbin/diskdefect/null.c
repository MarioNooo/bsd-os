/*
 * Copyright (c) 1995
 * Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

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

/*
 * The `null' defect module.  Records errors but refuses to do
 * anything useful with them (`store' always fails).
 */
struct bbnull {
	daddr_t	*bb_defects;	/* base of array of defects */
	int	bb_ndefects;	/* number of entries in array */
	int	bb_size;	/* size of array */
};

int	null_attach __P((struct ddstate *, int));
void	null_detach __P((struct ddstate *));
int	null_clear __P((struct ddstate *, int));
int	null_load __P((struct ddstate *, enum loadhow, char *));
int	null_setaux __P((struct ddstate *, char *));
void	null_scanlimit __P((struct ddstate *, daddr_t *, daddr_t *));
int	null_addbfi __P((struct ddstate *, int, int, int));
int	null_addsn __P((struct ddstate *, daddr_t));
int	null_print __P((struct ddstate *, enum printhow, int));
int	null_store __P((struct ddstate *));

struct bbops null_ops = {
	"null", "Accumulate for listing",
	null_attach, null_detach, null_clear, null_load,
	null_setaux, null_scanlimit, null_addbfi, null_addsn,
	null_print, null_store
};

static int sncompare __P((const void *, const void *));

/*
 * See whether to use null bad block replacement, or force it.
 */
int
null_attach(ds, force)
	struct ddstate *ds;
	int force;
{
	struct bbnull *bb;

	if (!force)
		return (NOT_AUTO);
	bb = emalloc(sizeof *bb);
	bb->bb_size = 100;	/* probably plenty, will grow if needed */
	bb->bb_defects = emalloc(bb->bb_size * sizeof *bb->bb_defects);
	bb->bb_ndefects = 0;
	ds->d_badinfo = bb;
	return (0);
}

/*
 * Detach.
 */
void
null_detach(ds)
	struct ddstate *ds;
{
	struct bbnull *bb = ds->d_badinfo;

	free(bb->bb_defects);
	free(bb);
}

/*
 * Clear loaded info.  Not sure if we will ever use this...
 */
int
null_clear(ds, which)
	struct ddstate *ds;
	int which;
{
	struct bbnull *bb = ds->d_badinfo;

	if (which & CLEAR_NEW)
		bb->bb_size = 0;
	return (0);
}

/*
 * Load existing bad block info.
 */
int
null_load(ds, how, userarg)
	struct ddstate *ds;
	enum loadhow how;
	char *userarg;
{

	switch (how) {

	case LOAD_DEFAULT:
	case LOAD_CUR:
		break;

	case LOAD_MFR:
	case LOAD_USER:
		return (EOPNOTSUPP);

	default:
		panic("null_load");
	}
	ds->d_bflags |= DB_LOADED;
	return (0);
}

/*
 * Adjust auto-scan limits.
 */
void
null_scanlimit(ds, amin, amax)
	struct ddstate *ds;
	daddr_t *amin, *amax;
{
	/* nothing to do */
}

/*
 * Set auxiliary info.
 */
int
null_setaux(ds, aux)
	struct ddstate *ds;
	char *aux;
{

	return (0);
}

/*
 * Add a <cyl,trk,bfi> triple as a bad spot.
 */
int
null_addbfi(ds, cyl, trk, bfi)
	struct ddstate *ds;
	int cyl, trk, bfi;
{

	/*
	 * XXX - need to find out how to do this, then do it!
	 *
	 * (I wonder if `bytes from index' is the same on all drives?)
	 */
	return (EOPNOTSUPP);
}

/*
 * Add an absolute sector number as a bad spot.
 * We do not bother checking to see if it is already here.
 */
int
null_addsn(ds, sn)
	struct ddstate *ds;
	daddr_t sn;
{
	struct bbnull *bb = ds->d_badinfo;

	if (bb->bb_ndefects >= bb->bb_size) {
		bb->bb_size *= 2;
		bb->bb_defects = erealloc(bb->bb_defects,
		    bb->bb_size * sizeof *bb->bb_defects);
	}
	bb->bb_defects[bb->bb_ndefects++] = sn;
	return (0);
}

/*
 * Print the table.
 */
int
null_print(ds, how, which)
	struct ddstate *ds;
	enum printhow how;
	int which;
{
	struct bbnull *bb = ds->d_badinfo;
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
		panic("null_print how");
	}
	if (which & PRINT_OLD)
		printf("(no on-disk information in this format)\n");
	if (which & PRINT_NEW) {
		snp = bb->bb_defects;
		qsort(snp, bb->bb_ndefects, sizeof *snp, sncompare);
		printf("bad blocks for %s:\n", ds->d_name);
		lastsn = -1;
		for (i = bb->bb_ndefects; --i >= 0;) {
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
 */
int
null_store(ds)
	struct ddstate *ds;
{
	struct bbnull *bb = ds->d_badinfo;

	if (bb->bb_ndefects == 0)
		return (0);	/* ??? */

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
