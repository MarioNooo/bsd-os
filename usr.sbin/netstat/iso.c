/*	BSDI iso.c,v 2.7 1997/10/31 04:13:39 jch Exp	*/
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
static char sccsid[] = "@(#)iso.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * /master/usr.sbin/netstat/iso.c,v 2.7 1997/10/31 04:13:39 jch Exp
 * /master/usr.sbin/netstat/iso.c,v
 */
/*******************************************************************************
	          Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*******************************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>

#include <netiso/iso.h>
#include <netiso/iso_errno.h>
#include <netiso/clnp.h>
#include <netiso/esis.h>
#include <netiso/clnp_stat.h>
#include <netiso/argo_debug.h>
#undef satosiso
#include <netiso/tp_param.h>
#include <netiso/tp_states.h>
#include <netiso/tp_pcb.h>
#include <netiso/tp_stat.h>
#include <netiso/iso_pcb.h>
#include <netiso/cltp_var.h>
#include <netiso/cons.h>
#ifdef IncStat
#undef IncStat
#endif
#include <netiso/cons_pcb.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netstat.h"

static void isonetprint __P((struct sockaddr_iso *, int));
static void hexprint __P((int, char *, char *));

/*
 *	Dump esis stats
 */

static struct esis_stat esis_stat;
static struct data_info esis_stat_info = {
	"_esis_stat",
	NULL, 0,
	&esis_stat, sizeof(esis_stat)
};

void
esis_stats(char *name)
{

	if (skread(name, &esis_stat_info))
		return;

	printf("%s:\n", name);

	printf("\t%d esh sent, %d esh rcvd\n", esis_stat.es_eshsent,
	    esis_stat.es_eshrcvd);
	printf("\t%d ish sent, %d ish rcvd\n", esis_stat.es_ishsent,
	    esis_stat.es_ishrcvd);
	printf("\t%d rd sent, %d rd rcvd\n", esis_stat.es_rdsent,
	    esis_stat.es_rdrcvd);
	printf("\t%d pdus not sent: insufficient memory\n",
	    esis_stat.es_nomem);
	printf("\t%d pdus rcvd w/bad cksum\n", esis_stat.es_badcsum);
	printf("\t%d pdus rcvd w/bad version number\n",
	    esis_stat.es_badvers);
	printf("\t%d pdus rcvd w/bad type field\n",
	    esis_stat.es_badtype);
	printf("\t%d short pdus rcvd\n", esis_stat.es_toosmall);
}

/*
 * Dump clnp statistics structure.
 */

static struct data_info clnp_stat_info = {
	"_clnp_stat",
	NULL, 0,
	&clnp_stat, sizeof(clnp_stat)
};

void
clnp_stats(char *name)
{

	if (skread(name, &clnp_stat_info))
		return;

	printf("%s:\n\t%d total pkts sent\n", name, clnp_stat.cns_sent);
	printf("\t%d total fragments sent\n", clnp_stat.cns_fragments);
	printf("\t%d total pkts rcvd\n", clnp_stat.cns_total);
	printf("\t%d w/fixed part of header too small\n",
	    clnp_stat.cns_toosmall);
	printf("\t%d w/header length not reasonable\n", 
	    clnp_stat.cns_badhlen);
	printf("\t%d incorrect cksum%s\n",
	    clnp_stat.cns_badcsum, PLURAL(clnp_stat.cns_badcsum));
	printf("\t%d w/unreasonable address lengths\n", 
	    clnp_stat.cns_badaddr);
	printf("\t%d w/forgotten segmentation information\n",
	    clnp_stat.cns_noseg);
	printf("\t%d w/an incorrect proto identifier\n", 
	    clnp_stat.cns_noproto);
	printf("\t%d w/an incorrect version\n", clnp_stat.cns_badvers);
	printf("\t%d dropped because the ttl has expired\n",
	    clnp_stat.cns_ttlexpired);
	printf("\t%d clnp cache misses\n", clnp_stat.cns_cachemiss);
	printf("\t%d clnp congestion experience bits set\n",
	    clnp_stat.cns_congest_set);
	printf("\t%d clnp congestion experience bits rcvd\n",
	    clnp_stat.cns_congest_rcvd);
}

/*
 * Dump CLTP statistics structure.
 */

static struct cltpstat cltpstat;
static struct data_info cltpstat_info = {
	"_cltpstat",
	NULL, 0,
	&cltpstat, sizeof(cltpstat)
};

void
cltp_stats(char *name)
{

	if (skread(name, &cltpstat_info))
		return;

	printf("%s:\n\t%u incomplete header%s\n", name,
	    cltpstat.cltps_hdrops, PLURAL(cltpstat.cltps_hdrops));
	printf("\t%u bad data length field%s\n",
	    cltpstat.cltps_badlen, PLURAL(cltpstat.cltps_badlen));
	printf("\t%u bad cksum%s\n",
	    cltpstat.cltps_badsum, PLURAL(cltpstat.cltps_badsum));
}

struct	tp_pcb tpcb;
struct	isopcb isopcb;
struct	socket sockb;
union	{
	struct sockaddr_iso	siso;
	char	data[128];
} laddr, faddr;
#define kget(o, p) \
	do { \
		if (kread((u_long)(o), &p, sizeof (p))) \
			return; \
	} while (0)

static	int first = 1;


static u_long tp;
struct data_info tp_info = {
	"_tp_refinfo",
	NULL, 0,
	&tp, sizeof(tp)
};

static u_long cltb;
struct data_info cltp_info = {
	"_cltb",
	NULL, 0,
	&cltb, sizeof(cltb)
};

/*
 * Print a summary of connections related to an Internet
 * protocol.  For TP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -v (all) flag is specified.
 */
void
iso_protopr(char *name, struct data_info *dip)
{
	struct isopcb cb;
	struct isopcb *prev, *next;

	if (skread(name, dip))
		return;

	if (strcmp(name, "tp") == 0) {
		tp_protopr(dip->di_off, name);
		return;
	}
	if (kread(dip->di_off, &cb, sizeof(cb)))
		return;
	isopcb = cb;
	prev = (struct isopcb *)dip->di_off;
	if (isopcb.isop_next == (struct isopcb *)dip->di_off)
		return;
	while (isopcb.isop_next != (struct isopcb *)dip->di_off) {
		next = isopcb.isop_next;
		kget(next, isopcb);
		if (isopcb.isop_prev != prev) {
			printf("prev %p next %p isop_prev %p isop_next %p???\n",
			    prev, next, isopcb.isop_prev, isopcb.isop_next);
			break;
		}
		kget(isopcb.isop_socket, sockb);
		iso_protopr1((u_long)next, 0);
		putchar('\n');
		prev = next;
	}
}

void
iso_protopr1(u_long kern_addr, int istp)
{
	if (first) {
		printf("Active ISO net connections");
		if (aflag)
			printf(" (including servers)");
		putchar('\n');
		if (Aflag)
			printf("%-8.8s ", "PCB");
		printf(Aflag ?
		    "%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
		    "%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
		    "Proto", "Recv-Q", "Send-Q",
		    "Local Address", "Foreign Address", "(state)");
		first = 0;
	}
	if (Aflag)
		printf("%8p ",
		    (sockb.so_pcb ? (void *)sockb.so_pcb : (void *)kern_addr));
	printf("%-5.5s %6lu %6lu ", "tp", sockb.so_rcv.sb_cc, 
	    sockb.so_snd.sb_cc);
	if (istp && tpcb.tp_lsuffixlen) {
		hexprint(tpcb.tp_lsuffixlen, tpcb.tp_lsuffix, "()");
		printf("\t");
	} else if (isopcb.isop_laddr == NULL)
		printf("*.*\t");
	else {
		if ((char *)isopcb.isop_laddr == ((char *)kern_addr) +
		    _offsetof(struct isopcb, isop_sladdr))
			laddr.siso = isopcb.isop_sladdr;
		else
			kget(isopcb.isop_laddr, laddr);
		isonetprint((struct sockaddr_iso *)&laddr, 1);
	}
	if (istp && tpcb.tp_fsuffixlen) {
		hexprint(tpcb.tp_fsuffixlen, tpcb.tp_fsuffix, "()");
		printf("\t");
	} else if (isopcb.isop_faddr == NULL)
		printf("*.*\t");
	else {
		if ((char *)isopcb.isop_faddr == ((char *)kern_addr) +
		    _offsetof(struct isopcb, isop_sfaddr))
			faddr.siso = isopcb.isop_sfaddr;
		else
			kget(isopcb.isop_faddr, faddr);
		isonetprint((struct sockaddr_iso *)&faddr, 0);
	}
}

void
tp_protopr(u_long off, char *name)
{
	extern char *tp_sstring[];
	struct tp_ref *tpr, *tpr_base;
	struct tp_refinfo tpkerninfo;
	int size;

	kget(off, tpkerninfo);
	size = tpkerninfo.tpr_size * sizeof (*tpr);
	tpr_base = (struct tp_ref *)malloc(size);
	if (tpr_base == NULL)
		return;
	if (kread((u_long)(tpkerninfo.tpr_base), tpr_base, size))
		return;
	for (tpr = tpr_base; tpr < tpr_base + tpkerninfo.tpr_size; tpr++) {
		if (tpr->tpr_pcb == NULL)
			continue;
		kget(tpr->tpr_pcb, tpcb);
		if (tpcb.tp_state == ST_ERROR)
			printf("undefined tpcb state: %p\n", tpr->tpr_pcb);
		if (!aflag &&
			(tpcb.tp_state == TP_LISTENING ||
			 tpcb.tp_state == TP_CLOSED ||
			 tpcb.tp_state == TP_REFWAIT)) {
			continue;
		}
		kget(tpcb.tp_sock, sockb);
		if (tpcb.tp_npcb) switch(tpcb.tp_netservice) {
			case IN_CLNS:
				tp_inproto((u_long)tpkerninfo.tpr_base);
				break;
			default:
				kget(tpcb.tp_npcb, isopcb);
				iso_protopr1((u_long)tpcb.tp_npcb, 1);
				break;
		}
		if (tpcb.tp_state >= tp_NSTATES)
			printf(" %d", tpcb.tp_state);
		else
			printf(" %-12.12s", tp_sstring[tpcb.tp_state]);
		putchar('\n');
	}
}

void
tp_inproto(u_long pcb)
{
	struct inpcb inpcb;
	kget(tpcb.tp_npcb, inpcb);
	if (!aflag && inet_lnaof(inpcb.inp_laddr) == INADDR_ANY)
		return;
	if (Aflag)
		printf("%08lx ", pcb);
	printf("%-5.5s %6lu %6lu ", "tpip",
	    sockb.so_rcv.sb_cc, sockb.so_snd.sb_cc);
	inetprint(AF_INET, &inpcb.inp_laddr, inpcb.inp_lport, "tp");
	inetprint(AF_INET, &inpcb.inp_faddr, inpcb.inp_fport, "tp");
}

/*
 * Pretty print an iso address (net address + port).
 * If the nflag was specified, use numbers instead of names.
 */

#ifdef notdef
char *
isonetname(struct iso_addr *iso)
{
	struct sockaddr_iso sa;
	struct iso_hostent *ihe = 0;
	struct iso_hostent *iso_gethostentrybyaddr();
	struct iso_hostent *iso_getserventrybytsel();
	struct iso_hostent Ihe;
	static char line[80];

	bzero(line, sizeof(line));
	if (iso->isoa_afi) {
		sa.siso_family = AF_ISO;
		sa.siso_addr = *iso;
		sa.siso_tsuffix = 0;

		if (!nflag)
			ihe = iso_gethostentrybyaddr(&sa, 0, 0);
		if (ihe) {
			Ihe = *ihe;
			ihe = &Ihe;
			(void)sprintf(line, "%s", ihe->isoh_hname);
		} else {
			(void)sprintf(line, "%s", iso_ntoa(iso));
		}
	} else
		(void)sprintf(line, "*");
	return line;
}

static void
isonetprint(struct iso_addr *iso, char *sufx, u_short sufxlen, int islocal)
{
	struct iso_hostent *iso_getserventrybytsel(), *ihe;
	struct iso_hostent Ihe;
	char *line, *cp;
	int Alen = Aflag?18:22;

	line =  isonetname(iso);
	cp = index(line, '\0');
	ihe = NULL;

	if (islocal)
		islocal = 20;
	else
		islocal = 22 + Alen;

	if (Aflag)
		islocal += 10 ;

	if (!nflag) {
		if ((cp -line)>10) {
			cp = line+10;
			bzero(cp, sizeof(line)-10);
		}
	}

	*cp++ = '.';
	if (sufxlen) {
		if (!Aflag && !nflag &&
		    (ihe = iso_getserventrybytsel(sufx, sufxlen)) != NULL) {
			Ihe = *ihe;
			ihe = &Ihe;
		}
		if (ihe != NULL && (strlen(ihe->isoh_aname) > 0))
			(void)sprintf(cp, "%s", ihe->isoh_aname);
		else
			iso_sprinttsel(cp, sufx, sufxlen);
	} else
		(void)sprintf(cp, "*");
	/*
	fprintf(stdout, Aflag?" %-18.18s":" %-22.22s", line);
	*/

	if (strlen(line) > Alen) {
		printf(" %s", line);
		printf("\n %*.s", islocal+Alen," ");
	} else
		printf(" %-*.*s", Alen, Alen,line);
}
#endif

#ifdef notdef
static void
x25_protopr(u_long off, char *name)
{
	static char *xpcb_states[] = {
		"CLOSED",
		"LISTENING",
		"CLOSING",
		"CONNECTING",
		"ACKWAIT",
		"OPEN",
	};
	struct isopcb *prev, *next;
	struct x25_pcb xpcb;

	if (off == 0) {
		printf("%s control block: symbol not in namelist\n", name);
		return;
	}
	if (kread(off, &xpcb, sizeof (struct x25_pcb)))
		return;
	prev = (struct isopcb *)off;
	if (xpcb.x_next == (struct isopcb *)off)
		return;
	while (xpcb.x_next != (struct isopcb *)off) {
		next = isopcb.isop_next;
		if (kread((u_long)next, &xpcb, sizeof (struct x25_pcb)))
			return;
		if (xpcb.x_prev != prev) {
			printf("???\n");
			break;
		}
		if (kread((u_long)xpcb.x_socket, &sockb, sizeof (sockb)))
			return;

		if (!aflag &&
			xpcb.x_state == LISTENING ||
			xpcb.x_state == TP_CLOSED) {
			prev = next;
			continue;
		}
		if (first) {
			printf("Active X25 net connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
			    "%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
			    "%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
			    "Proto", "Recv-Q", "Send-Q",
			    "Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		printf("%-5.5s %6d %6d ", name, sockb.so_rcv.sb_cc,
		    sockb.so_snd.sb_cc);
		isonetprint(&xpcb.x_laddr.siso_addr, &xpcb.x_lport,
		    sizeof(xpcb.x_lport), 1);
		isonetprint(&xpcb.x_faddr.siso_addr, &xpcb.x_fport,
		    sizeof(xpcb.x_lport), 0);
		if (xpcb.x_state < 0 || xpcb.x_state >= x25_NSTATES)
			printf(" 0x0x0x0x0x0x0x0x0x%x", xpcb.x_state);
		else
			printf(" %-12.12s", xpcb_states[xpcb.x_state]);
		putchar('\n');
		prev = next;
	}
}
#endif

static struct data_info tp_stat_info = {
	"_tp_stat",
	NULL, 0,
	&tp_stat, sizeof(tp_stat)
};

void
tp_stats(char *name)
{
	int j;

	if (skread(name, &tp_stat_info))
		return;

	printf("%s:\n", name);

	printf("Receiving:\n");
	printf("\t%qu variable parameter%s ignored\n",
	    (u_quad_t)tp_stat.ts_param_ignored ,PLURAL(tp_stat.ts_param_ignored));
	printf("\t%qu invalid parameter code%s\n",
	    (u_quad_t)tp_stat.ts_inv_pcode ,PLURAL(tp_stat.ts_inv_pcode));
	printf("\t%qu invalid parameter value%s\n",
	    (u_quad_t)tp_stat.ts_inv_pval ,PLURAL(tp_stat.ts_inv_pval));
	printf("\t%qu invalid dutype%s\n",
	    (u_quad_t)tp_stat.ts_inv_dutype ,PLURAL(tp_stat.ts_inv_dutype));
	printf("\t%qu negotiation failure%s\n",
	    (u_quad_t)tp_stat.ts_negotfailed ,PLURAL(tp_stat.ts_negotfailed));
	printf("\t%qu invalid destination reference%s\n",
	    (u_quad_t)tp_stat.ts_inv_dref ,PLURAL(tp_stat.ts_inv_dref));
	printf("\t%qu invalid suffix parameter%s\n",
	    (u_quad_t)tp_stat.ts_inv_sufx ,PLURAL(tp_stat.ts_inv_sufx));
	printf("\t%qu invalid length\n",
	    (u_quad_t)tp_stat.ts_inv_length);
	printf("\t%qu invalid cksum%s\n",
	    (u_quad_t)tp_stat.ts_bad_csum ,PLURAL(tp_stat.ts_bad_csum));
	printf("\t%qu DT%s out of order\n",
	    (u_quad_t)tp_stat.ts_dt_ooo ,PLURAL(tp_stat.ts_dt_ooo));
	printf("\t%qu DT%s not in window\n",
	    (u_quad_t)tp_stat.ts_dt_niw ,PLURAL(tp_stat.ts_dt_niw));
	printf("\t%qu dup DT%s\n",
	    (u_quad_t)tp_stat.ts_dt_dup ,PLURAL(tp_stat.ts_dt_dup));
	printf("\t%qu XPD%s not in window\n",
	    (u_quad_t)tp_stat.ts_xpd_niw ,PLURAL(tp_stat.ts_xpd_niw));
	printf("\t%qu XPD%s w/o credit to stash\n",
	    (u_quad_t)tp_stat.ts_xpd_dup ,PLURAL(tp_stat.ts_xpd_dup));
	printf("\t%qu time%s local credit reneged\n",
	    (u_quad_t)tp_stat.ts_lcdt_reduced ,PLURAL(tp_stat.ts_lcdt_reduced));
	printf("\t%qu concatenated TPDU%s\n",
	    (u_quad_t)tp_stat.ts_concat_rcvd ,PLURAL(tp_stat.ts_concat_rcvd));
	printf("Sending:\n");
	printf("\t%qu XPD mark%s dropped\n",
	    (u_quad_t)tp_stat.ts_xpdmark_del ,PLURAL(tp_stat.ts_xpdmark_del));
	printf("\tXPD stopped data flow %qu time%s\n",
	    (u_quad_t)tp_stat.ts_xpd_intheway ,PLURAL(tp_stat.ts_xpd_intheway));
	printf("\t%qu time%s foreign window closed\n",
	    (u_quad_t)tp_stat.ts_zfcdt ,PLURAL(tp_stat.ts_zfcdt));
	printf("Miscellaneous:\n");
	printf("\t%qu small mbuf%s\n",
	    (u_quad_t)tp_stat.ts_mb_small ,PLURAL(tp_stat.ts_mb_small));
	printf("\t%qu cluster%s\n",
	    (u_quad_t)tp_stat.ts_mb_cluster, PLURAL(tp_stat.ts_mb_cluster));
	printf("\t%qu source quench \n",
	    (u_quad_t)tp_stat.ts_quench);
	printf("\t%qu dec bit%s\n",
	    (u_quad_t)tp_stat.ts_rcvdecbit, PLURAL(tp_stat.ts_rcvdecbit));
	printf("\tM:L ( M mbuf chains of length L)\n");

	printf("\t%qu: over 16\n",
	    (u_quad_t)tp_stat.ts_mb_len_distr[0]);
	for (j = 1; j <= 8; j++) {
		printf("\t%qu: %d\t\t%qu: %d\n",
		    (u_quad_t)tp_stat.ts_mb_len_distr[j],j,
		    (u_quad_t)tp_stat.ts_mb_len_distr[j<<1],j<<1);
	}

	printf("\t%qu EOT rcvd\n",
	    (u_quad_t)tp_stat.ts_eot_input);
	printf("\t%qu EOT sent\n",
	    (u_quad_t)tp_stat.ts_EOT_sent);
	printf("\t%qu EOT indication%s\n",
	    (u_quad_t)tp_stat.ts_eot_user ,PLURAL(tp_stat.ts_eot_user));

	printf("Connections:\n");
	printf("\t%qu connection%s used extended format\n",
	    (u_quad_t)tp_stat.ts_xtd_fmt ,PLURAL(tp_stat.ts_xtd_fmt));
	printf("\t%qu connection%s allowed transport expedited data\n",
	    (u_quad_t)tp_stat.ts_use_txpd ,PLURAL(tp_stat.ts_use_txpd));
	printf("\t%qu connection%s turned off cksumming\n",
	    (u_quad_t)tp_stat.ts_csum_off ,PLURAL(tp_stat.ts_csum_off));
	printf("\t%qu connection%s dropped: retrans limit\n",
	    (u_quad_t)tp_stat.ts_conn_gaveup ,PLURAL(tp_stat.ts_conn_gaveup));
	printf("\t%qu tp 4 connection%s\n",
	    (u_quad_t)tp_stat.ts_tp4_conn ,PLURAL(tp_stat.ts_tp4_conn));
	printf("\t%qu tp 0 connection%s\n",
	    (u_quad_t)tp_stat.ts_tp0_conn ,PLURAL(tp_stat.ts_tp0_conn));
    {
		int j;
		static char *name[]= {
			"~LOCAL, PDN",
			"~LOCAL,~PDN",
			" LOCAL,~PDN",
			" LOCAL, PDN"
		};

		printf("\nRound trip times, listed in ticks:\n");
		printf("\t%11.11s  %12.12s | %12.12s | %s\n",
		    "Category",
		    "Smoothed avg", "Deviation", "Deviation/Avg");
		for (j = 0; j <= 3; j++) {
			printf("\t%11.11s: %-11d | %-11d | %-11d | %-11d\n",
			    name[j],
			    tp_stat.ts_rtt[j], tp_stat.ts_rtt[j],
			    tp_stat.ts_rtv[j], tp_stat.ts_rtv[j]);
		}
	}
	printf("\nTpdus RECVD [%qu valid, %3.6f %% of total (%qu); "
	    "%qu dropped]\n",
	    (u_quad_t)tp_stat.ts_tpdu_rcvd ,
	    ((tp_stat.ts_pkt_rcvd > 0) ?
	    ((100 * (float)tp_stat.ts_tpdu_rcvd)/(float)tp_stat.ts_pkt_rcvd) : 0),
            (u_quad_t)tp_stat.ts_pkt_rcvd,
	    (u_quad_t)tp_stat.ts_recv_drop);

	printf("\tDT  %6qu   AK  %6qu   DR  %4qu   CR  %4qu \n",
	    (u_quad_t)tp_stat.ts_DT_rcvd, (u_quad_t)tp_stat.ts_AK_rcvd,
	    (u_quad_t)tp_stat.ts_DR_rcvd, (u_quad_t)tp_stat.ts_CR_rcvd);
	printf("\tXPD %6qu   XAK %6qu   DC  %4qu   CC  %4qu   ER  %4qu\n",
	    (u_quad_t)tp_stat.ts_XPD_rcvd,
	    (u_quad_t)tp_stat.ts_XAK_rcvd,
	    (u_quad_t)tp_stat.ts_DC_rcvd,
	    (u_quad_t)tp_stat.ts_CC_rcvd,
	    (u_quad_t)tp_stat.ts_ER_rcvd);
	printf("\nTpdus SENT [%qu total, %qu dropped]\n",
	    (u_quad_t)tp_stat.ts_tpdu_sent, (u_quad_t)tp_stat.ts_send_drop);

	printf("\tDT  %6qu   AK  %6qu   DR  %4qu   CR  %4qu \n",
	    (u_quad_t)tp_stat.ts_DT_sent, (u_quad_t)tp_stat.ts_AK_sent,
	    (u_quad_t)tp_stat.ts_DR_sent, (u_quad_t)tp_stat.ts_CR_sent);
	printf("\tXPD %6qu   XAK %6qu   DC  %4qu   CC  %4qu   ER  %4qu\n",
	    (u_quad_t)tp_stat.ts_XPD_sent, (u_quad_t)tp_stat.ts_XAK_sent,
	    (u_quad_t)tp_stat.ts_DC_sent, (u_quad_t)tp_stat.ts_CC_sent,
	    (u_quad_t)tp_stat.ts_ER_sent);

	printf("\nRetransmissions:\n");
#define PERCENT(X,Y) (((Y)>0)?((100 *(float)(X)) / (float) (Y)):0)

	printf("\tCR  %6qu   CC  %6qu   DR  %6qu \n",
	    (u_quad_t)tp_stat.ts_retrans_cr,
	    (u_quad_t)tp_stat.ts_retrans_cc,
	    (u_quad_t)tp_stat.ts_retrans_dr);
	printf("\tDT  %6qu (%5.2f%%)\n",
	    (u_quad_t)tp_stat.ts_retrans_dt,
	    PERCENT(tp_stat.ts_retrans_dt, tp_stat.ts_DT_sent));
	printf("\tXPD %6qu (%5.2f%%)\n",
	    (u_quad_t)tp_stat.ts_retrans_xpd,
	    PERCENT(tp_stat.ts_retrans_xpd, tp_stat.ts_XPD_sent));


	printf("\nE Timers: [%6qu ticks]\n",
	    (u_quad_t)tp_stat.ts_Eticks);
	printf("%6qu timer%s set \t%6qu timer%s expired \t"
	    "%6qu timer%s cancelled\n",
	    (u_quad_t)tp_stat.ts_Eset ,PLURAL(tp_stat.ts_Eset),
	    (u_quad_t)tp_stat.ts_Eexpired ,PLURAL(tp_stat.ts_Eexpired),
	    (u_quad_t)tp_stat.ts_Ecan_act ,PLURAL(tp_stat.ts_Ecan_act));

	printf("\nC Timers: [%6qu ticks]\n",
	    (u_quad_t)tp_stat.ts_Cticks);
	printf("%6qu timer%s set \t%6qu timer%s expired \t"
	    "%6qu timer%s cancelled\n",
	    (u_quad_t)tp_stat.ts_Cset ,PLURAL(tp_stat.ts_Cset),
	    (u_quad_t)tp_stat.ts_Cexpired ,PLURAL(tp_stat.ts_Cexpired),
	    (u_quad_t)tp_stat.ts_Ccan_act ,PLURAL(tp_stat.ts_Ccan_act));
	printf("%6qu inactive timer%s cancelled\n",
	    (u_quad_t)tp_stat.ts_Ccan_inact ,PLURAL(tp_stat.ts_Ccan_inact));

	printf("\nPathological debugging activity:\n");
	printf("\t%6qu CC%s sent to zero dref\n",
	    (u_quad_t)tp_stat.ts_zdebug ,PLURAL(tp_stat.ts_zdebug));
	/* SAME LINE AS ABOVE */
	printf("\t%6qu random DT%s dropped\n",
	    (u_quad_t)tp_stat.ts_ydebug ,PLURAL(tp_stat.ts_ydebug));
	printf("\t%6qu illegally large XPD TPDU%s\n",
	    (u_quad_t)tp_stat.ts_vdebug ,PLURAL(tp_stat.ts_vdebug));
	printf("\t%6qu faked reneging of cdt\n",
	    (u_quad_t)tp_stat.ts_ldebug);

	printf("\nACK reasons:\n");
	printf("\t%6qu not acked immediately\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_DONT_]);
	printf("\t%6qu strategy==each\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_STRAT_EACH_]);
	printf("\t%6qu strategy==fullwindow\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_STRAT_FULLWIN_]);
	printf("\t%6qu dup DT\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_DUP_]);
	printf("\t%6qu EOTSDU\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_EOT_]);
	printf("\t%6qu reordered DT\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_REORDER_]);
	printf("\t%6qu user rcvd\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_USRRCV_]);
	printf("\t%6qu fcc reqd\n",
	    (u_quad_t)tp_stat.ts_ackreason[_ACK_FCC_]);
}
#ifndef SSEL
#define SSEL(s) ((s)->siso_tlen + TSEL(s))
#define PSEL(s) ((s)->siso_slen + SSEL(s))
#endif

static void
isonetprint(struct sockaddr_iso *siso, int islocal)
{
	hexprint(siso->siso_nlen, siso->siso_addr.isoa_genaddr, "{}");
	if (siso->siso_tlen || siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_tlen, TSEL(siso), "()");
	if (siso->siso_slen || siso->siso_plen)
		hexprint(siso->siso_slen, SSEL(siso), "[]");
	if (siso->siso_plen)
		hexprint(siso->siso_plen, PSEL(siso), "<>");
	putchar(' ');
}

static char hexlist[] = "0123456789abcdef", obuf[128];

static void
hexprint(int n, char *buf, char *delim)
{
	u_char *in = (u_char *)buf, *top = in + n;
	char *out = obuf;
	int i;

	if (n == 0)
		return;
	while (in < top) {
		i = *in++;
		*out++ = '.';
		if (i > 0xf) {
			out[1] = hexlist[i & 0xf];
			i >>= 4;
			out[0] = hexlist[i];
			out += 2;
		} else
			*out++ = hexlist[i];
	}
	*obuf = *delim; *out++ = delim[1]; *out = 0;
	printf("%s", obuf);
}
