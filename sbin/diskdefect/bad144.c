/*
 * Copyright (c) 1995
 * Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*-
 * Copyright (c) 1980,1986,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/dkbad.h>
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
#include <time.h>
#include <unistd.h>

#include "diskdefect.h"
#include "bbops.h"

/*
 * XXX should be elsewhere (if anywhere...)
 */
#ifdef i386
#define	DKBAD_MAGIC	0x4321		/* XXX !!! */
#else
#define	DKBAD_MAGIC	0
#endif

struct bb144 {
	int	bb_file;	/* index # of bad-sector file for bb_cur */
	int	bb_justonefile;	/* if true, only write one bad-sector file */
	daddr_t	bb_replarea;	/* start of replacement block area */
	daddr_t	bb_bfarea;	/* start of bad-sector-file area (>replarea) */
	int	bb_ncur;	/* number of used entries in bb_cur */
	int	bb_nnew;	/* number of used entries in bb_new */
	int	bb_new_unsorted;/* true => bb_new is jumbled */
	struct	dkbad bb_cur;	/* current bad blocks as read from disk */
	struct	dkbad bb_new;	/* new bad blocks added */
};

int	bad144_attach __P((struct ddstate *, int));
void	bad144_detach __P((struct ddstate *));
int	bad144_clear __P((struct ddstate *, int));
int	bad144_load __P((struct ddstate *, enum loadhow, char *));
int	bad144_setaux __P((struct ddstate *, char *));
void	bad144_scanlimit __P((struct ddstate *, daddr_t *, daddr_t *));
int	bad144_addbfi __P((struct ddstate *, int, int, int));
int	bad144_addsn __P((struct ddstate *, daddr_t));
int	bad144_print __P((struct ddstate *, enum printhow, int));
int	bad144_store __P((struct ddstate *));

struct bbops bad144_ops = {
	"bad144", "DEC STD 144",
	bad144_attach, bad144_detach, bad144_clear, bad144_load,
	bad144_setaux, bad144_scanlimit, bad144_addbfi, bad144_addsn,
	bad144_print, bad144_store
};

static int addone __P((struct ddstate *, int, int));
static int btcompare __P((const void *, const void *));
static int checkbf __P((struct ddstate *, int, struct dkbad *, int *, int));
static int domoves __P((struct ddstate *, int, daddr_t *));
static int firstvalidbf __P((struct ddstate *));
static void initbf __P((struct dkbad *, long));
static int merge __P((struct ddstate *, struct bt_bad *, struct bt_bad *,
			struct bt_bad *, daddr_t *));
static void printbf __P((struct ddstate *, enum printhow, struct dkbad *));
static int readbf __P((struct ddstate *, int, struct dkbad *));
static void sortbf __P((struct dkbad *));
static int writebf __P((struct ddstate *, int, struct dkbad *));

#define	BTSIZE (sizeof(((struct dkbad *)0)->bt_bad) / sizeof(struct bt_bad))

/* macros for breaking and building bt_trksec */
#define	BT_TRK(ts)	((ts) >> 8)
#define	BT_SEC(ts)	((ts) & 0xff)
#define	BT_TRKSEC(t, s)	(((t) << 8) | (s))

/*
 * See whether to use DEC STD 144 bad block replacement, or force it.
 */
int
bad144_attach(ds, force)
	struct ddstate *ds;
	int force;
{
	struct bb144 *bb;
	long default_csn;

	switch (ds->d_dl.d_type) {

	case DTYPE_SMD:
	case DTYPE_DEC:
	case DTYPE_ESDI:
	case DTYPE_ST506:
		break;

	default:
		if (!force)
			return (NOT_AUTO);
		break;
	}

	/*
	 * Check for geometries that cannot work.
	 */
	if (ds->d_dl.d_secsize < sizeof(struct dkbad)) {
		warnx("%s: sector size %d too small, minimum is %d",
		    ds->d_name, (int)ds->d_dl.d_secsize,
		    (int)sizeof(struct dkbad));
		return (EINVAL);
	}
	if (ds->d_dl.d_nsectors > 255) {
		warnx("%s: cannot handle %d sectors per track (max 255)",
		    ds->d_name, (int)ds->d_dl.d_nsectors);
		return (EINVAL);
	}
	if (ds->d_dl.d_ntracks >= 0xff) {
		warnx("%s: cannot handle %d tracks per cylinder (max 255)",
		    ds->d_name, (int)ds->d_dl.d_ntracks);
		return (EINVAL);
	}
	if (ds->d_dl.d_ncylinders >= 0xfffe) {
		warnx("%s: cannot handle %d cylinders (max 65534)",
		    ds->d_name, (int)ds->d_dl.d_ncylinders);
		return (EINVAL);
	}

	/*
	 * Looks good, take it.
	 */
	bb = emalloc(sizeof *bb);
	bb->bb_justonefile = 0;
	bb->bb_bfarea = ds->d_dl.d_secperunit - ds->d_dl.d_nsectors;
	bb->bb_replarea = bb->bb_bfarea - BTSIZE;
	bb->bb_ncur = 0;
	bb->bb_nnew = 0;
	default_csn = time(NULL);
	initbf(&bb->bb_cur, default_csn);
	initbf(&bb->bb_new, default_csn);
	ds->d_badinfo = bb;
	return (0);
}

/*
 * Tear down state.  Nothing special here, just free the space we allocated.
 */
void
bad144_detach(ds)
	struct ddstate *ds;
{

	free(ds->d_badinfo);
}

/*
 * Clear loaded info.  Not sure if we will ever use this...
 */
int
bad144_clear(ds, which)
	struct ddstate *ds;
	int which;
{
	struct bb144 *bb = ds->d_badinfo;

	if (which & CLEAR_OLD)
		initbf(&bb->bb_cur, bb->bb_cur.bt_csn);
	if (which & CLEAR_NEW)
		initbf(&bb->bb_new, bb->bb_new.bt_csn);
	return (0);
}

/*
 * Load existing bad block info.
 */
int
bad144_load(ds, how, userarg)
	struct ddstate *ds;
	enum loadhow how;
	char *userarg;
{
	int bfnum, n, fixed;
	long t;
	char *ep;
	struct bb144 *bb;

	bfnum = -1;
	switch (how) {

	case LOAD_DEFAULT:
	case LOAD_CUR:
		break;

	case LOAD_MFR:
		return (EOPNOTSUPP);

	case LOAD_USER:
		t = strtol(userarg, &ep, 10);
		if (t < 0 || t > 4 || ep == userarg) {
			warnx("`%s' is nonsense: need a number in [0..4]",
			    userarg);
			return (EINVAL);
		}
		bfnum = t;
		break;

	default:
		panic("bad144_load");
	}

	fixed = 0;
	bb = ds->d_badinfo;
	if (bfnum == -1) {
		/* find first valid bad-sector file */
		for (bfnum = 0; bfnum < 5; bfnum++) {
			if (readbf(ds, bfnum, &bb->bb_cur) == 0 &&
			    (n = checkbf(ds, bfnum, &bb->bb_cur, &fixed,
				    0)) >= 0)
				goto gotit;
		}
		warnx("%s: cannot locate valid bad block table", ds->d_name);
		return (ESRCH);	/* needs translation */
gotit:
		if (verbose)
			printf("using bad-sector file %d\n", bfnum);
	} else {
		/* try to use the given bad-sector file */
		if (!readbf(ds, bfnum, &bb->bb_cur))
			return (EINVAL);
		if ((n = checkbf(ds, bfnum, &bb->bb_cur, &fixed, 0)) < 0)
			return (EINVAL);
		bb->bb_justonefile = 1;
	}
	bb->bb_new.bt_csn = bb->bb_cur.bt_csn;
	bb->bb_file = bfnum;
	bb->bb_ncur = n;
	ds->d_bflags |= fixed ? DB_LOADED | DB_MODIFIED : DB_LOADED;
	return (0);
}

/*
 * Adjust auto-scan limits.  All we do here is clip off the end, where
 * the bad-sector files and replacement sectors live (one track + 126
 * sectors).
 */
void
bad144_scanlimit(ds, amin, amax)
	struct ddstate *ds;
	daddr_t *amin, *amax;
{
	struct bb144 *bb = ds->d_badinfo;

	if (*amax > bb->bb_replarea)
		*amax = bb->bb_replarea;
}

/*
 * Set auxiliary info (here, the cartridge serial number).
 */
int
bad144_setaux(ds, aux)
	struct ddstate *ds;
	char *aux;
{
	long new_sn;
	char *ep;
	struct bb144 *bb;

	new_sn = strtoul(aux, &ep, 0);
	if (ep == aux)
		return (EINVAL);
	bb = ds->d_badinfo;
	bb->bb_cur.bt_csn = bb->bb_new.bt_csn = new_sn;
	return (0);
}

/*
 * Add a <cyl,trk,bfi> triple as a bad spot.
 */
int
bad144_addbfi(ds, cyl, trk, bfi)
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
bad144_addsn(ds, sn)
	struct ddstate *ds;
	daddr_t sn;
{
	struct bb144 *bb = ds->d_badinfo;

	if (sn >= bb->bb_replarea)
		return (ERANGE);
	return (addone(ds, CYL(ds, sn), BT_TRKSEC(TRK(ds, sn), SEC(ds, sn))));
}

/*
 * Print the old and/or new tables.
 */
int
bad144_print(ds, how, which)
	struct ddstate *ds;
	enum printhow how;
	int which;
{
	struct bb144 *bb = ds->d_badinfo;

	switch (how) {

	case PRINT_DEFAULT:
	case PRINT_CTS:
	case PRINT_SN:
		break;

	case PRINT_BFI:
		return (EOPNOTSUPP);

	default:
		panic("bad144_print how");
	}
	if (which & PRINT_OLD) {
		printf("bad-block information at sector %ld in %s:\n",
		    (long)(bb->bb_bfarea + 2 * bb->bb_file), ds->d_name);
		printbf(ds, how, &bb->bb_cur);
	}
	if (which & PRINT_NEW) {
		if (bb->bb_new_unsorted) {
			sortbf(&bb->bb_new);
			bb->bb_new_unsorted = 0;
		}
		printf("new bad blocks for %s:\n", ds->d_name);
		printbf(ds, how, &bb->bb_new);
	}
	return (0);
}

static void
printbf(ds, how, bp)
	struct ddstate *ds;
	enum printhow how;
	struct dkbad *bp;
{
	struct bt_bad *bt;
	int i, cyl, trk, sec;
	daddr_t sn;

	printf("cartridge serial number: %ld(10) ", (long)bp->bt_csn);
	switch (bp->bt_flag) {
	case (u_short)-1:
		printf("alignment cartridge");
		break;
	case DKBAD_MAGIC:
		break;
	default:
		printf("type=%#x?", bp->bt_flag);
		break;
	}
	printf("\n");
	for (bt = bp->bt_bad, i = 0; i < BTSIZE; bt++, i++) {
		if ((cyl = bt->bt_cyl) == 0xffff && bt->bt_trksec == 0xffff)
			break;
		trk = BT_TRK(bt->bt_trksec);
		sec = BT_SEC(bt->bt_trksec);
		sn = SN(ds, cyl, trk, sec);
		printf("Bad Block: ");
		switch (how) {

		case PRINT_SN:
			printf("sn=%ld\n", (long)sn);
			break;

		case PRINT_DEFAULT:
			printf("sn=%ld, ", (long)sn);
			/* FALLTHROUGH */

		default:
			printf("cyl=%d, trk=%d, sec=%d\n", cyl, trk, sec);
			break;
		}
	}
}

/*
 * Commit the new changes (if any).
 *
 * If we fail here, we are pretty much dead, but...
 */
int
bad144_store(ds)
	struct ddstate *ds;
{
	struct bb144 *bb = ds->d_badinfo;
	int bug, err, i, tomove, yn;
	char *msg;
	struct dkbad *new;
	struct dkbad merged;
	daddr_t sources[BTSIZE];

	if (bb->bb_new_unsorted) {
		sortbf(&bb->bb_new);
		bb->bb_new_unsorted = 0;
	}
	if (ds->d_bflags & DB_LOADED) {
		initbf(&merged, bb->bb_new.bt_csn);
		tomove = merge(ds,
		    &bb->bb_cur.bt_bad[0], &bb->bb_new.bt_bad[0],
		    &merged.bt_bad[0], sources);
		if (tomove != bb->bb_ncur + bb->bb_nnew)
			panic("bad144_store: merge != ncur + nnew");
		new = &merged;
		msg = "Update";
	} else {
		if (!yflag && firstvalidbf(ds) >= 0)
			printf(
	"WARNING: about to overwrite an existing bad-block table!!!\n");
		msg = "Replace";
		new = &bb->bb_new;
		tomove = 0;
	}
	if (checkbf(ds, -1, new, &bug, 0) < 0 || bug)
		panic("bad144_store");

	if (nflag) {
		printf("%s bad block table -- skipped\n", msg);
		return (0);
	}

	do {
		printf("%s bad block table on %s (y/n)? ", msg, ds->d_name);
	} while ((yn = yesno()) < 0);
	if (yn != 'y')
		return (0);

	err = 0;

	/* Shuffle and/or initialize replacement sectors as needed. */
	if (tomove)
		err += domoves(ds, tomove, sources);

	if (bb->bb_justonefile)
		err += writebf(ds, bb->bb_file, new);
	else
		for (i = 0; i < 5; i++)
			err += writebf(ds, i, new);

#ifdef DIOCSBAD
	/* if faking the writes, no point in the ioctl */
	if (!no_io && ioctl(ds->d_fd, DIOCSBAD, (caddr_t)new) < 0)
		warn("can't sync kernel copy of bad-sector file");
#else
	warnx("can't sync kernel copy of bad-sector file");
#endif
	fprintf(stderr, "Bad-sector table written successfully\n");
	return (0);
}

/*
 * Convert bad block index `x' to replacement sector number,
 * where x \elem [0..BTSIZE).  Note that the first replacement
 * sector is the last sector of the second-to-last track.
 */
#define	REPL(bfarea, x) ((bfarea) - 1 - (x))

/*
 * Merge (sorted) current and new tables into one big (sorted) table.
 * We must also arrange to shuffle the replacement sectors such that
 * existing relocated sectors keep their old replacement data.  Whenever
 * a new replacement goes before a current replacement, it forces all
 * of those old replacements to get copied.  Rather than trying to be
 * tricky, we simply mark every possible replacement sector with its
 * `previous source' sector.  If no insertion applies to that replacement
 * sector, it will have itself as its source, while if an insertion
 * occurs, the new sector will have a non-replacement sector as its
 * source and all the `shuffled' old-replacements behind it will take
 * their source from higher-numbered replacement sectors.
 *
 * Example: suppose sectors 3 and 7 are currently replaced, and that
 * now 5 will be replaced as well.  The old and new (final) maps,
 * assuming sn999 is the first replacement:
 *
 * 	  (old)		  (new)
 *	sn0 => sn0	sn0 => sn0
 *	sn1 => sn1	sn1 => sn1
 *	sn2 => sn2	sn2 => sn2
 *	sn3 => sn999	sn3 => sn999
 *	sn4 => sn4	sn4 => sn4
 *	sn5 => sn5	sn5 => sn998
 *	sn6 => sn6	sn6 => sn6
 *	sn7 => sn998	sn7 => sn997
 *	sn8 => sn8	sn8 => sn8 [etc]
 *
 * Thus, sn997 (replacement #2) will be marked `source from sn998',
 * sn998 (replacement #1) will be marked `source from sn5', and sn999
 * (replacement #0) will be marked `source from sn999'.  The new total
 * replacement count is 3, and moving backwards through the list (i.e.,
 * (2,1,0)) and copying in that order (997=998; 998=5; 999=999) will
 * preserve the data correctly.
 */
static int
merge(ds, cur, new, merge, sources)
	struct ddstate *ds;
	struct bt_bad *cur, *new, *merge;
	daddr_t *sources;
{
	daddr_t bfarea;
	struct bb144 *bb = ds->d_badinfo;
	int cx, nx, mx;			/* index: cur,new,merge */

	bfarea = bb->bb_bfarea;
	for (cx = nx = mx = 0;; mx++) {
		if (mx >= BTSIZE)
			panic("merge: overrun");
		/*
		 * Pick lower numbered cylinder or trksec.
		 * Continue until both are at end (which is always
		 * greater than any actual replacement).
		 */
		if (cur[cx].bt_cyl < new[nx].bt_cyl)
			goto less;
		if (cur[cx].bt_cyl > new[nx].bt_cyl)
			goto greater;
		if (cur[cx].bt_trksec < new[nx].bt_trksec)
			goto less;
		if (cur[cx].bt_trksec > new[nx].bt_trksec)
			goto greater;
		if (cur[cx].bt_cyl == 0xffff && cur[cx].bt_trksec == 0xffff)
			break;
		panic("merge: equal");

	less:	/* cur[cx] < new[nx]: select cur replacement */
		merge[mx] = cur[cx];
		/* copy previous replacement's contents */
		sources[mx] = REPL(bfarea, cx);
		cx++;
		continue;

	greater: /* cur[cx] > new[nx]: select new replacement */
		merge[mx] = new[nx];
		/* copy original sector's contents */
		sources[mx] = SN(ds, new[nx].bt_cyl,
		    BT_TRK(new[nx].bt_trksec), BT_SEC(new[nx].bt_trksec));
		nx++;
	}
	return (mx);
}

/*
 * Apply the sector moves recorded in merge().
 */
static int
domoves(ds, n, sources)
	struct ddstate *ds;
	int n;
	daddr_t *sources;
{
	int errcnt, i;
	daddr_t dsn, ssn, bfarea, replarea;
	struct bb144 *bb = ds->d_badinfo;

	bfarea = bb->bb_bfarea;
	replarea = bb->bb_replarea;
	errcnt = 0;
	for (i = n; --i >= 0;) {
		/*
		 * Destination sector is always the i'th replacement.
		 * Source sector is as recorded in the table, and may
		 * be the same as the destination.
		 * Note the backwards copy (important!).
		 */
		ssn = sources[i];
		dsn = REPL(bfarea, i);
		bzero(ds->d_secbuf, ds->d_dl.d_secsize);
		if (ssn >= replarea) {
			/*
			 * Moving a replacement sector.  Should always
			 * work (bad sector area must be clean).
			 */
			if (verbose)
				printf("relocate sn%ld to sn%ld\n",
				    (long)ssn, (long)dsn);
			if (ssn == dsn || copysec(ds, ssn, dsn) == 0)
				continue;
			warn("relocation from sn%ld to sn%ld",
			    (long)ssn, (long)dsn);
			errcnt++;
			continue;
		}
		/*
		 * Copying an original sector.  OK to fail, will zero
		 * target sector in that case, but zero should always succeed.
		 */
		if (copytries) {
			switch (copysec(ds, ssn, dsn)) {

			case COPY_OK:
				if (verbose)
					printf("copied from sn%ld to sn%ld\n",
					    (long)ssn, (long)dsn);
				continue;

			case COPY_RDFAILED:
				if (verbose)
					printf(
	"read failed from sn%ld, wrote best guess at data to sn%ld\n",
					    (long)ssn, (long)dsn);
				continue;

			default:
				warn("copy failed writing sn%ld", (long)dsn);
				break;
			}
		} else {
			if (zerosec(ds, dsn) == 0) {
				if (verbose)
					printf("zero'ed sn%ld\n", (long)ssn);
				continue;
			}
			warn("clear failed on sn%ld", (long)dsn);
		}
		errcnt++;
	}
	return (errcnt);
}

/*
 * Similar to the search above, but does not do anything with the
 * resulting bad-sector file, just says whether there is one we think
 * is valid.  Returns the file number (0..4) or -1 for error.
 */
static int
firstvalidbf(ds)
	struct ddstate *ds;
{
	int bfnum, fixed;
	struct dkbad bt;

	for (bfnum = 0; bfnum < 5; bfnum++)
		if (readbf(ds, bfnum, &bt) == 0 &&
		    checkbf(ds, bfnum, &bt, &fixed, 1) >= 0)
			return (bfnum);
	return (-1);
}

/*
 * Initialize bad-sector-file info.
 */
static void
initbf(bp, csn)
	struct dkbad *bp;
	long csn;
{
	int i;

	bp->bt_csn = csn;
	bp->bt_mbz = 0;
	bp->bt_flag = DKBAD_MAGIC;
	for (i = 0; i < BTSIZE; i++) {
		bp->bt_bad[i].bt_cyl = 0xffff;
		bp->bt_bad[i].bt_trksec = 0xffff;
	}

}

/*
 * Read the specified bad-sector file from the disk.
 */
static int
readbf(ds, bfnum, abt)
	struct ddstate *ds;
	int bfnum;
	struct dkbad *abt;
{
	struct bb144 *bb = ds->d_badinfo;
	int err;
	daddr_t sn;

	sn = bb->bb_bfarea + (2 * bfnum);
	if (ds->d_dl.d_secsize == sizeof *abt)
		err = rsec(ds, abt, sn);
	else {
		err = rsec(ds, ds->d_secbuf, sn);
		memcpy(abt, ds->d_secbuf, sizeof *abt);
	}
	if (err && verbose)
		warn("%s: bad-sector file %d: read", ds->d_name, bfnum);
	return (err);
}

/*
 * Write the specified bad-sector file to the disk.
 */
static int
writebf(ds, bfnum, abt)
	struct ddstate *ds;
	int bfnum;
	struct dkbad *abt;
{
	struct bb144 *bb = ds->d_badinfo;
	int err;
	daddr_t sn;

	sn = bb->bb_bfarea + (2 * bfnum);
	if (verbose)
		printf("writing bad-sector file %d at %ld\n", bfnum, (long)sn);
	if (ds->d_dl.d_secsize == sizeof *abt)
		err = wsec(ds, abt, sn);
	else {
		/*
		 * dkbad info occupies less than 1 sector -- read whole
		 * sector, insert, write back.  (???)
		 */
		err = rsec(ds, ds->d_secbuf, sn);
		if (err) {
			warn("%s: sn%ld: bad-sector file %d: read",
			    ds->d_name, (long)sn, bfnum);
			return (err);
		}
		memcpy(ds->d_secbuf, abt, sizeof *abt);
		err = wsec(ds, ds->d_secbuf, sn);
	}
	if (err)
		warn("%s: sn%ld: bad-sector file %d: write",
		    ds->d_name, (long)sn, bfnum);
	return (err);
}

/*
 * Check the given bad-sector file, and count the number of entries
 * used.  Return -1 if bad, 0 if OK or fixed-up (and set *afixed).
 */
static int
checkbf(ds, bfnum, bp, afixed, quiet)
	struct ddstate *ds;
	int bfnum;
	struct dkbad *bp;
	int *afixed, quiet;
{
	int fixed, errs, i, sorted, cyl, trk, sec;
	long sn, prevsn;
	struct bt_bad *bt;

	fixed = errs = sorted = 0;
	if (bp->bt_flag != 0xffff && bp->bt_flag != DKBAD_MAGIC) {
		if (!quiet)
			warnx("%s: bad-sector file %d: bad flag (%x != %x)",
			    ds->d_name, bfnum, bp->bt_flag, DKBAD_MAGIC);
		errs = 1;
	}
	if (bp->bt_mbz != 0) {
		if (!quiet)
			warnx(
"%s: bad-sector file %d: bad magic number (%x != 0)",
			    ds->d_name, bfnum, bp->bt_mbz);
		errs = 1;
	}
	prevsn = -1;
	for (bt = bp->bt_bad, i = 0; i < BTSIZE; bt++, i++) {
		if ((cyl = bt->bt_cyl) == 0xffff && bt->bt_trksec == 0xffff)
			continue;
		trk = BT_TRK(bt->bt_trksec);
		sec = BT_SEC(bt->bt_trksec);
		sn = SN(ds, cyl, trk, sec);
		if (cyl >= ds->d_dl.d_ncylinders ||
		    trk >= ds->d_dl.d_ntracks ||
		    sec >= ds->d_dl.d_nsectors) {
			if (!quiet)
				warnx(
"%s: bad-sector file %d: out of range: sn=%ld, cyl=%d, trk=%d, sec=%d",
				    ds->d_name, bfnum, sn, cyl, trk, sec);
			errs = 1;
		}
		if (sn < prevsn)
			sorted = 1;	/* well, soon to be sorted */
		prevsn = sn;
	}
	if (sorted) {
		if (!quiet)
			warnx("%s: bad-sector file %d is jumbled",
			    ds->d_name, bfnum);
		sortbf(bp);
	}

	/* now that it is sorted, run a second pass to check for dups */
	prevsn = -1;
	for (bt = bp->bt_bad, i = 0; i < BTSIZE; bt++, i++) {
		if ((cyl = bt->bt_cyl) == 0xffff && bt->bt_trksec == 0xffff)
			break;
		trk = BT_TRK(bt->bt_trksec);
		sec = BT_SEC(bt->bt_trksec);
		sn = SN(ds, cyl, trk, sec);
		if (sn == prevsn) {
			if (!quiet)
				warnx(
"%s: bad-sector file %d contains duplicates (sn %ld): %d", 
				    ds->d_name, bfnum, sn, i);
			errs = 1;
		}
	}
	if (errs)
		return (-1);
	if (sorted)
		fixed = 1;
	*afixed = fixed;
	return (i);
}

static void
sortbf(bp)
	struct dkbad *bp;
{

	qsort(bp->bt_bad, BTSIZE, sizeof *bp->bt_bad, btcompare);
}

/* bt_bad sorter for qsort() */
static int
btcompare(l0, r0)
	const void *l0, *r0;
{
	const struct bt_bad *l, *r;

	l = l0;
	r = r0;
	if (l->bt_cyl < r->bt_cyl)
		return (-1);
	if (l->bt_cyl > r->bt_cyl)
		return (1);
	if (l->bt_trksec < r->bt_trksec)
		return (-1);
	if (l->bt_trksec > r->bt_trksec)
		return (1);
	return (0);
}

static int
addone(ds, cyl, trksec)
	struct ddstate *ds;
	int cyl, trksec;
{
	struct bt_bad *bt;
	struct bb144 *bb = ds->d_badinfo;
	int i;

	if (debug)
		printf("add bad block: cyl=%d trk=%d sec=%d ... ",
		    cyl, BT_TRK(trksec), BT_SEC(trksec));

	/*
	 * Look through the current list to see if we already have it.
	 * The list is sorted, so we can stop early.
	 */
	if (ds->d_bflags & DB_LOADED) {
		for (bt = bb->bb_cur.bt_bad, i = 0; i < BTSIZE; bt++, i++) {
			if (bt->bt_cyl < cyl)
				continue;
			if (bt->bt_cyl > cyl)
				break;
			if (bt->bt_trksec == trksec) {
				if (debug)
					printf("already in map\n");
				return (0);
			}
			if (bt->bt_trksec > trksec)
				break;
		}
	}

	/*
	 * Looks like a new block.  Make sure there is room, and
	 * add it to the `new blocks' table.
	 */
	if (bb->bb_ncur + bb->bb_nnew + 1 >= BTSIZE)
		return (ENOSPC);	/* needs translation */

	for (bt = bb->bb_new.bt_bad, i = 0; i < BTSIZE; bt++, i++)
		if (bt->bt_cyl == 0xffff && bt->bt_trksec == 0xffff)
			goto found;
	panic("addone: no room");
found:
	bt->bt_cyl = cyl;
	bt->bt_trksec = trksec;
	bb->bb_nnew++;
	bb->bb_new_unsorted = 1;
	if (debug)
		printf("\n");
	return (0);
}
