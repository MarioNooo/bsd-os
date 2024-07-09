/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getopt.c	8.3 (Berkeley) 4/27/95";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int	optopt,			/* character checked for validity */
	optreset;		/* reset getopt */
char	*optarg;		/* argument associated with option */

#define	BADCH	(int)'?'
#define	BADARG	(int)':'
#define	EMSG	""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int
getopt(nargc, nargv, ostr)
	int nargc;
	char * const *nargv;
	const char *ostr;
{
	extern char *__progname;
	static char *place = EMSG;		/* option letter processing */
	char *p;				/* working copy of place */
	char *oli;				/* option letter list index */

	/*
	 * Pick up from where we left off, or start anew if necessary.
	 * When starting on a new argument, check for "-" and "--".
	 * Compatibility exception: a lone "-" is considered an option
	 * if the option string includes "-".
	 */
	p = place;
	if (optreset || *p == '\0') {
		optreset = 0;
		if (optind >= nargc || *(p = nargv[optind]) != '-') {
			place = EMSG;		/* no more options */
			return (-1);
		}
		if (*++p == '\0') {
			if ((oli = strchr(ostr, '-')) != NULL) {
				optopt = '-';	/* treat it as an option */
				goto ugly_compat;
			}
			place = EMSG;		/* found "-", end options */
			return (-1);
		}
		if (*p == '-' && p[1] == '\0') {
			++optind;		/* found "--", end & skip */
			place = EMSG;
			return (-1);
		}
	}					/* option letter okay? */
	if ((optopt = *p++) == ':' || (oli = strchr(ostr, optopt)) == NULL) {
		if (*p == '\0')
			++optind;
		if (opterr && *ostr != ':')
			(void)fprintf(stderr,
			    "%s: illegal option -- %c\n", __progname, optopt);
		place = p;
		return (BADCH);
	}
ugly_compat:
	if (*++oli != ':') {			/* don't need argument */
		optarg = NULL;
		if (*p == '\0')
			++optind;
		place = p;
	}
	else {					/* need an argument */
		if (*p != '\0')			/* no white space */
			optarg = p;
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    __progname, optopt);
			return (BADCH);
		}
	 	else				/* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt);			/* dump back option letter */
}
