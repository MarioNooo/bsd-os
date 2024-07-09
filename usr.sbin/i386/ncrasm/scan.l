%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI scan.l,v 1.3 1999/12/09 18:41:32 cp Exp
 */

#include <stdlib.h>
#include <string.h>
#include <dev/pci/ncrreg.h>
#include "symbol.h"
#include "y.tab.h"
#include "script.h"

int	yyline;


#define	yywrap() 1

%}

PATH	[-/A-Za-z0-9_.]*[./][-/A-Za-z0-9_.]*
WORD	[A-Za-z_][-A-Za-z_0-9]*
SPC	[ \t]+
NL	\n
END	{NL}|{SPC}|\;

%%


ld/{END}		{ yylval.value = NCR_RW_LD;	return T_LD;	}
add/{END} 		{ yylval.value = NCR_RW_ADD;	return T_ADD;	}
and/{END}		{ yylval.value = NCR_RW_AND;	return T_AND;	}
or/{END}		{ yylval.value = NCR_RW_OR;	return T_OR;	}
shiftl/{END}		{ yylval.value = NCR_RW_LD;	return T_SHFTL; }

blkmv/{END}		{ return T_BLKMV;	}
memmv/{END}		{ return T_MEMMV;	}
r/{END}			{ return T_R;		}

not/{END}		{ return T_NOT;		} 	

always/{END}		{ yylval.value = 1;		return T_TC_FORCE;}
never/{END}		{ yylval.value = 0;		return T_TC_FORCE;}

carry/{END}		{ return T_CARRY;	} 	
target/{END}		{ return T_TARGET;	}
ack/{END}		{ return T_ACK;		}
atn/{END}		{ return T_ATN;		}
fly/{END}		{ return T_FLY;		}
mv/{END}		{ return T_MV;		}

data_in/{END}	{ yylval.value = NCR_PHASE_DI;		return T_PHASE;}
data_out/{END}	{ yylval.value = NCR_PHASE_DO;		return T_PHASE;}
cmd/{END}	{ yylval.value = NCR_PHASE_CMD;		return T_PHASE;}
msg_in/{END}	{ yylval.value = NCR_PHASE_MI;		return T_PHASE;}
msg_out/{END}	{ yylval.value = NCR_PHASE_MO;		return T_PHASE;}
status/{END}	{ yylval.value = NCR_PHASE_STATUS;	return T_PHASE;}

jmp/{END}	{ yylval.value = NCR_TC_JUMP;		return T_JUMP;}
call/{END}	{ yylval.value = NCR_TC_CALL;		return T_CALL;}
ret/{END}	{ yylval.value = NCR_TC_RETURN;		return T_RETURN;}
int/{END}	{ yylval.value = NCR_TC_INTERRUPT;	return T_INTERRUPT;}


ti/{END}		{ return T_TI;		}
i/{END}			{ return T_I;		}

select/{END}	    { yylval.value = NCR_IO_SELECT;	return T_SELECT;}
reselect/{END}	    { yylval.value = NCR_IO_RESELECT;	return T_RESELECT;}
disconnect/{END}    { yylval.value = NCR_IO_DISCONNECT;	return T_DISCONNECT;}
set/{END}	    { yylval.value = NCR_IO_SET;	return T_SET;}
clear/{END}	    { yylval.value = NCR_IO_CLEAR;	return T_CLEAR;}

\./{END}	    { yylval.value = location_counter;	return T_DOT;}


data/{END}			{ return T_DATA;	}
mask/{END}			{ return T_MASK;	}
wait/{END}			{ return T_WAIT;	}


sfbr/{END}	{yylval.value = SFBR;		return T_SFBR;}		

scntl0/{END}	{yylval.value = SCNTL0;		return T_REGISTER;}		
scntl1/{END}	{yylval.value = SCNTL1;		return T_REGISTER;}		
scntl2/{END}	{yylval.value = SCNTL2;		return T_REGISTER;}		
scntl3/{END}	{yylval.value = SCNTL3;		return T_REGISTER;}		
scid/{END}	{yylval.value = SCID;		return T_REGISTER;}		
sxfer/{END}	{yylval.value = SXFER;		return T_REGISTER;}		
sdid/{END}	{yylval.value = SDID;		return T_REGISTER;}		
gpreg/{END}	{yylval.value = GPREG;		return T_REGISTER;}		
socl/{END}	{yylval.value = SOCL;		return T_REGISTER;}		
ssid/{END}	{yylval.value = SSID;		return T_REGISTER;}		
sbcl/{END}	{yylval.value = SBCL;		return T_REGISTER;}		
dstat/{END}	{yylval.value = DSTAT;		return T_REGISTER;}		
sstat0/{END}	{yylval.value = SSTAT0;		return T_REGISTER;}		
sstat1/{END}	{yylval.value = SSTAT1;		return T_REGISTER;}		
sstat2/{END}	{yylval.value = SSTAT2;		return T_REGISTER;}		
dsa/{END}	{yylval.value = DSA;		return T_REGISTER;}		
dsa0/{END}	{yylval.value = DSA + 0;	return T_REGISTER;}		
dsa1/{END}	{yylval.value = DSA + 1;	return T_REGISTER;}		
dsa2/{END}	{yylval.value = DSA + 2;	return T_REGISTER;}		
dsa3/{END}	{yylval.value = DSA + 3;	return T_REGISTER;}		
istat/{END}	{yylval.value = ISTAT;		return T_REGISTER;}		
ctest0/{END}	{yylval.value = CTEST0;		return T_REGISTER;}		
ctest1/{END}	{yylval.value = CTEST1;		return T_REGISTER;}		
ctest2/{END}	{yylval.value = CTEST2;		return T_REGISTER;}		
ctest3/{END}	{yylval.value = CTEST3;		return T_REGISTER;}		
temp/{END}	{yylval.value = TEMP;		return T_REGISTER;}		
temp0/{END}	{yylval.value = TEMP + 0;	return T_REGISTER;}		
temp1/{END}	{yylval.value = TEMP + 1;	return T_REGISTER;}		
temp2/{END}	{yylval.value = TEMP + 2;	return T_REGISTER;}		
temp3/{END}	{yylval.value = TEMP + 3;	return T_REGISTER;}		
dfifo/{END}	{yylval.value = DFIFO;		return T_REGISTER;}		
ctest4/{END}	{yylval.value = CTEST4;		return T_REGISTER;}		
ctest5/{END}	{yylval.value = CTEST5;		return T_REGISTER;}		
ctest6/{END}	{yylval.value = CTEST6;		return T_REGISTER;}		
dbc/{END}	{yylval.value = DBC;		return T_REGISTER;}		
dbc0/{END}	{yylval.value = DBC + 0;	return T_REGISTER;}		
dbc1/{END}	{yylval.value = DBC + 1;	return T_REGISTER;}		
dbc2/{END}	{yylval.value = DBC + 2;	return T_REGISTER;}		
dcmd/{END}	{yylval.value = DCMD;		return T_REGISTER;}		
dnad/{END}	{yylval.value = DNAD;		return T_REGISTER;}		
dnad0/{END}	{yylval.value = DNAD + 0;	return T_REGISTER;}		
dnad1/{END}	{yylval.value = DNAD + 1;	return T_REGISTER;}		
dnad2/{END}	{yylval.value = DNAD + 2;	return T_REGISTER;}		
dnad3/{END}	{yylval.value = DNAD + 3;	return T_REGISTER;}		
dsp/{END}	{yylval.value = DSP;		return T_REGISTER;}		
dsp0/{END}	{yylval.value = DSP + 0;	return T_REGISTER;}		
dsp1/{END}	{yylval.value = DSP + 1;	return T_REGISTER;}		
dsp2/{END}	{yylval.value = DSP + 2;	return T_REGISTER;}		
dsp3/{END}	{yylval.value = DSP + 3;	return T_REGISTER;}		
dsps/{END}	{yylval.value = DSP;		return T_REGISTER;}		
dsps0/{END}	{yylval.value = DSPS + 0;	return T_REGISTER;}		
dsps1/{END}	{yylval.value = DSPS + 1;	return T_REGISTER;}		
dsps2/{END}	{yylval.value = DSPS + 2;	return T_REGISTER;}		
dsps3/{END}	{yylval.value = DSPS + 3;	return T_REGISTER;}		
scratcha/{END}	{yylval.value = SCRATCHA;	return T_REGISTER;}		
scratcha0/{END}	{yylval.value = SCRATCHA + 0;	return T_REGISTER;}		
scratcha1/{END}	{yylval.value = SCRATCHA + 1;	return T_REGISTER;}		
scratcha2/{END}	{yylval.value = SCRATCHA + 2;	return T_REGISTER;}		
scratcha3/{END}	{yylval.value = SCRATCHA + 3;	return T_REGISTER;}		
dmode/{END}	{yylval.value = DMODE;		return T_REGISTER;}		
dien/{END}	{yylval.value = DIEN;		return T_REGISTER;}		
dwt/{END}	{yylval.value = DWT;		return T_REGISTER;}		
dcntl/{END}	{yylval.value = DCNTL;		return T_REGISTER;}		
adder/{END}	{yylval.value = ADDER;		return T_REGISTER;}		
adder0/{END}	{yylval.value = ADDER + 0;	return T_REGISTER;}		
adder1/{END}	{yylval.value = ADDER + 0;	return T_REGISTER;}		
adder2/{END}	{yylval.value = ADDER + 0;	return T_REGISTER;}		
adder3/{END}	{yylval.value = ADDER + 0;	return T_REGISTER;}		
sien0/{END}	{yylval.value = SIEN0;		return T_REGISTER;}		
sien1/{END}	{yylval.value = SIEN1;		return T_REGISTER;}		
sist0/{END}	{yylval.value = SIST0;		return T_REGISTER;}		
sist1/{END}	{yylval.value = SIST1;		return T_REGISTER;}		
slpar/{END}	{yylval.value = SLPAR;		return T_REGISTER;}		
swide/{END}	{yylval.value = SWIDE;		return T_REGISTER;}		
macntl/{END}	{yylval.value = MACNTL;		return T_REGISTER;}		
gpcntl/{END}	{yylval.value = GPCNTL;		return T_REGISTER;}		
stime0/{END}	{yylval.value = STIME0;		return T_REGISTER;}		
stime1/{END}	{yylval.value = STIME1;		return T_REGISTER;}		
respid0/{END}	{yylval.value = RESPID0;	return T_REGISTER;}		
respid1/{END}	{yylval.value = RESPID1;	return T_REGISTER;}		
stest0/{END}	{yylval.value = STEST0;		return T_REGISTER;}		
stest1/{END}	{yylval.value = STEST1;		return T_REGISTER;}		
stest2/{END}	{yylval.value = STEST2;		return T_REGISTER;}		
stest3/{END}	{yylval.value = STEST3;		return T_REGISTER;}		
sidl/{END}	{yylval.value = XXX_SIDL;	return T_REGISTER;}		
sidl0/{END}	{yylval.value = XXX_SIDL + 0;	return T_REGISTER;}		
sidl1/{END}	{yylval.value = XXX_SIDL + 1;	return T_REGISTER;}		
sodl/{END}	{yylval.value = SODL;		return T_REGISTER;}		
sodl0/{END}	{yylval.value = SODL+ 0;	return T_REGISTER;}		
sodl1/{END}	{yylval.value = SODL+ 1;	return T_REGISTER;}		
sbdl/{END}	{yylval.value = SBDL;		return T_REGISTER;}		
sbdl0/{END}	{yylval.value = SBDL + 0;	return T_REGISTER;}		
sbdl1/{END}	{yylval.value = SBDL + 1;	return T_REGISTER;}		
scratchb/{END}	{yylval.value = SCRATCHB;	return T_REGISTER;}		
scratchb0/{END}	{yylval.value = SCRATCHB + 0;	return T_REGISTER;}		
scratchb1/{END}	{yylval.value = SCRATCHB + 1;	return T_REGISTER;}		
scratchb2/{END}	{yylval.value = SCRATCHB + 2;	return T_REGISTER;}		
scratchb3/{END}	{yylval.value = SCRATCHB + 3;	return T_REGISTER;}		

id[0-7] {
	yylval.value = strtol(yytext+2, NULL, 10);
	return T_SCSI_ID;
}

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
	yylval.symbol = getsym(yytext);
	return T_SYMBOL;
}

{WORD}/: {
	yylval.symbol = getsym(yytext);
	(void)input();	/* eat colon */
	return T_TAG;
}


\; {
	return T_EOS;
}

{NL} {
	yyline++;
}

#.* {
	/* ignored (comment) */;
}

{SPC} {
}

. {
	return yytext[0];
}

<<EOF>> {
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
	return (0);
}

int
rewindfile()
{
	fseek(yyin, 0, SEEK_SET);
	yyline = 1;
}
