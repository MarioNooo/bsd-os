/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI	bsd2dos.c,v 2.4 2003/06/03 14:48:35 prb Exp
 */

#include <sys/types.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, last;
	char *sf, *df;

	sf = "stdin";
	df = "stdout";
	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 2:
		if (freopen(df = argv[1], "w", stdout) == NULL)
			err(1, "%s", argv[1]);
		/* FALLTHROUGH */
	case 1:
		if (freopen(sf = argv[0], "r", stdin) == NULL)
			err(1, "%s", argv[0]);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	for (last = 'a'; (ch = getchar()) != EOF; last = ch) {
		if (ch == '\n' && last != '\r' && putchar('\r') == EOF)
			break;
		if (putchar(ch) == EOF)
			break;
	}
	if (ferror(stdin) || fclose(stdin))
		err(1, "%s", sf);
	if (ferror(stdout) || fclose(stdout))
		err(1, "%s", df);
	exit(0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: bsd2dos [input [output]]\n");
	exit(1);
}
