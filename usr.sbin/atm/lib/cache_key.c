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
 *	@(#) cache_key.c,v 1.1 1998/07/09 21:45:27 johnc Exp
 *
 */


/*
 * User Space Library Functions
 * ----------------------------
 *
 * SCSP cache key computation
 *
 */


#ifndef lint
static char *RCSid = "@(#) cache_key.c,v 1.1 1998/07/09 21:45:27 johnc Exp";
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
#include "../lib/md5global.h"
#include "../lib/md5.h"


/*
 * Compute an SCSP cache key
 *
 * Arguments:
 *	ap	pointer to an Atm_addr with the ATM address
 *	ip	pointer to a struct in_addr with the IP address
 *	ol	the required length of the cache key
 *	op	pointer to receive cache key
 *
 * Returns:
 *	none
 *
 */
void
scsp_cache_key(ap, ip, ol, op)
	Atm_addr	*ap;
	struct in_addr	*ip;
	int		ol;
	char 		*op;
{
	int	i, key, len;
	char	*cp;
	char	buff[32], digest[16];
	MD5_CTX	context;

	/*
	 * Initialize
	 */
	UM_ZERO(buff, sizeof(buff));

	/*
	 * Copy the addresses into a buffer for MD5 computation
	 */
	len = sizeof(struct in_addr) + ap->address_length;
	if (len > sizeof(buff))
		len = sizeof(buff);
	UM_COPY(ip, buff, sizeof(struct in_addr));
	UM_COPY(ap->address, &buff[sizeof(struct in_addr)],
			len - sizeof(struct in_addr));

	/*
	 * Compute the MD5 digest of the combined IP and ATM addresses
	 */
	MD5Init(&context);
	MD5Update(&context, buff, len);
	MD5Final(digest, &context);

	/*
	 * Fold the 16-byte digest to the required length
	 */
	UM_ZERO((caddr_t)op, ol);
	for (i = 0; i < 16; i++) {
		op[i % ol] = op[i % ol] ^ digest[i];
	}
}
