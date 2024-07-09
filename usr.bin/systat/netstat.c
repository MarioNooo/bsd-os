/*	BSDI netstat.c,v 2.5 2003/08/20 23:51:36 polk Exp	*/

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
static char sccsid[] = "@(#)netstat.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * netstat
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <net/route.h>
#include <net/if.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/igmp_var.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "systat.h"
#include "extern.h"
#include "if_util.h"
#include "stats.h"

static void delta __P((void *, void *, void *, size_t));
static char *inetname __P((struct sockaddr *));
static void sum __P((void *, void *, void *, size_t));

#define	YMAX(w)		(maxy-1)

#define	IF_LABEL(w)	0
#define	IF_START(w)	(IF_LABEL(w) + 2)

/* column locations */

#define	STAT_WIDTH	8
#define	STAT_PREC	3
#define	UTIL_WIDTH	3

#define	IF_NAME_WIDTH		IFNAMSIZ
#define	C_IF_NAME		0
#define	C_IF_PACKETS_IN		(C_IF_NAME + IF_NAME_WIDTH + 1)
#define	C_IF_BYTES_IN		(C_IF_PACKETS_IN + STAT_WIDTH + 1)
#define	C_IF_ERRS_IN		(C_IF_BYTES_IN + STAT_WIDTH + 1)
#define	C_IF_UTIL_IN		(C_IF_ERRS_IN + STAT_WIDTH + 1)
#define	C_IF_PACKETS_OUT	(C_IF_UTIL_IN + UTIL_WIDTH + 1)
#define	C_IF_BYTES_OUT		(C_IF_PACKETS_OUT + STAT_WIDTH + 1)
#define	C_IF_ERRS_OUT		(C_IF_BYTES_OUT + STAT_WIDTH + 1)
#define	C_IF_UTIL_OUT		(C_IF_ERRS_OUT + STAT_WIDTH + 1)

#define	ADDR_NAME_WIDTH		24
#define	C_ADDR_NAME		0
#define	C_ADDR_PACKETS_IN	(C_ADDR_NAME + ADDR_NAME_WIDTH + 1)
#define	C_ADDR_BYTES_IN		(C_ADDR_PACKETS_IN + STAT_WIDTH + 1)
#define	C_ADDR_UTIL_IN		(C_ADDR_BYTES_IN + STAT_WIDTH + 1)
#define	C_ADDR_PACKETS_OUT	(C_ADDR_UTIL_IN + UTIL_WIDTH + 1)
#define	C_ADDR_BYTES_OUT	(C_ADDR_PACKETS_OUT + STAT_WIDTH + 1)
#define	C_ADDR_UTIL_OUT		(C_ADDR_BYTES_OUT + STAT_WIDTH + 1)

#define	MIN_WIDTH	40
struct win_state {
	u_int	first_row;
	u_int	cur_row;
	u_int	cur_col;
	u_int	width;
	char buf[LINE_MAX];
};
struct win_state stat_state;

static int aflag;		/* Display all stats */
static int fflag = AF_INET;	/* Address family */
static int iflag;		/* Display all interfaces */
static int nflag;		/* Display addresses numerically */
static int zflag;		/* Zero running stats */
static enum state { BOOT, TIME, RUN } state = TIME;
static int valid;		/* Data is valid */
static time_t speed_time;

#define	STATF_IP	0x01
#define	STATF_ICMP	0x02
#define	STATF_IGMP	0x04
#define	STATF_TCP	0x08
#define	STATF_UDP	0x10
#define	STATF_ALL      	0x1F
static int stat_flags = STATF_ALL;

struct counters {
	u_quad_t packets_in;
	u_quad_t packets_out;
	u_quad_t bytes_in;
	u_quad_t bytes_out;
	u_quad_t errs_in;
	u_quad_t errs_out;
};

struct if_stats {
	char	*ifs_name;
	struct counters ifs_last;
	struct counters ifs_current;
	struct counters ifs_delta;
	u_quad_t ifs_sum;
	u_int	ifs_flags;
	int	ifs_speed;
	int	ifs_util_in;
	int	ifs_util_out;
	TAILQ_ENTRY(if_stats) ifs_sort;
};
static struct if_stats *if_stats, **if_stats_sort;
static int n_ifs, if_maxindex = -1, n_if_sort;

struct addr_stats {
	int ads_delete;
	struct sockaddr_in ads_addr;
	struct counters ads_last;
	struct counters ads_current;
	struct counters ads_delta;
	u_quad_t ads_sum;
	TAILQ_ENTRY(addr_stats) ads_sort;
	TAILQ_ENTRY(addr_stats) ads_hash;
};

#define	IN_ADDRHASH	997
TAILQ_HEAD(addr_stats_hash, addr_stats) addr_stats_hash;
struct addr_stats_hash in_addr_hash[IN_ADDRHASH];	/* hash buckets for local IP addrs */
static struct addr_stats **addr_stats_sort;
static int n_addr_sort;


struct ipstat ipstat_last, ipstat_current, ipstat_delta;
struct icmpstat icmpstat_last, icmpstat_current, icmpstat_delta;
static u_quad_t icmps_received, icmps_sent;
struct igmpstat igmpstat_last, igmpstat_current, igmpstat_delta;
struct tcpstat tcpstat_last, tcpstat_current, tcpstat_delta;
struct udpstat udpstat_last, udpstat_current, udpstat_delta;
static u_quad_t udps_delivered;

#define	FMT(s)	s
#define	FMT_I(s)	"  " s
#define	FMT_Q	"%s"
#include "stats_inet.h"

WINDOW *
opennetstat()
{
	int i;

	for (i = 0; i < IN_ADDRHASH; i++)
		TAILQ_INIT(&in_addr_hash[i]);

	sethostent(1);
	setnetent(1);

	valid = 0;

	return (subwin(stdscr, LINES-R_LOADAV_END-1, 0, R_LOADAV_END, 0));
}


void
closenetstat(w)
        WINDOW *w;
{

	endhostent();
	endnetent();

	/* ... */

        if (w != NULL) {
		wclear(w);
		wrefresh(w);
		delwin(w);
	}
}

int
initnetstat()
{

	icmp_stat_init();

	return (1);
}

static void
fetch_stats(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name)
{
	struct if_data *ifdp;
	struct if_stats *ifsp;
	u_int speed;

	ifdp = &ifm->ifm_data;

	if (ifm->ifm_index > if_maxindex)
		if_maxindex = ifm->ifm_index;

	if (ifm->ifm_index >= n_ifs) {
		int n;

		n = ifm->ifm_index * 8;
		if ((if_stats = realloc(if_stats, n * sizeof(*if_stats)))
		    == NULL) {
			errno = ENOMEM;
			err(1, "");
		}
		memset(if_stats + n_ifs, 0, (n - n_ifs) * sizeof(*if_stats));
		n_ifs = n;
	}
	ifsp = &if_stats[ifm->ifm_index];

	ifsp->ifs_last = ifsp->ifs_current;

	ifsp->ifs_name = strdup(name);
	ifsp->ifs_flags = ifm->ifm_flags;
	ifsp->ifs_speed = ifdp->ifi_baudrate;
	ifsp->ifs_current.packets_in = ifdp->ifi_ipackets;
	ifsp->ifs_current.packets_out = ifdp->ifi_opackets;
	ifsp->ifs_current.bytes_in = ifdp->ifi_ibytes;
	ifsp->ifs_current.bytes_out = ifdp->ifi_obytes;
	ifsp->ifs_current.errs_in = ifdp->ifi_ierrors;
	ifsp->ifs_current.errs_out = ifdp->ifi_oerrors;

	if_stats[0].ifs_current.packets_in += ifdp->ifi_ipackets;
	if_stats[0].ifs_current.packets_out += ifdp->ifi_opackets;
	if_stats[0].ifs_current.bytes_in += ifdp->ifi_ibytes;
	if_stats[0].ifs_current.bytes_out += ifdp->ifi_obytes;
	if_stats[0].ifs_current.errs_in += ifdp->ifi_ierrors;
	if_stats[0].ifs_current.errs_out += ifdp->ifi_oerrors;

	if (state == RUN && !zflag) {
		sum(&ifsp->ifs_delta, &ifsp->ifs_current,
		    &ifsp->ifs_last, sizeof(ifsp->ifs_delta));
	} else {
		delta(&ifsp->ifs_delta, &ifsp->ifs_current,
		    &ifsp->ifs_last, sizeof(ifsp->ifs_delta));
	}

	if ((speed = ifsp->ifs_speed / NBBY * speed_time) != 0) {
		struct counters *cp;

		cp = state == BOOT ? &ifsp->ifs_current : &ifsp->ifs_delta;
		ifsp->ifs_util_in =
		    100 - (100 * (speed - cp->bytes_in) / speed);
		ifsp->ifs_util_out =
		    100 - (100 * (speed - cp->bytes_out) / speed);
	}
}

static void
fetch_stats_addr(struct if_msghdr *ifm, struct sockaddr_dl *sdl, char *name,
    struct ifa_msghdr *ifam, struct rt_addrinfo *rti)
{
	struct if_data *ifdp;
	struct addr_stats *adsp;
	struct addr_stats_hash *adsp_head;
	struct sockaddr_in *addr;
	
	ifdp = &ifm->ifm_data;
	addr = (struct sockaddr_in *)ifaddr(rti);

	if (addr->sin_addr.s_addr == INADDR_ANY)
		return;

	adsp_head = &in_addr_hash[addr->sin_addr.s_addr % IN_ADDRHASH];
	for (adsp = TAILQ_FIRST(adsp_head); adsp != TAILQ_END(adsp_head);
	     adsp = TAILQ_NEXT(adsp, ads_hash)) {
		if (adsp->ads_addr.sin_addr.s_addr == addr->sin_addr.s_addr) {
			break;
		}
	}
	
	if (adsp == TAILQ_END(adsp_head)) {
		adsp = calloc(1, sizeof(*adsp));
		adsp->ads_addr = *addr;
		TAILQ_INSERT_TAIL(adsp_head, adsp, ads_hash);
	} else
		adsp->ads_delete = 0;

	adsp->ads_current.packets_in += ifdp->ifi_ipackets;
	adsp->ads_current.packets_out += ifdp->ifi_opackets;
	adsp->ads_current.bytes_in += ifdp->ifi_ibytes;
	adsp->ads_current.bytes_out += ifdp->ifi_obytes;
}

static int
if_stats_compar(const void *v1, const void *v2)
{
	struct if_stats *ifsp1 = *(struct if_stats **)v1;
	struct if_stats *ifsp2 = *(struct if_stats **)v2;

	return (ifsp2->ifs_sum - ifsp1->ifs_sum);
}

static int
addr_stats_compar(const void *v1, const void *v2)
{
	struct addr_stats *adsp1 = *(struct addr_stats **)v1;
	struct addr_stats *adsp2 = *(struct addr_stats **)v2;

	return (adsp2->ads_sum - adsp1->ads_sum);
}

static void
fetch_sysctl(const char *name, int *mib, size_t mib_len,
    void *vc, void *vl, void *vd, size_t size)
{
	size_t osize;

	memcpy(vl, vc, size);

	osize = size;
	if (sysctl(mib, mib_len/sizeof(*mib), vc, &osize, NULL, 0)
	    != 0)
		err(1, "reading %s", name);
	if (osize != size)
		errx(1, "incomplete data reading %s", name);

	if (state == RUN && !zflag)
		sum(vd, vc, vl, size);
	else
		delta(vd, vc, vl, size);
}

void
fetchnetstat()
{
	struct if_stats *ifsp;
	struct addr_stats *adsp, *adsp_next;
	struct addr_stats_hash *adsp_head;
	int i;
	int ipstat_mib[] = { CTL_NET, AF_INET, IPPROTO_IP, IPCTL_STATS };
	int icmpstat_mib[] = { CTL_NET, AF_INET, IPPROTO_ICMP, ICMPCTL_STATS };
	int igmpstat_mib[] = { CTL_NET, AF_INET, IPPROTO_IGMP, IGMPCTL_STATS };
	int tcpstat_mib[] = { CTL_NET, AF_INET, IPPROTO_TCP, TCPCTL_STATS };
	int udpstat_mib[] = { CTL_NET, AF_INET, IPPROTO_UDP, UDPCTL_STATS };

	fetch_sysctl("ipstat", ipstat_mib, sizeof(ipstat_mib),
	    &ipstat_current, &ipstat_last, &ipstat_delta,
	    sizeof(ipstat_current));

	fetch_sysctl("icmpstat", icmpstat_mib, sizeof(icmpstat_mib),
	    &icmpstat_current, &icmpstat_last, &icmpstat_delta,
	    sizeof(icmpstat_current));
	if (state == BOOT)
		for (i = 0; i < ICMP_MAXTYPE + 1; i++) {
			icmps_received += icmpstat_current.icps_inhist[i];
			icmps_sent += icmpstat_current.icps_outhist[i];
		}
	else {
		if (zflag || state != RUN)
			icmps_sent = icmps_received = 0;
		for (i = 0; i < ICMP_MAXTYPE + 1; i++) {
			icmps_received += icmpstat_delta.icps_inhist[i];
			icmps_sent += icmpstat_delta.icps_outhist[i];
		}
	}

	fetch_sysctl("igmpstat", igmpstat_mib, sizeof(igmpstat_mib),
	    &igmpstat_current, &igmpstat_last, &igmpstat_delta,
	    sizeof(igmpstat_current));

	fetch_sysctl("tcpstat", tcpstat_mib, sizeof(tcpstat_mib),
	    &tcpstat_current, &tcpstat_last, &tcpstat_delta,
	    sizeof(tcpstat_current));

	fetch_sysctl("udpstat", udpstat_mib, sizeof(udpstat_mib),
	    &udpstat_current, &udpstat_last, &udpstat_delta,
	    sizeof(udpstat_current));
	if (state == BOOT)
		udps_delivered = udpstat_current.udps_ipackets -
		    udpstat_current.udps_hdrops - udpstat_current.udps_badlen -
		    udpstat_current.udps_badsum - udpstat_current.udps_noport -
		    udpstat_current.udps_fullsock;
	else
		udps_delivered = udpstat_delta.udps_ipackets -
		    udpstat_delta.udps_hdrops - udpstat_delta.udps_badlen -
		    udpstat_delta.udps_badsum - udpstat_delta.udps_noport -
		    udpstat_delta.udps_fullsock;

	/* Reset the interface list */
	for (i = 1; i <= if_maxindex; i++) {
		if (if_stats[i].ifs_name != NULL) {
			free(if_stats[i].ifs_name);
			if_stats[i].ifs_name = NULL;
		}
	}
	if (if_maxindex > -1) {
		if_stats[0].ifs_last = if_stats[0].ifs_current;
		memset(&if_stats[0].ifs_current, 0,
		    sizeof(if_stats[0].ifs_current));
	}
	if_maxindex = -1;

	/* Reset the address list */
	for (i = 0; i < IN_ADDRHASH; i++) {
		adsp_head = &in_addr_hash[i];
		for (adsp = TAILQ_FIRST(adsp_head); 
		     adsp != TAILQ_END(adsp_head);
		     adsp = TAILQ_NEXT(adsp, ads_hash)) {
			adsp->ads_delete = 1;
			adsp->ads_last = adsp->ads_current;
			memset(&adsp->ads_current, 0, sizeof(adsp->ads_current));
		}
	}

	if (state == RUN && !zflag) {
		speed_time += naptime;
	} else if (state == BOOT) {
		struct timeval time_boot, time_now;

		if (kvm_boottime(kd, &time_boot) < 0)
			error("error reading boot time");
		if (gettimeofday(&time_now, NULL) < 0)
			error("error reading current time");
		speed_time = time_now.tv_sec - time_boot.tv_sec;
	} else
		speed_time = naptime;

	/* Read the statistics */
	if_loop(kd, NULL, fflag, fetch_stats, fetch_stats_addr);

	if (state == RUN && !zflag)
		sum(&if_stats[0].ifs_delta, &if_stats[0].ifs_current,
		    &if_stats[0].ifs_last, sizeof(if_stats[0].ifs_delta));
	else
		delta(&if_stats[0].ifs_delta, &if_stats[0].ifs_current,
		    &if_stats[0].ifs_last, sizeof(if_stats[0].ifs_delta));

	/* Sort the interface list */
	if (if_stats_sort != NULL)
		free(if_stats_sort);
	if ((if_stats_sort = malloc(if_maxindex * sizeof(*if_stats_sort)))
	    == NULL) {
		errno = ENOMEM;
		err(1, "malloc");
	}
	for (n_if_sort = 0, i = 1; i <= if_maxindex; i++)
		if ((ifsp = &if_stats[i])->ifs_name != NULL
			&& (iflag || (ifsp->ifs_flags & IFF_UP))) {
			struct counters *cp;

			cp = state == BOOT ? &ifsp->ifs_current :
			    &ifsp->ifs_delta;
			ifsp->ifs_sum = cp->bytes_in + cp->bytes_out;
			if_stats_sort[n_if_sort++] = ifsp;
		}
	qsort(if_stats_sort, n_if_sort, sizeof(*if_stats_sort), 
	    if_stats_compar);

	/* Calculate deltas for the address list */
	for (n_addr_sort = i = 0; i < IN_ADDRHASH; i++) {
		adsp_head = &in_addr_hash[i];
		for (adsp = TAILQ_FIRST(adsp_head); 
		     adsp != TAILQ_END(adsp_head); adsp = adsp_next) {
			/* Remember the next in case of delte */
			adsp_next = TAILQ_NEXT(adsp, ads_hash);
			if (adsp->ads_delete) {
				/* Remove an unused entry */
				TAILQ_REMOVE(adsp_head, adsp, ads_hash);
				free(adsp);
				continue;
			}
			n_addr_sort++;
		}
	}
	if (addr_stats_sort != NULL)
		free(addr_stats_sort);
	if ((addr_stats_sort = malloc(n_addr_sort * sizeof(*addr_stats_sort)))
	    == NULL) {
		errno = ENOMEM;
		err(1, "malloc");
	}
	for (n_addr_sort = i = 0; i < IN_ADDRHASH; i++) {
		adsp_head = &in_addr_hash[i];
		for (adsp = TAILQ_FIRST(adsp_head); 
		     adsp != TAILQ_END(adsp_head); 
		     adsp = TAILQ_NEXT(adsp, ads_hash)) {

			/* Calculate the deltas */
			if (state == RUN && !zflag)
				sum(&adsp->ads_delta, &adsp->ads_current, 
				    &adsp->ads_last, sizeof(adsp->ads_delta));
			else
				delta(&adsp->ads_delta, &adsp->ads_current, 
				    &adsp->ads_last, sizeof(adsp->ads_delta));
			adsp->ads_sum = adsp->ads_delta.bytes_in + 
			    adsp->ads_delta.bytes_out;
			
			addr_stats_sort[n_addr_sort++] = adsp;
		}
	}
	qsort(addr_stats_sort, n_addr_sort, sizeof(*addr_stats_sort), 
	    addr_stats_compar);

	zflag = 0;
}

void
labelnetstat()
{
	int row;

	wmove(wnd, 0, 0); wclrtobot(wnd);

	row = IF_LABEL(wnd);
	mvwprintw(wnd, row, C_IF_NAME, "%-*.*s",
	    IF_NAME_WIDTH, IF_NAME_WIDTH, "Interfaces");
	mvwprintw(wnd, row, C_IF_PACKETS_IN + STAT_WIDTH, "Input");
	mvwprintw(wnd, row, C_IF_PACKETS_OUT + STAT_WIDTH, "Output");

	row++;
	mvwprintw(wnd, row, C_IF_NAME, "%-*.*s",
	    IF_NAME_WIDTH, IF_NAME_WIDTH, "  (by traffic)");
	mvwprintw(wnd, row, C_IF_PACKETS_IN, "%*s", STAT_WIDTH, "Packets");
	mvwprintw(wnd, row, C_IF_BYTES_IN, "%*s", STAT_WIDTH, "Bytes");
	mvwprintw(wnd, row, C_IF_ERRS_IN, "%*s", STAT_WIDTH, "Errors");
	mvwprintw(wnd, row, C_IF_UTIL_IN, "%*s", UTIL_WIDTH, "%");
	mvwprintw(wnd, row, C_IF_PACKETS_OUT, "%*s", STAT_WIDTH, "Packets");
	mvwprintw(wnd, row, C_IF_BYTES_OUT, "%*s", STAT_WIDTH, "Bytes");
	mvwprintw(wnd, row, C_IF_ERRS_OUT, "%*s", STAT_WIDTH, "Errors");
	mvwprintw(wnd, row, C_IF_UTIL_OUT, "%*s", UTIL_WIDTH, "%");
}

static char *
print_stat(char *buf, int width, int prec, u_quad_t stat)
{

	if (stat < 10000)
		(void)sprintf(buf, "%*qu", width, stat);
	else if (stat < 1000000)
		(void)sprintf(buf, "%*.*gk",
		    width - 1, prec, stat / 1000.0);
	else if (stat < 1000000000)
		(void)sprintf(buf, "%*.*gM",
		    width - 1, prec, stat / 1000000.0);
	else
		(void)sprintf(buf, "%*.*gG",
		    width - 1, prec, stat / 1000000000.0);

	return (buf);
}

static void
print_stats(WINDOW *wnd, struct win_state *ws, int prec, 
    struct stats *stats, void *ssd, const char *head)
{
	struct stats *sp;

	for (sp = stats; sp->s_flags != 0; sp++) {
		u_quad_t stat1, stat2;
		const char *plural;
		char buf1[STAT_WIDTH + 5], buf2[STAT_WIDTH + 5];

		if (sp->s_flags & S_FMT_DIRECT)
			stat1 = *(u_quad_t *)sp->s_offset[0];
		else
			stat1 = *((u_quad_t *)(ssd + sp->s_offset[0]));
		if (stat1 == 0) {
			if ((sp->s_aux == 0 || sp->s_aux-- == 0)
			    && (!aflag || sp->s_flags & S_FMT_SKIPZERO))
				continue;
		} else
			sp->s_aux = 10;
		(void)print_stat(buf1, 0, prec, stat1);

		switch (S_FMT_TYPE(sp->s_flags)) {
		case S_FMT_5:
			if (stat1 != 0 ||
			    (aflag && !(sp->s_flags & S_FMT_SKIPZERO))) {
				(void)snprintf(ws->buf, ws->width, 
				    sp->s_fmt, buf1);
				break;
			}
			continue;

		case S_FMT1:
			plural = stat1 == 1 ? "" : "s";
			goto fmt1;

		case S_FMT1ES:
			plural = stat1 == 1 ? "" : "es";
			goto fmt1;

		case S_FMT1IES:
			plural = stat1 == 1 ? "y" : "ies";
			/* Fall through */

		fmt1:
			if (stat1 != 0 ||
				(aflag && !(sp->s_flags & S_FMT_SKIPZERO))) {
				(void)snprintf(ws->buf, ws->width,
				    sp->s_fmt, buf1, plural);
				break;
			}
			continue;

		case S_FMT1_5:
			stat2 = *((u_quad_t *)(ssd + sp->s_offset[1]));
			(void)print_stat(buf2, 0, prec, stat2);
			if (stat1 != 0 || stat2 != 0 ||
			    (aflag && !(sp->s_flags & S_FMT_SKIPZERO))) {
				(void)snprintf(ws->buf, ws->width, 
				    sp->s_fmt, buf1, stat1 == 1 ? "" : "s",
				    buf2);
				break;
			}
			continue;

		case S_FMT2:
			stat2 = *((u_quad_t *)(ssd + sp->s_offset[1]));
			(void)print_stat(buf2, 0, prec, stat2);
			if (stat1 != 0 || stat2 != 0 ||
			    (aflag && !(sp->s_flags & S_FMT_SKIPZERO))) {
				(void)snprintf(ws->buf, ws->width,
				    sp->s_fmt,
				    buf1, stat1 == 1 ? "" : "s",
				    buf2, stat2 == 1 ? "" : "s");
				break;
			}
			continue;
		}

		if ((ws->cur_col + ws->width) > maxx)
			break;

		if (head != NULL) {
			mvwprintw(wnd, ws->cur_row, ws->cur_col,
			    "--%s--", head);
			head = NULL;
			if (++(ws->cur_row) > YMAX(wnd)) {
				ws->cur_row = ws->first_row;
				ws->cur_col += ws->width;
			}
		}

		mvwaddstr(wnd, ws->cur_row, ws->cur_col, ws->buf);
		if (++(ws->cur_row) > YMAX(wnd)) {
			ws->cur_row = ws->first_row;
			ws->cur_col += ws->width;

		}
	}
}

void
shownetstat()
{
	struct if_stats *ifsp;
	struct counters *cp;
	u_int erow, row, j, if_rows, addr_rows;
	char buf[STAT_WIDTH + 5];

	if (!valid) {
		valid = 1;
		return;
	}

	if (stat_flags)
		j = YMAX(wnd) / 3;
	else
		j = YMAX(wnd);
	if (n_addr_sort + 4 < j / 2)
		if_rows = j - n_addr_sort - 4;
	else
		if_rows = j / 2;
	addr_rows = j - if_rows;

	row = IF_START(wnd);
	wmove(wnd, row, 0); wclrtobot(wnd);
	for (j = 0, erow = row + MIN(n_if_sort, if_rows); row < erow - 1; 
	     row++, j++) {
		ifsp = if_stats_sort[j];
		cp = (state == BOOT) ? &ifsp->ifs_current : &ifsp->ifs_delta;
		mvwprintw(wnd, row, C_IF_NAME, "%-*.*s",
		    IF_NAME_WIDTH, IF_NAME_WIDTH, ifsp->ifs_name);
		mvwaddstr(wnd, row, C_IF_PACKETS_IN, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_in));
		mvwaddstr(wnd, row, C_IF_BYTES_IN, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_in));
		mvwaddstr(wnd, row, C_IF_ERRS_IN, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->errs_in));
		if (ifsp->ifs_speed)
			mvwprintw(wnd, row, C_IF_UTIL_IN, "%*d", 
			    UTIL_WIDTH, ifsp->ifs_util_in);
		mvwaddstr(wnd, row, C_IF_PACKETS_OUT, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_out));
		mvwaddstr(wnd, row, C_IF_BYTES_OUT, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_out));
		mvwaddstr(wnd, row, C_IF_ERRS_OUT, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->errs_in));
		if (ifsp->ifs_speed)
			mvwprintw(wnd, row, C_IF_UTIL_OUT, "%*d", 
			    UTIL_WIDTH, ifsp->ifs_util_out);
	}
	/* Print totals for all interfaces */
	ifsp = &if_stats[0];
	cp = (state == BOOT) ? &ifsp->ifs_current : &ifsp->ifs_delta;
	mvwprintw(wnd, row, C_IF_NAME, "%-*.*s", 
	    IF_NAME_WIDTH, IF_NAME_WIDTH, "Totals");
	mvwaddstr(wnd, row, C_IF_PACKETS_IN, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_in));
	mvwaddstr(wnd, row, C_IF_BYTES_IN, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_in));
	mvwaddstr(wnd, row, C_IF_ERRS_IN, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->errs_in));
	mvwaddstr(wnd, row, C_IF_BYTES_OUT, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_out));
	mvwaddstr(wnd, row, C_IF_PACKETS_OUT, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_out));
	mvwaddstr(wnd, row, C_IF_ERRS_OUT, 
	    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->errs_out));

	/* Print the address statistics */
	row += 2;
	mvwprintw(wnd, row, C_ADDR_NAME, "%-*.*s", 
	    ADDR_NAME_WIDTH, ADDR_NAME_WIDTH, "Addresses");
	mvwprintw(wnd, row, C_ADDR_PACKETS_IN + STAT_WIDTH, "Input");
	mvwprintw(wnd, row, C_ADDR_PACKETS_OUT + STAT_WIDTH, "Output");
	row++;
	mvwprintw(wnd, row, C_ADDR_NAME, "%-*.*s",
	    ADDR_NAME_WIDTH, ADDR_NAME_WIDTH, "  (by traffic)");
	mvwprintw(wnd, row, C_ADDR_PACKETS_IN, "%*s", STAT_WIDTH, "Packets");
	mvwprintw(wnd, row, C_ADDR_BYTES_IN, "%*s", STAT_WIDTH, "Bytes");
	mvwprintw(wnd, row, C_ADDR_PACKETS_OUT, "%*s", STAT_WIDTH, "Packets");
	mvwprintw(wnd, row, C_ADDR_BYTES_OUT, "%*s", STAT_WIDTH, "Bytes");
	for (row++, j = 0, erow = row + MIN(n_addr_sort, addr_rows);
	     row < erow; row++, j++) {
		struct addr_stats *adsp = addr_stats_sort[j];

		cp = (state == BOOT) ? &adsp->ads_current : &adsp->ads_delta;
		mvwprintw(wnd, row, C_ADDR_NAME, "%-*.*s",
		    ADDR_NAME_WIDTH, ADDR_NAME_WIDTH, 
		    inetname((struct sockaddr *)&adsp->ads_addr));
		mvwaddstr(wnd, row, C_ADDR_PACKETS_IN, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_in));
		mvwaddstr(wnd, row, C_ADDR_PACKETS_OUT, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->packets_out));
		mvwaddstr(wnd, row, C_ADDR_BYTES_IN, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_in));
		mvwaddstr(wnd, row, C_ADDR_BYTES_OUT, 
		    print_stat(buf, STAT_WIDTH, STAT_PREC, cp->bytes_out));
	}
	row++;

	memset(&stat_state, 0, sizeof(stat_state));
	stat_state.cur_row = stat_state.first_row = row;
	if (maxx < MIN_WIDTH)
		stat_state.width = maxx;
	else
		stat_state.width = maxx / (maxx / MIN_WIDTH);
	if (stat_flags & STATF_IP)
		print_stats(wnd, &stat_state, 3, ip_stat,
		    (state == BOOT) ? &ipstat_current : &ipstat_delta, "IP");
	if (stat_flags & STATF_ICMP)
		print_stats(wnd, &stat_state, 3, icmp_stat,
		    (state == BOOT) ? &icmpstat_current : &icmpstat_delta, "ICMP");
	if (stat_flags & STATF_IGMP)
		print_stats(wnd, &stat_state, 3, igmp_stat,
		    (state == BOOT) ? &igmpstat_current : &igmpstat_delta, "IGMP");	
	if (stat_flags & STATF_TCP)
		print_stats(wnd, &stat_state, 3, tcp_stat,
		    (state == BOOT) ? &tcpstat_current : &tcpstat_delta, "TCP");
	if (stat_flags & STATF_UDP)
		print_stats(wnd, &stat_state, 3, udp_stat,
		    (state == BOOT) ? &udpstat_current : &udpstat_delta, "UDP");
}

int
cmdnetstat(cmd, args)
	char *cmd, *args;
{
	int new;

	if (prefix(cmd, "all")) {
		mvprintw(CMDLINE, 0,
		    "ambiguous command: %s", cmd);
		clrtoeol();
		return (1);
	} else if (prefix(cmd, "allifs"))
		iflag = !iflag;
	else if (prefix(cmd, "allstats"))
		aflag = !aflag;
	else if (prefix(cmd, "boot")) {
		state = BOOT;
	} else if (prefix(cmd, "time"))
		state = TIME;
	else if  ((new = prefix(cmd, "numbers")) || prefix(cmd, "names")) {
		if (new == nflag)
			return (1);
		nflag = new;
	} else if (prefix(cmd, "reset")) {
		stat_flags = STATF_ALL;
		nflag = aflag = 0;
		state = TIME;
	} else if (prefix(cmd, "run")) {
		zflag = 1;
		state = RUN;
	} else if (prefix(cmd, "display")) {
		int new_flags;
		char *ap;

		if (*args == 0) {
			wmove(wnd, CMDLINE, 0);
			wclrtoeol(wnd);
			if (stat_flags == STATF_ALL)
				printf(" all");
			if (stat_flags == 0)
				printf(" none");
			else
				printf("%s%s%s%s%s",
				    stat_flags & STATF_IP ? " ip" : "",
				    stat_flags & STATF_ICMP ? " icmp" : "",
				    stat_flags & STATF_IGMP ? " igmp" : "",
				    stat_flags & STATF_TCP ? " tcp" : "",
				    stat_flags & STATF_UDP ? " udp" : "");
			return (1);
		}
		new_flags = 0;
		while ((ap = strsep(&args, " ")) != NULL) {
			int new_flag;

			if (*ap == 0)
				continue;
			if (prefix(ap, "all")) {
				new_flags = STATF_ALL;
				continue;
			} else if (prefix(ap, "none")) {
				new_flags = 0;
				continue;
			}
			if (*ap == '!') {
				new = 0;
				ap++;
			} else
				new = 1;
			new_flag = 0;
			if (prefix(ap, "tcp")) {
				new_flag = STATF_TCP;
			} else if (prefix(ap, "udp")) {
				new_flag = STATF_UDP;
			} else if (prefix(ap, "ip")) {
				new_flag = STATF_IP;
			} else if (prefix(ap, "icmp")) {
				new_flag = STATF_ICMP;
			} else if (prefix(ap, "igmp")) {
				new_flag = STATF_IGMP;
			} else {
				mvprintw(CMDLINE, 0,
				    "invalid argument: %s", ap);
				clrtoeol();
				return (1);
			}
			if (new)
				new_flags |= new_flag;
			else
				new_flags &= ~new_flag;
		}
		if (new_flags == stat_flags)
			return (1);
		stat_flags = new_flags;
	} else if (prefix(cmd, "zero")) {
		if (state != RUN) {
			mvaddstr(CMDLINE, 0, "not in run mode");
			clrtoeol();
			return (1);
		}
		zflag = 1;
	} else
		return (0);
			
	shownetstat();
	refresh();
	return (1);
}

static void
sum(void *vd, void *vc, void *vl, size_t size)
{
	u_quad_t *c, *l, *d, *e;

	for (c = vc, l = vl, d = vd, e = vc + size ; c < e; c++, l++, d++)
		*d += *c - *l;
}

static void
delta(void *vd, void *vc, void *vl, size_t size)
{
	u_quad_t *c, *l, *d, *e;

	for (c = vc, l = vl, d = vd, e = vc + size ; c < e; c++, l++, d++)
		*d = *c - *l;
}

/**/

/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give 
 * numeric value, otherwise try for symbolic name.
 */
static char *
inetname(struct sockaddr *addr)
{
	static char hostname[MAXHOSTNAMELEN];
	int flags = 0;

	if (nflag)
		flags |= NI_NUMERICHOST;

	(void)getnameinfo(addr, addr->sa_len, hostname, sizeof(hostname), NULL,
	    0, NI_NOFQDN | flags);

	return (hostname);
}
