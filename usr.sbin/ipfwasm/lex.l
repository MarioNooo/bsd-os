/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI lex.l,v 1.5 2002/12/20 16:36:11 prb Exp
 */
%{
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/bpf.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include "asm.h"
#include "y.tab.h"
%}

%%

0[0-7]*			{ yylval.k = strtoul(yytext, 0, 8); return NUMBER; }
[1-9][0-9]*		{ yylval.k = strtoul(yytext, 0, 10); return NUMBER; }
0[Xx][0-9A-Fa-f]+	{ yylval.k = strtoul(yytext, 0, 16); return NUMBER; }
Q\#[ \t]*[0-9A-Fa-f:]+	{ char *p = yytext + 2;
			  while (*p == ' ' || *p == '\t')
				++p;
#ifdef	BPF_128
			  if (inet_pton(AF_INET6, p, &yylval.q) == 0) {
				extern int error;
				extern char *srcfile;
				extern int linecnt;
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s: invalid IPv6 address\n",
				    srcfile, linecnt, p);
				error = 1;
			  }
#endif
			  return QNUMBER;
			}

(service|SERVICE)\([-a-zA-Z0-9_]+\/[-a-zA-Z0-9_]+\) {
			struct servent *s;
			char *name = strchr(yytext, '(') + 1;
			char *proto = strchr(name, '/');
			*proto++ = '\0';
			*strchr(proto, ')') = '\0';
			if ((s = getservbyname(name, proto)) != NULL) {
				yylval.k = htons(s->s_port);
			} else {
				extern int error;
				extern char *srcfile;
				extern int linecnt;
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s/%s: no such service\n",
				    srcfile, linecnt, name, proto);
				error = 1;
			}
			return NUMBER;
		}
(host|HOST)\([-a-zA-Z0-9_\.]+\) {
			struct hostent *h;
			char *name = strchr(yytext, '(') + 1;
			*strchr(name, ')') = '\0';
			if ((h = gethostbyname(name)) != NULL &&
			    h->h_addrtype == AF_INET && h->h_length == 4) {
				yylval.k = (h->h_addr[0] << 24) |
					   (h->h_addr[1] << 16) |
					   (h->h_addr[2] << 8) |
					   (h->h_addr[3] << 0);
				return NUMBER;
			}
#ifdef	BPF_128
			if (h != NULL &&
			    h->h_addrtype == AF_INET6 && h->h_length == 16) {
				memcpy(&yylval.q, h->h_addr, 16);
				return QNUMBER;
			}
#endif
			{
				extern int error;
				extern char *srcfile;
				extern int linecnt;
				yylval.k = 0;
				fprintf(stderr,
				    "%s: %d: %s: no such hostname\n",
				    srcfile, linecnt, name);
				error = 1;
			}
			return NUMBER;
		}
(interface|INTERFACE)\([a-zA-Z#]+[0-9]+\) {
			yylval.iface = strchr(yytext, '(') + 1;
			*strchr(yylval.iface, ')') = '\0';
			if (yylval.iface[0] == '#') {
				yylval.k = strtol(yylval.iface+1, 0, 0);
				return NUMBER;
			}
			return IFACE;
		}
(filter|FILTER)\([_a-zA-Z][-._a-zA-Z0-9]*\) {
			yylval.iface = strchr(yytext, '(');
			yylval.iface[0] = '*';
			*strchr(yylval.iface, ')') = '\0';
			if (yylval.iface[1] == '#') {
				yylval.k = strtol(yylval.iface+2, 0, 0);
				return NUMBER;
			}
			return FILTER;
		}
add|ADD			return ADD;
and|AND			return AND;
div|DIV			return DIV;
jeq|JEQ			return JEQ;
jge|JGE			return JGE;
jgt|JGT			return JGT;
jle|JLE			return JLE;
jlt|JLT			return JLT;
jmp|JMP			return JMP;
jne|JNE			return JNE;
jset|JSET		return JSET;
jnet|JNET		return JNET;
ldb|LDB			return LDB;
ldh|LDH			return LDH;
ldx|LDX			return LDX;
ld|LD			return LD;
len|LEN			return LEN;
lsh|LSH			return LSH;
mod|MOD			return MOD;
msh|MSH			return MSH;
mul|MUL			return MUL;
neg|NEG			return NEG;
or|OR			return OR;
ret|RET			return RET;
rsh|RSH			return RSH;
stx|STX			return STX;
st|ST			return ST;
sub|SUB			return SUB;
tax|TAX			return TAX;
time|TIME		return TIME;
txa|TXA			return TXA;
ifn|IFN			return IFN;
zmem|ZMEM		return ZMEM;
call|CALL		return CALL;
ipsrcrt|IPSRCRT		return IPSRCRT;
ifflags|IFFLAGS		return IFFLAGS;

"<<"			return SHL;
">>"			return SHR;

[-~^#&()*+:\[\]\.\n;,\|%\/]	return yytext[0];

a128|A128		return A128;
[alrmxALMRX]		return toupper(yytext[0]);
\/\/.*\n		return '\n';
^#[ \t]+[0-9]+[ \t]+\"[^\"\n]*\".*\n {
			extern char *srcfile;
			extern int linecnt;
			char *p = yytext;
			linecnt = atol(p+1);
			p = strchr(p, '"') + 1;
			*strchr(p, '"') = '\0';
			if (*p)
				srcfile = strdup(p);
			else
				srcfile = "<STDIN>";
		}
^#.*\n			return '\n';
[ \t]+			;
.			return ERR;
%%
