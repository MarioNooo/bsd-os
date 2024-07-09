/*	BSDI gram.y,v 2.10 2002/03/18 20:05:24 prb Exp	*/

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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sem.h"

#define	FORMAT(n) ((n) > -10 && (n) < 10 ? "%d" : "0x%x")

#define	stop(s)	error(s), exit(1)

int	include __P((const char *, int));
void	yyerror __P((const char *));
int	yylex __P((void));
int	mm_firstfile __P((const char *));
int	mmparse __P((void));
int	cc_include __P((const char *, int));
extern const char *lastfile;
extern const char *srctok;
extern const char *yyfile;

static	struct	config conf;	/* at most one active at a time */
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
static	void	setmachine __P((const char *));

int files_read;

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
%token	VECTOR
%token	<val> FFLAG NUMBER
%token	<str> PATHNAME WORD

%type	<list>	fopts fexpr fatom
%type	<val>	fflgs
%type	<str>	rule
%type	<str>	value
%type	<list>	locators locator
%type	<list>	swapdev_list dev_spec
%type	<str>	device_instance
%type	<str>	attachment
%type	<val>	major_minor signed_number npseudo tz
%type	<val>	flags_opt

%left	'|'
%left	'&'
%left	'!'

%%

Configuration:
	specs ENDFILE		= { YYACCEPT; } ;

hdrs:
	hdrs hdr |
	/* EMPTY */ ;

hdr:
	variable |
	INCLUDE WORD '\n' 		= { (void)cc_include($2, '\n'); } |
	INCLUDE PATHNAME '\n' 		= { (void)cc_include($2, '\n'); } |
	SYSTREE PATHNAME '\n' 		= { assign("SYS", $2, 0); } ;

machine_spec:
	XMACHINE WORD '\n'		= { setmachine($2); } ;
	

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

include:
	INCLUDE WORD			= { (void)cc_include(expand($2, yyline), '\n'); } ;

files:
	XFILES WORD			= { mm_firstfile(expand($2, yyline)); 
					    if (mmparse())
						yyerror($2);
					  } |
	XFILES PATHNAME			= { mm_firstfile(expand($2, yyline)); 
					    if (mmparse())
						yyerror($2);
					  } ;

/*
 * The configuration grammar.
 */
specs:
	specs spec |
	/* empty */;

spec:
	config_spec '\n'		= { adepth = 0; } |
	'\n' |
	error '\n'			= { cleanup(); };

config_spec:
	file |
	include |
	files |
	variable |
	OPTIONS opt_list |
	MAKEOPTIONS mkopt_list |
	SYSTREE PATHNAME 		= { assign("SYS", $2, 0); } |
	XMACHINE WORD			= { setmachine($2); } |
	TIMEZONE tz			= { settz($2); } |
	MAXUSERS NUMBER			= { setmaxusers($2); } |
	CONFIG conf sysparam_list	= { addconf(&conf); } |
	PSEUDO_DEVICE WORD npseudo	= { addpseudo($2, $3); } |
	device_instance AT attachment locators flags_opt
					= { adddev($1, $3, $4, $5); };

mkopt_list:
	mkopt_list ',' mkoption |
	mkoption;

mkoption:
	WORD '=' value			= { addmkoption($1, $3); }

opt_list:
	opt_list ',' option |
	option;

option:
	WORD				= { addoption($1, NULL); } |
	WORD '=' value			= { addoption($1, $3); };

tz:
	signed_number			= { $$ = $1 * 60; } |
	NUMBER ':' NUMBER		= { $$ = $1 * 60 + $3; } |
	'-' NUMBER ':' NUMBER		= { $$ = -$2 * 60 - $4; };

conf:
	WORD				= { conf.cf_name = $1;
					    conf.cf_lineno = cc_currentline();
					    conf.cf_root = NULL;
					    conf.cf_swap = NULL;
					    conf.cf_dump = NULL; };

sysparam_list:
	sysparam_list sysparam |
	sysparam;

sysparam:
	ROOT on_opt dev_spec	 = { setconf(&conf.cf_root, "root", $3); } |
	SWAP on_opt swapdev_list = { setconf(&conf.cf_swap, "swap", $3); } |
	DUMPS on_opt dev_spec	 = { setconf(&conf.cf_dump, "dumps", $3); };

swapdev_list:
	dev_spec AND swapdev_list	= { ($$ = $1)->nv_next = $3; } |
	dev_spec			= { $$ = $1; };

dev_spec:
	WORD				= { $$ = new_si($1, NODEV); } |
	major_minor			= { $$ = new_si(NULL, $1); };

major_minor:
	MAJOR NUMBER MINOR NUMBER	= { $$ = makedev($2, $4); };

on_opt:
	ON | /* empty */;

npseudo:
	NUMBER				= { $$ = $1; } |
	/* empty */			= { $$ = 1; };

device_instance:
	WORD '*'			= { $$ = starref($1); } |
	WORD				= { $$ = $1; };

attachment:
	ROOT				= { $$ = NULL; } |
	WORD '?'			= { $$ = wildref($1); } |
	WORD '*'			= { $$ = starref($1); } |
	WORD				= { $$ = $1; };

locators:
	locators locator		= { ($$ = $2)->nv_next = $1; } |
	/* empty */			= { $$ = NULL; };

locator:
	WORD value			= { $$ = new_ns($1, $2); } |
	WORD '?'			= { $$ = new_ns($1, NULL); };

flags_opt:
	FLAGS NUMBER			= { $$ = $2; } |
	/* empty */			= { $$ = 0; };

value:
	WORD				= { $$ = $1; } |
	signed_number			= { char bf[40];
					    (void)sprintf(bf, FORMAT($1), $1);
					    $$ = intern(bf); };
 
signed_number:
	NUMBER				= { $$ = $1; } |
	'-' NUMBER			= { $$ = -$2; };

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

static void
setmachine(mch)
	const char *mch;
{
	char buf[MAXPATHLEN];
	const char *source = check("SYS");

	if (files_read)
		errx(1, "machine must be specified before files files");
	oldformat = 1;
	assign("ARCH", mch, 0);
	if (source == NULL)
		assign("SYS", "../..", 0);

	/*
	 * Determine if we look for the new or old set of files files
	 * Modern config files do not set the machine type and this
	 * is no longer used, but just in case...
	 */
	if (source != NULL) {
		if (mm_firstfile("files") || mmparse())
			exit(1);

		(void)sprintf(buf, "%s/%s/conf/files", source, mch);
		if (mm_firstfile(buf) || mmparse())
			exit(1);

		(void)sprintf(buf, "%s/conf/files", source);
		if (mm_firstfile(buf) || mmparse())
			exit(1);
	} else {
		(void)sprintf(buf, "files.%s", mch);
		if (mm_firstfile("../../conf/files") || mmparse() ||
		    mm_firstfile(buf)  || mmparse())
			exit(1);
	}
}
