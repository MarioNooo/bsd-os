/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ttydesc.c,v 1.4 1998/06/11 19:56:36 prb Exp
 */
#include <sys/types.h>

#include <err.h>
#include <libdialer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ttyent.h>
#include <unistd.h>

void
main(int argc, char **argv)
{
	cap_t *cp;
	char *newline = "\n";
	char *tty = NULL;
	char buf[8192];
	int c;
	int r = 0;
	ttydesc_t *td;

	while ((c = getopt(argc, argv, "rt:")) != EOF)
		switch (c) {
		case 'r':
			newline = "";
			break;
		case 't':
			tty = optarg;
			break;
		default:
			goto usage;
		}

	if (argc - optind > 2) {
usage:
		fprintf(stderr, "usage: ttydesc [-t tty] [field [element]]\n");
		exit(1);
	}

	if (tty == NULL && (tty = ttyname(STDIN_FILENO)) == NULL)
		err(1, NULL);

	if (strncmp(tty, "/dev/", 5) == 0)
		tty += 5;

	if ((td = getttydescbyname(tty)) == NULL)
		errx(1, "%s: no such entry in %s", tty, _PATH_TTYS);

	if (optind < argc) {
		if (optind + 1 < argc)
			c = strtol(argv[optind+1], NULL, 0);
		else
			c = -1; 

		if ((cp = cap_look(td->ty_cap, argv[optind])) && cp->value)
			if (c < 0)
				for (argv = cp->values; *argv != NULL; ++argv)
					printf("%s%s", *argv, newline);
			else {
				for (argv = cp->values; *argv != NULL && c > 0;
				    ++argv, --c)
					;
				if (*argv != NULL)
					printf("%s%s", *argv, newline);
			}
		else
			r = 1;
	} else
		printf("%s:%s", tty, cap_dump(td->ty_cap, buf, sizeof(buf)));
	exit(r);
}
