/*
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI scan.l,v 2.1 1997/11/13 19:09:23 donn Exp
 */

%option batch
%option nodefault
%option yylineno
%option noyywrap

%{
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include "scsicmd.h"
#include "y.tab.h"
%}

WHITE	[ \t]
DELIM	[:()\n]
NUMBER	0x[0-9a-fA-f]+|[0-9]+
WD	{WHITE}|{DELIM}|#|\+

%x S

%%

#.*\n				/* comment */
{WHITE}|{DELIM}			/* skip spaces and delimiters among keywords */
COMMAND/{WD}			return (COMMAND);
CDB/{WD}			return (BUFFER);
BUFFER/{WD}			return (BUFFER);
BYTE(S)?/{WD}			return (BYTE);
BIT(S)?/{WD}			return (BIT);
BITLENGTH/{WD}			return (LENGTH);
\+				return ('+');
CMD_READ/{WD}			yylval.cmdtype = CMD_READ; return (CMDTYPE);
CMD_WRITE/{WD}			yylval.cmdtype = CMD_WRITE; return (CMDTYPE);
CMD_READ_VAR/{WD}		yylval.cmdtype = CMD_READ_VAR; return (CMDTYPE);
CMD_READ_DATA/{WD}		yylval.cmdtype = CMD_READ_DATA; return (CMDTYPE);
CMD_SELECT/{WD}			yylval.cmdtype = CMD_SELECT; return (CMDTYPE);
CMD_SENSE/{WD}			yylval.cmdtype = CMD_SENSE; return (CMDTYPE);
CMD_RS/{WD}			yylval.cmdtype = CMD_RS; return (CMDTYPE);
FORMAT_DEFAULT/{WD}		yylval.format = FORMAT_DEFAULT; return (FORMAT);
FORMAT_CODE/{WD}		yylval.format = FORMAT_CODE; return (FORMAT);
FORMAT_STRING/{WD}		yylval.format = FORMAT_STRING; return (FORMAT);
{NUMBER}{WHITE}*"-"{WHITE}*{NUMBER}	{
					yylval.range = scan_range();
					return (RANGE);
				}
{NUMBER}			{
					yylval.number = strtoul(yytext, 0, 0);
					return (NUMBER);
				}
.				BEGIN S; yymore();
<S>./{WHITE}*{DELIM}		{
					yylval.string = strdup(yytext);
					BEGIN 0;
					return (STRING);
				}
<S>.				yymore();

%%

struct range *
scan_range()
{
	int low, high;
	char *end;

	low = strtoul(yytext, &end, 0);
	end += strspn(end, " \t-");
	high = strtoul(end, 0, 0);

	return (add_range(low, high));
}
