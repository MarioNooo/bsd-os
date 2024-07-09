/*
 * Copyright (c) 1995
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
static char sccsid[] = "@(#)vfslist.c	8.1 (Berkeley) 5/8/95";
#endif /* not lint */

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _list {
	const char *name;		/* type name. */
	int skipvfs;			/* 1 if skip, 0 if perform action */
} LIST;

const char *
vfsremap(const char *vfsname)
{
        if (strcmp(vfsname, "iso9660") == 0)
                return("cd9660");
        if (strcmp(vfsname, "null") == 0) 
                return("loopback");
        if (strcmp(vfsname, "netc") == 0) 
                return("tfs");
	return(vfsname);
}

int
checkvfsname(vfsname, vfslist)
	const char *vfsname;
	void *vfslist;
{
	LIST *lp;
	int ret = 0;

	if (vfslist == NULL)
		return (0);

	vfsname = vfsremap(vfsname);
	for (lp = vfslist; lp->name != NULL; ++lp)
		if (strcmp(vfsname, lp->name) == 0)
			return (lp->skipvfs);
		else
			ret |= (1 << lp->skipvfs);

	switch (ret) {
	default:
	case 0:	/* No entries found */
		return (0);
	case 1:	/* no "noxxx" entries found */
		return (1);
	case 2: /* only "noxxx" entries found */
		return (0);
	case 3: /* both "noxxx" and "xxx" entries found */
		return (1);
	}
}

void *
makevfslist(fslist)
	char *fslist;
{
	LIST *list, *lp;
	int cnt;
	char *p;

	if (fslist == NULL)
		return (NULL);
	for (cnt = 0, p = fslist; *p != '\0'; ++p)
		if (*p == ',')
			++cnt;
	if ((lp = list = malloc((size_t)(cnt + 2) * sizeof(LIST))) == NULL)
		err(1, NULL);

	while ((lp->name = strsep(&fslist, ", \t")) != NULL)
		if (lp->name[0] != '\0') {
			if (lp->name[0] == 'n' &&
			    lp->name[1] == 'o' && lp->name[2] != '\0') {
				lp->name += 2;
				lp->skipvfs = 1;
			} else
				lp->skipvfs = 0;
			lp->name = vfsremap(lp->name);
			++lp;
		}
	lp->name = NULL;
	return (list);
}
