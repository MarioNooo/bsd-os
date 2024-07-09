/*	BSDI vmstat.c,v 2.6 2001/01/15 18:24:44 donn Exp	*/

/*-
 * Copyright (c) 1983, 1989, 1992, 1993
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
static char sccsid[] = "@(#)vmstat.c	8.2 (Berkeley) 1/12/94";
#endif /* not lint */

/*
 * Cursed vmstat -- from Robert Elz.
 */

#include <sys/param.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/sysctl.h>

#include <vm/vm.h>

#include <signal.h>
#include <nlist.h>
#include <ctype.h>
#include <utmp.h>
#include <paths.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "systat.h"
#include "extern.h"
#include "disks.h"

static struct Info {
	/* area through info_endcopy must be "copyable" (see copyinfo) */
	struct	cpustats cs;
	struct	vmmeter cnt;
	struct	vmtotal total;
	struct	nchstats nchstats;
	long	nchcount;
#define info_endcopy dk_stats
	struct	diskstats *dk_stats;
	long	*intrcnt;
} s, s1, s2, z;

static	enum state { BOOT, TIME, RUN } state = TIME;

static void allocinfo __P((struct Info *));
static void copyinfo __P((struct Info *, struct Info *));
static float cputime __P((int));
static void dinfo __P((int, int));
static void getinfo __P((struct Info *, enum state));
static void putint __P((int, int, int, int));
static void putfloat __P((double, int, int, int, int, int));
static int ucount __P((void));

static	int ut;
static	char buf[26];
static	double etime;
static	int nintr;
static	int *intrloc;
static	char **intrname;
static	int nextintsrow;

struct	utmp utmp;


WINDOW *
openkre()
{

	ut = open(_PATH_UTMP, O_RDONLY);
	if (ut < 0)
		error("No utmp");
	return (stdscr);
}

void
closekre(w)
	WINDOW *w;
{

	(void) close(ut);
	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
}


#ifndef i386	/* XXX */
#define DOINTR	/* XXX */
#endif		/* XXX */
#ifdef	DOINTR
static struct nlist namelist[] = {
#define	X_INTRNAMES	0
	{ "_intrnames" },
#define	X_EINTRNAMES	1
	{ "_eintrnames" },
#define	X_INTRCNT	2
	{ "_intrcnt" },
#define	X_EINTRCNT	3
	{ "_eintrcnt" },
	{ NULL }
};
#endif

/*
 * These constants define where the major pieces are laid out
 */
#define STATROW		 0	/* uses 1 row and 68 cols */
#define STATCOL		 2
#define MEMROW		 2	/* uses 4 rows and 31 cols */
#define MEMCOL		 0
#define PAGEROW		 2	/* uses 4 rows and 26 cols */
#define PAGECOL		36
#define INTSROW		 2	/* uses all rows to bottom and 17 cols */
#define INTSCOL		63
#define PROCSROW	 7	/* uses 2 rows and 20 cols */
#define PROCSCOL	 0
#define GENSTATROW	 7	/* uses 2 rows and 30 cols */
#define GENSTATCOL	20
#define VMSTATROW	 7	/* uses 17 rows and 12 cols */
#define VMSTATCOL	48
#define GRAPHROW	10	/* uses 4 rows and 51 cols */
#define GRAPHCOL	 0
#define NAMEIROW	15	/* uses 3 rows and 38 cols */
#define NAMEICOL	 0
#define DISKROW		19	/* uses 5 rows and 50 cols (for 9 drives) */
#define DISKCOL		 0

#define	DRIVESPACE	 9	/* max # for space */

#define	MAXDRIVES	DRIVESPACE	 /* max # to display */

int
initkre()
{
#ifdef	DOINTR
	char *intrnamebuf, *cp;
	int i;
#endif
	static int once = 0;

#ifdef DOINTR
	if (namelist[0].n_type == 0) {
		if (kvm_nlist(kd, namelist)) {
			nlisterr(namelist);
			return (0);
		}
	}
#endif
	if (!dkinit())
		return (0);
	if (dk_ndrive && !once) {
		/* share current & previous info with disk code */
		s.dk_stats = dk_stats;
		s1.dk_stats = dk_ostats;
		s2.dk_stats = calloc(dk_ndrive, sizeof *s2.dk_stats);
		z.dk_stats = calloc(dk_ndrive, sizeof *z.dk_stats);
		if (s2.dk_stats == NULL || z.dk_stats == NULL) {
			error("Out of memory\n");
			free(s2.dk_stats);
			free(z.dk_stats);
			return (0);
		}
		once = 1;
	}
#ifdef DOINTR
	if (nintr == 0) {
		nintr = (namelist[X_EINTRCNT].n_value -
			namelist[X_INTRCNT].n_value) / sizeof (long);
		intrloc = calloc(nintr, sizeof *intrloc);
		intrname = calloc(nintr, sizeof *intrname);
		intrnamebuf = malloc(namelist[X_EINTRNAMES].n_value -
			namelist[X_INTRNAMES].n_value);
		if (intrnamebuf == 0 || intrname == 0 || intrloc == 0) {
			error("Out of memory\n");
			free(intrnamebuf);
			free(intrname);
			free(intrloc);
			nintr = 0;
			return (0);
		}
		KREAD((void *)namelist[X_INTRNAMES].n_value, intrnamebuf,
		    namelist[X_EINTRNAMES].n_value -
		    namelist[X_INTRNAMES].n_value);
		for (cp = intrnamebuf, i = 0; i < nintr; i++) {
			intrname[i] = cp;
			cp += strlen(cp) + 1;
		}
		nextintsrow = INTSROW + 2;
		allocinfo(&s);
		allocinfo(&s1);
		allocinfo(&s2);
		allocinfo(&z);
	}
#endif
	getinfo(&s2, RUN);
	copyinfo(&s2, &s1);
	return(1);
}

void
fetchkre()
{
	time_t now;

	time(&now);
	strcpy(buf, ctime(&now));
	buf[16] = '\0';
	getinfo(&s, state);
}

void
labelkre()
{
	register int i, j;

	clear();
	mvprintw(STATROW, STATCOL + 4, "users    Load");
	mvprintw(MEMROW, MEMCOL, "Mem:KB  REAL        VIRTUAL");
	mvprintw(MEMROW + 1, MEMCOL, "      Tot Share    Tot  Share");
	mvprintw(MEMROW + 2, MEMCOL, "Act");
	mvprintw(MEMROW + 3, MEMCOL, "All");

	mvprintw(MEMROW + 1, MEMCOL + 31, "Free");

	mvprintw(PAGEROW, PAGECOL,     "        PAGING   SWAPPING ");
	mvprintw(PAGEROW + 1, PAGECOL, "        in  out   in  out ");
	mvprintw(PAGEROW + 2, PAGECOL, "count");
	mvprintw(PAGEROW + 3, PAGECOL, "pages");

	mvprintw(INTSROW, INTSCOL + 3, " Interrupts");
	mvprintw(INTSROW + 1, INTSCOL + 9, "total");

	mvprintw(VMSTATROW + 0, VMSTATCOL + 10, "cow");
	mvprintw(VMSTATROW + 1, VMSTATCOL + 10, "objlk");
	mvprintw(VMSTATROW + 2, VMSTATCOL + 10, "objht");
	mvprintw(VMSTATROW + 3, VMSTATCOL + 10, "zfod");
	mvprintw(VMSTATROW + 4, VMSTATCOL + 10, "nzfod");
	mvprintw(VMSTATROW + 5, VMSTATCOL + 10, "%%zfod");
	mvprintw(VMSTATROW + 6, VMSTATCOL + 10, "kern");
	mvprintw(VMSTATROW + 7, VMSTATCOL + 10, "wire");
	mvprintw(VMSTATROW + 8, VMSTATCOL + 10, "act");
	mvprintw(VMSTATROW + 9, VMSTATCOL + 10, "inact");
	mvprintw(VMSTATROW + 10, VMSTATCOL + 10, "free");
	mvprintw(VMSTATROW + 11, VMSTATCOL + 10, "daefr");
	mvprintw(VMSTATROW + 12, VMSTATCOL + 10, "prcfr");
	mvprintw(VMSTATROW + 13, VMSTATCOL + 10, "react");
	mvprintw(VMSTATROW + 14, VMSTATCOL + 10, "scan");
	mvprintw(VMSTATROW + 15, VMSTATCOL + 10, "hdrev");
	if (LINES - 1 > VMSTATROW + 16)
		mvprintw(VMSTATROW + 16, VMSTATCOL + 10, "intrn");

	mvprintw(GENSTATROW, GENSTATCOL, "  Csw  Trp  Sys  Int  Sof  Flt");

	mvprintw(GRAPHROW, GRAPHCOL,
		"  System   Interrupt   User      Nice      Idle");
	mvprintw(GRAPHROW + 1, GRAPHCOL,
		"     . %%       . %%       . %%       . %%       . %%");
	mvprintw(GRAPHROW + 2, GRAPHCOL,
		"|    |    |    |    |    |    |    |    |    |    |");

	mvprintw(PROCSROW, PROCSCOL, "Proc:r  p  d  s  w");

	mvprintw(NAMEIROW, NAMEICOL, "Namei         Sys-cache     Proc-cache");
	mvprintw(NAMEIROW + 1, NAMEICOL,
		"    Calls     hits    %%     hits     %%");
	mvprintw(DISKROW, DISKCOL, "Discs");
	mvprintw(DISKROW + 1, DISKCOL, "seeks");
	mvprintw(DISKROW + 2, DISKCOL, "xfers");
	mvprintw(DISKROW + 3, DISKCOL, " blks");
	if (LINES - 1 > DISKROW + 4)
		mvprintw(DISKROW + 4, DISKCOL, " msps");
	j = 0;
	for (i = 0; i < dk_ndrive && j < MAXDRIVES; i++)
		if (dk_select[i]) {
			mvprintw(DISKROW, DISKCOL + 5 + 5 * j,
				"  %3.3s", dr_name[j]);
			j++;
		}
	for (i = 0; i < nintr; i++) {
		if (intrloc[i] == 0)
			continue;
		mvprintw(intrloc[i], INTSCOL + 9, "%-8.8s", intrname[i]);
	}
}

static int
ncpu()
{
	int mib[2];
	int n;
	size_t s = sizeof (n);

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, &n, &s, NULL, 0) < 0 || s != sizeof (n) || n < 1)
		return (1);
	return (n);
}

#define DELTA(fld) { \
	register long t = s.fld; \
	s.fld -= s1.fld; \
	if (state == TIME) \
		s1.fld = t; \
}

#define NCHDELTA(fld) { \
	register u_long t = s.nchstats.fld; \
	s.nchstats.fld -= s1.nchstats.fld; \
	if (state == TIME) \
		s1.nchstats.fld = t; \
}

#define PUTRATE(fld, l, c, w) \
	DELTA(fld); \
	putint((int)((float)s.fld / etime + 0.5), l, c, w)

#define MAXFAIL 5

static	char cpuchar[CPUSTATES] = { '=', '+', '>', '-', ' ' };
static	char cpuorder[CPUSTATES] =
    { CP_SYS, CP_INTR, CP_USER, CP_NICE, CP_IDLE };

void
showkre()
{
	float f1, f2;
	int psiz, inttotal;
	int i, l, c;
	static int failcnt = 0;

	for (i = 0; i < dk_ndrive; i++) {
		DELTA(dk_stats[i].dk_xfers);
		DELTA(dk_stats[i].dk_seek);
		DELTA(dk_stats[i].dk_sectors);
		DELTA(dk_stats[i].dk_time);
	}
	etime = 0.0;
	for(i = 0; i < CPUSTATES; i++) {
		DELTA(cs.cp_time[i]);
		etime += s.cs.cp_time[i];
	}
	if (etime < 5.0) {	/* < 5 ticks - ignore this trash */
		if (failcnt++ >= MAXFAIL) {
			clear();
			mvprintw(2, 10, "The alternate system clock has died!");
			mvprintw(3, 10, "Reverting to ``pigs'' display.");
			move(CMDLINE, 0);
			refresh();
			failcnt = 0;
			sleep(5);
			command("pigs");
		}
		return;
	}
	failcnt = 0;
	etime /= stathz * ncpu();
	inttotal = 0;
	for (i = 0; i < nintr; i++) {
		if (s.intrcnt[i] == 0)
			continue;
		if (intrloc[i] == 0) {
			if (nextintsrow == LINES)
				continue;
			intrloc[i] = nextintsrow++;
			mvprintw(intrloc[i], INTSCOL + 9, "%-8.8s",
				intrname[i]);
		}
		DELTA(intrcnt[i]);
		l = (int)((float)s.intrcnt[i] / etime + 0.5);
		inttotal += l;
		putint(l, intrloc[i], INTSCOL, 8);
	}
	putint(inttotal, INTSROW + 1, INTSCOL, 8);
	NCHDELTA(ncs_goodhits);
	NCHDELTA(ncs_badhits);
	NCHDELTA(ncs_miss);
	NCHDELTA(ncs_long);
	NCHDELTA(ncs_pass2);
	NCHDELTA(ncs_2passes);
	s.nchcount = s.nchstats.ncs_goodhits + s.nchstats.ncs_badhits +
	    s.nchstats.ncs_miss + s.nchstats.ncs_long;
	if (state == TIME)
		s1.nchcount = s.nchcount;

	psiz = 0;
	f2 = 0.0;
	for (c = 0; c < CPUSTATES; c++) {
		i = cpuorder[c];
		f1 = cputime(i);
		f2 += f1;
		l = (int) ((f2 + 1.0) / 2.0) - psiz;
		putfloat(f1, GRAPHROW + 1, GRAPHCOL + 10 * c + 2, 5, 1, 0);
		move(GRAPHROW + 2, psiz);
		psiz += l;
		while (l-- > 0)
			addch(cpuchar[c]);
	}

	putint(ucount(), STATROW, STATCOL, 3);
	putfloat(avenrun[0], STATROW, STATCOL + 17, 6, 2, 0);
	putfloat(avenrun[1], STATROW, STATCOL + 23, 6, 2, 0);
	putfloat(avenrun[2], STATROW, STATCOL + 29, 6, 2, 0);
	mvaddstr(STATROW, STATCOL + 53, buf);
#define pgtokb(pg)	((pg) * s.cnt.v_page_size / 1024)
	putint(pgtokb(s.total.t_arm), MEMROW + 2, MEMCOL + 3, 6);
	putint(pgtokb(s.total.t_armshr), MEMROW + 2, MEMCOL + 9, 6);
	putint(pgtokb(s.total.t_avm), MEMROW + 2, MEMCOL + 15, 7);
	putint(pgtokb(s.total.t_avmshr), MEMROW + 2, MEMCOL + 22, 7);
	putint(pgtokb(s.total.t_rm), MEMROW + 3, MEMCOL + 3, 6);
	putint(pgtokb(s.total.t_rmshr), MEMROW + 3, MEMCOL + 9, 6);
	putint(pgtokb(s.total.t_vm), MEMROW + 3, MEMCOL + 15, 7);
	putint(pgtokb(s.total.t_vmshr), MEMROW + 3, MEMCOL + 22, 7);
	putint(pgtokb(s.total.t_free), MEMROW + 2, MEMCOL + 29, 6);
	putint(s.total.t_rq - 1, PROCSROW + 1, PROCSCOL + 3, 3);
	putint(s.total.t_pw, PROCSROW + 1, PROCSCOL + 6, 3);
	putint(s.total.t_dw, PROCSROW + 1, PROCSCOL + 9, 3);
	putint(s.total.t_sl, PROCSROW + 1, PROCSCOL + 12, 3);
	putint(s.total.t_sw, PROCSROW + 1, PROCSCOL + 15, 3);
	PUTRATE(cnt.v_cow_faults, VMSTATROW + 0, VMSTATCOL + 3, 6);
	PUTRATE(cnt.v_lookups, VMSTATROW + 1, VMSTATCOL + 3, 6);
	PUTRATE(cnt.v_hits, VMSTATROW + 2, VMSTATCOL + 3, 6);
	PUTRATE(cnt.v_zfod, VMSTATROW + 3, VMSTATCOL + 4, 5);
	PUTRATE(cnt.v_nzfod, VMSTATROW + 4, VMSTATCOL + 3, 6);
	putfloat(s.cnt.v_nzfod == 0 ? 0.0 :
	    (100.0 * s.cnt.v_zfod / s.cnt.v_nzfod),
	    VMSTATROW + 5, VMSTATCOL + 2, 7, 2, 1);
	putint(pgtokb(s.cnt.v_kernel_pages), VMSTATROW + 6, VMSTATCOL, 9);
	putint(pgtokb(s.cnt.v_wire_count), VMSTATROW + 7, VMSTATCOL, 9);
	putint(pgtokb(s.cnt.v_active_count), VMSTATROW + 8, VMSTATCOL, 9);
	putint(pgtokb(s.cnt.v_inactive_count), VMSTATROW + 9, VMSTATCOL, 9);
	putint(pgtokb(s.cnt.v_free_count), VMSTATROW + 10, VMSTATCOL, 9);
	PUTRATE(cnt.v_dfree, VMSTATROW + 11, VMSTATCOL, 9);
	PUTRATE(cnt.v_pfree, VMSTATROW + 12, VMSTATCOL, 9);
	PUTRATE(cnt.v_reactivated, VMSTATROW + 13, VMSTATCOL, 9);
	PUTRATE(cnt.v_scan, VMSTATROW + 14, VMSTATCOL, 9);
	PUTRATE(cnt.v_rev, VMSTATROW + 15, VMSTATCOL, 9);
	if (LINES - 1 > VMSTATROW + 16)
		PUTRATE(cnt.v_intrans, VMSTATROW + 16, VMSTATCOL, 9);
	PUTRATE(cnt.v_pageins, PAGEROW + 2, PAGECOL + 5, 5);
	PUTRATE(cnt.v_pageouts, PAGEROW + 2, PAGECOL + 10, 5);
	PUTRATE(cnt.v_swpin, PAGEROW + 2, PAGECOL + 15, 5);	/* - */
	PUTRATE(cnt.v_swpout, PAGEROW + 2, PAGECOL + 20, 5);	/* - */
	PUTRATE(cnt.v_pgpgin, PAGEROW + 3, PAGECOL + 5, 5);	/* ? */
	PUTRATE(cnt.v_pgpgout, PAGEROW + 3, PAGECOL + 10, 5);	/* ? */
	PUTRATE(cnt.v_pswpin, PAGEROW + 3, PAGECOL + 15, 5);	/* - */
	PUTRATE(cnt.v_pswpout, PAGEROW + 3, PAGECOL + 20, 5);	/* - */
	PUTRATE(cnt.v_swtch, GENSTATROW + 1, GENSTATCOL, 5);
	PUTRATE(cnt.v_trap, GENSTATROW + 1, GENSTATCOL + 5, 5);
	PUTRATE(cnt.v_syscall, GENSTATROW + 1, GENSTATCOL + 10, 5);
	PUTRATE(cnt.v_intr, GENSTATROW + 1, GENSTATCOL + 15, 5);
	PUTRATE(cnt.v_soft, GENSTATROW + 1, GENSTATCOL + 20, 5);
	PUTRATE(cnt.v_faults, GENSTATROW + 1, GENSTATCOL + 25, 5);
	mvprintw(DISKROW, DISKCOL + 5, "                              ");
	for (i = 0, c = 0; i < dk_ndrive && c < MAXDRIVES; i++)
		if (dk_select[i]) {
			mvprintw(DISKROW, DISKCOL + 5 + 5 * c,
				"  %3.3s", dr_name[i]);
			dinfo(i, ++c);
		}
	putint(s.nchcount, NAMEIROW + 2, NAMEICOL, 9);
	putint(s.nchstats.ncs_goodhits, NAMEIROW + 2, NAMEICOL + 9, 9);
#define nz(x)	((x) ? (x) : 1)
	putfloat(s.nchstats.ncs_goodhits * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 19, 4, 0, 1);
	putint(s.nchstats.ncs_pass2, NAMEIROW + 2, NAMEICOL + 23, 9);
	putfloat(s.nchstats.ncs_pass2 * 100.0 / nz(s.nchcount),
	   NAMEIROW + 2, NAMEICOL + 34, 4, 0, 1);
#undef nz
}

int
cmdkre(cmd, args)
	char *cmd, *args;
{

	if (prefix(cmd, "run")) {
		copyinfo(&s2, &s1);
		state = RUN;
		return (1);
	}
	if (prefix(cmd, "boot")) {
		state = BOOT;
		copyinfo(&z, &s1);
		return (1);
	}
	if (prefix(cmd, "time")) {
		state = TIME;
		return (1);
	}
	if (prefix(cmd, "zero")) {
		if (state == RUN)
			getinfo(&s1, RUN);
		return (1);
	}
	return (dkcmd(cmd, args));
}

/* calculate number of users on the system */
static int
ucount()
{
	register int nusers = 0;

	if (ut < 0)
		return (0);
	/* skip first unused entry; read remainder */
	(void)lseek(ut, sizeof(utmp), L_SET);
	while (read(ut, &utmp, sizeof(utmp)))
		if (utmp.ut_name[0] != '\0')
			nusers++;

	return (nusers);
}

static float
cputime(indx)
	int indx;
{
	double t;
	register int i;

	t = 0;
	for (i = 0; i < CPUSTATES; i++)
		t += s.cs.cp_time[i];
	if (t == 0.0)
		t = 1.0;
	return (s.cs.cp_time[indx] * 100.0 / t);
}

static void
putint(n, l, c, w)
	int n, l, c, w;
{
	char b[128];

	move(l, c);
	if (n == 0) {
		while (w-- > 0)
			addch(' ');
		return;
	}
	sprintf(b, "%*d", w, n);
	if (strlen(b) > w) {
		while (w-- > 0)
			addch('*');
		return;
	}
	addstr(b);
}

static void
putfloat(f, l, c, w, d, nz)
	double f;
	int l, c, w, d, nz;
{
	char b[128];

	move(l, c);
	if (nz && f == 0.0) {
		while (--w >= 0)
			addch(' ');
		return;
	}
	sprintf(b, "%*.*f", w, d, f);
	if (strlen(b) > w) {
		while (--w >= 0)
			addch('*');
		return;
	}
	addstr(b);
}

static void
getinfo(sp, st)
	struct Info *sp;
	enum state st;
{
	int err;

	err = 0;
#define GET(fn, var) if (fn(kd, &var)) { err = 1; bzero(&var, sizeof var); }
	GET(kvm_cpustats, sp->cs);
	GET(kvm_vmmeter, sp->cnt);
	GET(kvm_vmtotal, sp->total);
	GET(kvm_ncache, sp->nchstats);
	if (err)
		error("Can't get kernel info: %s", kvm_geterr(kd));
#undef	GET

	if (kvm_disks(kd, sp->dk_stats, dk_ndrive) != dk_ndrive)
		error("disk stats mismatch");
#ifdef DOINTR
	KREAD((void *)namelist[X_INTRCNT].n_value, sp->intrcnt,
	    nintr * sizeof(long));
#endif
}

#ifdef	DOINTR
static void
allocinfo(sp)
	struct Info *sp;
{

	sp->intrcnt = malloc(nintr * sizeof(long));
	if (sp->intrcnt == NULL) {
		fprintf(stderr, "systat: out of memory\n");
		exit(2);
	}
}
#endif

static void
copyinfo(from, to)
	register struct Info *from, *to;
{

	bcopy(from, to, (char *)&to->info_endcopy - (char *)to);
	bcopy(from->dk_stats, to->dk_stats, dk_ndrive * sizeof *to->dk_stats);
	bcopy(from->intrcnt, to->intrcnt, nintr * sizeof *to->intrcnt);
}

static void
dinfo(dn, c)
	int dn, c;
{
	int secs, nseek;
	struct diskstats *dk;
	double atime, itime, msps, bytes, xtime;

	dk = &s.dk_stats[dn];
	c = DISKCOL + c * 5;

	bytes = dk->dk_sectors * dk->dk_secsize; /* bytes xferred */
	atime = dk->dk_time;		/* ticks busy */
	atime /= stathz;		/* seconds busy */
	xtime = bytes / dk->dk_bpms;	/* xfer time */
	itime = atime - xtime;		/* time not xfer'ing */
	if (itime <= 0.0)
		msps = 0.0;
	else {
		/*
		 * Distribute disk idle time to get milliseconds per seek.
		 * If driver does not count seeks, assume one per transfer.
		 */
		if ((nseek = dk->dk_seek) == 0)
			if ((nseek = dk->dk_xfers) == 0)
				nseek = 1;
		msps = itime * 1000.0 / nseek;
	}

	putint((int)(dk->dk_seek / etime + 0.5), DISKROW + 1, c, 5);
	putint((int)(dk->dk_xfers / etime + 0.5), DISKROW + 2, c, 5);
	putint((int)(bytes / etime / 1024.0 + 0.5), DISKROW + 3, c, 5);
	if (LINES - 1 > DISKROW + 4)
		putfloat(msps, DISKROW + 4, c, 5, 1, 1);
}
