/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI lex.l,v 1.2 1999/11/18 01:26:37 prb Exp
 */
%{
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include "yacc.h"
#include "y.tab.h"
extern int linecnt;
%}

%%

[Pp][Rr][Ee][Ff][Ii][Ll][Ll]		{ return PREFILL; }
[Mm][Aa][Xx][Ss][Ee][Ss][Ss][Ii][Oo][Nn][Ss]		{ return MAXSESSIONS; }
[Bb][Uu][Cc][Kk][Ee][Tt][Ss]		{ return BUCKETS; }
[Ii][Nn][Tt][Ee][Rr][Ff][Aa][Cc][Ee]	{ return INTERFACE; }
[Ii][Nn][Tt][Ee][Rr][Nn][Aa][Ll]	{ return INTERNAL; }
[Ee][Xx][Tt][Ee][Rr][Nn][Aa][Ll]	{ return EXTERNAL; }
[Ss][Ee][Rr][Vv][Ii][Cc][Ee]		{ return SERVICE; }
[Tt][Ii][Mm][Ee][Oo][Uu][Tt]		{ return TIMEOUT; }
[Mm][Aa][Pp]				{ return MAP; }
[Ii][Cc][Mm][Pp]			{ return ICMP; }
[Tt][Cc][Pp]				{ return TCP; }
[Uu][Dd][Pp]				{ return UDP; }
[Tt][Aa][Gg]				{ return TAG; }
[Pp][Rr][Ii][Oo][Rr][Ii][Tt][Yy]	{ return PRIORITY; }
[Ee][Xx][Pp][Ii][Rr][Ee]		{ return EXPIRE; }
[Ii][Nn]				{ return IN; }
[Aa][Tt]				{ return AT; }
"->"					{ return POINTER; }
[-\[\];,./:]				{ return yytext[0]; }
0[Xx][[:xdigit:]]+			{ yylval.n = strtoul(yytext, 0, 16);
					  return NUMBER; }
0[0-7]*					{ yylval.n = strtoul(yytext, 0, 8);
					  return NUMBER; }
[0-9]+					{ yylval.n = strtoul(yytext, 0, 10);
					  return NUMBER; }

#.*\n					++linecnt;
\n					++linecnt;
[ \t]+					;

\"[^\n]+\"				{ yylval.str = strdup(yytext+1);
					  yylval.str[strlen(yylval.str)-1]='\0';
					  return QSTRING; }
[[:alpha:]_][[:alnum:]_]*		{ yylval.str = strdup(yytext); return STRING; }

.					{ if (isprint(yytext[0]))
						yyerror("error at ``%c''", yytext[0]);
					    else
						yyerror("error at 0x%02x", yytext[0]);
					}

%%
