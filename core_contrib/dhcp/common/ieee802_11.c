/* packet.c

   Packet assembly code, originally contributed by Archie Cobbs. */

/*
 * Copyright (c) 1995, 1996 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#ifndef lint
static char copyright[] =
"ieee802_11.c,v 1.4 2001/09/19 01:02:26 geertj Exp Copyright (c) 1996 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"

#if defined (PACKET_ASSEMBLY) || defined (PACKET_DECODING)
#include "includes/netinet/if_ether.h"
#include <net/if_802_11.h>
#include <net/if_ethernet.h>
#endif /* PACKET_ASSEMBLY || PACKET_DECODING */

#if defined (PACKET_ASSEMBLY)
/* Assemble an hardware header... */
/* XXX currently only supports ethernet; doesn't check for other types. */

void assemble_ieee802_11_header (interface, buf, bufix, to)
	struct interface_info *interface;
	unsigned char *buf;
	int *bufix;
	struct hardware *to;
{
	struct ieee_802_11_header eh;

	if (to && to -> hlen == 6) /* XXX */
		memcpy (eh.e11_addr1, to -> hbuf, sizeof eh.e11_addr1);
	else
		memset (eh.e11_addr1, 0xff, sizeof (eh.e11_addr1));
	if ((interface -> hw_address.hlen - 1) == sizeof (eh.e11_addr2))
		memcpy (eh.e11_addr2, interface -> hw_address.hbuf + 1,
			sizeof (eh.e11_addr2));
	else
		memset (eh.e11_addr2, 0x00, sizeof (eh.e11_addr2));

	memcpy (&buf [*bufix], &eh, IEEE_802_11_HDR_LEN);
	*bufix += IEEE_802_11_HDR_LEN + IEEE_802_11_SNAPHDR_LEN;
}
#endif /* PACKET_ASSEMBLY */

#ifdef PACKET_DECODING
/* Decode a hardware header... */

ssize_t decode_ieee802_11_header (interface, buf, bufix, from)
     struct interface_info *interface;
     unsigned char *buf;
     int bufix;
     struct hardware *from;
{
  struct ieee_802_11_header ih;
  unsigned char snap[IEEE_802_11_SNAPHDR_LEN];
  int type;

  memcpy (&ih, buf + bufix, IEEE_802_11_HDR_LEN);
  memcpy (&snap, buf + bufix + IEEE_802_11_HDR_LEN, IEEE_802_11_SNAPHDR_LEN);
  type = (snap[6] << 8) | snap[7];

  if (type != ETHERTYPE_IP)
    return -1;

  switch ((ih.e11_frame_ctl1 << 8) & AC_802_11_MODE) {
  case AC_802_11_IBSS:
    memcpy (from -> hbuf, ih.e11_addr2, sizeof (ih.e11_addr2));
    break;

  case AC_802_11_ESS:
    memcpy (from -> hbuf, ih.e11_addr3, sizeof (ih.e11_addr3));
    break;

  case AC_802_11_AP:
    memcpy (from -> hbuf, ih.e11_addr2, sizeof (ih.e11_addr2));
    break;

  case AC_802_11_REP:
    memcpy (from -> hbuf, ih.e11_addr4, sizeof (ih.e11_addr4));
    break;
  }

  from -> hlen = sizeof ih.e11_addr2;

  return IEEE_802_11_HDR_LEN + IEEE_802_11_SNAPHDR_LEN;
}
#endif /* PACKET_DECODING */
