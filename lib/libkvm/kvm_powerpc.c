/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	kvm_powerpc.c,v 1.6 2002/08/11 03:24:16 donn Exp
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
 * Power PC kvm routines.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <nlist.h>
#include <kvm.h>
#include <stdlib.h>
#include <stddef.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#include <machine/pmap.h>

struct vmstate {
	u_long kernbase;
	u_long kernel_end;
	u_long page_table;
	u_long page_table_phys;
	u_long pte_count;
	pid_t pid;
	u_long key;
};

#define KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

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
	struct nlist nlist[7];
	u_long avail_end;
	u_long ptbytes;
	u_long bootramdisk_memlen;

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == 0)
		return (-1);
	kd->vmst = vm;

	nlist[0].n_name = "SYSTEM";
	nlist[1].n_name = "page_table";
	nlist[2].n_name = "pmap_pte_count";
	nlist[3].n_name = "mem_size";
	nlist[4].n_name = "bootramdisk";
	nlist[5].n_name = "virtual_avail";
	nlist[6].n_name = 0;

	if (kvm_nlist(kd, nlist)) {
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}

	vm->kernbase = nlist[0].n_value;
	vm->kernel_end = 0xffffffff;	/* until we finish */

	if (KREAD(kd, (u_long)nlist[1].n_value, &vm->page_table)) {
		_kvm_err(kd, kd->program, "can't get page table address");
		return (-1);
	}
	if (KREAD(kd, (u_long)nlist[2].n_value, &vm->pte_count)) {
		_kvm_err(kd, kd->program, "can't get page table size");
		return (-1);
	}
	if (KREAD(kd, (u_long)nlist[3].n_value, &avail_end)) {
		_kvm_err(kd, kd->program, "can't get physical page table offset");
		return (-1);
	}
	if (KREAD(kd, (u_long)nlist[4].n_value + offsetof(struct boot_mem, memlen),
	    &bootramdisk_memlen)) {
		_kvm_err(kd, kd->program, "can't get physical page table offset");
		return (-1);
	}
	ptbytes = vm->pte_count * sizeof (struct pte);
	avail_end -= ppc_round_page(bootramdisk_memlen);
	avail_end -= ptbytes;
	avail_end &= ~(ptbytes - 1);
	vm->page_table_phys = avail_end;
	if (KREAD(kd, (u_long)nlist[5].n_value, &vm->kernel_end)) {
		_kvm_err(kd, kd->program, "can't get kernel size");
		return (-1);
	}
	vm->pid = 0;
	vm->key = 0;
	return (0);
}

static u_int
pte_hash(vm, v)
	struct vmstate *vm;
	u_long v;
{
	u_int vsid = vm->key << 4 | v >> SEGMENT_SHIFT;

	v = (v & HASH_MASK) >> PGSHIFT;
	v ^= vsid;			/* compute primary hash */
	v <<= PTEG_SHIFT;		/* convert to PTE index */
	v &= vm->pte_count - 1;		/* mask off high bits */

	return (v);
}

#define	pteg_alternate(vm, i)	(~(i) & vm->pte_count - 1 & PTEG_MASK)

#define	pteg_pair_next(vm, g) \
	(((g) & NPTE_PTEG ?  ~(~(g) + PTE_LINEAR_REHASH) : \
	    (g) + PTE_LINEAR_REHASH) & vm->pte_count - 1)

static int
_kvm_vatop(kvm_t *kd, u_long va, u_long *pa)
{
	struct pte pte;
	struct pte pbuf[8];
	struct vmstate *vm = kd->vmst;
	struct pte *pt = (struct pte *)vm->page_table;
	int found_end = 0;
	u_int h, rehash;
	u_int pteg, offset;
	u_long w;

	pte.pte_v = 1;
	pte.pte_key = vm->key;
	pte.pte_seg = va >> SEGMENT_SHIFT;
	pte.pte_api = va >> API_SHIFT;

	pteg = pte_hash(vm, va);
	for (rehash = 0; found_end == 0 && rehash < PTE_MAX_REHASH; ++rehash) {
		pte.pte_rehash = rehash;
		for (h = 0; h < 2; ++h) {
			pte.pte_h = h;
			if (kvm_read(kd, (u_long)&pt[pteg], (char *)pbuf,
			    sizeof (pbuf)) != sizeof (pbuf)) {
				_kvm_err(kd, kd->program,
				    "page table read failed");
				return (0);
			}
			for (offset = 0; offset < NPTE_PTEG; ++offset) {
				if ((w = pbuf[offset].pteh.w) == 0) {
					found_end = 1;
					break;
				}
				if (pte.pteh.w == w)
					goto out;
			}
			pteg = pteg_alternate(vm, pteg);
		}
		pteg = pteg_pair_next(vm, pteg);
	}
	_kvm_err(kd, 0, "invalid address (%x)", va);
	return (0);

out:
	*pa = (pbuf[offset].pte_rpn << PGSHIFT) + (va & PGOFSET);
	return (NBPG - (va & PGOFSET));
}

int
_kvm_kvatop(kvm_t *kd, u_long va, u_long *pa)
{
	struct vmstate *vm = kd->vmst;

	if (va >= vm->kernbase && va < vm->kernel_end) {
		*pa = va - vm->kernbase;
		return (NBPG - (va & PGOFSET));
	}
	if (va >= vm->page_table &&
	    va < vm->page_table + vm->pte_count * sizeof (struct pte)) {
		*pa = (va - vm->page_table) + vm->page_table_phys;
		return (NBPG - (va & PGOFSET));
	}
	/* PCI memory space? */

	vm->pid = 0;
	vm->key = 0;

	return (_kvm_vatop(kd, va, pa));
}

/*
 * Translate a user virtual address to a physical address.
 */
int
_kvm_uvatop(kvm_t *kd, const struct proc *p, u_long va, u_long *pa)
{
	struct vmspace *vms = p->p_vmspace;
	struct vmstate *vm;
	u_long kva;
	int key;

	if (kd->vmst == 0 && _kvm_initvtop(kd) == -1)
		return (0);
	vm = kd->vmst;

	if (p->p_pid != vm->pid) {
		kva = (int)&vms->vm_pmap.pm_key;
		if (KREAD(kd, kva, &key)) {
			_kvm_err(kd, 0, "can't recover pmap key");
			return (0);
		}
		vm->pid = p->p_pid;
		vm->key = key;
	}

	return (_kvm_vatop(kd, va, pa));
}
