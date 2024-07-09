%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI scan.l,v 1.5 1999/12/09 19:09:47 cp Exp
 */

#include <stdlib.h>
#include <string.h>
#include <i386/isa/aicreg.h>
#include "symbol.h"
#include "y.tab.h"
#include "sequencer.h"

int	yyline;
char	*yyfile;


#define	yywrap() 1

%}

PATH		[-/A-Za-z0-9_.]*[./][-/A-Za-z0-9_.]*
WORD		[A-Za-z_][-A-Za-z_0-9]*
SPC		[ \t]+
NL		\n
COMMA		,
SEMICOLON	\;
AMPERSAND	\&
COLON		\:
DOT		\.
POUND		#
END		{NL}|{SPC}|{COMMA}|{SEMICOLON}|{COLON}

%x CMNT
%%
"/*"	BEGIN CMNT;
<CMNT>.	{};
<CMNT>\n {yyline++;};
<CMNT>"*/"	BEGIN INITIAL;



jz/{END}	{ yylval.value = AIC_S_JZ;	return T_JZ;	}
jnz/{END}	{ yylval.value = AIC_S_JNZ;	return T_JNZ;	}

js/{END}	{ yylval.value = AIC_S_JNZ;	return T_JS;	}
jns/{END}	{ yylval.value = AIC_S_JZ;	return T_JNS;	}

je/{END}	{ yylval.value = AIC_S_JZ;	return T_JE;	}
jne/{END}	{ yylval.value = AIC_S_JNZ;	return T_JNE;	}

rr/{END}	{ yylval.value = AIC_S_ROTATERIGHT;	return T_RR; }
rl/{END}	{ yylval.value = AIC_S_ROTATELEFT;	return T_RL; }
sr/{END}	{ yylval.value = AIC_S_SHIFTRIGHT;	return T_SR; }
sl/{END}	{ yylval.value = AIC_S_SHIFTLEFT;	return T_SL; }


or2si/{END}	{ return T_ORSI; }
ldsi/{END}	{ return T_LDSI; }
ld/{END}	{ return T_LD; }
mv/{END}	{ return T_MV; }
set/{END}	{ return T_SET; }
clr/{END}	{ return T_CLR; }
jmp/{END}	{ yylval.value = AIC_S_JMP;	return T_JMPorCALL; }
call/{END}	{ yylval.value = AIC_S_CALL;	return T_JMPorCALL; }
jc/{END}	{ yylval.value = AIC_S_JC;	return T_JMPorCALL; }
jnc/{END}	{ yylval.value = AIC_S_JNC;	return T_JMPorCALL; }
tst/{END}	{ yylval.value = AIC_S_TST;	return T_TST; }
cmp/{END}	{ yylval.value = AIC_S_CMP;	return T_CMP; }
add/{END}	{ yylval.value = AIC_S_ADD;	return T_ADD; }
and/{END}	{ yylval.value = AIC_S_AND;	return T_AND; }
xor/{END}	{ yylval.value = AIC_S_XOR;	return T_XOR; }
or/{END}	{ yylval.value = AIC_S_OR;	return T_OR; }
addc/{END}	{ yylval.value = AIC_S_ADDC;	return T_ADDC; }

ret/{END}	{ return T_RETURN; }

SECTION/{END}	{ return T_SECTION; }
S_SRAM/{END}	{ yylval.value = AIC_S_SRAM;	return T_SRAM; }
S_SCB/{END}	{ yylval.value = AIC_S_SCB;	return T_SCB; }
S_CODE/{END}	{ yylval.value = AIC_S_CODE;	return T_CODE;}

{DOT}		{ yylval.value = location_counter; return T_DOT; }

0[0-7]*	{
	yylval.value = strtol(yytext, NULL, 8);
	return T_NUMBER;
}

0[xX][0-9a-fA-F]+ {
	yylval.value = strtoul(yytext + 2, NULL, 16);
	return T_NUMBER;
}

[1-9][0-9]* {
	yylval.value = strtol(yytext, NULL, 10);
	return T_NUMBER;
}

{WORD} {
	symbol_t *symbol = getsym(yytext);
	switch (symbol->s_section) {
	case AIC_S_CODE:
		yylval.symbol = symbol;
		return T_SYMBOL;
	case AIC_S_SRAM:
		yylval.value = symbol->s_value + AIC_SRAM;
		return T_SRAM;
	case AIC_S_SCB:
		yylval.value = symbol->s_value + AIC_SCB;
		return T_SCB;
	case AIC_S_REG_RW:
		yylval.value = symbol->s_value;
		return T_RWREG;
	case AIC_S_REG_RO:
		yylval.value = symbol->s_value;
		return T_ROREG;
	case AIC_S_REG_WO:
		yylval.value = symbol->s_value;
		return T_WOREG;
	case AIC_S_REG_AC:
		yylval.value = symbol->s_value;
		return T_AC;
	case AIC_S_REG_HO:
		yylval.value = symbol->s_value;
		return T_HOREG;	/* host only reg */;
	}
}

{PATH} {
	yylval.str = strdup (yytext);
	return T_PATH;
}

{WORD}/:: {
	yylval.symbol = getsym(yytext);
	yylval.symbol->s_etag = 1;
	(void)input();	/* eat colon */
	(void)input();	/* eat colon */
	return T_TAG;
}

{WORD}/: {
	yylval.symbol = getsym(yytext);
	(void)input();	/* eat colon */
	return T_TAG;
}

{AMPERSAND} {
	return T_AMPERSAND;
}

{SEMICOLON} {
	return T_EOS;
}

{COMMA} {
	return T_COMMA;
}

{NL} {
	yyline++;
}

#.* {
	if (strncmp(yytext, "#define", 7) == 0) {
		REJECT;
	}
	if (strncmp(yytext, "#include", 8) == 0) {
		REJECT;
	}
}

"#include"/{END}	{
	return T_INCLUDE;
}

{SPC} {
}

"REG_RW"/{END}	{ yylval.value = AIC_S_REG_RW;	return T_REGISTER; }
"REG_RO"/{END}	{ yylval.value = AIC_S_REG_RO;	return T_REGISTER; }
"REG_WO"/{END}	{ yylval.value = AIC_S_REG_WO;	return T_REGISTER; }
"REG_AC"/{END}	{ yylval.value = AIC_S_REG_AC;	return T_REGISTER; }
"REG_HO"/{END}	{ yylval.value = AIC_S_REG_HO;	return T_REGISTER; }


. {
	return yytext[0];
}

<<EOF>> {
	if (endinclude())
		return T_EOF;
}

%%

/*
 * Open the file).
 */
int
openfile(fname)
	const char *fname;
{

	if ((yyin = fopen(fname, "r")) == NULL)
		return (-1);
	yyline = 1;
	yyfile = strdup (fname);
	return (0);
}

typedef struct incl {
	struct incl	*in_prev;
	YY_BUFFER_STATE	 in_buf;
	int		 in_line;
	char		*in_fname;
} incl_t;

static incl_t *incl;

int
includefile(fname)
	const char *fname;
{
	FILE *fp;
	incl_t	*in;

	if ((fp = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);
	
	in = malloc (sizeof *in);
	in->in_prev = incl;
	in->in_buf = YY_CURRENT_BUFFER;
	in->in_line = yyline;
	in->in_fname = yyfile;
	incl = in;
	yy_switch_to_buffer(yy_create_buffer(fp, YY_BUF_SIZE));
	yyfile = strdup(fname);
	yyline = 1;
	return 0;
}

int
endinclude()
{
	struct incl *in;

	if ((in = incl) == NULL)
		return 1;
	incl = in->in_prev;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	(void)fclose(yyin);
	yy_switch_to_buffer(in->in_buf);
	yyfile = in->in_fname;
	yyline = in->in_line;
	free(in);
	return 0;
}

int
rewindfile()
{
	fseek(yyin, 0, SEEK_SET);
	yyline = 1;
}
