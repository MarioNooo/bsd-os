/*	BSDI nlist.c,v 2.5 2001/03/20 04:38:46 seebs Exp	*/

/*-
 * Copyright (c) 1990, 1993, 1994
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
static char sccsid[] = "@(#)nlist.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/resource.h>

#include <machine/param.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <nlist.h>
#include <stdio.h>
#include <string.h>

#include "ps.h"

#if	!defined(__KERNBASE) && defined(KERNBASE)
#define	__KERNBASE	KERNBASE
#endif

/* XXX should get all of these via sysctl() */
struct	nlist psnl[] = {
	{"_fscale"},
#define	X_FSCALE	0		/* CTL_VM VM_LOADAVG */
	{"_ccpu"},
#define	X_CCPU		1		/* not yet via sysctl */
	{"_avail_start"},		/* CTL_HW HW_USERMEM */
#define	X_AVAILSTART	2
	{"_avail_end"},			/* CTL_HW HW_USERMEM */
#define	X_AVAILEND	3
	{NL_KERNBASE},
#define	X_KERNBASE	4
	{NULL}
};

fixpt_t	ccpu;				/* kernel _ccpu variable */
int	nlistread;			/* if nlist already read. */
vm_offset_t
	mempages;			/* number of pages of phys. memory */
int	fscale;				/* kernel _fscale variable */
vm_offset_t
	kernbase;

extern kvm_t *kd;

#define kread(x, v) \
	kvm_read(kd, psnl[x].n_value, (char *)&v, sizeof v) != sizeof(v)

int
donlist()
{
	int rval;
	vm_offset_t tmp;

	rval = 0;
	nlistread = 1;
	rval = kvm_nlist(kd, psnl);

	/* 
	 * For compatibility with old kernels, use old default value 
	 * if there is no KERNBASE symbol.
	 */
	if (psnl[X_KERNBASE].n_type != 0) {
		kernbase = psnl[X_KERNBASE].n_value;
	} else {
		/* we pretend this one wasn't an error */
		--rval;
		kernbase = __KERNBASE;
	}
	if (rval != 0) {
		nlisterr(psnl);
		eval = 1;
		return (1);
	}
	if (kread(X_FSCALE, fscale)) {
		warnx("fscale: %s", kvm_geterr(kd));
		eval = rval = 1;
	}
	if (kread(X_AVAILEND, mempages)) {
		warnx("avail_start: %s", kvm_geterr(kd));
		eval = rval = 1;
	}
	if (kread(X_AVAILSTART, tmp)) {
		warnx("avail_end: %s", kvm_geterr(kd));
		eval = rval = 1;
	}
	mempages -= tmp;
	mempages >>= CLSHIFT;
	if (kread(X_CCPU, ccpu)) {
		warnx("ccpu: %s", kvm_geterr(kd));
		eval = rval = 1;
	}
	return (rval);
}

void
nlisterr(nl)
	struct nlist nl[];
{
	int i;

	(void)fprintf(stderr, "ps: nlist: can't find following symbols:");
	for (i = 0; nl[i].n_name != NULL; i++)
		if (nl[i].n_value == 0)
			(void)fprintf(stderr, " %s", nl[i].n_name);
	(void)fprintf(stderr, "\n");
}