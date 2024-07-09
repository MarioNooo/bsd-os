%{
/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI scan.l,v 1.1 1995/08/17 15:58:14 cp Exp
 */

#include <stdlib.h>
#include <string.h>
#include "y.tab.h"

int	yyline;
char	*yyfile;


#define	yywrap() 1

%}

PATH		[-/A-Za-z0-9_.]*[./][-/A-Za-z0-9_.]*
WORD		[A-Za-z_][-A-Za-z_0-9]*
S		[ \t]+
NL		\n
SEMICOLON	\;
COMMA		\,
EQUAL		\=
ANSI		ansi|ansi{S}t1\.403
TX		c\/r{S}address{S}bit{S}of{S}transmitted{S}prms|tx
RX		c\/r{S}address{S}bit{S}of{S}received{S}prms|rx
MG		messages|messages{S}generated
AT		att|at&t
ATT		{AT}|{AT}{S}54016
TAM		target{S}address{S}match|tam


%%


off				{ yylval.value = 0; return T_ON;	}
on				{ yylval.value = 1; return T_ON;	}

no				{ yylval.value = 0; return T_YES;	}
yes				{ yylval.value = 1; return T_YES;	}

a{S}or{S}z			{ yylval.value = 0; return T_BY;	}
aorz				{ yylval.value = 0; return T_BY;	}
b{S}or{S}y			{ yylval.value = 1; return T_BY;	}
bory				{ yylval.value = 1; return T_BY;	}

flag				{ yylval.value = 0; return T_MARK;	}
mark				{ yylval.value = 1; return T_MARK;	}


normal				{ yylval.value = 0; return T_INVERT;	}
invert				{ yylval.value = 1; return T_INVERT;	}

external			{ yylval.value = 0; return T_INTERNAL;	}
internal			{ yylval.value = 1; return T_INTERNAL;	}

transmit{S}clock		{ yylval.value = 0; return T_RX_CLK;	}
tx{S}clock			{ yylval.value = 0; return T_RX_CLK;	}
receive{S}clock			{ yylval.value = 1; return T_RX_CLK;	}
rx{S}clock			{ yylval.value = 1; return T_RX_CLK;	}

notify				{ yylval.value = 0; return T_SILENT;	}
silent				{ yylval.value = 1; return T_SILENT;	}
quiet				{ yylval.value = 1; return T_SILENT;	}

b8zs				{ yylval.value = 0; return T_AMI;	}
ami				{ yylval.value = 1; return T_AMI;	}

sf				{ yylval.value = 0; return T_ESF;	}
d4				{ yylval.value = 0; return T_ESF;	}
esf				{ yylval.value = 1; return T_ESF;	}

use				{ yylval.value = 0; return T_IGNORE;	}
use{S}dcd			{ yylval.value = 0; return T_IGNORE;	}
ignore				{ yylval.value = 1; return T_IGNORE;	}
ignore{S}dcd			{ yylval.value = 1; return T_IGNORE;	}


active{S}channels		{ return T_CHANNELS;			}
channels			{ return T_CHANNELS;			}
all				{ return T_ALL;				}

clocking			{ return T_CLOCKING;			}
clock{S}source			{ return T_CLOCKING;			}
clock				{ return T_CLOCKING;			}
line{S}coding			{ return T_LINECODE;			}
line{S}code			{ return T_LINECODE;			}
rate{S}multiplier		{ return T_RATEMULTIPLIER;		}
rate				{ return T_RATEMULTIPLIER;		}
frame{S}mode			{ return T_FRAMING;			}
framing{S}mode			{ return T_FRAMING;			}
egl				{ return T_EGL;				}
equalizer{S}gain{S}limit	{ return T_EGL;				}
lbo				{ return T_LBO; 			}
line{S}build{S}out		{ return T_LBO; 			}
csu{S}data{S}polarity		{ return T_POLARITY;			}
db				{ return T_DB;				}
kb				{ return T_KB;				}
dcd				{ return T_DCD;				}
data{S}carrier{S}detect		{ return T_DCD;				}
receive{S}data{S}errors		{ return T_RX_ERRORS;			}
transmission{S}frequency	{ return T_TRANSMISSIONFREQ;		}
receive{S}errors		{ return T_RX_ERRORS;			}
rx{S}data{S}errors		{ return T_RX_ERRORS;			}
rx{S}errors			{ return T_RX_ERRORS;			}
tx{S}clock{S}source		{ return T_TX_SOURCE;			}
transmit{S}clock{S}source	{ return T_TX_SOURCE;			}
baud{S}rate{S}table{S}entry	{ return T_BAUD_RATE_TABLE_ENTRY;	}
source{S}clock			{ return T_SOURCE_CLOCK;		}

{ANSI}{S}{MG}			{ return T_ANSI_GENERATE;		}
{ANSI}{S}{TX}			{ return T_ANSI_TRANSMIT;		}
{ANSI}{S}{RX}			{ return T_ANSI_RECEIVE;		}
{ATT}{S}{TAM}			{ return T_ATT_54016;			}
{ATT}				{ return T_ATT_54016;			}
fdl{S}idle{S}code		{ return T_IDLE_CODE;			}

{SEMICOLON} 		{ return T_EOS;				}
{COMMA}		 	{ return T_COMMA;			}
{EQUAL}			{ return T_EQUAL;			}



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


{NL} {
	yyline++;
}

#.* {
	/* comment */
}

{S} {
	/* space */
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

	yyline = 1;
	if (fname == NULL) {
		yyfile = strdup("stdin");
		yyin = stdin;
		return;
	}


	if ((yyin = fopen(fname, "r")) == NULL)
		err(1, "open of %s said", fname);
	yyfile = strdup (fname);
	return (0);
}
