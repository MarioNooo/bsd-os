/*-
 * Copyright (c) 1995, 1996  Berkeley Software Design, Inc. 
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI console.c,v 1.1 1997/01/02 17:38:35 pjd Exp
 */

#include <sys/types.h>
#include <sys/fcntl.h>

#include <err.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define	_PATH_CONSKBD	"/dev/conskbd0"

__dead void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, fd, hflag;
	char *file;

	hflag = 0;
	file = NULL;
	while ((ch = getopt(argc, argv, "f:h")) != EOF)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'h':
			hflag = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (hflag) {
		bell_help();
		repeat_help();
		keymap_help();
		exit (0);
	}

	/*
	 * We could be logged in on /dev/conskbd0 or on /dev/console, and
	 * could then be running X (so we don't have a descriptor on the
	 * console from the current shell).  The console might be a serial
	 * port or conskbd0.
	 *
	 * Open /dev/conskbd0 if possible, which should be true if we are
	 * logged in there.  If that fails and we can open the console, use
	 * it.  If we can open neither, we don't have permission to do this.
	 */
	if (file == NULL) {
		if ((fd = open(_PATH_CONSKBD, O_RDWR, 0)) == -1)
			fd = open(_PATH_CONSOLE, O_RDWR);
		file = _PATH_CONSOLE;
	} else {
		if (strcmp(file, "-") == 0)
			fd = STDIN_FILENO;
		else
			fd = open(file, O_RDWR, 0);
	}
	if (fd == -1)
		err(1, "%s", file);

	/* If no arguments, report all current settings */
	if (argc == 0) {
		bell(0, argv, fd);
		repeat(0, argv, fd);
	} else {
		if (strcmp(argv[0], "bell") == 0)
			exit(bell(argc - 1, argv + 1, fd));

		if (strcmp(argv[0], "repeat") == 0)
			exit(repeat(argc - 1, argv + 1, fd));

		if (strcmp(argv[0], "keymap") == 0)
			exit(keymap(argc - 1, argv + 1, fd));
		usage();
	}
	exit (0);
}

__dead void
usage()
{
	fprintf(stderr,
	    "usage: setcons [-h] [-f device-file] [category [command ...]]\n");
	exit(1);
}
