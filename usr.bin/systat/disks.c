/*	BSDI disks.c,v 2.2 2002/01/07 21:17:47 polk Exp	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)disks.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/types.h>
#include <machine/mutex.h>
#include <sys/buf.h>
#include <sys/diskstats.h>

#include <nlist.h>
#include <ctype.h>
#include <paths.h>
#include <string.h>
#include <stdlib.h>
#include "systat.h"
#include "extern.h"
#include "disks.h"

static void dkselect __P((char *, int, int []));

int
dkinit()
{
	register int i;

	if (dr_name != NULL)
		return (1);

	dr_name = kvm_dknames(kd, &dk_ndrive);
	if (dk_ndrive <= 0) {
		error("no disks to display");
		return (0);
	}
	if ((dk_stats = calloc(dk_ndrive + 1, sizeof *dk_stats)) == NULL ||
	    (dk_ostats = calloc(dk_ndrive, sizeof *dk_ostats)) == NULL ||
	    (dk_select = calloc(dk_ndrive, sizeof *dk_select)) == NULL) {
		error("no memory for %d disks", dk_ndrive);
		return (0);
	}

	/* maybe we should just select them all */
	(void)kvm_disks(kd, dk_stats, dk_ndrive);
	for (i = 0; i < dk_ndrive; i++)
		if (dk_stats[i].dk_bpms)
			dk_select[i] = 1;

	return (1);
}

int
dkcmd(cmd, args)
	char *cmd, *args;
{

	if (prefix(cmd, "display") || prefix(cmd, "add")) {
		dkselect(args, 1, dk_select);
		return (1);
	}
	if (prefix(cmd, "ignore") || prefix(cmd, "delete")) {
		dkselect(args, 0, dk_select);
		return (1);
	}
	if (prefix(cmd, "drives")) {
		register int i;

		move(CMDLINE, 0); clrtoeol();
		for (i = 0; i < dk_ndrive; i++)
			if (dk_stats[i].dk_bpms != 0)
				printw("%s ", dr_name[i]);
		return (1);
	}
	return (0);
}

static void
dkselect(args, truefalse, selections)
	char *args;
	int truefalse, selections[];
{
	register char *cp;
	register int i;

	cp = strchr(args, '\n');
	if (cp)
		*cp = '\0';
	for (;;) {
		args += strspn(args, " \t");
		cp = args + strcspn(args, " \t");
		if (cp == args)
			break;
		*cp++ = '\0';
		for (i = 0; i < dk_ndrive; i++)
			if (strcmp(args, dr_name[i]) == 0) {
				if (dk_stats[i].dk_bpms != 0)
					selections[i] = truefalse;
				else
					error("%s: drive not configured",
					    dr_name[i]);
				break;
			}
		if (i >= dk_ndrive)
			error("%s: unknown drive", args);
		args = cp;
	}
}
