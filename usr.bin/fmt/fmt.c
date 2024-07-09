/*
 * Copyright (c) 1980, 1993
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)fmt.c	8.1 (Berkeley) 7/20/93";
#endif /* not lint */

#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NBUFSIZ 100000	/* longer than most paragraphs */
/*
 * fmt -- format the concatenation of input files or standard input
 * onto standard output.  Designed for use with Mail ~|
 *
 * Syntax : fmt [ -d ] [ goal [ max ] ] [ name ... ]
 */

#define GOAL_LENGTH 65
#define MAX_LENGTH 75

int	goal_length;		/* Target or goal line length in output */
int	max_length;		/* Max line length in output */
int	pfx;			/* Current leading blank count */
int	lineno;			/* Current input line */
int	mark;			/* Last place we saw a head line */
int	dotsok;

char	*headnames[] = {"To", "Subject", "Cc",  NULL};

void	fmt __P((FILE *));
int	ispref	__P((char *, char *));
void	leadin __P((void));
void	oflush __P((void));
void	pack __P((char *, int));
void	prefix __P((char *));
void	setout __P((void));
void	split __P((char *));
void	tabulate __P((char *));

/*
 * Drive the whole formatter by managing input files.  Also, cause
 * initialization of the output stuff and flush it out at the end.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *fi;
	int errs, number;
	int c;

	goal_length = GOAL_LENGTH;
	max_length = MAX_LENGTH;

	while ((c = getopt(argc, argv, "d")) != -1)
		switch (c) {
		case 'd':
			dotsok = 1;
			break;
		/* XXX could goal/max parameters be options? */
		default:
			fprintf(stderr, "usage: fmt [-d] [goal [max]] [files ...]\n");
			exit(1);
		}
	argv += optind;

	setout();
	lineno = 1;
	mark = -10;

	if (*argv != NULL && sscanf(*argv, "%d", &number) == 1) {
		argv++;
		goal_length = number;
		if (*argv != NULL && sscanf(*argv, "%d", &number) == 1) {
			argv++;
			max_length = number;
		}
	}
	if (max_length <= goal_length)
		errx(1, "Max length must be greater than %d", goal_length);

	errs = 0;
	if (*argv == NULL)
		fmt(stdin);
	else
		for (; *argv != NULL; ++argv) {
			if ((fi = fopen(*argv, "r")) == NULL) {
				warn("%s", *argv);
				errs = 1;
				continue;
			}
			fmt(fi);
			fclose(fi);
		}
	oflush();
	exit(errs);
}

/*
 * Read up characters from the passed input file, forming lines, doing ^H
 * processing, expanding tabs, stripping trailing blanks, and sending each
 * line down for analysis.
 */
void
fmt(fi)
	FILE *fi;
{
	int c, col;
	char *cp, *cp2, linebuf[NBUFSIZ], canonb[NBUFSIZ];

	c = getc(fi);
	while (c != EOF) {
		/*
		 * Collect a line, doing ^H processing.
		 * Leave tabs for now.
		 */
		cp = linebuf;
		while (c != '\n' && c != EOF && cp-linebuf < NBUFSIZ-1) {
			if (c == '\b') {
				if (cp > linebuf)
					cp--;
				c = getc(fi);
				continue;
			}
			*cp++ = c;
			c = getc(fi);
		}
		*cp = '\0';

		/*
		 * Toss anything remaining on the input line.
		 *
		 * XXX
		 * Huh!?!?
		 */
		while (c != '\n' && c != EOF)
			c = getc(fi);
		
		/*
		 * Expand tabs on the way to canonb.
		 */
		col = 0;
		cp = linebuf;
		cp2 = canonb;
		while (c = *cp++) {
			if (c != '\t') {
				col++;
				if (cp2 - canonb < NBUFSIZ-1)
					*cp2++ = c;
				continue;
			}
			do {
				if (cp2 - canonb < NBUFSIZ-1)
					*cp2++ = ' ';
				col++;
			} while ((col & 07) != 0);
		}

		/*
		 * Swipe trailing blanks from the line.
		 */
		for (cp2--; cp2 >= canonb && *cp2 == ' '; cp2--)
			;
		*++cp2 = '\0';
		prefix(canonb);
		if (c != EOF)
			c = getc(fi);
	}
}

/*
 * Take a line devoid of tabs and other garbage and determine its blank prefix.
 * If the indent changes, call for a linebreak.  If the input line is blank,
 * echo the blank line on the output.  Finally, if the line minus the prefix
 * is a mail header, try to keep it on a line by itself.
 */
void
prefix(line)
	char *line;
{
	int np, h;
	char *cp, **hp;

	if (strlen(line) == 0) {
		oflush();
		putchar('\n');
		return;
	}
	for (cp = line; *cp == ' '; cp++)
		;
	np = cp - line;

	/*
	 * The following horrible expression attempts to avoid linebreaks
	 * when the indent changes due to a paragraph.
	 */
	if (np != pfx && (np > pfx || abs(pfx-np) > 8))
		oflush();
	if (h = ishead(cp))
		oflush(), mark = lineno;
	if (lineno - mark < 3 && lineno - mark > 0)
		for (hp = &headnames[0]; *hp != (char *) 0; hp++)
			if (ispref(*hp, cp)) {
				h = 1;
				oflush();
				break;
			}
	if (!dotsok && !h && (h = (*cp == '.')))
		oflush();
	pfx = np;
	if (h)
		pack(cp, strlen(cp));
	else	split(cp);
	if (h)
		oflush();
	lineno++;
}

/*
 * Split up the passed line into output "words" which are maximal strings
 * of non-blanks with the blank separation attached at the end.  Pass these
 * words along to the output line packer.
 */
void
split(line)
	char *line;
{
	int wordl;
	char *cp, *cp2, word[NBUFSIZ];

	for (cp = line; *cp != '\0';) {
		cp2 = word;
		wordl = 0;

		/*
		 * Collect a 'word,' allowing it to contain escaped white
		 * space. 
		 */
		while (*cp && *cp != ' ') {
			if (*cp == '\\' && isspace(cp[1]))
				*cp2++ = *cp++;
			*cp2++ = *cp++;
			wordl++;
		}

		/*
		 * Guarantee a space at end of line. Two spaces after end of
		 * sentence punctuation. 
		 */
		if (*cp == '\0') {
			*cp2++ = ' ';
			if (index(".:!?", cp[-1]))
				*cp2++ = ' ';
		}
		while (*cp == ' ')
			*cp2++ = *cp++;
		*cp2 = '\0';
		pack(word, wordl);
	}
}

/*
 * Output section.
 *
 * Build up line images from the words passed in.  Prefix each line with
 * correct number of blanks.  The buffer "outbuf" contains the current
 * partial line image, including prefixed blanks.  "outp" points to the
 * next available space therein.  When outp is NULL, there ain't nothing
 * in there yet.  At the bottom of this whole mess, leading tabs are
 * reinserted.
 */
char	outbuf[NBUFSIZ];			/* Sandbagged output line image */
char	*outp;				/* Pointer in above */

/*
 * Initialize the output section.
 */
void
setout()
{
	outp = NULL;
}

/*
 * Pack a word onto the output line.  If this is the beginning of the line,
 * push on the appropriately-sized string of blanks first.  If the word won't
 * fit on the current line, flush and begin a new line.  If the word is too
 * long to fit all by itself on a line, just give it its own and hope for the
 * best.
 *
 * If the new word will fit in at less than the goal length, take it.  If not,
 * then check to see if the line will be over the max length; if so put the
 * word on the next line.  If not, check to see if the line will be closer to
 * the goal length with or without the word and take it or put it on the next
 * line accordingly.
 */
void
pack(word, wl)
	char *word;
	int wl;
{
	int s, t;
	char *cp;

	if (outp == NULL)
		leadin();

	/*
	 * Change condition to check goal_length; s is the length of the
	 * line before the word is added; t is now the length of the line
	 * after the word is added.
	 */
	s = outp - outbuf;
	t = wl + s;
	if (t <= goal_length ||
	    (t <= max_length && t - goal_length <= goal_length - s)) {
		/*
		 * In like flint! 
		 */
		for (cp = word; *cp; *outp++ = *cp++);
		return;
	}
	if (s > pfx) {
		oflush();
		leadin();
	}
	for (cp = word; *cp != '\0'; *outp++ = *cp++);
}

/*
 * If there is anything on the current output line, send it on its way.  Set
 * outp to NULL to indicate the absence of the current line prefix.
 */
void
oflush()
{
	if (outp != NULL) {
		*outp = '\0';
		tabulate(outbuf);
		outp = NULL;
	}
}

/*
 * Take the passed line buffer, insert leading tabs where possible, and
 * output on standard output (finally).
 */
void
tabulate(line)
	char *line;
{
	int b, t;
	char *cp;

	/*
	 * Toss trailing blanks in the output line.
	 */
	cp = line + strlen(line) - 1;
	while (cp >= line && *cp == ' ')
		cp--;
	*++cp = '\0';
	
	/*
	 * Count the leading blank space and tabulate.
	 */
	for (cp = line; *cp == ' '; cp++)
		;
	b = cp-line;
	t = b >> 3;
	b &= 07;
	if (t > 0)
		do
			(void)putchar('\t');
		while (--t);
	if (b > 0)
		do
			(void)putchar(' ');
		while (--b);
	for (; *cp != '\0'; ++cp)
		(void)putchar(*cp);
	(void)putchar('\n');
}

/*
 * Initialize the output line with the appropriate number of
 * leading blanks.
 */
void
leadin()
{
	int b;
	char *cp;

	for (b = 0, cp = outbuf; b < pfx; b++)
		*cp++ = ' ';
	outp = cp;
}

/*
 * Save a string in dynamic space.  This little goodie is needed for
 * a headline detector in head.c
 */
char *
savestr(str)
	char *str;
{
	char *p;

	if ((p = strdup(str)) == NULL)
		err(1, NULL);
	return (p);
}

/*
 * Is s1 a prefix of s2??
 */
int
ispref(s1, s2)
	char *s1, *s2;
{

	while (*s1++ == *s2)
		;
	return (*s1 == '\0');
}
