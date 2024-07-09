/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	kvm_sparc_v9.c,v 2.4 2000/01/24 22:08:06 jch Exp
 *
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering group at Lawrence Berkeley Laboratory.
 *
 * 4. The name of the Laboratory may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

/*
 * SPARC v9 kvm routines.
 */

#ifndef SPARCV9
#define	SPARCV9
#endif

#include <sys/param.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <nlist.h>
#include <kvm.h>
#include <stdlib.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>
#include <unistd.h>

#include "kvm_private.h"

#define	vm_page_size		NBPG

#define	MAX_MA_MAXSIZE	128	/* XXX: >= pmap.c MA_MAXSIZE */

/*
 * We use u_long to represent kernel virtual pointers...
 */
struct vmstate {
	u_long kernbase;
	u_long kernel_end;
	u_long kernel_tsb;
	vm_offset_t kernel_tsb_phys;
	u_long kernel_tsb_range;
	vm_size_t pv_reserve;
	int npm_total;
	struct memarr pmap_totalmem[MAX_MA_MAXSIZE];
};

/* translate V9 phys address to core file offset as appropriate */
#define	V9_XPHYS(kd, pa) (ISALIVE(kd) ? (pa) : _kvm_sparcv9_core_pa(kd, pa))

/* Read an object from a kernel virtual address; return 0 for success.  */
#define KREAD(kd, addr, p) \
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))
/* Read n bytes from a physical address; return 0 for success.  */
#define	PREAD(kd, paddr, p, n) \
	(lseek((kd)->pmfd, (off_t)V9_XPHYS(kd, paddr), SEEK_SET) == -1 || \
	read((kd)->pmfd, p, (n)) != (n))

void
_kvm_freevtop(kvm_t *kd)
{

	if (kd->vmst != 0)
		free(kd->vmst);
}

int
_kvm_initvtop(kvm_t *kd)
{
	struct vmstate *vm;
	static struct nlist nlist[8];
	int invalid;
	int npm_total;
	int size;

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == 0)
		return (-1);
	kd->vmst = vm;
	if (!ISALIVE(kd)) {
		/*
		 * Set up initial one-to-one translation: physical 0
		 * is core file offset 0.
		 */
		vm->npm_total = 0;
	}

	/*
	 * Note peculiarity: "alive" kvm reads /dev/kmem, which
	 * has virtual memory that starts at NL_SYSTEM, but "dead"
	 * (kernel core file) kvm reads core file that starts at
	 * NL_KERNBASE.
	 */
	nlist[0].n_name = ISALIVE(kd) ? NL_SYSTEM : NL_KERNBASE;
	nlist[1].n_name = "kernel_tsb";
	nlist[2].n_name = "kernel_tsb_phys";
	nlist[3].n_name = "kernel_tsb_range";
	nlist[4].n_name = "pv_reserve";
	if (ISALIVE(kd))
		nlist[5].n_name = 0;
	else {
		nlist[5].n_name = "npm_total";
		nlist[6].n_name = "pmap_totalmem";
		nlist[7].n_name = 0;
	}

	invalid = kvm_nlist(kd, nlist);
	if (invalid) {
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}

	vm->kernbase = nlist[0].n_value;
	vm->kernel_end = ULONG_MAX;		/* until we finish */

	if (KREAD(kd, nlist[1].n_value, &vm->kernel_tsb)) {
		_kvm_err(kd, kd->program,
		    "can't get kernel TSB virtual address");
		return (-1);
	}
	if (KREAD(kd, nlist[2].n_value, &vm->kernel_tsb_phys)) {
		_kvm_err(kd, kd->program,
		    "can't get kernel TSB physical address");
		return (-1);
	}
	if (KREAD(kd, nlist[3].n_value, &vm->kernel_tsb_range)) {
		_kvm_err(kd, kd->program, "can't get kernel TSB size");
		return (-1);
	}
	vm->kernel_end = vm->kernbase +
	    (vm->kernel_tsb_range << (PAGE_SHIFT - STTE_SHIFT));
	if (KREAD(kd, nlist[4].n_value, &vm->pv_reserve)) {
		_kvm_err(kd, kd->program, "can't get PV table size");
		return (-1);
	}
	if (!ISALIVE(kd)) {	/* only need translations for dead kernel */
		if (KREAD(kd, nlist[5].n_value, &npm_total))
			goto badphys;
		if (npm_total >= MAX_MA_MAXSIZE) {
			_kvm_err(kd, kd->program,
			    "can't handle %d pmap_totalmem entries (max %d)",
			    npm_total, MAX_MA_MAXSIZE);
			return (-1);
		}
		size = npm_total * sizeof(struct memarr);
		if (kvm_read(kd, nlist[6].n_value,
		    (char *)&vm->pmap_totalmem, size) != size) {
	badphys:
			_kvm_err(kd, kd->program, "can't get phys mem info");
			return (-1);
		}
		vm->npm_total = npm_total;
	}
	return (0);
}

/*
 * Translate a true V9 physical address to a crash dump ("bsdcore")
 * file offset.  This is just a matter of subtracting off holes in
 * the dump space.
 *
 * These "phys address" values should really be off_t, since that is
 * how they are used (to index into /dev/mem or the crash dump).
 */
/*static*/
u_long
_kvm_sparcv9_core_pa(kvm_t *kd, u_long pa)
{
	int n;
	u_long ret;
	u_long endofchunk;
	struct memarr *mp;

	ret = pa;
	/*
	 * The array is sorted, so we do not need to check whether
	 * pa >= mp->addr, and can just stop when pa < mp->addr + mp->len.
	 * If adjacent chunks occur, mp[1].addr == endofchunk, so this
	 * works even then.
	 */
	for (mp = kd->vmst->pmap_totalmem, n = kd->vmst->npm_total; --n > 0;
	    mp++) {
		endofchunk = mp->addr + mp->len;
		if (pa < endofchunk)
			break;
		ret -= mp[1].addr - endofchunk;
	}
	/*
	 * If we get here and n < 0, either we are initializing,
	 * or we have a bad phys address.  Complete the translation
	 * anyway.
	 */
	return (ret);
}

int
_kvm_kvatop(kvm_t *kd, u_long va, u_long *pa)
{
	struct vmstate *vm = kd->vmst;
	u_long tsb_mask = (vm->kernel_tsb_range / sizeof (struct stte)) - 1;
	int off = va & PGOFSET;
	u_long stp;
	u_int64_t data;
	u_long truepa;

	/*
	 * Check direct-mapped kernel VM ranges.
	 */
	if (va >= vm->kernbase && va < vm->kernel_end) {
		*pa = va - vm->kernbase;
		return (NBPG - off);
	}
	if (va >= vm->kernel_tsb &&
	    va < vm->kernel_tsb + vm->kernel_tsb_range + vm->pv_reserve) {
		*pa = (va - vm->kernel_tsb) + vm->kernel_tsb_phys;
		return (NBPG - off);
	}
	/* I/O space? */

	/*
	 * Check page-mapped kernel VM.
	 */
	stp = vm->kernel_tsb + ((va >> PAGE_SHIFT) << STTE_SHIFT);
	if (stp >= vm->kernel_tsb + vm->kernel_tsb_range ||
	    KREAD(kd, stp + STTE_DATA, &data)) {
		_kvm_err(kd, kd->program, "bad kernel virtual address");
		return (0);
	}

	if ((data & DATA_VALID) == 0) {
		_kvm_err(kd, kd->program, "invalid address (%lu)", va);
		return (0);
	}
	truepa = (u_long)(data & DATA_PA) + off;
	*pa = V9_XPHYS(kd, truepa);

	return (NBPG - off);
}

/* Taken directly from pmap.c.  */
static struct stte *
tsb_base(u_int level)
{
	vm_offset_t base;
	size_t len;

	if (level == 0)
		base = UPT_MIN_ADDRESS;
	else {
		len = (size_t)1 << (level * BUCKET_SPREAD_SHIFT +
		    PRIMARY_USER_TSB_MASK_WIDTH +
		    SECONDARY_USER_TSB_BUCKET_SHIFT + STTE_SHIFT);
		/* Guarantee alignment */
		base = UPT_MIN_ADDRESS + len;
	}

	return ((struct stte *)(long)base);
}

/* Altered from tsb_get_bucket() in pmap.c.  */
static u_long
tsb_offset(u_int level, vm_offset_t v)
{
	struct stte *bucket = vtobucket(v, level);
	struct stte *stp;
	u_long bits;

	/*
	 * We need to get a bucket in a secondary TSB.  We assume that we
	 * have just searched the previous level and the page containing
	 * the bucket in the previous level is still locked in the TLB.
	 * We will extract the TTE for the current bucket's page from the
	 * page containing the previous level's bucket.  We locate this
	 * TTE using the significant bits that were added when we went
	 * from the previous level to the current level.  These bits are
	 * the high BUCKET_SPREAD bits of the masked virtual page number,
	 * and if the previous level was zero, the low page number bits
	 * of the bucket (the difference in bits between the width of the
	 * secondary bucket size and the primary bucket size).  We
	 * truncate the bucket address to a page and add the significant
	 * bits as the high page offset bits, then choose the last sTTE
	 * in the bucket.
	 */ 
	bits = (v & (tsb_mask(level) & ~tsb_mask(level - 1)) << PAGE_SHIFT)
	    >> tsb_mask_width(level);
	if (level == 1)
		bits |= ((long)bucket &
		    (SECONDARY_USER_TSB_BUCKET_SIZE - 1 &
			~(PRIMARY_USER_TSB_BUCKET_SIZE - 1)) <<
			    (PAGE_SHIFT - PRIMARY_USER_TSB_BUCKET_SHIFT)) >>
		    (BUCKET_SPREAD_SHIFT +
			(SECONDARY_USER_TSB_BUCKET_SHIFT -
			    PRIMARY_USER_TSB_BUCKET_SHIFT));
	stp = (struct stte *)(long)(trunc_page((u_long)vtobucket(v, level - 1))
	    | bits) + tsb_bucket_size(level - 1) - 1;
	return ((u_long)stp & PAGE_MASK);
}

/*
 * Translate a user virtual address to a physical address.
 */
int
_kvm_uvatop(kvm_t *kd, const struct proc *p, u_long va, u_long *pa)
{
	struct vmspace *vms = p->p_vmspace;
	struct vmstate *vm;
	int bucket_len;
	struct stte bucket[tsb_bucket_size(USER_TSB_DEPTH - 1)];
	u_int64_t data;
	u_long physbucketpage;
	u_long physbucket;
	u_long stp;
	u_long truepa;
	int off = va & PGOFSET;
	int level;
	int b;

	if (kd->vmst == 0 && _kvm_initvtop(kd) == -1)
		return (0);
	vm = kd->vmst;

	if (KREAD(kd, (u_long)&vms->vm_pmap.pm_primary.tte.data, &data)) {
		_kvm_err(kd, kd->program, "can't read pmap for pid %d",
		    p->p_pid);
		return (0);
	}

	va = trunc_page(va);

	/* XXX We should read USER_TSB_DEPTH out of the kernel...  */
	for (level = 0; level < USER_TSB_DEPTH; ++level) {

		if ((data & (DATA_VALID|DATA_TSBTTE)) !=
		    (DATA_VALID|DATA_TSBTTE))
			return (0);

		physbucketpage = (u_long)(data & DATA_PA);
		physbucket = physbucketpage +
		    ((u_long)vtobucket(va, level) & PAGE_MASK);
		bucket_len = tsb_bucket_size(level);

		if (PREAD(kd, physbucket, bucket, bucket_len << STTE_SHIFT)) {
			_kvm_err(kd, kd->program, "corrupted memory image");
			return (0);
		}

		for (b = 0; b < bucket_len; ++b)
			if (tte_match(bucket[b].tte, va)) {
				truepa = (bucket[b].tte.data & DATA_PA) + off;
				goto done;
			}

		if (PREAD(kd, physbucketpage + tsb_offset(level + 1, va) +
		    STTE_DATA, &data, sizeof (data))) {
			_kvm_err(kd, kd->program, "error reading TSB");
			return (0);
		}
	}

	return (0);

done:
	*pa = V9_XPHYS(kd, truepa);
	return (NBPG - off);
}
