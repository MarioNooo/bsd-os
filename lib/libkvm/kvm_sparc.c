/*	BSDI kvm_sparc.c,v 2.8 2000/01/24 22:08:06 jch Exp	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA contract
 * BG 91-66 and contributed to Berkeley.
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
static char sccsid[] = "@(#)kvm_sparc.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

/*
 * Sparc machine dependent routines for kvm.  Hopefully, the forthcoming 
 * vm code will one day obsolete this module.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <machine/cpu.h>

#include <sparc/sparc/sparcpmap.h>

#include <db.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kvm_private.h"

#define NPMEG 256	/* as saved in dump image; see sparc/sun4c/pmap44c.c */

/* bank translation `pages' are 4k regardless of h/w page size (8k on sun4) */
#define	PGTOBPG(pg)	(pg)	/* XXX needs shift for sun4 */
#define HWTOSW(pmap_stod, pg) \
	(pmap_stod[PGTOBPG(pg) >> BSHIFT] | (PGTOBPG(pg) & BOFFSET))

/* backwards compatibility */
#define	FLAT_COMPAT
#define	REV1_COMPAT

#ifdef FLAT_COMPAT
#define	FLATSEG(va) (((u_int)(va) >> SGSHIFT) & 0xfff)
#define	NUSEG	FLATSEG(0xf8000000)
struct flat_user_pmap {
	int	fill1[2];		/* context info */
	struct	pmap_statistics pm_stats;
	int	fill2[6];		/* refc; forw/back; seg,npte,pte */
	pmeg_t	pm_rsegmap[NUSEG];
	u_char	pm_rnpte[NUSEG];
	int	*pm_rpte[NUSEG];
};
#endif /* FLAT_COMPAT */

#ifdef REV1_COMPAT
struct rev1_suninfo {
	pmeg_t	ri_segs[NMAP_L2];
	u_char	ri_npte[NMAP_L2];
	int	*ri_pte[NMAP_L2];
	int	ri_nsegs;
};
struct rev1_pmap {
	short	pm_ctxnum;
	short	pm_refcount;
	simple_lock_data_t pm_lock;
	struct	pmap_statistics pm_stats;
	int	pm_misc;
	struct	rev1_suninfo *pm_ri[NMAP_L1];
};
#endif

static u_int kernbase;			/* address of start of kernel */

struct vmstate {
	u_int	vs_nonmapped;		/* kva's < here are in flat phys mem */
	int	vs_pmap_type;		/* for doing mapping */
	int	vs_nl1map_kern;		/* calculated at startup */
	union {				/* kernel mapping data */
		int	*refptes[NMAP_L1][NMAP_L2];
		struct {
			pmeg_t	segmap[NMAP_L1 * NMAP_L2];
			int	sunptes[NPMEG][NPTESG];
		} kern_sun;				/* if sunmmu */
	} vs_kern;
#define	vs_segmap	vs_kern.kern_sun.segmap
#define	vs_sunptes	vs_kern.kern_sun.sunptes
	int	vs_pmap_stod[BTSIZE];	/* sparse (hw) space to dense (sw) */
};

#define	KGET(kd, addr, var) \
	(kvm_read(kd, (u_long)(addr), (char *)&var, sizeof(var)) != sizeof(var))

static void
setkernbase(kd)
	kvm_t *kd;
{
	struct nlist nl[2];

	nl[0].n_name = NL_SYSTEM;
	nl[1].n_name = NULL;
	if (kvm_nlist(kd, nl))
#if	!defined(KERNBASE) && defined(__KERNBASE)
		kernbase = __KERNBASE;
#else
		kernbase = KERNBASE;
#endif
	else
		kernbase = nl[0].n_value - KERNOFFSET;
}

void
_kvm_freevtop(kvm_t *kd)
{

	if (kd->vmst != 0)
		free(kd->vmst);
}

int
_kvm_initvtop(kvm_t *kd)
{
	int i, off, reg;
	u_int endflat, e;
	struct vmstate *vs;
	struct pmap *pm;
	struct refreg *rr[NMAP_L1];
	int *rr_pte[NMAP_L2];
	struct stat st;
	struct nlist nlist[4];

	if (fstat(kd->pmfd, &st) < 0)
		return (-1);
	vs = (struct vmstate *)_kvm_malloc(kd, sizeof(*vs));
	if (vs == NULL)
		return (-1);
	setkernbase(kd);
	kd->vmst = vs;

	vs->vs_nl1map_kern = NMAP_L1 - VA_TO_R(kernbase);
	/*
	 * Until we finish initializing, we have to assume that kernel
	 * addresses are mapped one-to-one with physical pages.  (This
	 * is true for anything in kernel text, data, and bss.)
	 */
	vs->vs_nonmapped = 0xffffffff;

	/*
	 * It's okay to do this nlist separately from the one kvm_getprocs()
	 * does, since the only time we could gain anything by combining
	 * them is if we do a kvm_getprocs() on a dead kernel, which is
	 * not too common.
	 */
	nlist[0].n_name = "_kernel_pmap_store";
	nlist[1].n_name = "_pmap_stod";
	nlist[2].n_name = "_pmap_type";
#define	X_KPMAP	0
#define	X_STOD	1
#define	X_TYPE	2
	nlist[3].n_name = 0;
	(void)kvm_nlist(kd, nlist);

	if (nlist[X_TYPE].n_value == 0) {
#ifdef FLAT_COMPAT
		vs->vs_pmap_type = PMAP_OLD_FLAT;
#else
		_kvm_err(kd, kd->program, "pmap_type: no such symbol");
		return (-1);
#endif
	} else {
		if (KGET(kd, nlist[X_TYPE].n_value, vs->vs_pmap_type)) {
			_kvm_err(kd, kd->program, "cannot read pmap_type");
			return (-1);
		}
	}
	if (nlist[X_STOD].n_value == 0) {
		_kvm_err(kd, kd->program, "pmap_stod: no such symbol");
		return (-1);
	}

	endflat = 0;
	switch (vs->vs_pmap_type) {

	case PMAP_OLD_FLAT:
	case PMAP_OSW3SUN:
	case PMAP_SW3_SUN:
		/* Read segment table. */
		off = st.st_size - ctob(btoc(sizeof(vs->vs_segmap)));
		errno = 0;
		if ((lseek(kd->pmfd, (off_t)off, 0) == -1 && errno != 0) || 
		    read(kd->pmfd, vs->vs_segmap, sizeof(vs->vs_segmap)) < 0) {
			_kvm_err(kd, kd->program, "cannot read segment map");
			return (-1);
		}

		/* Read PTEs. */
		off = st.st_size - ctob(btoc(sizeof(vs->vs_sunptes)) +
		    btoc(sizeof(vs->vs_segmap)));
		errno = 0;
		if ((lseek(kd->pmfd, (off_t)off, 0) == -1 && errno != 0) || 
		    read(kd->pmfd, vs->vs_sunptes, sizeof vs->vs_sunptes) < 0) {
			_kvm_err(kd, kd->program, "cannot read PTE table");
			return (-1);
		}
		break;

	case PMAP_HW3_REF:
		/* Read the kernel's pte pointers out of the kernel pmap. */
		if ((pm = (struct pmap *)nlist[X_KPMAP].n_value) == NULL) {
			_kvm_err(kd, kd->program,
			    "_kernel_pmap_store: no such symbol");
			return (-1);
		}
		if (KGET(kd, &pm->pm_un.un_rm.rm_regs[VA_TO_R(kernbase)], rr)) {
			_kvm_err(kd, kd->program,
			    "cannot read kernel refreg pointers");
			return (-1);
		}
		for (reg = 0; reg < vs->vs_nl1map_kern; reg++) {
			/*
			 * N.B.: rr[reg] is valid even when empty, so we
			 * do not have to check for NULL first.  They are
			 * also out of one-to-one memory (so we can find
			 * them).
			 *
			 * (I suppose we could emulate the hardware instead...)
			 */
			if (KGET(kd, &rr[reg]->rr_pte[0], rr_pte)) {
				_kvm_err(kd, kd->program,
				    "cannot read kern region %x", reg);
				return (-1);
			}
			memcpy(vs->vs_kern.refptes[reg], rr_pte, sizeof rr_pte);
			for (i = 0; i < NMAP_L2; i++) {
				e = (u_int)rr_pte[i] +
				    NMAP_L3 * sizeof(struct refpte);
				if (e >= endflat)
					endflat = e;
			}
		}
		break;

	default:
		_kvm_err(kd, kd->program, "unknown pmap type %d",
		    vs->vs_pmap_type);
		return (-1);
	}

	/* Read sparse-to-dense table. */
	if (KGET(kd, nlist[X_STOD].n_value, vs->vs_pmap_stod)) {
		_kvm_err(kd, kd->program, "cannot read pmap_stod");
		return (-1);
	}

	/* Ready to map. */
	vs->vs_nonmapped = endflat;

	return (0);
}

/*
 * Translate a user virtual address to a physical address.
 */
int
_kvm_uvatop(kvm_t *kd, const struct proc *p, u_long va, u_long *pa)
{
	struct vmstate *vs;
	struct vmspace *vm;
	u_long ptebase;
	int off, pg, ppn, *pte0, reg, seg, sunpte;
	struct refpte refpte;
	int mib[2];
	static int haveptype, ptype;
	size_t size;
	struct refreg *rr;
	struct sunreg *sr;

	if (kernbase == 0)
		setkernbase(kd);
	if (va >= kernbase)
		return (0);
	vm = p->p_vmspace;
	if ((u_long)vm < kernbase) {
		_kvm_err(kd, kd->program, "_kvm_uvatop: corrupt proc");
		return (0);
	}

	vs = kd->vmst;
	reg = VA_TO_R(va);
	seg = VA_TO_S(va);
	pg = VA_VPG(va);	/* XXX needs work for sun4 */
	off = va & PGOFSET;	/* XXX needs work for sun4 */
	if (ISALIVE(kd)) {
		if (!haveptype) {
			mib[0] = CTL_MACHDEP;
			mib[1] = CPU_PMAP_TYPE;
			size = sizeof(ptype);
			if (sysctl(mib, 2, &ptype, &size, NULL, 0) < 0) {
#ifdef FLAT_COMPAT
				ptype = PMAP_OLD_FLAT;
#else
				_kvm_syserr(kd, kd->program,
				    "_kvm_uvatop: sysctl(CPU_PMAP_TYPE)");
				return (0);
#endif
			}
			haveptype = 1;
		}
	} else
		ptype = vs->vs_pmap_type;

	/*
	 * All of the variants below proceed similarly, finding the
	 * kernel virtual address of the vector holding the ptes for
	 * the page, and then the pte itself.  If the page is in an
	 * invalid region or segment, there may not be such a vector.
	 */
	switch (ptype) {

#ifdef FLAT_COMPAT
	case PMAP_OLD_FLAT: {
		struct flat_user_pmap *opm;

		opm = (struct flat_user_pmap *)&vm->vm_pmap;
		ptebase = (u_long)&opm->pm_rpte[FLATSEG(va)];
		goto get1;
	}
#endif

#ifdef REV1_COMPAT
	case PMAP_OSW3SUN: {
		struct rev1_pmap *opm;
		struct rev1_suninfo *ori;

		opm = (struct rev1_pmap *)&vm->vm_pmap;
		if (KGET(kd, &opm->pm_ri[reg], ori) || ori == NULL)
			goto invalid;
		ptebase = (u_long)&ori->ri_pte[seg];
		goto get1;
	}
#endif

	case PMAP_SW3_SUN:
		if (KGET(kd, &vm->vm_pmap.pm_un.un_sm.sm_regs[reg], sr) ||
		    sr == NULL)
			goto invalid;
		ptebase = (u_long)&sr->sr_pte[seg];
#if defined(FLAT_COMPAT) || defined(REV1_COMPAT)
	get1:
#endif
		if (KGET(kd, ptebase, pte0) || pte0 == NULL)
			goto invalid;
		if (KGET(kd, &pte0[pg], sunpte) || (sunpte & PG_V) == 0)
			goto invalid;
		ppn = sunpte & PG_PPN;
		break;

	case PMAP_HW3_REF:
		/*
		 * Given the region, segment, and page, we want:
		 *	vm->vm_pmap.pm_un.un_rm.rm_regs[reg]->rr_pte[seg][pg]
		 * but each pointer can be NULL if the page is invalid.
		 */
		if (KGET(kd, &vm->vm_pmap.pm_un.un_rm.rm_regs[reg], rr) ||
		    rr == NULL)
			goto invalid;
		ptebase = (u_long)&rr->rr_pte[seg];
		if (KGET(kd, ptebase, pte0) || pte0 == NULL)
			goto invalid;
		if (KGET(kd, &pte0[pg], refpte) || refpte.pg_et != SRMMU_ET_PTE)
			goto invalid;
		ppn = refpte.pg_ppn;
		break;

	default:
		abort();
	}

	/*
	 * /dev/mem adheres to the hardware model of physical memory
	 * (with holes in the address space), while crashdumps
	 * adhere to the contiguous software model.
	 */
	if (!ISALIVE(kd))
		ppn = HWTOSW(vs->vs_pmap_stod, ppn);
	*pa = (ppn << PGSHIFT) | off;		
	return (NBPG - off);
invalid:
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);
}

/*
 * Translate a kernel virtual address to a physical address using the
 * mapping information in kd->vm.  Returns the result in pa, and returns
 * the number of bytes that are contiguously available from this 
 * physical address.  This routine is used only for crashdumps.
 */
int
_kvm_kvatop(kvm_t *kd, u_long va0, u_long *pa)
{
	struct vmstate *vs;
	int pg, ppn, *pte0, off, s, sunpte;
	u_long va;
	struct refpte refpte;

	if (kernbase == 0)
		setkernbase(kd);
	if (va0 < kernbase)
		goto invalid;
	va = va0 - kernbase;
	vs = kd->vmst;

	/*
	 * Addresses below the "nonmapped" boundary come from low RAM.
	 * This allows us to bootstrap, and on the sun4m, lets us use
	 * the rest of the kvm library to read PTEs without going through
	 * all of the steps the hardware would (context table, L1 map, L2
	 * map, etc).
	 */
	if (va0 < vs->vs_nonmapped) {
		*pa = va;
		return (vs->vs_nonmapped - va);
	}

	pg = VA_VPG(va);
	off = va & PGOFSET;
	switch (vs->vs_pmap_type) {

	case PMAP_OLD_FLAT:
	case PMAP_OSW3SUN:
	case PMAP_SW3_SUN:
		/*
		 * The crash dump code has stashed the various mappings
		 * for us in this rather convenient layout.
		 */
		s = vs->vs_segmap[VA_TO_RS(va)];
		sunpte = vs->vs_sunptes[s][pg];
		if ((sunpte & PG_V) == 0)
			goto invalid;
		ppn = sunpte & PG_PPN;
		break;

	case PMAP_HW3_REF:
		/*
		 * We flattened somewhat in initvtop, but we still have
		 * to go read the pte itself.
		 */
		pte0 = vs->vs_kern.refptes[VA_TO_R(va)][VA_TO_S(va)];
		if (KGET(kd, &pte0[pg], refpte) || refpte.pg_et != SRMMU_ET_PTE)
			goto invalid;
		ppn = refpte.pg_ppn;
		break;

	default:
		abort();
	}
	ppn = HWTOSW(vs->vs_pmap_stod, ppn);
	*pa = (ppn << PGSHIFT) | off;
	return (NBPG - off);
invalid:
	_kvm_err(kd, 0, "invalid address (%lx)", va0);
	return (0);
}
