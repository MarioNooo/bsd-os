/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strip_aout.c,v 2.5 2001/10/03 17:29:58 polk Exp
 */

/*
 * Copyright (c) 1988, 1993
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

#include <sys/param.h>
#include <a.out.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "strip.h"

#ifdef	OMAGIC
int
is_aout(v, size)
	const void *v;
	size_t size;
{
	const struct exec *a;

	a = v;
	if (size < sizeof(*a) || N_BADMAG(*a))
		return (0);
	return (1);
}

ssize_t
aout_sym(v, size)
	void *v;
	size_t size;
{
	struct exec *ep = v;

	/* If no symbols or data/text relocation info, quit. */
	if (!ep->a_syms && !ep->a_trsize && !ep->a_drsize)
		return (size);

	/* Set symbol size and relocation info values to 0. */
	ep->a_syms = ep->a_trsize = ep->a_drsize = 0;

	/* New file size is the header plus text and data segments.  */
	return (N_DATOFF(*ep) + ep->a_data);
}

ssize_t
aout_stab(v, size)
	void *v;
	size_t size;
{
	int cnt, len;
	char *nstr, *nstrbase, *p, *strbase;
	struct nlist *sym, *nsym;
	struct nlist *symbase;
	struct exec *ep = v;
	char *base = v;

	/* Quit if no symbols. */
	if (ep->a_syms == 0)
		return (size);

	/*
	 * Initialize old and new symbol pointers.  They both point to the
	 * beginning of the symbol table in memory, since we're deleting
	 * entries.
	 */
	sym = nsym = symbase = (struct nlist *)(base + N_SYMOFF(*ep));

	/*
	 * Allocate space for the new string table, initialize old and
	 * new string pointers.  Handle the extra long at the beginning
	 * of the string table.
	 */
	strbase = base + N_STROFF(*ep);
	if ((nstrbase = malloc((size_t)*(u_long *)strbase)) == NULL)
		err(1, NULL);
	nstr = nstrbase + sizeof(u_long);

	/*
	 * Read through the symbol table.  For each non-debugging symbol,
	 * copy it and save its string in the new string table.  Keep
	 * track of the number of symbols.
	 */
	for (cnt = ep->a_syms / sizeof(struct nlist); cnt--; ++sym)
		if (!(sym->n_type & N_STAB) && sym->n_un.n_strx) {
			*nsym = *sym;
			nsym->n_un.n_strx = nstr - nstrbase;
			p = strbase + sym->n_un.n_strx;
			len = strlen(p) + 1;
			bcopy(p, nstr, len);
			nstr += len;
			++nsym;
		}

	/* Fill in new symbol table size. */
	ep->a_syms = (nsym - symbase) * sizeof(struct nlist);

	/* Fill in the new size of the string table. */
	*(u_long *)nstrbase = len = nstr - nstrbase;

	/*
	 * Copy the new string table into place.  Nsym should be pointing
	 * at the address past the last symbol entry.
	 */
	bcopy(nstrbase, nsym, len);

	/* Return the size of the stripped binary.  */
	return ((caddr_t)nsym + len - (caddr_t)ep);
}
#endif
