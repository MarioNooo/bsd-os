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
 *	@(#) atmarp_config.c,v 1.5 1998/08/13 20:11:11 johnc Exp
 *
 */

/*
 * Server Cache Synchronization Protocol (SCSP) Support
 * ----------------------------------------------------
 *
 * SCSP-ATMARP server interface: configuration support
 *
 */

#ifndef lint
static char *RCSid = "@(#) atmarp_config.c,v 1.5 1998/08/13 20:11:11 johnc Exp";
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

#include <netatm/uni/uniip_var.h>
#include "../lib/libatm.h"
#include "../scspd/scsp_msg.h"
#include "../scspd/scsp_if.h"
#include "../scspd/scsp_var.h"
#include "atmarp_var.h"


/*
 * Configure network interface for ATMARP cache synchronization
 *
 * Verify the network interface name and set the appropriate fields
 * in the ATMARP interface entry.
 *
 * Arguments:
 *	netif	pointer to network interface name
 *
 * Returns:
 *	0	success
 *	errno	reason for failure
 *
 */
int
atmarp_cfg_netif(netif)
	char	*netif;
{
	int			rc;
	Atmarp_intf		*aip = (Atmarp_intf *)0;
	Atm_addr_nsap		*anp;

	/*
	 * Get an ATMARP interface block
	 */
	aip = (Atmarp_intf *)UM_ALLOC(sizeof(Atmarp_intf));
	if (!aip)
		atmarp_mem_err("atmarp_cfg_netif: sizeof(Atmarp_intf)");
	UM_ZERO(aip, sizeof(Atmarp_intf));

	/*
	 * Make sure we're configuring a valid
	 * network interface
	 */
	rc = verify_nif_name(netif);
	if (rc == 0) {
		fprintf(stderr, "%s: \"%s\" is not a valid network interface\n",
				prog, netif);
		rc = EINVAL;
		goto cfg_fail;
	} else if (rc < 0) {
		rc = errno;
		fprintf(stderr, "%s: can't verify network interface \"%s\"\n",
				prog, netif);
		goto cfg_fail;
	}

	/*
	 * Update the interface entry
	 */
	strcpy(aip->ai_intf, netif);
	aip->ai_state = AI_STATE_NULL;
	aip->ai_scsp_sock = -1;
	LINK2TAIL(aip, Atmarp_intf, atmarp_intf_head, ai_next);

	return(0);

cfg_fail:
	if (aip)
		UM_FREE(aip);

	return(rc);
}
