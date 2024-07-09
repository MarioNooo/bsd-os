/*
 * Copyright (c) 1980, 1986, 1991, 1993
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
static char copyright[] =
"@(#) Copyright (c) 1980, 1986, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 3/1/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/ttystats.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct _disk {
	struct	cpustats cp_stats;
	struct	diskstats *dk_stats;
} cur, last;

#define MAXDRIVES 2

struct	vmmeter sum;
char	**dr_name;
int	*dr_select;
int	dk_ndrive;
int	ndrives;
int	winlines = 20;
int	stathz;
int	hdrcnt;

kvm_t *kd;

#define	FORKSTAT	0x01
#define	INTRSTAT	0x02
#define	MEMSTAT		0x04
#define	SUMSTAT		0x08
#define	TIMESTAT	0x10
#define	VMSTAT		0x20

double	cpu_etime __P((void));
void	cpustats __P((double));
void	dkstats __P((double));
void	doforkst __P((void));
void	dointr __P((void));
void	domem __P((void));
void	dosum __P((void));
void	dotimes __P((void));
void	dovmstat __P((u_int, int));
void   *ecalloc __P((size_t, size_t));
char  **getdrivedata __P((char **));
long	getuptime __P((void));
void	knlist __P((struct nlist *));
void	knread __P((struct nlist *, void *, size_t));
void	needhdr __P((int));
int	pct __P((long, long));
void	printhdr __P((void));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int c, todo;
	u_int interval;
	int reps;
	char *memf, *nlistf;
        char errbuf[_POSIX2_LINE_MAX];

	memf = nlistf = NULL;
	interval = reps = todo = 0;
	while ((c = getopt(argc, argv, "c:fiM:mN:stw:")) != EOF) {
		switch (c) {
		case 'c':
			reps = atoi(optarg);
			break;
		case 'f':
			todo |= FORKSTAT;
			break;
		case 'i':
			todo |= INTRSTAT;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			todo |= MEMSTAT;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 's':
			todo |= SUMSTAT;
			break;
		case 't':
			todo |= TIMESTAT;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (todo == 0)
		todo = VMSTAT;

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());

        if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf)) == NULL)
		errx(1, "kvm_openfiles: %s", errbuf);

	if (todo & VMSTAT) {
		struct winsize winsize;

		argv = getdrivedata(argv);
		winsize.ws_row = 0;
		(void) ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&winsize);
		if (winsize.ws_row > 0)
			winlines = winsize.ws_row;

	}

#ifdef BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval) {
		if (!reps)
			reps = -1;
	} else if (reps)
		interval = 1;

	if (todo & FORKSTAT)
		doforkst();
	if (todo & MEMSTAT)
		domem();
	if (todo & SUMSTAT)
		dosum();
	if (todo & TIMESTAT)
		dotimes();
	if (todo & INTRSTAT)
		dointr();
	if (todo & VMSTAT)
		dovmstat(interval, reps);
	exit(0);
}

void
knlist(nl)
	struct nlist *nl;
{
	int i;

	if ((i = kvm_nlist(kd, nl)) == 0)
		return;
	if (i < 0)
		errx(1, "kvm_nlist: %s", kvm_geterr(kd));
	(void)fprintf(stderr, "vmstat: undefined symbols:");
	for (; nl->n_name != NULL; nl++)
		if (nl->n_type == 0)
			fprintf(stderr, " %s", nl->n_name);
	(void)fputc('\n', stderr);
	exit(1);
}

void
knread(np, addr, size)
	struct nlist *np;
	void *addr;
	size_t size;
{
	char *p;

	if (kvm_read(kd, np->n_value, addr, size) == size)
		return;
	p = np->n_name;
	if (*p == '_')
		p++;
	errx(1, "can't get %s: %s", p, kvm_geterr(kd));
}

/*
 * calloc that dies on failure
 */
void *
ecalloc(s1, s2)
	size_t s1, s2;
{
	void *p;

	if ((p = calloc(s1, s2)) == NULL)
		err(1, "calloc(%lu, %lu)", (u_long)s1, (u_long)s2);
	return (p);
}

char **
getdrivedata(argv)
	char **argv;
{
	extern char *defdrives[];
	int i;
	char **cp;

	dk_ndrive = 0;
	dr_name = kvm_dknames(kd, &dk_ndrive);
	if (dr_name == NULL)
		warnx("disk names: %s", kvm_geterr(kd));
#ifdef notdef
	if (dk_ndrive == 0)
		warnx("no disks");
#endif
	dr_select = ecalloc(dk_ndrive, sizeof(*dr_select));
	cur.dk_stats = calloc(dk_ndrive + 1, sizeof(*cur.dk_stats));
	last.dk_stats = calloc(dk_ndrive, sizeof(*last.dk_stats));

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments, default drives.  If everything isn't filled
	 * in and there are drives not taken care of, display the first few
	 * that fit.
	 */
	for (ndrives = 0; *argv; ++argv) {
#ifdef BACKWARD_COMPATIBILITY
		if (isdigit(**argv))
			break;
#endif
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], *argv))
				continue;
			dr_select[i] = 1;
			++ndrives;
			break;
		}
	}
	for (i = 0; i < dk_ndrive && ndrives < MAXDRIVES; i++) {
		if (dr_select[i])
			continue;
		for (cp = defdrives; *cp; cp++)
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				++ndrives;
				break;
			}
	}
	for (i = 0; i < dk_ndrive && ndrives < MAXDRIVES; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		++ndrives;
	}
	return (argv);
}

long
getuptime()
{
	time_t now, uptime;
	struct timeval boottime;

	if (kvm_boottime(kd, &boottime))
		errx(1, "can't get boot time: %s", kvm_geterr(kd));
	(void)time(&now);
	uptime = now - boottime.tv_sec;

#define	YEAR (60L * 60 * 24 * 365)
	if (uptime <= 0 || uptime > 10 * YEAR)
		errx(1, "time makes no sense; namelist must be wrong.");

	return (uptime);
}

void
dovmstat(interval, reps)
	u_int interval;
	int reps;
{
	long et, halfet;	/* elapsed time, and half that */
	double etime;
	struct vmtotal total;
	struct vmmeter osum;

	et = getuptime();
	halfet = et / 2;
	(void)signal(SIGCONT, needhdr);

	stathz = kvm_stathz(kd);

    	memset(&osum, 0, sizeof(osum));

	for (hdrcnt = 1;;) {
		if (!--hdrcnt)
			printhdr();
		if (kvm_cpustats(kd, &cur.cp_stats))
			errx(1, "can't get cpu stats: %s", kvm_geterr(kd));
		if (dk_ndrive != 0 &&
		    kvm_disks(kd, cur.dk_stats, dk_ndrive + 1) != dk_ndrive)
			warnx("reading disk stats: %s", kvm_geterr(kd));
		if (kvm_vmmeter(kd, &sum))
			errx(1, "can't get vmmeter: %s", kvm_geterr(kd));
		if (kvm_vmtotal(kd, &total))
			errx(1, "can't get vmtotal: %s", kvm_geterr(kd));
		(void)printf("%2d %2d %1d",
		    total.t_rq - 1, total.t_dw + total.t_pw, total.t_sw);
#define pgtok(a) ((a) * sum.v_page_size >> 10)
#define	rate(x)	(((x) + halfet) / et)		/* round */
		(void)printf(" %6ld %6ld",
		    pgtok(total.t_avm), pgtok(total.t_free));
		(void)printf(" %4lu", rate(sum.v_faults - osum.v_faults));
		(void)printf(" %3lu",
		    rate(sum.v_reactivated - osum.v_reactivated));
		(void)printf(" %3lu", rate(sum.v_pageins - osum.v_pageins));
		(void)printf(" %3lu %3lu",
		    rate(sum.v_pageouts - osum.v_pageouts), 0);
		(void)printf(" %3lu", rate(sum.v_scan - osum.v_scan));
		etime = cpu_etime();
		dkstats(etime);
		(void)printf(" %4lu %4lu %3lu",
		    rate(sum.v_intr - osum.v_intr),
		    rate(sum.v_syscall - osum.v_syscall),
		    rate(sum.v_swtch - osum.v_swtch));
		cpustats(etime);
		(void)printf("\n");
		(void)fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		osum = sum;
		et = interval;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfet = (et + 1) / 2;
		(void)sleep(interval);
	}
}

void
printhdr()
{
	register int i, l;
	register char *p;

	(void)printf(" procs     memory      page%*s", 20, "");
	if (ndrives > 1)
		(void)printf(" disks %*s   faults      cpu\n",
		   ndrives * 4 - 6, "");
	else
		(void)printf("%*s  faults      cpu\n", ndrives * 4, "");
	(void)printf(" r  b w    avm    fre  flt  re  pi  po  fr  sr ");
	for (i = 0; i < dk_ndrive; i++) {
		if (dr_select[i]) {
			p = dr_name[i];
			l = strlen(p);
			(void)printf(" %c%c ", p[0], p[l - 1]);
		}
	}
	(void)printf("  in   sy  cs us sy  id\n");
	hdrcnt = winlines - 2;
}

/*
 * Force a header to be prepended to the next output.
 */
void
needhdr(sig)
	int sig;
{

	hdrcnt = 1;
}

void
dotimes()
{
#ifdef notdef
	u_int pgintime, rectime;
	static struct nlist nl[] = {
		{ "_rectime" },
		{ "_pgintime" },
		{ NULL }
	};

	knlist(nl);
	knread(&nl[0], &rectime, sizeof rectime);
	knread(&nl[1], &pgintime, sizeof pgintime);
	if (kvm_vmmeter(kd, &sum))
		errx(1, "can't get vmmeter: %s", kvm_geterr(kd));
	(void)printf("%u reclaims, %u total time (usec)\n",
	    sum.v_pgrec, rectime);
	(void)printf("average: %u usec / reclaim\n", rectime / sum.v_pgrec);
	(void)printf("\n");
	(void)printf("%u page ins, %u total time (msec)\n",
	    sum.v_pgin, pgintime / 10);
	(void)printf("average: %8.1f msec / page in\n",
	    pgintime / (sum.v_pgin * 10.0));
#else
	warnx("sorry, -t not yet (re)implemented");
#endif
}

int
pct(top, bot)
	long top, bot;
{
	long ans;

	if (bot == 0)
		return(0);
	ans = (quad_t)top * 100 / bot;
	return (ans);
}

#define	PCT(top, bot) pct((long)(top), (long)(bot))

#ifdef tahoe
#include <machine/cpu.h>
#endif

void
dosum()
{
	long nchtotal;
	struct nchstats nchstats;
#ifdef tahoe
	struct keystats keystats;
	static struct nlist keynl[] = {
		{ "_ckeystats" },
		{ "_dkeystats" },
		{ NULL }
#endif

	if (kvm_vmmeter(kd, &sum))
		errx(1, "can't get vmmeter: %s", kvm_geterr(kd));
	(void)printf("%9u cpu context switches\n", sum.v_swtch);
	(void)printf("%9u device interrupts\n", sum.v_intr);
	(void)printf("%9u software interrupts\n", sum.v_soft);
#ifdef vax
	(void)printf("%9u pseudo-dma dz interrupts\n", sum.v_pdma);
#endif
	(void)printf("%9u traps\n", sum.v_trap);
	(void)printf("%9u system calls\n", sum.v_syscall);
	(void)printf("%9u total faults taken\n", sum.v_faults);
	(void)printf("%9u swap ins\n", sum.v_swpin);
	(void)printf("%9u swap outs\n", sum.v_swpout);
	(void)printf("%9u pages swapped in\n", sum.v_pswpin / CLSIZE);
	(void)printf("%9u pages swapped out\n", sum.v_pswpout / CLSIZE);
	(void)printf("%9u page ins\n", sum.v_pageins);
	(void)printf("%9u page outs\n", sum.v_pageouts);
	(void)printf("%9u pages paged in\n", sum.v_pgpgin);
	(void)printf("%9u pages paged out\n", sum.v_pgpgout);
	(void)printf("%9u pages reactivated\n", sum.v_reactivated);
	(void)printf("%9u intransit blocking page faults\n", sum.v_intrans);
	(void)printf("%9u zero fill pages created\n", sum.v_nzfod / CLSIZE);
	(void)printf("%9u zero fill page faults\n", sum.v_zfod / CLSIZE);
	(void)printf("%9u pages examined by the clock daemon\n", sum.v_scan);
	(void)printf("%9u revolutions of the clock hand\n", sum.v_rev);
	(void)printf("%9u VM object cache lookups\n", sum.v_lookups);
	(void)printf("%9u VM object hits\n", sum.v_hits);
	(void)printf("%9u total VM faults taken\n", sum.v_vm_faults);
	(void)printf("%9u copy-on-write faults\n", sum.v_cow_faults);
	(void)printf("%9u pages freed by daemon\n", sum.v_dfree);
	(void)printf("%9u pages freed by exiting processes\n", sum.v_pfree);
	(void)printf("%9u pages free\n", sum.v_free_count);
	(void)printf("%9u pages wired down\n", sum.v_wire_count);
	(void)printf("%9u pages active\n", sum.v_active_count);
	(void)printf("%9u pages inactive\n", sum.v_inactive_count);
	(void)printf("%9u bytes per page\n", sum.v_page_size);
	(void)printf("%9u target inactive pages\n", sum.v_inactive_target);
	(void)printf("%9u target free pages\n", sum.v_free_target);
	(void)printf("%9u minimum free pages\n", sum.v_free_min);

	if (kvm_ncache(kd, &nchstats))
		errx(1, "can't get nchstats: %s", kvm_geterr(kd));
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_neghits +
	    nchstats.ncs_badhits + nchstats.ncs_falsehits +
	    nchstats.ncs_miss + nchstats.ncs_long;
	(void)printf("%9ld total name lookups\n", nchtotal);
	(void)printf(
	    "%9s cache hits (%d%% pos + %d%% neg) system %d%% per-process\n",
	    "", PCT(nchstats.ncs_goodhits, nchtotal),
	    PCT(nchstats.ncs_neghits, nchtotal),
	    PCT(nchstats.ncs_pass2, nchtotal));
	(void)printf("%9s deletions %d%%, falsehits %d%%, toolong %d%%\n", "",
	    PCT(nchstats.ncs_badhits, nchtotal),
	    PCT(nchstats.ncs_falsehits, nchtotal),
	    PCT(nchstats.ncs_long, nchtotal));
#if defined(tahoe)
	knlist(keynl);
	knread(keynl, &keystats, sizeof(keystats));
	(void)printf("%9d %s (free %d%% norefs %d%% taken %d%% shared %d%%)\n",
	    keystats.ks_allocs, "code cache keys allocated",
	    PCT(keystats.ks_allocfree, keystats.ks_allocs),
	    PCT(keystats.ks_norefs, keystats.ks_allocs),
	    PCT(keystats.ks_taken, keystats.ks_allocs),
	    PCT(keystats.ks_shared, keystats.ks_allocs));
	knread(keynl + 1, &keystats, sizeof(keystats));
	(void)printf("%9d %s (free %d%% norefs %d%% taken %d%% shared %d%%)\n",
	    keystats.ks_allocs, "data cache keys allocated",
	    PCT(keystats.ks_allocfree, keystats.ks_allocs),
	    PCT(keystats.ks_norefs, keystats.ks_allocs),
	    PCT(keystats.ks_taken, keystats.ks_allocs),
	    PCT(keystats.ks_shared, keystats.ks_allocs));
#endif
}

void
doforkst()
{
#ifdef notdef
	struct forkstat fks;
	static struct nlist nl[] = {
		{ "_forkstat" },
		{ NULL }
	};

	knlist(nl);
	knread(nl, &fks, sizeof(struct forkstat));
	(void)printf("%d forks, %d pages, average %.2f\n",
	    fks.cntfork, fks.sizfork, (double)fks.sizfork / fks.cntfork);
	(void)printf("%d vforks, %d pages, average %.2f\n",
	    fks.cntvfork, fks.sizvfork, (double)fks.sizvfork / fks.cntvfork);
#else
	warnx("sorry, -f not yet (re)implemented");
#endif
}

/*
 * Adjust cp_stats to show changes since previous; return sum thereof.
 */
double
cpu_etime()
{
	int state;
	long c, delta;
	double etime;

	etime = 0;
	for (etime = 0, state = 0; state < CPUSTATES; ++state) {
		c = cur.cp_stats.cp_time[state];
		delta = c - last.cp_stats.cp_time[state];
		last.cp_stats.cp_time[state] = c;
		cur.cp_stats.cp_time[state] = delta;
		etime += delta;
	}
	return (etime);
}

void
dkstats(etime)
	double etime;
{
	register int dn;
	long tmp;

	for (dn = 0; dn < dk_ndrive; ++dn) {
		tmp = cur.dk_stats[dn].dk_xfers;
		cur.dk_stats[dn].dk_xfers -= last.dk_stats[dn].dk_xfers;
		last.dk_stats[dn].dk_xfers = tmp;
	}
	if (etime == 0)
		etime = 1;
	etime /= stathz;
	for (dn = 0; dn < dk_ndrive; ++dn)
		if (dr_select[dn])
			(void)printf(" %3.0f",
			    cur.dk_stats[dn].dk_xfers / etime);
}

void
cpustats(total)
	double total;
{
	register u_long *tp;
	double pct;

	pct = total ? 100.0 / total : 0.0;
	tp = cur.cp_stats.cp_time;
	(void)printf(" %2.0f", (tp[CP_USER] + tp[CP_NICE]) * pct);
	(void)printf(" %2.0f", (tp[CP_SYS] + tp[CP_INTR]) * pct);
	(void)printf(" %3.0f", tp[CP_IDLE] * pct);
}

void
dointr()
{
#ifdef i386
	warnx("sorry, -i not yet (re)implemented");
#else
	long *intrcnt, inttotal, uptime;
	int nintr, inamlen;
	char *intrname;
	static struct nlist nl[] = {
		{ "_intrnames" },
		{ "_eintrnames" },
		{ "_intrcnt" },
		{ "_eintrcnt" },
	};

	uptime = getuptime();
	inamlen = nl[1].n_value - nl[0].n_value;
	nintr = nl[3].n_value - nl[2].n_value;
	if ((intrname = malloc(inamlen)) == NULL ||
	    (intrcnt = malloc(nintr)) == NULL)
		err(1, "malloc");
	knread(&nl[0], intrname, inamlen);
	knread(&nl[2], intrcnt, nintr);
	(void)printf("interrupt      total      rate\n");
	inttotal = 0;
	nintr /= sizeof(long);
	while (--nintr >= 0) {
		if (*intrcnt)
			(void)printf("%-12s %8ld %8ld\n", intrname,
			    *intrcnt, *intrcnt / uptime);
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	(void)printf("Total        %8ld %8ld\n", inttotal, inttotal / uptime);
#endif
}

/*
 * These names are defined in <sys/malloc.h>.
 */
char *kmemnames[] = INITKMEMNAMES;

void
domem()
{
	register struct kmembuckets *kp;
	register struct kmemstats *ks;
	register int i, j;
	int len, size, first, nb, ns;
	long totuse = 0, totfree = 0, totreq = 0;
	char *name;
	struct kmemstats kmemstats[M_LAST];
	struct kmembuckets buckets[MINBUCKET + 16];

	(void)kvm_kmemsize(kd, &nb, &ns);
	if (nb != MINBUCKET + 16 || ns != M_LAST)
		errx(1, "kmem bucket or stat mismatch (%d != %d or %d != %d)",
		    nb, MINBUCKET + 16, ns, M_LAST);

	if (kvm_kmem(kd, buckets, MINBUCKET + 16, kmemstats, M_LAST))
		errx(1, "can't get kmem info: %s", kvm_geterr(kd));

	(void)printf("Memory statistics by bucket size\n");
	(void)printf(
	    "    Size   In Use   Free   Requests  HighWater  Couldfree\n");
	for (i = MINBUCKET, kp = &buckets[i]; i < MINBUCKET + 16; i++, kp++) {
		if (kp->kb_calls == 0)
			continue;
		size = 1 << i;
		(void)printf("%8d %8ld %6ld %10ld %7ld %10ld\n", size, 
			kp->kb_total - kp->kb_totalfree,
			kp->kb_totalfree, kp->kb_calls,
			kp->kb_highwat, kp->kb_couldfree);
		totfree += size * kp->kb_totalfree;
	}

	(void)printf("\nMemory usage type by bucket size\n");
	(void)printf("    Size  Type(s)\n");
	kp = &buckets[MINBUCKET];
	for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1, kp++) {
		if (kp->kb_calls == 0)
			continue;
		first = 1;
		len = 8;
		for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
			if (ks->ks_calls == 0)
				continue;
			if ((ks->ks_size & j) == 0)
				continue;
			name = kmemnames[i] ? kmemnames[i] : "undefined";
			len += 2 + strlen(name);
			if (first)
				printf("%8d  %s", j, name);
			else
				printf(",");
			if (len >= 80) {
				printf("\n\t ");
				len = 10 + strlen(name);
			}
			if (!first)
				printf(" %s", name);
			first = 0;
		}
		printf("\n");
	}

	(void)printf(
	    "\nMemory statistics by type                        Type  Kern\n");
	(void)printf(
"         Type InUse MemUse HighUse  Limit Requests Limit Limit Size(s)\n");
	for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
		if (ks->ks_calls == 0)
			continue;
		(void)printf("%13s %5ld %5ldK %6ldK %5ldK %8ld %4u %5u",
		    kmemnames[i] ? kmemnames[i] : "undefined",
		    ks->ks_inuse, (ks->ks_memuse + 1023) / 1024,
		    (ks->ks_maxused + 1023) / 1024,
		    (ks->ks_limit + 1023) / 1024, ks->ks_calls,
		    ks->ks_limblocks, ks->ks_mapblocks);
		first = 1;
		for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1) {
			if ((ks->ks_size & j) == 0)
				continue;
			if (first)
				printf("  %d", j);
			else
				printf(",%d", j);
			first = 0;
		}
		printf("\n");
		totuse += ks->ks_memuse;
		totreq += ks->ks_calls;
	}
	(void)printf("\nMemory Totals:  In Use    Free    Requests\n");
	(void)printf("              %7ldK %6ldK    %8ld\n",
	     (totuse + 1023) / 1024, (totfree + 1023) / 1024, totreq);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: vmstat [-ims] [-c count] [-M core] "
	    "[-N system] [-w wait] [disks]\n");
	exit(1);
}
