/*	BSDI mgram.y,v 2.2 2001/12/14 21:38:04 prb Exp	*/

%{

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)gram.y	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sem.h"

#define	FORMAT(n) ((n) > -10 && (n) < 10 ? "%d" : "0x%x")

#define	stop(s)	error(s), exit(1)

int	include __P((const char *, int));
void	yyerror __P((const char *));
int	yylex __P((void));
extern const char *lastfile;
extern const char *srctok;
extern const char *yyfile;
extern int yyline;

/* the following is used to recover nvlist space after errors */
static	struct	nvlist *alloc[1000];
static	int	adepth;
#define	new0(n,s,p,i,x)	(alloc[adepth++] = newnvx(n, s, p, i, x))
#define	new_n(n)	new0(n, NULL, NULL, 0, NULL)
#define	new_nx(n, x)	new0(n, NULL, NULL, 0, x)
#define	new_ns(n, s)	new0(n, s, NULL, 0, NULL)
#define	new_si(s, i)	new0(NULL, s, NULL, i, NULL)
#define	new_nsi(n,s,i)	new0(n, s, NULL, i, NULL)
#define	new_s(s)	new0(NULL, s, NULL, 0, NULL)
#define	new_p(p)	new0(NULL, NULL, p, 0, NULL)
#define	new_px(p, x)	new0(NULL, NULL, p, 0, x)

#define	fx_atom(s)	new0(s, NULL, NULL, FX_ATOM, NULL)
#define	fx_not(e)	new0(NULL, NULL, NULL, FX_NOT, e)
#define	fx_and(e1, e2)	new0(NULL, NULL, e1, FX_AND, e2)
#define	fx_or(e1, e2)	new0(NULL, NULL, e1, FX_OR, e2)

static	void	cleanup __P((void));

%}

%union {
	struct	attr *attr;
	struct	devbase *devb;
	struct	nvlist *list;
	const char *str;
	int	val;
}

%token	AND AT COMPILE_WITH CONFIG DEFINE DEVICE DUMPS ENDFILE
%token	XFILE XFILES FLAGS INCLUDE XMACHINE MAJOR MAKEOPTIONS MAXUSERS MINOR
%token	OBJPATH ON OPTIONS PSEUDO_DEVICE REQUIRE ROOT SWAP SYSTREE TIMEZONE
%token  VECTOR
%token	<val> FFLAG NUMBER
%token	<str> PATHNAME WORD

%type	<list>	fopts fexpr fatom
%type	<val>	fflgs
%type	<str>	rule
%type	<attr>	attr
%type	<devb>	devbase
%type	<list>	atlist interface_opt
%type	<str>	atname
%type	<list>	loclist_opt loclist locdef
%type	<str>	locdefault
%type	<list>	veclist_opt veclist
%type	<list>	attrs_opt attrs
%type	<str>	value
%type	<val>	signed_number

%left	'|'
%left	'&'
%left	'!'

%%

Files:
	dev_defs ENDFILE		= { YYACCEPT; };

/*
 * Various nonterminals shared between the grammars.
 */
variable:
	REQUIRE WORD			= { require($2); } |
	WORD '=' PATHNAME 		= { assign($1, $3, 0); } |
	WORD '=' WORD 			= { assign($1, $3, 0); } ;

file:
	XFILE PATHNAME fopts fflgs rule	= { addfile($2, $3, $4, $5, yyfile); };

fopts:
	fexpr				= { $$ = $1; } |
	/* empty */			= { $$ = NULL; };

fexpr:
	fatom 				= { $$ = $1; } |
	'!' fatom			= { $$ = fx_not($2); } |
	fexpr '&' fexpr			= { $$ = fx_and($1, $3); } |
	fexpr '|' fexpr			= { $$ = fx_or($1, $3); } |
	'(' fexpr ')'			= { $$ = $2; };

fatom:
	WORD				= { $$ = fx_atom($1); };

fflgs:
	fflgs FFLAG			= { $$ = $1 | $2; } |
	/* empty */			= { $$ = 0; };

rule:
	COMPILE_WITH WORD		= { $$ = $2; } |
	/* empty */			= { $$ = NULL; };

/*
 * ``files path'' is the same as ``include path'' in a files file
 */
include:
	INCLUDE WORD			= { (void)mm_include(expand($2, yyline), '\n'); } |
	XFILES WORD			= { (void)mm_include(expand($2, yyline), '\n'); }
	INCLUDE PATHNAME		= { (void)mm_include(expand($2, yyline), '\n'); } |
	XFILES PATHNAME			= { (void)mm_include(expand($2, yyline), '\n'); }
	

/*
 * The machine definitions grammar.
 */
dev_defs:
	dev_defs dev_def |
	/* empty */;

dev_def:
	one_def '\n'			= { adepth = 0; } |
	'\n' |
	error '\n'			= { cleanup(); };

one_def:
	variable |
	file |
	include |
	root |
	DEFINE WORD interface_opt	= { (void)defattr($2, $3); } |
	DEVICE devbase AT atlist veclist_opt interface_opt attrs_opt
					= { defdev($2, 0, $4, $5, $6, $7); } |
	MAXUSERS NUMBER NUMBER NUMBER	= { setdefmaxusers($2, $3, $4); } |
	OBJPATH PATHNAME		= { setobjpath($2); } |
	PSEUDO_DEVICE devbase attrs_opt = { defdev($2,1,NULL,NULL,NULL,$3); } |
	MAJOR '{' majorlist '}';

root:
	ROOT WORD			= { expand($2, yyline); setroot($2); } |
	ROOT PATHNAME			= { expand($2, yyline); setroot($2); };

atlist:
	atlist ',' atname		= { $$ = new_nx($3, $1); } |
	atname				= { $$ = new_n($1); };

atname:
	WORD				= { $$ = $1; } |
	ROOT				= { $$ = NULL; };

veclist_opt:
	VECTOR veclist			= { $$ = $2; } |
	/* empty */			= { $$ = NULL; };

/* veclist order matters, must use right recursion */
veclist:
	WORD veclist			= { $$ = new_nx($1, $2); } |
	WORD				= { $$ = new_n($1); };

devbase:
	WORD				= { $$ = getdevbase($1); };

interface_opt:
	'{' loclist_opt '}'		= { $$ = new_nx("", $2); } |
	/* empty */			= { $$ = NULL; };

loclist_opt:
	loclist				= { $$ = $1; } |
	/* empty */			= { $$ = NULL; };

/* loclist order matters, must use right recursion */
loclist:
	locdef ',' loclist		= { ($$ = $1)->nv_next = $3; } |
	locdef				= { $$ = $1; };

/* "[ WORD locdefault ]" syntax may be unnecessary... */
locdef:
	WORD locdefault 		= { $$ = new_nsi($1, $2, 0); } |
	WORD				= { $$ = new_nsi($1, NULL, 0); } |
	'[' WORD locdefault ']'		= { $$ = new_nsi($2, $3, 1); };

locdefault:
	'=' value			= { $$ = $2; };

value:
	WORD				= { $$ = $1; } |
	signed_number			= { char bf[40];
					    (void)sprintf(bf, FORMAT($1), $1);
					    $$ = intern(bf); };

signed_number:
	NUMBER				= { $$ = $1; } |
	'-' NUMBER			= { $$ = -$2; };

attrs_opt:
	':' attrs			= { $$ = $2; } |
	/* empty */			= { $$ = NULL; };

attrs:
	attrs ',' attr			= { $$ = new_px($3, $1); } |
	attr				= { $$ = new_p($1); };

attr:
	WORD				= { $$ = getattr($1); };

majorlist:
	majorlist ',' majordef |
	majordef;

majordef:
	devbase '=' NUMBER		= { setmajor($1, $3); };

%%

void
yyerror(s)
	const char *s;
{

	error("%s", s);
	fflush(stdout);
}

/*
 * Cleanup procedure after syntax error: release any nvlists
 * allocated during parsing the current line.
 */
static void
cleanup()
{
	register struct nvlist **np;
	register int i;

	for (np = alloc, i = adepth; --i >= 0; np++)
		nvmaybefree(*np);
	adepth = 0;
}
