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
 *	@(#)scan.l	8.1 (Berkeley) 6/6/93
 */

/*
 * This file is built twice, once for each grammer.
 * The config grammer changes yy/YY to cc/CC
 * The files grammer changes yy/YY to mm/MM
 */

#include <sys/param.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "yy.tab.h"

int	yyline;
const char *yyfile;
const char *lastfile;
extern const char *srcfile;

int	yy_include(const char *, int);

static const char *stripquotes(const char *);

#define	YY_NO_UNPUT

/*
 * Data for returning to previous files from include files.
 */
struct incl {
	struct	incl *in_prev;	/* previous includes in effect, if any */
	YY_BUFFER_STATE in_buf;	/* previous lex state */
	const char *in_fname;	/* previous file name */
	int	in_lineno;	/* previous line number */
	int	in_preveof;	/* previous eoftoken */
};
static struct incl *incl;
static int eoftoken;		/* current EOF token */
static void endinclude(void);
#ifdef	MM_NULL
static int mmcount;
#endif
extern int files_read;

#define	yywrap() 1

%}

WORD	[A-Za-z_][-A-Za-z_0-9]*
PATHC	(([-/A-Za-z0-9_.\\])|(\$\{{WORD}\}))
PATH	([A-Za-z]:)?{PATHC}*[./\\]{PATHC}*
/* quoted double and single quotes */
QDQ	\\\"
QSQ	\\'

%%

		/* plain keywords */
and		{ return AND; }
at		{ return AT; }
compile-with	{ return COMPILE_WITH; }
config		{ return CONFIG; }
define		{ return DEFINE; }
device		{ return DEVICE; }
dumps		{ return DUMPS; }
flags		{ return FLAGS; }
file		{ return XFILE; }
files		{ return XFILES; }
include		{ return INCLUDE; }
machine		{ return XMACHINE; }
major		{ return MAJOR; }
makeoptions	{ return MAKEOPTIONS; }
maxusers	{ return MAXUSERS; }
minor		{ return MINOR; }
obj-directory	{ return OBJPATH; }
on		{ return ON; }
options		{ return OPTIONS; }
pseudo-device	{ return PSEUDO_DEVICE; }
require		{ return REQUIRE; }
root		{ return ROOT; }
swap		{ return SWAP; }
systree		{ return SYSTREE; }
timezone	{ return TIMEZONE; }
vector		{ return VECTOR; }

		/* keywords with values */
always-source	{ yylval.val = FI_ALWAYSSRC; return FFLAG; }
config-dependent { yylval.val = FI_CONFIGDEP; return FFLAG; }
device-driver	{ yylval.val = FI_DRIVER; return FFLAG; }
needs-count	{ yylval.val = FI_NEEDSCOUNT; return FFLAG; }
needs-flag	{ yylval.val = FI_NEEDSFLAG; return FFLAG; }

		/* all the rest */
{PATH}		{ yylval.str = convertposix(yytext); return PATHNAME; }
\$\{{WORD}\}	{ yylval.str = intern(yytext); return PATHNAME; }
{WORD}		{ yylval.str = intern(yytext); return WORD; }

\"([^"]|{QDQ})*\" {
		yylval.str = stripquotes(yytext);
		return WORD;
	}

'([^']|{QSQ})*' {
		yylval.str = stripquotes(yytext);
		return WORD;
	}
0[0-7]*	{
		yylval.val = strtoul(yytext, NULL, 8);
		return NUMBER;
	}
0[xX][0-9a-fA-F]+ {
		yylval.val = strtoul(yytext + 2, NULL, 16);
		return NUMBER;
	}
[1-9][0-9]* {
		yylval.val = strtoul(yytext, NULL, 10);
		return NUMBER;
	}
\n/[ \t] {
		yyline++;
	}
\n	{
		yyline++;
		return '\n';
	}
#[^\n]*\n[ \t] {
		yyline++;
	}
#[^\n]*\n {
		yyline++;
		return '\n';
	}
[ \t]*	{ /* ignored (white space) */; }
.	{ return yytext[0]; }
<<EOF>> {
		if (incl != NULL)
			endinclude();
		else {
#ifdef	MM_NULL
			mmcount = 0;
#endif
			return (ENDFILE);
		}
	}

%%

/*
 * Strip leading and trailing quotes, and embedded backslash-quote,
 * marks from a string, then intern the result.
 */
static const char *
stripquotes(txt)
	const char *txt;
{
	size_t len;
	int i, j;
	char qc;
	char cleaned[LINE_MAX];

	/*
	 * We are going to skip over the first quote and we want
	 * to stop before the last quote.
	 * Pick up the kind of quote mark to handle when embedded by
	 * looking at the first character of the buffer (the opening
	 * quote).
	 */
	len = strlen(txt) - 1;
	qc = txt[0];

	/* make sure the output will fit */
	if (len >= sizeof cleaned)
		panic("stripquotes");
	/*
	 * Strip out the outer quotes (by copying "len" chars max)
	 * and embedded backslash-quotemark characters too.
	 */
	for (i= 0, j = 1; j < len; j++)
		/* Skip if a backslash in front of the quoted char */
		if (txt[j] != '\\' || j + 1 >= len || txt[j+1] != qc)
			/* Put the char into the clean buffer */
			cleaned[i++] = txt[j];
	/* Anchor the string with terminating NULL */
	cleaned[i] = '\0';
	return (intern(cleaned));
}

/*
 * Open the "main" file (conffile).
 */
int
yy_firstfile(fname)
	const char *fname;
{

	if ((yyin = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);
#ifdef	MM_NULL
	++mmcount;
#endif
#ifdef	CC_NULL
	srcfile = yyfile = conffile = fname;
#else
	srcfile = yyfile = fname;
#endif
	yyline = 1;
	eoftoken = YY_NULL;
	return (0);
}

/*
 * Open the named file for inclusion at the current point.  Returns 0 on
 * success (file opened and previous state pushed), nonzero on failure
 * (fopen failed, complaint made).  The `ateof' parameter controls the
 * token to be returned at the end of the include file (typically '\n'
 * or ENDFILE).
 */
int
yy_include(fname, ateof)
	const char *fname;
	int ateof;
{
	FILE *fp;
	struct incl *in;
	extern int yychar;		/* XXX */

	if ((fp = fopen(fname, "r")) == NULL) {
		err(1, "%s", fname);
	}
#ifdef	MM_NULL
	files_read = 1;
	++mmcount;
#endif
	in = emalloc(sizeof *in);
	in->in_prev = incl;
	in->in_buf = YY_CURRENT_BUFFER;
	in->in_fname = yyfile;
	in->in_lineno = yyline;
	in->in_preveof = eoftoken;
	incl = in;
	yy_switch_to_buffer(yy_create_buffer(fp, YY_BUF_SIZE));
	srcfile = yyfile = intern(fname);
	yyline = 1;
	eoftoken = ateof;
#if 1
	/*
	 * If yacc has not already read the newline following an
	 * `include' directive, feed it one, so that the grammar
	 * (which requires one) will see one even if it is not there.
	 * This prevents included files from having to begin with
	 * a newline.  This is a gross hack, but what the heck,
	 * it was easier than changing the grammar everywhere (and
	 * besides, I use those newlines for error recovery).
	 */
	if (yychar < 0)			/* XXX */
		yychar = '\n';		/* XXX */
#endif
	return (0);
}

/*
 * Terminate the most recent inclusion.
 */
static void
endinclude()
{
	struct incl *in;

	if ((in = incl) == NULL)
		panic("endinclude");
	incl = in->in_prev;
	lastfile = yyfile;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	(void)fclose(yyin);
	yy_switch_to_buffer(in->in_buf);
	srcfile = yyfile = in->in_fname;
	yyline = in->in_lineno;
	eoftoken = in->in_preveof;
	free(in);
}

/*
 * Return the current line number.  If yacc has looked ahead and caused
 * us to consume a newline, we have to subtract one.  yychar is yacc's
 * token lookahead, so we can tell.
 */
int
yy_currentline()
{
	extern int yychar;

	return (yyline - (yychar == '\n'));
}

#ifdef	MM_NULL
int
currentline()
{
	return (mmcount ? mm_currentline() : cc_currentline());
}
#endif
