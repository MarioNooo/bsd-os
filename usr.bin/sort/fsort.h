/* Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 * 
 * Derived from software contributed to Berkeley by Peter McIlroy.
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

/* BSDI fsort.h,v 2.1 1995/02/03 13:05:47 polk Exp */

#define POW 21				/* exponent for buffer size */
#define BUFSIZE ((1 << POW)-12)		/* -12 to avoid malloc problems */
#define MAXNUM ((BUFSIZE+12)/10 - 4)	/* lowish guess at average rec size */
#define BUFFEND (EOF-2)
#define MAXFCT 1000
#define MAXLLEN ((1 << 16) - 14)

extern u_char **keylist, **l2buf, *buffer, *linebuf;

/* 
 * temp files in the stack have a file descriptor, a largest bin (maxb)
 * which becomes the last non-empty bin (lastb) when the actual largest
 * bin is smaller than max(half the total file, BUFSIZE)
 * Max_o is the offset of maxb so it can be sought after the other bins
 * are sorted.
 */
struct tempfile {
	FILE *fd;
	u_char maxb;
	u_char lastb;
	long max_o;
};

extern struct tempfile fstack[MAXFCT];
