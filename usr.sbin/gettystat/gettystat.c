/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gettystat.c,v 1.3 1996/12/03 18:20:27 prb Exp
 */

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdialer.h>

main(int ac, char **av)
{
	char buf[8192];
	int fd;
	int r;
	int reset = 0;

	buf[0] = 0;
	while ((r = getopt(ac, av, "ar")) != EOF)
		switch (r) {
		case 'a':
			strcpy(buf, "doall:");
			break;
		case 'r':
			reset = 1;
			break;
		default:
			fprintf(stderr, "Usage: gettystat [-r] [line [...]]\n");
			exit(1);
		}

	while (optind < ac) {
		strncat(buf, av[optind++], sizeof(buf));
		strncat(buf, ":", sizeof(buf));
	}

	if (reset) {
		if ((fd = dialer_reset(buf[0] ? buf : NULL)) < 0)
			err(1, NULL);
	} else {
		if ((fd = dialer_status(buf[0] ? buf : NULL)) < 0)
			err(1, 0);
	}
	if ((r = read(fd, buf, sizeof(buf))) > 0) {
		if (strncmp(buf, "error:", 6) == 0) {
			if (buf[r-1] == '\n')
				buf[r-1] = '\0';
			errx(1, "%.*s", r - 6, buf + 6);
		}
		write(1, buf, r);
		while ((r = read(fd, buf, sizeof(buf))) > 0)
			write(1, buf, r);
	}
	exit(0);
}
