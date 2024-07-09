/*	BSDI options.c,v 2.6 1998/02/14 14:32:13 donn Exp	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
 */

#ifndef lint
static char sccsid[] = "@(#)options.c	8.2 (Berkeley) 5/4/95";
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#define DEFINE_OPTIONS
#include "options.h"
#undef DEFINE_OPTIONS
#include "nodes.h"	/* for other header files */
#include "eval.h"
#include "jobs.h"
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#ifndef NO_HISTORY
#include "myhistedit.h"
#endif

char *arg0;			/* value of $0 */
struct shparam shellparam;	/* current positional parameters */
char **argptr;			/* argument list for builtin commands */
char *optarg;			/* set by nextopt (like getopt) */
char *optptr;			/* used by nextopt */

char *minusc;			/* argument to -c option */

int vexport;			/* set to VEXPORT if -a option is used */


STATIC void options __P((int));
STATIC void minus_o __P((char *, int));
STATIC void setoption __P((int, int));


/*
 * Process the shell command line arguments.
 */

void
procargs(argc, argv)
	int argc;
	char **argv;
{
	int i;

	argptr = argv;
	if (argc > 0)
		argptr++;
	for (i = 0; i < NOPTS; i++)
		optlist[i].val = 2;
	options(1);
	if (*argptr == NULL && minusc == NULL)
		sflag = 1;
	if (iflag == 2 && sflag == 1 && isatty(0) && isatty(1))
		iflag = 1;
	if (mflag == 2)
		mflag = iflag;
	for (i = 0; i < NOPTS; i++)
		if (optlist[i].val == 2)
			optlist[i].val = 0;
	arg0 = argv[0];
	if (sflag == 0) {
		if (*argptr)
			arg0 = *argptr++;
		commandname = arg0;
		if (minusc == NULL)
			setinputfile(commandname, 0);
	}
	shellparam.p = argptr;
	/* assert(shellparam.malloc == 0 && shellparam.nparam == 0); */
	while (*argptr) {
		shellparam.nparam++;
		argptr++;
	}
	optschanged();
}


void
optschanged()
{
	setinteractive(iflag);
#ifndef NO_HISTORY
	histedit();
#endif
	setjobctl(mflag);
}

/*
 * Process shell options.  The global variable argptr contains a pointer
 * to the argument list; we advance it past the options.
 */

STATIC void
options(cmdline) 
	int cmdline;
{
	register char *p;
	int val;
	int c;

	if (cmdline)
		minusc = NULL;
	while ((p = *argptr) != NULL) {
		argptr++;
		if ((c = *p++) == '-') {
			val = 1;
                        if (p[0] == '\0' || (p[0] == '-' && p[1] == '\0')) {
                                if (!cmdline) {
                                        /* "-" means turn off -x and -v */
                                        if (p[0] == '\0')
                                                xflag = vflag = 0;
                                        /* "--" means reset params */
                                        else if (*argptr == NULL)
						setparam(argptr);
                                }
				break;	  /* "-" or  "--" terminates options */
			}
		} else if (c == '+') {
			val = 0;
		} else {
			argptr--;
			break;
		}
		while ((c = *p++) != '\0') {
			if (c == 'c' && cmdline) {
				char *q;
#ifdef NOHACK	/* removing this code allows sh -ce 'foo' for compat */
				if (*p == '\0')
#endif
					q = *argptr++;
				if (q == NULL || minusc != NULL)
					error("Bad -c option");
				minusc = q;
#ifdef NOHACK
				break;
#endif
			} else if (c == 'o') {
				minus_o(*argptr, val);
				if (*argptr)
					argptr++;
			} else if (c == 'a')
				vexport = VEXPORT;
			else if (c == 'r' && val) {
				setoption(c, val);
				checkrestricted("rsh");
			} else if (rflag == 1)
				error("%c%c: restricted", val ? '-' : '+', c);
			else
				setoption(c, val);
		}
	}
}

STATIC void
minus_o(name, val)
	char *name;
	int val;
{
	int i;

	if (name == NULL) {
		out1str("Current option settings\n");
		for (i = 0; i < NOPTS; i++)
			out1fmt("%-16s%s\n", optlist[i].name,
				optlist[i].val ? "on" : "off");
	} else {
		for (i = 0; i < NOPTS; i++)
			if (equal(name, optlist[i].name)) {
				setoption(optlist[i].letter, val);
				return;
			}
		error("Illegal option -o %s", name);
	}
}
			

STATIC void
setoption(flag, val)
	char flag;
	int val;
	{
	int i;

	for (i = 0; i < NOPTS; i++)
		if (optlist[i].letter == flag) {
			optlist[i].val = val;
			if (val) {
				/* #%$ hack for ksh semantics */
				if (flag == 'V')
					Eflag = 0;
				else if (flag == 'E')
					Vflag = 0;
			}
			return;
		}
	error("Illegal option -%c", flag);
}


/*
 * Set the shell parameters.
 */

void
setparam(argv)
	char **argv;
	{
	char **newparam;
	char **ap;
	int nparam;

	for (nparam = 0 ; argv[nparam] ; nparam++);
	ap = newparam = ckmalloc((nparam + 1) * sizeof *ap);
	while (*argv) {
		*ap++ = savestr(*argv++);
	}
	*ap = NULL;
	freeparam(&shellparam);
	shellparam.malloc = 1;
	shellparam.nparam = nparam;
	shellparam.p = newparam;
	shellparam.optnext = NULL;
}


/*
 * Free the list of positional parameters.
 */

void
freeparam(param)
	struct shparam *param;
	{
	char **ap;

	if (param->malloc) {
		for (ap = param->p ; *ap ; ap++)
			ckfree(*ap);
		ckfree(param->p);
	}
}



/*
 * The shift builtin command.
 */

int
shiftcmd(argc, argv)
	int argc;
	char **argv; 
{
	int n;
	char **ap1, **ap2;

	n = 1;
	if (argc > 1)
		n = number(argv[1]);
	if (n > shellparam.nparam)
		error("can't shift that many");
	INTOFF;
	shellparam.nparam -= n;
	for (ap1 = shellparam.p ; --n >= 0 ; ap1++) {
		if (shellparam.malloc)
			ckfree(*ap1);
	}
	ap2 = shellparam.p;
	while ((*ap2++ = *ap1++) != NULL);
	shellparam.optnext = NULL;
	INTON;
	return 0;
}



/*
 * The set command builtin.
 */

int
setcmd(argc, argv)
	int argc;
	char **argv; 
{
	if (argc == 1)
		return showvarscmd(argc, argv);
	INTOFF;
	options(0);
	optschanged();
	if (*argptr != NULL) {
		setparam(argptr);
	}
	INTON;
	return 0;
}



/*
 * The getopts command builtin.
 * The opt_count variable counts the number of iterations of getopt()
 * that we have made with the given option string since the last reset.
 * The opt_hack variable communicates with setvareq() to tell it that
 * getoptscmd() is setting OPTIND, rather than the user.
 * The opt_string variable saves a copy of the option string so that
 * we can automatically reset if it changes.
 */
int opt_count;
int opt_hack;
char *opt_string;

/* POSIX.2 wants us to print the script name, rather than just 'getopts'.  */
static char *
getoptserrname()
{
	char *s;

	if (arg0 == NULL)
		return ("getopts");
	if ((s = strrchr(arg0, '/')) != NULL)
		return (++s);
	return (arg0);
}

int
getoptscmd(argc, argv)
	int argc;
	char **argv;
{
	char buf[32];
	char *s;
	char *var;
	int i;
	int c = 0;

	if (argc < 3)
		error("Usage: getopts optstring var [arg ...]");

	/* If the option string changed, reset our state.  */
	if (opt_string == NULL || strcmp(argv[1], opt_string) != 0) {
		opt_count = 0;
		if (opt_string != NULL)
			ckfree(opt_string);
		opt_string = savestr(argv[1]);
	}

	/* Regenerate our state from scratch.  */
	s = *++argv;
	var = *++argv;
	if (argc == 3) {
		/* No explicit arguments -- use the positional parameters.  */
		argc = shellparam.nparam + 1;
		argv = shellparam.p - 1;
	} else
		argc -= 2;
	optreset = 1;
	optind = 1;
	opterr = 0;
	for (i = 0; i < opt_count; ++i)
		if ((c = getopt(argc, argv, s)) == -1)
			break;

	/* Scan the next option.  */
	if (c != -1)
		c = getopt(argc, argv, s);

	opt_hack = 1;
	fmtstr(buf, 10, "%d", optind);
	setvar("OPTIND", buf, 0);
	opt_hack = 0;

	if (c == -1) {
		/* No more options.  */
		unsetvar("OPTARG");
		setvar(var, "?", vexport);
		return (1);
	}

	/* Update our counters.  */
	++opt_count;

	buf[0] = optopt;
	buf[1] = '\0';

	/* Check for errors.  Handle the ':' convention.  */
	if (s[0] == ':' && c == ':') {
		setvar("OPTARG", buf, 0);
		setvar(var, ":", vexport);
		return (0);
	}
	if (c == '?' && optopt != '?') {
		if (optopt == ':' || strchr(s, optopt) == NULL) {
			if (s[0] == ':') {
				setvar("OPTARG", buf, 0);
				setvar(var, "?", vexport);
				return (0);
			}
			/* Imitate getopt()'s error messages.  */
			outfmt(&errout, "%s: illegal option -- %c\n",
			    getoptserrname(), optopt);
		} else
			outfmt(&errout, "%s: option requires an argument -- %c\n",
			    getoptserrname(), optopt);
		optarg = NULL;
		buf[0] = '?';
	}

	/* Finalize the values of variables.  */
	if (optarg != NULL)
		setvar("OPTARG", optarg, 0);
	else
		unsetvar("OPTARG");
	setvar(var, buf, vexport);

	return (0);
}



/*
 * XXX - should get rid of.  have all builtins use getopt(3).  the
 * library getopt must have the BSD extension static variable "optreset"
 * otherwise it can't be used within the shell safely.
 *
 * Standard option processing (a la getopt) for builtin routines.  The
 * only argument that is passed to nextopt is the option string; the
 * other arguments are unnecessary.  It return the character, or '\0' on
 * end of input.
 */

int
nextopt(optstring)
	char *optstring;
	{
	register char *p, *q;
	char c;

	if ((p = optptr) == NULL || *p == '\0') {
		p = *argptr;
		if (p == NULL || *p != '-' || *++p == '\0')
			return '\0';
		argptr++;
		if (p[0] == '-' && p[1] == '\0')	/* check for "--" */
			return '\0';
	}
	c = *p++;
	for (q = optstring ; *q != c ; ) {
		if (*q == '\0')
			error("Illegal option -%c", c);
		if (*++q == ':')
			q++;
	}
	if (*++q == ':') {
		if (*p == '\0' && (p = *argptr++) == NULL)
			error("No arg for -%c option", c);
		optarg = p;
		p = NULL;
	}
	optptr = p;
	return c;
}
