/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI shlist.c,v 2.9 2001/10/03 17:30:00 polk Exp
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <a.out.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shlist.h"

int aflag;

static int (* const shlist[])(const char *, void *, size_t) = {
#ifdef	OMAGIC
	_aout_shlist,
#endif
	_elf32_shlist,
	_elf64_shlist
};

static void usage(void);

int
main(int argc, char **argv)
{
	struct stat s;
	void *v;
	int c;
	int e = 0;
	int f;
	size_t i;
	int r;

	while ((c = getopt(argc, argv, "a")) != -1)
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (; *argv; ++argv) {

		if ((f = open(*argv, O_RDONLY)) == -1) {
			warn("%s", *argv);
			e = 1;
			continue;
		}

		if (fstat(f, &s) == -1 ||
		    ((s.st_mode & S_IFMT) != S_IFREG && (errno = ENODEV)) ||
		    (v = mmap(NULL, (size_t)s.st_size, PROT_READ, MAP_SHARED,
			f, 0)) == MAP_FAILED) {
			warn("%s", *argv);
			close(f);
			e = 1;
			continue;
		}

		for (i = 0; i < sizeof (shlist) / sizeof (shlist[0]); ++i)
			if ((r = (shlist[i])(*argv, v, (size_t)s.st_size)) != 0)
				break;

		if (r == -1) {
			if (errno == EFTYPE)
				warnx("%s: file corrupted", *argv);
			else
				warn("%s", *argv);
			e = 1;
		}

		munmap(v, (size_t)s.st_size);
		close(f);
	}

	return (e);
}

static void
usage()
{

	fprintf(stderr, "usage: shlist [-a] file ...\n");
	exit(1);
}
