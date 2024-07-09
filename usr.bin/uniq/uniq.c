/*	BSDI uniq.c,v 2.6 1997/01/22 01:48:41 bostic Exp	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Case Larsen.
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
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)uniq.c	8.3 (Berkeley) 5/4/95";
#endif /* not lint */

#include <sys/param.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cflag, dflag, uflag;
int numchars, numfields, repeats;

FILE	*file __P((char *, char *));
void	 show __P((FILE *, u_char *, size_t));
u_char	*skip __P((u_char *, size_t));
void	 obsolete __P((char *[]));
__dead void	
	usage __P((void)) __attribute__((volatile));

int
main (argc, argv)
	int argc;
	char *argv[];
{
	FILE *ifp, *ofp;
	size_t alloclen, plen, prevlen;
	int ch;
	u_char *p, *prev, *t1, *t2;
	char *ep;

	obsolete(argv);
	while ((ch = getopt(argc, argv, "-cdf:s:u")) != EOF)
		switch (ch) {
		case '-':
			--optind;
			goto done;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			numfields = strtol(optarg, &ep, 10);
			if (numfields < 0 || *ep)
				errx(1, "illegal field skip value: %s", optarg);
			break;
		case 's':
			numchars = strtol(optarg, &ep, 10);
			if (numchars < 0 || *ep)
				errx(1,
				    "illegal character skip value: %s", optarg);
			break;
		case 'u':
			uflag = 1;
			break;
		case '?':
		default:
			usage();
	}

done:	argc -= optind;
	argv +=optind;

	/* If no flags are set, default is -d -u. */
	if (cflag) {
		if (dflag || uflag)
			usage();
	} else if (!dflag && !uflag)
		dflag = uflag = 1;

	switch(argc) {
	case 0:
		ifp = stdin;
		ofp = stdout;
		break;
	case 1:
		ifp = file(argv[0], "r");
		ofp = stdout;
		break;
	case 2:
		ifp = file(argv[0], "r");
		ofp = file(argv[1], "w");
		break;
	default:
		usage();
		/* NOTREACHED */
	}

	/* Save a copy of the first line. */
	if ((p = (u_char *)fgetln(ifp, &plen)) == NULL)
		exit(0);
#define	MAXLINELEN	(8 * 1024)
	alloclen = MAX(plen, MAXLINELEN);
	if ((prev = malloc(alloclen)) == NULL)
		err(1, NULL);
	memcpy(prev, p, plen);

	/* Calculate the information we'll need for loop comparisons. */
	prevlen = plen;
	if (numfields || numchars)
		t2 = skip(prev, prevlen);
	else
		t2 = prev;

	/* Process the rest of the lines. */
	while ((p = (u_char *)fgetln(ifp, &plen)) != NULL) {
		/* If requested, get the chosen fields + character offsets. */
		if (numfields || numchars) {
			t1 = skip(p, plen);
			if (plen - (t1 - p) != prevlen - (t2 - prev))
				goto length;
		} else {
			t1 = p;
			if (plen != prevlen)
				goto length;
		}

		/*
		 * If different:
		 *	print the last value.
		 *	allocate space for a copy of the new value.
		 *	copy the new value.
		 *	replace old comparison information with new.
		 *	reset repeat count.
		 */
		if (memcmp(t1, t2, plen - (t1 - p))) {
length:			show(ofp, prev, prevlen);

			if (plen > alloclen && (prev =
			    realloc(prev, alloclen = plen + 1024)) == NULL)
				err(1, NULL);
			memcpy(prev, p, prevlen = plen);
			t2 = prev + (t1 - p);

			repeats = 0;
		} else
			++repeats;
	}
	show(ofp, prev, prevlen);
	exit(0);
}

/*
 * show --
 *	Output a line depending on the flags and number of repetitions
 *	of the line.
 */
void
show(ofp, str, len)
	FILE *ofp;
	u_char *str;
	size_t len;
{

	if (cflag)
		(void)fprintf(ofp, "%4d ", repeats + 1);
	if (cflag || (dflag && repeats) || (uflag && !repeats))
		(void)fwrite(str, sizeof(*str), len, ofp);
}

u_char *
skip(str, len)
	u_char *str;
	size_t len;
{
	int infield, nchars, nfields;

	for (nfields = numfields, infield = 0; nfields && len > 0; ++str, --len)
		if (isspace(*str)) {
			if (infield) {
				infield = 0;
				--nfields;
			}
		} else if (!infield)
			infield = 1;
	for (nchars = numchars; nchars-- && len > 0; ++str, --len);
	return (str);
}

FILE *
file(name, mode)
	char *name, *mode;
{
	FILE *fp;

	if ((fp = fopen(name, mode)) == NULL)
		err(1, "%s", name);
	return (fp);
}

void
obsolete(argv)
	char *argv[];
{
	size_t len;
	char *ap, *p, *start;

	while ((ap = *++argv) != NULL) {
		/* Return if "--" or not an option of any form. */
		if (ap[0] != '-') {
			if (ap[0] != '+')
				return;
		} else if (ap[1] == '-')
			return;
		if (!isdigit(ap[1]))
			continue;
		/*
		 * Digit signifies an old-style option.  Malloc space for dash,
		 * new option and argument.
		 */
		len = strlen(ap);
		if ((start = p = malloc(len + 3)) == NULL)
			err(1, NULL);
		*p++ = '-';
		*p++ = ap[0] == '+' ? 's' : 'f';
		(void)strcpy(p, ap + 1);
		*argv = start;
	}
}

__dead void
usage()
{
	(void)fprintf(stderr,
	    "usage: uniq [-c | -du] [-f fields] [-s chars] [input [output]]\n");
	exit(1);
}
