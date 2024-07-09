/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwasm.c,v 1.5 2002/12/20 16:36:11 prb Exp
 */
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *yyin;
char *srcfile;
int error = 0;
int stupid, docache, doprofile, xbug;
extern void yyparse();
extern void verify_program();
extern void write_program(int);

int
main(int ac, char **av)
{
	int c;
	char *cmd, *dstfile, *name, *p;
	int (*close)(FILE *);

	dstfile = NULL;
	name = NULL;

	while ((c = getopt(ac, av, "cf:o:pSx")) != EOF) {
		switch (c) {
		case 'c':
			docache++;
			break;

		case 'f':
			name = optarg;
			break;

		case 'o':
			dstfile = optarg;
			break;

		case 'p':
			doprofile++;
			break;

		case 'S':
			stupid++;
			break;

		case 'x':		/* for debugging messages */
			++xbug;
			break;

		default: usage:
			fprintf(stderr, "Usage: ipfwasm [-S] [-o file] file\n");
			exit(1);
		}
	}

	if (optind + 1 != ac)
		goto usage;

	srcfile = av[optind];

	if (dstfile && strcmp(dstfile, "-") && strcmp(dstfile, srcfile) == 0) {
		fprintf(stderr, "would overwrite %s\n", srcfile);
		exit(1);
	}

	if ((p = strrchr(srcfile, '.')) && strcmp(p, ".filter") == 0) {
		fprintf(stderr, "would overwrite %s\n", srcfile);
		exit(1);
	}
	if (p && strcmp(p, ".ipfw") != 0)
		p = NULL;
	if (dstfile == NULL) {
		if (strcmp(srcfile, "-") == 0)
			dstfile = "ipfw.filter";
		else {
			if (p)
				*p = '\0';

			if ((dstfile = malloc(strlen(srcfile) + 8)) == NULL)
				err(1, NULL);
			strcpy(dstfile, srcfile);
			strcat(dstfile, ".filter");
			if (p)
				*p = '.';
		}
	}

	if (strcmp(srcfile, "-") == 0) {
		if (name)
			srcfile = name;
		yyin = stdin;
		close = fclose;
	} else {
		cmd = malloc(strlen(srcfile) + 5);
		sprintf(cmd, "cpp %s", srcfile);
		if ((yyin = popen(cmd, "r")) == NULL)
			err(1, "cpp");
		close = pclose;
	}

	yyparse();
	if (error)
		exit(error);
	verify_program();
	if (error)
		exit(error);
	if ((error = (*close)(yyin)) != 0)
		exit(error);

	if (strcmp(dstfile, "-") != 0) {
		unlink(dstfile);
		if ((c = creat(dstfile, 0666)) < 0)
			err(1, dstfile);
	} else
		c = 1;
	write_program(c);
	exit(0);
}
