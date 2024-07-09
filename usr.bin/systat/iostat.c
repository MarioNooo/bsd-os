/*	BSDI iostat.c,v 2.5 2003/06/06 15:55:11 polk Exp	*/

/*
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)iostat.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/cpustats.h>
#include <sys/diskstats.h>

#include <string.h>
#include <stdlib.h>
#include <nlist.h>
#include <paths.h>
#include "systat.h"
#include "extern.h"
#include "disks.h"

static struct cpustats cs, ocs;
static  int linesperregion;
static  double etime;
static  int numbers = 0;		/* default display bar graphs */
static  int msps = 0;			/* default ms/seek shown */

static int barlabels __P((int));
static void histogram __P((double, int, double));
static int numlabels __P((int));
static int stats __P((int, int, int));
static void stat1 __P((int, int));


WINDOW *
openiostat()
{
	return (subwin(stdscr, LINES-1-R_LOADAV_END, 0, R_LOADAV_END, 0));
}

void
closeiostat(w)
	WINDOW *w;
{
	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}

int
initiostat()
{

	return (dkinit());
}

void
fetchiostat()
{

	if (kvm_cpustats(kd, &cs) && verbose)
		error("error reading cputime stats");
	if (kvm_disks(kd, dk_stats, dk_ndrive) != dk_ndrive && verbose)
		error("error reading disk stats");
}

#define	INSET	10

void
labeliostat()
{
	int row;

	row = 0;
	wmove(wnd, row, 0); wclrtobot(wnd);
	mvwaddstr(wnd, row++, INSET,
	    "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
	mvwaddstr(wnd, row++, 0, "cpu  user|");
	mvwaddstr(wnd, row++, 0, "     nice|");
	mvwaddstr(wnd, row++, 0, "   system|");
	mvwaddstr(wnd, row++, 0, "interrupt|");
	mvwaddstr(wnd, row++, 0, "     idle|");
	if (numbers)
		row = numlabels(row + 1);
	else
		row = barlabels(row + 1);
}

static int
numlabels(row)
	int row;
{
	int i, col, regions, ndrives;

#define COLWIDTH	14
#define DRIVESPERLINE	((maxx - INSET) / COLWIDTH)
	for (ndrives = 0, i = 0; i < dk_ndrive; i++)
		if (dk_select[i])
			ndrives++;
	regions = howmany(ndrives, DRIVESPERLINE);
	/*
	 * Deduct -regions for blank line after each scrolling region.
	 */
	linesperregion = (maxy - row - regions) / regions;
	/*
	 * Minimum region contains space for two
	 * label lines and one line of statistics.
	 */
	if (linesperregion < 3)
		linesperregion = 3;
	col = 0;
	for (i = 0; i < dk_ndrive; i++)
		if (dk_select[i] && dk_stats[i].dk_bpms != 0) {
			if (col + COLWIDTH >= maxx - INSET) {
				col = 0, row += linesperregion + 1;
				if (row > maxy - (linesperregion + 1))
					break;
			}
			mvwaddstr(wnd, row, col + 4, dr_name[i]);
			mvwaddstr(wnd, row + 1, col, "bps tps msps");
			col += COLWIDTH;
		}
	if (col)
		row += linesperregion + 1;
	return (row);
}

static int
barlabels(row)
	int row;
{
	int i;

	mvwaddstr(wnd, row++, INSET,
	    "/0   /5   /10  /15  /20  /25  /30  /35  /40  /45  /50");
	linesperregion = 2 + msps;
	for (i = 0; i < dk_ndrive; i++)
		if (dk_select[i] && dk_stats[i].dk_bpms != 0) {
			if (row > maxy - linesperregion)
				break;
			mvwprintw(wnd, row++, 0, "%4.4s  bps|", dr_name[i]);
			mvwaddstr(wnd, row++, 0, "      tps|");
			if (msps)
				mvwaddstr(wnd, row++, 0, "     msps|");
		}
	return (row);
}


void
showiostat()
{
	register long t;
	register int i, row, col;

	for (i = 0; i < dk_ndrive; i++) {
#define X(fld)	\
	t = dk_stats[i].fld, \
	dk_stats[i].fld -= dk_ostats[i].fld, \
	dk_ostats[i].fld = t
		X(dk_xfers);
		X(dk_seek);
		X(dk_sectors);
		X(dk_time);
#undef X
	}
	etime = 0;
	for (i = 0; i < CPUSTATES; i++) {
		t = cs.cp_time[i];
		cs.cp_time[i] -= ocs.cp_time[i];
		ocs.cp_time[i] = t;
		etime += cs.cp_time[i];
	}
	if (etime == 0.0)
		etime = 1.0;
	etime /= stathz;
	row = 1;

	for (i = 0; i < CPUSTATES; i++)
		stat1(row++, i);
	if (!numbers) {
		row += 2;
		for (i = 0; i < dk_ndrive; i++)
			if (dk_select[i] && dk_stats[i].dk_bpms != 0) {
				if (row > maxy - linesperregion)
					break;
				row = stats(row, INSET, i);
			}
		return;
	}
	col = 0;
	wmove(wnd, row + linesperregion, 0);
	wdeleteln(wnd);
	wmove(wnd, row + 3, 0);
	winsertln(wnd);
	for (i = 0; i < dk_ndrive; i++)
		if (dk_select[i] && dk_stats[i].dk_bpms != 0) {
			if (col + COLWIDTH >= maxx) {
				col = 0, row += linesperregion + 1;
				if (row > maxy - (linesperregion + 1))
					break;
				wmove(wnd, row + linesperregion, 0);
				wdeleteln(wnd);
				wmove(wnd, row + 3, 0);
				winsertln(wnd);
			}
			(void) stats(row + 3, col, i);
			col += COLWIDTH;
		}
}

static int
stats(row, col, dn)
	int row, col, dn;
{
	struct diskstats *dk;
	double atime, bytes, bps, tps, dmsps, itime, xtime;

	dk = &dk_stats[dn];
	atime = dk->dk_time / (double)stathz;	/* time active (seconds) */
	bytes = dk->dk_sectors * dk->dk_secsize;/* bytes transferred */
	xtime = bytes / dk->dk_bpms;		/* transfer time */
	itime = atime - xtime;			/* time not transferring */
	if (xtime < 0)
		itime += xtime, xtime = 0.0;
	if (itime <= 0) {
		xtime += itime, itime = 0.0;
		dmsps = 0.0;
	} else if (dk->dk_seek == 0 && dk->dk_xfers == 0)
		dmsps = 0.0;
	else {
		/*
		 * Compute ms per seek based on seek count if
		 * driver does explicit seeks, xfer count otherwise.
		 */
		dmsps = itime * 1000.0 /
		    (dk->dk_seek ? dk->dk_seek : dk->dk_xfers);
	}
	bps = bytes / 1024.0 / etime;
	tps = dk->dk_xfers / etime;
	if (numbers) {
		mvwprintw(wnd, row, col, "%3.0f%4.0f%5.1f", bps, tps, dmsps);
		return (row);
	}
	wmove(wnd, row++, col);
	histogram(bps, 50, 1.0);
	wmove(wnd, row++, col);
	histogram(tps, 50, 1.0);
	if (msps) {
		wmove(wnd, row++, col);
		histogram(dmsps, 50, 1.0);
	}
	return (row);
}

static void
stat1(row, o)
	int row, o;
{
	register int i;
	double etime;

	etime = 0;
	for (i = 0; i < CPUSTATES; i++)
		etime += cs.cp_time[i];
	if (etime == 0.0)
		etime = 1.0;
	wmove(wnd, row, INSET);
#define CPUSCALE	0.5
	histogram(100.0 * cs.cp_time[o] / etime, 50, CPUSCALE);
}

static void
histogram(val, colwidth, scale)
	double val;
	int colwidth;
	double scale;
{
	char buf[10];
	register int k;
	register int v = (int)(val * scale) + 0.5;

	k = MIN(v, colwidth);
	if (v > colwidth) {
		sprintf(buf, "% 6.1f", val);
		k -= strlen(buf);
		while (k--)
			waddch(wnd, 'X');
		waddstr(wnd, buf);
		return;
	}
	while (k--)
		waddch(wnd, 'X');
	wclrtoeol(wnd);
}

int
cmdiostat(cmd, args)
	char *cmd, *args;
{

	if (prefix(cmd, "msps"))
		msps = !msps;
	else if (prefix(cmd, "numbers"))
		numbers = 1;
	else if (prefix(cmd, "bars"))
		numbers = 0;
	else if (!dkcmd(cmd, args))
		return (0);
	wclear(wnd);
	labeliostat();
	refresh();
	return (1);
}
