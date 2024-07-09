/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1996-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *	@(#) atmarp_timer.c,v 1.2 1998/08/13 20:11:12 johnc Exp
 *
 */

/*
 * Server Cache Synchronization Protocol (SCSP) Support
 * ----------------------------------------------------
 *
 * SCSP-ATMARP server interface: timer routines
 *
 */

#ifndef lint
static char *RCSid = "@(#) atmarp_timer.c,v 1.2 1998/08/13 20:11:12 johnc Exp";
#endif

#include <netatm/user_include.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/cdefs.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>

#include <arpa/inet.h>
#include <netinet/if_ether.h>

#include "../lib/libatm.h"
#include "../scspd/scsp_msg.h"
#include "../scspd/scsp_if.h"
#include "../scspd/scsp_var.h"
#include "atmarp_var.h"


/*
 * Cache update timeout processing
 *
 * When the cache update timer fires, we read the cache from the
 * kernel, update the internal cache, and restart the timer.
 *
 * Arguments:
 *	tp	pointer to a HARP timer block
 *
 * Returns:
 *	None
 *
 */
void
atmarp_cache_timeout(tp)
	Harp_timer	*tp;
{
	Atmarp_intf	*aip;

	/*
	 * Verify the status of all configured interfaces
	 */
	for (aip = atmarp_intf_head; aip; aip = aip->ai_next) {
		if (atmarp_if_ready(aip)) {
			/*
			 * The interface is up but we don't have
			 * a connection to SCSP--make a connection
			 */
			if (aip->ai_state == AI_STATE_NULL)
				(void)atmarp_scsp_connect(aip);
		} else {
			/*
			 * The interface is down--disconnect from SCSP
			 */
			if (aip->ai_state != AI_STATE_NULL)
				(void)atmarp_scsp_disconnect(aip);
		}
	}

	/*
	 * Read the cache from the kernel
	 */
	atmarp_get_updated_cache();

	/*
	 * Restart the cache update timer
	 */
	HARP_TIMER(tp, ATARP_CACHE_INTERVAL, atmarp_cache_timeout);
}


/*
 * Permanent cache entry timer processing
 *
 * Permanent cache entries (entries that are administratively added
 * and the entry for the server itself) don't ever get refreshed, so
 * we broadcast updates for them every 10 minutes so they won't get
 * deleted from the remote servers' caches
 *
 * Arguments:
 *	tp	pointer to a HARP timer block
 *
 * Returns:
 *	None
 *
 */
void
atmarp_perm_timeout(tp)
	Harp_timer	*tp;
{
	int		i, rc;
	Atmarp_intf	*aip;
	Atmarp		*aap;

	/*
	 * Loop through all interfaces
	 */
	for (aip = atmarp_intf_head; aip; aip = aip->ai_next) {
		/*
		 * Loop through the interface's cache
		 */
		for (i = 0; i < ATMARP_HASHSIZ; i++) {
			for (aap = aip->ai_arptbl[i]; aap;
					aap = aap->aa_next) {
				/*
				 * Find and update permanent entries
				 */
				if ((aap->aa_flags & (AAF_PERM |
						AAF_SERVER)) != 0) {
					aap->aa_seq++;
					rc = atmarp_scsp_update(aap,
						SCSP_ASTATE_UPD);
				}
			}
		}
	}

	/*
	 * Restart the permanent cache entry timer
	 */
	HARP_TIMER(tp, ATARP_PERM_INTERVAL, atmarp_perm_timeout);
}


/*
 * Keepalive timeout processing
 *
 * When the keepalive timer fires, we send a NOP to SCSP.  This
 * will help us detect a broken connection.
 *
 * Arguments:
 *	tp	pointer to a HARP timer block
 *
 * Returns:
 *	None
 *
 */
void
atmarp_keepalive_timeout(tp)
	Harp_timer	*tp;
{
	Atmarp_intf	*aip;
	Scsp_if_msg	*msg;

	/*
	 * Back off to start of DCS entry
	 */
	aip = (Atmarp_intf *) ((caddr_t)tp -
			(int)(&((Atmarp_intf *)0)->ai_keepalive_t));

	/*
	 * Get a message buffer
	 */
	msg = (Scsp_if_msg *)UM_ALLOC(sizeof(Scsp_if_msg));
	if (!msg) {
	}
	UM_ZERO(msg, sizeof(Scsp_if_msg));

	/*
	 * Build a NOP message
	 */
	msg->si_type = SCSP_NOP_REQ;
	msg->si_proto = SCSP_PROTO_ATMARP;
	msg->si_len = sizeof(Scsp_if_msg_hdr);

	/*
	 * Send the message to SCSP
	 */
	(void)atmarp_scsp_out(aip, (char *)msg, msg->si_len);
	UM_FREE(msg);

	/*
	 * Restart the keepalive timer
	 */
	HARP_TIMER(&aip->ai_keepalive_t, ATARP_KEEPALIVE_INTERVAL,
			atmarp_keepalive_timeout);
}
