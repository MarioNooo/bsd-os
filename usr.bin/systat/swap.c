/*-
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
static char sccsid[] = "@(#)swap.c	8.3 (Berkeley) 4/29/95";
#endif /* not lint */

/*
 * swapinfo - based on a program of the same name by Kevin Lahey
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/stat.h>

#include <vm/swap_pager.h>

#include <kvm.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "systat.h"
#include "extern.h"

extern char *getbsize __P((int *headerlenp, long *blocksizep));
void showspace __P((char *header, int hlen, long blocksize));

kvm_t	*kd;

struct nlist syms[] = {
	{ "_swapmap" },	/* list of free swap areas */
#define VM_SWAPMAP	0
	{ "_swdevt" },	/* list of swap devices and sizes */
#define VM_SWDEVT	1
	{ "_dmmax" },	/* maximum size of a swap block */
#define VM_DMMAX	2
	{ "_niswap" },
#define	VM_NISWAP	3
	{ "_niswdev" },
#define	VM_NISWDEV	4
	{ "_swseq" },
#define	VM_SWSEQ	5
#ifndef notyet
	{ "_swapstats" }, /* do we need to work on dead kernels? */
#define	VM_SWAPSTATS	6
#endif
	{ NULL }
};

static struct swapstats swapstats;
static int dmmax, niswap, niswdev;
static struct swdevt *sw;
static long *perdev, blocksize;
static struct map *swapmap, *kswapmap;
static struct mapent *mp;
static int nfree, hlen;

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var) \
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg) \
	KGET2(syms[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg) \
	if (kvm_read(kd, addr, p, s) != s) { \
		error("cannot read %s: %s", msg, kvm_geterr(kd)); \
		return (0); \
	}

WINDOW *
openswap()
{
	return (subwin(stdscr, LINES-R_LOADAV_END-1, 0, R_LOADAV_END, 0));
}

void
closeswap(w)
	WINDOW *w;
{
	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}

/*
 * Grab a snapshot of the swap setup.  If new (sequential) swap devices
 * are added after this, we never see them or report about them.
 */
int
initswap()
{
	int i;
	char msgbuf[BUFSIZ];
#ifdef notyet
	int mib[2];
#endif
	struct swdevt *swseq;
	static int once = 0;

	if (once)
		return (1);
	if (kvm_nlist(kd, syms)) {
		strcpy(msgbuf, "systat: swap: cannot find");
		for (i = 0; syms[i].n_name != NULL; i++) {
			if (syms[i].n_value == 0) {
				strcat(msgbuf, " ");
				strcat(msgbuf, syms[i].n_name);
			}
		}
		error(msgbuf);
		return (0);
	}
#ifdef notyet
	mib[0] = CTL_VM;
	mib[1] = VM_SWAPSTATS;
	if (sysctl(mib, 2, NULL, NULL, &swapstats, sizeof swapstats)) {
		error("cannot get swapstats");
		return (0);
	}
#else
	KGET(VM_SWAPSTATS, swapstats);
#endif
	KGET(VM_NISWAP, niswap);
	KGET(VM_NISWDEV, niswdev);
	KGET(VM_DMMAX, dmmax);
	KGET(VM_SWAPMAP, kswapmap);	/* kernel `swapmap' is a pointer */
	sw = malloc(swapstats.swap_nswdev * sizeof(*sw));
	perdev = malloc(swapstats.swap_nswdev * sizeof(*perdev));
	mp = malloc(swapstats.swap_mapsize * sizeof(*mp));
	if (sw == NULL || perdev == NULL || mp == NULL) {
		/* XXX should give back any non-NULL ones */
		error("swap malloc");
		return (0);
	}
	/* The first niswdev swap devices are in swdevt[]. */
	KGET1(VM_SWDEVT, sw, niswdev * sizeof(*sw), "swdevt");
	if (swapstats.swap_nswdev > niswdev) {
		/* The rest were allocated individually. */
		KGET(VM_SWSEQ, swseq);
		for (i = niswdev; i < swapstats.swap_nswdev; i++) {
			KGET2((u_long)swseq, &sw[i], sizeof(sw[i]), "swseq");
			swseq = sw[i].sw_next;
		}
	}
	once = 1;
	return (1);
}

void
fetchswap()
{
	int i;
	long s, siz;
	struct swdevt *sp;
	struct mapent *m;

	i = swapstats.swap_mapsize * sizeof(*mp);
	if (kvm_read(kd, (long)kswapmap, mp, i) != i)
		error("cannot read swapmap: %s", kvm_geterr(kd));

	/* first entry in map is `struct map'; rest are mapent's */
	swapmap = (struct map *)mp;
	if (swapstats.swap_mapsize !=
	    swapmap->m_limit + 1 - (struct mapent *)kswapmap)
		error("panic: swap: swapstats.swap_mapsize goof");

	/*
	 * Count up swap space.  Each entry in the map marks a region
	 * of free space.  This space is split among the configured
	 * disks in a complex manner:
	 *
	 *	- the first niswdev disks are ``interleaved''
	 *	- the remaining disks, if any, are sequential
	 *
	 * In the interleaved areas, there is a dmmax-sized block on the
	 * first swap device, then a dmmax-sized hole, then a dmmax-sized
	 * block on the second swap device, then another hole, and so on.
	 * Since the holes are never free, they do not appear in the map
	 * (but do make the map take lots of entries).
	 *
	 * The interleaved devices are simpler; however, each one also
	 * begins with a hole large enough to preserve a label and/or boot
	 * block, so again no map entry can span a disk.
	 */
	nfree = 0;
	bzero(perdev, swapstats.swap_nswdev * sizeof(*perdev));
	for (m = mp + 1; (siz = m->m_size) != 0xffffffff; m++) {
		s = m->m_addr;
		nfree += siz;

		/* This code mimics swstrategy(). */
		if (swapstats.swap_nswdev > 1) {
			if (s < niswap) {
				if (niswdev > 1)
					i = (s / dmmax / 2) % niswdev;
				else
					i = 0;
				if (sw[i].sw_flags & SW_SEQUENTIAL)
					error("panic: swap: interlv/seq 1");
			} else {
				s -= niswap;
				for (sp = &sw[i = niswdev];
				     i < swapstats.swap_nswdev; sp++, i++) {
					if (s <= sp->sw_nblks)
						break;
					s -= sp->sw_nblks;
				}
				if ((sw[i].sw_flags & SW_SEQUENTIAL) == 0)
					error("panic: swap: interlv/seq 2");
			}
		} else
			i = 0;

		perdev[i] += siz;
	}
}

void
labelswap()
{
	int row, i;
	char *header, *p;

	row = 0;
	wmove(wnd, row, 0); wclrtobot(wnd);
	header = getbsize(&hlen, &blocksize);
	mvwprintw(wnd, row++, 0, "%-5s%*s%9s  %55s",
	    "Disk", hlen, header, "Used",
	    "/0%  /10% /20% /30% /40% /50% /60% /70% /80% /90% /100%");
	for (i = 0; i < swapstats.swap_nswdev; i++) {
		if ((p = devname(sw[i].sw_dev, S_IFBLK)) == NULL)
			p = "??";
		mvwprintw(wnd, i + 1, 0, "%-5s", p);
		if (sw[i].sw_flags & SW_SEQUENTIAL)
			waddch(wnd, '*');
	}
	if (swapstats.swap_nswdev > niswdev)
		mvwprintw(wnd, i + 2, 0, "* sequential swap space");
}

void
showswap()
{
	int col, div, i, j, avail, npfree, nblks, used, xfree;

	div = blocksize / 512;
	avail = npfree = 0;
	for (i = 0; i < swapstats.swap_nswdev; i++) {
		col = 5;
		nblks = sw[i].sw_nblks;
		if (nblks >= ctod(CLSIZE))
			nblks -= ctod(CLSIZE);	/* neither used nor free */
		mvwprintw(wnd, i + 1, col, "%*d", hlen, nblks / div);
		col += hlen;
		/*
		 * Don't report statistics for partitions which have not
		 * yet been activated via swapon(8).
		 */
		if ((sw[i].sw_flags & SW_FREED) == 0) {
			mvwprintw(wnd, i + 1, col + 8,
			    "0  *** not available for swapping ***");
			continue;
		}
		xfree = perdev[i];
		used = nblks - xfree;
		mvwprintw(wnd, i + 1, col, "%9d  ", used / div);
		for (j = (100 * used / nblks + 1) / 2; j > 0; j--)
			waddch(wnd, 'X');
		wclrtoeol(wnd);
		npfree++;
		avail += nblks;
	}
	/* 
	 * If only one partition has been set up via swapon(8), we don't
	 * need to bother with totals.
	 */
	if (npfree > 1) {
		used = avail - nfree;
		mvwprintw(wnd, i + 1, 0, "%-5s%*d%9d  ",
		    "Total", hlen, avail / div, used / div);
		for (j = (100 * used / avail + 1) / 2; j > 0; j--)
			waddch(wnd, 'X');
	}
}
