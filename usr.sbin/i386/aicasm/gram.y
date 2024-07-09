%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gram.y,v 1.3 1999/12/09 19:09:47 cp Exp
 */


#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "sequencer.h"
#include <symbol.h>
#include <i386/isa/aicreg.h>

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
extern char* yyfile;
extern pass;

void
yyerror(const char *string)
{ 
	fprintf(stderr, "file %s, line %d %s\n", yyfile, yyline, string);
	exit (1);
}

static int wod;		/* write only destination */

%}

%union {
	symbol_t	*symbol;
	char		*str;
	int		 value;
}

%token  T_EOF T_EOS T_INCLUDE

%token	<str>	T_PATH

%token  T_EOF T_EOS T_SECTION

%token	T_RETURN
%token	T_COMMA
%token <value> T_REGISTER

%token <symbol> T_SYMBOL
%token <symbol> T_TAG
%token <value>	T_SCB T_SRAM T_CODE

%token  <value> T_NUMBER
%token  <value> T_DOT

%token	<value> T_RR  T_RL T_SR T_SL

%token	<value> T_REG

%token	<value> T_OR T_AND T_XOR T_ADD T_ADDC T_ORI 
%token	<value> T_TST T_CMP 
%token		T_SET T_CLR
%token	<value> T_JMPorCALL
%token	T_ORSI T_LDSI T_LD T_MV
%token	T_AMPERSAND

%token	<value>	T_AC 

%token <value>	T_RWREG
%token <value>	T_ROREG
%token <value>	T_WOREG
%token <value>	T_HOREG



%token	<value> T_JZ T_JNZ T_JS T_JNS T_JE  T_JNE

%left           '-' '+'
%left           '*' '/'
%left           '^' '|' '&'
%nonassoc	UMINUS
%nonassoc	UPLUS
%nonassoc	UNOT

%type   <value> expression
%type	<value> C_RETURN
%type 	<value> DESTINATION SOURCE PSOURCE IMMEDIATE ADDRESS
%type	<value> C_SOURCE

%type	<value> F1_OPCODE F2_OPCODE

%type	<value> JZorJNZ JSorJNS JEorJNE

%type	<value> ROREG WOREG
%type	<value> ADDRESSOF ANYADDR
%type	<value> SCBorSRAM
%type	<value> SRAMorSCBorCODE
%type	<value> ANYREG

%%


pgm:	code
|	pgm code
|	assignment 
|	pgm assignment
|	setloc
|	pgm setloc
|	include
|	pgm include
;

include: T_INCLUDE T_PATH
	{
		includefile($2);
	};

setloc:	T_DOT '=' expression
	{
	location_counter = $3;
	};

assignment:	T_SYMBOL '=' expression
	{
	    $1->s_value = $3;
	    $1->s_section = section;
	    $1->s_set = 1;
	    $1->s_define = 1;
	    if (pass == 2) {
		    if ($1->s_setpass2)
			    fprintf(stderr,
				"symbol %s redefined file %s line %d\n",
				$1->s_name, yyfile, yyline);
		    $1->s_setpass2 = 1;
	    }
	};

assignment:	T_REGISTER T_SYMBOL expression
	{
	    $2->s_value = $3;
	    $2->s_section = $1;
	    $2->s_set = 1;
	    $2->s_define = 1;
	    if (pass == 2) {
		    if ($2->s_setpass2)
			    fprintf(stderr,
				"symbol %s redefined file %s line %d\n",
				$2->s_name, yyfile, yyline);
		    $2->s_setpass2 = 1;
	    }
	};

assignment:	T_REGISTER ANYREG expression
	{
		if ($2 != $3)
			fprintf(stderr,
			    "attempted register redefinition file %s line %d\n",
			     yyfile, yyline);
	};

ANYREG:	T_RWREG | T_ROREG | T_WOREG | T_AC | T_HOREG

code:	T_SECTION SRAMorSCBorCODE	{ section = $2; };

SRAMorSCBorCODE:	T_SRAM | T_SCB | T_CODE

code:	T_TAG
	{
	    $1->s_value = location_counter; 
	    $1->s_section = section;
	    $1->s_set = 1;
	    $1->s_tag = 1;
	    if (pass == 2) {
		    if ($1->s_setpass2)
			    fprintf(stderr,
				"symbol %s redefined file %s line %d\n",
				$1->s_name, yyfile, yyline);
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
|		'-' expression %prec UMINUS     { $$ = -$2;     } 
|		'+' expression %prec UPLUS     { $$ = $2;     } 
|		'~' expression %prec UNOT	{ $$ = ~$2; }
|		T_SYMBOL			{ $$ = getsymvalue($1); }
;


code:	T_EOF
	{
	fprintf(stderr, "eof seen\n");
	return (0);
	};

code:	T_RETURN T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format1.f1_opcode  = AIC_S_OR;
	s->s_format1.f1_destination = AIC_NONE;
	s->s_format1.f1_source = AIC_ALLZEROS;
	s->s_format1.f1_immediate = 0xff;
	s->s_format1.f1_return = 1;
	location_counter++;
	}

code:	F1_OPCODE DESTINATION T_COMMA IMMEDIATE C_SOURCE C_RETURN  T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format1.f1_opcode  = $1;
	s->s_format1.f1_destination = $2;
	if ($5 > 0)
		s->s_format1.f1_source = $5;
	else {
		s->s_format1.f1_source = $2;
		if (wod)
			fprintf(stderr, "write only source %d\n", yyline);
	}
	s->s_format1.f1_immediate = $4;
	if ($4 & 0x100 && pass == 2)
		fprintf(stderr, "illegal immediate line %d\n", yyline);

	s->s_format1.f1_return = $6;
	location_counter++;
	}

code:	T_SET DESTINATION T_COMMA IMMEDIATE C_RETURN  T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format1.f1_opcode = AIC_S_OR;
	s->s_format1.f1_destination = $2;
	s->s_format1.f1_source = $2;
	if (wod)
		fprintf(stderr, "write only source %d\n", yyline);
	s->s_format1.f1_immediate = $4;
	if ($4 & 0x100 && pass == 2)
		fprintf(stderr, "illegal immediate line %d\n", yyline);

	s->s_format1.f1_return = $5;
	location_counter++;
	}

code:	T_CLR DESTINATION T_COMMA IMMEDIATE C_RETURN  T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format1.f1_opcode = AIC_S_AND;
	s->s_format1.f1_destination = $2;
	s->s_format1.f1_source = $2;
	if (wod)
		fprintf(stderr, "write only source %d\n", yyline);
	s->s_format1.f1_immediate = 0xff - $4;
	if (s->s_format1.f1_immediate == 0  && pass == 2)
		fprintf(stderr, "illegal immediate line %d\n", yyline);

	s->s_format1.f1_return = $5;
	location_counter++;
	}

code:	T_LD DESTINATION T_COMMA IMMEDIATE C_RETURN T_EOS
	{
	    sequencer_t *s = sequencer_alloc();

	    if ($4 & 0x100)
		s->s_format1.f1_opcode  = AIC_S_AND;
	    else
		s->s_format1.f1_opcode  = AIC_S_OR;
		    
	    s->s_format1.f1_destination = $2;
	    s->s_format1.f1_source = AIC_ALLZEROS;
	    s->s_format1.f1_return = $5;
	    s->s_format1.f1_immediate = $4;
	    location_counter++;
	};

code:	T_LD DESTINATION T_COMMA PSOURCE C_RETURN T_EOS
	{
	    sequencer_t *s = sequencer_alloc();

	    s->s_format1.f1_opcode  = AIC_S_AND;
	    s->s_format1.f1_destination = $2;
	    s->s_format1.f1_source = $4;
	    s->s_format1.f1_return = $5;
	    s->s_format1.f1_immediate = 0xff;
	    location_counter++;
	};

code:	T_MV DESTINATION T_COMMA PSOURCE C_RETURN T_EOS
	{
	    sequencer_t *s = sequencer_alloc();

	    s->s_format1.f1_opcode  = AIC_S_MV;
	    s->s_format1.f1_destination = $2;
	    s->s_format1.f1_source = $4;
	    s->s_format1.f1_return = $5;
	    s->s_format1.f1_immediate = 1;
	    location_counter++;
	};

code:	T_MV DESTINATION T_COMMA PSOURCE T_COMMA IMMEDIATE C_RETURN T_EOS
	{
	    sequencer_t *s = sequencer_alloc();

	    s->s_format1.f1_opcode  = AIC_S_MV;
	    s->s_format1.f1_destination = $2;
	    s->s_format1.f1_source = $4;
	    s->s_format1.f1_immediate = $6;
	    s->s_format1.f1_return = $7;
	    location_counter++;
	};
		
		

code:	F2_OPCODE DESTINATION T_COMMA expression C_SOURCE C_RETURN T_EOS
	{
	    sequencer_t *s = sequencer_alloc();

	    s->s_format2.f2_opcode = AIC_S_ROL;
	    s->s_format2.f2_destination = $2;
	    s->s_format2.f2_return = $6;
	    if ($5 > 0)
		    s->s_format2.f2_source = $5;
	    else {
		    s->s_format2.f2_source = $2;
		    if (wod)
			    fprintf(stderr, "write only source %d\n", yyline);
	    }
	    if ($4 > 8 || $4 <= 0)
		    fprintf(stderr, "illegal shift value line %d\n", yyline);
	    if ($4 == 8)
		    fprintf(stderr, "warning shift value 8 %d\n", yyline);
	    /*
	     * this code assumes that a count 0 zero generates
	     * a shift  of 8. So far this is not proved
	     */
	    switch ($1) {
	    case AIC_S_SHIFTLEFT:
		    if ($4 == 8) {
			    s->s_format2.f2_shift_control = 0xf0;
			    break;
		    }
		    s->s_format2.f2_shift_control = ($4 << 4) | $4;
		    break;
	    case AIC_S_SHIFTRIGHT:
		    if ($4 == 8) {
			    s->s_format2.f2_shift_control = 0xf8;
			    break;
		    }
		    s->s_format2.f2_shift_control = 
			($4 << 4) | 8 - $4 | 0x08;	
		    break;
	    case AIC_S_ROTATELEFT:
		    s->s_format2.f2_shift_control = $4 & 0x7;
		    break;
	    case AIC_S_ROTATERIGHT:
		    s->s_format2.f2_shift_control = (8 - $4) | 0x08;
		    break;
	    };
	    location_counter++;
	};

code:	T_TST SOURCE T_COMMA IMMEDIATE JSorJNS ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();
	s->s_format3.f3_opcode = AIC_S_TST + $5;
	s->s_format3.f3_source = $2;
	s->s_format3.f3_immediate =  $4;
	if ($4 & 0x100 && pass == 2)
		fprintf(stderr, "illegal immediate line %d\n", yyline);
	s->s_format3.f3_address = $6;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};

code:	T_CMP SOURCE T_COMMA IMMEDIATE JEorJNE ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	if (!($4 & 0x100)) {
		s->s_format3.f3_opcode = AIC_S_CMP + $5;
		s->s_format3.f3_immediate =  $4;
	} else {
		s->s_format3.f3_opcode = AIC_S_TST + $5;
		s->s_format3.f3_immediate =  0xff;
	}
	s->s_format3.f3_source = $2;
	s->s_format3.f3_address = $6;
	s->s_format3.f3_format3 = 1;

	location_counter++;
	};

code:	T_ORSI	SOURCE T_COMMA expression T_JMPorCALL ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format3.f3_opcode = $5;
	s->s_format3.f3_source = $2;
	s->s_format3.f3_immediate =  $4;
	s->s_format3.f3_address = $6;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};

code:	T_LDSI IMMEDIATE T_JMPorCALL ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format3.f3_opcode = $3;
	s->s_format3.f3_source = AIC_ALLZEROS;
	s->s_format3.f3_immediate =  $2;
	s->s_format3.f3_address = $4;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};

code:	T_LDSI PSOURCE T_JMPorCALL ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format3.f3_opcode = $3;
	s->s_format3.f3_source = $2;
	s->s_format3.f3_immediate = 0;
	s->s_format3.f3_address = $4;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};

code:	T_JMPorCALL ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	s->s_format3.f3_opcode = $1;
	s->s_format3.f3_source = AIC_SINDEX;
	s->s_format3.f3_immediate =  0;
	s->s_format3.f3_address = $2;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};

code:	JZorJNZ ADDRESS T_EOS
	{
	sequencer_t *s = sequencer_alloc();

	$1 = $1 == AIC_S_JZ ? AIC_S_JNZ : AIC_S_JZ; /* switch zero non zero */
	s->s_format3.f3_opcode = AIC_S_TST + $1;
	s->s_format3.f3_source = AIC_FLAGS;
	s->s_format3.f3_immediate =  02;	/* zero bit */
	s->s_format3.f3_address = $2;
	s->s_format3.f3_format3 = 1;
	location_counter++;
	};


F1_OPCODE:	T_OR | T_AND |T_XOR  | T_ADD | T_ADDC

F2_OPCODE:	T_SR | T_SL | T_RR | T_RL

JZorJNZ:	T_JZ | T_JNZ
JSorJNS:	T_JS | T_JNS
JEorJNE:	T_JE | T_JNE


ADDRESS:	expression

SOURCE:		ROREG | T_AC | T_RWREG | SCBorSRAM;

PSOURCE:	T_RWREG | ROREG | SCBorSRAM;

DESTINATION:
		T_RWREG { $$ = $1; wod = 0; };
|		T_AC { $$ = $1; wod = 0; };
|		WOREG { $$ = $1; wod = 1; }; 
|		SCBorSRAM { $$ = $1; wod = 0; }

SCBorSRAM:	T_SRAM
|		T_SCB
|		T_SRAM expression	{$$ = $1 + $2;}
|		T_SCB expression	{$$ = $1 + $2;};

ROREG:		T_ROREG

WOREG:		T_WOREG

ANYADDR:	DESTINATION | T_ROREG;

ADDRESSOF:	T_AMPERSAND  ANYADDR { $$ = $2; };

IMMEDIATE: expression
	{
	    if ($1 > 0xff)
		fprintf(stderr, "immediate too large line %d\n", yyline);
	    if (($1 & 0xff) == 0) 
		$$ = 0x100;	/* flag to indicate zero */
	    else
		$$ = $1 & 0xff;
	};
| 	T_AC { $$ = 0; }
|	ADDRESSOF { $$ = $1; };


C_SOURCE:
	{$$ = -1;}
|	T_COMMA  SOURCE	{ $$ = $2;};

C_RETURN:
	{$$ = 0;}
|	T_RETURN		{$$ = 1;};
%%
