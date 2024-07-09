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
static char sccsid[] = "@(#)strings.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * String allocation routines.  Strings handed out here are reclaimed at
 * the top of the command loop each time, so they need not be freed.
 */
#include <sys/types.h>
#include <sys/queue.h>

#include <errno.h>

#include "def.h"
#include "extern.h"

struct entry {					/* Memory list entry. */
	LIST_ENTRY(entry) q;
	void *addr;
};
static LIST_HEAD(listhead, entry) head;		/* Head of memory list. */

/*
 * salloc --
 *	Allocate size bytes of space and return the address to the caller.
 */
void *
salloc(size, old, new)
	size_t size;
	char *old, *new;
{
	struct entry *lp;
	size_t len, s;
	char *p;

	/* Allocate the space. */
	if (old == NULL) {
		len = 0;
		p = malloc(size);
		if (p == NULL)
			panic(strerror(ENOMEM));
		lp = LIST_END(&head);
	} else {
		len = strlen(old) + 1;		/* +1 for adding a comma */
		p = realloc(old, len + size);
		if (p == NULL)
			panic(strerror(ENOMEM));
		*(p + len - 1) = ',';	/* Add a comma between strings */ 
					/* Will be changed to a space */
					/* for merged subject lines */

		/* Find the old entry in the string list */
		for (lp = LIST_FIRST(&head); lp != LIST_END(&head);
		    lp = LIST_NEXT(lp, q))
			if (lp->addr == old)
				break;
	}

	if (lp == LIST_END(&head)) {
		lp = malloc(sizeof(struct entry));
		if (lp == NULL)
			panic(strerror(ENOMEM));

		/* Add the string to the current list. */
		LIST_INSERT_HEAD(&head, lp, q);
	}
	lp->addr = p;

	/* Copy any new and/or appended string into place. */
	if (new != NULL)
		strcpy(p + len, new);

	return (p);
}

/*
 * Reset the string area to be empty.
 * Called to free all strings allocated
 * since last reset.
 */
void
sreset()
{
	struct entry *lp;

	while ((lp = LIST_FIRST(&head)) != NULL) {
		LIST_REMOVE(lp, q);
		free(lp->addr);
		free(lp);
	}
}

/*
 * Make the string area permanent.
 * Meant to be called in main, after initialization.
 */
void
spreserve()
{
	struct entry *lp;

	while ((lp = LIST_FIRST(&head)) != NULL) {
		LIST_REMOVE(lp, q);
		free(lp);
	}
}
