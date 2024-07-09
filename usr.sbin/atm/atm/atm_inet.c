/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1994-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *	@(#) atm_inet.c,v 1.9 1998/08/26 23:29:31 mks Exp
 *
 */

/*
 * User configuration and display program
 * --------------------------------------
 *
 * IP support
 *
 */

#ifndef lint
static char *RCSid = "@(#) atm_inet.c,v 1.9 1998/08/26 23:29:31 mks Exp";
#endif

#include <netatm/user_include.h>

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../lib/libatm.h"
#include "atm.h"


/*
 * Process add command for a TCP/IP PVC
 * 
 * Command format: 
 *	atm add pvc <intf> <vpi> <vci> <aal> <encaps> IP <netif>
 *		<IP addr> | dynamic
 *
 * Arguments:
 *	argc	number of remaining arguments to command
 *	argv	pointer to remaining argument strings
 *	cmdp	pointer to command description 
 *	app	pointer to AIOCAPVC structure
 *	intp	pointer to air_int_rsp structure with information
 *		about the physical interface that is the PVC is for.
 *
 * Returns:
 *	none
 *
 */
void
ip_pvcadd(argc, argv, cmdp, app, intp)
	int			argc;
	char			**argv;
	struct cmd		*cmdp;
	struct atmaddreq	*app;
	struct air_int_rsp	*intp;
{
	char	*cp;
	char	nhelp[128];
	int	i, netif_pref_len, netif_no;

	/*
	 * Yet more validation
	 */
	if (argc != 2) {
		strcpy(nhelp, cmdp->help);
		cp = strstr(nhelp, "<netif>");
		if (cp)
			strcpy(cp, "ip {dyn|<dst>}");
		fprintf(stderr, "%s: Invalid number of arguments:\n",
				prog);
		fprintf(stderr, "\tformat is: %s%s %s\n",
				prefix, cmdp->name, nhelp);
		exit(1);
	}

	/*
	 * Validate and set network interface
	 */
	UM_ZERO(app->aar_pvc_intf, sizeof(app->aar_pvc_intf));
	netif_pref_len = strlen(intp->anp_nif_pref);
	cp = &argv[0][netif_pref_len];
	netif_no = atoi(cp);
	for (i=0; i<strlen(cp); i++) {
		if (cp[i] < '0' || cp[i] > '9') {
			netif_no = -1;
			break;
		}
	}
	if ((strlen(argv[0]) > sizeof(app->aar_pvc_intf) - 1) ||
			(netif_no < 0)) {
		fprintf(stderr, "%s: Illegal network interface name\n",
				prog);
		exit(1);
	}
	if (strncasecmp(intp->anp_nif_pref, argv[0], netif_pref_len) ||
			strlen (argv[0]) <= netif_pref_len ||
			netif_no > intp->anp_nif_cnt - 1) {
		fprintf(stderr, "%s: network interface %s is not associated with interface %s\n",
				prog,
				argv[0],
				intp->anp_intf);
		exit(1);
	}
	strcpy(app->aar_pvc_intf, argv[0]);
	argc--; argv++;

	/*
	 * Set PVC destination address
	 */
	UM_ZERO(&app->aar_pvc_dst, sizeof(struct sockaddr));
	if (strcasecmp(argv[0], "dynamic") == 0 ||
			strcasecmp(argv[0], "dyn") == 0) {

		/*
		 * Destination is dynamically determined
		 */
		app->aar_pvc_flags |= PVC_DYN;
	} else {

		/*
		 * Get destination IP address
		 */
		struct sockaddr_in	*sin;

		sin = &app->aar_pvc_dst.sin;
		sin->sin_addr.s_addr =
				get_ip_addr(argv[0])->sin_addr.s_addr;
	}
	argc--; argv++;
}

