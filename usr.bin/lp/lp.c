/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI lp.c,v 2.1 1998/04/16 19:23:22 jch Exp
 */
#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pathnames.h"

void usage __P((void));

/*
 * lp --
 *	Stub front-end for lpr matching the POSIX 1003.2-1992 lp
 *	specification.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	char **av, **avp, narg[100], *ncopy, *printer;

	if ((av = calloc(argc + 6, sizeof(const char *))) == NULL)
		err(1, NULL);

	ncopy = printer = NULL;
	while ((ch = getopt(argc, argv, "cd:n:")) != EOF)
		switch (ch) {
		/* -c says don't copy files, which is lpr's default. */
		case 'c':
			break;

		/* -d allows specification of a printer. */
		case 'd':
			printer = optarg;
			break;

		/* -n allows specification of the number of copies. */
		case 'n':
			ncopy = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * POSIX.2 says that LPDEST environment variable overrides PRINTER,
	 * but is itself overridden by -d.
	 */
	if (printer == NULL)
		printer = getenv("LPDEST");

	avp = av;
	*avp++ = "lpr";
	if (printer != NULL) {
		*avp++ = "-P";
		*avp++ = printer;
	}
	if (ncopy != NULL) {
		/*
		 * XXX
		 * Historically, lpr(1) didn't permit <blank> characters
		 * between the "-#" and the argument.
		 */
		(void)snprintf(narg, sizeof(narg), "-#%s", ncopy);
		*avp++ = narg;
	}

	while (argc-- > 0) {
		if (strcmp(*argv, "-") == 0)
			*avp++ = "/dev/stdin";
		else
			*avp++ = *argv++;
	}
	*avp = NULL;

	execv(_PATH_LPR, av);
	err(1, "%s", _PATH_LPR);
	/* NOTREACHED */
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: lp [-c] [-d printer] [-n copies] [file ...]\n");
	exit(1);
}
