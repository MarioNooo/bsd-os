/*	BSDI aif.c,v 1.2 2001/03/06 23:03:20 polk Exp	*/

#include "dhcpd.h"
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <net/if_aif.h>

/*
 * Routine to add an ethernet<->IP-address mapping into the
 * kernel AIF mapping table.
 */
aif_add(packet, lease)
	struct packet *packet;
	struct lease *lease;
{
	struct aifreq ar;
	struct sockaddr_in sin;
	int sock;

	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		warn ("Can't create AIF socket");
		return;
	}

	memset (&sin, 0, sizeof sin);
	memcpy (&sin.sin_addr, lease -> ip_addr.iabuf, 4);
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;

	memset (&ar, 0, sizeof ar);
	memcpy (ar.aifr_name, packet -> interface -> name,
	    sizeof ar.aifr_name);
	ar.aifr_addr = (struct sockaddr *)&sin;
	ar.aifr_addrlen = sizeof(sin);
	memcpy (ar.aifr_ether, lease -> hardware_addr.hbuf,
	    sizeof ar.aifr_ether);

	if (ioctl(sock, AIFIOCSADDRE, &ar) < 0)
		warn ("AIFIOCSADDRE: %m");

	close (sock);
}
