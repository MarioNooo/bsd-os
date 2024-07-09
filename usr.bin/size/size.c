/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI size.c,v 2.6 2002/05/16 17:11:56 donn Exp
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

#include "size.h"

static int (* const size[])(const char *, const void *, size_t) = {
	_ar_size,
#ifdef	OMAGIC
	_aout_size,
#endif
	_elf32_size,
	_elf64_size
};

static void usage(void);

int
main(int argc, char **argv)
{
	static char *default_argv[] = { "a.out", NULL };
	struct stat s;
	void *v;
	int first = 1;
	int c;
	int e = 0;
	int f;
	size_t i;
	int r;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		argv = default_argv;

	for (; *argv; ++argv) {

		if ((f = open(*argv, O_RDONLY)) == -1) {
			warn("%s", *argv);
			e = 1;
			continue;
		}

		if (first) {
			printf("text\tdata\tbss\tdec\thex\n");
			first = 0;
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

		for (i = 0; i < sizeof (size) / sizeof (size[0]); ++i)
			if ((r = (size[i])(*argv, v, (size_t)s.st_size)) != 0)
				break;

		if (r == 0) {
			errno = EFTYPE;
			warn("%s", *argv);
			e = 1;
		}
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

void
display(const char *name, u_long text, u_long data, u_long bss)
{
	u_long total = text + data + bss;

	printf("%lu\t%lu\t%lu\t%lu\t%lx\t%s\n", text, data, bss, total, total,
	    name);
}

static void
usage()
{

	fprintf(stderr, "usage: size [file ...]\n");
	exit(1);
}
