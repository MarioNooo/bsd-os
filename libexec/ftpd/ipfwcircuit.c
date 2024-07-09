/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997,1999,2000 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwcircuit.c,v 1.2 2001/10/03 17:29:55 polk Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_circuit.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include "ftpd.h"

int
gsa_to_ia(struct sockaddr_generic *sa, struct in_addr *ia)
{
	switch (SA(sa)->sa_family) {
	case AF_INET6:
		if (!IN6_IS_ADDR_V4MAPPED(&(SIN6(sa))->sin6_addr))
			return (-1);
		memcpy(ia, (u_int8_t *)&SIN6(sa)->sin6_addr + 12,
		    sizeof(struct in_addr));
		return (0);
	

	case AF_INET:
		*ia = SIN(sa)->sin_addr;
		return (0);
	}
	return (-1);
}

int
insert_entry(int serial, struct sockaddr_generic *src, struct sockaddr_generic *dst, u_long ports)
{
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_INPUT,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_ADD };
	ipfw_cf_list_t list[1];
	uid_t uid;
	int e;

	if (gsa_to_ia(src, &list[0].cl_a1) < 0 ||
	    gsa_to_ia(dst, &list[0].cl_a2) < 0)
		return (-1);

	mib[7] = serial;
	list[0].cl_ports = ports;
	list[0].cl_size = sizeof(list[0].cl_core);

	uid = geteuid();
	seteuid((uid_t)0);
	e = sysctl(mib, 9, NULL, NULL, list, list[0].cl_size);
	seteuid(uid);
	return (e);
}

int
delete_entry(int serial, struct sockaddr_generic *src, struct sockaddr_generic *dst, u_long ports)
{
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_INPUT,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_DELETE };
	ipfw_cf_list_t list[1];
	uid_t uid;
	int e;

	mib[7] = serial;
	if (gsa_to_ia(src, &list[0].cl_a1) < 0 ||
	    gsa_to_ia(dst, &list[0].cl_a2) < 0)
		return (-1);
	list[0].cl_ports = ports;
	list[0].cl_size = sizeof(list[0].cl_core);

	uid = geteuid();
	seteuid((uid_t)0);
	e = sysctl(mib, 9, NULL, NULL, list, list[0].cl_size);
	seteuid(uid);
	return (e);
}

#define	NLOCS	0x80 /* 0x80 */
int nlocs = NLOCS;

void
expire(int serial, int location)
{
	static u_quad_t lastclock[NLOCS] = { 0 };
	size_t size;
	int i;

#define	LASTCLOCK	lastclock[(location+1) & (nlocs-1)]
#define	THISCLOCK	lastclock[location & (nlocs-1)]

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_INPUT,
		IPFWCTL_SYSCTL, IPFW_CIRCUIT, /*serial*/0, CIRCUITCTL_CLOCK };

	mib[7] = serial;
	size = sizeof(u_quad_t);
	if (LASTCLOCK)
		i = sysctl(mib, 9, &THISCLOCK, &size, &LASTCLOCK, sizeof(u_quad_t));
	else
		i = sysctl(mib, 9, &THISCLOCK, &size, NULL, 0);
	if (i < 0)
		warn("circuit clock %d, tick %d", serial, location);
}

int
install_circuit_cache(int size, u_long mask, char *tag, int prio)
{
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_INPUT,
		0 };
	int serial, n;
	ipfw_filter_t *ift;
	void *b;
	struct {
		ipfw_filter_t filter;
		ipfw_cf_hdr_t circuit;
	} fc;

	/*
	 * First check to see if there is an existing circuit cache for
	 * us to use.  If so, just use that one.
	 */
	mib[5] = IPFWCTL_LIST;
	if (sysctl(mib, 6, NULL, &n, NULL, NULL) < 0)
		err(1, "listing circuit filters");

	if (n != 0) {
		if ((b = malloc(n)) == NULL)
			err(1, NULL);

		if (sysctl(mib, 6, b, &n, NULL, NULL) < 0)
			err(1, "getting NAT entries");
		ift = (ipfw_filter_t *)b;
		serial = 0;
		while (n > 0) {
			if (ift->type == IPFW_CIRCUIT &&
			    strcmp(ift->tag, tag) == 0) {
				serial = ift->serial;
				free(b);
				return (serial);
			}
			n -= ift->hlength;
			ift = (ipfw_filter_t *)(((char *)ift) + ift->hlength);
		}
		free(b);
	}

	/*
	 * Generate an empty circuit table entry
	 */

	memset(&fc, 0, sizeof(fc));

	n = sizeof(serial);
	mib[5] = IPFWCTL_SERIAL;
	if (sysctl(mib, 6, &serial, &n, NULL, NULL))
		err(1, "getting serial number");

	fc.filter.type = IPFW_CIRCUIT;
	fc.filter.hlength = sizeof(fc.filter);
	fc.filter.length = sizeof(fc.circuit);
	strcpy(fc.filter.tag, tag);
	fc.filter.priority = prio;
	fc.filter.serial = serial;

	fc.circuit.size = size <= 0 ? 997 : size;
	fc.circuit.hitcode = IPFW_ACCEPT;
	fc.circuit.misscode = IPFW_REJECT;
	fc.circuit.misscode |= IPFW_NEXT;
	fc.circuit.datamask = mask;
	fc.circuit.protocol = IPPROTO_TCP;
	fc.circuit.flip16 = 0;
	if (mask == 0xffffffff)
		fc.circuit.follow = 1;

	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 6, NULL, NULL, &fc, sizeof(fc)))
		err(1, "pushing circuit");
	return (serial);
}
