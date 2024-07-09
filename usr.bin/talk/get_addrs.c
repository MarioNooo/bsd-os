/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)get_addrs.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <protocols/talkd.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include "talk_ctl.h"

/*
 * Try to find the interface address that is used to route an IP
 * packet to a remote peer.
 */
static void
get_iface(dst, iface, port)
	struct in_addr *dst;
	struct in_addr *iface;
	u_short port;
{
	struct sockaddr_in addr;
	struct hostent *hp;
	int s, namelen;

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "getting local address: socket");

	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		err(1, "getting local address: bind");

	addr.sin_port = port;
	addr.sin_addr = *dst;	/* struct copy */
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		err(1, "getting local address: connect");

	namelen = sizeof(addr);
	if (getsockname(s, (struct sockaddr *)&addr, &namelen) < 0)
		err(1, "getting local address: getsockname");

	*iface = addr.sin_addr;	/* struct copy */
}


get_addrs(my_machine_name, his_machine_name)
	char *my_machine_name, *his_machine_name;
{
	struct hostent *hp;
	struct servent *sp;

	msg.pid = htonl(getpid());

	if ((hp = gethostbyname(his_machine_name)) == NULL)
		errx(1, "%s: %s", his_machine_name, hstrerror(h_errno));
	bcopy(hp->h_addr, (char *)&his_machine_addr, hp->h_length);

	/* find the server's port */
	sp = getservbyname("ntalk", "udp");
	if (sp == NULL)
		errx(1, "%s/%s: service is not registered.", "ntalk", "udp");
	daemon_port = sp->s_port;

	get_iface(&his_machine_addr, &my_machine_addr, sp->s_port);
}
