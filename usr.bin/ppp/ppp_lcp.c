/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_lcp.c,v 2.33 2001/10/03 17:29:57 polk Exp
 */

/*
 * This code is partially derived from CMU PPP.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * PPP Link Control Protocol
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_pif.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

#define	PPP_LOOP_MAX	10	/* # times dup packets -> link looped */
int ppp_loop_max = PPP_LOOP_MAX;

void
ppp_lcp_init(ppp)
	ppp_t *ppp;
{
	ppp_cp_up(ppp, FSM_LCP);
}

int
ppp_lcp_isup(ppp)
	ppp_t *ppp;
{
	return (ppp->ppp_lcp.fsm_state == CP_OPENED ? 1 : 0);
}

/*
 * Turn upper levels Up
 */
void
ppp_lcp_tlu(ppp)
	ppp_t *ppp;
{
	/* Load the async control characters map */
	ppp->ppp_txcmap = ppp->ppp_tcmap;
	ppp->ppp_rxcmap = ppp->ppp_rcmap;
	ppp->ppp_flags |= PPP_RUNNING;
	ppp_setparam(ppp, -1);
	/*
	 * Currently only PAP and CHAP are available and if both
	 * are still marked needed then we must have negotiated
	 * CHAP.  Turn of PAP since we only allow one of the two.
	 */
	if (ppp->ppp_flags & PPP_NEEDCHAP)
		ppp->ppp_flags &= ~PPP_NEEDPAP;
	dprintf(D_STATE, "LCP is up\n");
}

/*
 * Turn upper levels Down
 */
void
ppp_lcp_tld(ppp)
	ppp_t *ppp;
{
	ppp_cp_down(ppp, FSM_IPCP);
	ppp_ml_down(ppp);
}

/*
 * Timeout
 */
void
ppp_lcp_timeout(ppp)
	void *ppp;
{
	ppp_cp_timeout((ppp_t *)ppp, FSM_LCP);
}

/*
 * Handle the extra LCP packet codes
 */
void
ppp_lcp_xcode(ppp, m, cp)
	ppp_t *ppp;
	pbuf_t *m;
	ppp_cp_t *cp;
{
	u_short type;
	u_long magic;

	switch (cp->cp_code) {
	default:
		break;

	case LCP_PROTO_REJ:     /* Protocol-Reject */
		type = ntohs(*(u_short *)(m->data));
		switch (type) {
		case PPP_IP:
		case PPP_IPCP:
			dprintf(D_DT, "	IP disabled\n");
			ppp_cp_close(ppp, FSM_IPCP);
			break;
		case PPP_IPV6CP:
			dprintf(D_DT, "	IPv6CP disabled\n");
			ppp_cp_close(ppp, FSM_IPV6CP);
			break;
		default:
			dprintf(D_DT, "	<0x%x> -- dropped\n", type);
			break;
		}
		break;

	case LCP_ECHO_REQ:      /* Echo-Request */
		magic = ntohl(*(u_long *)(m->data));
		if (magic && magic != ppp->ppp_remotemagic)
			dprintf(D_DT, "	bad magic=%x\n	send Echo-Reply\n",
			    magic);
		else
			dprintf(D_DT, "	magic=%x\n	send Echo-Reply\n",
			    magic);
		*(u_long *)(m->data) = htonl(ppp->ppp_magic);
		m = ppp_cp_prepend(m, PPP_LCP, LCP_ECHO_REPL, cp->cp_id, 0);
		sendpacket(ppp, m);
		break;

	case LCP_ECHO_REPL:     /* Echo-Reply */
		magic = ntohl(*(u_long *)(m->data));
		if (magic && magic != ppp->ppp_remotemagic)
			dprintf(D_DT, "	bad magic=%x\n	send Echo-Reply\n",
			    magic);
		else
			dprintf(D_DT, "	magic=%x\n	send Echo-Reply\n",
			    magic);
		ppp_echorepl(ppp, m, cp);
		break;
	case LCP_DISC_REQ:      /* Discard-Request */
		dprintf(D_DT, "	magic=%x\n	dropped\n",
		    *(u_long *)(m->data));
		break;
	}
}

/*
 * Compose a LCP Config-Request packet
 */
void
ppp_lcp_creq(ppp)
	ppp_t *ppp;
{
	pbuf_t *m;
	pppopt_t opt;

	dprintf(D_PKT, "BUILD LCP_CREQ:\n");
	m = 0;

	/* Magic-Number */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_MAGIC))) {
		dprintf(D_DT, "	{MAGIC: 0x%x}\n", ppp->ppp_magic);
		opt.po_type = LCP_MAGIC;
		opt.po_len = 6;
		opt.po_data.l = htonl(ppp->ppp_magic);
		m = ppp_addopt(m, &opt);
	}

	/* Max-Receive-Unit */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_MRU))) {
		dprintf(D_DT, "	{MRU: %d}\n", ppp->ppp_mru);
		opt.po_type = LCP_MRU;
		opt.po_len = 4;
		opt.po_data.s = htons(ppp->ppp_mru);
		m = ppp_addopt(m, &opt);
	}

	/* Asynchronous-Control-Characters-Map */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_ACCM))) {
		dprintf(D_DT, "	{ACCM: %x}\n", ppp->ppp_rcmap);
		opt.po_type = LCP_ACCM;
		opt.po_len = 6;
		opt.po_data.l = htonl(ppp->ppp_rcmap);
		m = ppp_addopt(m, &opt);
	}

	/* Protocol-Field-Compression */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_PFC)) &&
	    (ppp->ppp_flags & PPP_PFC)) {
		dprintf(D_DT, "	{PFC}\n");
		opt.po_type = LCP_PFC;
		opt.po_len = 2;
		m = ppp_addopt(m, &opt);
	}

	/* Address-and-Control-Field-Compression */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_ACFC)) &&
	    (ppp->ppp_flags & PPP_ACFC)) {
		dprintf(D_DT, "	{ACFC}\n");
		opt.po_type = LCP_ACFC;
		opt.po_len = 2;
		m = ppp_addopt(m, &opt);
	}

	/*
	 * Always try for just CHAP.  If they don't like CHAP
	 * we will turn off CHAP and try again with PAP
	 */
	if (ppp->ppp_flags & PPP_NEEDCHAP) {
		dprintf(D_DT, "	{CHAP}\n");
		dprintf(D_DT, "	{MODE %d}\n", ppp->ppp_chapmode);
		opt.po_type = LCP_AP;
		opt.po_len = 5;
		opt.po_data.s = htons(PPP_CHAP);
		opt.po_data.c[2] = ppp->ppp_chapmode;
		m = ppp_addopt(m, &opt);
	} else if (ppp->ppp_flags & PPP_NEEDPAP) {
		dprintf(D_DT, "	{PAP}\n");
		opt.po_type = LCP_AP;
		opt.po_len = 4;
		opt.po_data.s = htons(PPP_PAP);
		m = ppp_addopt(m, &opt);
	}

	if ((ppp->ppp_mrru <= 0) && (ppp->ppp_flags & PPP_USEML))
		ppp->ppp_mrru = LCP_DFLT_MRRU;

	/* Max-Receive-Reconstructed-Unit */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_MRRU)) &&
	    ppp->ppp_mrru > 0) {
		dprintf(D_DT, "	{MRRU: %d}\n", ppp->ppp_mrru);
		opt.po_type = LCP_MRRU;
		opt.po_len = 4;
		opt.po_data.s = htons(ppp->ppp_mrru);
		m = ppp_addopt(m, &opt);

		/*
		 * Short Sequence Numbers
		 * Only allowed when MRRU is sent - see RFC 1990
		 */
		if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_SSEQ)) &&
		    (ppp->ppp_flags & PPP_RCV_SSEQ)) {
			dprintf(D_DT, "\t{SSEQ}\n");
			opt.po_type = LCP_SSEQ;
			opt.po_len = 2;
			m = ppp_addopt(m, &opt);
		}
	}


	/* Endpoint Discriminator */
	if (!(ppp->ppp_lcp.fsm_rej & (1<<LCP_ED)) &&
	    (ppp->ppp_snd_ed.ed_type >= 0)) {
		int i;

		switch (ppp->ppp_snd_ed.ed_type) {
		case ED_NULL:
			dprintf(D_DT, "\t{ED NULL}\n");
			break;
	
		case ED_LOCAL:
			dprintf(D_DT, "\t{ED LOCAL");		/*}*/
			goto print_bytes;
	
		case ED_IP:
			dprintf(D_DT, "\t{ED IP %d.%d.%d.%d}\n",
			    ppp->ppp_snd_ed.ed[0], ppp->ppp_snd_ed.ed[1],
			    ppp->ppp_snd_ed.ed[2], ppp->ppp_snd_ed.ed[3]);
			break;
	
		case ED_802_1:
			dprintf(D_DT, "\t{ED 802.1 %x:%x:%x:%x:%x:%x}\n",
			    ppp->ppp_snd_ed.ed[0], ppp->ppp_snd_ed.ed[1],
			    ppp->ppp_snd_ed.ed[2], ppp->ppp_snd_ed.ed[3],
			    ppp->ppp_snd_ed.ed[4], ppp->ppp_snd_ed.ed[5]);
			break;
	
		case ED_PPP_MNB:		/* depricated */
			dprintf(D_DT, "\t{ED MNB");
			for (i = 0; i < ppp->ppp_snd_ed.ed_len; i += 4)
				dprintf(D_DT, " %02x%02x%02x%02x",
				    ppp->ppp_snd_ed.ed[i+0],
				    ppp->ppp_snd_ed.ed[i+1],
				    ppp->ppp_snd_ed.ed[i+2],
				    ppp->ppp_snd_ed.ed[i+3]);
			dprintf(D_DT, "}\n");
			break;
	
		case ED_PSND:
			dprintf(D_DT, "\t{ED PSND");		/*}*/
			goto print_bytes;
	
		default:
			dprintf(D_DT, "\t{ED #%d", ppp->ppp_snd_ed.ed_type);
		print_bytes:
			for (i = 0; i < ppp->ppp_snd_ed.ed_len; i++)
				dprintf(D_DT, " %02x", ppp->ppp_snd_ed.ed[i]);
			dprintf(D_DT, "}\n");
			break;
		}
		opt.po_type = LCP_ED;
		opt.po_len = 3 + ppp->ppp_snd_ed.ed_len;
		opt.po_data.c[0] = ppp->ppp_snd_ed.ed_type;
		memcpy(&opt.po_data.c[1], ppp->ppp_snd_ed.ed, ppp->ppp_snd_ed.ed_len);
		m = ppp_addopt(m, &opt);
	}

	dprintf(D_DT, "	id=%d len=%d\n", ppp->ppp_lcp.fsm_id + 1, m ? m->len:0);
	m = ppp_cp_prepend(m, PPP_LCP, CP_CONF_REQ, ++(ppp->ppp_lcp.fsm_id), 0);
	sendpacket(ppp, m);
}

/*
 * Compose an LCP Config-Nak packet.  This is a one-time
 * operation, done up front to ask the other side to include
 * these options in its Config-Request packet.  The other
 * side is free to ignore them.
 */

void
ppp_lcp_cnak(ppp)
	ppp_t *ppp;
{
	pbuf_t *m;
	pppopt_t opt;

	dprintf(D_PKT, "BUILD LCP_CNAK:\n");
	m = 0;

	/* Short Sequence Numbers */
	if (ppp->ppp_flags & PPP_SND_SSEQ) {
		dprintf(D_DT, "\t{SSEQ}\n");
		opt.po_type = LCP_SSEQ;
		opt.po_len = 2;
		m = ppp_addopt(m, &opt);
	}

	if (m == NULL) {
		dprintf(D_DT, "\t<no options, nothing sent>\n");
		return;
	}
	dprintf(D_DT, "	id=%d len=%d\n", ppp->ppp_lcp.fsm_id + 1, m ? m->len:0);
	m = ppp_cp_prepend(m, PPP_LCP, CP_CONF_NAK, ++(ppp->ppp_lcp.fsm_id), 0);
	sendpacket(ppp, m);
}

/*
 * Process the rejected option
 */
void
ppp_lcp_optrej(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	switch (opt->po_type) {
	case LCP_MRU:
		dprintf(D_DT, "	{MRU: len=%d mtu=%d (use 1500)}\n",
			opt->po_len, ntohs(opt->po_data.s));
		/* use the RFC default value */
		ppp->ppp_mru = PPPMTU;
		break;

	case LCP_PFC:
		dprintf(D_DT, "	{PFC}\n");
		/* indicate that peer won't send compressed */
		ppp->ppp_flags &= ~PPP_PFC;
		break;

	case LCP_ACFC:
		dprintf(D_DT, "	{ACFC}\n");
		/* indicate that peer won't send compressed */
		ppp->ppp_flags &= ~PPP_ACFC;
		break;

	case LCP_AP:
		switch (ntohs(opt->po_data.s)) {
		case PPP_PAP:
			dprintf(D_DT, "	{AP: PAP}\n");
			if ((ppp->ppp_flags & PPP_NEEDAUTH) == PPP_NEEDPAP)
				ppp_shutdown(ppp);
			ppp->ppp_flags &= ~PPP_NEEDPAP;
			break;
		case PPP_CHAP:
			dprintf(D_DT, "	{AP: CHAP}\n");
			if ((ppp->ppp_flags & PPP_NEEDAUTH) == PPP_NEEDCHAP)
				ppp_shutdown(ppp);
			ppp->ppp_flags &= ~PPP_NEEDCHAP;
			break;
		default:
			dprintf(D_DT, "	{AP: proto=%04x}\n",
			    ntohs(opt->po_data.s));
			break;
		}
		break;

	case LCP_MRRU:
		dprintf(D_DT, "	{MRRU: len=%d mtru=%d}\n",
			opt->po_len, ntohs(opt->po_data.s));
		ppp->ppp_mrru = -1;
		break;

	case LCP_SSEQ:
		dprintf(D_DT, "\t{SSEQ}\n");
		ppp->ppp_flags &= ~PPP_RCV_SSEQ;
		break;

	case LCP_ED:
		dprintf(D_DT, "\t{ED...}\n");
		break;

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the NAK-ed option
 */
void
ppp_lcp_optnak(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	int need;

	switch (opt->po_type) {
	case LCP_MRU:
		dprintf(D_DT, "	{MRU: len=%d mtu=%d}\n",
			opt->po_len, ntohs(opt->po_data.s));
		/* agree on what the peer says */
		ppp->ppp_mru = ntohs(opt->po_data.s);
		break;

	case LCP_ACCM:
		dprintf(D_DT, "	{ACCM: len=%d m=0x%x}\n",
			opt->po_len, ntohl(opt->po_data.l));
		/*
		 * Agree with the peer, for the most part.
		 * We only add the bits the peer requests as
		 * we assume we set our bits for a reason and
		 * do not expect them to be unset.
		 */
		ppp->ppp_rcmap |= ntohl(opt->po_data.l);
		break;

	case LCP_MAGIC:
		dprintf(D_DT, "	{MAGIC: len=%d m=0x%x}\n",
			opt->po_len, ntohl(opt->po_data.l));
		if (ppp->ppp_magic == ntohl(opt->po_data.l))
			ppp->ppp_magic = ppp_lcp_magic();
		break;

	case LCP_AP:
		switch (ntohs(opt->po_data.s)) {
		case PPP_PAP:
			dprintf(D_DT, "	{AP: PAP}\n");
			break;
		case PPP_CHAP:
			dprintf(D_DT, "	{AP: CHAP}\n");
			break;
		default:
			dprintf(D_DT, "	{AP: proto=%04x}\n",
			    ntohs(opt->po_data.s));
			break;
		}
		/*
		 * Keep track of what authentication we needed
		 * then disable the authentication that we last
		 * requested.  If we disable the last authentication
		 * then shutdown the connection.
		 * We really should try to switch to what the other side
		 * asked for, but it is so much easier to just cycle through
		 * our options.
		 */
		need = ppp->ppp_flags & PPP_NEEDAUTH;

		/*
		 * Turn off the bit that got them to complain
		 */
		if (ppp->ppp_flags & PPP_NEEDCHAP)
			ppp->ppp_flags &= ~PPP_NEEDCHAP;
		else if (ppp->ppp_flags & PPP_NEEDPAP)
			ppp->ppp_flags &= ~PPP_NEEDPAP;

		if (need && (ppp->ppp_flags & PPP_NEEDAUTH) == 0)
			ppp_shutdown(ppp);
		break;

	case LCP_MRRU:
		dprintf(D_DT, "	{MRRU: len=%d mtu=%d}\n",
		    opt->po_len, ntohs(opt->po_data.s));
		/* agree on what the peer says */
		if (ppp->ppp_mrru > 0 || (ppp->ppp_flags & PPP_SND_SSEQ))
			ppp->ppp_mrru = ntohs(opt->po_data.s);
		else
			dprintf(D_DT, "	{NAKed MRRU IGNORED}\n");
		break;

	case LCP_ED:
		dprintf(D_DT, "\t{ED...}\n");
		break;

	case LCP_SSEQ:
		dprintf(D_DT, "\t{SSEQ}\n");
		ppp->ppp_flags &= ~PPP_RCV_SSEQ;
		break;

	default:
		dprintf(D_DT, "	{%d: len=%d}\n", opt->po_type, opt->po_len);
	}
}

/*
 * Process the received option
 */
int
ppp_lcp_opt(ppp, opt)
	ppp_t *ppp;
	pppopt_t *opt;
{
	u_long mask;
	int i;

	switch (opt->po_type) {
	case LCP_MRU:   /* Max-Receive-Unit */
		dprintf(D_DT, "	{MRU: len=%d mtu=%d}\n",
			opt->po_len, ntohs(opt->po_data.s));
		if (opt->po_len != 4)
			return (OPT_REJ);

		ppp->ppp_mtu = ntohs(opt->po_data.s);
		ppp_setparam(ppp, PPP_SETMTU);	/* XXX - Even on small mtu? */
		if (ppp->ppp_mtu < LCP_MIN_MRU) {
			opt->po_data.s = htons(LCP_MIN_MRU);
			return (OPT_NAK);
		}
		return (OPT_OK);

	case LCP_ACCM:
		dprintf(D_DT, "	{ACCM: len=%d m=0x%x}\n",
			opt->po_len, ntohl(opt->po_data.l));
		if (opt->po_len != 6)
			return (OPT_REJ);
		mask = ntohl(opt->po_data.l);
		if (~mask & ppp->ppp_tcmap) {
			opt->po_data.l = htonl(mask | ppp->ppp_tcmap);
			return (OPT_NAK);
		}
		ppp->ppp_tcmap = mask;
		return (OPT_OK);

	case LCP_AP:
		switch (ntohs(opt->po_data.s)) {
		case PPP_PAP:
			dprintf(D_DT, "	{AP: PAP}\n");
			if (opt->po_len != 4)
				return (OPT_REJ);
			if ((ppp->ppp_flags & PPP_CANPAP) == 0)
				break;
			ppp->ppp_flags |= PPP_WANTPAP;
			return (OPT_OK);
		case PPP_CHAP:
			dprintf(D_DT, "	{AP: CHAP}\n");
			if (opt->po_len != 5)
				return (OPT_REJ);
			dprintf(D_DT, "	{CHAP mode=%d}\n", opt->po_data.c[2]);
			if ((ppp->ppp_flags & PPP_CANCHAP) == 0)
				break;
			if (ppp->ppp_chapmode != opt->po_data.c[2]) {
				opt->po_data.c[2] = ppp->ppp_chapmode;
				return (OPT_NAK);
			}
			ppp->ppp_flags |= PPP_WANTCHAP;
			return (OPT_OK);
		default:
			dprintf(D_DT, "	{AP: proto=%04x}\n",
			    ntohs(opt->po_data.s));
			break;
		}

		/*
		 * If the last 3 protocols we NAK/REJ'd are this protocol,
		 * just reject it.  Microsoft NT plays stupid loop games
		 * thinking that maybe we will stop NAKing and actually
		 * accept its broken parameters.
		 */
		if (ppp->ppp_rejectauth[0] == opt->po_data.s &&
		    ppp->ppp_rejectauth[1] == ppp->ppp_rejectauth[0] &&
		    ppp->ppp_rejectauth[2] == ppp->ppp_rejectauth[1])
			return (OPT_REJ);

		ppp->ppp_rejectauth[2] = ppp->ppp_rejectauth[1];
		ppp->ppp_rejectauth[1] = ppp->ppp_rejectauth[0];
		ppp->ppp_rejectauth[0] = opt->po_data.s;

		if (ppp->ppp_flags & PPP_CANCHAP) {
			opt->po_len = 5;
			opt->po_data.s = htons(PPP_CHAP);
			opt->po_data.c[2] = ppp->ppp_chapmode;
			return (OPT_NAK);
		}
		if (ppp->ppp_flags & PPP_CANPAP) {
			opt->po_len = 4;
			opt->po_data.s = htons(PPP_PAP);
			return (OPT_NAK);
		}
		return (OPT_REJ);

	case LCP_QP:
		dprintf(D_DT, "	{QP}\n");
		/* currently not supported */
		return (OPT_REJ);

	case LCP_MAGIC:
		dprintf(D_DT, "	{MAGIC: len=%d m=0x%x}\n",
			opt->po_len, ntohl(opt->po_data.l));
		if (opt->po_len != 6)
			return (OPT_REJ);
		mask = ntohl(opt->po_data.l);
		if (ppp->ppp_magic == mask && mask != 0) {
			if (ppp->ppp_badmagic++ >= ppp_loop_max) {
				/* say it only once */
				if (ppp->ppp_badmagic == ppp_loop_max+1)
					dprintf(D_DT, "	link looped\n");
				return (OPT_FATAL);
			}
			ppp->ppp_magic = ppp_lcp_magic();
			opt->po_data.l = htonl(ppp->ppp_magic);
			return (OPT_NAK);
		}
		ppp->ppp_remotemagic = mask;
		return (OPT_OK);

	case LCP_PFC:
		dprintf(D_DT, "	{PFC: len=%d}\n", opt->po_len);
		if (opt->po_len != 2)
			return (OPT_REJ);
		if (!(ppp->ppp_flags & PPP_PFC))
			return (OPT_REJ);
		ppp->ppp_txflags |= PPP_PFC;
		return (OPT_OK);

	case LCP_ACFC:
		dprintf(D_DT, "	{ACFC: len=%d}\n", opt->po_len);
		if (opt->po_len != 2)
			return (OPT_REJ);
		if (!(ppp->ppp_flags & PPP_ACFC))
			return (OPT_REJ);
		ppp->ppp_txflags |= PPP_ACFC;
		return (OPT_OK);

	case LCP_MRRU:	/* Max-Receive-Reconstructed-Unit */
		dprintf(D_DT, "\t{MRRU: len=%d mtu=%d}\n",
			opt->po_len, ntohs(opt->po_data.s));
		if (opt->po_len != 4)
			return (OPT_REJ);
		ppp->ppp_mtru = ntohs(opt->po_data.s);
		/* Reject it if we aren't doing multilink */
		if (ppp->ppp_mrru <= 0 && !(ppp->ppp_flags & PPP_SND_SSEQ))
			return (OPT_REJ);
		if (ppp->ppp_mtru < LCP_MIN_MRU) {
			opt->po_data.s = htons(LCP_MIN_MRU);
			return (OPT_NAK);
		}
		ppp->ppp_flags |= PPP_ISML;
		return (OPT_OK);
	
	case LCP_SSEQ:	/* Short Sequence Number Header Format */
		dprintf(D_DT, "\t{SSEQ}\n");
		if (opt->po_len != 2)
			return (OPT_REJ);
		if (ppp->ppp_mtru <= 0)
			ppp->ppp_mtru = LCP_DFLT_MRRU;
		/* Reject it if we aren't don't want short sequence numbers */
		if (!(ppp->ppp_flags & PPP_SND_SSEQ))
			return (OPT_REJ);
		ppp->ppp_txflags |= PPP_SND_SSEQ;
		ppp->ppp_flags |= PPP_ISML;
		return (OPT_OK);
	
	case LCP_ED:	/* Endpoint Discriminator */
		switch (opt->po_data.c[0]) {
		case ED_NULL:
			dprintf(D_DT, "\t{ED NULL}\n");
			if (opt->po_len != 3)
				return (OPT_REJ);
			break;
	
		case ED_LOCAL:		/* depricated */
			dprintf(D_DT, "\t{ED LOCAL");
			for (i = 1; i < opt->po_len - 2; i++)
				dprintf(D_DT, " %02x", opt->po_data.c[i]);
			dprintf(D_DT, "}\n");
			if (opt->po_len < 3 || opt->po_len > 3 + 20)
				return (OPT_REJ);
			break;
	
		case ED_IP:
			dprintf(D_DT, "\t{ED IP %d.%d.%d.%d}\n",
				opt->po_data.c[1], opt->po_data.c[2],
				opt->po_data.c[3], opt->po_data.c[4]);
			if (opt->po_len != 7)
				return (OPT_REJ);
			break;
	
		case ED_802_1:
			dprintf(D_DT, "\t{ED 802.1 %x:%x:%x:%x:%x:%x}\n",
				opt->po_data.c[1], opt->po_data.c[2],
				opt->po_data.c[3], opt->po_data.c[4],
				opt->po_data.c[5], opt->po_data.c[6]);
			if (opt->po_len != 9)
				return (OPT_REJ);
			break;
	
		case ED_PPP_MNB:		/* depricated */
			dprintf(D_DT, "\t{ED MNB");
			for (i = 1; i < opt->po_len - 2; i += 4)
				dprintf(D_DT, " %02x%02x%02x%02x",
				    opt->po_data.c[i+0], opt->po_data.c[i+1],
				    opt->po_data.c[i+2], opt->po_data.c[i+3]);
			dprintf(D_DT, "}\n");
			if (opt->po_len < 3 || opt->po_len > 3 + 20)
				return (OPT_REJ);
			break;
	
		case ED_PSND:
			dprintf(D_DT, "\t{ED PSND");
			for (i = 1; i < opt->po_len - 2; i++)
				dprintf(D_DT, " %02x", opt->po_data.c[i]);
			dprintf(D_DT, "}\n");
			if (opt->po_len < 3 || opt->po_len > 3 + 15)
				return (OPT_REJ);
			break;
	
		default:
			dprintf(D_DT, "\t{ED %d\n", opt->po_data.c[0]);
			for (i = 1; i < opt->po_len - 2; i++)
				dprintf(D_DT, " %02x", opt->po_data.c[i]);
			dprintf(D_DT, "}\n");
			if (opt->po_len < 3)
				return (OPT_REJ);
			break;
		}
		if (ppp->ppp_mrru <= 0 && !(ppp->ppp_flags & PPP_SND_SSEQ))
			return (OPT_REJ);
		ppp->ppp_rcv_ed.ed_type = opt->po_data.c[0];
		i = opt->po_len - 3;
		if (i > sizeof(ppp->ppp_rcv_ed.ed))
			i = sizeof(ppp->ppp_rcv_ed.ed);
		ppp->ppp_rcv_ed.ed_len = i;
		memset(ppp->ppp_rcv_ed.ed, 0, sizeof(ppp->ppp_rcv_ed.ed));
		if (i > 0)
			memcpy(ppp->ppp_rcv_ed.ed, &opt->po_data.c[1], i);
		return (OPT_OK);


	case LCP_FCS:
	case LCP_SDP:
	case LCP_NM:
	case LCP_MLP:
	case LCP_CBCP:
	case LCP_CT:
	case LCP_CF:
	case LCP_NDE:
	case LCP_PROP:
	case LCP_DCE_ID:
		dprintf(D_DT, "	{%s: len=%d}\n",
		    opt->po_type == LCP_FCS	? "FCS-Alt"	:
		    opt->po_type == LCP_SDP	? "SDP"		:
		    opt->po_type == LCP_NM	? "NM"		:
		    opt->po_type == LCP_MLP	? "MLP"		:
		    opt->po_type == LCP_CBCP	? "CBCP"	:
		    opt->po_type == LCP_CT	? "CT"		:
		    opt->po_type == LCP_CF	? "CF"		:
		    opt->po_type == LCP_NDE	? "NDE"		:
		    opt->po_type == LCP_PROP	? "PROP"	: "DCE-ID",
		    opt->po_len);
		return (OPT_REJ);

	default:
		dprintf(D_DT, "	{?%d: len=%d}\n", opt->po_type, opt->po_len);
		return (OPT_REJ);
	}
	/*NOTREACHED*/
}

/*
 * This level finished -- drop DTR
 */
void
ppp_lcp_tlf(ppp)
	ppp_t *ppp;
{
	ppp_clrstate(ppp, PPP_RUNNING);

	if (ppp->ppp_lcp.fsm_state == CP_CLOSED ||
	    ppp->ppp_lcp.fsm_state == CP_STOPPED)
		ppp_dtr(ppp, 0);
}

/*
 * Calculate the magic number
 */
u_long
ppp_lcp_magic()
{
	return(random());
}

/*
 * Initialize Multilink PPP state, and see if we
 * are running multilink.
 * Return values:
 *	-1: Multilink failed to init
 *	0: Multilink isn't running, or this is a new bundle
 *	1: Multilink running, we joined an existing bundle.
 */

int
ppp_ml_init(ppp) 
	ppp_t *ppp;
{
	struct ifpppreq ifrp;
	struct pifreq pifreq;
	int new;

	if (!ppp_ml_isup(ppp)) {
		ppp->ppp_pifname[0] = '\0';
		return(0);
	}

	dprintf(D_DT, "Initializing Multilink state\n");

	/*
	 * First, look and see if the connection exists.
	 */
	memset(&ifrp, 0, sizeof(ifrp));
	strncpy(ifrp.ifrp_name, ppp->ppp_ifname, sizeof (ifrp.ifrp_name));
	memcpy(&ifrp.ifrp_ed, &ppp->ppp_rcv_ed, sizeof (ifrp.ifrp_ed));
	memcpy(&ifrp.ifrp_auth, &ppp->ppp_rcv_auth, sizeof (ifrp.ifrp_auth));
	new = (ioctl(skt, PPPIOCFID, (caddr_t) &ifrp) < 0);
	if (new)
		dprintf(D_DT, "\tCreate new Multilink bundle\n");
	else
		dprintf(D_DT, "\tJoin Multilink bundle %s\n", ifrp.ifrp_pif);

	/*
	 * Next, create/join the bundle.
	 */
	memset(&ifrp, 0, sizeof(ifrp));
	strncpy(ifrp.ifrp_name, ppp->ppp_ifname, sizeof (ifrp.ifrp_name));
	memcpy(&ifrp.ifrp_ed, &ppp->ppp_rcv_ed, sizeof (ifrp.ifrp_ed));
	memcpy(&ifrp.ifrp_auth, &ppp->ppp_rcv_auth, sizeof (ifrp.ifrp_auth));
	if (ioctl(skt, PPPIOCSID, (caddr_t) &ifrp) < 0) {
		dprintf(D_DT, "\tFailed to %s Multilink bundle: %s\n",
		    new ? "create" : "join", strerror(errno));
		ppp->ppp_pifname[0] = '\0';
		return(-1);
	}
	dprintf(D_DT, "\t%s Multilink bundle %s\n",
	    new ? "Created" : "Joined", ifrp.ifrp_pif);
	strncpy(ppp->ppp_pifname, ifrp.ifrp_pif, sizeof(ppp->ppp_pifname));

	if (new) {
		/* While we're here, initialize the mrru */
		memset(&ifrp, 0, sizeof(ifrp));
		strncpy(ifrp.ifrp_name, ppp->ppp_ifname,
		    sizeof (ifrp.ifrp_name));
		ifrp.ifrp_mrru = ppp->ppp_mrru;
		(void)ioctl(skt, PPPIOCSMRRU, (caddr_t) &ifrp);

	}

	/* Check/set the pif_flags */
	memset(&pifreq, 0, sizeof(pifreq));
	strncpy(pifreq.pifr_name, ppp->ppp_pifname,
	    sizeof(pifreq.pifr_name));
	if (ioctl(skt, PIFIOCGFLG, &pifreq) < 0) {
		dprintf(D_DT, "\tFailed to get pif_flags for %s\n",
		    ppp->ppp_pifname);
	} else if (ppp->ppp_pifflags != pifreq.pifr_flags) {
		if (new) {
			pifreq.pifr_flags = ppp->ppp_pifflags;
			if (ioctl(skt, PIFIOCSFLG, &pifreq) < 0)
				dprintf(D_DT, "\tFailed to set pif_flags for "
				    "%s\n", ppp->ppp_pifname);
		} else {
			dprintf(D_DT, "\tWarning: requested pif_flags 0x%x "
			    "don't match %s flags: 0x%x\n", ppp->ppp_pifflags,
			    ppp->ppp_pifname, pifreq.pifr_flags);
		}
	}

	return (new ? 0 : 1);
}

int
ppp_ml_down(ppp)
	ppp_t *ppp;
{
	struct ifpppreq ifrp;

	if (!ppp_ml_isup(ppp))
		return(0);

	dprintf(D_DT, "%s leaving Multilink bundle %s\n",
		ppp->ppp_ifname, ppp->ppp_pifname);

	memset(&ifrp, 0, sizeof(ifrp));
	strncpy(ifrp.ifrp_name, ppp->ppp_ifname, sizeof (ifrp.ifrp_name));
	if (ioctl(skt, PPPIOCCID, (caddr_t) &ifrp) < 0) {
		dprintf(D_DT, "\t%s failed to leave Multilink bundle %s: %s\n",
		    ppp->ppp_ifname, ppp->ppp_pifname, strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * A PPP session is running multilink PPP if either an MRRU
 * or Short Sequence option was received.
 */
int
ppp_ml_isup(ppp) 
	ppp_t *ppp;
{
	return(ppp->ppp_flags & PPP_ISML);
}

int
ppp_ml_wanted(ppp)
	ppp_t *ppp;
{
	return ((ppp->ppp_mtru > 0) && ((ppp->ppp_mrru > 0) ||
	    (ppp->ppp_flags & PPP_SND_SSEQ) || (ppp->ppp_flags & PPP_USEML)));
}
