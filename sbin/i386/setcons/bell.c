/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bell.c,v 1.1 1997/01/02 17:38:35 pjd Exp
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <i386/isa/pcconsioctl.h>

#include "extern.h"

static void bell_usage __P((void));

int
bell(argc, argv, fd)
	int argc, fd;
	char *argv[];
{
	struct cnbell bell;

	if (ioctl(fd, PCCONIOCGBELL, (caddr_t)&bell) == -1)
		err(1, "get console bell parameters");

	if (argc == 0) {
		printf("bell ");
		switch (bell.type) {
		case BEL_NONE:
			printf("none\n");
			break;
		case BEL_AUDIBLE:
			printf("audible freq %d duration %d\n",
			    bell.freq, bell.duration);
			break;
		default:
			printf("unknown?\n");
			break;
		}
		return (0);
	}
	for (; argc; argc--, argv++) {
		if (strcmp(argv[0], "none") == 0) {
			if (argc > 1)
				goto usage;
			bell.type = BEL_NONE;
			continue;
		}
		if (strcmp(argv[0], "audible") == 0) {
			bell.type = BEL_AUDIBLE;
			continue;
		}
		if (strcmp(argv[0], "freq") == 0) {
			if (argc < 2)
				goto usage;
			bell.type = BEL_AUDIBLE;
			bell.freq = atoi(argv[1]);
			argc--;
			argv++;
			continue;
		}
		if (strcmp(argv[0], "duration") == 0) {
			if (argc < 2)
				goto usage;
			bell.type = BEL_AUDIBLE;
			bell.duration = atoi(argv[1]);
			argc--;
			argv++;
			continue;
		}
usage:		bell_usage();
		return (1);
	}
	if (ioctl(fd, PCCONIOCSBELL, (caddr_t)&bell) == -1)
		err(1, "set console bell parameters");
	return (0);
}

void
bell_help()
{
	printf("bell category:\n");
	printf("\t    none -- enable bell\n");
	printf("\t audible -- disable bell\n");
	printf("\t    freq -- frequency in hertz\n");
	printf("\tduration -- duration in milliseconds\n");
}

static void
bell_usage()
{
	fprintf(stderr, "usage: setcons bell none\n");
	fprintf(stderr,
	    "usage: setcons bell audible [freq hz] [duration ms]\n");
}
