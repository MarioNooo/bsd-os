/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI tdump.c,v 1.5 2002/02/26 04:22:32 db Exp
 */

#define KTR
#define KERNEL
#define NETHER 1

#ifdef __i386__
#define _MPS_APIC_H
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ktr.h>
#include <sys/file.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/user.h>

#include <machine/pcpu.h>
#include <machine/cpu.h>

#undef KERNEL

#include <err.h>
#include <kvm.h>
#include <nlist.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char	*nlistf;
char	*memf;
kvm_t	*kvmd;
int	aflag;
int	vflag;
enum { TIME_CLK, TIME_TIMEVAL }	timetype;
char	omode;
int	cache_str = 1;
int	trace_size;

void *emalloc(size_t);
void usage(char *self);
void getbuf(u_long, char *, int, int);
void getstr(u_int, char *, int);

void printtime_delta(time_t, u_quad_t, u_quad_t);
u_quad_t cvttimeval(struct timeval *tvp);
#define cvttime(tr) \
	(timetype == TIME_CLK ? (tr)->u.ktr_clk : cvttimeval(&(tr)->u.ktr_tv))

struct td *gettraces(void);

#define USAGE "Usage: %s [-N namelist] [-M memfile] [-vn]\n"

#undef KTR_SIZE

#ifdef __i386__
struct nlist nl[] = {
	{ "_cpuhead" },			/* 0 */
	{ "_ktr_size" },		/* 1 */
	{ "_KERNBASE" },		/* 2 */
	{ "_cpu" },			/* 3 */
	{ NULL },
};

#define CPUHEAD		nl[0].n_value
#define	KTR_SIZE	nl[1].n_value
#undef	KERNBASE
#define	KERNBASE	nl[2].n_value
#define	CPU		nl[3].n_value

#define	VALID_ADDR(x) ((x) > (char *)KERNBASE)
#endif /* __i386__ */

#ifdef __sparc__
struct nlist nl[] = {
	{ "_cputyp" },			/* 0 */
	{ "_pc_ktr_buf" },		/* 1 */
	{ "_pc_ktr_idx" },		/* 2 */
	{ "_ktr_size" },		/* 3 */
	{ NULL }
};

#define	CPUTYP		nl[0].n_value
#define	PC_KTR_BUF	nl[1].n_value
#define	PC_KTR_IDX	nl[2].n_value
#define	KTR_SIZE	nl[3].n_value

#define	VALID_ADDR(x) (1) /* XXX (good enough for now) */
#endif /* __sparc__ */

#ifdef __powerpc__
struct nlist nl[] = {
	{ "cpuhead" },			/* 0 */
	{ "ktr_size" },			/* 1 */
	{ "_KERNBASE" },		/* 2 */
	{ "cpu" },			/* 3 */
	{ NULL },
};

#define CPUHEAD		nl[0].n_value
#define	KTR_SIZE	nl[1].n_value
#undef	KERNBASE
#define	KERNBASE	nl[2].n_value
#define	CPU		nl[3].n_value

#define	VALID_ADDR(x) ((x) > (char *)KERNBASE)
#endif /* __powerpc__ */

#define NSYMS	(sizeof(nl) / sizeof(nl[0]) - 1)

/*
 * Trace data structure.  There is one of these per CPU; it tells us
 * where that cpu's most recent ktr activity went.
 */
struct td {
	struct td *next;
	int cpuno;
	int idx;
	int start_idx;
	u_long buf;		/* value for kvm_read to read an entry */
	struct ktr_entry ent;
	int ent_idx;
};

struct sent {
	struct sent *next;
	u_int addr;
	char str[0];
};

#define	SHSIZE	512
#define	SHASH(a)	(((a) >> 4) & (SHSIZE-1))
struct sent *shead[SHSIZE];

#define RD(s,b,l)	getbuf((u_long)(s), (char *)(b), l, __LINE__)

void getbuf(u_long addr, char *buf, int len, int line);
void getstr(u_int addr, char *buf, int len);
void usage(char *self);

int
main(ac, av)
	int ac;
	char **av;
{
	extern char *optarg;
	extern int optind;
	int ch;
	char buf[1024];
	int i;
	u_quad_t clk_delta, clk_start, clk_last, clk;
	struct ktr_entry *cp;
	struct ktr_entry *bcp;
	struct td *tdhead;
	struct td *tp;
	struct td *btp;
	struct timeval *tvp, tv;
	time_t clks_per_usec;
	int tparm[5];
	int bidx;
	char *p;
	char sbuf[5][80];
	char c;

	clks_per_usec = 0;

	while ((ch = getopt(ac, av, "ac:N:M:vn")) != EOF) {
		switch (ch) {
		case 'N':
			nlistf = optarg;
			break;

		case 'M':
			memf = optarg;
			break;

		case 'a':
			aflag = 1;
			break;

		case 'c':
			clks_per_usec = strtol(optarg, NULL, 10);
			if (clks_per_usec <= 0)
				usage(av[0]);
			break;

		case 'v':
			vflag = 1;
			break;

		case 'n':
			cache_str = 0;
			break;

		default:
			usage(av[0]);
		}
	}
	ac -= optind;
	if (ac != 0) {
		usage(av[0]);
	}
	av += optind;

	/* Attach */
	if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf))==NULL) {
		fprintf(stderr,"kvm_open: %s\n", buf);
		return (1);
	}

	/* Get syms */
	if (kvm_nlist(kvmd, nl) != 0) {
		for (i = 0; i < NSYMS; i++)
			if (nl[i].n_type == 0)
				fprintf(stderr, "kvm_nlist: %s not found\n",
				    nl[i].n_name);
		return (1);
	}

	/* Extract trace data (in some machine-dependent way). */
	tdhead = gettraces();

	if (clks_per_usec != 0 && timetype != TIME_CLK)
		err(1, "-c option not applicable, system uses timevals for tracing");

	clk_start = clk_last = 0;
	btp = NULL;
	bidx = 0;
	if (vflag)
		printf("C Index Time Stamp       Delta-T      T from start\n"
		       "= ===== ================ ============ ============\n");
	else if (aflag)
		printf("C Index Time Stamp       Trace\n"
		       "= ===== ================ =====\n");
	else
		printf("C Index Delta-T      Trace\n"
		       "= ===== ============ =====\n");
	for (;;) {
		clk = 0;
		bcp = NULL;
		/* Find the cronologically first entry */
		for (tp = tdhead; tp != NULL; tp = tp->next) {
			if (tp->idx == -1)
				continue;
			if (tp->ent_idx != tp->idx) {
				RD(tp->buf + tp->idx, &tp->ent,
				    sizeof(tp->ent));
				tp->ent_idx = tp->idx;
			}
			cp = &tp->ent;
			if (cvttime(cp) > clk) {
				btp = tp;
				bcp = cp;
				bidx = (u_int)btp->idx / sizeof(tp->ent);
				clk = cvttime(cp);
			}
		}
		if (bcp) {
			i = (u_int)btp->idx / sizeof(btp->ent);
			btp->idx -= sizeof(struct ktr_entry);
			if (btp->idx < 0)
				btp->idx += trace_size;
			if (btp->idx == btp->start_idx)
				btp->idx = -1;
		} else
			break;

		tvp = &tv;
		switch (timetype) {
		case TIME_CLK:
			clk = bcp->u.ktr_clk;
			if (clks_per_usec != 0) {
				clk_delta = clk / clks_per_usec;
				tvp->tv_sec = clk_delta / 1000000;
				tvp->tv_usec = clk_delta % 1000000;
			}
			break;

		case TIME_TIMEVAL:
			tvp = &bcp->u.ktr_tv;
			clk = cvttimeval(&bcp->u.ktr_tv);
			break;
		}

		printf("%x %5x ", btp->cpuno, bidx);
		if (!VALID_ADDR(bcp->ktr_desc)) {
			printf(" --- trace entry corrupt ---\n");
		} else {
			if (aflag || vflag) {
				if (timetype != TIME_CLK || clks_per_usec != 0)
					printf("%9lu.%6.6lu ",
					    (u_long)tvp->tv_sec,
					    (u_long)tvp->tv_usec);
				else
					printf("%16.16qx ", clk);
			}
			if (clk_start == 0) {
				if (!aflag)
					printf("%*s ", aflag ? 16 : 12, "newest");
				if (vflag)
					printf("\n\t");
				clk_start = clk_last = clk;
			} else {
				if (!aflag) {
					printtime_delta(clks_per_usec, clk_last, clk);
					clk_last = clk;
				} 
				if (vflag) {
					printtime_delta(clks_per_usec, clk_start, clk);
					printf("\n\t");
				}
			}
			getstr((u_int)bcp->ktr_desc, buf, 80);
			buf[79] = 0;

			/* XXX cheating... */
			/* Read up strings that are pointed to */
			p = buf;
			i = 0;
			while ((c = *p++) != NULL) {
				if (c != '%')
					continue;
				if ((c = *p++) == NULL)
					break;
				switch (c) {
				case 's':
					RD(*(&bcp->ktr_parm1 + i), sbuf[i], 80);
					sbuf[i][79] = 0;
					tparm[i] = (int) sbuf[i];
					i++;
					break;

				default:
					tparm[i] = *(&bcp->ktr_parm1 + i);
					i++;
					break;
					
				case '%':
					break;
				}
				if (i == 6)
					printf("*** too many %%'s\n");
			}

			printf(buf, tparm[0], tparm[1], tparm[2], tparm[3],
			    tparm[4]);
			printf("\n");
		}
		if (vflag)
			printf("\t%8.8lx %8.8lx %8.8lx %8.8lx %8.8lx\n",
				bcp->ktr_parm1, bcp->ktr_parm2, bcp->ktr_parm3,
				bcp->ktr_parm4, bcp->ktr_parm5);
	}
	return (0);
}

void 
getbuf(u_long addr, char *buf, int len, int line)
{
	if (kvm_read(kvmd, addr, buf, len) < 0) {
		fprintf(stderr, "kvm read(0x%lx, %d) - (line %d)",
		    addr, len, line);
	}
}

void
getstr(u_int addr, char *buf, int len)
{
	int this;
	struct sent *sp;

	if (cache_str) {
		for (sp = shead[SHASH(addr)]; sp != NULL; sp = sp->next) {
			if (addr == sp->addr) {
				strncpy(buf, sp->str, len);
				buf[len - 1] = NULL;
				return;
			}
		}
	}
	while (len) {
		this = len < 32 ? len : 32;
		RD(addr, buf, this);
		if (memchr(buf, 0, this))
			break;
		len -= this;
		buf += this;
		addr += this;
	}
	if (cache_str) {
		buf[len - 1] = NULL;
		sp = emalloc(sizeof(struct sent) + strlen(buf) + 1);
		sp->next = shead[SHASH(addr)];
		shead[SHASH(addr)] = sp;
		sp->addr = addr;
		strcpy(sp->str, buf);
	}
}

u_quad_t
cvttimeval(struct timeval *tv)
{

	return (tv->tv_sec * (u_quad_t)1000000 + tv->tv_usec);
}

void
printtime_delta(time_t clks_per_usec, u_quad_t clk_last, u_quad_t clk)
{
	u_quad_t delta;

	delta = clk_last - clk;
	if (clks_per_usec == 0)
		printf("%12qx ", delta);
	else {
		delta /= clks_per_usec;
		printf("%5qd.%06qd ",
		    delta / 1000000,
		    delta % 1000000);
	}
}

void *
emalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p == NULL)
		err(1, "can't allocate %lu bytes", (u_long)size);
	return (p);
}

#ifdef __i386__
struct td *
gettraces(void)
{
	struct cpuhead cpuhead;
	struct td *lp, *tdhead = NULL;
	struct pcpu *pp;
	int cpu;
	char *bufp;
#define	PADDR(p,x)	((int)(&(((struct pcpu *)(p))->x)))

	/* Get trace buffer size (in bytes) */
	RD(KTR_SIZE, &trace_size, sizeof(trace_size));

	RD(CPU, &cpu, sizeof(cpu));
	timetype = cpu < CPU_586 ? TIME_TIMEVAL : TIME_CLK;

	/* Get head of per cpu chain */
	RD(CPUHEAD, &cpuhead, sizeof(cpuhead));

	/* Get each per-cpu area */
	pp = cpuhead.lh_first;
	while (pp) {
		lp = emalloc(sizeof(*lp));
		RD(PADDR(pp, pc_ktr_idx), &lp->idx, sizeof(int));
		RD(PADDR(pp, pc_ktr_buf), &bufp, sizeof(bufp));
		lp->buf = (u_long)bufp;
		RD(PADDR(pp, cpuno), &lp->cpuno, sizeof(int));
		lp->next = tdhead;
		lp->idx = lp->idx & (trace_size - 1);	/* pre-wrap */
		lp->idx = lp->idx - sizeof(struct ktr_entry);
		if (lp->idx < 0)
			lp->idx += trace_size;
		lp->start_idx = lp->idx;
		lp->ent_idx = -1;
		tdhead = lp;
		RD(PADDR(pp, pc_allcpu.le_next), &pp, sizeof(int *));
	}
	return (tdhead);
}
#endif

#ifdef __sparc__
struct td *
gettraces(void)
{
	int x, cputyp;
	char *pc_ktr_buf;
	int pc_ktr_idx;
	struct td *td;

	/* Get trace buffer size (in bytes) */
	RD(KTR_SIZE, &trace_size, sizeof(trace_size));

	RD(CPUTYP, &cputyp, sizeof(cputyp));
	timetype = cputyp < CPU_SUN4U ? TIME_TIMEVAL : TIME_CLK;

	/* currently only one CPU on sparc */
	RD(PC_KTR_BUF, &pc_ktr_buf, sizeof(pc_ktr_buf));
	RD(PC_KTR_IDX, &pc_ktr_idx, sizeof(pc_ktr_idx));

	td = emalloc(sizeof(*td));
	td->next = NULL;
	td->cpuno = 0;

	td->buf = (u_long)pc_ktr_buf;
	x = pc_ktr_idx & (trace_size - 1);
	td->idx = x;
	td->start_idx = x;
	td->ent_idx = -1;	/* ??? */

	return (td);
}
#endif

#ifdef __powerpc__
struct td *
gettraces(void)
{
	struct cpuhead cpuhead;
	struct td *lp, *tdhead = NULL;
	struct pcpu *pp;
	int cpu;
	char *bufp;
#define	PADDR(p,x)	((int)(&(((struct pcpu *)(p))->x)))

	/* Get trace buffer size (in bytes) */
	RD(KTR_SIZE, &trace_size, sizeof(trace_size));

	RD(CPU, &cpu, sizeof(cpu));
	/* For now always TIME_CLK */
	timetype = TIME_CLK;

	/* Get head of per cpu chain */
	RD(CPUHEAD, &cpuhead, sizeof(cpuhead));

	/* Get each per-cpu area */
	pp = cpuhead.lh_first;
	while (pp) {
		lp = emalloc(sizeof(*lp));
		RD(PADDR(pp, pc_ktr_idx), &lp->idx, sizeof(int));
		RD(PADDR(pp, pc_ktr_buf), &bufp, sizeof(bufp));
		lp->buf = (u_long)bufp;
		RD(PADDR(pp, cpuno), &lp->cpuno, sizeof(int));
		lp->next = tdhead;
		lp->idx = lp->idx & (trace_size - 1);	/* pre-wrap */
		lp->idx = lp->idx - sizeof(struct ktr_entry);
		if (lp->idx < 0)
			lp->idx += trace_size;
		lp->start_idx = lp->idx;
		lp->ent_idx = -1;
		tdhead = lp;
		RD(PADDR(pp, pc_allcpu.le_next), &pp, sizeof(int *));
	}
	return (tdhead);
}
#endif

void
usage(self)
	char *self;
{
	fprintf(stderr, USAGE, self);
	exit(1);
}
