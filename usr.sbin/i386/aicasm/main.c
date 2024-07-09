/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI main.c,v 1.4 1999/12/09 19:09:47 cp Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sequencer.h"
#include "symbol.h"
#include <i386/isa/aicreg.h>

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

	fflush(stdout);

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
		(void)fputs("usage: aicasm [-p prefix] [-c] [-d] [-t] file\n",
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
	sequencer_passtwo();
	pass = 2;

	if (yyparse())
		stop();

	if (code) {
		sequencer_t *s;
		/* first put out the code */
		printf("u_int %scode[] = {\n",prefix);
		for (s = sequencer_head; s!= NULL; s= s->s_next)
		{
			printf("\t0x%08x,\t", s->s_bits);
			printf("\t/* %04x, %03d */",s->s_lc, s->s_srcline);
			printf("\n");
		}
		printf("};\n");
	}
	if (tag || define) {
		symbol_t *s;
		u_int	value;
		for (s = symlist; s != NULL; s = s->s_next) {
			if(define == 0 && s->s_define == 0)
				continue;
			if (tag == 0 && s->s_tag == 1)
				continue;
			if (s->s_tag == 1 && s->s_etag == 0)
				continue;
			value = s->s_value;
			if (s->s_section == AIC_S_SRAM)
				value += AIC_SRAM; 
			if (s->s_section == AIC_S_SCB)
				value += AIC_SCB; 
			printf("#define %s%s 0x%x\n",
			    prefix, s->s_name, value);
		}
	}
	if (list) {
		char buffer[1000];
		sequencer_t *s;
		int line = 1;

		rewindfile();
		for (s = sequencer_head; s!= NULL; s= s->s_next) {
			while (line < s->s_srcline) {
				fgets(buffer, sizeof buffer, yyin);
				printf("\t\t\t%s", buffer);
				line++;
			}
			printf("%03x ", s->s_lc); 
			if (s->s_format3.f3_format3) {
			    printf("%x . %03x %03x %02x",
				s->s_format3.f3_opcode,
				s->s_format3.f3_address,
				s->s_format3.f3_source,
				s->s_format3.f3_immediate);
			} else {
			    printf("%x %x %03x %03x %02x",
				s->s_format1.f1_opcode,
				s->s_format1.f1_return,
				s->s_format1.f1_destination,
				s->s_format1.f1_source,
				s->s_format1.f1_immediate);
			}
			fgets(buffer, sizeof buffer, yyin);
			printf("\t%s", buffer);
			line++;
		}
	}

	exit(0);
}
