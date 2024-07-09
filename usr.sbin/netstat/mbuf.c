/*	BSDI mbuf.c,v 2.8 2001/07/25 20:21:50 chrisk Exp	*/

/*
 * Copyright (c) 1983, 1988, 1993
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
static char sccsid[] = "@(#)mbuf.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>

#include <err.h>
#include <malloc.h>
#include <stdio.h>

#include "netstat.h"

#define	YES	1
typedef int bool;

static struct mbtypes {
	int	mt_type;
	char	*mt_name;
} mbtypes[] = {
	{ MT_DATA,	"data" },
	{ MT_OOBDATA,	"oob data" },
	{ MT_CONTROL,	"ancillary data" },
	{ MT_HEADER,	"packet headers" },
	{ MT_SOCKET,	"socket structures" },			/* XXX */
	{ MT_PCB,	"proto control blocks" },		/* XXX */
	{ MT_RTABLE,	"routing table entries" },		/* XXX */
	{ MT_HTABLE,	"IMP host table entries" },		/* XXX */
	{ MT_ATABLE,	"address resolution tables" },
	{ MT_FTABLE,	"fragment reassembly queue headers" },	/* XXX */
	{ MT_SONAME,	"socket names and addresses" },
	{ MT_SOOPTS,	"socket options" },
	{ MT_RIGHTS,	"access rights" },
	{ MT_IFADDR,	"interface addresses" },		/* XXX */
	{ 0, 0 }
};

static int nmbclusters;
static int nmbclusters_mib[] = { CTL_NET, PF_UNSPEC, SOCTL_NMBCLUSTERS };
static struct data_info nmbclusters_info = {
    "_nmbclusters",
    nmbclusters_mib, sizeof(nmbclusters_mib)/sizeof(*nmbclusters_mib),
    &nmbclusters, sizeof(nmbclusters)
};

static struct mbstat mbstat;
#ifdef	SOCTL_MBSTAT
static int mbstat_mib[] = { CTL_NET, PF_UNSPEC, SOCTL_MBSTAT };
#endif
static struct data_info mbstat_info = {
    "_mbstat",
#if	SOCTL_MBSTAT
    mbstat_mib, sizeof(mbstat_mib),
#else
    NULL, 0,
#endif
    &mbstat, sizeof(mbstat)
};
static int nmbtypes = sizeof(mbstat.m_mtypes) / sizeof(short);

/*
 * Print mbuf statistics.
 */
void
mbpr(void)
{
	struct mbtypes *mp;
	int totmem, totfree, totmbufs;
	int i;
	static bool *seen;			/* "have we seen this type yet?" */

	if (skread("mbuf", &mbstat_info) ||
	    skread("mbuf", &nmbclusters_info))
		return;

	if ((seen = calloc(nmbtypes, sizeof(*seen))) == NULL) {
		warnx("out of memory");
		return;
	}
		
	totmbufs = 0;
	for (mp = mbtypes; mp->mt_name; mp++)
		totmbufs += mbstat.m_mtypes[mp->mt_type];
	printf("%u mbufs in use:\n", totmbufs);
	for (mp = mbtypes; mp->mt_name; mp++)
		if (mbstat.m_mtypes[mp->mt_type]) {
			seen[mp->mt_type] = YES;
			printf("\t%u mbufs allocated to %s\n",
			    mbstat.m_mtypes[mp->mt_type], mp->mt_name);
		}
	seen[MT_FREE] = YES;
	for (i = 0; i < nmbtypes; i++)
		if (!seen[i] && mbstat.m_mtypes[i]) {
			printf("\t%u mbufs allocated to <mbuf type %d>\n",
			    mbstat.m_mtypes[i], i);
		}
	free(seen);
	printf("%u cached free mbufs\n", mbstat.m_mtypes[MT_FREE]);
	printf("%lu/%lu mbuf clusters in use (current limit %lu, max %lu)\n",
	    (u_long)(mbstat.m_clusters - mbstat.m_clfree),
	    (u_long)mbstat.m_clusters,
	    (u_long)nmbclusters,
	    (u_long)mbstat.m_maxclusters);
	totmem = totmbufs * MSIZE + mbstat.m_clusters * MCLBYTES;
	totfree = mbstat.m_clfree * MCLBYTES;
	printf("%lu Kbytes allocated to network (%lu%% in use)\n",
	       (u_long)(totmem / 1024),
	       (u_long)((u_int64_t)(totmem - totfree) * 100 / totmem));
	printf("%lu requests for memory denied\n", mbstat.m_drops);
	printf("%lu requests for memory delayed\n", mbstat.m_wait);
	printf("%lu calls to proto drain routines\n", mbstat.m_drain);
}
