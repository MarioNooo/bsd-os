#include <stdio.h>
#include <sys/param.h>
#define _SYS_SYSTM_H_
#define KERNEL
#include <sys/ucred.h>
#include <sys/file.h>
#include <machine/pcpu.h>
#include <machine/vmlayout.h>
#undef KERNEL
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/stat.h>
#include <vm/vm_page.h>
#include <vm/vnode_pager.h>
#include <i386/isa/isa.h>
#include <kvm.h>
#include <nlist.h>
#include <stdlib.h>
#include <limits.h>
#include <a.out.h>


#define	BUFSIZE	65536

#define USAGE "Usage: %s -N namelist -M memfile [flags] val32\n\
	-f val32 Find val32 in dump (as 4 bytes of data), print phys and \n\
			virtual addresses located\n\
	-t val32 Translate physical address to virtual\n\
	-m mask	Only bits set in the mask are compared during a find\n\
			operation, the default mask is all 32 bits set.\n\
	-v	Print verbose information on procs, VM maps, and VM objects\n\
	-d	Print debug information while processing dump\n\
	-s addr	Print single vmspace information (addr=struct vmspace *)\n\
	-p	Print additional information on virtual pages (very verbose)\n"

struct nlist    nl[] = {
	{"vm_page_array"},
#define	X_VM_PAGE_ARRAY		0
	{"first_page"},
#define	X_FIRST_PAGE		1
	{"allproc"},
#define	X_ALLPROC		2
	{"cpuhead"},
#define	X_CPUHEAD		3
	{"kernel_map"},
#define	X_KERNEL_MAP		4
	{"kmem_map"},
#define	X_KMEM_MAP		5
	{"mb_map"},
#define	X_MB_MAP		6
	{"pager_map"},
#define	X_PAGER_MAP		7
	{"vm_page_buckets"},
#define	X_VM_PAGE_BUCKETS	8
	{"vm_page_bucket_count"},
#define	X_VM_PAGE_BUCKET_COUNT	9
	{"basemem"},
#define	X_BASEMEM		10
	{"last_page"},
#define	X_LAST_PAGE		11
	{NL_KERNBASE},
#define	X_KERNBASE		12
	{NULL},
};

#define PHYS_TO_VM_PAGE(pa) \
                (&vm_page_array[atop(pa) - first_page ])
#define atop(x)         (((unsigned)(x)) >> PAGE_SHIFT)

kvm_t          *kvmd;
int	first_page;
int	last_page;
struct vm_page *vm_page_array;

struct ref {
	struct ref *next;
	void *ref;
	void *ref2;
	char *desc;
};

struct xproc {
	struct xproc *next;
	struct proc *va;
	struct proc d;
} *proc_head;

struct xobj {
	struct xobj *next;
	struct vm_object *va;
	struct ref *refs;
	int refcnt;
	struct vm_object d;
} *vmobj_head;

struct xmap {
	struct xmap *next;
	struct vm_map *va;
	struct ref *refs;
	int refcnt;
	struct vm_map d;
} *vmmap_head;

struct xvmspace {
	struct xvmspace *next;
	int refcnt;
	struct ref *refs;
	struct vmspace *va;
	struct vmspace d;
} *vmspace_head;

int vflag = 0;
int dflag = 0;
int pflag = 0;
u_long saddr = 0;

void usage(char *self);
void ppginfo(u_long pa);
void getprocs();
void findmaps();
void getvmspace();
struct xmap *add_map(struct vm_map *mapp, void *ref, char *desc);
void add_obj(struct vm_object *objp, void *, struct vm_map_entry *,
    char *desc);
void runmap(struct xmap *mp);
void getobjs(void);
void mapinfo(void);
struct xobj *findobj(struct vm_object *op);
struct xvmspace *findvmspace(struct vmspace *op);
void objinfo(void);
void pmapinfo(struct xmap *mp);
void pobjinfo(struct xobj *xp);
void ppager(struct pager_struct *pp);
void chkhash(void);
void rd(void *buf, u_long addr, int len, int lineno);
void disp_vmmap(struct vm_map *map);

#define NSYMS	(sizeof(nl)/sizeof(struct nlist))-1

#define RD(var, addr)	rd(&var, (u_long) addr, sizeof(var), __LINE__);

int
main(int ac, char **av)
{
	int             ch;
	char            buf[1024];
	char           *nlistf = NULL;
	char           *memf = NULL;
	struct device   dv;
	int             i;
	int dmp;
	u_long val;
	u_char vstart;
	u_long fmask = 0xffffffff;
	u_char fstart = 0xff;
	u_char *f;
	u_char *fs;
	u_char *fe;
	u_char *s;
	u_char *b;
	int fl;
	int cnt;
	u_long sa;
	u_long resid;
	struct stat st;
	u_long pa;
	int fflag = 0;

	while ((ch = getopt(ac, av, "N:M:vdpf:m:s:t:")) != EOF) {
		switch (ch) {
		case 'N':
			nlistf = optarg;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'f':
			val = strtoul(optarg, NULL, 0);
			fflag = 1;
			break;
		case 't':
			val = strtoul(optarg, NULL, 0);
			break;
		case 's':
			saddr = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			fmask = strtol(optarg, NULL, 0);
			fstart = fmask & 0xff;
			break;
		default:
			usage(av[0]);
		}
	}
	if (nlistf == NULL)
		nlistf = "/bsd";
	if (memf == NULL)
		memf = "/dev/mem";
	if (ac != optind)
		usage(av[0]);

	/* For search */
	if (fflag) {
		vstart = val & 0xff;		/* XXX Assumes little endian */
		vstart &= fmask;
		val &= fmask;
	}

	/* Attach to kernel in question */
	if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL) {
		fprintf(stderr, "kvm_open: %s\n", buf);
		exit(1);
	}
	/* Get the kernel symbols */
	if (kvm_nlist(kvmd, nl) != 0) {
		for (i = 0; i < NSYMS; i++)
			if (nl[i].n_type == 0)
				fprintf(stderr, "kvm_nlist: %s not found\n",
					nl[i].n_name);
		exit(1);
	}

	/* Do space info request if requested */
	if (saddr != 0) {
		disp_vmmap((struct vm_map *)saddr);
		exit(0);
	}

	/* Read up stuff we need */
	RD(vm_page_array, nl[X_VM_PAGE_ARRAY].n_value);
	RD(first_page, nl[X_FIRST_PAGE].n_value);
	RD(last_page, nl[X_LAST_PAGE].n_value);
	if (vflag) {
		printf("Vm_page_array is at 0x%x\n", vm_page_array);
		printf("First page is %5.5x\n", first_page);
		printf("Last page is %5.5x\n", last_page);
	}

	/* Load everything */
	findmaps();

	/* Open dump raw for search */
	if ((dmp = open(memf, O_RDONLY)) < 0)
		err(1,"Couldn't open %s", memf);
	if (fstat(dmp, &st) < 0)
		err(1,"stat of %s", memf);
	if ((st.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "Can't search non-files yet\n");
		exit(1);
	}

	/* If not finding a value just do a phys->virt lookup */
	if (!fflag) {
		ppginfo(val);
		exit(0);
	}

	printf("Finding %8.8x...\n", val);
	b = malloc(BUFSIZE + 4);
	s = b + 4;
	resid = st.st_size;
	cnt = (resid < BUFSIZE) ? resid : BUFSIZE;
	if (read(dmp, s, cnt) != cnt)
		err(1, "read failed");
	resid -= cnt;
	f = s;
	fl = cnt;
	sa = 0;
	while (fl > 3) {
		/*
		 * We get here with:
		 *	f	- points at buffer to search
		 *	fl	- valid data in buffer
		 *	sa	- address in file
		 */
		fe = f + fl - 4;
		fs = f;
		while (f <= fe) {
			if ((*f & fstart) == vstart &&
			    (*(u_long *)f & fmask) == val) {
				pa = sa + (f - fs);
				printf("Found %8.8x at %8.8x\n", *(u_long *)f,
				    pa);
				ppginfo(pa);
			}
			f++;
		}

		sa += fl - 3;
		bcopy(f, b + 1, 3);
		f = b + 1;
		fl = 3;

		cnt = (resid < BUFSIZE) ? resid : BUFSIZE;
		if (cnt > 0) {
			if (read(dmp, s, cnt) != cnt)
				err(1, "read failed");
			resid -= cnt;
			fl += cnt;
		}
	}
}

void
usage(char *self)
{
	fprintf(stderr, USAGE, self);
	exit(1);
}

#define ptok(a) ((a) - ((a) >= basemem ? (IOM_END - basemem) : 0) + nl[X_KERNBASE].n_value)

void
ppginfo(u_long pa)
{
	struct vm_page vp;
	struct xobj *xp;
	struct ref *rp;
	struct vm_map_entry *ep;
	struct vm_map_entry ent;
	int pno;
	vm_offset_t roff;
	vm_offset_t va;
	int basemem;

	RD(basemem, nl[X_BASEMEM].n_value);
	pno = atop(pa);
	printf("%8.8x: page=%5.5x ", pa, pno);
	if (pno <= first_page) {
		printf("vaddr=%8.8x (lowmem)\n", ptok(pa));
		return;
	}
	if (pno > last_page) {
		printf("XXX page out of range\n");
		return;
	}
	RD(vp, (u_long)PHYS_TO_VM_PAGE(pa));
	/* Print vaddr for each place object is referenced */
	printf("obj=%8.8x off=0x%x\n", vp.object, vp.offset);
	if ((xp = findobj(vp.object)) == NULL) {
		printf(" XXX Obj not found\n");
		return;
	}
	for (rp = xp->refs; rp != NULL; rp = rp->next) {
		ep = rp->ref2;
		RD(ent, ep);
		roff = vp.offset - ent.offset;	/* How far into this slice */
		va = ent.start + roff;
		if (va > ent.end)
			continue;
		printf("   %s %8.8x va=%8.8x\n", rp->desc, rp->ref,
			va + (pa & 0xfff));
	}
}

/*
 * Find all vm maps
 */
void
findmaps()
{
	struct xmap *mp;
	struct vm_map *vmp;

	getprocs();
	getvmspace();

	/* Add special maps */
	RD(vmp, nl[X_KERNEL_MAP].n_value);
	if (vflag)
		printf("kernel_map = %8.8x\n", vmp);
	add_map(vmp, (void *)nl[X_KERNEL_MAP].n_value, "kernel_map");

	RD(vmp, nl[X_KMEM_MAP].n_value);
	if (vflag)
		printf("kmem_map = %8.8x\n", vmp);
	add_map(vmp, (void *)nl[X_KMEM_MAP].n_value, "kmem_map");

	RD(vmp, nl[X_MB_MAP].n_value);
	if (vflag)
		printf("mb_map = %8.8x\n", vmp);
	add_map(vmp, (void *)nl[X_MB_MAP].n_value, "mb_map");

	RD(vmp, nl[X_PAGER_MAP].n_value);
	if (vflag)
		printf("pager_map = %8.8x\n", vmp);
	add_map(vmp, (void *)nl[X_PAGER_MAP].n_value, "pager_map");

	/* All top level maps are in, scan the tree for sub and share maps */
	for (mp = vmmap_head; mp != NULL; mp = mp->next)
		runmap(mp);

	/*
	 * We should know about all maps now, run through them all looking
	 * for objects.
	 */
	getobjs();

	/* Check page hash queues */
	chkhash();

	if (vflag) {
		mapinfo();
		objinfo();
	}
}

/*
 * Extract all proc table entries
 */
void
getprocs()
{
	struct proc *pp;
	struct proc *allproc;
	struct proc *curprocs[MAXCPUS];
	int cpu;
	struct proc **exp_prev;
	struct proc *cp;
	struct xproc *xp;
	struct cpuhead chd;
	int pcnt = 0;
	struct pcpu *pcp;

	if (vflag)
		printf("Proc entries\n================\n");
	RD(allproc, nl[X_ALLPROC].n_value);

	/* Get all curprocs */
	bzero(curprocs, sizeof(curprocs));
	RD(chd, nl[X_CPUHEAD].n_value);
	pcp = chd.lh_first;
	while (pcp != NULL) {
		RD(cpu, &pcp->cpuno);
		if (cpu < 0 || cpu >= MAXCPUS) {
			warnx("Per-cpu area hosed, cpuno == %d\n", cpu);
			break;
		}
		RD(curprocs[cpu], &pcp->curproc);
		RD(pcp, &pcp->pc_allcpu.le_next);
	}

	pp = allproc;
	exp_prev = (struct proc **)nl[X_ALLPROC].n_value;
	if (vflag)
		printf("  Paddr      Pid Vmspace  Cmd\n");
	while (pp != NULL) {
		xp = malloc(sizeof(*xp));
		RD(xp->d, pp);
		xp->va = pp;
		xp->next = proc_head;
		proc_head = xp;
		cp = &xp->d;
		if (vflag) {
			for (cpu = 0; cpu < MAXCPUS; cpu++)
				if (curprocs[cpu] == pp)
					break;
			if (cpu != MAXCPUS)
				printf("%x>", cpu);
			else
				printf("  ");
			printf("%8.8x %5d %8.8x %s\n", pp, cp->p_pid,
			    cp->p_vmspace, cp->p_comm);
		}

#if _BSDI_VERSION < 199701
		if (exp_prev != cp->p_prev)
			printf("XXX p_prev exp=%x act=%x\n", 
			    exp_prev, cp->p_prev);
		exp_prev = &pp->p_next;

		pp = cp->p_next;
#else
                if (exp_prev != cp->p_list.le_prev)
                        printf("XXX p_prev exp=%x act=%x\n",
                            exp_prev, cp->p_list.le_prev);
                exp_prev = &pp->p_list.le_next;
                pp = LIST_NEXT(cp, p_list);
#endif
		pcnt++;
	}
	if (vflag)
		printf("Read %d proc entries\n\n", pcnt);
}

/*
 * Read up vmspace entries, print interesting info if possible
 */
void
getvmspace()
{
	struct xproc *cp;
	struct xvmspace *vp;
	struct vmspace *vmp;
	struct xproc *first;
	struct ref *vpref;
	int fflag;
	int vcnt = 0;

	/* First - read unique vmspace entries */
	if (vflag) {
		printf("Vmspace entries\n===============\n");
		printf("Addr     Nent Vsize    Start    End      Pdir\n");
	}
	for (cp = proc_head; cp != NULL; cp = cp->next) {
		/* See if we already have this one */
		for (vp = vmspace_head; vp != NULL; vp = vp->next)
			if (vp->va == cp->d.p_vmspace)
				break;
		if (vp == NULL) {
			vp = malloc(sizeof(struct xvmspace));
			vp->next = vmspace_head;
			vp->refcnt = 0;
			vmspace_head = vp;
			vp->refs= NULL;
		}
		vpref = malloc(sizeof(*vpref));
		vpref->ref = cp;
		vpref->next = vp->refs;
		vp->refs = vpref;
		vp->refcnt++;
		vp->va = cp->d.p_vmspace;
		RD(vp->d, cp->d.p_vmspace);
		vmp = &vp->d;

		add_map(&cp->d.p_vmspace->vm_map, vp->va, "vmspace");

		if (vflag) {
			printf("%8.8x %4d %8.8x %8.8x %8.8x %8.8x\n",
			    cp->d.p_vmspace, vmp->vm_map.nentries,
			    vmp->vm_map.size,
			    vmp->vm_map.header.start, vmp->vm_map.header.end,
			    vmp->vm_pmap.pm_pdir);
		}
		if (vmp->vm_map.pmap != &cp->d.p_vmspace->vm_pmap)
			printf("XXX vm_map.pmap exp=%x act=%x\n",
			    &cp->d.p_vmspace->vm_pmap, vmp->vm_map.pmap);
		vcnt++;
	}
	if (vflag)
		printf("Read %d unique vmspace entries\n\n", vcnt);

	/*
	 * Now run through vmspace entries and report any sharing at
	 * the vmspace level
	 */
	if (!vflag)
		return;

	printf("Vmspace sharing:\n================\n");
	for (vp = vmspace_head; vp != NULL; vp = vp->next) {
		if (vp->refcnt > 1) {
			printf("%8.8x: \n", vp->va);
			for (vpref = vp->refs; vpref != NULL;
			    vpref = vpref->next) {
				cp = vpref->ref;
				printf(" %d (%s)", cp->d.p_pid, cp->d.p_comm);
			}
			printf("\n");
		}
	}
	printf("\n");
}

/*
 * Add a vmmap
 */
struct xmap *
add_map(struct vm_map *mapp, void *ref, char *desc)
{
	struct xmap *xp;
	struct ref *rp;

	for (xp = vmmap_head; xp != NULL; xp = xp->next)
		if (xp->va == mapp)
			break;
	if (xp == NULL) {
		xp = malloc(sizeof(*xp));
		xp->va = mapp;
		RD(xp->d, mapp);
		xp->refcnt = 0;
		xp->refs = NULL;
		xp->next = vmmap_head;
		vmmap_head = xp;
	}
	rp = malloc(sizeof(*rp));
	xp->refcnt++;
	rp->ref = ref;
	rp->desc = desc;
	rp->next = xp->refs;
	xp->refs = rp;
	return (xp);
}

/*
 * Add an object
 */
void
add_obj(struct vm_object *objp, void *mref, struct vm_map_entry *eref,
    char *desc)
{
	struct xobj *xp;
	struct ref *rp;

	for (xp = vmobj_head; xp != NULL; xp = xp->next)
		if (xp->va == objp)
			break;
	if (xp == NULL) {
		xp = malloc(sizeof(*xp));
		xp->va = objp;
		RD(xp->d, objp);
		xp->refcnt = 0;
		xp->refs = NULL;
		xp->next = vmobj_head;
		vmobj_head = xp;
	}
	rp = malloc(sizeof(*rp));
	xp->refcnt++;
	rp->ref = mref;
	rp->ref2 = eref;
	rp->desc = desc;
	rp->next = xp->refs;
	xp->refs = rp;
}

/*
 * Run a single vm map looking for sub maps and share maps
 */
void
runmap(struct xmap *mp)
{
	struct vm_map_entry e;
	struct vm_map_entry *ep;
	struct xmap *submp;

	ep = mp->d.header.next;
	if (dflag)
		printf("Scanning map %8.8x for submaps and share maps\n",
		    mp->va);
	while (ep != &mp->va->header) {
		RD(e, ep);
		if (dflag)
			printf("map entry %8.8x nxt=%8.8x prv=%8.8x\n",
				ep, e.next, e.prev);
		if (e.is_a_map) {
			if (dflag)
				printf("  is a share map: %8.8x\n",
				    e.object.share_map);
			submp = add_map(e.object.share_map, ep,
			    "map_entry(share)");
			runmap(submp);
		} else if (e.is_sub_map) {
			if (dflag)
				printf("  is a submap: %8.8x\n",
				    e.object.share_map);
			submp = add_map(e.object.sub_map, ep, "map_entry(sub)");
			runmap(submp);
		}
		ep = e.next;
	}
	if (dflag)
		printf("Done scanning map %8.8x\n", mp->va);
}

/*
 * Run through all known maps and pick up objects and their refs
 * Also check back links on vm_map_entries.
 */
void
getobjs()
{
	struct xmap *mp;
	struct vm_map_entry e;
	struct vm_map_entry *ep;
	struct vm_map_entry *ep_exp;
	struct vm_object *sp;
	struct vm_object *sp1;
	struct vm_object o;
	struct vm_object o2;

	if (dflag)
		printf("Scanning for objects\n");
	for (mp = vmmap_head; mp != NULL; mp = mp->next) {
		if (dflag)
			printf("Scanning map %8.8x\n", mp->va);
		ep = mp->d.header.next;
		ep_exp = &mp->va->header;
		while (ep != &mp->va->header) {
			RD(e, ep);
			if (dflag)
				printf("entry %8.8x: nxt=%8.8x prv=%8.8x\n",
				    ep, e.next, e.prev);
			if (e.prev != ep_exp) {
				printf("XXX map entry list corrupt: "
				    "map=%8.8x entry=%8.8x exp=%8.8x "
				    "act=%8.8x\n",
				    mp->va, ep, ep_exp, e.prev);
			}
			if (!e.is_a_map && !e.is_sub_map &&
			    e.object.vm_object != NULL) {
				if (dflag)
					printf("   object %8.8x\n",
					    e.object.vm_object);
				add_obj(e.object.vm_object, mp->va, ep, "map");
				RD(o, e.object.vm_object);
				sp = o.shadow;
				sp1 = e.object.vm_object;
				while (sp != NULL) {
					RD(o, sp);
					if (dflag)
						printf("    shadow %8.8x\n",
						    o.shadow);
					add_obj(sp, sp1, ep, "shadow");
					sp1 = sp;
					sp = o.shadow;
				}
			}
			ep_exp = ep;
			ep = e.next;
		}
	}
}

/*
 * Print info about maps
 */
void
mapinfo()
{
	struct xmap *mp;

	printf("Vm maps\n====================================\n");
	for (mp = vmmap_head; mp != NULL; mp = mp->next)
		pmapinfo(mp);
	printf("\n");
}

/*
 * Print info about a kernel map
 */
void
pmapinfo(struct xmap *mp)
{
	struct vm_map *p;
	struct vm_map_entry *mep;
	struct vm_map_entry me;
	struct xobj *xo;
	int cnt = 0;
	struct ref *rp;

	p = &mp->d;
	printf("\nMap %x\n================\n", mp->va);
	printf("        pmap=%8.8x    nentries=%d\n", p->pmap, p->nentries);
	printf("        size=%8.8x        main=%d\n", p->size, p->is_main_map);
	printf("         ref=%-8d    pageable=%d\n", p->ref_count, 
	    p->entries_pageable);
	printf("       start=%8.8x         end=%8.8x\n", p->header.start,
	    p->header.end);
	printf("       first=%8.8x        last=%8.8x\n", p->header.next,
	    p->header.prev);
	mep = p->header.next;
	printf("\n");
	printf("  Entry    Vstart   Vend     Obj/Map  Offset    Prot "
	    "Cow Copy Wired\n");
	while (mep != &mp->va->header) {
		RD(me, mep);
		printf("  %8.8x %8.8x %8.8x %8.8x %8.8x %5x %3x %4x %5x\n",
			mep, me.start, me.end, me.object.vm_object, me.offset,
			me.protection, me.copy_on_write, me.needs_copy,
			me.wired_count);
		if (me.is_a_map)
			printf("\tshare map\n");
		else if (me.is_sub_map)
			printf("\tsub map\n");
		else if ((xo = findobj(me.object.vm_object)) != NULL) {
			printf("    ");
			if (xo->d.pager != NULL)
				ppager(xo->d.pager);
			if (xo->d.paging_offset)
				printf("paging_offset=0x%x",
				    xo->d.paging_offset);
			if (xo->d.shadow != NULL)
				printf(" shadow=%8.8x", xo->d.shadow);
			printf("\n");
		}
		mep = me.next;
		cnt++;
	}
	if (cnt != p->nentries)
		printf("XXX nentries incorrect, actual=%d\n", cnt);

	printf("\nReferences:\n");
	for (rp = mp->refs; rp != NULL; rp = rp->next) {
		printf("   %8.8x %s\n", rp->ref, rp->desc);
		if (strcmp(rp->desc, "vmspace") == 0) {
			struct ref *rp2;
			struct xvmspace *xvm = findvmspace(rp->ref);

			for (rp2 = xvm->refs; rp2 != NULL; rp2 = rp2->next) {
				struct xproc *xpp;

				xpp = rp2->ref;
				printf("      proc %8.8x pid %d %s\n",
				    xpp->va, xpp->d.p_pid, xpp->d.p_comm);
			}
		}
	}
}

/*
 * find an object (but don't add a reference)
 */
struct xobj *
findobj(struct vm_object *op)
{
	struct xobj *xp;

	for (xp = vmobj_head; xp != NULL; xp = xp->next)
		if (xp->va == op)
			break;
	return (xp);
}


/*
 * find an object (but don't add a reference)
 */
struct xvmspace *
findvmspace(struct vmspace *op)
{
	struct xvmspace *xp;

	for (xp = vmspace_head; xp != NULL; xp = xp->next)
		if (xp->va == op)
			break;
	return (xp);
}


/*
 * Print info on all objects
 */
void
objinfo()
{
	struct xobj *xp;

	for (xp = vmobj_head; xp != NULL; xp = xp->next)
		pobjinfo(xp);
}

struct fld {
	int flags;
	char *desc;
} fldesc[] = {
	{ 0x0001,	"inactive" },
	{ 0x0002,	"active" },
	{ 0x0004,	"laundry" },
	{ 0x0008,	"clean" },
	{ 0x0010,	"busy" },
	{ 0x0020,	"wanted" },
	{ 0x0040,	"tabled" },
	{ 0x0080,	"cow" },
	{ 0x0100,	"fictitious" },
	{ 0x0200,	"fake" },
	{ 0x0400,	"filled" },
	{ 0x0800,	"dirty" },
	{ 0x4000,	"pager" },
	{ 0x8000,	"ptpage" },
};

#define	NFD	(sizeof(fldesc) / sizeof(struct fld))

/*
 * Print detailed info on an object
 */
void
pobjinfo(struct xobj *xp)
{
	struct vm_object *vp;
	struct vm_object *svp;
	struct vm_object s;
	struct ref *rp;
	struct vm_page *np;
	struct vm_page **exp_pp;
	struct vm_page pg;
	int i;
	int cnt;

	vp = &xp->d;
	printf("\nObject %x\n================\n", xp->va);
	printf("       flags=%4.4x", vp->flags);
	if (vp->flags & OBJ_CANPERSIST)
		printf(" persist");
	if (vp->flags & OBJ_INTERNAL)
		printf(" internal");
	if (vp->flags & OBJ_ACTIVE)
		printf(" active");
	if (vp->flags & OBJ_KERNEL)
		printf(" kernel");
	printf("\n");
	printf("       pager=%8.8x      offset=%8.8x\n", vp->pager,
	    vp->paging_offset);
	printf("      shadow=%8.8x      offset=%8.8x\n", vp->shadow,
	    vp->shadow_offset);
	printf("        copy=%8.8x    resident=%d\n", vp->copy,
	    vp->resident_page_count);
	printf("         ref=%-8d        size=%8.8x\n", vp->ref_count, 
	    vp->size);
	printf("      paging=%d ", vp->paging_in_progress);
	ppager(vp->pager);
	printf("\n");

	if (vp->shadow) {
		printf(" Shadow   Offset\n");
		svp = vp->shadow;
		s = *vp;
		while (svp != NULL) {
			printf("   %8.8x %8.8x\n", svp, s.shadow_offset);
			RD(s, svp);
			svp = s.shadow;
		}
	}

	printf("\nReferences:\n");
	for (rp = xp->refs; rp != NULL; rp = rp->next)
		printf("   %s %8.8x entry=%8.8x\n", rp->desc, rp->ref,
		    rp->ref2);

	if (!pflag)
		return;
	exp_pp = &xp->va->memq.tqh_first;
	np = vp->memq.tqh_first;
	printf("\nPages:\n");
	printf("  Page     Offset   Phys     Wire Flags\n");
	cnt = 0;
	while (np != NULL) {
		RD(pg, np);

		printf("  %8.8x %8.8x %8.8x %4d %4.4x", np, pg.offset,
		    pg.phys_addr, pg.wire_count, pg.flags);
		for (i = 0; i < NFD; i++)
			if (fldesc[i].flags & pg.flags)
				printf(" %s", fldesc[i].desc);
		printf("\n");

		if (pg.object != xp->va)
			printf("XXX obj %8.8x page %8.8x in obj %8.8x\n",
			    xp->va, np, pg.object);
		if (pg.listq.tqe_prev != exp_pp)
			printf("XXX obj %8.8x page %8.8x listq.tqe_prev "
			    "exp=%8.8x act=%8.8x\n", xp->va, np, exp_pp,
			    pg.listq.tqe_prev);

		exp_pp = &np->listq.tqe_next;
		np = pg.listq.tqe_next;
		cnt++;
	}
	if (cnt != vp->resident_page_count)
		printf("XXX obj %8.8x expected resident_page_count %d\n", 
		    xp->va, cnt);
	printf("\n");
}

/*
 * Check page hash queue integrity
 */
void
chkhash()
{
	struct pglist *page_buckets;
	struct pglist pb;
	int bucket_count;
	struct vm_page pg;
	struct vm_page *np;
	struct vm_page **exp_pp;
	int cnt;
	int i;

	RD(page_buckets, nl[X_VM_PAGE_BUCKETS].n_value);
	RD(bucket_count, nl[X_VM_PAGE_BUCKET_COUNT].n_value);
	if (dflag)
		printf("checking page hash chains\n");
	for (i = 0; i < bucket_count; i++) {
		exp_pp = &page_buckets[i].tqh_first;
		RD(pb, &page_buckets[i]);
		if (dflag)
			printf("bucket %d %8.8x\n", i, &page_buckets[i]);
		np = pb.tqh_first;
		cnt = 0;
		while (np != NULL) {
			RD(pg, np);

			if (dflag)
				printf("%8.8x: nxt=%8.8x prv=%8.8x\n",
				    np, pg.hashq.tqe_next, pg.hashq.tqe_prev);
			if (exp_pp != pg.hashq.tqe_prev)
				printf("XXX page %8.8x hashq.tqe_prev "
				    "exp=%8.8x act=%8.8x\n", np, exp_pp,
				    pg.hashq.tqe_prev);

			exp_pp = &np->hashq.tqe_next;
			np = pg.hashq.tqe_next;
			cnt++;
		}
		if (exp_pp != pb.tqh_last)
			printf("XXX page bucket %d tqh_last"
			    " exp=%8.8x act=%8.8x\n", i, exp_pp, pb.tqh_last);
		if (dflag && cnt)
			printf("Bucket %d page_count=%d\n", i, cnt);
	}
	if (dflag)
		printf("\n");
}

void
ppager(struct pager_struct *pp)
{
	struct pager_struct p;
	struct vnpager vpd;
	
	if (pp == 0) {
		printf("no pager");
		return;
	}
	RD(p, pp);
	switch (p.pg_type) {
	case PG_DFLT:
		printf("default (handle=%8.8x data=%8.8x flags=0x%x)",
		    p.pg_handle, p.pg_data, p.pg_flags);
		break;
	case PG_SWAP:
		printf("swap (handle=%8.8x data=%8.8x flags=0x%x)",
		    p.pg_handle, p.pg_data, p.pg_flags);
		break;
	case PG_VNODE:
		RD(vpd, p.pg_data);
		printf("vnode (handle=%8.8x vp=%8.8x)",
		    p.pg_handle, p.pg_data, vpd.vnp_vp);
		break;
	case PG_DEVICE:
		printf("device (handle=%8.8x data=%8.8x flags=0x%x)",
		    p.pg_handle, p.pg_data, p.pg_flags);
		break;
	default:
		printf("unk %x (handle=%8.8x data=%8.8x flags=0x%x)",
		    p.pg_type, p.pg_handle, p.pg_data, p.pg_flags);
		break;
	}
}

void
rd(void *buf, u_long addr, int len, int lineno)
{
	if (kvm_read(kvmd, addr, buf, len) < 0)
		err(1, "kvm read (0x%x, %d) failed line=%d", addr, len, lineno);
}

/*
 * Print info on a map and its subordinate structures without running
 * the whole tree first.
 */
void
disp_vmmap(struct vm_map *map)
{
	struct xmap *xmp;

	xmp = add_map(map, NULL, "manual");
	runmap(xmp);
	getobjs();
	chkhash();
	mapinfo();
	objinfo();
}
