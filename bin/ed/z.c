/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996, 1999 Rodney Ruddock
 *
 * This code is derived from software contributed to Berkeley by
 * Rodney Ruddock of the University of Guelph.
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
static char sccsid[] = "@(#)z.c	8.1 (Berkeley) 5/31/93";
#endif /* not lint */

#include <sys/types.h>

#include <regex.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifdef DBI
#include <db.h>
#endif

#include "ed.h"
#include "extern.h"

/*
 * This prints out the next group of lines (as spec'd by the skicky
 * number; 22 by default). It's really useful for scrolling in chunks
 * through the buffer (better than l, n, or p). Shame on POSIX for
 * not including it; yaaa! BSD for keeping it! :-)
 * Extension  and "zc" for "z-" (very useful) ==> "z+" for balance. New form is
 * now "z[c|+|^|-][n]"
 */
void
z(inputt, errnum, lineno)
	FILE *inputt;
	int *errnum;
	int lineno;
{
	register int l_cnt, l_line;
	int l_flag=0, l_eof=0, l_back=0;

	if (current == NULL) {
		*errnum = -1;
		strcpy(help_msg, "buffer empty");
		return;
	}
	/* Set zsnum if need be here. */
	ss = getc(inputt);
	if (ss == '+') {
		/* This was the implied default action; a no-op really */
		ss = getc(inputt);
	}
	else if (ss == 'c') {
		/* reprint the "current" chunk */
		l_flag = 1;
		l_back = 0;	/* yes -- "0" */
		ss = getc(inputt);
	}
	else if (ss == '-' || ss == '^') {
		/* Move back (up to) zsnum before printing zsnum lines; for
		 * scrolling back in "chunks"
		 */
		l_flag = 1;
		l_back = -zsnum;	/* yes -- "-zsnum" */
		ss = getc(inputt);
	}
	if ((ss > 48) && (ss < 57))
		/* Default set in main. */
		zsnum = dig_num_conv(inputt, errnum);
	else
		if ((ss != '\n') && (ss != EOF)) {
			ungetc(ss, inputt);
			if (rol(inputt, errnum))
				return;
		}
	if (top == NULL) {
		strcpy(help_msg, "buffer empty");
		*errnum = -1;
		ungetc('\n', inputt);
		return;
	}
	if (End_default) {
		if ((current->below) != NULL) {
			Start = current->below;
		}
		else if (l_flag) {
			Start = bottom;
			l_back++;
			l_eof = 1;
		}
		else {
			strcpy(help_msg, "at end of buffer");
			*errnum = -1;
			ungetc('\n', inputt);
			return;
		}
	}
	else {
		Start = End;
	}
	if (Start == NULL) {
		strcpy(help_msg, "bad address");
		*errnum = -1;
		ungetc('\n', inputt);
		return;
	}
	Start_default = End_default = 0;

	if (l_flag) {
		l_cnt = l_back + l_eof;	/* Yes, set to = l_back */
		while (1) {
			if (Start->above == NULL) {
				break;
			}
			if (sigint_flag && (!sigspecial)) {
				SIGINT_ACTION;
			}
			l_cnt++;
			if (zsnum < l_cnt) {
				break;
			}
			Start = Start->above;
		}
	}
	current = Start;
	if (lineno) {
		l_line = line_number(Start);
	}
	l_cnt = 1;		/* Yes, set to = 1. */
	while (1) {
		/* Scroll-out the next 'zsnum' of lines or until bottom. */
		if (current == NULL) {
			break;
		}
		if (sigint_flag && (!sigspecial)) {
			SIGINT_ACTION;
		}
		get_line(current->handle, current->len);
		if (lineno) {
			printf("%d\t", l_line++);
		}
		printf("%s\n", text);
		if (current == bottom) {
			break;
		}
		l_cnt++;
		if (zsnum < l_cnt) {
			break;
		}
		current = current->below;
	}
	*errnum = 1;
}
