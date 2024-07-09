/*
 * Copyright (c) 1992, 1995
 * Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * Subroutine interface between `high level' code and bad-block functions.
 * Generally, we just call the function, then do any needed error printing
 * (including translation of overloaded error codes) and return 0
 * for success.
 */

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "diskdefect.h"
#include "bbops.h"

/* XXX the below should be elsewhere (if anywhere) */
/* disk partition containing badsector tables */
#ifdef tahoe
#define RAWPART 'a'
#else
#define	RAWPART	'c'
#endif

static void gotgeom __P((struct ddstate *));

/*
 * Open the given disk and return 0, or print an eror message and
 * return nonzero.  Handles default "/dev/rwd0c"-style naming.
 */
int
opendisk(ds, name, omode)
	struct ddstate *ds;
	char *name;
	int omode;
{
	int fd;
	char *path;
	char pathbuf[PATH_MAX];

	if (*name != '/') {
		(void)sprintf(pathbuf, "%sr%s%c", _PATH_DEV, name, RAWPART);
		path = pathbuf;
	} else
		path = name;
	if ((fd = open(path, omode, 0600)) < 0) {
		warn("open(%s, read-%s)", path,
		    omode == O_RDWR ? "write" : "only");
		return (-1);
	}

	/* Got it; stash the name and descriptor. */
	if ((ds->d_name = strdup(path)) == NULL) {
		warn("out of memory");
		(void)close(fd);
		return (-1);
	}
	ds->d_fd = fd;
	ds->d_part = ds->d_name[strlen(ds->d_name) - 1] - 'a';
	ds->d_rawpart = RAWPART - 'a';
	ds->d_bb = NULL;
	return (0);
}

void
closedisk(ds)
	struct ddstate *ds;
{

	if (ds->d_bb != NULL)
		ds->d_bb->bb_detach(ds);
	close(ds->d_fd);
	free(ds->d_name);
	ds->d_fd = -1;
	ds->d_name = NULL;
	ds->d_bb = NULL;
	ds->d_badinfo = NULL;
}

/*
 * Read in the disk's geometry.
 */
int
getgeom(ds)
	struct ddstate *ds;
{

	if (ioctl(ds->d_fd, DIOCGDINFO, &ds->d_dl)) {
		warn("%s: ioctl(DIOCGDINFO)", ds->d_name);
		return (-1);
	}
	gotgeom(ds);
	return (0);
}

/*
 * Assign a geometry via disktab.
 */
int
setgeom(ds, geomname)
	struct ddstate *ds;
	char *geomname;
{
	struct disklabel *d;

	if ((d = getdiskbyname(geomname)) == NULL) {
		warnx("%s: unknown disk type", geomname);
		return (-1);
	}
	ds->d_dl = *d;
	gotgeom(ds);
	return (0);
}

static void
gotgeom(ds)
	struct ddstate *ds;
{
	struct disklabel *dlp;

	dlp = &ds->d_dl;
	if (verbose)
		printf("\
Disk geometry:\n\
    secsize:         %ld\n\
    nsectors/track:  %ld\n\
    ntracks/cyl:     %ld\n\
    ncylinders/unit: %ld\n\
    secpercyl:       %ld\n\
    secperunit:      %ld\n\n",
		    dlp->d_secsize, dlp->d_nsectors, dlp->d_ntracks,
		    dlp->d_ncylinders, dlp->d_secpercyl, dlp->d_secperunit);

#ifdef notdef
	ds->d_phys_spt = dlp->d_nsectors + dlp->d_sparespertrack;
	ds->d_phys_tpc = dlp->d_ntracks + ?;
	ds->d_phys_spc = dlp->d_secpercyl + dlp->d_sparespercylinder;
#endif
	ds->d_secbuf = emalloc(dlp->d_secsize);	/* ??? elsewhere? */
}

/*
 * Pick the proper bad-block revectoring methods.
 */
int
getbadtype(ds)
	struct ddstate *ds;
{
	struct bbops *bb;
	int i;

	if (verbose) {
		printf("auto-sense bad block ops for %s ...", ds->d_name);
		fflush(stdout);
	}
	for (i = 0; i < nbbops; i++) {
		bb = bbops[i];
		if (bb->bb_attach(ds, 0) == 0)
			goto found;
	}
	if (verbose)
		printf(" failed\n");
	warnx("cannot find bad block method for %s", ds->d_name);
	return (-1);
found:
	if (verbose)
		printf(" using %s\n(%s)\n", bb->bb_name, bb->bb_longname);
	ds->d_bb = bb;
	return (0);
}

/*
 * Select the given bad-block revectoring methods.
 */
int
setbadtype(ds, ty)
	struct ddstate *ds;
	char *ty;
{
	struct bbops *bb;
	int error, i;

	for (i = 0; i < nbbops; i++) {
		bb = bbops[i];
		if (strcmp(bb->bb_name, ty) == 0)
			goto found;
	}
	warnx("%s: no such bad block method", ty);
	fprintf(stderr, "(known methods are:");
	for (i = 0; i < nbbops; i++)
		fprintf(stderr, " %s", bbops[i]->bb_name);
	fprintf(stderr, ")\n");
	return (-1);
found:
	if ((error = bb->bb_attach(ds, 1)) == 0) {
		ds->d_bb = bb;
		return (0);
	}
	warnx("%s: %s: %s", ds->d_name, ty, strerror(error));
	return (-1);
}

/*
 * Clear some defects.
 */
int
cleardefects(ds, which)
	struct ddstate *ds;
	int which;
{
	int error;

	error = ds->d_bb->bb_clear(ds, which);
	if (error)
		warnx("%s: clear %s defect list%s: %s", ds->d_name,
		    which == CLEAR_OLD ? "old" :
		    which == CLEAR_NEW ? "new" :
		    which == CLEAR_BOTH ? "both" : "???",
		    which == CLEAR_BOTH ? "s" : "",
		    strerror(error));
	return (error);
}

/*
 * Load current or manufacturer defect table, or as specified by user.
 */
int
loaddefects(ds, how, arg)
	struct ddstate *ds;
	enum loadhow how;
	char *arg;
{
	int error;

	if (ds->d_bflags & DB_LOADED) {
		warnx("%s: defect list already loaded; clearing old info",
		    ds->d_name);
		if ((error = cleardefects(ds, CLEAR_OLD)) != 0)
			return (error);
	}
	error = ds->d_bb->bb_load(ds, how, arg);
	if (error) {
		/*
		 * ESRCH translates to "cannot find defect list";
		 * others are used as is.
		 */
		warnx("%s: %s", ds->d_name, error == ESRCH ?
		    "cannot find defect list" : strerror(error));
	}
	return (error);
}

/*
 * Set auxiliary info.
 * This is really silly, but compatibility calls...
 */
int
setaux(ds, arg)
	struct ddstate *ds;
	char *arg;
{
	int error;

	error = ds->d_bb->bb_setaux(ds, arg);
	if (error)
		warnx("%s: set auxiliary info to `%s': %s",
		    ds->d_name, arg, strerror(error));
	return (error);
}

/*
 * Adjust scan limits to leave room for bad-sector information.
 */
void
adjlimits(ds, amin, amax)
	struct ddstate *ds;
	daddr_t *amin, *amax;
{

	ds->d_bb->bb_scanlimit(ds, amin, amax);
}

/*
 * Print out defect list.
 */
int
printdefects(ds, how, which)
	struct ddstate *ds;
	enum printhow how;
	int which;
{
	int error;

	error = ds->d_bb->bb_print(ds, how, which);
	if (error)
		warnx("%s: print %s: %s", ds->d_name,
		    how == PRINT_DEFAULT ? "default" :
		    how == PRINT_CTS ? "cyl/trk/sec" :
		    how == PRINT_SN ? "sector number" :
		    how == PRINT_BFI ? "cyl/trk/bfi" : "???",
		    strerror(error));
	return (error);
}

int
store(ds)
	struct ddstate *ds;
{
	int error;

	error = ds->d_bb->bb_store(ds);
	if (error)
		warnx("%s (-t %s): store new table: %s",
		    ds->d_name, ds->d_bb->bb_name, strerror(error));
	return (error);
}

char *
str_adderror(error)
	int error;
{
	char *msg;

	switch (error) {

	case ERANGE:
		msg = "out of range (no such sector)";
		break;

	case ENOSPC:
		msg = "bad block table is full";
		break;

	default:
		msg = strerror(error);
		break;
	}
	return (msg);
}
