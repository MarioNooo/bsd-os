/*	BSDI if_util.c,v 2.2 1997/12/10 16:53:15 jch Exp	*/

/*
 * Copyright (c) 1983, 1988, 1993
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

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <kvm_stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "if_util.h"

/* Cache of info about interfaces we have already seen */
static	struct if_entry *ifs;		/* Pointers to interfaces we've found */
static	int n_ifs;			/* Size of array */

/* End of current list of messages */
static	struct if_msghdr *ifm_ep;

static struct if_entry *if_saveptr __P((struct if_msghdr *));

/* 
 * Scan the interface list and call the given linkfn for each
 * RTM_IFINFO and addrn for each RTM_NEWADDR.  Specification of an
 * interface name or address family will limit the search.
 */
void
if_loop(kvm_t *kvmd, char *interface, int af,
    void (*linkfn) (struct if_msghdr *, struct sockaddr_dl *, char *),
    void (*addrfn) (struct if_msghdr *, struct sockaddr_dl *, char *,
	struct ifa_msghdr *, struct rt_addrinfo *))
{
	struct ifa_msghdr *ifam;
	struct if_msghdr *link_ifm, *ifmm, *ifm0;
	struct if_data *ifdp;
	struct rt_addrinfo info;
	struct sockaddr_dl *sdl;
	size_t len;
	int ifindex;
	char *name;

	ifindex = 0;
	name = NULL;
	sdl = NULL;
	link_ifm = NULL;
	ifdp = NULL;

	if ((ifm0 = kvm_iflist(kvmd, 0, af, &len)) == NULL)
		err(1, "kvm_iflist");
	ifm_ep = (struct if_msghdr *)((u_char *)ifm0 + len);

	if (ifm0->ifm_version != RTM_VERSION)
		errx(1, "kvm_iflist: version mismatch");

	len = interface == NULL ? 0 : strlen(interface);

	for (ifmm = ifm0; ifmm < ifm_ep; 
	     ifmm = (struct if_msghdr *)((u_char *)ifmm + ifmm->ifm_msglen)) {
		/* Take the short way out */
		if (ifindex != 0 && ifindex != ifmm->ifm_index)
			break;
		if_saveptr(ifmm);
		if (ifmm->ifm_type == RTM_IFINFO) {
			link_ifm = ifmm;
			if_xaddrs(link_ifm + 1, link_ifm->ifm_addrs, &info);
			sdl = (struct sockaddr_dl *)info.rti_info[RTAX_IFP];
			if (len != 0) {
				if (len != sdl->sdl_nlen ||
				    strncmp(sdl->sdl_data, interface, len) != 0)
					continue;
				ifindex = link_ifm->ifm_index;
				len = 0;
			}
			if (name != NULL)
				free(name);
			name = malloc(sdl->sdl_nlen + 2);
			(void)sprintf(name, "%.*s%s", 
			    sdl->sdl_nlen, sdl->sdl_data,
			    link_ifm->ifm_flags & IFF_UP ? "" : "*");
			if (linkfn != NULL)
				(*linkfn)(link_ifm, sdl, name);
			continue;
		}

		/* If we are not processing addresses, skip this one */
		if (addrfn == NULL)
			continue;

		if (len != 0 && ifindex == 0)
			continue;

		ifam = (struct ifa_msghdr *)ifmm;
		if_xaddrs(ifam + 1, ifam->ifam_addrs, &info);

		(*addrfn)(link_ifm, sdl, name, ifam, &info);
	}
	free(ifm0);
}


/* Parse the address in a routine socket message. */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(s) (s = (struct sockaddr *) (ROUNDUP((s)->sa_len) + (caddr_t) sa))

void
if_xaddrs(void *vp, int addrs, struct rt_addrinfo *info)
{
	struct sockaddr *sa;
	int i;

	sa = vp;
	info->rti_addrs = addrs;
	memset(info->rti_info, 0, sizeof(info->rti_info));

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			info->rti_info[i] = sa;
			switch (i) {
			case RTAX_NETMASK:
			case RTAX_GENMASK:
				if (sa->sa_len == 0)
					info->rti_info[i] = NULL;
				break;
			}
			ADVANCE(sa);
		}
	}
}

/* 
 * Fix the ifm_msghdr for the specified index.  If we do not have the
 * given interface in the cache we'll scan forward from the given
 * pointer.
 */
struct if_entry *
if_byindex(struct if_msghdr *ifm0, int if_index)
{
	struct if_msghdr *ifm;

	if (if_index < n_ifs && ifs[if_index].ife_msghdr != NULL)
		return (&ifs[if_index]);

	for (ifm = ifm0; ifm < ifm_ep; 
	     ifm = (struct if_msghdr *)((u_char *)ifm + ifm->ifm_msglen))
		if (ifm->ifm_type == RTM_IFINFO && ifm->ifm_index == if_index)
			break;

	return (ifm != NULL ? if_saveptr(ifm) : NULL);
}

/*
 * Save a reference to this if_msghdr by index for future reference.
 */
struct if_entry *
if_saveptr(struct if_msghdr *ifm)
{
	struct if_entry *ife;
	struct rt_addrinfo info;

	if (ifm->ifm_index > n_ifs) {
		int n;

		n = ifm->ifm_index * 8;
		if ((ifs = realloc(ifs, n * sizeof(*ifs))) == NULL) {
			errno = ENOMEM;
			err(1, "");
		}
		memset(ifs + n_ifs, 0, (n - n_ifs) * sizeof(*ifs));
		n_ifs = n;
	}
	ife = ifs + ifm->ifm_index;
	ife->ife_msghdr = ifm;
	if_xaddrs(ifm + 1, ifm->ifm_addrs, &info);
	ife->ife_addr = (struct sockaddr_dl *)info.rti_info[RTAX_IFP];

	return (ife);
}
