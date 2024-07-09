/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_pap.c,v 2.8 1998/04/09 21:32:54 chrisk Exp
 */

/*
 * PPP Password Authentication Protocol
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

static void ppp_pap_nakack(ppp_t *, int, char *, int);
static void ppp_pap_send(ppp_t *, pbuf_t *);
extern char *pap_authenticate(char *, char *, char *);
extern char *login_approve(char *, char *);

int 
ppp_pap_req(ppp)
	ppp_t *ppp;
{
	return(ppp_lcp_isup(ppp));
}

/*
 * Handle PAP packets
 */
void
ppp_pap_xcode(ppp, m, cp)
	ppp_t *ppp;
	pbuf_t *m;
	ppp_cp_t *cp;
{
	extern char *sysfile;
	extern char getauthcap();

	int idl;
	int pwl;
	char *id;
	char *style;
	char *pw;
	int err;
	char nakerrmsg[256];

	switch (cp->cp_code) {
	case PAP_REQUEST:
		dprintf(D_DT, "	{PAP REQUEST}\n");
		id = (char *)m->data;
		idl = *id++;
		dprintf(D_DT, "	{PEERLEN = %d}\n", idl);
		if (idl < 0 || idl + 2 > m->len) {
			dprintf(D_DT, "	{SHORT (%d)}\n", m->len);
			ppp_pap_nakack(ppp, cp->cp_id, "short packet", PAP_NAK);
			ppp_shutdown(ppp);
			break;
		}
		pw = id + idl;
		pwl = *pw++;
		dprintf(D_DT, "	{PASSLEN = %d}\n", pwl);
		if (pwl < 0 || pwl + idl + 2 > m->len) {
			dprintf(D_DT, "	{SHORT (%d)}", m->len);
			ppp_pap_nakack(ppp, cp->cp_id, "short packet", PAP_NAK);
			ppp_shutdown(ppp);
			break;
		}

		id[idl] = 0;
		/* See if the username has a : to indicate auth style */
		if ( (style = index(id,':')) ) {
			/* Split the name up into name and style */
			idl = style - id;
			/* Put a zero in for the : */
			*style++ = 0;
		}
		pw[pwl] = 0;
		ppp_auth_save(ppp, id, idl);
		dprintf(D_DT, "	{PEER/STYLE/PASS=%s/%s/%s}\n", id, style, pw);
		if ((ppp->ppp_flags & PPP_NEEDPAP) == 0) {
			syslog(LOG_INFO,
			    "%s: PAP authentication provided but not needed"
			    " (ignored)", id);
			dprintf(D_DT, "	{BLINDLY ACCEPT}\n");
			ppp_pap_nakack(ppp, cp->cp_id, "repeat", PAP_ACK);
		} else if ((pw = pap_authenticate(id, style, pw)) != NULL) {
                        syslog(LOG_INFO, "%s: failed PAP authentication", id);
			dprintf(D_DT, "	{%s}\n", pw);
			ppp_pap_nakack(ppp, cp->cp_id, pw, PAP_NAK);
			if (ppp->ppp_authretries-- == 0)
				ppp_shutdown(ppp);
                } else if ((pw = login_approve(id, PAP_DEFAULT)) != NULL) {
                        syslog(LOG_INFO, "%s: failed login approval", id);
                        dprintf(D_DT, " {%s}\n", pw);
                        ppp_pap_nakack(ppp, cp->cp_id, pw, PAP_NAK);
                        if (ppp->ppp_authretries-- == 0)
                                ppp_shutdown(ppp);
                } else if ((err = getauthcap(ppp, id, sysfile, nakerrmsg,
                            256)) == GETAUTHCAP_OK ||
                    (err == GETAUTHCAP_NOHOST && getauthcap(ppp, PAP_DEFAULT,
                            sysfile, NULL, 0 ) == GETAUTHCAP_OK)) {
			syslog(LOG_INFO, "%s: authenticated by PAP", id);
			dprintf(D_DT, "	{ACCEPTED}\n");
			free(ppp->ppp_sysname);
			ppp->ppp_sysname = strdup(id);
			ppp_pap_nakack(ppp, cp->cp_id, "okay", PAP_ACK);
			ppp->ppp_flags &= ~PPP_NEEDPAP;
		} else {
			syslog(LOG_INFO,
			    "%s: authenticated by PAP and approved but no ppp.sys entry",
			    id);
			dprintf(D_DT, "	{%s}\n", nakerrmsg);
			ppp_pap_nakack(ppp, cp->cp_id, nakerrmsg, PAP_NAK);
			if (ppp->ppp_authretries-- == 0)
				ppp_shutdown(ppp);
		}
		break;

	case PAP_ACK:
		dprintf(D_DT, "	{ASK msglen=%d}\n", m->len ? m->data[0] : 0);
		if (m->len > 0) {
			idl = m->data[0];
			if (idl && idl < m->len) {
				uprintf("PAP ACK message: ");
				for (pwl = 1; pwl <= idl; ++pwl)
					if (isprint(m->data[pwl]))
						uprintf("%c", m->data[pwl]);
					else
						uprintf("\\%03o", m->data[pwl]);
				uprintf("\n");
			}
		}
		untimeout(ppp_pap_start, ppp);
		ppp->ppp_flags &= ~PPP_WANTPAP;
		break;

	case PAP_NAK:
		dprintf(D_DT, "	{NAK msglen=%d}\n", m->len ? m->data[0] : 0);
		if (m->len > 0) {
			idl = m->data[0];
			if (idl && idl < m->len) {
				uprintf("PAP NAK message: ");
				for (pwl = 1; pwl <= idl; ++pwl)
					if (isprint(m->data[pwl]))
						uprintf("%c", m->data[pwl]);
					else
						uprintf("\\%03o", m->data[pwl]);
				uprintf("\n");
			}
		}
		untimeout(ppp_pap_start, ppp);
		ppp_shutdown(ppp);
		break;

	default:
		dprintf(D_DT, "	Unknown code %d\n", cp->cp_code);
		break;
	}
}

void
ppp_pap_start(ppp)
	ppp_t *ppp;
{
	ppp_header_t *ph;
	u_char *p;
	struct timeval tv;
	pbuf_t pb;

	pbuf_init(&pb);

	dprintf(D_PKT, "BUILD PAP REQUEST\n");

	pbuf_prepend(&pb, sizeof(ppp_header_t));

	ph = (ppp_header_t *)pb.data;
	ph->phdr_addr = PAP_REQUEST;
	ph->phdr_ctl = ppp->ppp_nid++;
	ph->phdr_type = htons(strlen(ppp->ppp_peerid) + strlen(ppp->ppp_passwd)
	    + 2 + sizeof(ppp_header_t));
	if (pbuf_append(&pb,ntohs(ph->phdr_type)-sizeof(ppp_header_t)) == NULL){
		dprintf(D_MAIN, "PAP user/password too long\n");
		ppp_shutdown(ppp);
		return;
	}
	p = (u_char *)(ph+1);

	*p++ = strlen(ppp->ppp_peerid);
	strcpy((char *)p, ppp->ppp_peerid);
	while (*p)
		++p;

	*p++ = strlen(ppp->ppp_passwd);
	strcpy((char *)p, ppp->ppp_passwd);

	ppp_pap_send(ppp, &pb);

	/*
	 * Send every 5 seconds until received
	 */
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	timeout(ppp_pap_start, ppp, &tv);
}

static void
ppp_pap_nakack(ppp, id, msg, code)
	ppp_t *ppp;
	int id;
	char *msg;
	int code;
{
	ppp_cp_t *cph;
	pbuf_t pb;

	pbuf_init(&pb);
	pbuf_append(&pb, sizeof(ppp_cp_t) + strlen(msg) + 1);

	cph = (ppp_cp_t *)(pb.data);;
	cph->cp_code = code;
	cph->cp_id = id;
	cph->cp_length = htons(strlen(msg) + sizeof(ppp_cp_t) + 1);
	*(u_char *)(cph+1) = strlen(msg);
	strncpy((char *)(cph+1) + 1, msg, *(u_char *)(cph+1));
	ppp_pap_send(ppp, &pb);
	if (code == PAP_NAK)
		ppp_auth_clear(ppp);
}

static void
ppp_pap_send(ppp, m)
	ppp_t *ppp;
	pbuf_t *m;
{
	ppp_header_t *ph;

	pbuf_prepend(m, sizeof(ppp_header_t));

	ph = (ppp_header_t *)m->data;
	ph->phdr_addr = PPP_ADDRESS;
	ph->phdr_ctl = PPP_CONTROL;
	ph->phdr_type = htons(PPP_PAP);
	sendpacket(ppp, m);
}
