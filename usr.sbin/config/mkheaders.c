/* 
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *
 *	@(#)mkheaders.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

static int emitcnt __P((struct nvlist *));
static int err __P((const char *, char *, FILE *));

/*
 * Make headers containing counts, as needed.
 */
int
mkheaders()
{
	register struct files *fi;

	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_HIDDEN) == 0)
			if (fi->fi_flags & (FI_NEEDSCOUNT | FI_NEEDSFLAG) &&
			    emitcnt(fi->fi_optf))
				return (1);
	}
	return (0);
}

static int
emitcnt(head)
	register struct nvlist *head;
{
	register struct nvlist *nv;
	register FILE *fp;
	register char *fname;
	int cnt;
	char nam[100], ubuf[100];
	char buf[LINE_MAX];

	(void)sprintf(buf, "%s.h", head->nv_name);
	fname = path(buf);
	if ((fp = fopen(fname, "r")) == NULL)
		goto writeit;
	nv = head;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (nv == NULL)
			goto writeit;
		if (sscanf(buf, "#define N%s %d", nam, &cnt) != 2 ||
		    strcmp(nam, mkupper(nv->nv_name, ubuf)) != 0 ||
		    cnt != nv->nv_int)
			goto writeit;
		nv = nv->nv_next;
	}
	if (ferror(fp))
		return (err("read", fname, fp));
	(void)fclose(fp);
	if (nv == NULL)
		return (0);
writeit:
	if ((fp = fopen(fname, "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write %s: %s\n",
		    fname, strerror(errno));
		return (1);
	}
	for (nv = head; nv != NULL; nv = nv->nv_next)
		if (fprintf(fp, "#define\tN%s\t%d\n",
		    mkupper(nv->nv_name, ubuf), nv->nv_int) < 0)
			return (err("writ", fname, fp));
	if (fclose(fp))
		return (err("writ", fname, NULL));
	return (0);
}

static int
err(what, fname, fp)
	const char *what;
	char *fname;
	FILE *fp;
{

	(void)fprintf(stderr, "config: error %sing %s: %s\n",
	    what, fname, strerror(errno));
	if (fp)
		(void)fclose(fp);
	free(fname);
	return (1);
}