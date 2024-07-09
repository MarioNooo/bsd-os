/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_fsm.c,v 1.3 1993/04/17 17:57:18 karels Exp
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
 * PPP Control Protocols Finite State Machine
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <stdlib.h>
#include <string.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

static char *pktcodes[] = {
	"NULL", "Configure-Request", "Configure-Ack", "Configure-Nak",
	"Configure-Reject", "Terminate-Request", "Terminate-Ack", "Code-Reject",
	"Protocol-Reject", "Echo-Request", "Echo-Reply", "Discard-Request",
};
static char *papcodes[] = {
	"NULL", "Authenticate-Request", "Authenticate-Ack", "Authenticate-Nak",
};
static char *chapcodes[] = {
	"NULL", "Challenge", "Response", "Success", "Failure",
};


/*
 * The table of protocol events/actions
 */
struct ppp_fsm_tab {
	u_short	ft_type;		/* protocol number */
	u_short	ft_offset;		/* offset of ppp_fsm in ppp_sc */
	char	*ft_name;		/* protocol name */
	char	**ft_codes;
	int	ft_ncodes;
	int	(*ft_req)(ppp_t *);	/* Requisits done yet? */
	void	(*ft_creq)(ppp_t *);
					/* Config-Request composition */
	void	(*ft_xcodes)(ppp_t *, pbuf_t *, ppp_cp_t *);
					/* extra codes handling routine */
	void	(*ft_tlu)(ppp_t *);	/* top levels up */
	void	(*ft_tld)(ppp_t *);	/* top levels down */
	void	(*ft_tlf)(ppp_t *);	/* this level finished */
	void	(*ft_optnak)(ppp_t *, pppopt_t *);
					/* process nak-ed options */
	void	(*ft_optrej)(ppp_t *, pppopt_t *);
					/* process rejected options */
	int	(*ft_opt)(ppp_t *, pppopt_t *);
					/* process peer's options */
	void	(*ft_timeout)(void *);	/* timeout handler */
};

static struct ppp_fsm_tab ppp_fsm_tab[] = {
	/* Line Control Protocol -- FSM_LCP = 0 */
	{
		PPP_LCP,
		(u_short) &(((ppp_t *)0)->ppp_lcp),
		"LCP", pktcodes, 12,

		0,			/* LCP always works */
		ppp_lcp_creq,
		ppp_lcp_xcode,
		ppp_lcp_tlu,
		ppp_lcp_tld,
		ppp_lcp_tlf,
		ppp_lcp_optnak,
		ppp_lcp_optrej,
		ppp_lcp_opt,
		ppp_lcp_timeout
	},

	/* IP Control Protocol -- FSM_IPCP = 1 */
	{
		PPP_IPCP,
		(u_short) &(((ppp_t *)0)->ppp_ipcp),
		"IPCP", pktcodes, 12,

		ppp_ipcp_req,
		ppp_ipcp_creq,
		(void (*)())0,		/* xcode */
		ppp_ipcp_tlu,
		(void (*)())0,		/* tld */
		ppp_ipcp_tlf,
		ppp_ipcp_optnak,
		ppp_ipcp_optrej,
		ppp_ipcp_opt,
		ppp_ipcp_timeout,
	},

	/* Password Authentication Protocol -- FSM_PAP = 2 */
	{
		PPP_PAP, 0, "PAP", papcodes, 4,
		ppp_pap_req,
		0,
		ppp_pap_xcode,
		0, 0, 0, 0, 0, 0, 0,
	},

	/* Challange-Handshake Authentication Protocol -- FSM_CHAP = 3 */
	{
		PPP_CHAP, 0, "CHAP", chapcodes, 5,
		ppp_chap_req,
		0,
		ppp_chap_xcode,
		0, 0, 0, 0, 0, 0, 0,
	},

	/* IPv6 Control Protocol -- FSM_IPV6CP = 4 */
	{
		PPP_IPV6CP,
		(u_short) &(((ppp_t *)0)->ppp_ipv6cp),
		"IPV6CP", pktcodes, 12,

		ppp_ipv6cp_req,
		ppp_ipv6cp_creq,
		(void (*)())0,		/* xcode */
		ppp_ipv6cp_tlu,
		(void (*)())0,		/* tld */
		ppp_ipv6cp_tlf,
		ppp_ipv6cp_optnak,
		ppp_ipv6cp_optrej,
		ppp_ipv6cp_opt,
		ppp_ipv6cp_timeout,
	},

};

#define	newstate(fs, state)	_newstate(ppp, fs, state, #state)

void
_newstate(ppp, fs, state, name)
	ppp_t *ppp;
	struct ppp_fsm *fs;
	u_char state;
	char *name;
{
	dprintf(D_STATE, "state <- %s\n", name);

	if (fs->fsm_state == state)
		return;

	/*
	 * Keep the kernel in sync with our state of ip
	 */
	if (fs == &ppp->ppp_ipcp) {
		if (fs->fsm_state == CP_OPENED)
			ppp_clrstate(ppp, PPP_IPOKAY);
		if (state == CP_CLOSED || state == CP_CLOSING) {
			if (!(ppp->ppp_flags & PPP_DEMAND)) {
				dprintf(D_STATE, "Rejecting IP packets\n");
				ppp_setstate(ppp, PPP_IPREJECT);
			}
		} else if (fs->fsm_state == CP_CLOSED ||
		   fs->fsm_state == CP_CLOSING)
			ppp_clrstate(ppp, PPP_IPREJECT);
		if (state == CP_OPENED)
			ppp_setstate(ppp, PPP_IPOKAY);
	}
	fs->fsm_state = state;
}

/*
 * Prepend the PPP frame and CP headers
 */
pbuf_t *
ppp_cp_prepend(m, type, code, id, xlen)
	pbuf_t *m;
	int type;
	int code;
	int id;
	int xlen;
{
	register ppp_header_t *ph;
	register ppp_cp_t *lh;

	xlen += sizeof(ppp_header_t) + sizeof(ppp_cp_t);
	if ((m == 0 && (m = pbuf_alloc(LCP_DFLT_MRU)) == NULL) ||
	    pbuf_prepend(m, xlen) == NULL)
		return (0);

	ph = (ppp_header_t *)(m->data);
	ph->phdr_addr = PPP_ADDRESS;
	ph->phdr_ctl = PPP_CONTROL;
	ph->phdr_type = htons(type);
	lh = (ppp_cp_t *) (ph + 1);
	lh->cp_code = code;
	lh->cp_id = id;
	lh->cp_length = htons(m->len - sizeof(ppp_header_t));
	return (m);
}

/*
 * Add an option to the list
 */
pbuf_t *
ppp_addopt(m, opt)
	pbuf_t *m;
	pppopt_t *opt;
{
	u_char *p;

	if ((m == 0 && (m = pbuf_alloc(LCP_DFLT_MRU)) == NULL) ||
	    (p = pbuf_append(m, opt->po_len)) == NULL)
		return (0);
	memcpy(p, &opt->po_type, opt->po_len);
	return (m);
}

#define CP_DATA(m) (mtod((m), caddr_t) + sizeof(ppp_header_t) +\
		    sizeof(ppp_cp_t))

/*
 * Process an incoming control packet
 */
void
ppp_cp_in(ppp, m)
	ppp_t *ppp;
	struct pbuf_t *m;
{
	int type;
	ppp_header_t *ph = (ppp_header_t *)(m->data);
	ppp_cp_t *cph;
	pbuf_t *m0;
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft;
	pppopt_t *op;
	int len, off, ol, ot;
	int reject, nak;
	u_char cpid;
	static char *fsmstates[] = {
		"CP_INITIAL", "CP_STARTING", "CP_CLOSED", "CP_STOPPED",
		"CP_CLOSING", "CP_STOPPING", "CP_REQSENT", "CP_ACKRCVD",
		"CP_ACKSENT", "CP_OPENED",
	};

	m0 = NULL;
	op = NULL;
	reject = 0;
	nak = 0;

	if (m->len < sizeof(ppp_header_t)) {
		dprintf(D_PKT, "Dropping short packet:");
		for (len = 0; len < m->len; ++len)
			dprintf(D_PKT, " %02x", m->data[len]);
		dprintf(D_PKT, "\n");
		return;
	}

	switch (((u_short *)m->data)[0]) {
	case 0x0000:	/* API_SKIP */
		dprintf(D_PKT, "Dropping skipped packet\n");
		return;
	case 0x0001:	/* API_OK */
		ph = (ppp_header_t *)(m->data + 4);
		switch (ntohs(ph->phdr_type)) {
		case PPP_IP: {
			struct ip *ip = (struct ip *)(ph + 1);
			dprintf(D_PKT, "Bad IP packet: hl=%d ver=%d len=%d+6 FCS %04x (%d bytes)\n",
			    ip->ip_hl, ip->ip_v, ntohs(ip->ip_len),
			    *(u_short *)(m->data + m->len - 2), m->len - 4);
		    }
			return;
#if 0 /* this is just for testing */
		case PPP_LCP:
			pbuf_trim(m, 4);
			m->len -= 2;	/*remove FCS*/
			ph = (ppp_header_t *)(m->data);
			cph = (ppp_cp_t *)(m->data + 4);
			len = ntohs(cph->cp_length);
			dprintf(D_PKT, "LCP with bad length? %d %d\n",
				len + 4, m->len);
			if (len < m->len - 4) {
				cph->cp_length = htons(m->len - 4);
			}
			break;
#endif
		default:
			dprintf(D_PKT, "Dropping packet type %04x (%d bytes): "
			    "bad FCS\n",
			    ntohs(ph->phdr_type),
			    m->len - 4);
			return;
		}
		break;
	case 0x0002:	/* API_ESC */
		dprintf(D_PKT, "Dropping escape packet\n");
		return;
	case PPPD_PKTSENT:	/* Packet Sent */
		dprintf(D_PKT, "Sent packet of %d bytes FCS of %04x\n",
		    ((u_short *)m->data)[1], ((u_short *)m->data)[2]);
		return;
	case PPPD_PKTRECV:	/* Packet Received */
		dprintf(D_PKT, "Recv packet of %d bytes FCS of %04x\n",
		    ((u_short *)m->data)[1], ((u_short *)m->data)[2]);
		return;
#ifdef	PPPD_PKTIN
	case PPPD_PKTIN:	/* Input Packet */
		/*
		 * We don't print anything here, let the normal packet
		 * printing take care of this.
		 */
		return;
#endif
	default:	
		break;
	}

	type = ntohs(ph->phdr_type);

	if (ph->phdr_addr != PPP_ADDRESS || ph->phdr_ctl != PPP_CONTROL) {
		dprintf(D_PKT, "Dropping invalid packet: %02x %02x type %04x\n",
			ph->phdr_addr, ph->phdr_ctl, type);
		return;
	}

	/*
	 * Nothing left of the header that we need to process.  Strip it away.
	 */
	pbuf_trim(m, sizeof(ppp_header_t));

	/*
	 * Look for a protocol in the table
	 */
	ft = ppp_fsm_tab;
	for (len = (sizeof ppp_fsm_tab) / (sizeof ppp_fsm_tab[0]); len--; ft++)
		if (ft->ft_type == type)
			goto found;

	/*
	 * Handle bad (unknown) protocol
	 */
	if (ppp->ppp_lcp.fsm_state != CP_OPENED) {
		dprintf(D_PKT, "unkown protocol %04x (len=%d) -- discarded\n",
			type, m->len);
		return;
	}

	/*
	 * Push the protocol number back on the front.
	 * Trim the packet to not exceed the MRU.
	 * Prepend LCP_PROTO_REJ.
	 */
	dprintf(D_PKT, "unkown protocol %04x (len=%d) -- rejected\n", type, m->len);

	pbuf_prepend(m, 2);
	if (m->len + 4 > ppp->ppp_mru)
		pbuf_trim(m, ppp->ppp_mru - (m->len + 4));

	m = ppp_cp_prepend(m, PPP_LCP, LCP_PROTO_REJ, ppp->ppp_nid++, 0);

	sendpacket(ppp, m);
	return;

found:	/* Protocol is known -- get the pointer to fsm data */

	/*
	 * Check and skip the CP header
	 */
	cph = (ppp_cp_t *)(m->data);
	len = ntohs(cph->cp_length);

	dprintf(D_PKT, "RCV PACKET %s%d %s id=%d len=%d",
	    ft->ft_name, cph->cp_code,
	    (cph->cp_code < ft->ft_ncodes) ? ft->ft_codes[cph->cp_code] : "?",
	    cph->cp_id, len);

	if (len > m->len || len < 0) {
		dprintf(D_PKT, "\n	BAD LENGTH (total=%d)\n", m->len);
		return;
	}

	pbuf_trim(m, sizeof(ppp_cp_t));

	len -= sizeof(ppp_cp_t);

	/*
	 * Protocols which do not use the LCP state machine will have
	 * an offset of zero.  No further processing is needed.
	 */
	if (ft->ft_offset == 0) {
		dprintf(D_PKT, "\n");
		(*ft->ft_xcodes)(ppp, m, cph);
		return;
	}

	/*
	 * Ignore incoming packets if link is supposed to be down
	 */
	fs = (struct ppp_fsm *) ((char *)ppp + ft->ft_offset);

	if (ft->ft_req && (*ft->ft_req)(ppp) < 0) {
		dprintf(D_PKT, "\n	prerequisites not met, ignored.\n");
		return;
	}

	if (fs->fsm_state == CP_INITIAL || fs->fsm_state == CP_STARTING) {
		/*
		 * We allow TERM ACK's through even if the line is down.
		 */
		if (cph->cp_code != CP_TERM_ACK) {
			dprintf(D_PKT, "\n	link is down, ignored.\n");
			return;
		}
	}

	dprintf(D_PKT, " [%s]\n", (fs->fsm_state < 10) ? fsmstates[fs->fsm_state] : "?");

	/*
	 * Select by incoming packet code
	 */
	cpid = cph->cp_id;
	switch (cph->cp_code) {
	default:			/* Unknown code */
		if (ft->ft_xcodes != 0) {
			(*ft->ft_xcodes)(ppp, m, cph);
			return;
		}

		/*
		 * Report unknown code and senf Conf-Reject
		 * Replace the configure header
		 */
		pbuf_prepend(m, sizeof(ppp_cp_t));
		dprintf(D_DT, "	code %d unknown - rejected\n", cph->cp_code);
		m = ppp_cp_prepend(m, type, CP_CODE_REJ, 0, 0);
		sendpacket(ppp, m);
		break;

	case CP_CONF_REQ:		/* Configure-Request */
		switch (fs->fsm_state) {
		case CP_CLOSING:
		case CP_STOPPING:
			dprintf(D_DT, "	ignored\n");
			return;

		case CP_CLOSED:
			dprintf(D_DT, "	send Terminate-Ack\n");
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			sendpacket(ppp, m);
			pbuf_free(m);
			return;

		case CP_STOPPED:
			/* Initialize restart counter */
			fs->fsm_rc = ppp->ppp_maxconf;
			/* FALLTHROUGH */
		case CP_OPENED:
			/* Set conservative TX ccm */
			if (type == PPP_LCP)
				ppp->ppp_txcmap = 0xffffffff;
			dprintf(D_DT, "	send Conf-Req AGAIN\n");

			/*
			 * The messy case of resynching from OPENED state
			 */
			if (ft->ft_tld)
				(*ft->ft_tld)(ppp);

			/*
			 * Since we're renegotiating, zero out the
			 * reject bits.
			 */
			fs->fsm_rej = 0;
			/*
			 * Send out our own Configure-Request
			 */
			(*ft->ft_creq)(ppp);
			timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
		}

		/*
		 * Parse options
		 */
		off = 0;
		while (off < len) {
			/*
			 * get the next option from the list
			 */
			if (len - off < 2) {
				dprintf(D_DT, "	{too short %d}\n", len - off);
				break;
			}
			ot = m->data[off];
			ol = m->data[off+1];

			if (off + ol > len) {
				dprintf(D_DT, "	{truncated}\n");
				goto reject;
			}
			pbuf_init(&ppp->ppp_opt);
			op = (pppopt_t *)ppp->ppp_opt.data;

			memcpy(&op->po_type, m->data + off, ol);
			off += ol;

			/*
			 * process the option
			 */
			switch ((*ft->ft_opt)(ppp, op)) {
			case OPT_OK:
				continue;

			case OPT_REJ:
			reject:
				dprintf(D_DT, "	REJECT\n");
				reject++;
				if (nak && m0) {
					pbuf_free(m0);
					m0 = 0;
					nak = 0;
				}
				break;

			case OPT_NAK:
				dprintf(D_DT, "	NAK\n");
				/*
				 * If we NAK'ed the MAGIC number then we are
				 * probably looking at our own packet echo'ed
				 * back to us. So, just send back a NAK of
				 * the MAGIC number and nothing else.
				 */
                                if (op->po_type == LCP_MAGIC) {
                                	if (m0) {
						pbuf_free(m0);
						m0 = NULL;
					}
					m0 = ppp_addopt(m0, op);
					goto nakmagic;
					}
				if (reject)
					continue;
				nak++;
				break;

			case OPT_FATAL:
				dprintf(D_DT, "	FATAL\n");
				if (m0)
					pbuf_free(m0);
				ppp_cp_close(ppp, ft - ppp_fsm_tab);
				return;
			}
			m0 = ppp_addopt(m0, op);
		}

		/*
		 * Send Nak/Reject/Ack depending on circumstances
		 */
		if (reject) {
			if (m0 == 0)
				return;
			dprintf(D_DT, "	send Conf-Rej\n");
			m = ppp_cp_prepend(m0, type, CP_CONF_REJ, cpid, 0);
			if (fs->fsm_state != CP_ACKRCVD)
				newstate(fs, CP_REQSENT);
			sendpacket(ppp, m);
			pbuf_free(m);
		} else if (nak) {
nakmagic:		if (m0 == 0)
				return;
			m = ppp_cp_prepend(m0, type, CP_CONF_NAK, cpid, 0);
			sendpacket(ppp, m);
			dprintf(D_DT, "	send Conf-Nak (len=%d)\n", m->len);
			if (fs->fsm_state != CP_ACKRCVD)
				newstate(fs, CP_REQSENT);
			pbuf_free(m);
		} else {
			m = ppp_cp_prepend(m, type, CP_CONF_ACK, cpid, 0);
			dprintf(D_DT, "	send Conf-Ack (len=%d)\n", m->len);

			/*
			 * We SHOULD queue Conf-Ack first, and only then
			 * turn on upper levels.
			 */
			if (m == 0)
				return;
			sendpacket(ppp, m);

			/*
			 * Change FSM state and issue Up to upper levels
			 */
			if (fs->fsm_state == CP_ACKRCVD) {
				newstate(fs, CP_OPENED);
				if (ft->ft_tlu)
					(*ft->ft_tlu)(ppp);
			} else
				newstate(fs, CP_ACKSENT);
		}
		break;

	case CP_CONF_ACK:		/* Configure-Ack */
		if (cpid != fs->fsm_id) {
			dprintf(D_DT, "	wrong id %d (exp %d)\n", cpid, fs->fsm_id);
			break;
		}

		/*
		 * Clear timeouts
		 */
		untimeout(ft->ft_timeout, ppp);

		switch (fs->fsm_state) {
		case CP_CLOSED:
		case CP_STOPPED:
			dprintf(D_DT, "	send Terminate-Ack\n");
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			sendpacket(ppp, m);
			pbuf_free(m);
			break;

		default:
			dprintf(D_DT, "	ignored\n");
			break;

		case CP_REQSENT:
			dprintf(D_DT, "	init restart cntr\n");
			newstate(fs, CP_ACKRCVD);
			/*
			 * Initialize restart counter
			 */
			fs->fsm_rc = ppp->ppp_maxconf;
			break;

		case CP_OPENED:
			if (ft->ft_tld)
				(*ft->ft_tld)(ppp);
			/* FALLTHROUGH */

		case CP_ACKRCVD:
			dprintf(D_DT, "	send Conf-Req\n");
			newstate(fs, CP_REQSENT);
			/*
			 * Send Config-Request
			 */
			(*ft->ft_creq)(ppp);
			timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
			break;

		case CP_ACKSENT:
			newstate(fs, CP_OPENED);
			/*
			 * Initialize restart counter
			 */
			fs->fsm_rc = ppp->ppp_maxconf;
			if (ft->ft_tlu)
				(*ft->ft_tlu)(ppp);
			break;
		}
		break;

	case CP_CONF_NAK:		/* Configure-Nak */
	case CP_CONF_REJ:		/* Configure-Reject */
		if (cpid != fs->fsm_id) {
			dprintf(D_DT, "	wrong id %d (exp %d)\n", cpid, fs->fsm_id);
			break;
		}

		/*
		 * Clear timeouts
		 */
		untimeout(ft->ft_timeout, ppp);

		switch (fs->fsm_state) {
		default:
			dprintf(D_DT, "	ignored\n");
			return;

		case CP_CLOSED:
		case CP_STOPPED:
			dprintf(D_DT, "	send Terminate-Ack\n");
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			sendpacket(ppp, m);
			pbuf_free(m);
			return;

		case CP_REQSENT:
		case CP_ACKSENT:
			/*
			 * Initialize restart counter
			 */
			fs->fsm_rc = ppp->ppp_maxconf;
			break;

		case CP_OPENED:
			if (ft->ft_tld)
				(*ft->ft_tld)(ppp);
			/*FALLTHROUGH*/
		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			break;
		}

		/*
		 * Parse options
		 */
		off = 0;
		while (off < len) {
			/*
			 * get the next option from the list
			 */
			if (len - off < 2) {
				dprintf(D_DT, "	{too short %d}\n", len - off);
				break;
			}
			ol = m->data[off+1];

			if (off + ol > len) {
				dprintf(D_DT, "	{truncated}\n");
				goto reject;
			}
			pbuf_init(&ppp->ppp_opt);
			op = (pppopt_t *)ppp->ppp_opt.data;

			memcpy(&op->po_type, m->data + off, ol);
			off += ol;

			if (cph->cp_code == CP_CONF_REJ) {
				fs->fsm_rej |= 1 << op->po_type;
				(*ft->ft_optrej)(ppp, op);
			} else
				(*ft->ft_optnak)(ppp, op);
		}

		/*
		 * Send the Configure-Request
		 */
		dprintf(D_DT, "	send Conf-Req\n");
		(*ft->ft_creq)(ppp);
		timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
		break;

	case CP_TERM_REQ:
		switch (fs->fsm_state) {
		case CP_OPENED:
			fs->fsm_rc = 1;		/* Zero the restart counter */
			if (ft->ft_tld)
				(*ft->ft_tld)(ppp);
			newstate(fs, CP_STOPPING);
			timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
			break;

		case CP_ACKRCVD:
		case CP_ACKSENT:
			newstate(fs, CP_REQSENT);
			break;
		}

		/*
		 * Set conservative TX ccm
		 */
		if (type == PPP_LCP)
			ppp->ppp_txcmap = 0xffffffff;

		dprintf(D_DT, "	send Term-Ack\n");
		m = ppp_cp_prepend(m, type, CP_TERM_ACK, cpid, 0);
		sendpacket(ppp, m);
		break;

	case CP_TERM_ACK:
		/*
		 * Clear timeouts
		 */
		untimeout(ft->ft_timeout, ppp);

		switch (fs->fsm_state) {
		case CP_OPENED:
			dprintf(D_DT, "	send Conf-Req\n");
			if (ft->ft_tld)
				(*ft->ft_tld)(ppp);
			(*ft->ft_creq)(ppp);
			timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
			break;

		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			/* FALLTHROUGH */
		default:
			dprintf(D_DT, "	no action\n");
			break;

		case CP_INITIAL:
		case CP_CLOSING:
			newstate(fs, CP_CLOSED);
			goto drop_conn;

		case CP_STOPPING:
			newstate(fs, CP_STOPPED);
		drop_conn:
			ppp_ctl(ppp, PPP_FLUSHQ);

			/* Clear timeouts */
			untimeout(ft->ft_timeout, ppp);

			/* This level finished! */
			if (ft->ft_tlf)
				(*ft->ft_tlf)(ppp);
			break;
		}
		break;

	case CP_CODE_REJ:	/* Code-Reject */
		/*
		 * We've got a problem!
		 */
		dprintf(D_PKT, "Code-Reject received for code %d\n",
			((ppp_cp_t *)(m->data))->cp_code);
		break;
	}
}

/*
 * Restart timeout
 */
void
ppp_cp_timeout(ppp, fsm)
	ppp_t *ppp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	fs = (struct ppp_fsm *) ((char *)ppp + ft->ft_offset);
	dprintf(D_PKT, "%s[rc=%d] TO", ft->ft_name, fs->fsm_rc);

	if (fs->fsm_rc == 1) {	/* TO- */
		dprintf(D_PKT, "- Drop Connection\n");
		fs->fsm_rc = 0;

		/*
		 * Go to the new state
		 */
		if (fs->fsm_state == CP_CLOSING)
			newstate(fs, CP_CLOSED);
		else
			newstate(fs, CP_STOPPED);

		/*
		 * Do same stuff as at "drop_conn" label in ppp_cp_in().
		 */

		ppp_ctl(ppp, PPP_FLUSHQ);

		/*
		 * This level finished!
		 */
		if (ft->ft_tlf)
			(*ft->ft_tlf)(ppp);

	} else {		/* TO+ */
		if (fs->fsm_rc > 0)
			fs->fsm_rc--;

		switch (fs->fsm_state) {
		default:
			dprintf(D_PKT, "+ -- illegal state %d\n", fs->fsm_state);
			return;

		case CP_CLOSING:
		case CP_STOPPING:
			/*
			 * Send the termination request
			 */
			dprintf(D_PKT, "+ - send Term-Req\n");
			ppp->ppp_termtries = 0;
			ppp_sendterm(ppp);
			break;

		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			/* FALL THROUGH */
		case CP_REQSENT:
		case CP_ACKSENT:
			dprintf(D_PKT, "+ - send Conf-Req\n");
			(*ft->ft_creq)(ppp);
		}
		timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
	}
}

/*
 * The Up event from lower level
 */
void
ppp_cp_up(ppp, fsm)
	ppp_t *ppp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(D_PKT, "ppp_cp_up: %s\n", ft->ft_name);
	fs = (struct ppp_fsm *) ((char *)ppp + ft->ft_offset);

	/*
	 * Initialize restart counter
	 */
	fs->fsm_rc = ppp->ppp_maxconf;
	fs->fsm_rej = 0;

	/*
	 * Send out Config-Req
	 */
	newstate(fs, CP_REQSENT);
	(*ft->ft_creq)(ppp);
	timeout(ft->ft_timeout, ppp, &ppp->ppp_timeout);
}

/*
 * The Down event from lower level
 */
void
ppp_cp_down(ppp, fsm)
	ppp_t *ppp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(D_PKT, "ppp_cp_down: %s\n", ft->ft_name);
	fs = (struct ppp_fsm *) ((char *)ppp + ft->ft_offset);
	switch (fs->fsm_state) {
	case CP_CLOSING:
	case CP_CLOSED:
		newstate(fs, CP_INITIAL);
		break;

	case CP_OPENED:
		if (ft->ft_tld)
			(*ft->ft_tld)(ppp);
		/* FALL THROUGH */
	default:
		newstate(fs, CP_STARTING);
	}

	/* Clear timeouts */
	untimeout(ft->ft_timeout, ppp);
}

/*
 * Close the protocol
 */
void
ppp_cp_close(ppp, fsm)
	ppp_t *ppp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(D_PKT, "ppp_cp_close: %s\n", ft->ft_name);
	fs = (struct ppp_fsm *) ((char *)ppp + ft->ft_offset);
	switch (fs->fsm_state) {
	case CP_STARTING:
		newstate(fs, CP_INITIAL);
		break;

	case CP_STOPPED:
		newstate(fs, CP_CLOSED);
		break;

	case CP_OPENED:
		if (ft->ft_tld)
			(*ft->ft_tld)(ppp);
		/*FALLTHROUGH*/

	case CP_REQSENT:
	case CP_ACKRCVD:
	case CP_ACKSENT:
		/*
		 * Initialize restart counter
		 */
		fs->fsm_rc = ppp->ppp_maxterm;
		newstate(fs, CP_CLOSING);

		/* Set conservative txcmap */
		if (fsm == FSM_LCP)
			ppp->ppp_txcmap = 0xffffffff;

		/* Send terminate-request */
		dprintf(D_PKT, "Send Term-Req\n");
		ppp->ppp_termtries = 0;
		ppp_sendterm(ppp);
		break;

	case CP_STOPPING:
		newstate(fs, CP_CLOSING);
		break;
	}
}
