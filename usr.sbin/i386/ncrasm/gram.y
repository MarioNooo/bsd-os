%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gram.y,v 1.3 1999/12/09 18:41:32 cp Exp
 */


#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <dev/pci/ncrreg.h>
#include "script.h"
#include <symbol.h>

#define	FORMAT(n) ((n) > -10 && (n) < 10 ? "%d" : "0x%x")
#if 0
#define	stop(s)	error(s), exit(1)
#endif

stop()
{
	exit (1);
}

int	yylex __P((void));

extern yyline;
extern pass;

void
yyerror(const char *string)
{ 
	fprintf(stderr, "line %d %s\n",yyline, string);
	exit (1);
}

static int phase;

%}

%union {
	symbol_t	*symbol;
	char		*str;
	int		 value;
}


%token	T_BLKMV					/* block move */
%token	T_MEMMV
%token	T_CARRY T_TARGET T_ACK T_ATN
%token	T_EOF T_EOS
%token	T_TI				/* table indirect */
%token	T_I				/* indirect */
%token	T_R
%token	T_NOT
%token	T_FLY
%token	T_MV

%token 	T_DATA	T_MASK T_WAIT;

%token	<value>	T_OR T_LD T_AND T_ADD T_SHFTL



%token <value>	T_REGISTER				/* a register */
%token <value>	T_SFBR
%token <value>	T_PHASE
%token <symbol>	T_SYMBOL
%token <symbol>	T_TAG

%token <value>	T_SELECT T_RESELECT T_DISCONNECT T_SET T_CLEAR
%token <value>	T_SCSI_ID

%token <value>	T_RETURN T_CALL T_INTERRUPT T_JUMP
%token <value>	T_TC_FORCE


%token	T_DO T_DI T_CMD T_STATUS T_MO T_MI
%token	<value>	T_NUMBER
%token	<value> T_DOT

%left		'-' '+'
%left		'*' '/'
%left		'^' '|' '&'
/* %nonassoc       UMINUS */

%type	<value>	expression
%type	<value> COND_ATN COND_CARRY COND_TARGET COND_ACK COND_FLY
%type	<value> IO_3
%type	<value> RW
%type	<value> REG_OR_SFBR
%type	<value> COND_R COND_NOT
%type	<value>	COND_DATA COND_MASK 
%type	<value> COND_PHASE
%type	<value>	OFFSET
%type	<value>	ADDR
%type	<value>	COUNT
%type	<value>	TC


%%

pgm:	code
|	pgm code
|	assignment 
|	pgm assignment
|	setloc
|	pgm setloc
;

setloc:	T_DOT '=' expression
	{
	location_counter = $3;
	};

assignment:	T_SYMBOL '=' expression
	{
	    $1->s_value = $3;
	    $1->s_set = 1;
	    if (pass == 2) {
		    if ($1->s_setpass2)
			    fprintf(stderr, "symbol %s redefined line %d\n",
				    $1->s_name, yyline);
		    $1->s_setpass2 = 1;
	    }
	};

code:	T_TAG
	{
	    $1->s_value = location_counter; 
	    $1->s_set = 1;
	    $1->s_tag = 1;
	    if (pass == 2) {
		    if ($1->s_setpass2)
			    fprintf(stderr, "symbol %s redefined line %d\n",
				    $1->s_name, yyline);
		    $1->s_setpass2 = 1;
	    }
	};


expression:	expression '+' expression	{ $$ = $1 + $3;	}
|		expression '-' expression	{ $$ = $1 - $3; }
|		expression '*' expression	{ $$ = $1 * $3; }
|		expression '/' expression	{ $$ = $1 / $3; }
|		expression '|' expression	{ $$ = $1 | $3; }
|		expression '&' expression	{ $$ = $1 & $3; }
|		expression '^' expression	{ $$ = $1 ^ $3; }
|		'(' expression ')'		{ $$ = $2;	}
|		T_NUMBER
|		T_DOT
/* |		'-' expression %prec UMINUS     { $$ = -$2;     } */
|		T_SYMBOL
		{
		if ( pass == 2 && $1->s_set == 0) {
			fprintf(stderr, "symbol %s undefined line %d\n",
				$1->s_name, yyline);
		}
		$$ = $1->s_value; }
;


REG_OR_SFBR:	T_REGISTER 	{$$ = $1;}
|	T_SFBR			{$$ = $1;};

code:	T_LD expression REG_OR_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_opcode = NCR_RW_RMW;
	s->s_rwi.rw_operator = $1;
	s->s_rwi.rw_register = $3;
	s->s_rwi.rw_data = $2;
	location_counter += 8;
	}

code:	T_SHFTL REG_OR_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_opcode = NCR_RW_RMW;
	s->s_rwi.rw_operator = $1;
	s->s_rwi.rw_register = $2;
	s->s_rwi.rw_data = 0;
	s->s_rwi.rw_carry_enable = 1;	/* this turn load into shift left */
	location_counter += 8;
	}

code:	RW COND_CARRY expression REG_OR_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_opcode = NCR_RW_RMW;
	s->s_rwi.rw_operator = $1;
	s->s_rwi.rw_carry_enable = $2;
	s->s_rwi.rw_data = $3;
	s->s_rwi.rw_register = $4;
	location_counter += 8;
	}

code:	RW COND_CARRY T_SFBR expression REG_OR_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_operator = $1;
	s->s_rwi.rw_carry_enable = $2;
	s->s_rwi.rw_opcode = NCR_RW_FROMSFBR;
	s->s_rwi.rw_data = $4;
	s->s_rwi.rw_register = $5;
	location_counter += 8;
	}

code:	T_MV T_SFBR T_REGISTER T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_operator = NCR_RW_OR;
	s->s_rwi.rw_opcode = NCR_RW_FROMSFBR;
	s->s_rwi.rw_register = $3;
	location_counter += 8;
	}

code:	T_MV T_REGISTER T_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_operator = NCR_RW_OR;
	s->s_rwi.rw_opcode = NCR_RW_TOSFBR;
	s->s_rwi.rw_register = $2;
	location_counter += 8;
	}

code:	RW COND_CARRY T_REGISTER expression T_SFBR T_EOS
	{
	script_t *s = script_alloc();
	s->s_rwi.rw_itype = NCR_RWI;
	s->s_rwi.rw_operator = $1;
	s->s_rwi.rw_carry_enable = $2;
	s->s_rwi.rw_opcode = NCR_RW_TOSFBR;
	s->s_rwi.rw_register = $3;
	s->s_rwi.rw_data = $4;
	location_counter += 8;
	}

RW:	T_OR | T_AND | T_ADD;

code:	T_BLKMV T_PHASE COUNT ADDR T_EOS 
	{
	script_t *s = script_alloc();
	s->s_blkmv.bm_itype = NCR_BLKMV;
	s->s_blkmv.bm_opcode = 1;
	s->s_blkmv.bm_phase = $2;
	s->s_blkmv.bm_byte_count = $3;
	s->s_blkmv.bm_addr = $4;
	s->s_w1_tablerelative = 1;
	location_counter += 8;
	};

code:	T_BLKMV T_PHASE I COUNT ADDR  T_EOS 
	{
	script_t *s = script_alloc();
	s->s_blkmv.bm_itype = NCR_BLKMV;
	s->s_blkmv.bm_opcode = 1;
	s->s_blkmv.bm_indirect = 1;
	s->s_blkmv.bm_phase = $2;
	s->s_blkmv.bm_byte_count = $4;
	s->s_blkmv.bm_addr = $5;
	s->s_w1_tablerelative = 1;
	location_counter += 8;
	};

code:	T_BLKMV T_PHASE T_TI OFFSET T_EOS
	{
	script_t *s = script_alloc();
	s->s_blkmv.bm_itype = NCR_BLKMV;
	s->s_blkmv.bm_opcode = 1;
	s->s_blkmv.bm_table_indirect = 1;
	s->s_blkmv.bm_phase = $2;
	s->s_blkmv.bm_addr = $4;
	location_counter += 8;
	};

IO_3:	T_SET | T_CLEAR;

OFFSET:	expression;

ADDR:	expression;

COUNT:	expression;

code:	T_MEMMV COUNT REG_OR_SFBR OFFSET T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_memmv.mm_itype = NCR_MEMMV;
	    s->s_memmv.mm_byte_count = $2;
	    s->s_triple = 1;
	    s->s_memmv.mm_source_addr = $3;
	    s->s_memmv.mm_dest_addr = $4;
	    s->s_w1_iorelative = 1;
	    s->s_w2_tablerelative = 1;
	    location_counter += 12;
	};

code:	T_MEMMV COUNT OFFSET REG_OR_SFBR T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_memmv.mm_itype = NCR_MEMMV;
	    s->s_memmv.mm_byte_count = $2;
	    s->s_triple = 1;
	    s->s_memmv.mm_source_addr = $3;
	    s->s_memmv.mm_dest_addr = $4;
	    s->s_w1_tablerelative = 1;
	    s->s_w2_iorelative = 1;
	    location_counter += 12;
	};

code:	T_MEMMV COUNT OFFSET OFFSET T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_memmv.mm_itype = NCR_MEMMV;
	    s->s_memmv.mm_byte_count = $2;
	    s->s_triple = 1;
	    s->s_memmv.mm_source_addr = $3;
	    s->s_memmv.mm_dest_addr = $4;
	    s->s_w1_tablerelative = 1;
	    s->s_w2_tablerelative = 1;
	    location_counter += 12;
	};

code:	T_MEMMV COUNT REG_OR_SFBR REG_OR_SFBR T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_memmv.mm_itype = NCR_MEMMV;
	    s->s_memmv.mm_byte_count = $2;
	    s->s_triple = 1;
	    s->s_memmv.mm_source_addr = $3;
	    s->s_memmv.mm_dest_addr = $4;
	    s->s_w1_iorelative = 1;
	    s->s_w2_iorelative = 1;
	    location_counter += 12;
	};


code:	T_SELECT COND_ATN T_SCSI_ID COND_R OFFSET T_EOS
	{
	script_t *s = script_alloc();
	s->s_ioi.io_itype = NCR_IOI;
	s->s_ioi.io_opcode = $1;
	s->s_ioi.io_select_attn = $2;
	s->s_ioi.io_scsi_id = 1 << $3;
	if (s->s_ioi.io_relative =  $4)
	    s->s_ioi.io_addr = $5 - (location_counter +8);
	else {
	    s->s_ioi.io_addr = $5;
	    s->s_w1_tablerelative = 1;
	}
	location_counter += 8;
	};

code:	T_SELECT COND_ATN T_TI OFFSET COND_R OFFSET T_EOS
	{
	script_t *s = script_alloc();
	s->s_IOI.io_itype = NCR_IOI;
	s->s_IOI.io_opcode = $1;
	s->s_IOI.io_select_attn = $2;
	s->s_IOI.io_table_indirect = 1;
	s->s_IOI.io_table_offset = $4;
	if (s->s_IOI.io_relative =  $5)
	    s->s_IOI.io_addr = $6 - (location_counter + 8);
	else {
	    s->s_IOI.io_addr = $6;
	    s->s_w1_tablerelative = 1;
	}
	location_counter += 8;
	}

code:	T_RESELECT COND_R OFFSET T_EOS
	{
	script_t *s = script_alloc();
	s->s_IOI.io_itype = NCR_IOI;
	s->s_IOI.io_opcode = $1;
	if (s->s_IOI.io_relative =  $2)
	    s->s_IOI.io_addr = $3 - (location_counter + 8);
	else {
	    s->s_IOI.io_addr = $3;
	    s->s_w1_tablerelative = 1;
	}
	location_counter += 8;
	}


code:	T_DISCONNECT T_EOS
	{
	script_t *s = script_alloc();
	s->s_ioi.io_itype = NCR_IOI;
	s->s_ioi.io_opcode = $1;
	location_counter += 8;
	}


code:	IO_3 COND_CARRY COND_TARGET COND_ACK COND_ATN  T_EOS;
	{
	script_t *s = script_alloc();
	s->s_ioi.io_itype = NCR_IOI;
	s->s_ioi.io_opcode = $1;
	s->s_ioi.io_sc_carry = $2;
	s->s_ioi.io_sc_targetmode = $3;
	s->s_ioi.io_sc_ack = $4;
	s->s_ioi.io_sc_attn = $5;
	location_counter += 8;
	};

COND_ATN:
	{$$ = 0;}
|	T_ATN		{$$ = 1;};

COND_FLY:
	{$$ = 0;}
|	T_FLY		{$$ = 1;};

COND_TARGET:
	{$$ = 0;}
|	T_TARGET	{$$ = 1;};

COND_CARRY:
	{$$ = 0;}
|	T_CARRY		{$$ = 1;};

COND_ACK:
	{$$ = 0;}
|	T_ACK		{$$ = 1;};

COND_R:
	{$$ = 0;}
|	T_R		{$$ = 1;};

COND_NOT:
	{$$ = 0;}
|	T_NOT		{$$ = 1;};

COND_DATA:
	{$$  = -1;}
|	T_DATA expression	{$$ = $2;};

COND_MASK:
	{$$  = 0;}
|	T_MASK expression	{$$ = 0xff - $2;};

COND_PHASE:
	{$$ = -1;}
|	T_PHASE
|	T_WAIT T_PHASE
	{$$ = $2 | 0x10;};

TC:	T_JUMP
|	T_INTERRUPT
|	T_CALL;

code:	TC COND_FLY COND_NOT COND_PHASE COND_CARRY COND_DATA COND_MASK
	COND_R OFFSET T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_tci.tc_itype = NCR_TCI;
	    s->s_tci.tc_opcode = $1;
	    s->s_tci.tc_on_the_fly = $2;
	    if (s->s_tci.tc_relative = $8)
		s->s_tci.tc_addr = $9 - (location_counter + 8);
	    else {
		s->s_tci.tc_addr = $9;
		if ($1 != NCR_TC_INTERRUPT)
		    s->s_w1_tablerelative = 1;
	    }
	    s->s_tci.tc_branch_true = $3 ? 0 : 1; 
	    if ($4 != -1) {
		    s->s_tci.tc_compare_phase = 1; 
		    s->s_tci.tc_phase = $4 & 0x7; 
		    s->s_tci.tc_wait_phase = ($4 >> 4) & 1;
	    }
	    s->s_tci.tc_carry_test = $5;
	    if ($6 != -1) {
		    s->s_tci.tc_compare_data = 1;
		    s->s_tci.tc_data = $6;
		    s->s_tci.tc_mask = $7;
	    }
	location_counter += 8;
	};

code:	T_RETURN COND_NOT COND_PHASE COND_CARRY COND_DATA COND_MASK;
	T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_tci.tc_itype = NCR_TCI;
	    s->s_tci.tc_opcode = $1;
	    s->s_tci.tc_branch_true = $2 ? 0 : 1; 
	    if ($3 != -1) {
		    s->s_tci.tc_compare_phase = 1; 
		    s->s_tci.tc_phase = $3 & 0x7; 
		    s->s_tci.tc_wait_phase = ($3 >> 4) & 1;
	    }
	    s->s_tci.tc_carry_test = $4;
	    if ($5 != -1) {
		    s->s_tci.tc_compare_data = 1;
		    s->s_tci.tc_data = $5;
		    s->s_tci.tc_mask = $6;
	    }
	location_counter += 8;
	};


code:	TC T_TC_FORCE COND_R OFFSET T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_tci.tc_itype = NCR_TCI;
	    s->s_tci.tc_opcode = $1;
	    if (s->s_tci.tc_relative = $3)
		s->s_tci.tc_addr = $4 - (location_counter + 8);
	    else {
		s->s_tci.tc_addr = $4;
		if ($1 != NCR_TC_INTERRUPT)
		    s->s_w1_tablerelative = 1;
	    }
	    s->s_tci.tc_branch_true = $2;
	location_counter += 8;
	};

code:	T_RETURN T_TC_FORCE T_EOS
	{
	    script_t *s = script_alloc();
	    s->s_tci.tc_itype = NCR_TCI;
	    s->s_tci.tc_opcode = $1;
	    s->s_tci.tc_branch_true = $2;
	location_counter += 8;
	};


I:	T_I;

code:	T_EOF
	{
	fprintf(stderr, "eof seen\n");
	return (0);
	};

%%
