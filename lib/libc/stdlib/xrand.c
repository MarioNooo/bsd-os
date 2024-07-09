/*	BSDI xrand.c,v 2.1 1999/09/16 00:37:19 polk Exp	*/

/*-
 * Copyright (c) 1990, 1993
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
static char sccsid[] = "@(#)rand.c	8.1 (Berkeley) 6/14/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <pthread.h>
#include <stdlib.h>

#include "thread_private.h"

static u_int next = 1;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * We use the ANSI/ISO C standard's example Linear Congruential
 * Random Number Generator here.  It does not work very well, but
 * people think things are broken if you use anything else.
 *
 * This generator produces about 32 bits of value (which we assume
 * fits in an unsigned int -- unsigned long would work on a PDP-11,
 * but the rand_r interface was handed to us engraved in stone).
 * The lower bits are less random than the upper bits, so we assume
 * RAND_MAX is something like 32767 or 65535, and extract the top
 * 16 bits, then reduce them mod RAND_MAX+1 to give a value in
 * [0..RAND_MAX].
 */
#define	UPDATE(x)	((x) = (x) * 1103515245 + 12345)
#define	VALUE(x)	(((x) / 65536) % ((u_int)RAND_MAX + 1))

/* 
 * Provide a reentrant POSIX.4 rand.
 * This permits individual threads to maintain a distinct random
 * number generator.
 */
int
rand_r(seed)
	u_int *seed;
{

	UPDATE(*seed);
	return (VALUE(*seed));
}

/*
 * Traditional rand().  This is modified to be thread safe.
 * The intent is to allow a single rand generator per process
 * that is still callable by multiple threads.
 */
int
rand()
{
	int result;

	if (THREAD_SAFE())
		pthread_mutex_lock(&mutex);
	UPDATE(next);
	result = VALUE(next);
	if (THREAD_SAFE())
		pthread_mutex_unlock(&mutex);
	return (result);
}

void
srand(seed)
	u_int seed;
{

	next = seed;	/* We can assume assignment is atomic. */
}
