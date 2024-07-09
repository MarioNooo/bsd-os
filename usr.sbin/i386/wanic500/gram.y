%{
/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gram.y,v 1.3 1999/07/29 16:18:30 cp Exp
 */


#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <i386/pci/if_w5ioctl.h>
#include "wanic500.h"
#include <err.h>

stop()
{
	exit (1);
}

extern yyline;
extern char* yyfile;


u_char cb1, cb2, cb3;

void
yyerror(const char *string)
{ 
	fprintf(stderr, "file %s, line %d %s\n", yyfile, yyline, string);
	exit (1);
}


%}

%union {
	char		*str;
	int		 value;
}

%token  <value> T_NUMBER
%token  T_EOF T_EOS T_COMMA
%token	<value> T_REG

%token	<value> T_INVERT T_INTERNAL T_AMI T_ESF T_ON;

%token	T_ANSI_GENERATE T_ANSI_TRANSMIT T_ANSI_RECEIVE T_ATT_54016 T_IDLE_CODE

%token <value>	T_MARK T_BY

%token	T_CHANNELS T_ALL T_CLOCK T_LINECODE T_RATEMULTIPLIER 
%token	T_EGL T_LBO 
%token	T_FRAMING
%token	T_POLARITY 
%token	T_DCD T_RX_ERRORS T_BAUD_RATE 
%token	T_CSU_RESET
%token	T_RESET_DELAY
%token	T_TBUFFERS

%token	<value> T_YES T_SILENT T_IGNORE

%token	T_DB T_KB T_MB T_EQUAL

%token	T_CLOCK
%token	<value> T_CM_IS

%type	<value> C_EQUAL C_KB C_MB C_DB C_DOT5
%type	<value> YES_ON

%%


file:	line
|	file line
;

line:	T_RX_ERRORS C_EQUAL T_SILENT T_EOS;
	{
	w5.w5_rxquiet = $3;
	warnx("notify/silent obsolete use \"debug\" parameter to ifconfig(8)");
	};

line:	T_DCD C_EQUAL T_IGNORE T_EOS;
	{
	w5.w5_nodcd = $3;
	};

line:	T_BAUD_RATE C_EQUAL T_NUMBER C_MB C_KB T_EOS;
	{
	verify(IS_V35);
	w5.w5_baudrate = $3 * $4 * $5;
	};

line:	T_CLOCK C_EQUAL T_CM_IS T_EOS;
	{
	w5.w5_clockmode = $3;
	}

line:	T_POLARITY C_EQUAL T_INVERT T_EOS;
	{
	verify(IS_CSU);
	w5.w5_invertdata = $3;
	};

line:	T_CHANNELS C_EQUAL CNUMBERS T_EOS;
	{
	verify(IS_CSU);
	w5.w5_cb[0] = cb1; 
	w5.w5_cb[1] = cb2; 
	w5.w5_cb[2] = cb3; 
	cb1 = 0;
	cb2 = 0;
	cb3 = 0;
	}


CNUMBERS:	T_ALL;
	{
	cb1 = 0xff;
	cb2 = 0xff;
	cb3 = 0xff;
	}
|	CNUMBER 
|	CNUMBERS T_COMMA CNUMBER

CNUMBER:	T_NUMBER;
	{
	if ($1 <= 0 || $1 > 24)
		errx(1, "legal channels 1 to 24");
	if ($1 < 9)
		cb1 |= 1 << ($1 - 1);
	else if ($1 < 17)
		cb2 |= 1 << ($1 - 9);
	else
		cb3 |= 1 << ($1 - 17);
	}

line:	T_CSU_RESET C_EQUAL YES_ON T_EOS;
	{
	verify(IS_CSU);
	w5.w5_csu_reset = $3;
	if (w5.w5_reset_delay == 0)
		w5.w5_reset_delay = 10;
		
	};

line:	T_RESET_DELAY C_EQUAL T_NUMBER T_EOS;
	{
	verify(IS_CSU);
	w5.w5_reset_delay = $3;
	};

line:	T_TBUFFERS C_EQUAL T_NUMBER T_EOS;
	{
	if ($3 < 1)
		yyerror("transmit packets must be greater than 0");
	w5.w5_tpackets = $3;
	}

line:	T_ANSI_GENERATE C_EQUAL YES_ON T_EOS;
	{
	verify(IS_CSU);
	w5.w5_ansi_gen = $3;
	};

line:	T_ANSI_TRANSMIT C_EQUAL YES_ON T_EOS;
	{
	verify(IS_CSU);
	w5.w5_ansi_tx = $3;
	};

line:	T_ANSI_RECEIVE C_EQUAL YES_ON T_EOS;
	{
	verify(IS_CSU);
	w5.w5_ansi_rx = $3;
	};

line:	T_ATT_54016 C_EQUAL T_BY T_EOS;
	{
	verify(IS_CSU);
	w5.w5_att = $3;
	};

line:	T_IDLE_CODE C_EQUAL T_MARK T_EOS;
	{
	verify(IS_CSU);
	w5.w5_att = $3;
	};

line:	T_CLOCK C_EQUAL T_INTERNAL T_EOS;
	{
	verify(IS_CSU);
	w5.w5_intclk = $3;
	};

line:	T_FRAMING C_EQUAL T_ESF T_EOS;
	{
	verify(IS_CSU);
	w5.w5_esf = $3;
	};

line:	T_EGL C_EQUAL YES_ON T_EOS;
	{
	verify(IS_CSU);
	w5.w5_egl = $3;
	};

line:	T_LBO C_EQUAL T_NUMBER C_DOT5 C_DB T_EOS;
	{
	    verify(IS_CSU);
	    switch ($3) {
	    case 0:
	    case 15:
		if ($4 == 0) 
		    break;
		fprintf(stderr, "lbo assuming %d.0\n", $3);
		break;
		break;
	    case 7:
	    case 22:
		if ($4 == 5) 
		    break;
		fprintf(stderr, "lbo assuming %d.5\n", $3);
		break;
	    default:
		errx(1, "legal lbo values 0, 7.5, 15, 22.5\n");
	    }
	w5.w5_lbo = $3;
	};

line:	T_LINECODE C_EQUAL T_AMI T_EOS;
	{
	verify(IS_CSU);
	w5.w5_ami = $3;
	};

/*
 * set up rate multipler
 */
line:	T_RATEMULTIPLIER C_EQUAL T_NUMBER C_KB T_EOS;
	{
	verify(IS_CSU);
	if ($3 == 56)
		w5.w5_rm56 = 1;
	else if ($3 == 64)
		w5.w5_rm56 = 0;
	else
		errx(1, "argument to rate multiplier must be 56 or 64\n");
	};

line:	T_REG C_EQUAL T_NUMBER T_EOS;
	{
	w5.w5_reg_v[$1] = $3;
	w5.w5_reg_s[$1] = 1;
	};
	

line:	T_EOF
	{
	return (0);
	};

C_EQUAL:		{ $$ = 0; }
|	 T_EQUAL	{ $$ = 1; };

C_DB:		{ $$ = 0; }
|	 T_DB	{ $$ = 1; };

C_KB:		{ $$ = 1; }
|	 T_KB	{ $$ = 1000; };

C_MB:		{ $$ = 1; }
|	 T_MB	{ $$ = 1000000; };

C_DOT5:			{ $$ = 0; }
|	 '.' T_NUMBER	{ $$ = $2; };

YES_ON:	T_YES |	T_ON;

%%
