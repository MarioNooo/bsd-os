/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_var.h,v 2.23 2001/10/03 17:29:57 polk Exp
 */

/*
 * The software state of a PPP link
 */

#include <net/if_pppioctl.h>
/*
 * PPP finite state machine definitions
 */
struct ppp_fsm {
	u_char  fsm_state;	/* Control protocol state */
	u_char  fsm_id;		/* Control protocol id */
	u_short fsm_rc;		/* Restart counter */
	u_long fsm_rej;		/* Rejected options */
};

typedef struct {
	int	ppp_fd;			/* filedes to raw link layer */
	struct sockaddr_dl ppp_dl;	/* link level address */
	char	*ppp_ifname;		/* name of interface */
	char	*ppp_pifname;		/* name of parallel interface */
	char	*ppp_sysname;		/* name of system */

	int	ppp_mru;		/* The current MRU */
	int	ppp_mtu;		/* The current MTU */
	int	ppp_maxconf;		/* max config retries */
	int	ppp_maxterm;		/* max term retries */
	int	ppp_termtries;		/* how many times we tried already */
	int	ppp_nid;		/* Next ID to send */
	pbuf_t	ppp_opt;		/* option buffer */

	u_long	ppp_txcmap;		/* active TX chars map */
	u_long	ppp_rxcmap;		/* active TX chars map */
	u_long	ppp_tcmap;		/* chars map sent */
	u_long	ppp_rcmap;		/* chars map received */
	u_long	ppp_flags;		/* lower 16 bits share with kernel */

	/* Timer parameters */
	struct	timeval ppp_timeout;	/* timeout to resend a packet */
	struct	timeval ppp_idletime;	/* idle time counter */
	struct	timeval ppp_bostimeout;	/* how long to wait for bos */
	int	ppp_idletimesent;	/* idle time sent to kernel */

	struct	timeval ppp_echofreq;	/* how often to send echo packets */
	struct	timeval ppp_echowait;	/* how long to wait for echo reply */

	/* LCP state */
	struct	ppp_fsm ppp_lcp;	/* line control protocol state */
	u_long	ppp_magic;		/* our magic number */
	u_long	ppp_remotemagic;	/* their magic number */
	u_char	ppp_badmagic;		/* number of retries with magic */
	u_char	ppp_ncps;		/* the number of active NCPs */
	u_char	ppp_txflags;		/* TX modes accepted by peer */
	u_char	ppp_wdrop;		/* Someone's waiting for dropped */

	/* IPCP state */
	struct	ppp_fsm ppp_ipcp;	/* internet control protocol state */
	u_long	ppp_dns[2];		/* DNS addresses */
	u_long	ppp_nbs[2];		/* NetBUI addresses */
	u_long	ppp_us;			/* Our IP address */
	u_long	ppp_them;		/* Their IP address */

	/* PAP */
	char	*ppp_peerid;		/* PeerID to *send* to remote side */
	char	*ppp_passwd;		/* Passwd to *send* to remote side */

	/* CHAP */
	int	ppp_chapmode;		/* Mode to run CHAP in (should be 5) */
	char	*ppp_chapscript;	/* Script which does CHAP */
	char	*ppp_chapname;		/* What name we use */
	char	*ppp_chapsecret;	/* What name we use to look up secret */
	u_short	ppp_rejectauth[3];	/* last three types we rejected */

	int	ppp_authretries;	/* How many times we can retry auth */

	/* IPv6CP state */
	struct	ppp_fsm ppp_ipv6cp;	/* IPv6 control protocol state */
	char ppp_us6[8];		/* Our interface token */
	char ppp_them6[8];		/* Their interface token */

	void	*ppp_session;		/* back to where we came from */

	/* Multilink Protocol state */
	int	ppp_mrru;		/* Like MRU, but for the whole bundle */
	int	ppp_mtru;		/* Like MTU, but for the whole bundle */
	struct	ppp_ed ppp_snd_ed;
	struct	ppp_ed ppp_rcv_ed;
	struct	ppp_auth ppp_rcv_auth;
	u_long	ppp_pifflags;		/* PIF specific flags */
} ppp_t;

/* FSM id numbers */
#define	FSM_LCP		0
#define	FSM_IPCP	1
#define	FSM_PAP		2
#define	FSM_CHAP	3
#define	FSM_IPV6CP	4

/* Option parsing result */
#define	OPT_OK		0
#define	OPT_REJ		1
#define	OPT_NAK		2
#define	OPT_FATAL	3

#define PPPMTU 1500		/* MTU for sync lines */

#define	PPP_ASYNC	0x80000000	/* async line */
#define	PPP_TOUT	0x40000000	/* timed out */
#define	PPP_TRACE	0x20000000	/* Enable protocol tracing output */
					/* this is a backward compat hack */

#define	PPP_USEPAP	0x10000000	/* Require other side to use PAP */
#define	PPP_USECHAP	0x08000000	/* Require other side to use CHAP */
#define	PPP_USEAUTH	0x18000000	/* Requrie other side to authenticate */

#define	PPP_NEEDPAP	0x04000000	/* Do not continue until PAP is done */
#define	PPP_NEEDCHAP	0x02000000	/* Do not continue until CHAP is done */
#define	PPP_NEEDAUTH	0x06000000	/* Do not continue until authorized */

#define	PPP_WANTPAP	0x01000000	/* Other side wants PAP */
#define	PPP_WANTCHAP	0x00800000	/* Other side wants CHAP */
#define	PPP_WANTAUTH	0x01800000	/* Other side wants authentication */

#define	PPP_CANPAP	0x00400000	/* We can do PAP */
#define	PPP_CANCHAP	0x00200000	/* We can do CHAP */
#define	PPP_CANAUTH	0x00600000	/* We can do authentication */

#define	PPP_CHGADDR	0x00100000	/* Always accept remote IPCP_ADDR */
#define	PPP_NODCD	0x00080000	/* We lost DCD */
#define	PPP_USEML	0x00040000	/* Wants to be a Multilink session */

#define	PPP_DEMAND	0x00020000	/* We are a demand dial connection */
#define	PPP_ISML	0x00010000	/* It is a Multilink session */

#define PPP_WANTV4	0x00008000	/* Want IPv4 connectivitiy (IPCP) */
#define PPP_WANTV6	0x00004000	/* Want IPv6 connectivitiy (IPv6CP) */

#define	ppp_auth_isup(ppp) \
	(((ppp)->ppp_flags & (PPP_NEEDAUTH|PPP_WANTAUTH)) == 0)

#define	ppp_auth_save(ppp, auth_info, len) {			\
	(ppp)->ppp_rcv_auth.auth_len =				\
	    (len > sizeof((ppp)->ppp_rcv_auth.auth_name)) ?	\
	    sizeof((ppp)->ppp_rcv_auth.auth_name) : len;	\
	memcpy((ppp)->ppp_rcv_auth.auth_name, auth_info,	\
	    (ppp)->ppp_rcv_auth.auth_len);			\
}

#define	ppp_auth_clear(ppp) {					\
	memset((ppp)->ppp_rcv_auth.auth_name, 0,		\
	    sizeof((ppp)->ppp_rcv_auth.auth_name));		\
	(ppp)->ppp_rcv_auth.auth_len = 0;			\
}
	

extern	int skt;
extern	int debug;

void	ppp_lcp_init(ppp_t *);
int	ppp_lcp_isup(ppp_t *);
void	ppp_lcp_creq(ppp_t *);
void	ppp_lcp_cnak(ppp_t *);
void	ppp_lcp_xcode(ppp_t *, pbuf_t *, ppp_cp_t *);
void	ppp_lcp_tlu(ppp_t *);
void	ppp_lcp_tld(ppp_t *);
void	ppp_lcp_tlf(ppp_t *);
void	ppp_lcp_optnak(ppp_t *, pppopt_t *);
void	ppp_lcp_optrej(ppp_t *, pppopt_t *);
int	ppp_lcp_opt(ppp_t *, pppopt_t *);
void	ppp_lcp_timeout(void *);
u_long	ppp_lcp_magic();

int	ppp_ml_init(ppp_t *);
int	ppp_ml_isup(ppp_t *);
int	ppp_ml_down(ppp_t *);
int	ppp_ml_wanted(ppp_t *);

void	ppp_pap_xcode(ppp_t *, pbuf_t *, ppp_cp_t *);

void	ppp_ipcp_init(ppp_t *);
int	ppp_ipcp_isup(ppp_t *);
int	ppp_ipcp_req(ppp_t *);
void	ppp_ipcp_creq(ppp_t *);
void	ppp_ipcp_tlu(ppp_t *);
void	ppp_ipcp_tlf(ppp_t *);
void	ppp_ipcp_optnak(ppp_t *, pppopt_t *);
void	ppp_ipcp_optrej(ppp_t *, pppopt_t *);
int	ppp_ipcp_opt(ppp_t *, pppopt_t *);
void	ppp_ipcp_timeout(void *);
u_long	ppp_ipcp_getaddr(ppp_t *, int);

int	ppp_pap_req(ppp_t *);
void	ppp_pap_xcode(ppp_t *, pbuf_t *, ppp_cp_t *);
void	ppp_pap_start(ppp_t *);

int	ppp_chap_req(ppp_t *);
void	ppp_chap_xcode(ppp_t *, pbuf_t *, ppp_cp_t *);
void	ppp_chap_start(ppp_t *);

struct sockaddr_in6;
void	ppp_ipv6cp_init(ppp_t *);
int	ppp_ipv6cp_isup(ppp_t *);
int	ppp_ipv6cp_req(ppp_t *);
void	ppp_ipv6cp_creq(ppp_t *);
void	ppp_ipv6cp_tlu(ppp_t *);
void	ppp_ipv6cp_tlf(ppp_t *);
void	ppp_ipv6cp_optnak(ppp_t *, pppopt_t *);
void	ppp_ipv6cp_optrej(ppp_t *, pppopt_t *);
int	ppp_ipv6cp_opt(ppp_t *, pppopt_t *);
void	ppp_ipv6cp_timeout(void *);
int	ppp_ipv6cp_getaddr(struct sockaddr_in6 *, ppp_t *, int);

pbuf_t	*ppp_cp_prepend(pbuf_t *, int, int, int, int);
pbuf_t	*ppp_addopt(pbuf_t *, pppopt_t *);
void	ppp_cp_in(ppp_t *, struct pbuf_t *);
void	ppp_cp_timeout(ppp_t *, int);
void	ppp_cp_up(ppp_t *, int);
void	ppp_cp_down(ppp_t *, int);
void	ppp_cp_close(ppp_t *, int);

void	dprintf(int, char *, ...);
void	uprintf(char *, ...);
void	sendpacket(ppp_t *, pbuf_t *);
int	process_packet(ppp_t *);

void	ppp_setstate(ppp_t *, long);
void	ppp_clrstate(ppp_t *, long);
long	ppp_getstate(ppp_t *);
void	ppp_setparam(ppp_t *, int);
void	ppp_ctl(ppp_t *, int);
void	ppp_bos(ppp_t *, int);
void	ppp_ifconfig(ppp_t *, int);
void	ppp_init(ppp_t *);
void	ppp_shutdown(ppp_t *);
void	ppp_modem(ppp_t *, int, int);
void	ppp_echorepl(ppp_t *, pbuf_t *, ppp_cp_t *);
int	pif_isup(ppp_t *);
void	ppp_up(ppp_t *);
void	ppp_down(ppp_t *);
int	ppp_dtr(ppp_t *, int);
int	ppp_getdcd(ppp_t *);
int	ppp_start(ppp_t *);
void	ppp_openskt();
void	ppp_cmsg(ppp_t *, pbuf_t *);
void	ppp_waitforeos(ppp_t *);
void	ppp_sendterm(ppp_t *);

int startup(session_t *, char *);
void tv_add(struct timeval *, struct timeval *);
int tv_cmp(struct timeval *, struct timeval *);
void tv_delta(struct timeval *, struct timeval *);
void disconnect(session_t *s);
void set_lparms(session_t *s);
void getpppcap(session_t *s);
session_t *getsyscap(char *, char*);
void getslcap(session_t *s);
int setlinktype(char *, int);
int setdebug(char *);


char	*ppp_authenticate(char *, char *);

#define	D_MAIN	0x00000001		/* main state level */
#define	D_PKT	0x00000002		/* packets */
#define	D_DT	(0x00000004 | D_PKT)	/* packet detail */
#define	D_STATE	0x00000008		/* state changes */
#define	D_TRC	0x00000010		/* dump of packets */
#define	D_DEBUG	0x00000020		/* internal debugging */
#define	D_XDEB	0x00000040		/* the -x debuging stuff */

/* Getauthcap return results */

#define	GETAUTHCAP_OK		0
#define	GETAUTHCAP_BADTC	1
#define	GETAUTHCAP_NOHOST	-1
#define	GETAUTHCAP_NODATA	-2
#define	GETAUTHCAP_TCLOOP	-3

void	timeout(void (*)(), ppp_t *, struct timeval *);
void	untimeout(void (*)(), ppp_t *);

#define	PAP_DEFAULT	"pap_default"	/* Default user to use for PAP */
#define	CHAP_DEFAULT	"chap_default"	/* Default user to use for CHAP */
#define	PAP_CLASS	"pap_default"	/* Class to try if user not in
					 * master.passwd file when using
					 * PAP authentication
					 */
#define	CHAP_CLASS	"chap_default"	/* Class to try for CHAP users */

/* For calls to ppp_ipcp_getaddr and ppp_ipcp_setaddr */
#define	DST_ADDR	1
#define	SRC_ADDR	0
