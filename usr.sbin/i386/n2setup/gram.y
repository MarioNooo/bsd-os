%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gram.y,v 1.3 1997/07/24 13:50:32 cp Exp
 */


#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "n2setup.h"
#include <err.h>

stop()
{
	exit (1);
}

int	yylex __P((void));

extern yyline;
extern char* yyfile;


extern n2_setupinfo_t si;

u_char cb1, cb2, cb3;

void
yyerror(const char *string)
{ 
	fprintf(stderr, "file %s, line %d %s\n", yyfile, yyline, string);
	exit (1);
}

extern void check4csu();
extern void check4dds();
extern void check4std();
extern void check4csu_or_dds();


%}

%union {
	char		*str;
	int		 value;
}

%token  <value> T_NUMBER
%token  T_EOF T_EOS T_COMMA

%token	<value> T_INVERT T_INTERNAL T_AMI T_ESF T_ON;

%token	T_ANSI_GENERATE T_ANSI_TRANSMIT T_ANSI_RECEIVE T_ATT_54016 T_IDLE_CODE
%token	T_TRANSMISSIONFREQ

%token <value>	T_MARK T_BY

%token	T_CHANNELS T_ALL T_CLOCKING T_LINECODE T_RATEMULTIPLIER 
%token	T_EGL T_LBO 
%token	T_FRAMING
%token	T_POLARITY 
%token	T_DCD T_RX_ERRORS T_TX_SOURCE T_BAUD_RATE_TABLE_ENTRY T_SOURCE_CLOCK

%token	<value> T_YES T_RX_CLK T_SILENT T_IGNORE

%token	T_DB T_KB T_EQUAL

%type	<value> C_EQUAL C_KB C_DB C_DOT5
%type	<value> YES_ON

%%


file:	line
|	file line
;

line:	T_RX_ERRORS C_EQUAL T_SILENT T_EOS;
	{
	si.si_rxquiet = $3;
	warnx("notify/silent obsolete use \"debug\" parameter to ifconfig(8)");
	};

line:	T_DCD C_EQUAL T_IGNORE T_EOS;
	{
	si.si_nodcd = $3;
	};

line:	T_SOURCE_CLOCK C_EQUAL YES_ON T_EOS;
	{
	check4std;
	si.si_sourceclock = $3;
	};

line:	T_TX_SOURCE C_EQUAL T_RX_CLK T_EOS;
	{
	check4std();
	si.si_rxclk2txclk = $3;
	};

line:	T_BAUD_RATE_TABLE_ENTRY C_EQUAL T_NUMBER T_EOS;
	{
	si.si_baud_rate_table_entry = $3;
	};

line:	T_POLARITY C_EQUAL T_INVERT T_EOS;
	{
	check4csu();
	si.si_invertdata = $3;
	};

line:	T_CHANNELS C_EQUAL CNUMBERS T_EOS;
	{
	check4csu();
	si.si_cb[0] = cb1; 
	si.si_cb[1] = cb2; 
	si.si_cb[2] = cb3; 
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
|	NUMBER 
|	CNUMBERS T_COMMA NUMBER

NUMBER:	T_NUMBER;
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

line:	T_ANSI_GENERATE C_EQUAL YES_ON T_EOS;
	{
	check4csu();
	si.si_ansi_gen = $3;
	};

line:	T_ANSI_TRANSMIT C_EQUAL YES_ON T_EOS;
	{
	check4csu();
	si.si_ansi_tx = $3;
	};

line:	T_ANSI_RECEIVE C_EQUAL YES_ON T_EOS;
	{
	check4csu();
	si.si_ansi_rx = $3;
	};

line:	T_ATT_54016 C_EQUAL T_BY T_EOS;
	{
	check4csu();
	si.si_att = $3;
	};

line:	T_IDLE_CODE C_EQUAL T_MARK T_EOS;
	{
	check4csu();
	si.si_att = $3;
	};

line:	T_CLOCKING C_EQUAL T_INTERNAL T_EOS;
	{
	check4csu_or_dds();
	si.si_intclk = $3;
	};

line:	T_FRAMING C_EQUAL T_ESF T_EOS;
	{
	check4csu();
	si.si_esf = $3;
	};

line:	T_EGL C_EQUAL YES_ON T_EOS;
	{
	check4csu();
	si.si_egl = $3;
	};

line:	T_LBO C_EQUAL T_NUMBER C_DOT5 C_DB T_EOS;
	{
	    check4csu();
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
	si.si_lbo = $3;
	};

line:	T_LINECODE C_EQUAL T_AMI T_EOS;
	{
	check4csu();
	si.si_ami = $3;
	};

/*
 * set up rate multipler
 */
line:	T_RATEMULTIPLIER C_EQUAL T_NUMBER C_KB T_EOS;
	{
	check4csu();
	if ($3 == 56)
		si.si_rm56 = 1;
	else if ($3 == 64)
		si.si_rm56 = 0;
	else
		errx(1, "argument to rate multiplier must be 56 or 64\n");
	};

/*
 * set up transmition frequency
 */
line:	T_TRANSMISSIONFREQ C_EQUAL T_NUMBER C_KB T_EOS;
	{
	check4dds();
	if ($3 == 56)
		si.si_tf72 = 0;
	else if ($3 == 72)
		si.si_tf72 = 1;
	else
		errx(1, "argument to transmission frquency be 56 or 72\n");
	};

line:	T_EOF
	{
	return (0);
	};

C_EQUAL:		{ $$ = 0; }
|	 T_EQUAL	{ $$ = 1; };

C_DB:		{ $$ = 0; }
|	 T_DB	{ $$ = 1; };

C_KB:		{ $$ = 0; }
|	 T_KB	{ $$ = 1; };

C_DOT5:			{ $$ = 0; }
|	 '.' T_NUMBER	{ $$ = $2; };

YES_ON:	T_YES |	T_ON;

%%
