/*
 * Copyright (c) 1992, 1995
 * Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifndef lint
static char rcsid[] = "BSDI diskdefect.c,v 2.4 1995/10/16 22:13:37 prb Exp";
static char copyright[] =
"@(#) Copyright (c) 1980,1986,1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif

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
#include <time.h>
#include <unistd.h>

#include "diskdefect.h"
#include "bbops.h"

#define	RETRIES		10

/*
 * Global operational flags.
 */
int	copytries;	/* -c => copy original contents (w/ # tries) */
int	debug;		/* -d => debug flag */
int	nflag;		/* -n => no changes to disk */
int	no_io;		/* -N => fake all read/write calls */
int	verbose;	/* -v => talkative */
int	yflag;		/* -y => assume `yes' to any question */

/*
 * Secret debugging trick (in util.c, routine doio()).
 */
int	fake_errors;
double	error_rate;

static void usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, i, omode, no_changes;
	int aflag, sflag, wflag;
	char *auxarg, *bbarg, *geomarg, *typearg;
	char *diskname;
	daddr_t sn;
	struct ddstate dd;

	copytries = debug = nflag = no_io = verbose = yflag = 0;
	aflag = sflag = wflag = 0;
	auxarg = bbarg = geomarg = typearg = NULL;
	while ((ch = getopt(argc, argv, "ab:cC:dE:g:inNqst:vwy")) != EOF) {
		switch (ch) {

		case 'a':
			aflag = 1;
			break;

		case 'b':
			if (bbarg != NULL)
				usage();
			bbarg = optarg;
			break;

		case 'c':
			copytries = RETRIES;
			break;

		case 'C':
			copytries = strtol(optarg, 0, NULL);
			break;

		case 'd':
			debug = 1;
			break;

		case 'E':	/* secret debug flag: force errors in scan */
			fake_errors = 1;
			error_rate = strtod(optarg, NULL) / 100.0;
			(void)srand((int)time(NULL) ^ (int)getpid());
			break;

		case 'g':
			if (geomarg != NULL)
				usage();
			geomarg = optarg;
			break;

		case 'n':
			nflag = 1;
			break;

		case 'N':	/* secret debug flag: skip all I/O */
			no_io = 1;
			break;

		case 's':
			sflag = 1;
			break;

		case 't':
			if (typearg != NULL)
				usage();
			typearg = optarg;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'w':
			wflag = sflag = 1;
			break;

		case 'y':
			yflag = 1;
			break;

		case '?':
			usage();
			/* NOTREACHED */

		default:
			panic("main: option letter");
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * Remaining arguments are:
	 *	disk name
	 *	aux info (yech!)
	 *	sectors to add/set
	 */
	if (argc == 0) {
		usage();
		/* NOTREACHED */
	}

	argc--;
	diskname = *argv++;

	if (argc > 0) {			/* XXX compat */
		argc--;			/* XXX compat */
		auxarg = *argv++;	/* XXX compat */
	}				/* XXX compat */

	/*
	 * Remaining arguments are sector numbers.
	 * Try to open the named disk, then check a few things.
	 */
	omode = (nflag && !wflag) || no_io ||
	    (auxarg == 0 && argc == 0 && !sflag) ? O_RDONLY : O_RDWR;
	if (opendisk(&dd, diskname, omode))
		exit(1);
	if (geomarg) {
		if (setgeom(&dd, geomarg))
			exit(1);
	} else {
		if (getgeom(&dd))
			exit(1);
	}
	if (typearg) {
		if (setbadtype(&dd, typearg))
			exit(1);
	} else {
		if (getbadtype(&dd))
			exit(1);
	}
	if (nflag && wflag) {
		do {
			printf(
	"Write-scan without adding bad blocks -- are you sure? ");
		} while ((i = yesno()) < 0);
		if (i != 'y')
			exit(1);
	}

	no_changes = auxarg == NULL && argc == 0 && !sflag;

	if (aflag || no_changes)
		if (loaddefects(&dd, bbarg ? LOAD_USER : LOAD_CUR, bbarg))
			exit(1);

	if (no_changes) {
		printdefects(&dd, PRINT_DEFAULT, PRINT_OLD);
		exit(0);
	}

	/*
	 * Set auxiliary info if desired.
	 */
	if (auxarg != NULL)
		if (setaux(&dd, auxarg))
			exit(1);

	/*
	 * Scan for new bad blocks.
	 */
	if (sflag)
		if (scandisk(&dd, wflag, verbose))
			exit(1);

	/*
	 * Add any sectors specified on the command line.
	 */
	for (i = 0; i < argc; i++) {
		char *arg, *ep;

		arg = argv[i];
		sn = strtol(arg, &ep, 0);
		if (ep == arg || *ep != '\0')
			errx(1, "%s: not a sector number", arg);
		if ((u_long)sn >= dd.d_dl.d_secperunit)
			errx(1, "sector %ld out of range [0,%ld) for disk %s",
			    (long)sn, (long)dd.d_dl.d_secperunit, dd.d_name);
		if (addsn(&dd, sn))
			exit(1);
	}

	if (verbose)
		printdefects(&dd, PRINT_DEFAULT, PRINT_NEW);

	/*
	 * Commit changes.
	 */
	if (store(&dd))
		exit(1);

	return (0);
}

static void
usage()
{
	char *opts = "[-cswnvy] [-b arg] [-t type]";

	fprintf(stderr, "\
usage:  diskdefect %s disk [disk-info [bad-block ...]]\n\
\t\tread or overwrite bad-sector table, e.g.: diskdefect wd0\n\
   or:  diskdefect -a %s disk [bad-block ...]\n\
\t\tadd to bad-sector table.\n\
\noptions are:\n\
\tdisk      name of disk (e.g. wd0)\n\
\tdisk-info new disk info (serial number)\n\
\tbad-block block numbers to add to bad-block table\n\
\t-a        add new bad sectors to the table\n\
\t-c        copy original sector to replacement\n\
\t-s        scan disk for bad sectors (read only unless -w)\n\
\t-w        enable write test in scan (off by default)\n\
\t-n        do not write out bad-block table (enables verbose)\n\
\t-q        scan quietly\n\
\t-v        verbose mode\n\
\t-y        don't ask any questions (assume yes)\n\
\t-b arg    auxiliary bad-block selection/specification\n",
	    opts, opts);
	exit(1);
}
