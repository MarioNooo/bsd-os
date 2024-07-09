/*	BSDI ns.c,v 2.7 1997/11/25 03:49:39 jch Exp	*/
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
static char sccsid[] = "@(#)ns.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/tcp_fsm.h>

#include <netns/ns.h>
#include <netns/ns_pcb.h>
#include <netns/idp.h>
#include <netns/idp_var.h>
#include <netns/ns_error.h>
#include <netns/sp.h>
#include <netns/spidp.h>
#include <netns/spp_timer.h>
#include <netns/spp_var.h>
#define SANAMES
#include <netns/spp_debug.h>

#include <nlist.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "netstat.h"

struct	nspcb nspcb;
struct	sppcb sppcb;
struct	socket sockb;

static char *ns_prpr __P((struct ns_addr *));
static void ns_erputil __P((int, int));

static	int first = 1;

struct data_info idp_info = {
	"_nspcb",
	NULL, 0,
	NULL, 0
};

/*
 * Print a summary of connections related to a Network Systems
 * protocol.  For SPP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -v (all) flag is specified.
 */

void
nsprotopr(char *name, struct data_info *dip)
{
	struct nspcb cb;
	struct nspcb *prev, *next;
	int isspp;

	dip->di_ptr = &cb;
	dip->di_size = sizeof(cb);
	if (skread(name, dip))
		return;

	isspp = strcmp(name, "spp") == 0;

	nspcb = cb;
	prev = (struct nspcb *)dip->di_off;
	if (nspcb.nsp_next == (struct nspcb *)dip->di_off)
		return;
	for ( ; nspcb.nsp_next != (struct nspcb *)dip->di_off; prev = next) {
		u_long ppcb;

		next = nspcb.nsp_next;
		if (kread((u_long)next, &nspcb, sizeof (nspcb)))
			return;
		if (nspcb.nsp_prev != prev) {
			printf("???\n");
			break;
		}
		if (!aflag && ns_nullhost(nspcb.nsp_faddr)) {
			continue;
		}
		if (kread((u_long)nspcb.nsp_socket, &sockb, sizeof (sockb)))
			return;
		ppcb = (u_long) nspcb.nsp_pcb;
		if (ppcb) {
			if (isspp &&
			    kread(ppcb, &sppcb, sizeof (sppcb)))
				return;
			else
				continue;
		} else if (isspp) 
			continue;
		if (first) {
			printf("Active NS connections");
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
			printf("%08lx ", ppcb);
		printf("%-5.5s %6lu %6lu ", name, sockb.so_rcv.sb_cc,
			sockb.so_snd.sb_cc);
		printf("  %-22.22s", ns_prpr(&nspcb.nsp_laddr));
		printf(" %-22.22s", ns_prpr(&nspcb.nsp_faddr));
		if (isspp) {
			if (sppcb.s_state >= TCP_NSTATES)
				printf(" %d", sppcb.s_state);
			else
				printf(" %s", tcpstates[sppcb.s_state]);
		}
		putchar('\n');
		prev = next;
	}
}
#define ANY(x,y,z) \
	((x) != 0 || vflag ? printf("\t%qu %s%s%s -- %s\n",(u_quad_t)x,y,PLURAL(x),z,#x) : 0)

/*
 * Dump SPP statistics structure.
 */

static struct spp_istat spp_istat;
static struct data_info spp_istat_info = {
	"_spp_istat",
	NULL, 0,
	&spp_istat, sizeof(spp_istat)
};

void
spp_stats(char *name)
{
#define sppstat spp_istat.newstats

	if (skread(name, &spp_istat_info))
		return;

	printf("%s:\n", name);

	ANY(spp_istat.nonucn, "connection", " dropped: no new sockets ");
	ANY(spp_istat.gonawy, "connection",
	    " terminated: our end dying");
	ANY(spp_istat.nonucn, "connection",
	    " dropped: inability to connect");
	ANY(spp_istat.noconn, "connection",
	    " dropped: inability to connect");
	ANY(spp_istat.notme, "connection",
	    " incompleted: mismatched id's");
	ANY(spp_istat.wrncon, "connection", " dropped: mismatched id's");
	ANY(spp_istat.bdreas, "pkt", " dropped out of sequence");
	ANY(spp_istat.lstdup, "pkt", " duplicating the highest pkt");
	ANY(spp_istat.notyet, "pkt", " refused as exceeding allocation");
	ANY(sppstat.spps_connattempt, "connection", " initiated");
	ANY(sppstat.spps_accepts, "connection", " accepted");
	ANY(sppstat.spps_connects, "connection", " established");
	ANY(sppstat.spps_drops, "connection", " dropped");
	ANY(sppstat.spps_conndrops, "embryonic connection", " dropped");
	ANY(sppstat.spps_closed, "connection", " closed (includes drops)");
	ANY(sppstat.spps_segstimed, "pkt", " where we tried to get rtt");
	ANY(sppstat.spps_rttupdated, "time", " we got rtt");
	ANY(sppstat.spps_delack, "delayed ack", " sent");
	ANY(sppstat.spps_timeoutdrop, "connection",
	    " dropped in rxmt timeout");
	ANY(sppstat.spps_rexmttimeo, "rexmit timeout", "");
	ANY(sppstat.spps_persisttimeo, "persist timeout", "");
	ANY(sppstat.spps_keeptimeo, "keepalive timeout", "");
	ANY(sppstat.spps_keepprobe, "keepalive probe", " sent");
	ANY(sppstat.spps_keepdrops, "connection", " dropped in keepalive");
	ANY(sppstat.spps_sndtotal, "total pkt", " sent");
	ANY(sppstat.spps_sndpack, "data pkt", " sent");
	ANY(sppstat.spps_sndbyte, "data byte", " sent");
	ANY(sppstat.spps_sndrexmitpack, "data pkt", " rexmitted");
	ANY(sppstat.spps_sndrexmitbyte, "data byte", " rexmitted");
	ANY(sppstat.spps_sndacks, "ack-only pkt", " sent");
	ANY(sppstat.spps_sndprobe, "window probe", " sent");
	ANY(sppstat.spps_sndurg, "pkt", " sent w/URG only");
	ANY(sppstat.spps_sndwinup, "window update-only pkt", " sent");
	ANY(sppstat.spps_sndctrl, "control (SYN|FIN|RST) pkt", " sent");
	ANY(sppstat.spps_sndvoid, "request", " to send a non-existant pkt");
	ANY(sppstat.spps_rcvtotal, "total pkt", " rcvd");
	ANY(sppstat.spps_rcvpack, "pkt", " rcvd in sequence");
	ANY(sppstat.spps_rcvbyte, "byte", " rcvd in sequence");
	ANY(sppstat.spps_rcvbadsum, "pkt", " rcvd w/ccksum errs");
	ANY(sppstat.spps_rcvbadoff, "pkt", " rcvd w/bad offset");
	ANY(sppstat.spps_rcvshort, "pkt", " rcvd too short");
	ANY(sppstat.spps_rcvduppack, "dup-only pkt", " rcvd");
	ANY(sppstat.spps_rcvdupbyte, "dup-only byte", " rcvd");
	ANY(sppstat.spps_rcvpartduppack, "pkt",
	    " w/some dup data");
	ANY(sppstat.spps_rcvpartdupbyte, "dup byte", " in part-dup. pkt");
	ANY(sppstat.spps_rcvoopack, "out-of-order pkt", " rcvd");
	ANY(sppstat.spps_rcvoobyte, "out-of-order byte", " rcvd");
	ANY(sppstat.spps_rcvpackafterwin, "pkt", " w/data after window");
	ANY(sppstat.spps_rcvbyteafterwin, "byte", " rcvd after window");
	ANY(sppstat.spps_rcvafterclose, "pkt", " rcvd after 'close'");
	ANY(sppstat.spps_rcvwinprobe, "rcvd window probe pkt", "");
	ANY(sppstat.spps_rcvdupack, "rcvd dup ack", "");
	ANY(sppstat.spps_rcvacktoomuch, "rcvd ack", " for unsent data");
	ANY(sppstat.spps_rcvackpack, "rcvd ack pkt", "");
	ANY(sppstat.spps_rcvackbyte, "byte", " acked by rcvd acks");
	ANY(sppstat.spps_rcvwinupd, "rcvd window update pkt", "");
}
#undef ANY
#define ANY(x,y,z)  ((x) != 0 || vflag ? printf("\t%d %s%s%s\n",x,y,PLURAL(x),z) : 0)

/*
 * Dump IDP statistics structure.
 */

static struct idpstat idpstat;
static struct data_info idpstat_info = {
	"_idpstat",
	NULL, 0,
	&idpstat, sizeof(idpstat)
};

void
idp_stats(char *name)
{

	if (skread(name, &idpstat_info))
		return;

	printf("%s:\n", name);

	ANY(idpstat.idps_toosmall, "pkt", " smaller than a header");
	ANY(idpstat.idps_tooshort, "pkt", " smaller than advertised");
	ANY(idpstat.idps_badsum, "pkt", " w/bad cksums");
}

static	struct {
	u_short code;
	char *name;
	char *where;
} ns_errnames[] = {
	{0, "Unspecified Err", " at Destination"},
	{1, "Bad Cksum", " at Destination"},
	{2, "No Listener", " at Socket"},
	{3, "Pkt", " Refused: lack of space at Destination"},
	{01000, "Unspecified Err", " while gatewayed"},
	{01001, "Bad Cksum", " while gatewayed"},
	{01002, "Pkt", " forwarded too many times"},
	{01003, "Pkt", " too large to be forwarded"},
	{-1, 0, 0},
};

/*
 * Dump NS Error statistics structure.
 */

static struct ns_errstat ns_errstat;
static struct data_info ns_errstat_info = {
	"_ns_errstat",
	NULL, 0,
	&ns_errstat, sizeof(ns_errstat)
};

/*ARGSUSED*/
void
nserr_stats(char *name)
{
	int j;
	int histoprint = 1;
	int z;

	if (skread(name, &ns_errstat_info))
		return;

	printf("NS err statistics:\n");

	ANY(ns_errstat.ns_es_error, "call", " to ns_error");
	ANY(ns_errstat.ns_es_oldshort, "err",
		" ignored: insufficient addressing");
	ANY(ns_errstat.ns_es_oldns_err, "err request",
		" in response to err pkts");
	ANY(ns_errstat.ns_es_tooshort, "err pkt",
		" rcvd incomplete");
	ANY(ns_errstat.ns_es_badcode, "err pkt",
		" rcvd of unknown type");
	for (j = 0; j < NS_ERR_MAX; j ++) {
		z = ns_errstat.ns_es_outhist[j];
		if (z && histoprint) {
			printf("Output Err Histogram:\n");
			histoprint = 0;
		}
		ns_erputil(z, ns_errstat.ns_es_codes[j]);

	}
	histoprint = 1;
	for (j = 0; j < NS_ERR_MAX; j ++) {
		z = ns_errstat.ns_es_inhist[j];
		if (z && histoprint) {
			printf("Input Err Histogram:\n");
			histoprint = 0;
		}
		ns_erputil(z, ns_errstat.ns_es_codes[j]);
	}
}

static void
ns_erputil(int z, int c)
{
	int j;
	char codebuf[30];
	char *name, *where;

	for (j = 0;; j ++) {
		if ((name = ns_errnames[j].name) == NULL)
			break;
		if (ns_errnames[j].code == c)
			break;
	}
	if (name == NULL)  {
		if (c > 01000)
			where = "in transit";
		else
			where = "at destination";
		(void)sprintf(codebuf, "Unknown XNS/IPX err code 0%o", c);
		name = codebuf;
	} else
		where =  ns_errnames[j].where;
	ANY(z, name, where);
}

static struct sockaddr_ns ssns = {AF_NS};

static
char *ns_prpr(struct ns_addr *x)
{
	struct sockaddr_ns *sns = &ssns;

	sns->sns_addr = *x;
	return(ns_print((struct sockaddr *)sns));
}
