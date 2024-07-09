/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI iostat.c,v 2.6 1998/02/25 22:56:06 cp Exp
 */

/*-
 * Copyright (c) 1986, 1991, 1993
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
"@(#) Copyright (c) 1986, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n\
@(#) Copyright (c) 1992, 1994 Berkeley Software Design, Inc. \
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)iostat.c	8.3 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>
#include <sys/ioctl.h>
#include <sys/ttystats.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

kvm_t	*kd;

/*
 * internal stat structure
 */
struct _disk {
	long	cp_time[CPUSTATES];
	struct	diskstats *dk_stats;
	long	tk_nin;
	long	tk_nout;
} cur, last;

int	dk_ndrive;
int	*dr_select;
char	**dr_name;

int	stathz;
double	etime;

void	cpustats __P((void));
void	dkstats __P((void));
void	*ecalloc __P((size_t, size_t));
void	phdr __P((int));
void	usage __P((void));

#define	TTY_WIDTH	9
#define	CPU_WIDTH	(1 + 4 * CPUSTATES)
#define	DISK_WIDTH	16

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct winsize win;
	long tmp;
	int ch, hdrcnt, i, interval, reps, termwidth;
	u_int ndrives, maxdrives;
	char *memf, *nlistf, *p;
	char buf[_POSIX2_LINE_MAX];
	struct timeval oldtime, newtime;
	int mib[2];
	size_t size;


	/* Calculate max drives */
	termwidth = 80;
	if (isatty(STDOUT_FILENO)) {
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == -1 ||
		    !win.ws_col) {
			if ((p = getenv("COLUMNS")) != NULL)
				termwidth = atoi(p);
		} else
			termwidth = win.ws_col;
		maxdrives = (termwidth - CPU_WIDTH - TTY_WIDTH) / DISK_WIDTH;
	} else
		maxdrives = UINT_MAX;

	interval = reps = 0;
	nlistf = memf = NULL;
	while ((ch = getopt(argc, argv, "c:M:N:w:")) != EOF)
		switch(ch) {
		case 'c':
			if ((reps = atoi(optarg)) <= 0)
				errx(1, "repetition count <= 0.");
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'w':
			if ((interval = atoi(optarg)) <= 0)
				errx(1, "interval <= 0.");
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * Discard any privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(oldtime);
	if (sysctl(mib, 2, &oldtime, &size, NULL, 0) == -1)
		err(1, "error getting boottime");

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == 0)
		errx(1, "kvm_openfiles: %s", buf);

	/*
	 * read disk names, counting drives, then allocate drive data --
	 * note one extra stat for `cur'
	 */
	dr_name = kvm_dknames(kd, &dk_ndrive);
	cur.dk_stats = ecalloc(dk_ndrive + 1, sizeof *cur.dk_stats);
	last.dk_stats = ecalloc(dk_ndrive, sizeof *last.dk_stats);
	dr_select = ecalloc(dk_ndrive, sizeof *dr_select);

	stathz = kvm_stathz(kd);

	/*
	 * Choose drives to be displayed: those named as arguments, then
	 * any remaining space with the first few that fit.
	 *
	 * The backward compatibility #ifdefs permit the syntax:
	 *	iostat [ drives ] [ interval [ count ] ]
	 */
	for (ndrives = 0; *argv; ++argv) {
#ifdef	BACKWARD_COMPATIBILITY
		if (isdigit(**argv))
			break;
#endif
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], *argv))
				continue;
			dr_select[i] = 1;
			++ndrives;
		}
	}
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval) {
		if (!reps)
			reps = -1;
	} else
		if (reps)
			interval = 1;

	/* Read initial stats for selection criteria. */
	if (kvm_disks(kd, cur.dk_stats, dk_ndrive + 1) != dk_ndrive)
		errx(1, "disk stats mismatch");

	for (i = 0; i < dk_ndrive && ndrives < maxdrives; i++) {
		if (dr_select[i])
			continue;
		dr_select[i] = 1;
		++ndrives;
	}

	(void)signal(SIGCONT, phdr);

	for (hdrcnt = 1;;) {
		struct cpustats cs;
		struct ttytotals ts;

		if (!--hdrcnt) {
			phdr(0);
			hdrcnt = 20;
		}

		if (kvm_disks(kd, cur.dk_stats, dk_ndrive + 1) != dk_ndrive)
			errx(1, "disk stats mismatch");

		(void)kvm_cpustats(kd, &cs);
		bcopy(cs.cp_time, cur.cp_time, sizeof cur.cp_time);

		(void)kvm_ttytotals(kd, &ts);
		cur.tk_nin = ts.tty_nin;
		cur.tk_nout = ts.tty_nout;

		for (i = 0; i < dk_ndrive; i++) {
			if (!dr_select[i])
				continue;
#define X(fld)	tmp = cur.fld; cur.fld -= last.fld; last.fld = tmp
			X(dk_stats[i].dk_xfers);
			X(dk_stats[i].dk_seek);
			X(dk_stats[i].dk_sectors);
			X(dk_stats[i].dk_time);
		}
		X(tk_nin);
		X(tk_nout);

		for (i = 0; i < CPUSTATES; i++) {
			X(cp_time[i]);
		}

		if (gettimeofday(&newtime, NULL) == -1)
			err(1, "gettimeofday");

		etime = newtime.tv_sec + (double)newtime.tv_usec / 1000000.0;
		etime -= oldtime.tv_sec + (double)oldtime.tv_usec / 1000000.0;
		oldtime = newtime;
		if (etime == 0.0)
			etime = 1.0;
		(void)printf("%4.0f %4.0f",
		    cur.tk_nin / etime, cur.tk_nout / etime);
		dkstats();
		cpustats();
		(void)printf("\n");
		(void)fflush(stdout);

		if (reps >= 0 && --reps <= 0)
			break;
		(void)sleep(interval);
	}
	exit(0);
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
		err(1, "calloc(%lu, %lu)", (long)s1, (long)s2);
	return (p);
}

/* ARGUSED */
void
phdr(signo)
	int signo;
{
	register int i;

	(void)printf("%*.*s", TTY_WIDTH, TTY_WIDTH, "tty");
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			(void)printf("%*.*s", 
			    DISK_WIDTH, DISK_WIDTH, dr_name[i]);
	(void)printf("%*.*s\n%*.*s",
	    CPU_WIDTH, CPU_WIDTH, "% cpu",
	    TTY_WIDTH, TTY_WIDTH, " tin tout");
	for (i = 0; i < dk_ndrive; i++)
		if (dr_select[i])
			(void)printf("%*.*s", 
			    DISK_WIDTH, DISK_WIDTH, "sps  tps msps");
	(void)printf("%*.*s\n",
	    CPU_WIDTH, CPU_WIDTH, "usr nic sys int idl");
}

void
dkstats()
{
	register int dn, secs;
	register struct diskstats *dk;
	double atime, itime, msps, bytes, xtime;

	for (dk = cur.dk_stats, dn = 0; dn < dk_ndrive; dk++, dn++) {
		if (!dr_select[dn])
			continue;
		secs = dk->dk_sectors;		/* sectors xfer'd */
		(void)printf("  %4.0f", secs / etime);
		bytes = secs * dk->dk_secsize;

		(void)printf(" %4.0f", dk->dk_xfers / etime);

		if (dk->dk_bpms && dk->dk_xfers) {
			atime = dk->dk_time;		/* ticks disk busy */
			atime /= (float)stathz;		/* ticks to seconds */
			xtime = bytes / dk->dk_bpms;	/* transfer time */
			itime = atime - xtime;		/* time not xfer'ing */
			if (itime < 0)
				msps = 0;
			else {
				/*
				 * Use dk_seek if set (driver counts seeks),
				 * else use dk_xfers (assumes 1 seek / xfer).
				 */
				msps = itime * 1000.0 /
				    (dk->dk_seek ? dk->dk_seek : dk->dk_xfers);
			}
		} else
			msps = 0;
		(void)printf(msps < 100.0 ? " %4.1f" : " %4.0f", msps);
	}
}

void
cpustats()
{
	register int state;
	double time;

	time = 0;
	for (state = 0; state < CPUSTATES; ++state)
		time += cur.cp_time[state];
	(void)printf(" ");
	for (state = 0; state < CPUSTATES; ++state)
		(void)printf(" %3.0f",
		    100. * cur.cp_time[state] / (time ? time : 1));
}

void
usage()
{
	(void)fprintf(stderr,
"usage: iostat [-c count] [-M core] [-N system] [-w wait] [drives]\n");
	exit(1);
}
