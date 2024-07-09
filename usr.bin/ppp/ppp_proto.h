/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_proto.h,v 2.6 2001/10/03 17:29:57 polk Exp
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
 * The PPP definitions (RFC 1331)
 */

#include <net/if_ppp.h>

typedef struct ppp_header ppp_header_t;

/*
 * PPP Control Protocols definitions
 */

/* LCP finite state automaton states */
#define CP_INITIAL      0
#define CP_STARTING     1
#define CP_CLOSED       2
#define CP_STOPPED      3
#define CP_CLOSING      4
#define CP_STOPPING     5
#define CP_REQSENT      6
#define CP_ACKRCVD      7
#define CP_ACKSENT      8
#define CP_OPENED       9

typedef struct {
	u_char  cp_code;        /* packet code */
	u_char  cp_id;          /* packet id */
	u_short cp_length;      /* CP packet length */
} ppp_cp_t;

/* Control Protocols packet codes */
#define CP_CONF_REQ     1       /* Configure-Request */
#define CP_CONF_ACK     2       /* Configure-Ack */
#define CP_CONF_NAK     3       /* Configure-Nak */
#define CP_CONF_REJ     4       /* Configure-Reject */
#define CP_TERM_REQ     5       /* Terminate-Request */
#define CP_TERM_ACK     6       /* Terminate-Ack */
#define CP_CODE_REJ     7       /* Code-Reject */
#define LCP_PROTO_REJ   8       /* Protocol-Reject */
#define LCP_ECHO_REQ    9       /* Echo-Request */
#define LCP_ECHO_REPL   10      /* Echo-Reply */
#define LCP_DISC_REQ    11      /* Discard-Request */

/*
 * CP Options
 */
typedef struct {
	u_short po_PAD;         /* padding option structure to align lcp_data */
	u_char  po_type;        /* option type */
	u_char  po_len;         /* option length */
	union {
		u_short s;      /* short */
		u_long  l;      /* long */
		u_char  c[254];  /* chars */
	}       po_data;        /* option data (variable length) */
} pppopt_t;

#define LCP_OPT_MAXSIZE 34

/*
 * Line Control Protocol definitions
 */
/* LCP Option defaults */
#define LCP_DFLT_MRU    1500    /* our default packet size */
#define LCP_DFLT_MRRU	1600    /* our default reconstructed packet size */
#define LCP_MIN_MRU     128     /* minimal packet size the remote side should accept */

/* LCP option types */
#define LCP_MRU         1       /* Maximum-Receive-Unit */
#define LCP_ACCM        2       /* Async-Control-Character-Map */
#define LCP_AP          3       /* Authentication-Protocol */
#define LCP_QP          4       /* Quality-Protocol */
#define LCP_MAGIC       5       /* Magic-Number */
#define LCP_PFC         7       /* Protocol-Field-Compression */
#define LCP_ACFC        8       /* Address-and-Control-Field-Compression */
#define	LCP_FCS		9	/* FCS-Alternatives */
#define	LCP_SDP		10	/* Self-Describing-Pad */
#define	LCP_NM		11	/* Numbered-Mode */
#define	LCP_MLP		12	/* Multi-Link-Procedure */
#define	LCP_CBCP	13	/* Callback Control Protocol */
#define	LCP_CT		14	/* Connect-Time */
#define	LCP_CF		15	/* Compound-Frames */
#define	LCP_NDE		16	/* Nominal-Data-Encapsulation */
#define	LCP_MRRU	17	/* Max-Receive-Reconstructed-Unit */
#define	LCP_SSEQ	18	/* Short Sequence Number Header Format */
#define	LCP_ED		19	/* Endpoint Discriminator */
#define	LCP_PROP	20	/* Proprietary */
#define	LCP_DCE_ID	21	/* DCE-Identifier */

/*
 * PPP Network Control Protocol for IP (IPCP) definitions (RFC 1332)
 */

/* IPCP option types */
#define IPCP_ADDRS      1       /* IP-Addresses */
#define IPCP_CPROT      2       /* IP-Compression-Protocol */
#define IPCP_ADDR       3       /* IP-Address */

#define	IPCP_MS_PDNS	129	/* Microsoft Primary DNS request */
#define	IPCP_MS_SDNS	131	/* Microsoft Secondary DNS request */
#define	IPCP_MS_PNBS	130	/* Microsoft Primary NetBUI server request */
#define	IPCP_MS_SNBS	132	/* Microsoft Secondary NetBUI server request */

#define	PAP_REQUEST	1	/* Request authetication of peerid/passwd */
#define	PAP_ACK		2	/* Ack authenticatin (user authenticated) */
#define	PAP_NAK		3	/* Nak authenticatin (user NOT authenticated) */

#define	CHAP_CHALLANGE	1	/* Send CHAP challange */
#define	CHAP_RESPONSE	2	/* Send CHAP challange */
#define	CHAP_ACK	3	/* Ack authenticatin (user authenticated) */
#define	CHAP_NAK	4	/* Nak authenticatin (user NOT authenticated) */

#define	CHAP_MD5	5	/* The default CHAP protocol */

#define IPV6CP_IFTOKEN	1	/* Interface-Token */
#define IPV6CP_CPROT	2	/* IPv6-Compression-Protocol */

/*
 * Endpoint Descriminator subfields
 */
#define	ED_NULL		0	/* Null Class */
#define	ED_LOCAL	1	/* Locally Assigned Address */
#define	ED_IP		2	/* Internet Protocol (IP) Address */
#define	ED_802_1	3	/* IEEE 802.1 Globally Assigned MAC Address */
#define	ED_PPP_MNB	4	/* PPP Magic-Number Block */
#define	ED_PSND		5	/* Public Switched Network Directory Number */
