/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1993, 1994, 1995, 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_util.c,v 2.16 2001/10/03 17:29:57 polk Exp
 */

/*
 * Small portions of this code are partially derived from CMU PPP.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_p2p.h>

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

int skt = -1;

#define	OK	((ppp->ppp_flags & (PPP_TOUT|PPP_NODCD)) == 0)

static void ppp_sendecho(ppp_t *ppp);
static void ppp_alarm(ppp_t *ppp);

void
ppp_init(ppp_t *ppp)
{
	/* Initialize parameters */
	ppp->ppp_txcmap = 0;
	ppp->ppp_rxcmap = 0;
	ppp->ppp_lcp.fsm_state = CP_STARTING;
	ppp->ppp_ipcp.fsm_state = CP_INITIAL;
	ppp->ppp_flags &= ~(PPP_WANTAUTH|PPP_NEEDAUTH|PPP_TOUT);
	if (ppp->ppp_flags & PPP_USEPAP)
		ppp->ppp_flags |= PPP_NEEDPAP;
	if (ppp->ppp_flags & PPP_USECHAP)
		ppp->ppp_flags |= PPP_NEEDCHAP;

	if (ppp->ppp_flags & PPP_ASYNC) {
		ppp->ppp_txcmap = 0xffffffff;
		ppp->ppp_rxcmap = 0xffffffff;
	}

	ppp->ppp_nid = 0;
	ppp->ppp_txflags = 0;
	ppp->ppp_magic = ppp_lcp_magic();
	ppp->ppp_badmagic = 0;
	ppp->ppp_ncps = 0;
	ppp->ppp_idletimesent = 0;	/* wait for first data packet to set */

	/* Calculate timer parameters */
	if (ppp->ppp_timeout.tv_sec < 0)
		ppp->ppp_timeout.tv_sec = 0;
	if (ppp->ppp_timeout.tv_usec < 10000)		/* 100th sec */
		ppp->ppp_timeout.tv_usec = 500000;	/* half sec */

	ppp->ppp_flags |= PPP_RUNNING;
	ppp_setparam(ppp, -1);
}

void
ppp_openskt()
{
	if (skt < 0 && (skt = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket(AF_INET, SOCK_DGRAM)");
}

int
ppp_start(ppp_t *ppp)
{
	int state, ret;
	int mlisup, index;
	int wantv4, wantv6;

	ppp_openskt();

	if (setlinktype(ppp->ppp_ifname, IFT_PPP) < 0)
		err(1, "setting linktype");

#define	SDL	ppp->ppp_dl

	SDL.sdl_family = AF_LINK;
	if ((index = if_nametoindex(ppp->ppp_ifname)) < 1)
		err(1, "if_nametoindex(%s)", ppp->ppp_ifname);
	SDL.sdl_index = index;
	SDL.sdl_len = sizeof(SDL) - sizeof(SDL.sdl_data);

	if (ppp->ppp_fd >= 0)
		close(ppp->ppp_fd);

	/* SDL.sdl_len = sizeof(SDL) - sizeof(SDL.sdl_data); */

	if ((ppp->ppp_fd = socket(AF_LINK, SOCK_RAW, SDL.sdl_index)) <0)
		err(1, "socket(AF_LINK, SOCK_RAW)");

	if (bind(ppp->ppp_fd, (struct sockaddr*)&SDL, sizeof(SDL)))
		err(1, "bind");
#undef	SDL

	ppp->ppp_flags &= ~(PPP_TOUT|PPP_NODCD);

	ppp_init(ppp);
	if (ppp_dtr(ppp, 1) < 0) {
		dprintf(D_MAIN, "Raising DTR on interface: %s\n",
		    strerror(errno));
		err(1, "Raising DTR on interface");
	}

	if (ppp_getdcd(ppp) == 0) {
		dprintf(D_MAIN, "WAITING FOR CARRIER\n");
		while (ppp_getdcd(ppp) == 0)
			process_packet(ppp);
	}

	dprintf(D_MAIN, "WAITING FOR LCP\n");
	timeout(ppp_alarm, ppp, &ppp->ppp_bostimeout);
	ppp_lcp_init(ppp);

	wantv4 = wantv6 = 0;
	switch (ppp->ppp_flags & (PPP_WANTV4 | PPP_WANTV6)) {
	case 0:
		wantv4 = 1;	/*quickhack*/
		break;
	case PPP_WANTV4:
		wantv4 = 1;
		break;
	case PPP_WANTV6:
		wantv6 = 1;
		break;
	case PPP_WANTV4 | PPP_WANTV6:
		wantv4 = wantv6 = 1;
		break;
	}

	wantv4 = wantv6 = 0;
	switch (ppp->ppp_flags & (PPP_WANTV4 | PPP_WANTV6)) {
	case 0:
		wantv4 = 1;	/*quickhack*/
		break;
	case PPP_WANTV4:
		wantv4 = 1;
		break;
	case PPP_WANTV6:
		wantv6 = 1;
		break;
	case PPP_WANTV4 | PPP_WANTV6:
		wantv4 = wantv6 = 1;
		break;
	}

	state = 0;
	mlisup = 0;
	process_packet(ppp);
	while (OK) {
		switch (state) {
		case 0:
			if (ppp_lcp_isup(ppp)) {
				state = 1;
				mlisup = ppp_ml_isup(ppp);
				continue;
			}
			break;
		case 1:
			if (ppp->ppp_flags & PPP_NEEDCHAP)
				ppp_chap_start(ppp);
			if (ppp->ppp_flags & PPP_WANTPAP)
				ppp_pap_start(ppp);
			dprintf(D_MAIN, "WAITING FOR AUTH\n");
			state = 2;
			continue;
		case 2:
			if (ppp_lcp_isup(ppp) == 0) {
				dprintf(D_MAIN, "WAITING FOR LCP (again)\n");
				state = 0;
				break;
			}
			if (ppp_auth_isup(ppp)) {
				/*
				 * PAP/CHAP might have changed ppp_sysname,
				 * set the session sysname to that value here.
				 */
				((session_t *)(ppp->ppp_session))->sysname =
				    ppp->ppp_sysname;
				if (!mlisup && ppp_ml_wanted(ppp)) {
					dprintf(D_MAIN, "RESTARTING LCP TO "
					    "ENABLE MULTILINK\n");
					ppp_lcp_tld(ppp);
					ppp_lcp_init(ppp);
					state = 0;
					break;
				}
				state = 3;
				ret = ppp_ml_init(ppp);
				if (ret == 1)
					goto out;
				if (ret < 0)
					goto out;
				if ((ppp->ppp_flags & PPP_DEMAND) == 0 &&
				    startup(ppp->ppp_session, ppp->ppp_sysname))
					goto out;
				if (wantv4)
					ppp_ipcp_init(ppp);
				if (wantv6)
					ppp_ipv6cp_init(ppp);
				dprintf(D_MAIN, "WAITING FOR IPCP\n");
			}
			break;
		case 3:
			if (ppp_lcp_isup(ppp) == 0) {
				dprintf(D_MAIN, "WAITING FOR LCP (again)\n");
				state = 0;
				break;
			}
			if ((!wantv4 || ppp_ipcp_isup(ppp))
			 && (!wantv6 || ppp_ipv6cp_isup(ppp)))
				goto out;
		}
		process_packet(ppp);
	}
out:
	untimeout(ppp_alarm, ppp);

	if (ppp->ppp_flags & PPP_NODCD)
		errno = ENETDOWN;
	else if (ppp->ppp_flags & PPP_TOUT)
		errno = ETIMEDOUT;
	else if (errno == 0) {
		if (!ppp_auth_isup(ppp))
			errno = EACCES;
		else if (!ppp_ml_isup(ppp) && !ppp_ipcp_isup(ppp))
			errno = ENOPROTOOPT;
		else if (!ppp_lcp_isup(ppp))
			errno = ENOPROTOOPT;
	}
	return (OK &&
	    (ppp_ml_isup(ppp) || ppp_ipcp_isup(ppp)) && ppp_lcp_isup(ppp));
}

void
ppp_waitforeos(ppp_t *ppp)
{
	/*
	 * If we are running multilink, and we didn't bring up
	 * IPCP, then we still need to bring up the interface.
	 */
	if (ppp_ml_isup(ppp) && !ppp_ipcp_isup(ppp))
		ppp_up(ppp);

	dprintf(D_MAIN, "WAITING FOR EOS\n");

	if ((ppp->ppp_idletimesent = ppp->ppp_idletime.tv_sec) != NULL)
		ppp_setparam(ppp, PPP_SETIDLE);

	if (ppp->ppp_echofreq.tv_sec || ppp->ppp_echofreq.tv_usec) {
		/*
		 * by default we have to loose 3 echo packets to echo out
		 */
		if (!(ppp->ppp_echowait.tv_sec || ppp->ppp_echowait.tv_usec)) {
			ppp->ppp_echowait = ppp->ppp_echofreq;
			tv_add(&ppp->ppp_echowait, &ppp->ppp_echofreq);
			tv_add(&ppp->ppp_echowait, &ppp->ppp_echofreq);
		}
		ppp_echorepl(ppp, 0, 0);
		ppp_sendecho(ppp);
	}

	/*
	 * XXX - it probably is okay to stay up as long as lcp is
	 * connected.  We still wait of ipcp as otherwise we don't
	 * know when to configure out interfaces.
	 */
	while (OK && (ppp_ml_isup(ppp) || ppp_ipcp_isup(ppp)))
		process_packet(ppp);

	if (ppp->ppp_flags & PPP_TOUT) {
		syslog(LOG_INFO, "%s: session timed out\n",ppp->ppp_sysname);
		dprintf(D_MAIN, "Session timed out\n");
		}
	else if (ppp->ppp_flags & PPP_NODCD) {
		syslog(LOG_INFO, "%s: session lost carrier\n",ppp->ppp_sysname);
		dprintf(D_MAIN, "Session lost carrier\n");
		}
	if (!ppp_ipcp_isup(ppp)) {
		syslog(LOG_INFO, "%s: IPCP went down\n",ppp->ppp_sysname);
		dprintf(D_MAIN, "IPCP is no longer up\n");
		}
	if (!ppp_lcp_isup(ppp)) {
		syslog(LOG_INFO, "%s: LCP  went down\n",ppp->ppp_sysname);
		dprintf(D_MAIN, "LCP is no longer up\n");
		}

	if (ppp_ml_isup(ppp)) {
		ppp_ml_down(ppp);
		ppp_down(ppp);
	}
}

void
ppp_up(ppp_t *ppp)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	dprintf(D_STATE, "Begining of session\n");
	ppp_ifconfig(ppp, 1);			/* mark us as up */

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = PPP_ADDF;
	pio->ppp_flags = PPP_IPOKAY;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_up");

	pio->ppp_control = PPP_CLRF;
	pio->ppp_flags = PPP_IPREJECT;
	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_up (clearing REJECT)");

	ppp->ppp_flags |= PPP_IPOKAY;
	ppp->ppp_flags &= ~PPP_IPREJECT;
}

void
ppp_down(ppp_t *ppp)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	dprintf(D_STATE, "End of session\n");

	if (!(ppp->ppp_flags & PPP_DEMAND))
		ppp_ifconfig(ppp, 0);		/* Mark us down */

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = PPP_CLRF;
	pio->ppp_flags = PPP_IPOKAY | PPP_RUNNING;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_down");

	if (!(ppp->ppp_flags & PPP_DEMAND)) {
		dprintf(D_STATE, "Rejecting IP packets\n");
		pio->ppp_control = PPP_SETF;
		pio->ppp_flags = PPP_IPREJECT;

		if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
			warn("ppp_down");

		ppp->ppp_flags &= ~PPP_IPOKAY;
		ppp->ppp_flags |= PPP_IPREJECT;
	}

	untimeout(0, ppp);
}

void
ppp_shutdown(ppp_t *ppp)
{
	ppp->ppp_termtries = 0;

	ppp_cp_close(ppp, FSM_IPCP);
	ppp_cp_close(ppp, FSM_LCP);

	/*
	 * Wait for us to exit the closing state
	 */
	while (ppp->ppp_lcp.fsm_state == CP_CLOSING)
		process_packet(ppp);

	ppp_down(ppp);
}

void
ppp_setstate(ppp_t *ppp, long f)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	dprintf(D_STATE, "Set state %x\n", f);

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = PPP_ADDF;
	pio->ppp_flags = f;
	ppp->ppp_flags |= f;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_setstate");
}

void
ppp_clrstate(ppp_t *ppp, long f)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	dprintf(D_STATE, "Clr state %x\n", f);

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = PPP_CLRF;
	pio->ppp_flags = f;
	ppp->ppp_flags &= ~f;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_clrstate");
}

void
ppp_ctl(ppp_t *ppp, int c)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = c;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_ctl(%x)", c);
}

void
ppp_setparam(ppp_t *ppp, int ctl)
{
	struct ifreq ifr;
	struct ppp_ioctl *pio = ifr_pppioctl(&ifr);

	if (skt < 0)
		return;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof (ifr.ifr_name));

	pio->ppp_control = ctl;
	pio->ppp_control &= PPP_SETF|PPP_SETTX|PPP_SETRX|PPP_SETIDLE|PPP_SETMTU;
	pio->ppp_txcmap = ppp->ppp_txcmap;
	pio->ppp_rxcmap = ppp->ppp_rxcmap;
	pio->ppp_idletime = ppp->ppp_idletimesent;
	pio->ppp_mtu = ppp->ppp_mru;
	pio->ppp_flags = (u_short)ppp->ppp_flags & ~(PPP_PFC|PPP_ACFC|PPP_TCPC);
	pio->ppp_flags |= (u_short)ppp->ppp_txflags;

	if (ioctl(skt, PPPIOCSSTATE, (caddr_t) &ifr) < 0)
		warn("ppp_setparam");
}

int
ppp_dtr(ppp_t *ppp, int flag)
{
	int e;

	if (ppp->ppp_fd < 0)
		return(-1);

	flag = flag ? 1 : 0;

	dprintf(D_MAIN, "%sing DTR\n", flag ? "sett" : "drop");

	/*
	 * ptys return ENOTTY so simply don't consider ENOTTY an error
	 */
	errno = 0;
	e = setsockopt(ppp->ppp_fd, IFT_NONE, P2P_DTR, &flag,sizeof(flag));
	return (e < 0 && errno != ENOTTY ? e : 0);
}

int
ppp_getdcd(ppp_t *ppp)
{
	int e;
	int state;
	int len = sizeof(state);

	if (ppp->ppp_fd < 0)
		return(0);

	/*
	 * ptys return ENOTTY so simply don't consider ENOTTY an error
	 */
	errno = 0;
	e = getsockopt(ppp->ppp_fd, IFT_NONE, P2P_DCD, &state, &len);
	if (e < 0) {
		if (errno != ENOTTY)
			err(1, "P2P_DCD: should not fail");
		else
			state = TIOCM_CAR;
	}

	state &= TIOCM_CAR;

	dprintf(D_MAIN, "Getting DCD state: %s\n", state ? "ON" : "OFF");
	return (state ? 1 : 0);
}

/*
 * Modem events handling routine
 * (0 - DCD dropped; 1 - DCD raised)
 */
void
ppp_dcd(ppp_t *ppp, int flag)
{
	/*
	 * Got DCD -- send out Config-Request
	 */
	if (flag) {
		dprintf(D_MAIN, "GOT CARRIER DETECT\n");
		if (ppp->ppp_lcp.fsm_state == CP_INITIAL) {
			ppp->ppp_lcp.fsm_state = CP_CLOSED;
			return;
		}
		if (ppp->ppp_lcp.fsm_state != CP_STARTING)
			return;

		dprintf(D_STATE, "Initialize PPP session\n");

		ppp_init(ppp);
		ppp_lcp_init(ppp);
	} else {
		dprintf(D_MAIN, "LOST CARRIER DETECT\n");
		/*
		 * Carrier lost -- reinitialize everything.
		 */
		if ((ppp->ppp_flags & PPP_DEMAND) == 0)
			ppp_ctl(ppp, PPP_FLUSHQ | PPP_FLUSHDQ);

		ppp_clrstate(ppp, PPP_RUNNING);
		ppp_clrstate(ppp, PPP_IPOKAY);
		ppp_init(ppp);
		ppp->ppp_flags |= PPP_NODCD;
	}
}

/*
 * Called when the interface has changed state.
 * If the interface comes UP, we just ignore it.
 * If the interface goes DOWN or loses RUNNING and we are done with IPCP
 * then shutdown the connection.
 */
void
ppp_ifstate(ppp_t *ppp, int flags)
{
	dprintf(D_MAIN, "Interface change to %s\n",
	    (flags & IFF_UP) ? (flags & IFF_RUNNING) ?
	    "up and running" : "up and not running" : "down");

	if ((flags & (IFF_UP|IFF_RUNNING)) == (IFF_UP|IFF_RUNNING))
		;
	else if (ppp_lcp_isup(ppp))
		ppp_shutdown(ppp);
}

static void
ppp_sendecho(ppp_t *ppp)
{
	pbuf_t pb;

	/*
	 * Send echo packet
	 */
	pbuf_init(&pb);
	ppp_cp_prepend(&pb, PPP_LCP, LCP_ECHO_REQ, ppp->ppp_nid++, 4);
	*(long *)(pb.data + pb.len - 4) = htonl(ppp->ppp_magic);
	sendpacket(ppp, &pb);

	/*
	 * Set up to send another one
	 */
	timeout(ppp_sendecho, ppp, &ppp->ppp_echofreq);
}

void
ppp_echorepl(ppp_t *ppp, pbuf_t *m, ppp_cp_t *cp)
{
	m = NULL; cp = NULL;	/* get -Wall to shut up */

	timeout(ppp_shutdown, ppp, &ppp->ppp_echowait);
}

void
ppp_sendterm(ppp_t *ppp)
{
	pbuf_t *m;

	dprintf(D_MAIN, "Sending term request\n");
	m = ppp_cp_prepend(0, PPP_LCP, CP_TERM_REQ, 0, 0);
	sendpacket(ppp, m);
	pbuf_free(m);
	if (ppp->ppp_termtries++ < ppp->ppp_maxterm)
		timeout(ppp_sendterm, ppp, &ppp->ppp_timeout);
	else {
		m = ppp_cp_prepend(0, PPP_LCP, CP_TERM_ACK, 0, 0);
		ppp_cp_in(ppp, m);
		pbuf_free(m);
	}
}

void
ppp_ifconfig(ppp_t *ppp, int flag)
{
	struct ifreq ifr;

	if (skt < 0)
		return;
	/*
	 * Read flags and if IFF_UP is not right, fix it
	 */             
	strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof(ifr.ifr_name));
	if (ioctl(skt, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		err(1, "could not read interface flags");

	if (flag)
		flag = IFF_UP;
	   
	if ((ifr.ifr_flags & IFF_UP) != flag) {  
		strncpy(ifr.ifr_name, ppp->ppp_ifname, sizeof(ifr.ifr_name));
		ifr.ifr_flags ^= IFF_UP;
		if (ioctl(skt, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
			err(1, "could not set interface flags");
	}
}

int
pif_isup(ppp_t *ppp)
{
	struct ifreq ifr;

	if (skt < 0 || ppp->ppp_pifname[0] == '\0')
		return(0);
	strncpy(ifr.ifr_name, ppp->ppp_pifname, sizeof(ifr.ifr_name));
	if (ioctl(skt, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		return(0);
	return (ifr.ifr_flags & IFF_UP);
}

void
ppp_cmsg(ppp_t *ppp, pbuf_t *m)
{
	struct cmsghdr *cmsg = (struct cmsghdr *)m->data;

	if (cmsg->cmsg_level != IFT_NONE) {
		dprintf(D_DEBUG, "Control message of level %d received\n",
		    cmsg->cmsg_level);
		return;
	}

	switch (cmsg->cmsg_type) {
	case IFS_FLAGS:
		if (cmsg->cmsg_len != sizeof(u_short) + sizeof(struct cmsghdr))
			dprintf(D_DEBUG,
			    "invalid length for if_flags control message\n");
		else
			ppp_ifstate(ppp, *(u_short*)CMSG_DATA(cmsg));
		break;
	case IFS_DCD:
		if (cmsg->cmsg_len != sizeof(int) + sizeof(struct cmsghdr))
			dprintf(D_DEBUG,
			    "invalid length for DCD control message\n");
		else
			ppp_dcd(ppp, *(int*)CMSG_DATA(cmsg));
		break;
	default:
		dprintf(D_DEBUG, "Control message of type %d received\n",
		    cmsg->cmsg_type);
		break;
	}
}

static void ppp_alarm(ppp_t *ppp)
{   
	ppp->ppp_flags |= PPP_TOUT;
}
