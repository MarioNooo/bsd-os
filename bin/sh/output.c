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
static char sccsid[] = "@(#)output.c	8.2 (Berkeley) 5/4/95";
#endif /* not lint */

/*
 * Shell output routines.  We use our own output routines because:
 *	When a builtin command is interrupted we have to discard
 *		any pending output.
 *	When a builtin command appears in back quotes, we want to
 *		save the output of the command in a region obtained
 *		via malloc, rather than doing a fork and reading the
 *		output of the command via a pipe.
 *	Our output routines may be smaller than the stdio routines.
 */

#include <sys/ioctl.h>

#include <stdio.h>	/* defines BUFSIZ */
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"


#define OUTBUFSIZ BUFSIZ
#define BLOCK_OUT -2		/* output to a fixed block of memory */
#define MEM_OUT -3		/* output to dynamically allocated memory */
#define OUTPUT_ERR 01		/* error occurred on output */


struct output output = {NULL, 0, NULL, OUTBUFSIZ, 1, 0};
struct output errout = {NULL, 0, NULL, 100, 2, 0};;
struct output memout = {NULL, 0, NULL, 0, MEM_OUT, 0};
struct output *out1 = &output;
struct output *out2 = &errout;



#ifdef mkinit

INCLUDE "output.h"
INCLUDE "memalloc.h"

RESET {
	out1 = &output;
	out2 = &errout;
	if (memout.buf != NULL) {
		ckfree(memout.buf);
		memout.buf = NULL;
	}
}

#endif


#ifdef notdef	/* no longer used */
/*
 * Set up an output file to write to memory rather than a file.
 */

void
open_mem(block, length, file)
	char *block;
	int length;
	struct output *file;
	{
	file->nextc = block;
	file->nleft = --length;
	file->fd = BLOCK_OUT;
	file->flags = 0;
}
#endif


void
out1str(p)
	const char *p;
	{
	outstr(p, out1);
}


void
out2str(p)
	const char *p;
	{
	outstr(p, out2);
}


void
outstr(p, file)
	register const char *p;
	register struct output *file;
	{
	while (*p)
		outc(*p++, file);
	if (file == out2)
		flushout(file);
}


char out_junk[16];


void
emptyoutbuf(dest)
	struct output *dest;
	{
	int offset;

	if (dest->fd == BLOCK_OUT) {
		dest->nextc = out_junk;
		dest->nleft = sizeof out_junk;
		dest->flags |= OUTPUT_ERR;
	} else if (dest->buf == NULL) {
		INTOFF;
		dest->buf = ckmalloc(dest->bufsize);
		dest->nextc = dest->buf;
		dest->nleft = dest->bufsize;
		INTON;
	} else if (dest->fd == MEM_OUT) {
		offset = dest->bufsize;
		INTOFF;
		dest->bufsize <<= 1;
		dest->buf = ckrealloc(dest->buf, dest->bufsize);
		dest->nleft = dest->bufsize - offset;
		dest->nextc = dest->buf + offset;
		INTON;
	} else {
		flushout(dest);
	}
	dest->nleft--;
}


void
flushall() {
	flushout(&output);
	flushout(&errout);
}


void
flushout(dest)
	struct output *dest;
	{

	if (dest->buf == NULL || dest->nextc == dest->buf || dest->fd < 0)
		return;
	if (xwrite(dest->fd, dest->buf, dest->nextc - dest->buf) < 0)
		dest->flags |= OUTPUT_ERR;
	dest->nextc = dest->buf;
	dest->nleft = dest->bufsize;
}


void
freestdout() {
	INTOFF;
	if (output.buf) {
		ckfree(output.buf);
		output.buf = NULL;
		output.nleft = 0;
	}
	INTON;
}


#ifdef __STDC__
void
outfmt(struct output *file, char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	doformat(file, fmt, ap);
	va_end(ap);
}


void
out1fmt(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	doformat(out1, fmt, ap);
	va_end(ap);
}

void
dprintf(char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	doformat(out2, fmt, ap);
	va_end(ap);
	flushout(out2);
}

void
fmtstr(char *outbuf, int length, char *fmt, ...) {
	va_list ap;
	struct output strout;

	va_start(ap, fmt);
	strout.nextc = outbuf;
	strout.nleft = length;
	strout.fd = BLOCK_OUT;
	strout.flags = 0;
	doformat(&strout, fmt, ap);
	outc('\0', &strout);
	if (strout.flags & OUTPUT_ERR)
		outbuf[length - 1] = '\0';
}

#else /* not __STDC__ */

void
outfmt(va_alist)
	va_dcl
	{
	va_list ap;
	struct output *file;
	char *fmt;

	va_start(ap);
	file = va_arg(ap, struct output *);
	fmt = va_arg(ap, char *);
	doformat(file, fmt, ap);
	va_end(ap);
}


void
out1fmt(va_alist)
	va_dcl
	{
	va_list ap;
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
	doformat(out1, fmt, ap);
	va_end(ap);
}

void
dprintf(va_alist)
	va_dcl
	{
	va_list ap;
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
	doformat(out2, fmt, ap);
	va_end(ap);
	flushout(out2);
}

void
fmtstr(va_alist)
	va_dcl
	{
	va_list ap;
	struct output strout;
	char *outbuf;
	int length;
	char *fmt;

	va_start(ap);
	outbuf = va_arg(ap, char *);
	length = va_arg(ap, int);
	fmt = va_arg(ap, char *);
	strout.nextc = outbuf;
	strout.nleft = length;
	strout.fd = BLOCK_OUT;
	strout.flags = 0;
	doformat(&strout, fmt, ap);
	outc('\0', &strout);
	if (strout.flags & OUTPUT_ERR)
		outbuf[length - 1] = '\0';
}
#endif /* __STDC__ */

/*
 * Push formatted characters (in a stdio buffer) out to their final
 * destination via `op'.
 */
static int
sendout(op0, buf, len)
	void *op0;
	const char *buf;
	int len;
{
	struct output *op = op0;
	int i;

	for (i = 0; i < len; i++)
		outc(*buf++, op);
	return (len);
}

/*
 * Just like vfprintf, but first argument is a `struct output *' rather
 * than a FILE *.
 */
void
doformat(dest, f, ap)
	struct output *dest;
	char *f;				/* format string */
	va_list ap;
{
	FILE *ofp;
	char *p;
	char lastresort[1024];

	if ((ofp = fwopen(dest, sendout)) == NULL) {
		(void)vsnprintf(lastresort, sizeof(lastresort), f, ap);
		for (p = lastresort; *p; p++)
			outc(*p, dest);
	} else {
		(void)vfprintf(ofp, f, ap);
		(void)fclose(ofp);
	}
}



/*
 * Version of write that resumes after a signal is caught.
 */

int
xwrite(fd, buf, nbytes)
	int fd;
	char *buf;
	int nbytes;
	{
	int ntry;
	int i;
	int n;

	n = nbytes;
	ntry = 0;
	for (;;) {
		i = write(fd, buf, n);
		if (i > 0) {
			if ((n -= i) <= 0)
				return nbytes;
			buf += i;
			ntry = 0;
		} else if (i == 0) {
			if (++ntry > 10)
				return nbytes - n;
		} else if (errno != EINTR) {
			return -1;
		}
	}
}


/*
 * Version of ioctl that retries after a signal is caught.
 * XXX unused function
 */

int
xioctl(fd, request, arg) 
	int fd;
	unsigned long request;
	char * arg;
{
	int i;

	while ((i = ioctl(fd, request, arg)) == -1 && errno == EINTR);
	return i;
}
