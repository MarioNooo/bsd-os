/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI yacc.y,v 1.11 2002/12/20 16:36:11 prb Exp
 */
%{
#define	IPFW

#include <sys/types.h>
#include <sys/time.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/ipfw.h>

#include <assert.h>
#include <err.h>
#include <ipfw.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "asm.h"

void insert_instruction(struct statement *);
void add_label(int);
int linecnt = 1;
extern char *srcfile;
extern int stupid, docache, doprofile, xbug;
void *new(size_t);
void *renew(void *, size_t);
void _xbugmsg(int level, struct statement *j, char *msg, ...);
int xline__;
char *xfile__;
#define	xbugmsg xline__ = __LINE__; xfile__ = __FILE__; _xbugmsg
%}

%token	<k>	NUMBER
%token	<q>	QNUMBER
%token	<iface>	IFACE FILTER
%token		ADD AND DIV JEQ JGE JGT JMP JSET JNET LD LDB LDH LDX LEN LSH
%token		MOD MUL NEG OR RET RSH ST STX SUB TAX TXA EOP A128 ZMEM TIME
%token		ERR JLE JLT JNE SHL SHR IFN IPSRCRT IFFLAGS CALL MSH MSHX

%type	<k>	label qmem mem msh mshx quad number num rom
%type	<im>	imm fimm
%type	<sk>	abs ind
%type	<cmd>	instruction modifier
%left		'*' '/'
%left		'+' '-'
%left		'&'
%left		'|' '^'

%union {
	int k;				/* just a number */
#ifdef	BPF_128
	q128_t q;
#endif
	struct {
		int k;
		int w;
		int code;
	} sk;
	char	*iface;
	struct {
		int k;
		char *iface;
	} im;
	struct statement cmd;
}

%%

start	: program
	{
		YYACCEPT;
	}
	;

program : statement '\n'
	{
		++linecnt;
	}
	| program statement '\n'
	{
		++linecnt;
	}
	;

statement :	onestatement
	|	statement ';' onestatement
	;

onestatement :	/* empty */
	| instruction
	{
		insert_instruction(&$1);
	}
	| 'L' NUMBER ':' instruction
	{
		add_label($2);
		insert_instruction(&$4);
	}
	| 'L' NUMBER ':'
	{
		add_label($2);
	}
	;

instruction	: CALL fimm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_CCC;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| ADD imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_ADD|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| ADD 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_ADD|BPF_X;
	}
	| AND imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_AND|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| AND 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_AND|BPF_X;
	}
	| DIV imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_DIV|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| DIV 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_DIV|BPF_X;
	}
	| MOD imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_MOD|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| MOD 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_MOD|BPF_X;
	}
	| LSH imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_LSH|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| LSH 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_LSH|BPF_X;
	}
	| MUL imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_MUL|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| MUL 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_MUL|BPF_X;
	}
	| NEG
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_NEG;
	}
	| OR imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_OR|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| OR 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_OR|BPF_X;
	}
	| RSH imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_RSH|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| RSH 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_RSH|BPF_X;
	}
	| SUB imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_SUB|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| SUB 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ALU|BPF_SUB|BPF_X;
	}
	| JMP label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JA;
		if ($2 >= 0) {
			$$.needk = 1;
			$$.k = $2;
		}
	}
	| JEQ imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JEQ 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_X;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JGE imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JGE 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_X;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JGT imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JGT 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_X;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JNE imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JNE 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_X;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JLT imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JLT 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_X;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JLE imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JLE 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_X;
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
	}
	| JNET '#' number '/' NUMBER label label
	{
#ifdef	BPF_JNET
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JNET;
		$$.k = $3;
		$$.mask = $5;
		if ($5 < 1 || $5 > 31)
			yyerror("netmask must be between 1 and 31");
		if ($6 >= 0) {
			$$.jt = $6;
			$$.needt = 1;
		}
		if ($7 >= 0) {
			$$.jf = $7;
			$$.needf = 1;
		}
#else
		yyerror("jnet not supported on this system");
#endif
	}

	| JNET QNUMBER '/' number label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JNET|BPF_TRIPLE;
		$$.q = $2;
		$$.k = $4;
		if ($4 < 1 || $4 > 127)
			yyerror("IPv6 netmask must be between 1 and 127");
		if ($5 >= 0) {
			$$.jt = $5;
			$$.needt = 1;
		}
		if ($6 >= 0) {
			$$.jf = $6;
			$$.needf = 1;
		}
#else
		yyerror("jnet not supported on this system");
#endif
	}

	| JGT QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| JGE QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| JEQ QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| JLE QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGT|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
#else
		YERROR;
#endif
	}
	| JLT QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JGE|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
#else
		YERROR;
#endif
	}
	| JNE QNUMBER label label
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JEQ|BPF_TRIPLE;
		$$.q = $2;
		if ($3 >= 0) {
			$$.jf = $3;
			$$.needf = 1;
		}
		if ($4 >= 0) {
			$$.jt = $4;
			$$.needt = 1;
		}
#else
		YERROR;
#endif
	}
	| JSET imm label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JSET|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| JSET 'X' label label
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_JMP|BPF_JSET|BPF_X;
		if ($3 >= 0) {
			$$.jt = $3;
			$$.needt = 1;
		}
		if ($4 >= 0) {
			$$.jf = $4;
			$$.needf = 1;
		}
	}
	| LDX imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_IMM;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| LDX mem
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MEM;
		$$.k = $2;
	}
	| LDX rom
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_ROM;
		$$.k = $2;
	}
	| LDX msh
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MSH|BPF_B;
		$$.k = $2;
	}
	| LDX mshx
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MSHX|BPF_B;
		$$.k = $2;
	}
	| LDX MSH '[' number ']'
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MSH|BPF_128;
		$$.k = $4;
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| LDX MSH '[' 'X' '+' number ']'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MSHX|BPF_128;
		$$.k = $6;
	}
	| LDX MSH '[' number ']' '#' number
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_MSH|BPF_128;
		$$.k = $4;
		$$.jt = $7 & 0xff;
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| LDX '#' LEN
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LDX|BPF_W|BPF_LEN;
	}
	| LDX TIME
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_DKX;
		$$.k = BFP_DK_TIME;
	}
	| LDB abs modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_B|BPF_ABS|$2.code;
		if ($2.w && $2.w != 1)
			yyerror("width of data to LDB must be 1");
		$$.k = $2.k;
		if ($3.code)
			$$.next = &$3;
	}
	| LDB ind modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_B|BPF_IND|$2.code;
		if ($2.w && $2.w != 1)
			yyerror("width of data to LDB must be 1");
		$$.k = $2.k;
		if ($3.code)
			$$.next = &$3;
	}
	| LDH abs modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_H|BPF_ABS|$2.code;
		if ($2.w && $2.w != 2)
			yyerror("width of data to LDH must be 2");
		$$.k = $2.k;
		if ($3.code)
			$$.next = &$3;
	}
	| LDH ind modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_H|BPF_IND|$2.code;
		if ($2.w && $2.w != 2)
			yyerror("width of data to LDH must be 2");
		$$.k = $2.k;
		if ($3.code)
			$$.next = &$3;
	}
	| LD imm modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_IMM;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
		if ($3.code)
			$$.next = &$3;
	}
	| LD qmem
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_MEM|BPF_128;
		$$.k = $2;
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| LD mem modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_MEM;
		$$.k = $2;
		if ($3.code)
			$$.next = &$3;
	}
	| LD rom modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_ROM;
		$$.k = $2;
		if ($3.code)
			$$.next = &$3;
	}
	| LD QNUMBER
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_IMM|BPF_TRIPLE;
		$$.q = $2;
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| LD abs modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_ABS|$2.code;
		$$.k = $2.k;
		switch ($2.w) {
		case 0: case 4:
			$$.code |= BPF_W;
			break;
		case 1:
			$$.code |= BPF_B;
			break;
		case 2:
			$$.code |= BPF_H;
			break;
#ifdef	BFP_128
		case 16:
			$$.code |= BPF_128;
			break;
#endif
		default:
			yyerror("width must be 1, 2, 4 or 16");
		}
		if ($3.code)
			$$.next = &$3;
	}
	| LD ind modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_IND|$2.code;
		$$.k = $2.k;
		switch ($2.w) {
		case 0: case 4:
			$$.code |= BPF_W;
			break;
		case 1:
			$$.code |= BPF_B;
			break;
		case 2:
			$$.code |= BPF_H;
			break;
#ifdef	BPF_128
		case 16:
			$$.code |= BPF_128;
			break;
#endif
		default:
			yyerror("width must be 1, 2, 4, or 16");
		}
		if ($3.code)
			$$.next = &$3;
	}
	| LD '#' LEN modifier
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_LD|BPF_W|BPF_LEN;
		if ($4.code)
			$$.next = &$4;
	}
	| LD TIME
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_DKA;
		$$.k = BFP_DK_TIME;
	}
	| TAX
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_TAX;
	}
	| TXA
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_TXA;
	}
	| RET 'A'
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_RET|BPF_A;
	}
	| RET imm
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_RET|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| ST abs
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ST|BPF_ABS|$2.code;
		$$.k = $2.k;
		switch ($2.w) {
		case 0: case 4:
			$$.code |= BPF_W;
			break;
		case 1:
			$$.code |= BPF_B;
			break;
		case 2:
			$$.code |= BPF_H;
			break;
#ifdef	BPF_128
		case 16:
			$$.code |= BPF_128;
			break;
#endif
		default:
			yyerror("width must be 1, 2, 4, or 16");
		}
	}
	| ST ind
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ST|BPF_IND|$2.code;
		$$.k = $2.k;
		switch ($2.w) {
		case 0: case 4:
			$$.code |= BPF_W;
			break;
		case 1:
			$$.code |= BPF_B;
			break;
		case 2:
			$$.code |= BPF_H;
			break;
#ifdef	BPF_128
		case 16:
			$$.code |= BPF_128;
			break;
#endif
		default:
			yyerror("width must be 1, 2, 4, or 16");
		}
	}
	| ST qmem
	{
#ifdef	BPF_128
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ST|BPF_MEM|BPF_128;
		$$.k = $2;
#else
		yyerror("128 bit addresses not supported on this system");
#endif
	}
	| ST mem
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ST|BPF_MEM;
		$$.k = $2;
	}
	| ST rom
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_ST|BPF_ROM;
		$$.k = $2;
	}
	| STX abs
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_STX|BPF_ABS|$2.code;
		$$.k = $2.k;
		switch ($2.w) {
		case 0: case 4:
			$$.code |= BPF_W;
			break;
		case 1:
			$$.code |= BPF_B;
			break;
		case 2:
			$$.code |= BPF_H;
			break;
		default:
			yyerror("width must be 1, 2 or 4");
		}
	}
	| STX mem
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_STX|BPF_MEM;
		$$.k = $2;
	}
	| STX rom
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_STX|BPF_ROM;
		$$.k = $2;
	}
	| ZMEM mem
	{
		memset(&$$, 0, sizeof($$));
		$$.src = srcfile; $$.line = linecnt;
		$$.code = BPF_MISC|BPF_ZMEM;
		$$.k = $2;
	}
	;

abs	: '[' number ']'
	{
		$$.k = $2;
		$$.w = 0;
		$$.code = 0;
	}
	| '[' number ':' number ']'
	{
		$$.k = $2;
		$$.w = $4;
		$$.code = 0;
	}
	;

ind	: '[' 'X' '+' number ']'
	{
		$$.k = $4;
		$$.w = 0;
		$$.code = 0;
	}
	| '[' 'X' '+' number ':' number ']'
	{
		$$.k = $4;
		$$.w = $6;
		$$.code = 0;
	}
	;

imm	: '#' number
	{
		$$.iface = NULL;
		$$.k = $2;
	}
	| '#' IFACE
	{
		$$.iface = $2;
	}
	;

fimm	: '#' number
	{
		$$.iface = NULL;
		$$.k = $2;
	}
	| '#' FILTER
	{
		$$.iface = $2;
	}
	;

quad	: /* empty */
	{
		$$ = 0;
	}
	| NUMBER
	{
		if ($1 > 255)
			yyerror("dotted quad element > 255");
		if ($1 < 0)
			yyerror("negative dotted quad element");
		$$ = $1;
	}
	;

rom	: 'R' '[' number ']'
	{
		$$ = $3;
	}
	;

mem	: 'M' '[' number ']'
	{
		if ($3 >= BPF_MEMWORDS)
			yyerror("memory address to large");
		$$ = $3;
	}
	;

qmem	: 'M' '[' number ':' number ']'
	{
		if ($3 >= BPF_MEMWORDS)
			yyerror("memory address to large");
		if ($5 != 16)
			yyerror("16 only availabe size modifier for memory");
		$$ = $3;
	}
	;

msh	: number '*' '(' abs '&' number ')'
	{
		if ($1 != 4 || $6 != 0xf || $4.w)
			yyerror("msh syntax error");
		$$ = $4.k;
	}
	;

mshx	: number '*' '(' ind '&' number ')'
	{
		if ($1 != 4 || $6 != 0xf || $4.w)
			yyerror("msh syntax error");
		$$ = $4.k;
	}
	;

label	: 'L' NUMBER
	{
		$$ = $2;
	}
	| '-'
	{
		$$ = -1;
	}
	| ',' 'L' NUMBER
	{
		$$ = $3;
	}
	| ',' '-'
	{
		$$ = -1;
	}
	;

number	:	NUMBER
	{
		$$ = $1;
	}
	|	'-' NUMBER
	{
		$$ = - $2;
	}
	|	'~' NUMBER
	{
		$$ = - $2;
	}
	|	'(' num ')'
	{
		$$ = $2;
	}
	|	quad '.' quad
	{
		$$ = ($1 << 24) | $3;
	}
	|	quad '.' quad '.' quad
	{
		$$ = ($1 << 24) | ($3 << 16) | $5;
	}
	|	quad '.' quad '.' quad '.' quad
	{
		$$ = ($1 << 24) | ($3 << 16) | ($5 << 8) | $7;
	}
	;

num	:	NUMBER
	{
		$$ = $1;
	}
	|	'-' NUMBER
	{
		$$ = - $2;
	}
	|	'~' NUMBER
	{
		$$ = - $2;
	}
	|	num '|' num
	{
		$$ = $1 | $3;
	}
	|	num '&' num
	{
		$$ = $1 & $3;
	}
	|	num '+' num
	{
		$$ = $1 + $3;
	}
	|	num '-' num
	{
		$$ = $1 + $3;
	}
	|	num '^' num
	{
		$$ = $1 ^ $3;
	}
	|	num '*' num
	{
		$$ = $1 * $3;
	}
	|	num '/' num
	{
		$$ = $1 / $3;
	}
	;

modifier : /* empty */
	{
		memset(&$$, 0, sizeof($$));
	}
	| '&' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_AND|BPF_X;
	}
	| '&' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_AND|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| '+' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_ADD|BPF_X;
	}
	| '+' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_ADD|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| '*' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_MUL|BPF_X;
	}
	| '*' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_MUL|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| '/' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_DIV|BPF_X;
	}
	| '/' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_DIV|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| '-' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_SUB|BPF_X;
	}
	| '-' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_SUB|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| '|' 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_OR|BPF_X;
	}
	| '|' imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_OR|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| SHL 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_LSH|BPF_X;
	}
	| SHL imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_LSH|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	| SHR 'X'
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_RSH|BPF_X;
	}
	| SHR imm
	{
		memset(&$$, 0, sizeof($$));
		$$.code = BPF_ALU|BPF_RSH|BPF_K;
		if ($2.iface) {
			if (($$.iface = strdup($2.iface)) == NULL)
				err(1, NULL);
		} else
			$$.k = $2.k;
	}
	;

%%


static struct statement Prev;
static struct statement *program = 0;
static struct statement *lastins = 0;
static struct statement **labels;
static int nlabels = 0;
static int maxlabel = 0;
static int pc = 0;

static struct {
	int	code;	/* -1 -> unknown, 0 -> unused */
	u_long	where;
} scratch[BPF_MEMWORDS];

void
add_label(int label)
{
	struct statement *j;

	if (maxlabel == 0) {
		maxlabel = 64;
		labels = new(sizeof(*labels) * maxlabel);
	} else if (maxlabel <= nlabels) {
		maxlabel += 64;
		labels = renew(labels, sizeof(*labels) * maxlabel);
	}
	labels[nlabels] = NULL;

	if (label)
		for (j = program; j; j = j->next) {
			if (j->needk && j->k == label) {
				j->needk = 0;
				j->relocatek = 1;
				j->k = nlabels;
			}
			if (j->needt && j->jt == label) {
				j->needt = 0;
				j->relocatet = 1;
				j->jt = nlabels;
			}
			if (j->needf && j->jf == label) {
				j->needf = 0;
				j->relocatef = 1;
				j->jf = nlabels;
			}
		}
	++nlabels;
}

void
add_caller(struct statement *dst, struct statement *src)
{
	int i;

	for (i = 0; i < dst->ncallers; ++i)
		if (dst->callers[i] == src)
			return;

	if (dst->maxcallers == 0) {
		dst->maxcallers = 8;
		dst->callers = new(dst->maxcallers *  sizeof(*dst->callers));
	} else if (dst->maxcallers <= dst->ncallers) {
		dst->maxcallers += 8;
		dst->callers = renew(dst->callers,
		    dst->maxcallers *  sizeof(*dst->callers));
	}
	dst->callers[dst->ncallers++] = src;
}

void
insert_instruction(struct statement *s)
{
	struct statement *cmd;
	int j;

	cmd = new(sizeof(*cmd));
	*cmd = *s;
	if (lastins)
		lastins->next = cmd;
	else 
		program = cmd;
	lastins = cmd;
	if (cmd->next)
		insert_instruction(cmd->next);

	/*
	 * All labels without a statement must be for us
	 */
	for (j = nlabels - 1; j >= 0; --j)
		if (labels[j] == NULL)
			labels[j] = cmd;
		else
			break;
}

/*
 * Return 0 if we always have the same value in A that is about to be loaded.
 * Return 1 if our value could be different
 */
int
trace_ld(struct statement *j, int code, int k)
{
	struct statement *n;
	int i;

#ifdef	BPF_128
	int q = (code & BPF_TRIPLE) || (BPF_SIZE(code) == BPF_128);
#else
	int q = 0;
#endif
	if (j->visited != -1)
		return (j->visited);

	switch (BPF_CLASS(j->code)) {
	case BPF_ALU:
		/*
		 * Arithmetic instructions always modify A, but not A128
		 */
		if (q)
			goto instruction;
		return (j->visited = 1);

	case BPF_LD:
		/*
		 * If we are loading into A then we must identically
		 * match the previous load.
		 * We assume that index load checks have been taken care of.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL &&
		    j->k == k)
			return (j->visited = 0);
#ifdef	BPF_128
		/*
		 * Loads of A do not interfere with A128
		 */
		if ((q && BPF_SIZE(j->code) != BPF_128) ||
		    (!q && BPF_SIZE(j->code) == BPF_128))
			goto instruction;
#endif
		return (j->visited = 1);

	case BPF_LDX:
#ifdef	BPF_128
		/*
		 * Loading IPv6 MSH hits A but not A128
		 */
		if (q || (j->code != (BPF_LDX|BPF_MSH|BPF_128) &&
			   j->code != (BPF_LDX|BPF_MSHX|BPF_128)))

			goto instruction;
#endif
		return (j->visited = 1);

	case BPF_MISC:
		switch (BPF_MISCOP(j->code)) {
		case BPF_CCC:
		case BPF_TXA:
			if (!q)
				return (j->visited = 1);
		}
		goto instruction;

	case BPF_ST:
		/*
		 * If we are storing into the location we are loading from
		 * then we don't need the load.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			    return (j->visited = 0);
		/*
		 * If we are loading from memory and someone is storing
		 * into memory then just assume they may have stored
		 * a value into where we are loading from.
		 */
		if (BPF_MODE(code) == BPF_ABS &&
		    (BPF_MODE(j->code) == BPF_IND ||
		     BPF_MODE(j->code) == BPF_ABS))
			    return (j->visited = 1);
		goto instruction;

	case BPF_STX:
		/*
		 * If we are storing X into the location we are loading from
		 * then we need to load it.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			    return (j->visited = 1);
		/*
		 * If we are loading from memory and someone is storing
		 * into memory then just assume they may have stored
		 * a value into where we are loading from.
		 */
		if (BPF_MODE(code) == BPF_ABS && BPF_MODE(j->code) == BPF_ABS)
			    return (j->visited = 1);
		goto instruction;

	default: instruction:
		if (j->prev == NULL)
			return (j->visited = 1);
		for (i = 0; i < j->ncallers; ++i) {
			if ((n = j->callers[i]) == &Prev)
				n = j->prev;
			if (trace_ld(n, code, k))
				return (j->visited = 1);
		}
		break;
	}
	return (j->visited = 0);
}

/*
 * Return 0 if there is a history in which we don't hit base
 * Return 1 if all histories include base
 */
int
checkhistory(struct statement *j, struct statement *base)
{
	struct statement *n;
	int i;

	if (j == base)
		return (1);
	if (j == NULL)
		return (0);

	if (j->ncallers == 0)
		return (checkhistory(j->prev, base));

	for (i = 0; i < j->ncallers; ++i) {
		if ((n = j->callers[i]) == &Prev)
			n = j->prev;
		if (checkhistory(n, base) == 0)
			return (0);
	}
	return (1);
}

/*
 * Return 0 if we always have the same value in A that is about to be loaded.
 * Return 1 if our value could be different
 */
int
trace_ild(struct statement *j, int code, int k)
{
	struct statement *n;
	int i;

	if (j->visited != -1)
		return (j->visited);

	switch (BPF_CLASS(j->code)) {
	case BPF_ALU:
		/*
		 * Arithmetic instructions always modify A
		 */
		return (j->visited = 1);

	case BPF_LD:
		/*
		 * If we are loading into A then we must identically
		 * match the previous load.
		 * We assume that index load checks have been taken care of.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			return (j->visited = 0);
		return (j->visited = 1);

	case BPF_LDX:
		/*
		 * Any loads to X are considered unknown
		 */
		return (j->visited = 1);

	case BPF_MISC:
		/*
		 * These always modify either A or X
		 */
		return (j->visited = 1);

	case BPF_ST:
		/*
		 * If we are storing into the location we are loading from
		 * then we don't need the load.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			    return (j->visited = 0);
		/*
		 * Since we are loading from memory, any store into memory
		 * requires us to do the load.
		 */
		if ((BPF_MODE(j->code) == BPF_IND ||
		     BPF_MODE(j->code) == BPF_ABS))
			    return (j->visited = 1);
		goto instruction;

	case BPF_STX:
		/*
		 * Since we are loading from memory, any store into memory
		 * requires us to do the load.
		 */
		if (BPF_MODE(j->code) == BPF_ABS)
			    return (j->visited = 1);
		goto instruction;

	default: instruction:
		if (j->prev == NULL)
			return (j->visited = 1);
		for (i = 0; i < j->ncallers; ++i) {
			if ((n = j->callers[i]) == &Prev)
				n = j->prev;
			if (trace_ild(n, code, k))
				return (j->visited = 1);
		}
		break;
	}
	return (j->visited = 0);
}

/*
 * Return 0 if we have the same value in X that is about to be loaded.
 * Return 1 if our value could be different
 */
int
trace_ldx(struct statement *j, int code, int k)
{
	struct statement *n;
	int i;

	if (j->visited != -1)
		return (j->visited);

	switch (BPF_CLASS(j->code)) {
	case BPF_LDX:
		/*
		 * If we are loading into X then we must identically
		 * match the previous load.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			return (j->visited = 0);
		return (j->visited = 1);

	case BPF_MISC:
		switch (BPF_MISCOP(j->code)) {
		case BPF_CCC:
		case BPF_TAX:
			return (j->visited = 1);
		}
		goto instruction;

	case BPF_ST:
		/*
		 * If we are storing A into the location we are loading from
		 * then we need the load.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			    return (j->visited = 1);
		goto instruction;

	case BPF_STX:
		/*
		 * If we are storing into the location we are loading from
		 * then we don't need the load.
		 */
		if (BPF_SIZE(j->code) == BPF_SIZE(code) &&
		    BPF_MODE(j->code) == BPF_MODE(code) &&
		    j->iface == NULL && j->k == k)
			    return (j->visited = 0);
		goto instruction;

	default: instruction:
		if (j->prev == NULL)
			return (j->visited = 1);
		for (i = 0; i < j->ncallers; ++i) {
			if ((n = j->callers[i]) == &Prev)
				n = j->prev;
			if (trace_ldx(n, code, k))
				return (j->visited = 1);
		}
		break;
	}
	return (j->visited = 0);
}

/*
 * Return 0 if we don't use A before alterning it
 * Return 1 if we do use A
 */
int
follow_a(struct statement *j, int m)
{
	int v;

	if (j == NULL) {
		xbugmsg(2, j, "follow_a -> 0");
		return (0);
	}

	if (j->visited != -1) {
		xbugmsg(2, j, "follow_a -> %d", j->visited);
		return (j->visited);
	}

	v = 0;
	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
#ifdef	BPF_128
		if ((j->code & BPF_TRIPLE) || BPF_SIZE(j->code) == BPF_128)
			v = follow_a(j->next, m);
#endif
		break;
	case BPF_LDX:
#ifdef	BPF_128
		if (j->code != (BPF_LDX|BPF_MSH|BPF_128))
#endif
			v = follow_a(j->next, m);
		break;
	case BPF_ST:
		v = 1;
		break;

	case BPF_ALU:
		v = 1;
		break;

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA)
			v = follow_a(labels[j->k], m);
#ifdef	BPF_128
		if ((j->code & BPF_TRIPLE))
			v = follow_a(labels[j->jt], m) | follow_a(labels[j->jf],m);
		else
#endif
			v = 1;
		break;

	case BPF_RET:
		v  = BPF_RVAL(j->code) == BPF_A ? 1 : 0;
		break;

	case BPF_MISC:
		switch (BPF_MISCOP(j->code)) {
		case BPF_TXA:
			v = 0;
			break;

		case BPF_CCC:
		case BPF_TAX:
			v = 1;
			break;
		default:
			v = follow_a(j->next, m);
			break;
		}
		break;

	default:
		v = follow_a(j->next, m);
		break;
	}
	j->visited = v;
	xbugmsg(2, j, "follow_a -> %d", j->visited);
	return (v);
}

#ifdef	BPF_128
/*
 * Return 0 if we don't use A128 before alterning it
 * Return 1 if we do use A128
 */
int
follow_q(struct statement *j, int m)
{
	int v;

	if (j == NULL)
		return (0);

	if (j->visited != -1)
		return (j->visited);

	v = 0;
	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
		if (j->code & (BPF_128|BPF_TRIPLE))
			break;
		v = follow_q(j->next, m);
		break;

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA) {
			v = follow_q(labels[j->k], m);
			break;
		}
		if ((j->code & BPF_TRIPLE))
			v = 1;
		else
			v = follow_q(labels[j->jt], m) | follow_q(labels[j->jf],m);
		break;

	case BPF_RET:
		break;

	default:
		v = follow_q(j->next, m);
	}
	j->visited = v;
	return (v);
}
#endif

/*
 * Return 0 if we don't use X before alterning it
 * Return 1 if we do use X
 */
int
follow_x(struct statement *j, int m)
{
	int v;

	if (j == NULL)
		return (0);

	if (j->visited != -1)
		return (j->visited);

	v = 0;
	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
	case BPF_ST:
		if (BPF_MODE(j->code) == BPF_IND)
			v = 1;
		else
			v = follow_x(j->next, m);
		break;

	case BPF_LDX:
		/*
		 * The MSHX instruction both uses and loads X
		 */
		if (BPF_MODE(j->code) == BPF_MSHX) {
			v = 1;
		}
		break;

	case BPF_STX:
		v = 1;
		break;

	case BPF_ALU:
		if (BPF_SRC(j->code) == BPF_X)
			v = 1;
		else
			v = follow_x(j->next, m);
		break;

	case BPF_JMP:
		if (BPF_OP(j->code) != BPF_JA && BPF_SRC(j->code) == BPF_X)
			v = 1;
		else if (BPF_OP(j->code) == BPF_JA)
			v = follow_x(labels[j->k], m);
		else
			v = follow_x(labels[j->jt], m) | follow_x(labels[j->jf],m);
		break;

	case BPF_RET:
		v = BPF_RVAL(j->code) == BPF_X ? 1 : 0;
		break;

	case BPF_MISC:
		switch (BPF_MISCOP(j->code)) {
		case BPF_TXA:
			v = 1;
			break;

		case BPF_TAX:
			break;

		case BPF_CCC:
		default:
			v = follow_x(j->next, m);
			break;
		}
		break;

	default:
		v = follow_x(j->next, m);
		break;
	}

	j->visited = v;
	return (v);
}

int
follow_ax(struct statement *j, int m)
{
	return(follow_a(j, m) || follow_x(j, m));
}

/*
 * Return 0 if we don't use M[m] before alterning it
 * Return 1 if we do use M[m]
 */
int
follow_m(struct statement *j, int m)
{
	int v;
	if (j == NULL)
		return (0);

	if (j->visited != -1)
		return (j->visited);

	v = 0;

	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
	case BPF_LDX:
		if (BPF_MODE(j->code) == BPF_MEM &&
		    j->iface == NULL && j->k == m)
			v = 1;
		else
			v = follow_m(j->next, m);
		break;
		
	case BPF_ST:
	case BPF_STX:
		if (BPF_MODE(j->code) == BPF_MEM &&
		    j->iface == NULL && j->k == m)
			v = 0;
		else
			v = follow_m(j->next, m);
		break;

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA)
			v = follow_m(labels[j->k], m);
		else
			v = (follow_m(labels[j->jt], m) | follow_m(labels[j->jf],m));
		break;

	default:
		v = follow_m(j->next, m);
		break;
	}
	j->visited = v;
	return (v);
}

/*
 * Return 0 if we have various end results
 * Return 1 if we always return the same constant (and no stores into
 * ROM or the packet)
 */
int
follow(struct statement *j, u_long *kp)
{
	u_long t, f;

	if (j == NULL) {
		*kp = 0;
		return (0);
	}
	if (j->visited != -1) {
		if (j->visited == 0)
			*kp = j->value;
		return (j->visited);
	}

	switch (BPF_CLASS(j->code)) {
	case BPF_ST:
	case BPF_STX:
		switch (BPF_MODE(j->code)) {
		case BPF_IND:
		case BPF_ABS:
		case BPF_ROM:
			j->visited = 1;
			break;
		default:
			j->visited = follow(j->next, kp);
			break;
		}
		break;

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA)
			j->visited =  follow(labels[j->k], kp);
		else if (follow(labels[j->jt], &t) || follow(labels[j->jf], &f))
			j->visited = 1;
		else if (t == f) {
			*kp = t;
			j->visited = 0;
		} else
			j->visited = 1;
		break;

	case BPF_RET:
		if (BPF_RVAL(j->code) == BPF_K) {
			/* XXX - what if K is an iface?! */
			*kp = j->k;
			j->visited = 0;
		} else
			j->visited = 1;
		break;

	case BPF_MISC:
		switch (BPF_MISCOP(j->code)) {
		case BPF_CCC:
			/* follow(j->next, kp); */
			j->visited = 1;
			break;
		default:
			j->visited = follow(j->next, kp);
			break;
		}
		break;

	default:
		j->visited = follow(j->next, kp);
		break;
	}
	if (j->visited == 0)
		j->value = *kp;
	return (j->visited);
}

/*
 * Return 0 if we have various end results
 * Return 1 if we always return the same constant (and no stores into
 * ROM or the packet)
 */
int
combine(struct statement *j, struct statement *base)
{
	int t, f;

	if (j->visited)
		return (j->visited < 0 ? 0 : j->visited);
	j->visited = -1;

	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
		if (j->code == base->code && j->k == base->k &&
		    j->iface == NULL && base->iface == NULL) {
			for (f = -1, t = 0; t < BPF_MEMWORDS; ++t) {
				if (scratch[t].code == base->code &&
				    scratch[t].where == base->k) {
					f = t;
					break;
				}
				if (scratch[t].code == 0)
					f = t;
			}
			if (f == -1)
				return (0);
			scratch[f].code = base->code;
			scratch[f].where = base->k;

			j->same = base->same;
			base->same = j;
			combine(j->next, base);
			j->visited = f+1;
			return (f+1);
		}
		return (combine(j->next, base));

	case BPF_LDX:
		/*
		 * When doing an indexed load and change in X
		 * causes us to search no further.
		 */
		if (BPF_MODE(base->code) == BPF_IND)
			return (0);
		return (combine(j->next, base));

	case BPF_MISC:
		if (BPF_MISCOP(j->code) == BPF_CCC ||
		    (BPF_MISCOP(j->code) == TAX &&
		     BPF_MODE(base->code) == BPF_IND))
			return (0);
		return (combine(j->next, base));
		
	case BPF_ST:
	case BPF_STX:
		switch (BPF_MODE(j->code)) {
		case BPF_IND:
		case BPF_ABS:
			return (0);
		}
		return (combine(j->next, base));

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA)
			return (combine(labels[j->k], base));
		t = combine(labels[j->jt], base);
		f = combine(labels[j->jf], base);
		/*
		 * If both return substituion they will return the same value
		 */
		return (t|f);

	case BPF_RET:
		return (0);

	default:
		return (combine(j->next, base));
	}
	return (0);
}

/*
 * Return 0 if we have various end results
 * Return 1 if we always return the same constant (and no stores into
 * ROM or the packet)
 */
int
profile(struct statement *j, int ins, int lds, int sts, int calls)
{
	if (j == NULL)
		return (0);

	switch (BPF_CLASS(j->code)) {
	case BPF_LD:
	case BPF_LDX:
		if (BPF_MODE(j->code) == BPF_IND ||
		    BPF_MODE(j->code) == BPF_MSH ||
		    BPF_MODE(j->code) == BPF_MSHX ||
		    BPF_MODE(j->code) == BPF_ABS)
			++lds;
		return(profile(j->next, ins + 1, lds, sts, calls));

	case BPF_MISC:
		if (BPF_MISCOP(j->code) == BPF_CCC)
			++calls;
		return(profile(j->next, ins + 1, lds, sts, calls));
		
	case BPF_ST:
	case BPF_STX:
		switch (BPF_MODE(j->code)) {
		case BPF_IND:
		case BPF_ABS:
			++sts;
		}
		return(profile(j->next, ins + 1, lds, sts, calls));

	case BPF_JMP:
		if (BPF_OP(j->code) == BPF_JA)
			return(profile(labels[j->k], ins + 1, lds, sts, calls));
		profile(labels[j->jt], ins + 1, lds, sts, calls); 
		profile(labels[j->jf], ins + 1, lds, sts, calls); 
		return (0);

	case BPF_RET:
		printf("%10d %6d %6d %6d\n", ins+1, lds, sts, calls);
		return (0);

	default:
		return(profile(j->next, ins + 1, lds, sts, calls));
	}
}

void
verify_program()
{
	struct statement *j, *d, *n, *lj;
	extern int error;
	int more, redo;
	int optimized = 0;
	int i;

	if (program == NULL) {
		fprintf(stderr, "%s: no program\n", srcfile);
		error = 1;
		return;
	}
	if (nlabels > 0 && labels[nlabels-1] == NULL) {
		fprintf(stderr, "%s: labels past the end of the program\n",
		    srcfile);
		error = 1;
		return;
	}

	/*
	 * Number all the statements for future reference
	 */
	for (pc = 0, j = program; j; j = j->next) {
		j->icnt = ++pc;
		xbugmsg(1, j, "installed");
	}

all_over:
	/*
	 * Get an initial idea of where things will end up
	 */
	for (pc = 0, j = program; j; j = j->next) {
		j->reachable = stupid;
		j->pc = pc++;
#ifdef	BPF_128
		if (j->code & BPF_TRIPLE)
			pc += 2;
#endif
	}
	/*
	 * Verify that all labels are resolved and make all jumps dynamic
	 */
	program->reachable = 1;	/* We can always reach the first statement */
	xbugmsg(1, program, "marked as reachable");
	lj = NULL;
	for (j = program; j; j = (lj = j)->next) {
		if (j->reachable == 0) {
			xbugmsg(1, j, "unreachable -- deleted");
			/*
			 * We cannot be reached, just pull us out
			 * No one can possibly be calling us, so don't
			 * worry about that, either.
			 */
			while (j->next && j->next->reachable == 0) {
				j = j->next;
				xbugmsg(1, j, "unreachable -- deleted");
			}
			lj->next = j->next;
			j = lj;
			continue;
		}
		if (j->needk) {
			fprintf(stderr, "%s: %d: L%d: unresolved\n",
			    j->src, j->line, j->k);
			error = 1;
		}
		if (j->needt) {
			fprintf(stderr, "%s: %d: L%lu: unresolved\n",
			    j->src, j->line, j->jt);
			error = 1;
		}
		if (j->needf) {
			fprintf(stderr, "%s: %d: L%lu: unresolved\n",
			    j->src, j->line, j->jf);
			error = 1;
		}
		if (BPF_CLASS(j->code) == BPF_JMP) {
			/*
			 * We are some sort of jump.
			 * Make sure that the statements we jump to
			 * are all marked reachable.
			 */
			if (BPF_OP(j->code) == BPF_JA) {
				if (j->relocatek == 0 && j->k == 0) {
					j->k = nlabels;
					j->relocatek = 1;
					add_label(0);
					labels[j->k] = j->next;
				} else if (j->relocatek == 0) {
					fprintf(stderr, "%s: %d: fixed jump -- should never happen (k == %u)\n",
					    j->src, j->line, j->k);
					error = 1;
				}
				if (BPF_CLASS(labels[j->k]->code) == BPF_JMP &&
				    BPF_OP(labels[j->k]->code) == BPF_JA &&
				    labels[j->k]->relocatek) {
					j->k = labels[j->k]->k;
				}
				if (labels[j->k]->reachable == 0) {
					xbugmsg(1, labels[j->k], "marked as reachable");
				}
				labels[j->k]->reachable = 1;
				labels[j->k]->target = 1;
				add_caller(labels[j->k], j);
			} else {
				if (j->relocatef == 0 && j->jf == 0) {
					j->jf = nlabels;
					j->relocatef = 1;
					add_label(0);
					labels[j->jf] = j->next;
				} else if (j->relocatef == 0) {
					fprintf(stderr, "%s: %d: fixed jump -- should never happen (f == %lu)\n",
					    j->src, j->line, j->jf);
					error = 1;
				}
				if (BPF_CLASS(labels[j->jf]->code) == BPF_JMP &&
				    BPF_OP(labels[j->jf]->code) == BPF_JA &&
				    labels[j->jf]->relocatek &&
				    labels[labels[j->jf]->k]->pc - j->pc < 250)
					j->jf = labels[j->jf]->k;
				if (labels[j->jf]->reachable == 0) {
					xbugmsg(1, labels[j->jf], "marked as reachable");
				}
				labels[j->jf]->reachable = 1;
				labels[j->jf]->target = 1;
				add_caller(labels[j->jf], j);

				if (j->relocatet == 0 && j->jt == 0) {
					j->jt = nlabels;
					j->relocatet = 1;
					add_label(0);
					labels[j->jt] = j->next;
				} else if (j->relocatet == 0) {
					fprintf(stderr, "%s: %d: fixed jump -- should never happen (t == %lu)\n",
					    j->src, j->line, j->jt);
					error = 1;
				}
				if (BPF_CLASS(labels[j->jt]->code) == BPF_JMP &&
				    BPF_OP(labels[j->jt]->code) == BPF_JA &&
				    labels[j->jt]->relocatek &&
				    labels[labels[j->jt]->k]->pc - j->pc < 250)
					j->jt = labels[j->jt]->k;
				if (labels[j->jt]->reachable == 0) {
					xbugmsg(1, labels[j->jt], "marked as reachable");
				}
				labels[j->jt]->reachable = 1;
				labels[j->jt]->target = 1;
				add_caller(labels[j->jt], j);
			}
		} else if (j->next && BPF_CLASS(j->code) != BPF_RET) {
			/*
			 * If we are not a jump or return then the next
			 * statement is sure to be reached.
			 * Use Prev to indiciate who ever is previous
			 * should be looked at.
			 */
			add_caller(j->next, &Prev);
			if (j->next->reachable == 0) {
				xbugmsg(1, j->next, "marked as reachable");
			}
			j->next->reachable = 1;
		}
		j->prev = lj;
	}

	more = 1;
	while (more && !stupid) {
		more = 0;
		for (j = program; j && j->next; j = j->next) {
			u_long k;
			if (BPF_CLASS(j->code) == BPF_RET)
				continue;
			for (n = j; n; n = n->next)
				n->visited = -1;
			if (j->iface == NULL && follow(j, &k) == 0) {
				xbugmsg(1, j, "convert from");
				j->code = BPF_RET|BPF_K;
				xbugmsg(1, j, "converted to");
				j->k = k;
				j->jf = j->jt = 0;
				j->relocatek = j->relocatef = j->relocatet = 0;
				++more;
			}
		}
		if (more == 0)
			break;
		more = 0;

		for (j = program; j; j = j->next) {
			j->ncallers = 0;
			j->reachable = 0;
		}
		program->reachable = 1;
		lj = NULL;
		for (j = program; j; j = (lj = j)->next) {
			if (j->reachable == 0) {
				xbugmsg(1, j, "not reachable -- deleted");
				/*
				 * We cannot be reached, just pull us out
				 * No one can possibly be calling us, so don't
				 * worry about that, either.
				 */
				while (j->next && j->next->reachable == 0) {
					j = j->next;
					xbugmsg(1, j, "not reachable -- deleted");
				}
				lj->next = j->next;
				j = lj;
				continue;
			}
			if (BPF_CLASS(j->code) == BPF_JMP) {
				/*
				 * We are some sort of jump.
				 * Make sure that the statements we jump to
				 * are all marked reachable.
				 */
				if (BPF_OP(j->code) == BPF_JA) {
					if (BPF_CLASS(labels[j->k]->code) == BPF_JMP &&
					    BPF_OP(labels[j->k]->code) == BPF_JA &&
					    labels[j->k]->relocatek) {
						j->k = labels[j->k]->k;
					}
					labels[j->k]->reachable = 1;
					labels[j->k]->target = 1;
					add_caller(labels[j->k], j);
				} else {
					labels[j->jf]->reachable = 1;
					labels[j->jf]->target = 1;
					add_caller(labels[j->jf], j);

					labels[j->jt]->reachable = 1;
					labels[j->jt]->target = 1;
					add_caller(labels[j->jt], j);
				}
			} else if (j->next && BPF_CLASS(j->code) != BPF_RET) {
				/*
				 * If we are not a jump or return then the next
				 * statement is sure to be reached.
				 * Use Prev to indiciate who ever is previous
				 * should be looked at.
				 */
				add_caller(j->next, &Prev);
				j->next->reachable = 1;
			}
			j->prev = lj;
		}
	}

	if (error)
		return;

    do {
	/*
	 * Now keep looping through the instructions until we
	 * we make a pass that doesn't change anything.
	 * We look for:
	 *
	 *  o	JMP's to the next line of code (remove)
	 *  o	Jcond's where we jump to the same location for true
	 *	as well as false (convert to a JMP)
	 *  o	Jcond's past 255 instructions (insert a JMP to the
	 *	distant location and then jump to JMP)
	 *  o	Redundant LD's
	 *
	 * It should be noted that every line of the program is reachable
	 * at this point.  If the program starts with a JMP then we are
	 * assured it is a JMP to the next line (and we will just move the
	 * start of the program forward one instruction).
	 */
	do {
		redo = 0;
		more = 0;
		/*
		 * Pull out jmp's to next line
		 */
		pc = 0;
		for (j = program; j; j = j->next) {
			if (!stupid &&
			    BPF_CLASS(j->code) == BPF_JMP &&
			    BPF_OP(j->code) == BPF_JA &&
			    labels[j->k] == j->next) {
				++more;
				if (j->prev) {
					for (i = 0; i < nlabels; ++i) {
						if (labels[i] == j)
							labels[i] = j->next;
					}
					xbugmsg(1, j, "remove JMP to next");
					if (j->prev->next == j->next)
						j->next->prev = j->prev;
					j = j->prev;
				} else {
					xbugmsg(1, j, "remove JMP to next");
					program = j->next;
					program->prev = NULL;
				}
			} else {
				j->pc = pc++;
#ifdef	BPF_128
				if (j->code & BPF_TRIPLE)
					pc += 2;
#endif
			}
		}

		for (j = program; j; j = j->next) {
			if (j->relocatet) {
				d = labels[j->jt];
				if (stupid && d->pc - j->pc > 255) {
					fprintf(stderr, "%s: %d: conditional jump too far\n", j->src, j->line);
					error = 1;
				} else if (d->pc - j->pc > 255) {
					++more;
					n = new(sizeof(*n));
					memset(n, 0, sizeof(*n));
					add_label(0);
					n->icnt = -1;
					n->code = BPF_JMP|BPF_JA;
					n->k = j->jt;
					n->relocatek = 1;
					j->jt = nlabels - 1;
					labels[j->jt] = n;
					if ((n->next = j->next) != NULL)
						n->next->prev = n;
					n->prev = j;
					j->next = n;
					xbugmsg(1, n, "indirect jump added");
				}
			}
			if (j->relocatef) {
				d = labels[j->jf];
				if (stupid && d->pc - j->pc > 255) {
					fprintf(stderr, "%s: %d: conditional jump too far\n", j->src, j->line);
					error = 1;
				} else if (d->pc - j->pc > 255) {
					++more;
					n = new(sizeof(*n));
					memset(n, 0, sizeof(*n));
					add_label(0);
					n->icnt = -1;
					n->code = BPF_JMP|BPF_JA;
					n->k = j->jf;
					n->relocatek = 1;
					j->jf = nlabels - 1;
					labels[j->jf] = n;
					if ((n->next = j->next) != NULL)
						n->next->prev = n;
					n->prev = j;
					j->next = n;
					xbugmsg(1, n, "indirect jump added");
				}
			}
			if (!optimized && !stupid &&
			    BPF_CLASS(j->code) == BPF_JMP &&
			    BPF_OP(j->code) != BPF_JA &&
			    labels[j->jt] == labels[j->jf]) {
				/*
				 * If both the true and false labels
				 * go to the same instruction just
				 * convert this to an unconditional
				 * jump.
				 */
				xbugmsg(1, j, "convert from");
				++more;
				j->k = j->jt;
				j->relocatek = 1;
				j->jt = j->jf = 0;
				j->relocatet = 0;
				j->relocatef = 0;
				j->code = BPF_JMP|BPF_JA;
				xbugmsg(1, j, "converted to");
				redo = 1;
			}

			/*
			 * If we are producing an A or X then make sure
			 * it is used.
			 * Also check scracth memory.
			 */
			if (!optimized && !stupid && j->next) {
				int (*trace)();
				int i;

				switch (BPF_CLASS(j->code)) {
				case BPF_ST:
				case BPF_STX:
					if (BPF_MODE(j->code) != BPF_MEM)
						break;
					trace = follow_m;
					goto check;
					
				case BPF_MISC:
					switch (BPF_MISCOP(j->code)) {
					case BPF_TAX:
						trace = follow_x;
						goto check;
					case BPF_CCC:
						break;
					default:
						trace = follow_a;
						goto check;
					}
					break;

				case BPF_LD:
#ifdef	BPF_128
					if ((j->code & BPF_TRIPLE) ||
					    BPF_SIZE(j->code) == BPF_128)
						trace = follow_q;
					else
#endif
						trace = follow_a;
					goto check;

				case BPF_JMP:
					/*
					 * XXX Should also check
					 * BPF_JMP|BPF_JEQ|BPF_TRIPLE
					 */
					if (j->code == (BPF_JMP|BPF_JEQ|BPF_K)&&
					    j->iface == NULL && 
					    j->k == 0 && j->ncallers == 1 &&
					    (n = j->prev) && n->k == 0 &&
					    n->code == (BPF_ALU|BPF_AND|BPF_K)){
						xbugmsg(1, j, "convert from");
						j->code = BPF_JMP|BPF_JA;
						j->k = j->jt;
						j->relocatek = 1;
						j->jt = j->jf = 0;
						j->relocatet = 0;
						j->relocatef = 0;
						xbugmsg(1, j, "converted to");
						redo = 1;
					}
					break;

				case BPF_ALU:
					trace = follow_a;
					goto check;

				case BPF_LDX:
#ifdef	BPF_128
					if (j->code ==(BPF_LDX|BPF_MSH|BPF_128))
						break;
						/* trace = follow_ax; */
					else
#endif
						trace = follow_x;
					goto check;
				check:
					if (j->iface == NULL)
						break;
					for (n = j->next; n; n = n->next)
						n->visited = -1;
					if ((*trace)(j->next, j->k) == 0) {
					    xbugmsg(1, j, "deleted by %s",
#ifdef	BPF_128
trace == follow_q ? "follow_q" :
#endif
trace == follow_a ? "follow_a" :
trace == follow_m ? "follow_m" :
trace == follow_x ? "follow_x" :
"unknown");
					    for (i = 0; i < j->ncallers; ++i)
						    add_caller(j->next,
							j->callers[i]);
					    for (i = 0; i < nlabels; ++i)
						    if (labels[i] == j)
							    labels[i] = j->next;
					    j->next->prev = j->prev;
					    if (j->next->prev == NULL)
						    program = j->next;
					    else
						    j->prev->next = j->next;
					    ++more;
					}
					break;
				}
			}

			/*
			 * We are a load.  Trace all possible paths backwards
			 * to see if we really need to do this load
			 */
			if (!optimized && !stupid && j->next && j->prev &&
			    (BPF_CLASS(j->code) == BPF_LD ||
			     BPF_CLASS(j->code) == BPF_LDX) &&
			     j->iface == NULL) {
				int (*trace)();
				int i;

				for (n = program; n; n = n->next)
					n->visited = -1;

				if (BPF_CLASS(j->code) == BPF_LDX)
					trace = trace_ldx;
				else if (BPF_MODE(j->code) == BPF_IND)
					trace = trace_ild;
				else
					trace = trace_ld;
				
				for (i = 0; i < j->ncallers; ++i) {
					if ((n = j->callers[i]) == &Prev)
						n = j->prev;
					if ((*trace)(n, j->code, j->k))
						break;
				}
				if (i >= j->ncallers) {
					xbugmsg(1, j, "deleted (no callers)");
					for (i = 0; i < j->ncallers; ++i)
						add_caller(j->next,
						    j->callers[i]);
					for (i = 0; i < nlabels; ++i)
						if (labels[i] == j)
							labels[i] = j->next;
					j->next->prev = j->prev;
					j->prev->next = j->next;
					++more;
				}
			}
		}
		if (redo && !error)
			goto all_over;
	} while (!error && more);

	/*
	 * If we have already been by here we don't need to go down
	 * this path again.
	 */
	if (stupid || optimized++ || !docache)
		break;

	more = 0;
	for (j = program; j; j = j->next) {
		switch (BPF_CLASS(j->code)|BPF_MODE(j->code)) {
		case BPF_LD|BPF_MEM:
		case BPF_LDX|BPF_MEM:
		case BPF_ST:
		case BPF_STX:
		case BPF_ST|BPF_MEM:
		case BPF_STX|BPF_MEM:
			scratch[j->k].code = -1;
			break;
		}
	}

	/*
	 * check all our loads of A
	 */
	for (j = program; j; j = j->next) {
		if (BPF_CLASS(j->code) != BPF_LD)
			continue;
#ifdef	BPF_128
		if ((j->code & BPF_TRIPLE) || BPF_SIZE(j->code) == BPF_128)
			continue;
#endif
		switch (BPF_MODE(j->code)) {
		case BPF_MSH:
		case BPF_MSHX:
		case BPF_ABS:
		case BPF_IND:
			for (n = j->next; n; n = n->next)
				n->visited = 0;

			j->same = NULL;
			i = combine(j->next, j);
			if (i == 0 || j->same == NULL)
				break;

			/* 
			 * Make sure there are *no* possible stores
			 * into the packet within range of our cache.
			 * If we are an indirect load make sure there
			 * is no possibility of X changing as well.
			 */
			for (n = j, d = j->same; d; d = d->same)
				if (d->pc > n->pc)
					n = d;

			for (d = j->next; d->pc <= n->pc; d = d->next) {
				/*
				 * If we are indirect and X changes then
				 * we can't cache
				 */
				if (((BPF_MODE(j->code) == BPF_IND ||
				      BPF_MODE(j->code) == BPF_MSHX) &&
				     BPF_CLASS(d->code) == BPF_LDX) ||
				    d->code == (BPF_MISC|BPF_TAX)) {
					j->same = NULL;
					break;
				}
				/*
				 * stores invalidate us
				 */
				if (((BPF_CLASS(d->code) == BPF_ST ||
				      BPF_CLASS(d->code) == BPF_STX) &&
				     (BPF_MODE(d->code) == BPF_ABS ||
				      BPF_MODE(d->code) == BPF_IND)) ||
				    d->code == (BPF_MISC|BPF_CCC)) {
					j->same = NULL;
					break;
				}
			}

			if (j->same == NULL)
				break;

			/*
			 * Okay, it is safe to cache the value.
			 * Go change all the loads from memory to
			 * loads from the cache, but we must make sure
			 * the cache load will be hit from every possible
			 * history.
			 */
			n = NULL;
			for (d = j->same; d ; d = d->same) {
				if (checkhistory(d, j)) {
					n = d;
					d->code = BPF_LD|BPF_MEM;
					d->k = i - 1;
				}
			}
			if (n != NULL) {
				/*
				 * Now append a store into the cache after our
				 * load.
				 */
				n = new(sizeof(*n));
				n->code = BPF_ST|BPF_MEM;
				n->k = i - 1;
				n->prev = j;
				if ((n->next = j->next) != NULL)
					n->next->prev = n;
				j->next = n;
			} else {
				/*
				 * Okay, we don't need it, toss it
				 */
				scratch[i-1].code = 0;
				scratch[i-1].where = 0;
			}


			/*
			 * We added an instruction, we have to go through
			 * the whole thing again as we may have caused
			 * a conditional jump go beyond the 255 instruction
			 * limit.
			 */
			++more;
			break;
		}
	}
    } while (more);

	if (doprofile) {
		printf("%10s %6s %6s %6s\n",
		    "statements", "loads", "stores", "calls");
		profile(program, 0, 0, 0, 0);
	}

	for (j = program; j; j = j->next) {
		if (j->relocatek)
			j->k = labels[j->k]->pc - j->pc - 1;
		if (j->relocatet)
			j->jt = labels[j->jt]->pc - j->pc - 1;
		if (j->relocatef)
			j->jf = labels[j->jf]->pc - j->pc - 1;
	}
}

void
write_program(int fd)
{
	extern int error;
	struct statement *j;
	struct bpf_insn *p;
	struct ipfw_header *bpf;
	char *q;
	int relcnt = 0;
	int ipc;

	for (j = program; j; j = j->next) {
		if (j->iface)
			relcnt += (strlen(j->iface) + 2 * sizeof(int))
			    & ~(sizeof(int)-1);
	}

	bpf = new(sizeof(*p) * pc + relcnt + sizeof(*bpf));
	memset(bpf, 0, sizeof(*bpf));
	bpf->magic = IPFW_MAGIC;
	bpf->type = IPFW_BPF;
	p = (struct bpf_insn *)(bpf + 1);

	for (j = program, ipc = 0; j; j = j->next, ++p, ++ipc) {
		p->code = j->code;

		if (p->code == (BPF_ST|BPF_MEM))
			p->code = BPF_ST;
		if (p->code == (BPF_STX|BPF_MEM))
			p->code = BPF_STX;
#ifdef	BPF_JNET
		if (p->code == (BPF_JMP|BPF_JNET))
			p->code |= (j->mask << 8) & 0x1f00;
#endif
		p->k = j->k;
		p->jt = j->jt;
		p->jf = j->jf;
		j->pc = ipc;
#ifdef	BPF_128
		if (p->code & BPF_TRIPLE) {
			if (BPF_CLASS(j->code) == BPF_JMP &&
			    BPF_OP(j->code) != BPF_JA) {
				if (p->jt < 2 || p->jf < 2) {
					fprintf(stderr, "%s: %d: negative jump over triple\n", j->src, j->line);
					error = 1;
				}
				p->jt -= 2;
				p->jf -= 2;
			}
			*(q128_t *)(p + 1) = j->q;
			ipc += 2;
			p += 2;
		}
#endif
	}
	bpf->icnt = ipc;
	q = (char *)p;
	for (j = program; j; j = j->next) {
		if (j->iface) {
			*(int *)q = j->pc;
			q += sizeof(int);
			ipc = strlen(j->iface) + sizeof(int);
			ipc &= ~(sizeof(int) - 1);
			((int *)q)[ipc / sizeof(int) - 1] = 0;
			strcpy(q, j->iface);
			q += ipc;
		}
	}
	if (write(fd, bpf, q - (char *)bpf) != q - (char *)bpf)
		err(1, NULL);
}

void *
new(size_t s)
{
	void *v = malloc(s);
	if (v)
		return (v);
	err(1, NULL);
}

void *
renew(void *v, size_t s)
{
	v = realloc(v, s);
	if (v)
		return (v);
	err(1, NULL);
}

void
yyerror(const char *s)
{
	extern int error;
	fprintf(stderr, "%s: %d: %s\n", srcfile, linecnt, s);
	error = 1;
}

int
yywrap()
{
	return(1);
}

char *
lastpart(char *s)
{
	char *r = strrchr(s, '/');
	return (r ? r + 1 : s);
}


char *codenames[] = {
"LD_IMM", "LDX_IMM", "ST_IMM", "STX_IMM",
"ADDK", "JAK", "RET_K", "TAX",
"LD_IMMH", "LDX_IMMH", "ST_IMMH", "STX_IMMH",
"ADDX", "CODE 0d", "RET_X", "CODE 0f",
"LD_IMMB", "LDX_IMMB", "ST_IMMB", "STX_IMMB",
"SUBK", "JEQK", "RET_A", "CODE 17",
"LD_IMMQ", "LDX_IMMQ", "ST_IMMQ", "STX_IMMQ",
"SUBX", "JEQX", "CODE 1e", "CODE 1f",
"LD_ABS", "LDX_ABS", "ST_ABS", "STX_ABS",
"MULK", "JGTK", "CODE 26", "CODE 27",
"LD_ABSH", "LDX_ABSH", "ST_ABSH", "STX_ABSH",
"MULX", "JGTX", "CODE 2e", "CODE 2f",
"LD_ABSB", "LDX_ABSB", "ST_ABSB", "STX_ABSB",
"DIVK", "JGEK", "CODE 36", "CODE 37",
"LD_ABSQ", "LDX_ABSQ", "ST_ABSQ", "STX_ABSQ",
"DIVX", "JGEX", "CODE 3e", "CODE 3f",
"LD_IND", "LDX_IND", "ST_IND", "STX_IND",
"ORK", "JSETK", "CODE 46", "CCC",
"LD_INDH", "LDX_INDH", "ST_INDH", "STX_INDH",
"ORX", "JSETX", "CODE 4e", "CODE 4f",
"LD_INDB", "LDX_INDB", "ST_INDB", "STX_INDB",
"ANDK", "JNETK", "CODE 56", "CODE 57",
"LD_INDQ", "LDX_INDQ", "ST_INDQ", "STX_INDQ",
"ANDX", "JNETX", "CODE 5e", "CODE 5f",
"LD_MEM", "LDX_MEM", "ST_MEM", "STX_MEM",
"LSHK", "CODE 65", "CODE 66", "CODE 67",
"LD_MEMH", "LDX_MEMH", "ST_MEMH", "STX_MEMH",
"LSHX", "CODE 6d", "CODE 6e", "CODE 6f",
"LD_MEMB", "LDX_MEMB", "ST_MEMB", "STX_MEMB",
"RSHK", "CODE 75", "CODE 76", "CODE 77",
"LD_MEMQ", "LDX_MEMQ", "ST_MEMQ", "STX_MEMQ",
"RSHX", "CODE 7d", "CODE 7e", "CODE 7f",
"LD_LEN", "LDX_LEN", "ST_LEN", "STX_LEN",
"NEGK", "CODE 85", "CODE 86", "TXA",
"LD_LENH", "LDX_LENH", "ST_LENH", "STX_LENH",
"NEGX", "CODE 8d", "CODE 8e", "CODE 8f",
"LD_LENB", "LDX_LENB", "ST_LENB", "STX_LENB",
"CODE 94", "CODE 95", "CODE 96", "CODE 97",
"LD_LENQ", "LDX_LENQ", "ST_LENQ", "STX_LENQ",
"CODE 9c", "CODE 9d", "CODE 9e", "CODE 9f",
"LD_MSH", "LDX_MSH", "ST_MSH", "STX_MSH",
"CODE a4", "CODE a5", "CODE a6", "CODE a7",
"LD_MSHH", "LDX_MSHH", "ST_MSHH", "STX_MSHH",
"CODE ac", "CODE ad", "CODE ae", "CODE af",
"LD_MSHB", "LDX_MSHB", "ST_MSHB", "STX_MSHB",
"CODE b4", "CODE b5", "CODE b6", "CODE b7",
"LD_MSHQ", "LDX_MSHQ", "ST_MSHQ", "STX_MSHQ",
"CODE bc", "CODE bd", "CODE be", "CODE bf",
"LD_ROM", "LDX_ROM", "ST_ROM", "STX_ROM",
"CODE c4", "CODE c5", "CODE c6", "ZMEM",
"LD_ROMH", "LDX_ROMH", "ST_ROMH", "STX_ROMH",
"CODE cc", "CODE cd", "CODE ce", "CODE cf",
"LD_ROMB", "LDX_ROMB", "ST_ROMB", "STX_ROMB",
"CODE d4", "CODE d5", "CODE d6", "CODE d7",
"LD_ROMQ", "LDX_ROMQ", "ST_ROMQ", "STX_ROMQ",
"CODE dc", "CODE dd", "CODE de", "CODE df",
"CODE e0", "LD_MSHX", "ST_MSHX", "STX_MSHX",
"CODE e4", "CODE e5", "CODE e6", "CODE e7",
"CODE e8", "LD_MSHXH", "ST_MSHXH", "STX_MSHXH",
"CODE ec", "CODE ed", "CODE ee", "CODE ef",
"CODE f0", "LD_MSHXB", "ST_MSHXB", "STX_MSHXB",
"CODE f4", "CODE f5", "CODE f6", "CODE f7",
"CODE f8", "LD_MSHXQ", "CODE fa", "STX_MSHXQ",
"CODE fc", "CODE fd", "CODE fe", "CODE ff",
};

void
_xbugmsg(int level, struct statement *j, char *msg, ...)
{
	va_list ap;
	int c;

	if (level > xbug)
		return;

	fprintf(stderr, "%s:%d: ", lastpart(xfile__), xline__);
	if (j) {
		fprintf(stderr, "%d: ", j->icnt);
		c = j->code;
		if (c & BPF_TRIPLE) {
			fprintf(stderr, "Q ");
			c *= ~BPF_TRIPLE;
		}
		if (c < 0x100)
			fprintf(stderr, "%s ", codenames[c]);
		else
			fprintf(stderr, "%04x ", c);

		if (j->iface) fprintf(stderr, "[%s]", j->iface);
		if (j->needk) fprintf(stderr, "K");
		if (j->needt) fprintf(stderr, "T");
		if (j->needf) fprintf(stderr, "F");
		if (j->relocatek) fprintf(stderr, "k");
		if (j->relocatet) fprintf(stderr, "t");
		if (j->relocatef) fprintf(stderr, "f");
		if (j->reachable) fprintf(stderr, "r");
		if (j->target) fprintf(stderr, "g");
		fprintf(stderr, " %x[%lx,%lx]: ", j->k, j->jt, j->jf);
	}
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end (ap);
	fprintf(stderr, "\n");
}
