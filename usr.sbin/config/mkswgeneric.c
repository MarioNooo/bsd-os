/*	BSDI mkswgeneric.c,v 2.1 2002/04/24 17:34:42 prb Exp	*/

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
 *	@(#)mkioconf.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "template.h"

/*
 * Make bsp_swapgeneric.c if GENERIC is defined and template exists.
 */

static int doswap __P((FILE *));
static int doexterns __P((FILE *));
static int emithdr __P((FILE *));

static struct template templates[] = {
	{ "DEVICES", 0, 0, doswap },
	{ "EXTERNS", 0, 0, doexterns },
};

int
mkswgeneric()
{
	/*
	 * Only build the swapgeneric stuff when we are a generic kernel
	 */
	if (ht_lookup(selecttab, intern("generic")) == NULL)
		return (0);

	template("bsp_swapgeneric.c", emithdr, T_SIMPLE|T_OPTIONAL,
	    templates, sizeof templates / sizeof *templates);

	return (0);
}

static int
doswap(fp)
	register FILE *fp;
{
	struct devbase *db;

	/*
	 * Search through all the devices we know about
	 */
	for (db = allbases; db != NULL; db = db->d_next) {
		/*
		 * that have a major number (these are disks)
		 */
		if (db->d_major < 0)
			continue;
		/*
		 * Did we make on?
		 * pseudo-devices do not have d_ihead set but d_umax != 0
		 * real devices will have d_ihead set and perhaps d_umax
		 */
		if (db->d_umax == 0 && db->d_ihead == NULL)
			continue;

		fprintf(fp, "\t{ &%scd,\tdk_makedev(%d, 0, 0),\t},\n",
			db->d_name, db->d_major);
	}
	return (0);
}

static int
doexterns(fp)
	register FILE *fp;
{
	struct devbase *db;

	/*
	 * Search through all the devices we know about
	 */
	for (db = allbases; db != NULL; db = db->d_next) {
		/*
		 * that have a major number (these are disks)
		 */
		if (db->d_major < 0)
			continue;
		/*
		 * Did we make on?
		 * pseudo-devices do not have d_ihead set but d_umax != 0
		 * real devices will have d_ihead set and perhaps d_umax
		 */
		if (db->d_umax == 0 && db->d_ihead == NULL)
			continue;

		fprintf(fp, "extern struct cfdriver %scd;\n", db->d_name);
	}
	return (0);
}

static int
emithdr(fp)
	register FILE *fp;
{

	if (fprintf(fp, "\
/*\n\
 * MACHINE GENERATED: DO NOT EDIT\n\
 *\n\
 * bsp_swapgeneric.c, from \"%s\"\n\
 */\n\n", conffile) < 0 )
		return (1);
	return (0);
}
