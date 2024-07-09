/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_chap.c,v 2.9 1999/10/07 21:44:21 chrisk Exp
 */

/*
 * PPP Challange-Handshake Authentication Protocol
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

static void ppp_chap_nakack(ppp_t *, int, char *, int);
static void ppp_chap_send(ppp_t *, pbuf_t *);
static void ppp_chap_send_response(ppp_t *, int, pbuf_t *);

extern char *chap_challange(ppp_t *, int, pbuf_t *);
extern char *chap_response(ppp_t *, char *, int, u_char *, int, pbuf_t *);
extern char *chap_check(ppp_t *, char *, int, u_char *, int);
extern char *login_approve(char *, char *);

int 
ppp_chap_req(ppp)
	ppp_t *ppp;
{
	return(ppp_lcp_isup(ppp));
}

/*
 * Handle CHAP packets
 */
void
ppp_chap_xcode(ppp, m, cp)
	ppp_t *ppp;
	pbuf_t *m;
	ppp_cp_t *cp;
{
	extern char *sysfile;
	extern char getauthcap();

	pbuf_t r;
	int idl;
	int pwl;
	char *id;
	char *pw;
	int err;
	char nakerrmsg[256];

	switch (cp->cp_code) {
	case CHAP_CHALLANGE:
		dprintf(D_DT, "	{CHAP_CHALLANGE REQUEST}\n");
		pw = (char *)m->data;
		pwl = *(unsigned char *)pw++;
		dprintf(D_DT, "	{CHALLANGE = %d}\n", pwl);
		if (pwl < 0 || pwl + 2 > m->len) {
			dprintf(D_DT, "	{SHORT (%d)}\n", m->len);
			ppp_chap_nakack(ppp, cp->cp_id, "short packet",
			    CHAP_RESPONSE);
			break;
		}
		id = pw + pwl;
		idl = m->len - pwl - 1;
		id[idl] = 0;
		ppp_auth_save(ppp, id, idl);
		dprintf(D_DT, "	{NAME = %d:%s}\n", idl, id);
		if (idl < 0) {
			dprintf(D_DT, "	{SHORT (%d)}\n", m->len);
			ppp_chap_nakack(ppp, cp->cp_id, "short packet",
			    CHAP_RESPONSE);
			break;
		}

		pbuf_init(&r);

		if ((ppp->ppp_flags & PPP_CANCHAP) == 0) {
			dprintf(D_DT, "	{CANT CHAP}\n");
			ppp_chap_nakack(ppp, cp->cp_id, "chap not supported",
			    CHAP_RESPONSE);
		} else if ((pw = chap_response(ppp, id, cp->cp_id,
		    (u_char *)pw, pwl, &r)) != NULL) {
			dprintf(D_DT, "	{%s}\n", pw);
			ppp_chap_nakack(ppp, cp->cp_id, pw, CHAP_RESPONSE);
		} else
			ppp_chap_send_response(ppp, cp->cp_id, &r);
		break;

	case CHAP_RESPONSE:
		dprintf(D_DT, "	{CHAP_RESPONSE }\n");
		pw = (char *)m->data;
		pwl = *(unsigned char *)pw++;
		dprintf(D_DT, "	{RESPONSE = %d}\n", pwl);
		if (pwl < 0 || pwl + 2 > m->len) {
			dprintf(D_DT, "	{SHORT (%d)}\n", m->len);
			ppp_chap_nakack(ppp, cp->cp_id, "short packet",
			    CHAP_NAK);
			break;
		}
		id = pw + pwl;
		idl = m->len - pwl - 1;
		id[idl] = 0;
		dprintf(D_DT, "	{NAME = %d:%s}\n", idl, id);
		if (idl < 0) {
			ppp_chap_nakack(ppp, cp->cp_id, "short packet",
			    CHAP_NAK);
			break;
		}

		pbuf_init(&r);

		if ((pw = chap_check(ppp, id, cp->cp_id, (u_char *)pw, pwl))
		    != NULL) {
			syslog(LOG_INFO, "%s: failed CHAP authentication", id);
			dprintf(D_DT, "	{%s}\n", pw);
			ppp_chap_nakack(ppp, cp->cp_id, pw, CHAP_NAK);
		} else if ((pw = login_approve(id, CHAP_CLASS)) != NULL) {
			syslog(LOG_INFO, "%s: failed login approval", id);
			dprintf(D_DT, " {%s}\n", pw);
			ppp_chap_nakack(ppp, cp->cp_id, pw, CHAP_NAK);
		} else if ((err = getauthcap(ppp, id, sysfile, nakerrmsg,
			    256)) == GETAUTHCAP_OK ||
		    (err == GETAUTHCAP_NOHOST && getauthcap(ppp, CHAP_DEFAULT,
			    sysfile, NULL, 0 ) == GETAUTHCAP_OK)) {
			syslog(LOG_INFO, "%s: authenticated by CHAP", id);
			free(ppp->ppp_sysname);
			ppp->ppp_sysname = strdup(id);
			dprintf(D_DT, "	{verified}\n", pw);
			ppp_chap_nakack(ppp, cp->cp_id, "verified", CHAP_ACK);
			ppp->ppp_flags &= ~PPP_NEEDCHAP;
			untimeout(ppp_chap_start, ppp);
		} else {
			syslog(LOG_INFO,
			    "%s: authenticated by CHAP but no ppp.sys entry",
			    id);
			dprintf(D_DT, "	{%s}\n", nakerrmsg);
			ppp_chap_nakack(ppp, cp->cp_id, nakerrmsg, CHAP_NAK);
		}
		break;

	case CHAP_ACK:
		dprintf(D_DT, "	{ACK msglen=%d}\n", m->len);
		if (m->len > 0) {
			uprintf("CHAP ACK message: ");
			for (pwl = 0; pwl < m->len ; ++pwl)
				if (isprint(m->data[pwl]))
					uprintf("%c", m->data[pwl]);
				else
					uprintf("\\%03o", m->data[pwl]);
			uprintf("\n");
		}
		ppp->ppp_flags &= ~PPP_WANTCHAP;
		break;

	case CHAP_NAK:
		dprintf(D_DT, "	{NAK msglen=%d}\n", m->len);
		if (m->len > 0) {
			uprintf("CHAP NAK message: ");
			for (pwl = 0; pwl < m->len ; ++pwl)
				if (isprint(m->data[pwl]))
					uprintf("%c", m->data[pwl]);
				else
					uprintf("\\%03o", m->data[pwl]);
			uprintf("\n");
		}
		ppp_auth_clear(ppp);
		/*
		 * try again right away (if we have a limit)
		 */
		if (ppp->ppp_authretries >= 0)
			ppp_chap_start(ppp);
		break;

	default:
		dprintf(D_DT, "	Unknown code %d\n", cp->cp_code);
		break;
	}
}

void
ppp_chap_start(ppp)
	ppp_t *ppp;
{
	u_char *cp;
	struct timeval tv;
	pbuf_t pb;

	pbuf_init(&pb);

	dprintf(D_PKT, "BUILD CHAP CHALLANGE (%d)\n", ppp->ppp_authretries);
	if (ppp->ppp_authretries-- == 0) {
		ppp_shutdown(ppp);
		return;
	}

	cp = pbuf_prepend(&pb, sizeof(ppp_header_t));

	*cp++ = CHAP_CHALLANGE;
	*cp++ = ppp->ppp_nid++;
	pbuf_append(&pb, 1);

	chap_challange(ppp, ppp->ppp_nid - 1, &pb);
	cp[2] = pb.len - sizeof(ppp_header_t) - 1;

	dprintf(D_DT, "	{RESPONSE LEN = %d}\n", cp[2]);
	dprintf(D_DT, "	{NAME = %s}\n", ppp->ppp_chapname);

	strcpy((char *)pbuf_append(&pb, strlen(ppp->ppp_chapname)),
	    ppp->ppp_chapname);

	*cp++ = (pb.len >> 8) & 0xff;
	*cp++ = pb.len & 0xff;

	ppp_chap_send(ppp, &pb);

	/*
	 * Send every 5 seconds until received
	 */
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	timeout(ppp_chap_start, ppp, &tv);
}

static void
ppp_chap_send_response(ppp_t *ppp, int id, pbuf_t *m)
{
	u_char *cp;
	u_char *p;
	int len;

	dprintf(D_PKT, "BUILD CHAP RESPONSE\n");

	len = m->len;
	cp = pbuf_prepend(m, sizeof(ppp_cp_t) + 1);
	*cp++ = CHAP_RESPONSE;
	*cp++ = id;
	cp[2] = len;

	dprintf(D_DT, "	{RESPONSE LEN = %d}\n", len);
	dprintf(D_DT, "	{NAME = %s}\n", ppp->ppp_chapname);
	p = pbuf_append(m, strlen(ppp->ppp_chapname));
	strcpy((char *)p, ppp->ppp_chapname);

	len = m->len;
	*cp++ = (len >> 8) & 0xff;
	*cp++ = len & 0xff;
	ppp_chap_send(ppp, m);
}

static void
ppp_chap_nakack(ppp, id, msg, code)
	ppp_t *ppp;
	int id;
	char *msg;
	int code;
{
	u_char *p;
	pbuf_t pb;

	pbuf_init(&pb);
	pbuf_prepend(&pb, sizeof(ppp_cp_t) + strlen(msg));

	if (code == CHAP_RESPONSE) {
		p = pbuf_append(&pb, strlen(ppp->ppp_chapname) + 1);
		strcpy((char *)p + 1, ppp->ppp_chapname);
	}

	p = pb.data;
	*p++ = code;
	*p++ = id;
	*p++ = (pb.len >> 8) & 0xff;
	*p++ = pb.len & 0xff;
	if (code == CHAP_RESPONSE)
		*p++ = strlen(msg);
	strncpy((char *)p, msg, p[-1]);
	ppp_chap_send(ppp, &pb);
}

static void
ppp_chap_send(ppp, m)
	ppp_t *ppp;
	pbuf_t *m;
{
	u_char *p;

	p = pbuf_prepend(m, sizeof(ppp_header_t));

	*p++ = PPP_ADDRESS;
	*p++ = PPP_CONTROL;
	*p++ = (PPP_CHAP >> 8) & 0xff;
	*p++ = PPP_CHAP & 0xff;
	sendpacket(ppp, m);
}
