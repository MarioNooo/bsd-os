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
 *	@(#) ip_checksum.c,v 1.4 1998/08/11 18:11:48 johnc Exp
 *
 */


/*
 * User Space Library Functions
 * ----------------------------
 *
 * IP checksum computation
 *
 */


#ifndef lint
static char *RCSid = "@(#) ip_checksum.c,v 1.4 1998/08/11 18:11:48 johnc Exp";
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


/*
 * Compute an IP checksum
 *
 * This code was taken from RFC 1071.
 *
 * "The following "C" code algorithm computes the checksum with an inner
 * loop that sums 16 bits at a time in a 32-bit accumulator."
 *
 * Arguments:
 *	addr	pointer to the buffer whose checksum is to be computed
 *	count	number of bytes to include in the checksum
 *
 * Returns:
 *	the computed checksum
 *
 */
short
ip_checksum(addr, count)
	char 	*addr;
	int	count;
{
	/* Compute Internet Checksum for "count" bytes
	 * beginning at location "addr".
	 */
	register long sum = 0;

	while( count > 1 ) {
		/* This is the inner loop */
		sum += ntohs(* (unsigned short *) addr);
		addr += sizeof(unsigned short);
		count -= sizeof(unsigned short);
	}

	/* Add left-over byte, if any */
	if( count > 0 )
		sum += * (unsigned char *) addr;

	/* Fold 32-bit sum to 16 bits */
	while (sum>>16)
		sum = (sum & 0xffff) + (sum >> 16);

	return((short)~sum);
}
