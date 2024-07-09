/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwcmp.c,v 1.4 2002/12/20 17:01:52 prb Exp
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *yyin;
extern char *srcfile;
extern int error;
extern int debug;
extern int nocpplines;
extern int protocol;
extern int ipv6;
FILE *asmout;
extern void yyparse();

int
main(int ac, char **av)
{
	int c;
	char *dstfile, *p;
	char *cmd;
	int sflag = 0;
	int cflag = 0;
	int pflag = 0;
	int stupid = 0;
	int (*aclose)(FILE *);

	dstfile = NULL;
	while ((c = getopt(ac, av, "cdspPSo:v:")) != EOF) {
		switch (c) {
		case 'd':
			++debug;
			break;
		case 'P':
			++nocpplines;
			break;
		case 'o':
			dstfile = optarg;
			break;
		case 's':
			sflag = 1;
			break;
		case 'S':
			stupid = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'v':	
			protocol = strtol(optarg, &p, 0);
			if (p == optarg || *p)
				goto usage;
			if (protocol != 4 && protocol != 6)
				errx(1, "%s: invalid IP version", optarg);
			if (protocol == 6)
				ipv6 = 1;
			break;
		default: usage:
			fprintf(stderr, "Usage: ipfwcmp [-cpS] [-s] [-o file] [-v version] file\n");
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
	if (sflag == 0 && dstfile == NULL) {
		if (p)
			*p = '\0';

		if ((dstfile = malloc(strlen(srcfile) + 8)) == NULL)
			err(1, NULL);
		strcpy(dstfile, srcfile);
		strcat(dstfile, ".filter");
		if (p)
			*p = '.';
	}

	cmd = malloc(strlen(srcfile) + 5);
	sprintf(cmd, "cpp %s", srcfile);
	if ((yyin = popen(cmd, "r")) == NULL)
		err(1, "cpp");

	if (sflag) {
		aclose = fclose;
		if (dstfile == NULL || strcmp(dstfile, "-") == 0)
			asmout = stdout;
		else if ((asmout = fopen(dstfile, "w")) == NULL)
			err(1, dstfile);
	} else {
		cmd = malloc(strlen(dstfile) + strlen(srcfile) + 16 + 9);
		sprintf(cmd, "ipfwasm%s%s%s -f %s -o %s -",
		    stupid ? " -S" : "",
		    cflag ? " -c" : "",
		    pflag ? " -p" : "",
		    srcfile, dstfile);
		if ((asmout = popen(cmd, "w")) == NULL)
			err(1, "ipfwasm");
		aclose = pclose;
	}

	yyparse();
	if (error) {
		(*aclose)(asmout);
		exit(error);
	}

	if ((error = pclose(yyin)) != 0 || (error = (*aclose)(asmout)) != 0)
		exit(error);
	exit(0);
}
