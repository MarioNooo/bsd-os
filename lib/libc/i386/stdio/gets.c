/*	BSDI gets.c,v 1.2 1996/10/18 00:32:11 donn Exp	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
static char sccsid[] = "@(#)gets.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Return the number of bytes that can be written at the address 'v'
 * without overwriting a subsequent stack frame.
 */
static size_t
size_to_end_of_frame(v)
	caddr_t v;
{
	caddr_t frame = (caddr_t)&v - 2 * sizeof (char *);

	while (frame && (caddr_t)v >= frame)
		frame = *(caddr_t *)frame;

	return ((size_t)(frame - v));
}

/*
 * This function is inherently unsafe and should NEVER be used by real code.
 * Unfortunately some real code is lazy; we therefore check that we can't
 * overwrite a stack frame, papering over the easiest security hole.
 * (This means that the bad guys have to spend at least 10 more minutes
 * trying to figure out how to break the system.  Well, okay, 5 minutes.)
 */
char *
gets(buf)
	char *buf;
{
	int c;
	char *s;
	size_t limit = size_to_end_of_frame((caddr_t)buf);

	for (s = buf; (c = getchar()) != '\n';) {
		if (c == EOF) {
			if (s == buf)
				return (NULL);
			break;
		}
		if ((size_t)(s - buf) >= limit)
			break;
		*s++ = c;
	}
	if ((size_t)(s - buf) >= limit)
		/* buffer overrun */
		kill(getpid(), SIGSEGV);
	*s = 0;
	return (buf);
}