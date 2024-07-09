/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI	line.c,v 2.2 1995/08/02 17:40:32 bostic Exp
 */

#include <sys/types.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, eof;

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc != 0) {
		usage();
		/* NOTREACHED */
	}

	for (eof = 1; (ch = getchar()) != EOF;)
		if (putchar(ch) == EOF || ch == '\n') {
			eof = 0;
			break;
		}
	if (eof)
		(void)putchar('\n');
		
	if (ferror(stdin) || fclose(stdin))
		err(1, "stdin");
	if (ferror(stdout) || fclose(stdout))
		err(1, "stdout");

	exit (eof);
}

void
usage()
{
	(void)fprintf(stderr, "usage: line\n");
	exit(1);
}
