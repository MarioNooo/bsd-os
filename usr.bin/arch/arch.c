/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/* BSDI arch.c,v 2.2 1995/07/28 20:04:38 bostic Exp */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	size_t len;
	int ch, mib[2];
	char buf[1024];

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	len = sizeof(buf);
	if (sysctl(mib, 2, &buf, &len, NULL, 0) == -1)
		err(1, "sysctl");

	if (argc == 1)
		exit(len - 1 != strlen(*argv) || memcmp(buf, *argv, len));

	(void)printf("%s\n", buf);
	exit(0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: arch [archname]\n");
	exit(1);
}
