/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996,1998 Berkeley Software Design, Inc. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this notice is retained,
 * the conditions in the following notices are met, and terms applying
 * to contributors in the following notices also apply to Berkeley
 * Software Design, Inc.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Berkeley Software Design, Inc.
 * 4. Neither the name of the Berkeley Software Design, Inc. nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      BSDI misc.c,v 1.13 2002/01/22 16:09:59 dab Exp
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "ftpd.h"
#include "extern.h"

#define XBUFSIZ 8192

char *xferbuf;
int xferbufsize;
static char xbuf[XBUFSIZ];

void
setup_xferbuf(int f, int n, int which)
{
	struct stat st;
	int bsize;
	size_t len;

	if (f == -1 || fstat(f, &st) < 0)
		st.st_blksize = 0;
	len = sizeof(bsize);
	if (n == NULL || getsockopt(n, SOL_SOCKET, which, &bsize, &len))
		bsize = 0;
	if (bsize < st.st_blksize)
		bsize = st.st_blksize;
	if (bsize <= 0)
		bsize = XBUFSIZ;
	if (bsize > xferbufsize) {
		if (xferbuf && xferbuf != xbuf)
			(void) free(xferbuf);
		if (bsize <= sizeof(xbuf) ||
		    (xferbuf = malloc((unsigned)bsize)) == NULL) {
			xferbuf = xbuf;
			xferbufsize = sizeof(xbuf);
		} else
			xferbufsize = bsize;
	}
}

#define NRL_SA_LEN(x) ((struct sockaddr *)(x))->sa_len

#define	SIN_ADDR_BYTE(sa, n) \
	((u_int8_t *)(&((struct sockaddr_in *)(sa))->sin_addr))[n]

#define	SIN_PORT_BYTE(sa, n) \
	((u_int8_t *)(&((struct sockaddr_in *)(sa))->sin_port))[n]

#if INET6
#define	SIN6_ADDR_BYTE(sa, n) \
	((u_int8_t *)(&((struct sockaddr_in6 *)(sa))->sin6_addr))[n]

#define	SIN6_PORT_BYTE(sa, n) \
	((u_int8_t *)(&((struct sockaddr_in6 *)(sa))->sin6_port))[n]
#endif /* INET6 */

static int
addrcmp(struct sockaddr *sa1, struct sockaddr *sa2)
{
	if (sa1->sa_family != sa2->sa_family)
		return (sa1->sa_family < sa2->sa_family) ? -1 : 1;
	if (sa1->sa_len != sa2->sa_len)
		return (sa1->sa_len < sa2->sa_len) ? -1 : 1;

	switch(sa1->sa_family) {
	case AF_INET:
		return memcmp(&SIN(sa1)->sin_addr, &SIN(sa2)->sin_addr,
		    sizeof(struct in_addr));
#if INET6
	case AF_INET6:
		return memcmp(&SIN6(sa1)->sin6_addr, &SIN6(sa2)->sin6_addr,
		    sizeof(struct in6_addr));
#endif /* INET6 */
	default:
		return -1;
	}
}

int
satoport(char *port, struct sockaddr *sa)
{
	switch(sa->sa_family) {
	case AF_INET:
		sprintf(port, "%u,%u,%u,%u,%u,%u",
		    SIN_ADDR_BYTE(sa, 0),
		    SIN_ADDR_BYTE(sa, 1),
		    SIN_ADDR_BYTE(sa, 2),
		    SIN_ADDR_BYTE(sa, 3),
		    SIN_PORT_BYTE(sa, 0),
		    SIN_PORT_BYTE(sa, 1));
		return(0);
#if INET6
	case AF_INET6:
		if (!IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr))
			break;
		sprintf(port, "%u,%u,%u,%u,%u,%u",
		    SIN6_ADDR_BYTE(sa, 12),
		    SIN6_ADDR_BYTE(sa, 13),
		    SIN6_ADDR_BYTE(sa, 14),
		    SIN6_ADDR_BYTE(sa, 15),
		    SIN6_PORT_BYTE(sa, 0),
		    SIN6_PORT_BYTE(sa, 1));
		return(0);
#endif /* INET6 */
	}
	return(1);
}

#define GET(x) \
	c2 = c; \
	while(*c2 && isdigit(*c2)) \
		c2++; \
	if (*c2 != ',') \
		return (-1); \
	*(c2++) = 0; \
	x = atoi(c); \
	c = c2;

#define EGET(x) \
	c2 = c; \
	while(*c2 && isdigit(*c2)) \
		c2++; \
	if (*c2) \
		return (-1); \
	x = atoi(c);

int
porttosa(struct sockaddr *sa, char *port, struct sockaddr *protosa)
{
	int i;
	char *c, *c2;
	u_int8_t *p;

	c = port;

	memcpy(sa, protosa, NRL_SA_LEN(protosa));

	i = 4;

	switch(sa->sa_family) {
#if INET6
	case AF_INET6:
		if (!IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr))
			return (-2);
		p = (u_int8_t *)&(SIN6(sa))->sin6_addr + 12;
		break;
#endif /* INET6 */
	case AF_INET:
		p = (u_int8_t *)&(SIN(sa))->sin_addr;
		break;
	default:
		return (-2);
	}

	while(i--) {
		GET(*(p++))
	}

	if (addrcmp(sa, protosa))
		return (-3);

	switch(sa->sa_family) {
#if INET6
	case AF_INET6:
		p = (u_int8_t *)&(SIN6(sa))->sin6_port;
		break;
#endif /* INET6 */
	case AF_INET:
		p = (u_int8_t *)&(SIN(sa))->sin_port;
		break;
	}

	GET(*(p++))
	EGET(*p)

	return (0);
}

int
satolport(char *lport, struct sockaddr *sa)
{
#if INET6
	struct sockaddr_in sin;
#endif /* INET6 */

	switch(sa->sa_family) {
	case AF_INET:
#if INET6
	ipv4:
#endif /* INET6 */
		sprintf(lport, "4,4,%u,%u,%u,%u,2,%u,%u",
		    SIN_ADDR_BYTE(sa, 0),
		    SIN_ADDR_BYTE(sa, 1),
		    SIN_ADDR_BYTE(sa, 2),
		    SIN_ADDR_BYTE(sa, 3),
		    SIN_PORT_BYTE(sa, 0),
		    SIN_PORT_BYTE(sa, 1));
		return (0);

#if INET6
	case AF_INET6:
		if (IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr)) {
                        fix_v4mapped(sa, SA(&sin));
			sa = (struct sockaddr *)&sin;
			goto ipv4;
		}

		sprintf(lport,
		    "6,16,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,2,%u,%u",
		    SIN6_ADDR_BYTE(sa, 0),
		    SIN6_ADDR_BYTE(sa, 1),
		    SIN6_ADDR_BYTE(sa, 2),
		    SIN6_ADDR_BYTE(sa, 3),
		    SIN6_ADDR_BYTE(sa, 4),
		    SIN6_ADDR_BYTE(sa, 5),
		    SIN6_ADDR_BYTE(sa, 6),
		    SIN6_ADDR_BYTE(sa, 7),
		    SIN6_ADDR_BYTE(sa, 8),
		    SIN6_ADDR_BYTE(sa, 9),
		    SIN6_ADDR_BYTE(sa, 10),
		    SIN6_ADDR_BYTE(sa, 11),
		    SIN6_ADDR_BYTE(sa, 12),
		    SIN6_ADDR_BYTE(sa, 13),
		    SIN6_ADDR_BYTE(sa, 14),
		    SIN6_ADDR_BYTE(sa, 15),
		    SIN6_PORT_BYTE(sa, 0),
		    SIN6_PORT_BYTE(sa, 1));
		return (0);
#endif /* INET6 */
	}
	return (1);
}

int
lporttosa(struct sockaddr *sa, char *lport, struct sockaddr *protosa)
{
	int proto, i;
	char *c, *c2;
	u_int8_t *p;

	c = lport;

	GET(proto)
	GET(i)

	memcpy(sa, protosa, NRL_SA_LEN(protosa));

	switch(proto) {
#if INET6
	case 6:
		if (i != sizeof(struct in6_addr))
			return -1;

		switch(sa->sa_family) {
		case AF_INET6:
			p = (u_int8_t *)&(SIN6(sa))->sin6_addr;
			break;
		default:
			return (-2);
		}
		break;
#endif /* INET6 */
	case 4:
		if (i != sizeof(struct in_addr))
			return (-1);

		switch(sa->sa_family) {
#if INET6
		case AF_INET6:
			if (!IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr))
				return (-2);
			p = (u_int8_t *)&(SIN6(sa))->sin6_addr + 12;
			break;
#endif /* INET6 */
		case AF_INET:
			p = (u_int8_t *)&(SIN(sa))->sin_addr;
			break;
		default:
			return (-2);
		}
		break;
	default:
		return (-1);
	}

	while(i--) {
		GET(*(p++))
	}

	GET(i)

	switch(proto) {
#if INET6
	case 6:
		if (i != 2)
			return (-1);

		switch(sa->sa_family) {
		case AF_INET6:
			p = (u_int8_t *)&(SIN6(sa))->sin6_port;
			break;
		}
		break;
#endif /* INET6 */
	case 4:
		if (i != 2)
			return (-1);

		switch(sa->sa_family) {
#if INET6
		case AF_INET6:
			p = (u_int8_t *)&(SIN6(sa))->sin6_port;
			break;
#endif /* INET6 */
		case AF_INET:
			p = (u_int8_t *)&(SIN(sa))->sin_port;
			break;
		}
		break;
	}

	if (addrcmp(sa, protosa))
		return (-3);

	while(i-- > 1) {
		GET(*(p++))
	}
	EGET(*p)

	return 0;
}

int
satosport(char *sport, struct sockaddr *sa)
{
	switch(sa->sa_family) {
	case AF_INET:
		sprintf(sport, "%u,%u",
		    SIN_PORT_BYTE(sa, 0), SIN_PORT_BYTE(sa, 1));
		return (0);
#if INET6
	case AF_INET6:
		sprintf(sport, "%u,%u",
		    SIN6_PORT_BYTE(sa, 0), SIN6_PORT_BYTE(sa, 1));
		return (0);
#endif /* INET6 */
	}
	return (1);
}

int
sporttosa(struct sockaddr *sa, char *sport, struct sockaddr *protosa)
{
	char *c, *c2;
	u_int8_t *p;

	c = sport;

	memcpy(sa, protosa, NRL_SA_LEN(protosa));

	switch(sa->sa_family) {
	case AF_INET:
		p = (u_int8_t *)&(SIN(sa))->sin_port;
		break;
#if INET6
	case AF_INET6:
		p = (u_int8_t *)&(SIN6(sa))->sin6_port;
		break;
#endif /* INET6 */
	default:
		return (-1);
	}

	GET(*(p++))
	EGET(*p)

	return (0);
}

int
satoeport(char *eport, int eportlen, struct sockaddr *sa, int passive)
{
	char sbuf[NI_MAXSERV];
	char hbuf[NI_MAXHOST];
	char *netprt;

	if (passive) {
		if (getnameinfo(sa, NRL_SA_LEN(sa), NULL, 0, sbuf,
		    NI_MAXSERV, NI_NUMERICSERV))
			return 1;

		snprintf(eport, eportlen, "|||%s|", sbuf);
		return 0;
	}

	switch(sa->sa_family) {
	case AF_INET:
		netprt = "1";
		break;
#if INET6
	case AF_INET6:
		if (IN6_IS_ADDR_V4MAPPED(
		    &((struct sockaddr_in6 *)sa)->sin6_addr)) {
			struct sockaddr_in sin;

			memset(&sin, 0, sizeof(struct sockaddr_in));
			sin.sin_family = AF_INET;
			sin.sin_port = ((struct sockaddr_in6 *)sa)->sin6_port;
			memcpy(&sin.sin_addr, (u_int8_t *)
			    &((struct sockaddr_in6 *)sa)->sin6_addr + 12,
			    sizeof(struct in_addr));

			return satoeport(eport, eportlen,
			    (struct sockaddr *)&sin, passive);
		}
		netprt = "2";
		break;
#endif /* INET6 */
	default:
		return 1;
	}
	if (getnameinfo(sa, NRL_SA_LEN(sa), hbuf, NI_MAXHOST, sbuf, NI_MAXSERV,
	    NI_NUMERICHOST | NI_NUMERICSERV))
		return 1;

	snprintf(eport, eportlen, "|%s|%s|%s|", netprt, hbuf, sbuf);

	return 0;
}

int
eporttosa(struct sockaddr *sa, char *eport,
    struct sockaddr *protosa, int passive)
{
	struct addrinfo *ai, req;
	char d, *c;
	char *netprt, *host, *serv;

	memset(&req, 0, sizeof(struct addrinfo));
	req.ai_socktype = SOCK_STREAM;
#ifdef AI_NONAME
	req.ai_flags |= AI_NONAME;
#endif

	c = eport;
	d = *(c++);

#define ADVANCE while(*c != d) if (!*(c++)) return -1; *(c++) = 0;

	netprt = c;
	ADVANCE;

	if (passive) {
		if (*netprt)
			return -3;
	} else if (strcmp(netprt, "IP4") == 0)
		req.ai_family = AF_INET;
#if INET6
	else if (strcmp(netprt, "IP6") == 0)
		req.ai_family = AF_INET6;
#endif /* INET6 */
	else {
		switch(atoi(netprt)) {
		case 1:
			req.ai_family = AF_INET;
			break;
#if INET6
		case 2:
			req.ai_family = AF_INET6;
			break;
#endif /* INET6 */
		default:
			return -2;
		}
	}

	host = c;
	ADVANCE;
	serv = c;
	ADVANCE;

	if (passive ^ !*host)
		return -3;

	if (!*serv)
		return -1;

	if (passive) {
		char hbuf[NI_MAXHOST];

		if (getnameinfo(protosa, NRL_SA_LEN(protosa),
		    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST))
			return -1;

		if (getaddrinfo(hbuf, serv, &req, &ai))
			return -1;
	} else {
		if (getaddrinfo(host, serv, &req, &ai))
			return -1;

#if INET6
		/*
		 * If it's an AF_INET address, but our connection is
		 * with IPv4 mapped addresses, check the IPv4 part
		 * of the mapped address.  If they match, fill in
		 * sa as an IPv4 mapped address.
		 */
		if (ai->ai_addr->sa_family == AF_INET &&
		    protosa->sa_family == AF_INET6 &&
		    IN6_IS_ADDR_V4MAPPED(&SIN6(protosa)->sin6_addr) &&
		    memcmp(&(SIN6(protosa)->sin6_addr.s6_addr[12]),
		    &SIN(ai->ai_addr)->sin_addr, sizeof(struct in_addr)) == 0) {
			struct in6_addr *s6;
			SIN6(sa)->sin6_family = AF_INET6;
			SIN6(sa)->sin6_len = sizeof(struct sockaddr_in6);
			SIN6(sa)->sin6_port = SIN(ai->ai_addr)->sin_port;
			SIN6(sa)->sin6_flowinfo = 0;
			s6 = &SIN6(sa)->sin6_addr;
			memset(s6, 0, sizeof(*s6));
			s6->s6_addr[10] = s6->s6_addr[11] = 0xff;
			*(u_int32_t *)&s6->s6_addr[12] =
			    SIN(ai->ai_addr)->sin_addr.s_addr;
			return(0);
		}
#endif
		if (addrcmp(ai->ai_addr, protosa)) {
			freeaddrinfo(ai);
			return -3;
		}
	}

	memcpy(sa, ai->ai_addr, ai->ai_addrlen);
	freeaddrinfo(ai);

#if INET6
	/* For link and site local IPv6 addresses, copy over the scope ID */
	if ((sa->sa_family == AF_INET6) && (protosa->sa_family == AF_INET6) &&
	    ((IN6_IS_ADDR_LINKLOCAL(&SIN6(sa)->sin6_addr) &&
	    IN6_IS_ADDR_LINKLOCAL(&SIN6(protosa)->sin6_addr)) ||
	    (IN6_IS_ADDR_SITELOCAL(&SIN6(sa)->sin6_addr) &&
	    IN6_IS_ADDR_SITELOCAL(&SIN6(protosa)->sin6_addr))))
		SIN6(sa)->sin6_scope_id = SIN6(protosa)->sin6_scope_id;
#endif
	return 0;
}

void
fix_v4mapped(sa, sa2)
	struct sockaddr *sa, *sa2;
{
	struct sockaddr_in sin;

	if ((sa->sa_family != AF_INET6) ||
	    (!IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr)))
		return;

	if (sa2 == NULL)
		sa2 = SA(&sin);

	memset(sa2, 0, sizeof(struct sockaddr_in));
	SIN(sa2)->sin_family = AF_INET;
	SIN(sa2)->sin_len = sizeof(struct sockaddr_in);
	SIN(sa2)->sin_port = (SIN6(sa))->sin6_port;
	memcpy(&(SIN(sa2)->sin_addr), (u_int8_t *)&(SIN6(sa))->sin6_addr + 12,
	    sizeof(struct in_addr));

	if (sa2 == SA(&sin))
		memcpy(sa, &sin, sizeof(struct sockaddr_in));
}
