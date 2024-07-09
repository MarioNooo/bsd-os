/* @(#) /master/core_contrib/tcpdump/libpcap-0.4a6/ppp.h,v 1.2 1998/02/17 14:42:07 jch Exp (LBL) */
/*
 * Point to Point Protocol (PPP) RFC1331
 *
 * Copyright 1989 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon and Stanford
 * University.  Carnegie Mellon makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
#define PPP_ADDRESS	0xff	/* The address byte value */
#define PPP_CONTROL	0x03	/* The control byte value */

/* Protocol numbers */
#define PPP_IP		0x0021	/* Raw IP */
#define PPP_OSI		0x0023	/* OSI Network Layer */
#define PPP_NS		0x0025	/* Xerox NS IDP */
#define PPP_DECNET	0x0027	/* DECnet Phase IV */
#define PPP_APPLE	0x0029	/* Appletalk */
#define PPP_IPX		0x002b	/* Novell IPX */
#define PPP_VJC		0x002d	/* Van Jacobson Compressed TCP/IP */
#define PPP_VJNC	0x002f	/* Van Jacobson Uncompressed TCP/IP */
#define PPP_BRPDU	0x0031	/* Bridging PDU */
#define PPP_STII	0x0033	/* Stream Protocol (ST-II) */
#define PPP_VINES	0x0035	/* Banyan Vines */
#define	PPP_IPV6	0x0057	/* Raw IPv6 */

#define PPP_HELLO	0x0201	/* 802.1d Hello Packets */
#define PPP_LUXCOM	0x0231	/* Luxcom */
#define PPP_SNS		0x0233	/* Sigma Network Systems */

#define PPP_IPCP	0x8021	/* IP Control Protocol */
#define PPP_OSICP	0x8023	/* OSI Network Layer Control Protocol */
#define PPP_NSCP	0x8025	/* Xerox NS IDP Control Protocol */
#define PPP_DECNETCP	0x8027	/* DECnet Control Protocol */
#define PPP_APPLECP	0x8029	/* Appletalk Control Protocol */
#define PPP_IPXCP	0x802b	/* Novell IPX Control Protocol */
#define PPP_STIICP	0x8033	/* Strean Protocol Control Protocol */
#define PPP_VINESCP	0x8035	/* Banyan Vines Control Protocol */

#define PPP_LCP		0xc021	/* Link Control Protocol */
#define PPP_PAP		0xc023	/* Password Authentication Protocol */
#define PPP_LQR		0xc025	/* Link Quality Monitoring */
#define PPP_CHAP	0xc223	/* Challenge Handshake Authentication Protocol */

#define	PPP_CP_CODE		0	/* Offset to code (one byte) */
#define	PPP_CP_ID		1	/* Offset to ID (one byte) */
#define	PPP_CP_LEN		2	/* Offset to length (two bytes) */
#define	PPP_CPHDR_LEN		4	/* Length of CP header */

#define PPP_CP_CONF_REQ		1       /* Configure-Request */
#define PPP_CP_CONF_ACK		2       /* Configure-Ack */
#define PPP_CP_CONF_NAK		3       /* Configure-Nak */
#define PPP_CP_CONF_REJ		4       /* Configure-Reject */
#define PPP_CP_TERM_REQ		5       /* Terminate-Request */
#define PPP_CP_TERM_ACK		6       /* Terminate-Ack */
#define PPP_CP_CODE_REJ		7       /* Code-Reject */
#define PPP_LCP_PROTO_REJ	8       /* Protocol-Reject */
#define PPP_LCP_ECHO_REQ	9       /* Echo-Request */
#define PPP_LCP_ECHO_REPL	10      /* Echo-Reply */
#define PPP_LCP_DISC_REQ	11      /* Discard-Request */
#define	PPP_LCP_IDENT		12	/* Identification */
#define	PPP_LCP_TIME_REMAIN	13	/* Time remaintin */

#define	PPP_OP_TYPE		0	/* Offset to option type */
#define	PPP_OP_LEN		1	/* Offset to option length */
#define	PPP_OP_DATA		2	/* Offset to option data */

/* LCP option types */
#define PPP_LCP_MRU		1       /* Maximum-Receive-Unit */
#define PPP_LCP_ACCM		2       /* Async-Control-Character-Map */
#define PPP_LCP_AP		3       /* Authentication-Protocol */
#define PPP_LCP_QP		4       /* Quality-Protocol */
#define PPP_LCP_MAGIC		5       /* Magic-Number */
#define PPP_LCP_PFC		7       /* Protocol-Field-Compression */
#define PPP_LCP_ACFC		8       /* Address-and-Control-Field-Compression */

/* IPCP option types */
#define PPP_IPCP_ADDRS		1       /* IP-Addresses */
#define PPP_IPCP_CPROT		2       /* IP-Compression-Protocol */
#define PPP_IPCP_ADDR		3       /* IP-Address */
#define	PPP_IPCP_PDNS		129	/* Microsoft Primary DNS */
#define	PPP_IPCP_PNBNS		130	/* Microsoft Primary NBNS */
#define	PPP_IPCP_SDNS		131	/* Microsoft Secondary DNS */
#define	PPP_IPCP_SNBNS		132	/* Microsoft Secondary NBNS */

/* CHAP control packets */
#define	PPP_CHAP_CHALLANGE	1	/* Challange */
#define	PPP_CHAP_RESPONSE	2	/* Response */
#define	PPP_CHAP_SUCCESS	3	/* Success */
#define	PPP_CHAP_FAILURE	4	/* Failure */

/* CHAP Algorithms */
#define	PPP_CHAPA_MD5	5	/* MD5 */

/* PAP control packets */
#define	PPP_PAP_AUTH_REQ	1	/* Authentication-Request */
#define	PPP_PAP_AUTH_ACK	2	/* Authentication-Ack */
#define	PPP_PAP_AUTH_NAK	3	/* Authenitcation-Nak */
