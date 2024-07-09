/*
 * Copyright (c) 1995 Berkeley Software Design Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include <sys/ioctl.h>
#include <sparc/dev/fdio.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int c, fd;
	char *path;

	path = "/dev/rfd0c";
	while ((c = getopt(argc, argv, "f:t:")) != EOF) {
		switch (c) {

		case 'f':
		case 't':
			path = optarg;
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (optind < argc)
		usage();
	fd = open(path, O_RDONLY | O_NONBLOCK, 0);
	if (fd < 0)
		err(1, "open(%s)", path);
	if (ioctl(fd, FDIOCEJECT, 0))
		warn("ioctl(%s, FDIOCEJECT)", path);
	exit(0);
}

void
usage()
{

	(void)fprintf(stderr, "usage: fdeject [-f device]\n");
	exit(1);
}
