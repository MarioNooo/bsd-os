/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI rpasswdadm.c,v 1.2 1996/09/13 19:42:46 bostic Exp
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

int
main(int argc, char *argv[])
{
	int c, changed, ret;
	char *user, *pass;

	ret = changed = 0;
	while ((c = getopt(argc, argv, "a:r:")) != EOF)
		switch (c) {
		case 'a':
			if ((pass = strchr(optarg, ':')) == NULL ||
			    pass == optarg)
				goto usage;

			*pass++ = '\0';
			if (rpasswd_insert(optarg, pass)) {
				ret |= 1;
				warn(optarg);
			} else
				changed |= 1;
			break;
		case 'r':
			if (rpasswd_delete(optarg)) {
				ret |= 1;
				warn(optarg);
			} else
				changed |= 1;
			break;
		usage:
		default:
			fprintf(stderr, "usage: rpasswdadm "
			    "[-a user:encrypted-password] "
			    "[-r user]\n");
			rpasswd_close();
			if (changed)
				errx(1, "changes made");
			exit(1);
		}

	if (argc <= 1) {
		while (user = rpasswd_next(&pass))
			printf("%s:%s\n", user, pass);
	}
	rpasswd_close();
	if (ret && changed)
		errx(1, "changes made");
	exit(ret);
}
