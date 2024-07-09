/*
 * Copyright (c) 1992, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifndef lint
static char rcsid[] = "BSDI scan.c,v 2.1 1995/07/25 22:20:08 torek Exp";
#endif

/*
 * Scan a partition, looking for errors.
 */

#include <sys/param.h>
#include <sys/disklabel.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "diskdefect.h"
#include "bbops.h"

struct scaninfo {
	daddr_t	si_start;	/* beginning of scan region */
	daddr_t	si_wstart;	/* beginning of write scan region */
	daddr_t	si_end;		/* end of scan region */
	int	si_cutesy;	/* cutesy per-cylinder display */
	char	*si_bigbuf;	/* large buffer (1 cylinder, if possible) */
	int	si_bigsecs;	/* large buffer size, in sectors */
};

static int	check_bogon __P((struct scaninfo *, daddr_t, int));
static daddr_t	check_size __P((struct disklabel *, int));
static int	doscan __P((struct ddstate *, struct scaninfo *));
static int	dosectors __P((struct ddstate *, struct scaninfo *,
				daddr_t, int));
static void	scancatch __P((int));

static volatile sig_atomic_t interrupt;

/*
 * Scan the (open, set-up) disk for bad sectors.
 * Caller can control whether we write and/or do cutesy display.
 */
int
scandisk(ds, dowrite, docutesy)
	struct ddstate *ds;
	int dowrite, docutesy;
{
	int error;
	size_t sz;
	sig_t osig;
	struct scaninfo si;

	/*
	 * Set up scan region limits.
	 * Out of paranoia, we never write the first track at all.
	 * (XXX should be able to write from label onward)
	 */
	if ((si.si_end = check_size(&ds->d_dl, ds->d_part)) == 0)
		return (-1);
	si.si_start = 0;
	adjlimits(ds, &si.si_start, &si.si_end);
	if (dowrite) {
		si.si_wstart = si.si_start;
		if (si.si_wstart < ds->d_dl.d_nsectors)
			si.si_wstart = ds->d_dl.d_nsectors;
	} else
		si.si_wstart = si.si_end;
	si.si_cutesy = docutesy;

	sz = ds->d_dl.d_secpercyl * ds->d_dl.d_secsize;
	if (sz == 0 || sz != ds->d_dl.d_secpercyl * ds->d_dl.d_secsize) {
		warnx("can't scan by cylinders (geometry is wrong?)");
		return (-1);
	}
	if (sz > INT_MAX || (si.si_bigbuf = malloc(sz)) == NULL) {
		warnx("cannot allocate %lu bytes for cylinder buffer",
		    (u_long)sz);
		sz = ds->d_dl.d_nsectors * ds->d_dl.d_secsize;
		/* assume a track *has* to fit in an int */
		if ((si.si_bigbuf = malloc(sz)) == NULL) {
			warn("cannot allocate %lu bytes for track buffer",
			    (u_long)sz);
			return (-1);
		}
	}
	si.si_bigsecs = sz / ds->d_dl.d_secsize;

	printf("Scanning %s (%d cylinders) for defects:", ds->d_name,
	    (int)CYL(ds, si.si_end) - (int)CYL(ds, si.si_start));
	fflush(stdout);
	interrupt = 0;
	osig = signal(SIGINT, scancatch);
	error = doscan(ds, &si);
	(void)signal(SIGINT, osig);
	free(si.si_bigbuf);
	putchar('\n');

	if (error > 0) {
		printf("%s: %s\n", ds->d_name, str_adderror(error));
	} else if (interrupt) {
		printf("scan interrupted\n");
		error = -1;
	}
	return (error);
}

/*
 * Return the size of the partition (in sectors); prints warnings
 * if things don't agree.
 */
static daddr_t
check_size(dlp, part)
	struct disklabel *dlp;
	int part;
{
	daddr_t rawsize, diskgeom, disklabel;
	int w;

	rawsize = dlp->d_partitions[part].p_size;

	/*
	 * If the label type says SCSI, just trust the partition size,
	 * as the geometry is probably fictional.  Otherwise look at
	 * other ways of computing the disk size.
	 */
	if (dlp->d_type == DTYPE_SCSI)
		return (rawsize);

	diskgeom = dlp->d_nsectors * dlp->d_ntracks * dlp->d_ncylinders;
	disklabel = dlp->d_secperunit;
	w = 0;
	if (rawsize != diskgeom) {
		printf("WARNING: raw partition size(%ld)", (long)rawsize);
		printf(" does not match disk geometry(%ld)\n", (long)diskgeom);
		w = 1;
	}
	if (rawsize != disklabel) {
		printf("WARNING: raw partition size(%ld)", (long)rawsize);
		printf(" does not match disk label(%ld)\n", (long)disklabel);
		w = 1;
	}
	if (w) {
		do {
			printf("Continue anyway? ");
		} while ((w = yesno()) < 0);
		if (w != 'y')
			return (0);
	}
	return (rawsize);
}

/*
 * Bogon detector.  Call this to complain when read or write fails
 * to make sure that the failure was reasonable.
 */
static int
check_bogon(sp, sn, error)
	struct scaninfo *sp;
	daddr_t sn;
	int error;
{

	if (error == EIO)
		return (0);
	putchar('\n');
	if (error == 0)
		warnx("sn%ld: got EOF (partition size mangled?)", (long)sn);
	else
		warnx("sn%ld: unexpected (non I/O) error: %s", (long)sn,
		    strerror(error));
	return (-1);
}

/*
 * User ^C handler.
 */
static void
scancatch(sig)
	int sig;
{

	interrupt = 1;
}

static int
doscan(ds, sp)
	struct ddstate *ds;
	struct scaninfo *sp;
{
	daddr_t sn;
	int cyl, nsec, len, first, error;
	off_t off;

	first = 1;
	for (sn = sp->si_start; sn < sp->si_end; sn += nsec) {
		if (interrupt)
			return (-1);
		/*
		 * Scan from sn to next cyl boundary, to end,
		 * or for size of buffer, whichever is smallest.
		 */
		cyl = CYL(ds, sn);
		nsec = SN(ds, cyl + 1, 0, 0) - sn;	/* to boundary */
		if (sn + nsec > sp->si_end)
			nsec = sp->si_end - sn;		/* to end */
		if (nsec > sp->si_bigsecs)
			nsec = sp->si_bigsecs;		/* to buffer size */

		if (first || cyl % 100 == 0) {
			printf("\ncylinder %5.5d:", cyl);
			first = 0;
		}
		if (sp->si_cutesy) {
			if (cyl % 50 == 0)
				putchar('\n');
			putchar('r');
			fflush(stdout);
		}
		off = (off_t)sn * ds->d_dl.d_secsize;
		len = nsec * (int)ds->d_dl.d_secsize;
		errno = 0;

		/* Try reading the whole thing. */
		if (ratoff(ds, sp->si_bigbuf, off, len))
			goto pinpoint;

		/*
		 * Well, we can read it all ... write it (maybe only
		 * some of it) if it falls in the range marked writable.
		 * Note that if we are not writing, no range is writable.
		 */
		if (sn + nsec > sp->si_wstart) {
			if (sp->si_cutesy)
				printf("\bw"), fflush(stdout);

			if (sn >= sp->si_wstart) {
				/* Try writing the whole cylinder. */
				errno = 0;
				if (watoff(ds, sp->si_bigbuf, off, len))
					goto pinpoint;
			} else {
				/* Not whole cylinder; use sector scan. */
				int nskip;

				nskip = sp->si_wstart - sn;
				if (nskip >= nsec)
					panic("doscan: nskip >= nsec");
				error = dosectors(ds, sp, sn + nskip,
				    nsec - nskip);
				if (error != 0)
					return (error);
			}
		}
		if (sp->si_cutesy)
			printf("\b."), fflush(stdout);
		continue;
pinpoint:
		/* pinpoint the error */
		if ((error = check_bogon(sp, sn, errno)) != 0 ||
		    (error = dosectors(ds, sp, sn, nsec)) != 0)
			return (error);
	}
	if (sp->si_cutesy)
		putchar('\n');
	return (0);
}

/*
 * Read/write one sector at a time.
 *
 * (This way, when we get an error, we know which sector it is in.)
 */
static int
dosectors(ds, sp, sn, nsec)
	struct ddstate *ds;
	struct scaninfo *sp;
	daddr_t sn;
	int nsec;
{
	static char rot[] = "/-\\|";
	u_int rotx = 0;
	int i, cutechar, error;

	if (interrupt)
		return (-1);
	cutechar = '.';
	for (i = 0; i < nsec; sn++, i++) {
		if (sp->si_cutesy)
			printf("\b%c", rot[rotx++ & 3]), fflush(stdout);
		errno = 0;
		if (rsec(ds, sp->si_bigbuf, sn)) {
			if (interrupt)
				return (-1);
			cutechar = 'x';
			if ((error = check_bogon(sp, sn, errno)) != 0 ||
			    (error = addsn(ds, sn)) != 0)
				return (error);
		} else if (sn >= sp->si_wstart) {
			errno = 0;
			if (wsec(ds, sp->si_bigbuf, sn)) {
				if (interrupt)
					return (-1);
				cutechar = 'x';
				if ((error = check_bogon(sp, sn, errno)) != 0 ||
				    (error = addsn(ds, sn)) != 0)
					return (error);
			}
		}
	}
	if (sp->si_cutesy)
		printf("\b%c", cutechar), fflush(stdout);
	return (0);
}
