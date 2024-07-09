/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI main.c,v 1.2 1995/07/18 22:58:35 cp Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "script.h"
#include "symbol.h"

int	openfile __P((const char *));
int	rewindfile();
int	yyparse __P((void));

extern char *optarg;
extern int optind;
extern int yyline;

extern FILE *yyin;

int pass = 1;

int
main(argc, argv)
	int argc;
	char **argv;
{
	register char *p;
	int pflag, ch;
	struct stat st;
	char *prefix = "";
	int code = 0;
	int tag = 0;
	int define = 0;
	int list = 0;

	pflag = 0;
	while ((ch = getopt(argc, argv, "lcdp:t")) != EOF) {
		switch (ch) {
		case 'p':
		    prefix = optarg;
		    break;
		case 'c':
		    code = 1;
		    break;
		case 't':
		    tag = 1;
		    break;
		case 'd':
		    define = 1;
		    break;
		case 'l':
		    list = 1;
		    break;
		case '?':
		default:
			goto usage;
		}
	}

	argc -= optind;
	argv += optind;
	if (argc != 1) {
usage:
		(void)fputs("usage: ncrasm [-p prefix] [-c] [-d] [-t] file\n",
		    stderr);
		exit(1);
	}
	if (openfile(argv[0])) {
		(void)fprintf(stderr, "cannot read %s: %s\n",
		    argv[0], strerror(errno));
		exit(2);
	}

	if (yyparse())
		stop();

	rewindfile();
	script_passtwo();
	pass = 2;

	if (yyparse())
		stop();

	if (code) {
		script_t *s;
		/* first put out the code */
		printf("u_int %scode[] = {\n",prefix);
		for (s = script_head; s!= NULL; s= s->s_next)
		{
			printf("\t0x%08x,\t", s->s_bits.bi_data[0]);
			printf("0x%08x,", s->s_bits.bi_data[1]);
			if (s->s_triple)
			    printf("\t0x%08x,", s->s_bits.bi_data[2]);
			else
			    printf("\t\t");
			printf("\t/* %04x, %03d */",s->s_lc, s->s_srcline);
			printf("\n");
		}
		printf("};\n");
		/* now put out patch locations */

		printf("#define NCR_PATCH_TABLEREL 1\n");
		printf("#define NCR_PATCH_IOREL 2\n");
		printf("/* offsets are in u_ints */\n");
		printf("u_int %spatch[] = {\n",prefix);
		for (s = script_head; s!= NULL; s= s->s_next)
		{
			if (s->s_w1_tablerelative)
			    printf("\t1,\t0x%08x,\n", s->s_lc/4 + 1);
			if (s->s_w1_iorelative)
			    printf("\t2,\t0x%08x,\n", s->s_lc/4 + 1);
			if (s->s_w2_tablerelative)
			    printf("\t1,\t0x%08x,\n", s->s_lc/4 + 2);
			if (s->s_w2_iorelative)
			    printf("\t2,\t0x%08x,\n", s->s_lc/4 + 2);
		}
		printf("\t0\n};\n");
	}
	if (tag || define) {
		symbol_t *s;
		for (s = symlist; s != NULL; s = s->s_next) {
			if(define == 0 && s->s_tag == 0)
				continue;
			if (tag == 0 && s->s_tag == 1)
				continue;
			printf("#define %s%s 0x%08x\n",
			    prefix, s->s_name, s->s_value);
		}
	}
	if (list) {
		char buffer[1000];
		script_t *s;
		int line = 1;

		rewindfile();
		for (s = script_head; s!= NULL; s= s->s_next) {
			while (line < s->s_srcline) {
				fgets(buffer, sizeof buffer, yyin);
				printf("\t\t\t\t%s", buffer);
				line++;
			}
			printf("%03x", s->s_lc, s->s_bits);
			printf(" %08x ", s->s_bits.bi_data[0]);
			printf("%08x ", s->s_bits.bi_data[1]);
			if (s->s_triple)
			    printf("%08x", s->s_bits.bi_data[2]);
			else
			    printf("        ");
			fgets(buffer, sizeof buffer, yyin);
			printf("\t%s", buffer);
			line++;
		}
	}

	exit(0);
}
