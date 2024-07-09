/*
 * IPX protocol formats 
 *
 * @(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/ipx.h,v 1.2 1998/02/06 02:02:56 jch Exp
 */

/* well-known sockets */
#define	IPX_SKT_NCP		0x0451
#define	IPX_SKT_SAP		0x0452
#define	IPX_SKT_RIP		0x0453
#define	IPX_SKT_NETBIOS		0x0455
#define	IPX_SKT_DIAGNOSTICS	0x0456

/* IPX transport header */
struct ipxHdr {
    u_short	cksum;		/* Checksum */
    u_short	length;		/* Length, in bytes, including header */
    u_char	tCtl;		/* Transport Control (i.e. hop count) */
    u_char	pType;		/* Packet Type (i.e. level 2 protocol) */
    u_short	dstNet[2];	/* destination net */
    u_char	dstNode[6];	/* destination node */
    u_short	dstSkt;		/* destination socket */
    u_short	srcNet[2];	/* source net */
    u_char	srcNode[6];	/* source node */
    u_short	srcSkt;		/* source socket */
} ipx_hdr_t;

#define ipxSize	30

/* Sap */
typedef struct sapRec {
	u_char	name[48];
	u_char	net[4];
	u_char	node[6];
	u_char	socket[2];
	u_char	hops[2];
} sap_rec_t;

typedef struct sapHdr {
	u_char	cmd[2];
	u_char	srvtype[2];
} sap_hdr_t;

#define SAP_GEN_REQ	1
#define	SAP_GEN_RESP	2
#define	SAP_NEAR_REQ	3
#define	SAP_NEAR_RESP	4

/* Rip */
typedef struct ripRec {
	u_char	net[4];
	u_char	hops[2];
	u_char	ticks[2];
} rip_rec_t;

typedef struct ripHdr {
	u_char	type[2];
} rip_hdr_t;

#define	RIP_REQUEST	1
#define	RIP_RESPONSE	2
