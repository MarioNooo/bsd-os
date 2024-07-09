/*	BSDI print.c,v 2.8 2001/10/25 14:47:53 benoitp Exp	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
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
static char sccsid[] = "@(#)print.c	8.5 (Berkeley) 7/28/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <utmp.h>

#include "ls.h"
#include "extern.h"

#ifdef TORNADO
#include "fileToolsLib.h"
#endif

static int	printaname __P((FTSENT *, u_long, u_long));
static void	printlink __P((FTSENT *));
static void	printtime __P((time_t));
static int	printtype __P((u_int));

#define	IS_NOPRINT(p)	((p)->fts_number == NO_PRINT)

void
printscol(dp)
	DISPLAY *dp;
{
	FTSENT *p;

	for (p = dp->list; p; p = p->fts_link) {
		if (IS_NOPRINT(p))
			continue;
		(void)printaname(p, dp->s_inode, dp->s_block);
		(void)putchar('\n');
	}
}

void
printlong(dp)
	DISPLAY *dp;
{
	struct stat *sp;
	FTSENT *p;
	NAMES *np;
	char buf[80];

	if (dp->list->fts_level != FTS_ROOTLEVEL && (f_longform || f_size))
		(void)printf("total %lu\n", howmany(dp->btotal, blocksize));

	for (p = dp->list; p; p = p->fts_link) {
		if (IS_NOPRINT(p))
			continue;
		sp = p->fts_statp;
		if (f_inode)
			(void)printf("%*lu ", dp->s_inode, sp->st_ino);
		if (f_size)
			/* XXX bpn - CYGWIN does not accept modifier q */
			(void)printf("%*lld ",
			    dp->s_block, howmany(sp->st_blocks, blocksize));
		(void)strmode(sp->st_mode, buf);
		np = p->fts_pointer;
		(void)printf("%s %*u %-*s  %-*s  ", buf, dp->s_nlink,
		    sp->st_nlink, dp->s_user, np->user, dp->s_group,
		    np->group);
		if (f_flags)
			(void)printf("%-*s ", dp->s_flags, np->flags);
		if (S_ISCHR(sp->st_mode) || S_ISBLK(sp->st_mode)) {
#ifdef COMPAT_DEV
			/* The only device with major 0 is /dev/console. */
			if (major(sp->st_rdev) == 0 && sp->st_rdev != 0)
				(void)snprintf(buf, sizeof(buf), "%d,%d",
				    sp->st_rdev >> 8, sp->st_rdev & 0xff);
			else
#endif
			(void)snprintf(buf, sizeof(buf),
			    "%d,%d,%d", major(sp->st_rdev),
			    dv_unit(sp->st_rdev), dv_subunit(sp->st_rdev));
			(void)printf("%*s ", dp->s_size, buf);
		} else
			/* XXX bpn - CYGWIN does not accept modifier q */
			(void)printf("%*lld ", dp->s_size, sp->st_size);
		if (f_accesstime)
			printtime(sp->st_atime);
		else if (f_statustime)
			printtime(sp->st_ctime);
		else
			printtime(sp->st_mtime);
		(void)printf("%s", p->fts_name);
		if (f_type)
			(void)printtype(sp->st_mode);
		if (S_ISLNK(sp->st_mode))
			printlink(p);
		(void)putchar('\n');
	}
}

#define	TAB	8

/*
 * Print the entries, column-sorted.
 */
void
printcol(dp)
	DISPLAY *dp;
{
	extern int termwidth;
	static FTSENT **array;
	static int lastentries = -1;
	FTSENT *p;
	int base, chcnt, cnt, colwidth, num, padcolwidth;
	int endcol, numcols, numrows, row;

	/*
	 * Find out how much space we are going to use for each column,
	 * including padding for all but the last.  This must result in
	 * at least two columns; if not, punt.
	 */
	colwidth = dp->maxlen;
	if (f_inode)
		colwidth += dp->s_inode + 1;
	if (f_size)
		colwidth += dp->s_block + 1;
	if (f_type)
		colwidth += 1;
	padcolwidth = (colwidth + TAB) & ~(TAB - 1);
	if (termwidth < padcolwidth + colwidth) {
		printscol(dp);
		return;
	}

	/*
	 * Have to do random access in the linked list -- build a table
	 * of pointers.
	 */
	if (dp->entries > lastentries) {
		lastentries = dp->entries;
		if ((array =
		    realloc(array, dp->entries * sizeof(FTSENT *))) == NULL) {
			warn(NULL);
			printscol(dp);
		}
	}
	for (p = dp->list, num = 0; p; p = p->fts_link)
		if (p->fts_number != NO_PRINT)
			array[num++] = p;

	/*
	 * Figure out how many columns will fit.  Note that the last one
	 * is at most colwidth wide, so we could test for `>= colwidth',
	 * but then we might have to worry about how line-wrap works.
	 * Then calculate the number of rows when using that many columns.
	 */
	numcols = termwidth / padcolwidth;
	if (termwidth % padcolwidth > colwidth)
		++numcols;
	numrows = num / numcols;
	if (num % numcols)
		++numrows;

	if (dp->list->fts_level != FTS_ROOTLEVEL && (f_longform || f_size))
		(void)printf("total %lu\n", howmany(dp->btotal, blocksize));
	for (row = 0; row < numrows; ++row) {
		endcol = padcolwidth;
		for (base = row, chcnt = 0;;) {
			chcnt +=
			    printaname(array[base], dp->s_inode, dp->s_block);
			if ((base += numrows) >= num)
				break;		/* last entry for this row */
			while ((cnt = (chcnt + TAB & ~(TAB - 1))) <= endcol) {
				(void)putchar('\t');
				chcnt = cnt;
			}
			endcol += padcolwidth;
		}
		(void)putchar('\n');
	}
}

/*
 * print [inode] [size] name
 * return # of characters printed, no trailing characters.
 */
static int
printaname(p, inodefield, sizefield)
	FTSENT *p;
	u_long sizefield, inodefield;
{
	struct stat *sp;
	int chcnt;

	sp = p->fts_statp;
	chcnt = 0;
	if (f_inode)
		chcnt += printf("%*lu ", (int)inodefield, sp->st_ino);
	if (f_size)
		/* XXX bpn - CYGWIN does not accept modifier q */
		chcnt += printf("%*lld ",
		    (int)sizefield, howmany(sp->st_blocks, blocksize));
	chcnt += printf("%s", p->fts_name);
	if (f_type)
		chcnt += printtype(sp->st_mode);
	return (chcnt);
}

static void
printtime(ftime)
	time_t ftime;
{
	static time_t now;
	int i;
	char *longstring;

	longstring = ctime(&ftime);
	for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);

#define	SIXMONTHS	((DAYSPERNYEAR / 2) * SECSPERDAY)
	if (f_sectime)
		for (i = 11; i < 24; i++)
			(void)putchar(longstring[i]);
	else {
		if (now == 0)
			time(&now);
		if (ftime + SIXMONTHS > now && ftime < now + SIXMONTHS)
			for (i = 11; i < 16; ++i)
				(void)putchar(longstring[i]);
		else {
			(void)putchar(' ');
			for (i = 20; i < 24; ++i)
				(void)putchar(longstring[i]);
		}
	}

	(void)putchar(' ');
}

static int
printtype(mode)
	u_int mode;
{
	switch (mode & S_IFMT) {
	case S_IFDIR:
		(void)putchar('/');
		return (1);
	case S_IFIFO:
		(void)putchar('|');
		return (1);
	case S_IFLNK:
		(void)putchar('@');
		return (1);
	case S_IFSOCK:
		(void)putchar('=');
		return (1);
	case S_IFWHT:
		(void)putchar('%');
		return (1);
	}
	if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
		(void)putchar('*');
		return (1);
	}
	return (0);
}

static void
printlink(p)
	FTSENT *p;
{
	int lnklen;
	char name[MAXPATHLEN + 1], path[MAXPATHLEN + 1];
	char *pname;

	if (p->fts_level == FTS_ROOTLEVEL)
		(void)snprintf(name, sizeof(name), "%s", p->fts_name);
	else
		(void)snprintf(name, sizeof(name),
		    "%s/%s", p->fts_parent->fts_accpath, p->fts_name);

	pname = name;
#ifdef __CYGWIN__
	/*
	 * XXX bpn - For CYGWIN, pathnames beginning with '//' are network
	 * pathnames. But it is not possible to have a network path at
	 * this point.
	 */
	if (name[0] == '/' && name[1] == '/')
		pname = &name[1];
#endif
	if ((lnklen = readlink(pname, path, sizeof(path) - 1)) == -1) {
		(void)fprintf(stderr, "\nls: %s: %s\n", pname, strerror(errno));
		return;
	}
	path[lnklen] = '\0';
	(void)printf(" -> %s", path);
}
