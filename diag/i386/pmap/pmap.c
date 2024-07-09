#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#define _SYS_SYSTM_H_
#define KERNEL
#include <sys/ucred.h>
#include <sys/file.h>
#include <machine/pcpu.h>
#undef KERNEL
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
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


#define	BUFSIZE	(128*1024)

#define USAGE "Usage: %s -N namelist -M memfile [flags]\n\
	This will scan a dump image and print the physical page mappings\n\
	for each process (or a single process). A virtual address can be\n\
	given and all pages mapped to that va will be displayed.\n\n\
	Flags:\n\
	-d pdir		Limit operations to pages referred to by directory\n\
				at pdir (a physical page address)\n\
	-i pid		Dump entire map for a process (like above) - or when\n\
			  used with -v or -p, restrict output to that pid\n\
	-v vaddr	Print all pages that are mapped to this virtual addr\n\
	-p paddr	Print all virtual addrs pointing at this physical addr\n\
	-s string-data	Search for ASCII data in physical memory\n\
	-l data		Search for little-endian order word data\n\
	-m mask		Specify significant bits in word data search\n\
	-b		Display bogus mappings\n\
	-V		Verbose\n\
	-D		Debug\n\
	-x		Supress duplicate kernel mappings\n\
	-S memsize	Size of physical memory in MB (for /dev/mem only)\n\
"

struct nlist    nl[] = {
	{"allproc"},
#define	X_ALLPROC	0
	{"cpuhead"},
#define	X_CPUHEAD	1
	{"_NKPTD"},
#define	X_NKPTD		2
	{"_KPTDI_FIRST"},
#define	X_KPTDI_FIRST	3
	{NULL},
};

#define PHYS_TO_VM_PAGE(pa) \
                (&vm_page_array[atop(pa) - first_page ])
#define atop(x)         (((unsigned)(x)) >> PAGE_SHIFT)

kvm_t          *kvmd;
int	first_page;
int	last_page;
struct vm_page *vm_page_array;

struct xproc {
	struct xproc *next;
	struct proc *va;
	struct proc d;
	struct user *uva;
	struct user u;
} *proc_head;

struct igpad {
	struct xproc *p;
	u_long addr;
};

int vflag = 0;
int dflag = 0;
int pflag = 0;
int bflag = 0;
int dmp = -1;
u_long pdir = 0;
u_long pva = 0;
u_long ppa = 0;
int pid;
char *fstr = NULL;
int fflag = 0;
u_long fdata;
u_long fmask = ~0;
u_long memsize = 0;
char *memf = NULL;
int xflag = 1;

#define NSYMS	(sizeof(nl)/sizeof(struct nlist))-1

#define RD(var, addr)	rd(&var, (u_long) addr, sizeof(var), __LINE__);
#define RDP(var, addr)	rdp(&var, (u_long) addr, sizeof(var), __LINE__);

u_long pdp[4096/4];
u_long ptp[4096/4];

int main ( int ac, char **av );
void usage ( char *self );
void getprocs ( void );
void rd ( void *buf, u_long addr, int len, int lineno );
void rdp ( void *buf, u_long addr, int len, int lineno );
void ppdir ( u_long cr3 );
void ppinfo ( u_long va, u_long ent );
void ppflags ( u_long ent );
void vfind ( u_long vaddr );
void pfind ( u_long paddr, int quiet );
void bogus_check ( void );
void pscan ( u_long pa, struct igpad *ok, int nok );
u_long v2p ( u_long va );
void memsearch(void);
void search_found(u_long paddr, u_char *data);

int
main(int ac, char **av)
{
	int             ch;
	char            buf[1024];
	char           *nlistf = NULL;
	int		i;
	struct xproc	*pp;

	while ((ch = getopt(ac, av, "N:M:v:p:d:VDi:bl:m:s:S:x")) != EOF) {
		switch (ch) {
		case 'N':
			nlistf = optarg;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'd':
			pdir = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			ppa = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			pid = strtol(optarg, NULL, 0);
			break;
		case 'v':
			pva = strtoul(optarg, NULL, 0);
			break;
		case 'V':
			vflag = 1;
			break;
		case 'D':
			dflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'l':
			fflag = 1;
			fdata = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			fmask = strtoul(optarg, NULL, 0);
			break;
		case 's':
			fstr = optarg;
			break;
		case 'S':
			memsize = strtoul(optarg, NULL, 0) * 1024 * 1024;
			break;
		case 'x':
			xflag = 0;
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

	/* Open dump raw for search */
	if ((dmp = open(memf, O_RDONLY)) < 0)
		err(1,"Couldn't open %s", memf);

	/* Suck up proc/user data */
	getprocs();

	if (bflag)
		bogus_check();
	if (pdir)
		ppdir(pdir);
	if (pva) {
		vfind(pva);
		pid = 0;
	}
	if (ppa) {
		pfind(ppa, 0);
		pid = 0;
	}
	if (pid) {
		for (pp = proc_head; pp != NULL; pp = pp->next) {
			if (pp->d.p_pid != pid)
				continue;
			ppdir(pp->u.u_pcb.pcb_cr3);
		}
	}

	if (fstr || fflag)
		memsearch();
}

/*
 * Search memory for data or string
 *
 * Somewhat limited in that it won't find strings crossing a 128KB boundary
 * (this isn't really a problem since we scanning physical memory which is
 *  paged)
 */
void
memsearch()
{
	struct stat st;
	u_char *b;
	int flen = 0;
	u_long phys_base;
	u_long resid;
	int this;
	u_char *p;
	u_char c;
	u_char look_c;
	u_long ofs;

	if (fstat(dmp, &st) < 0)
		err(1,"stat of %s", memf);
	if ((st.st_mode & S_IFMT) == S_IFREG) {
		resid = st.st_size;
	} else {
		if (memsize == 0) {
			printf("Must enter memory size (in MB) with -S to "
			    "search non-files\n");
			return;
		}
		resid = memsize;
	}
	if (fflag) {
		printf("Finding %8.8x (mask=%8.8x) (%d Mb)\n", fdata, fmask,
		    resid / (1024 * 1024));
		look_c = *(u_char *)&fdata;
	} else if (fstr) {
		printf("Finding \"%s\" (%d Mb)\n", fstr,
		    resid / (1024 * 1024));
		flen = strlen(fstr);
		look_c = *(u_char *)fstr;
	}
	b = malloc(BUFSIZE + 256);
	bzero(b + BUFSIZE, 256);
	if (fstr && fflag) {
		printf("Warning: can't search for string and data at once, "
		    "searching data only\n");
	}

	phys_base = 0;
	while (resid != 0) {
		this = (resid < BUFSIZE) ? resid : BUFSIZE;
		lseek(dmp, phys_base, SEEK_SET);
		if (read(dmp, b, this) != this)
			err(1, "search read from dump failed");
		if (flen) {
			/* String search */
			for (p = b; p != b + this; p++) {
				c = *p;
				if (c != look_c)
					continue;
				if (bcmp(p, fstr, flen) != 0)
					continue;
				ofs = p - b;
				search_found(phys_base + ofs, p);
			}
		} else {
			/* Data search only */
			for (p = b; p != b + this; p++) {
				u_long lword;

				c = *p;
				if (c != look_c)
					continue;
				lword = *(u_long *)p;	/* Intel only! */
				if ((lword & fmask) != fdata)
					continue;
				ofs = p - b;
				search_found(phys_base + ofs, p);
			}
		}
		phys_base += this;
		resid -= this;
	}
}

/*
 * Search hit
 */
void
search_found(u_long paddr, u_char *data)
{
	static u_long lastfound = ~0;
	u_long dispp;
	u_char *dispd;
	int i, j;

	printf("\n=========> %8.8x\n", paddr);

	/* Hex dump 64 bytes around found data */
	dispd = (u_char *)((u_long)data & ~0x3f);
	dispp = paddr & ~0x3f;
	for (i = 0; i < 4; i++) {
		printf("  %8.8x: ", dispp);
		for (j = 0; j < 16; j++) {
			printf("%2.2x ", dispd[j]);
			if (j == 7)
				printf("  ");
		}
		printf(" ");
		for (j = 0; j < 16; j++) {
			if (isprint(dispd[j]))
				putc(dispd[j], stdout);
			else
				putc('_', stdout);
			if (j == 7)
				putc(' ', stdout);
		}
		printf("\n");
		dispd += 16;
		dispp += 16;
	}

	/* Find mappings first time on this page */
	if (lastfound  == (paddr & ~0xfff))	/* Same page */
		return;
	lastfound = paddr & ~0xfff;
	pfind(paddr, 1);
}

void
usage(char *self)
{
	fprintf(stderr, USAGE, self);
	exit(1);
}

/*
 * Extract all proc table entries
 */
void
getprocs()
{
	struct proc *pp;
	struct proc *allproc;
	struct proc **exp_prev;
	struct proc *cp;
	struct xproc *xp;
	int pcnt = 0;
	struct proc *curprocs[MAXCPUS];
	int cpu;
	struct cpuhead chd;
	struct pcpu *pcp;

	if (dflag)
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
	if (dflag)
		printf("  Paddr    Uaddr      Pid Vmspace  CR3      Cmd\n");
	while (pp != NULL) {
		xp = malloc(sizeof(*xp));
		RD(xp->d, pp);
		xp->va = pp;
		RD(xp->u, xp->d.p_addr);
		xp->next = proc_head;
		proc_head = xp;
		cp = &xp->d;
		if (dflag) {
			for (cpu = 0; cpu < MAXCPUS; cpu++)
				if (curprocs[cpu] == pp)
					break;
			if (cpu != MAXCPUS)
				printf("%x>", cpu);
			else
				printf("  ");
			printf("%8.8x %8.8x %5d %8.8x %8.8x %s\n", pp, 
			    cp->p_addr, cp->p_pid, cp->p_vmspace, 
			    xp->u.u_pcb.pcb_cr3, cp->p_comm);
		}

		if (exp_prev != cp->p_list.le_prev)
			printf("XXX p_prev exp=%x act=%x\n", 
			    exp_prev, cp->p_list.le_prev);
		exp_prev = &pp->p_list.le_next;
		pp = LIST_NEXT(cp, p_list);
		pcnt++;
	}
	if (dflag)
		printf("Read %d proc entries\n\n", pcnt);
}

void
rd(void *buf, u_long addr, int len, int lineno)
{
	if (kvm_read(kvmd, addr, buf, len) < 0)
		err(1, "kvm read (0x%x, %d) failed line=%d", addr, len, lineno);
}

void
rdp(void *buf, u_long addr, int len, int lineno)
{
	if (lseek(dmp, addr, SEEK_SET) < 0)
		err(1, "rdp (0x%x, %d) failed line=%d", addr, len, lineno);
	if (read(dmp, buf, len) < 0)
		err(1, "rdp read (0x%x, %d) failed line=%d", addr, len, lineno);
}

/*
 * Print tree of mapped pages given CR3 (pdir pointer in phys mem)
 */
void
ppdir(u_long cr3)
{
	int di;
	int pi;

	/* Read up the page directory */
	RDP(pdp, cr3);
	for (di = 0; di < 1024; di++) {
		if (!dflag && (pdp[di] & PG_V) == 0)
			continue;
		printf("D  %8.8x: ", cr3 + (di << 2));
		ppinfo(di << PD_SHIFT, pdp[di]);
		if ((pdp[di] & PG_V) == 0)
			continue;
		RDP(ptp, pdp[di] & ~0xfff);
		for (pi = 0; pi < 1024; pi++) {
			if (!dflag && (ptp[pi] & PG_V) == 0)
				continue;
			printf(" P %8.8x: ", (pdp[di] & ~0xfff) + (pi << 2));
			ppinfo(di << PD_SHIFT | pi << 12, ptp[pi]);
		}
	}
}

void
ppinfo(u_long va, u_long ent)
{
	printf("[%8.8x] ", va);
	printf("%8.8x ", ent & ~0xfff);
	ppflags(ent);
}

void
ppflags(u_long ent)
{
	if (ent & PG_V)
		printf("P ");
	else
		printf("  ");
	if (ent & PG_KW)
		printf("Wr ");
	else
		printf("   ");
	if (ent & PG_URKR)
		printf("User ");
	else
		printf("     ");
	if (ent & 0x8)
		printf("WrThru ");
	else
		printf("       ");
	if (ent & 0x10)
		printf("CDis ");
	else
		printf("     ");
	if (ent & PG_U)
		printf("Acc ");
	else
		printf("    ");
	if (ent & PG_M)
		printf("Dirty\n");
	else
		printf("     \n");
}

/*
 * Given a virtual address, display all physical pages mapped to it
 */
void
vfind(u_long vaddr)
{
	int di;
	int pi;
	struct xproc *pp;
	u_long page_addr = vaddr & ~0xfff;
	u_long pda;
	u_long pva;

	printf("Paddr      Pid Command\n");
	printf("======== ===== ==============\n");
	for (pp = proc_head; pp != NULL; pp = pp->next) {
		if (pid && pp->d.p_pid != pid)
			continue;
		/* Read up the page directory */
		RDP(pdp, pp->u.u_pcb.pcb_cr3);
		for (di = 0; di < 1024; di++) {
			if ((pdp[di] & PG_V) == 0)
				continue;
			RDP(ptp, pdp[di] & ~0xfff);
			pda = di << PD_SHIFT;	/* Upper bits of VA */
			for (pi = 0; pi < 1024; pi++) {
				if (!dflag && (ptp[pi] & PG_V) == 0)
					continue;
				pva = pda | pi << 12;
				if (pva == page_addr) {
					printf("%8.8x %5d %-20s ",
					   (ptp[pi] & ~0xfff) + (vaddr & 0xfff),
					    pp->d.p_pid, pp->d.p_comm);
					ppflags(ptp[pi]);
				}
			}
		}
	}
}

/*
 * Given a physical address, display all virtual mappings to it
 */
void
pfind(u_long paddr, int quiet)
{
	int di;
	int pi;
	struct xproc *pp;
	u_long page_addr = paddr & ~0xfff;
	u_long pda;
	u_long page_physaddr;
	int pdflag;
	u_long va;
	u_long lastva;

	if (!quiet) {
		printf("Vaddr      Pid Command\n");
		printf("======== ===== ==============\n");
	}
	lastva = ~0;
	for (pp = proc_head; pp != NULL; pp = pp->next) {
		if (pid && pp->d.p_pid != pid)
			continue;
		/* Read up the page directory */
		RDP(pdp, pp->u.u_pcb.pcb_cr3);
		for (di = 0; di < 1024; di++) {
			if ((pdp[di] & PG_V) == 0)
				continue;
			RDP(ptp, pdp[di] & ~0xfff);
			pda = di << PD_SHIFT;	/* Upper bits of VA */
			pdflag = 0;
			for (pi = 0; pi < 1024; pi++) {
				if (!dflag && (ptp[pi] & PG_V) == 0)
					continue;
				page_physaddr = ptp[pi] & ~0xfff;
				if (page_physaddr == page_addr) {
					if (vflag && !pdflag) {
						printf("--- Ptd=%8.8x (%3x) "
						    "Ptp=%8.8x (%3x)\n", 
						    pp->u.u_pcb.pcb_cr3, di,
						    pdp[di] & ~0xfff, pi);
						pdflag = 1;
					}
					va = pda | (pi << 12) | (paddr & 0xfff);
					if (va >= 0xf0000000 && xflag) {
						if (va == lastva)
							continue;
						lastva = va;
						printf("%8.8x  Kern %20s ", 
						    va, "");
						ppflags(ptp[pi]);
						continue;
					}
					printf("%8.8x %5d %-20s ", va,
					    pp->d.p_pid, pp->d.p_comm);
					ppflags(ptp[pi]);
				}
			}
		}
	}
}

/*
 * Check for obviously bogus mappings
 */
void
bogus_check()
{
	struct xproc *pp;
	int di;
	int pi;
	struct igpad ok[2];
	u_int kptdi_first;
	u_int nkptd;
	u_long ua;
	u_long pa0, pa1;
	u_long *kptd;

#define	USTK1	((UPAGES - 2) << 12)
#define USTK2	((UPAGES - 1) << 12)

	nkptd = nl[X_NKPTD].n_value;
	kptdi_first = nl[X_KPTDI_FIRST].n_value;

	kptd = malloc(nkptd * sizeof(*kptd));

	/*
	 * Check kernel stack pages, they should be mapped at exactly
	 * one place each, the u-area.
	 */
	for (pp = proc_head; pp != NULL; pp = pp->next) {
		if (pid && pp->d.p_pid != pid)
			continue;

		ok[0].p = ok[1].p = pp;
		ua = (u_long)pp->d.p_addr;

		/* Read up the page directory */
		RDP(pdp, pp->u.u_pcb.pcb_cr3);

		/* Check first kstack page */
		ok[0].addr = ua + USTK1;
		pa0 = v2p(ok[0].addr);
		pscan(pa0, ok, 1);

		/* Check second kstack page */
		ok[0].addr = ua + USTK2;
		pa0 = v2p(ok[0].addr);
		pscan(pa0, ok, 2);
	}

	/*
	 * Everyone should have matching kernel PTD entries
	 */
	bcopy(&pdp[kptdi_first], kptd, nkptd * sizeof(u_long));
	for (pp = proc_head; pp != NULL; pp = pp->next) {
		if (pid && pp->d.p_pid != pid)
			continue;

		/* Read up the page directory */
		RDP(pdp, pp->u.u_pcb.pcb_cr3);

		for (di = 0; di < nkptd; di++) {
			if (pdp[kptdi_first + di] != kptd[di]) {
				printf("%8.8x: Kernel PTD mismatch, "
				    "proc %d (%-10s), exp=%x act=%x\n",
				    (kptdi_first + di) << PDRSHIFT,
				    pp->d.p_pid, pp->d.p_comm,
				    kptd[di], pdp[kptdi_first + di]);
			}
		}
		
	}

	free(kptd);
}

/*
 * Scan for mappings on a given physical page and report on any that are 
 * not in the ok table.
 */
void
pscan(u_long pa, struct igpad *ok, int nok)
{
	struct xproc *pp;
	int msgflag = 0;
	u_long xpdp[1024];
	u_long xptp[1024];
	int di, pi;
	int va;
	int hdr;
	u_long page_physaddr;
	int i;

	hdr = 0;
	pa &= ~0xfff;
	for (pp = proc_head; pp != NULL; pp = pp->next) {
		/* Read up the page directory */
		RDP(xpdp, pp->u.u_pcb.pcb_cr3);
		for (di = 0; di < 1024; di++) {
			if ((xpdp[di] & PG_V) == 0)
				continue;
			RDP(xptp, xpdp[di] & ~0xfff);
			for (pi = 0; pi < 1024; pi++) {
				if ((xptp[pi] & PG_V) == 0)
					continue;
				page_physaddr = xptp[pi] & ~0xfff;
				if (page_physaddr != pa)
					continue;

				/* Match, see if its an OK mapping */
				va = di << PD_SHIFT | pi << 12;
				for (i = 0; i < nok; i++) {
					if ((ok[i].p == pp || 
					    va >= 0xf0000000) &&
					    ok[i].addr == va)
						break;
				}
				if (i != nok)
					continue;

				if (!hdr) {
					printf("%8.8x: Illegal mappings found, "
					    "legal mappings are:\n", pa);
					for (i = 0; i < nok; i++)
						printf("\t%d (%-10s): %8.8x\n",
						    ok[i].p->d.p_pid,
						    ok[i].p->d.p_comm,
						    ok[i].addr);
					hdr = 1;
				}
				printf("    Ptd=%8.8x (%3x) "
				    "Ptp=%8.8x (%3x) proc %d (%-10s) %8.8x\n\t",
				    pp->u.u_pcb.pcb_cr3, di,
				    xpdp[di] & ~0xfff, pi, pp->d.p_pid,
				    pp->d.p_comm, va);
				    ppflags(xptp[pi]);
			}
		}
	}
}

/*
 * Convert virtual to physical address assuming pdp is loaded with the
 * pte directory already.
 */
u_long
v2p(u_long va)
{
	int pde_idx;
	u_long pdent;

	pde_idx = va >> PD_SHIFT;
	pdent = pdp[pde_idx];
	if (pdent & PG_V == 0) {
		if (vflag) {
			printf("%8.8x: no pt (%8.8x) ",
			    va, pdent);
			ppflags(pdent);
		}
		return (0);
	}
	RDP(ptp, pdent & ~0xfff);
	return ((ptp[(va >> 12) & 0x3ff] & ~0xfff) | va & 0xfff);
}
