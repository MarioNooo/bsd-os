/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1997-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *	@(#) scsp_timer.c,v 1.2 1998/07/16 15:59:50 johnc Exp
 *
 */

/*
 * Server Cache Synchronization Protocol (SCSP) Support
 * ----------------------------------------------------
 *
 * Timer processing
 *
 */

#ifndef lint
static char *RCSid = "@(#) scsp_timer.c,v 1.2 1998/07/16 15:59:50 johnc Exp";
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

#include "scsp_msg.h"
#include "scsp_if.h"
#include "scsp_var.h"


/*
 * Process an SCSP Open timeout
 *
 * The open timer is set when an attempt to open a VCC to a DCS fails.
 * This routine will be called when the timer fires and will retry
 * the open.  Retries can continue indefinitely.
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_open_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of DCS entry
	 */
	dcsp = (Scsp_dcs *) ((caddr_t)stp -
			(int)(&((Scsp_dcs *)0)->sd_open_t));

	/*
	 * Retry the connection
	 */
	if (scsp_dcs_connect(dcsp)) {
		/*
		 * Connect failed -- we hope the error was temporary
		 * and set the timer to try again later
		 */
		HARP_TIMER(&dcsp->sd_open_t, SCSP_Open_Interval,
				scsp_open_timeout);
	}
}


/*
 * Process an SCSP Hello timeout
 *
 * The Hello timer fires every SCSP_HELLO_Interval seconds.  This
 * routine will notify the Hello FSM when the timer fires.
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_hello_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of DCS entry
	 */
	dcsp = (Scsp_dcs *) ((caddr_t)stp -
			(int)(&((Scsp_dcs *)0)->sd_hello_h_t));

	/*
	 * Call the Hello FSM
	 */
	(void)scsp_hfsm(dcsp, SCSP_HFSM_HELLO_T, (Scsp_msg *)0);

	return;
}


/*
 * Process an SCSP receive timeout
 *
 * The receive timer is started whenever the Hello FSM receives a
 * Hello message from its DCS.  If the timer fires, it means that no
 * Hello messages have been received in the DCS's Hello interval.
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_hello_rcv_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of DCS entry
	 */
	dcsp = (Scsp_dcs *) ((caddr_t)stp -
			(int)(&((Scsp_dcs *)0)->sd_hello_rcv_t));

	/*
	 * Call the Hello FSM
	 */
	(void)scsp_hfsm(dcsp, SCSP_HFSM_RCV_T, (void *)0);

	return;
}


/*
 * Process an SCSP CA retransmit timeout
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_ca_retran_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of DCS entry
	 */
	dcsp = (Scsp_dcs *) ((caddr_t)stp -
			(int)(&((Scsp_dcs *)0)->sd_ca_rexmt_t));

	/*
	 * Call the CA FSM
	 */
	(void)scsp_cafsm(dcsp, SCSP_CAFSM_CA_T, (void *)0);

	return;
}


/*
 * Process an SCSP CSUS retransmit timeout
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_csus_retran_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of DCS entry
	 */
	dcsp = (Scsp_dcs *) ((caddr_t)stp -
			(int)(&((Scsp_dcs *)0)->sd_csus_rexmt_t));

	/*
	 * Call the CA FSM
	 */
	(void)scsp_cafsm(dcsp, SCSP_CAFSM_CSUS_T, (void *)0);

	return;
}


/*
 * Process an SCSP CSU Req retransmit timeout
 *
 * Arguments:
 *	stp	pointer to an SCSP timer block
 *
 * Returns:
 *	None
 *
 */
void
scsp_csu_req_retran_timeout(stp)
	Harp_timer	*stp;
{
	Scsp_csu_rexmt	*rxp;
	Scsp_dcs	*dcsp;

	/*
	 * Back off to start of CSU Request retransmission entry
	 */
	rxp = (Scsp_csu_rexmt *) ((caddr_t)stp -
			(int)(&((Scsp_csu_rexmt *)0)->sr_t));
	dcsp = rxp->sr_dcs;

	/*
	 * Call the CA FSM
	 */
	(void)scsp_cafsm(dcsp, SCSP_CAFSM_CSU_T, (void *)rxp);

	return;
}
