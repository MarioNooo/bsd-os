/*	BSDI route.c,v 2.26 2002/01/07 21:10:56 polk Exp	*/

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

#ifndef lint
static char sccsid[] = "@(#)route.c	8.5 (Berkeley) 11/8/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_token.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#define  KERNEL
#include <net/route.h>
#undef KERNEL
#include <netinet/in.h>

#ifdef X25
#include <netccitt/x25.h>
#endif

#ifdef XNS
#include <netns/ns.h>
#endif

#include <err.h>
#include <errno.h>
#include <irs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

#define kget(p, d) (kread((u_long)(p), &(d), sizeof (d)))

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	u_int	b_mask;
	char	b_val;
} bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_REJECT,	'R' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_DONE,	'd' }, /* Completed -- for routing messages only */
	{ RTF_MASK,	'm' }, /* Mask Present -- for routing messages only */
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_STATIC,	'S' },
	{ RTF_BLACKHOLE,'B' },
	{ RTF_CLONED,	'c' },
	{ RTF_PROTO1,	'1' },
	{ RTF_PROTO2,	'2' },
	{ RTF_DEFAULT,	'd' }, /* Neighbor is a router */
	{ RTF_ISAROUTER, 'r' }, /* Neighbor is a router */
	{ RTF_TUNNEL,	'T' }, /* IPv6 tunnel bit */
	{ RTF_AUTH,	'A' }, /* IPSEC authenticated tunnel bit */
	{ RTF_CRYPT,	'E' }, /* IPSEC encrypted tunnel bit */
	{ 0 }
};

static union {
	struct	sockaddr u_sa;
	u_short	u_data[128];
} pt_u;

int	do_rtent = 0;
struct	rtentry rtentry;
struct	radix_node rnode;
struct	radix_mask rmask;

static struct	sockaddr *kgetsa __P((struct sockaddr *));
static void	p_tree __P((struct radix_node *));
static void	p_rtnode __P(());
static void	np_rtentry __P((struct rt_msghdr *));
static char	*p_flags __P((u_int));
static void	p_rtentry __P((struct rtentry *));
static void	pr_rthdr __P((int));
static void	p_dstgw __P((struct sockaddr *, struct sockaddr *,
		   int, struct sockaddr *));

static struct data_info rt_tables_info = {
    "_rt_tables",
    NULL, 0,
    rt_tables, sizeof(rt_tables)
};

/*
 * Print routing tables.
 */
void
routepr(void)
{
	struct radix_node_head *rnh, head;
	struct rt_msghdr *rtm;
	size_t needed;
	int i;
	int mib[] = { CTL_NET, PF_ROUTE, NET_ROUTE_TABLE, AF_UNSPEC, NET_RT_DUMP, 0 };
	char *buf, *next, *lim;

	if (fflag == AF_LOCAL)
		errx(1, "%s does not use routing table", af2name(AF_LOCAL));

	printf("Routing tables\n");

	if (!kflag && !Aflag) {
		mib[3] = fflag;
		if (sysctl(mib, sizeof(mib)/sizeof(*mib), NULL, &needed,
		    NULL, 0) < 0) {
			if (fflag != 0 && errno == EAFNOSUPPORT) {
				warnx("%s support not enabled", af2name(fflag));
				return;
			} else
				err(1, "route-sysctl-estimate");
		}
		if ((buf = malloc(needed)) == NULL) {
			warnx("unable to malloc %u bytes", needed);
			return;
		}
		if (sysctl(mib, sizeof(mib)/sizeof(*mib), buf, &needed,
		    NULL, 0) < 0)
			err(1, "sysctl of routing table");
		lim  = buf + needed;
		for (next = buf; next < lim; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			np_rtentry(rtm);
		}
		return;
	}

	if (skread("routepr", &rt_tables_info))
		return;

	/* One specific family */
	if (fflag != 0) {
		if ((rnh = rt_tables[fflag]) == NULL) {
			warnx("%s support not enabled", af2name(fflag));
			return;
		}
		if (kget(rnh, head))
			return;
		printf("\n%s:\n", af2name(fflag));
		do_rtent = 1;
		pr_rthdr(fflag);
		p_tree(head.rnh_treetop);
		return;
	}

	/* All families */
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) == NULL)
			continue;
		if (kget(rnh, head))
			return;
		if (i == AF_UNSPEC) {
			if (Aflag && fflag == AF_UNSPEC) {
				printf("Netmasks:\n");
			}
		} else {
			printf("\n%s:\n", af2name(i));
			do_rtent = 1;
			pr_rthdr(i);
			p_tree(head.rnh_treetop);
		}
	}
}


/* column widths; each followed by one space */
#define WID_DST 19		/* width of destination column */
#define WID_GW 18		/* width of gateway column */
int wid_dst, wid_gw;

/*
 * Print header for routing table columns.
 */
static void
pr_rthdr(af)
	int af;
{
	wid_dst = WID_DST;
	wid_gw = WID_GW;

	if (!Oflag && vflag < 2) {
		wid_dst += 6;
		wid_gw += 6;
	} else if (af == AF_INET6) {
		wid_dst += 2;
		wid_gw += 2;
	}

	if (af == AF_INET6) {
		wid_dst += 5;
		wid_gw += 6;
	}

	if (Aflag)
		printf("%-8.8s ","Address");
	printf("%-*.*s %-*.*s %-6.6s",
	    wid_dst, wid_dst, "Destination",
	    wid_gw, wid_gw, "Gateway", "Flags");
	if (Oflag || vflag > 1)
		printf(" %3.3s %8.8s", "Refs", "Use");
	if (!Oflag)
		printf(" %5.5s", "MTU");
	printf(" %s\n", "If");
}

static struct sockaddr *
kgetsa(struct sockaddr *dst)
{

	if (kget(dst, pt_u.u_sa))
		return (NULL);
	if (pt_u.u_sa.sa_len > sizeof (pt_u.u_sa) &&
	    kread((u_long)dst, pt_u.u_data, pt_u.u_sa.sa_len))
		return (NULL);
	return (&pt_u.u_sa);
}

static void
p_tree(struct radix_node *rn)
{

again:
	if (kget(rn, rnode))
		return;
	if (rnode.rn_b < 0) {
		if (Aflag)
			printf("%-8lx ", (u_long)rn);
		if (rnode.rn_flags & RNF_ROOT) {
			if (Aflag)
				printf("(root node)%s",
				    rnode.rn_dupedkey ? " =>\n" : "\n");
		} else if (do_rtent) {
			if (kget(rn, rtentry))
				return;
			p_rtentry(&rtentry);
			if (Aflag)
				p_rtnode();
		} else {
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_key),
			    NULL, 0, 44);
			putchar('\n');
		}
		if ((rn = rnode.rn_dupedkey))
			goto again;
	} else {
		if (Aflag && do_rtent) {
			printf("%-8lx ", (u_long)rn);
			p_rtnode();
		}
		rn = rnode.rn_r;
		p_tree(rnode.rn_l);
		p_tree(rn);
	}
}


static void
p_rtnode(void)
{
	struct radix_mask *rm = rnode.rn_mklist;
	char nbuf[20];

	if (rnode.rn_b < 0) {
		if (rnode.rn_mask) {
			printf("\t  mask ");
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_mask),
				   NULL, 0, -1);
		} else if (rm == NULL)
			return;
	} else {
		(void)sprintf(nbuf, "(%d)", rnode.rn_b);
		printf("%6.6s %8lx : %8lx", nbuf,
		    (u_long)rnode.rn_l, (u_long)rnode.rn_r);
	}
	while (rm) {
		if (kget(rm, rmask))
			return;
		(void)sprintf(nbuf, " %d refs, ", rmask.rm_refs);
		printf(" mk = %8lx {(%d),%s",
		    (u_long)rm, -1 - rmask.rm_b,
		    rmask.rm_refs ? nbuf : " ");
		if (rmask.rm_flags & RNF_NORMAL) {
			struct radix_node rnode_aux;
			printf(" <normal>, ");
			if (kget(rmask.rm_leaf, rnode_aux))
				return;
			p_sockaddr(
			    kgetsa((struct sockaddr *)rnode_aux.rn_mask),
			    NULL, 0, -1);
		} else
		    p_sockaddr(kgetsa((struct sockaddr *)rmask.rm_mask),
			NULL, 0, -1);
		putchar('}');
		if ((rm = rmask.rm_mklist))
			printf(" ->");
	}
	putchar('\n');
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(s) (s = (struct sockaddr *) (ROUNDUP((s)->sa_len) + (caddr_t) sa))

static void
np_rtentry(struct rt_msghdr *rtm)
{
	struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
#ifdef notdef
	static int masks_done, banner_printed;
#endif
	static int old_af;
	int af = 0;

#ifdef notdef
	/* for the moment, netmasks are skipped over */
	if (!banner_printed) {
		printf("Netmasks:\n");
		banner_printed = 1;
	}
	if (masks_done == 0) {
		if (rtm->rtm_addrs != RTA_DST ) {
			masks_done = 1;
			af = sa->sa_family;
		}
	} else
#endif
		af = sa->sa_family;
	if (af != old_af) {
		printf("\n%s:\n", af2name(af));
		pr_rthdr(af);
		old_af = af;
	}
	if (rtm->rtm_addrs == RTA_DST) {
		p_sockaddr(sa, NULL, 0, wid_dst);
		printf("%-*.*s %-6.6s", wid_gw, wid_gw, filler,
		    p_flags(rtm->rtm_flags));
		if (Oflag || vflag > 1)
			printf(" %3u %8qu", rtm->rtm_refcnt,
			    (u_quad_t)rtm->rtm_use);
		if (!Oflag)
			printf(" %5lu", (u_long)rtm->rtm_rmx.rmx_mtu);
	} else {
	        struct sockaddr *addr, *gate, *mask, *ifaddr = (struct sockaddr *) 0;

		/* Destination */
		if (rtm->rtm_addrs & RTA_DST) {
			addr = sa;
		} else {
			addr = 0;
		}

		/* Gateway */
		if (rtm->rtm_addrs & RTA_GATEWAY) {
			ADVANCE(sa);
			gate = sa;
		} else {
			gate = 0;
		}

		/* Netmask */
		if (rtm->rtm_addrs & RTA_NETMASK) {
			ADVANCE(sa);
			mask = sa->sa_len ? sa : 0;
		} else {
			mask = 0;
		}

		if (rtm->rtm_addrs & RTA_GENMASK) {
			ADVANCE(sa);
		}

		/* Print what we have so far, dest and gateway */
		p_dstgw(addr, mask, rtm->rtm_flags, gate);

		printf("%-6.6s", p_flags(rtm->rtm_flags));
		if (Oflag || vflag > 1)
			printf(" %3u %8qu", rtm->rtm_refcnt,
			    (u_quad_t)rtm->rtm_use);
		if (!Oflag)
			printf(" %5lu", (u_long)rtm->rtm_rmx.rmx_mtu);

		if (rtm->rtm_addrs & RTA_IFP) {
			struct sockaddr_dl *sdl;

			ADVANCE(sa);
			sdl = (struct sockaddr_dl *) sa;
			if (sdl->sdl_nlen) {
				printf(" %.*s",
				       sdl->sdl_nlen,
				       sdl->sdl_data);
			}
		}

		if (rtm->rtm_addrs & RTA_IFA) {
			ADVANCE(sa);
			if (!Oflag && vflag) {
				ifaddr = sa;
			}
		}

		if (rtm->rtm_addrs & RTA_AUTHOR) {
			ADVANCE(sa);
		}

		if (rtm->rtm_addrs & RTA_BRD) {
			ADVANCE(sa);
			if (!Oflag && vflag) {
				ifaddr = sa;
			}
		}
		if (ifaddr) {
			printf(" ");
			p_sockaddr(sa, 0, RTF_HOST, -1);
		}
	}
	putchar('\n');
}

int
prefix_length(mask, af)
	struct sockaddr *mask;
	int af;
{
	int nbytes;
	u_int8_t *ap, c;
	int nbits = 0;

	switch (af) {
	case AF_INET:
		nbytes = mask->sa_len - 4;
		if (nbytes > sizeof(struct in_addr))
			nbytes = sizeof(struct in_addr);
		ap = (u_int8_t *)&((struct sockaddr_in *)mask)->sin_addr;
		break;
	case AF_INET6:
		nbytes = mask->sa_len - 8;
		if (nbytes > sizeof(struct in6_addr))
			nbytes = sizeof(struct in6_addr);
		ap = (u_int8_t *)&((struct sockaddr_in6 *)mask)->sin6_addr;
		break;
	default:
		return(-1);
	}
	/* Step 1: count number of full bytes */
	while (nbytes > 0 && *ap == 0xff) {
		nbits += 8;
		ap++;
		nbytes--;
	}
	if (nbytes <= 0)
		return (nbits);
	if ((c = *ap)) {
		/* Step 2: count the bits in the byte */
		while (c & 0x80) {
			nbits++;
			c <<= 1;
		}
		/* If any more are set, they aren't contiguous... */
		if (c)
			return(-1);
		--nbytes;
		ap++;
	}
	/* The rest of the bytes should all be zero */
	while (nbytes > 0) {
		if (*ap)
			return(-1);
		++ap;
		--nbytes;
	}
	return(nbits);
}

void
fillscopeid(struct sockaddr *sa)
{
	struct sockaddr_in6 *sin6;

	if (sa->sa_family != AF_INET6)
		return;
	sin6 = (struct sockaddr_in6 *)sa;
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) ||
	    IN6_IS_ADDR_MC_LINKLOCAL(&sin6->sin6_addr)) {
		if (sin6->sin6_scope_id == 0)
			sin6->sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
		sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
	}
}

char *
g_sockaddr(char *workbuf, int wsize, struct sockaddr *sa, struct sockaddr *mask, int flags)
{
	char *cp = workbuf, *cplim;

#ifdef NI_WITHSCOPEID
	const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflags = NI_NUMERICHOST;
#endif
	switch(sa->sa_family) {
	case AF_INET:
#ifndef IPV4CIDR
	    {
		struct sockaddr_in *sinp = (struct sockaddr_in *)sa;

		if (sinp->sin_addr.s_addr == INADDR_ANY)
			cp = "default";
		else if (flags & RTF_HOST)
			cp = routename(sinp->sin_addr.s_addr);
		else if (mask)
			cp = netname(sinp->sin_addr.s_addr,
			  ntohl(((struct sockaddr_in *)mask)->sin_addr.s_addr));
		else
			cp = netname(sinp->sin_addr.s_addr, 0L);
		break;
	    }
#endif /* IPV4CIDR */
	case AF_INET6:
	    {
		int i;
		u_int8_t buf[sa->sa_len];

		if (mask) {
			u_int8_t *p =
			    (u_int8_t *)((struct sockaddr *)buf)->sa_data;
			u_int8_t *a = (u_int8_t *)sa->sa_data;
			u_int8_t *m = (u_int8_t *)mask->sa_data;

			if (mask->sa_len > sa->sa_len) {
				warnx("bogus mask length");
				mask->sa_len = sa->sa_len;
			}

			i = mask->sa_len -
			    ((unsigned)&mask->sa_data - (unsigned)mask);

			memset(&buf, 0, sizeof(buf));

			((struct sockaddr *)buf)->sa_len = sa->sa_len;
			((struct sockaddr *)buf)->sa_family = sa->sa_family;

			while(i-- > 0)
				*(p++) = *(a++) & *(m++);

			sa = (struct sockaddr *)buf;
		}

		if (sa->sa_family == AF_INET6) {
			if (!memcmp(&((struct sockaddr_in6 *)sa)->sin6_addr,
			    &in6addr_any, sizeof(struct in6_addr)) &&
			    (mask == NULL || mask->sa_len == 0)) {
				cp = "default";
				break;
			}
		}
#ifdef IPV4CIDR
		if (sa->sa_family == AF_INET) {
			if (!((struct sockaddr_in *)sa)->sin_addr.s_addr) {
				cp = "default";
				break;
			}
		}
#endif /* IPV4CIDR */

		fillscopeid(sa);
		if (getnameinfo(sa, sa->sa_len, workbuf, wsize - 5,
		    NULL, 0, nflag ? niflags : 0)) {
			warnx("g_sockaddr: getnameinfo(sa) failed");
			return(NULL);
		}

		if (mask) {
			cp = workbuf + strlen(workbuf);

			if ((i = prefix_length(mask, sa->sa_family)) < 0) {
				strcpy(cp, "/?? (netmask ");
				cp += strlen(cp);

				mask->sa_family = sa->sa_family;
				mask->sa_len = sa->sa_len;

				fillscopeid(mask);
				if (getnameinfo(mask, mask->sa_len, cp,
				    wsize - strlen(workbuf) - 1,
				    NULL, 0, nflag ? niflags : 0)) {
					warnx("g_sockaddr: "
					    "getnameinfo(mask) failed");
					return(NULL);
				}

				strcat(cp, ")");
			} else
				sprintf(cp, "/%d", i);
		}

		cp = workbuf;
		break;
	    }

#ifdef XNS
	case AF_NS:
		cp = ns_print(sa);
		break;
#endif

#ifdef X25
	case AF_CCITT:
	    {
		struct sockaddr_x25 *x25p = (struct sockaddr_x25 *)sa;
		cp += sprintf(cp, "%d:", x25p->x25_net);
		for (cplim = x25p->x25_addr;
		    *cplim && cplim < &x25p->x25_addr[16]; ++cplim)
			*cp++ = *cplim;
		*cp++ = '\0';
		cp = workbuf;
		break;
	    }
#endif
	
	case AF_LINK:
	    {
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;

		if (sdl->sdl_nlen == 0 && sdl->sdl_alen == 0 &&
		    sdl->sdl_slen == 0) {
			(void)sprintf(workbuf, "link#%d", sdl->sdl_index);
			break;
		}
		switch (sdl->sdl_type) {
		case IFT_L2VLAN:
		case IFT_ETHER:
		case IFT_FDDI:
		case IFT_IEEE80211:
		    {
			int i;
			u_char *lla = (u_char *)sdl->sdl_data +
			    sdl->sdl_nlen;

			cplim = "";
			for (i = 0; i < sdl->sdl_alen; i++, lla++) {
				cp += sprintf(cp, "%s%x", cplim, *lla);
				cplim = ":";
			}
			cp = workbuf;
			break;
		    }
		case IFT_ISO88025:
		    {
			int i;
			int nseg;
			struct token_max_hdr *addr;

			addr = (struct token_max_hdr *)(sdl->sdl_data +
			    sdl->sdl_nlen);
			for (i=0; i<ISO88025_ADDR_LEN; i++)
				cp += sprintf(cp, "%x:", 
				    addr->hdr.token_dhost[i]);
			*--cp = NULL;
			if (!HAS_ROUTE(addr))
				goto addr_done;
			nseg = addr->rif.rcf0 & RCF0_LEN_MASK;
			if (nseg <= 2) 
				goto addr_done;
			nseg = (nseg >> 1) - 1;
			if ((addr->rif.rcf1 & RCF1_DIRECTION) != 0)
				cp += sprintf(cp, "/r");
			else
				cp += sprintf(cp, "/f");
			for (i = 0; i < nseg; i++)
				cp += sprintf(cp, ":%x",
				    ntohs(addr->rif.rseg[i]));
addr_done:		cp = workbuf;
			break;
		    }
		default:
			cp = link_ntoa(sdl);
			break;
		}
		break;
	    }

	default:
	    {
		u_char *s = (u_char *)sa->sa_data, *slim;

		slim =  sa->sa_len + (u_char *) sa;
		cplim = cp + wsize - 6;
		cp += sprintf(cp, "(%d)", sa->sa_family);
		while (s < slim && cp < cplim) {
			cp += sprintf(cp, " %02x", *s++);
			if (s < slim)
			    cp += sprintf(cp, "%02x", *s++);
		}
		cp = workbuf;
	    }
	}

	return(cp);
}

void
p_sockaddr(struct sockaddr *sa, struct sockaddr *mask, int flags, int width)
{
	char workbuf[128], *cp;

	cp = g_sockaddr(workbuf, sizeof(workbuf), sa, mask, flags);
	if (cp == NULL)
		return;

	if (width < 0 )
		printf("%s ", cp);
	else if (nflag || Pflag)
		printf("%-*s ", width, cp);
	else
		printf("%-*.*s ", width, width, cp);

	return;
}

static void
p_dstgw(struct sockaddr *sa, struct sockaddr *mask, int flags,
	struct sockaddr *ga)
{
	char dstbuf[128], gwbuf[128];
	char *dst, *gw;
	int dstlen = wid_dst, gwlen = wid_gw;

	/* use the gwbuf for the working buffer */
	if ((dst = g_sockaddr(gwbuf, sizeof(gwbuf), sa, mask, flags)) != NULL)
		/* Copy to the dstbuf */
		strncpy(dstbuf, dst, sizeof(dstbuf));
	else
		dstbuf[0] = 0;
	dst = dstbuf;
	if ((gw = g_sockaddr(gwbuf, sizeof(gwbuf), ga, NULL, RTF_HOST))
	    == NULL) {
		gwbuf[0] = 0;
		gw =gwbuf;
	}

	
	/* If together dst and gw both fit into both columns adjust */
	if (strlen(dst) + strlen(gw) <= wid_dst + wid_gw) {
		/* If dest overflows move gw to the right */
		if (strlen(dst) >= wid_dst) {
			dstlen = strlen(dst);
			gwlen = (wid_dst + wid_gw) - strlen(dst);
		/* If gw overflows move gw to the left */
		} else if (strlen(gw) >= wid_gw) {
			dstlen = (wid_dst + wid_gw) - strlen(gw);
			gwlen = strlen(gw);
		}
	} else if ( nflag || Pflag ) {
		/* if -n or -P show everything whether it fits or not */
		dstlen = strlen(dst) > wid_dst ? strlen(dst): wid_dst;
		gwlen = strlen(gw) > (wid_dst + wid_gw) - dstlen ?
		    strlen(gw): (wid_dst + wid_gw) - dstlen;
	} else if (strlen(dst) > wid_dst && strlen(gw) < wid_gw) {
		/* gw fits so show it all and trunc dst */
		dstlen = (wid_dst + wid_gw) - strlen(gw);
		gwlen = strlen(gw);
	} else if (strlen(dst) < wid_dst && strlen(gw) > wid_gw) {
		/* dst fits so show it all and trunc gw */
		dstlen = strlen(dst);
		gwlen = (wid_dst + wid_gw) - strlen(dst);
	}
	printf("%-*.*s %-*.*s ", dstlen, dstlen, dst, gwlen, gwlen, gw);
        return;
}

static char *
p_flags(u_int f)
{
	static char name[17];
	char *flags;
	struct bits *p = bits;

	for (flags = name; p->b_mask; p++)
		if (p->b_mask & f)
			*flags++ = p->b_val;
	*flags = '\0';
	return name;
}

static void
p_rtentry(struct rtentry *rt)
{
	static struct ifnet ifnet, *lastif;
	static char name[16];
	struct sockaddr *sa;
	union {
		struct sockaddr sa;
		struct sockaddr_in sin; 
		struct sockaddr_in6 sin6; 
	} addr, mask;

	memset(&addr, 0, sizeof(addr));
	memset(&mask, 0, sizeof(mask));

	if ((sa = kgetsa(rt_key(rt))))
		memcpy(&addr, sa, sa->sa_len);

	if (rt_mask(rt) != NULL && (sa = kgetsa(rt_mask(rt))) != NULL) {
		memcpy(&mask, sa, sa->sa_len);
		p_sockaddr((struct sockaddr *)&addr, (struct sockaddr *)&mask,
		    rt->rt_flags, wid_dst);
	} else
		p_sockaddr((struct sockaddr *)&addr, NULL, rt->rt_flags,
		    wid_dst);
	p_sockaddr(kgetsa(rt->rt_gateway), NULL, RTF_HOST, wid_gw);
	printf("%-6.6s", p_flags(rt->rt_flags));
	if (Oflag || vflag > 1)
		printf(" %3u %8qu", rt->rt_refcnt, (u_quad_t)rt->rt_use);
	if (!Oflag)
		printf(" %5lu", (u_long)rt->rt_rmx.rmx_mtu);
	if (rt->rt_ifp) {
		if (rt->rt_ifp != lastif) {
			if (kget(rt->rt_ifp, ifnet))
				return;
			if (kread((u_long)ifnet.if_name, name, 16))
				return;
			lastif = rt->rt_ifp;
		}
		printf(" %.*s%d", IFNAMSIZ, name, ifnet.if_unit);
		if (!Oflag && vflag) {
			static struct ifaddr ifaddr, *lastifa;
			struct sockaddr *ifa;

			if (rt->rt_ifa != lastifa) {
				if (kget(rt->rt_ifa, ifaddr))
					return;
				ifa = (ifnet.if_flags & IFF_POINTOPOINT) ?
				    kgetsa(ifaddr.ifa_dstaddr) :
					kgetsa(ifaddr.ifa_addr);
				if (ifa) {
					putchar(' ');
					p_sockaddr(ifa, NULL, RTF_HOST, -1);
				}
			}
		}

	}
	printf("%s\n", rt->rt_nodes[0].rn_dupedkey ? " =>" : "");
}

char *
routename(u_long in)
{
	char *cp;
	static char line[MAXHOSTNAMELEN + 1];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = index(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag) {
		hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
			AF_INET);
		if (hp) {
			if ((cp = index(hp->h_name, '.')) &&
			    strcmp(cp + 1, domain) == 0)
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (cp)
		strncpy(line, cp, sizeof(line) - 1);
	else {
#define C(x)	((x) & 0xff)
		in = ntohl(in);
		(void)sprintf(line, "%lu.%lu.%lu.%lu",
		    C(in >> 24), C(in >> 16), C(in >> 8), C(in));
	}
	return (line);
}

static u_long
forgemask(u_long a)
{
	u_long m;

	if (IN_CLASSA(a))
		m = IN_CLASSA_NET;
	else if (IN_CLASSB(a))
		m = IN_CLASSB_NET;
	else
		m = IN_CLASSC_NET;
	return (m);
}

static void
domask(char *dst, u_long addr, u_long mask)
{
	int b, i;

	if (!mask || (forgemask(addr) == mask)) {
		*dst = '\0';
		return;
	}
	i = 0;
	for (b = 0; b < 32; b++)
		if (mask & (1 << b)) {
			int bb;

			i = b;
			for (bb = b+1; bb < 32; bb++)
				if (!(mask & (1 << bb))) {
					i = -1;	/* noncontig */
					break;
				}
			break;
		}
	if (i == -1)
		(void)sprintf(dst, "&0x%lx", mask);
	else
		(void)sprintf(dst, "/%d", 32-i);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(u_long in, u_long mask)
{
	static struct irs_acc *irs_gen;
	static struct irs_nw *irs_nw;
	static char line[MAXHOSTNAMELEN + 1];
	struct nwent *np;
	u_long net, omask;
	u_long i;
	int subnetshift, maskbits;
	u_char addr[4];

#if 0
	if (irs_gen == NULL)
		irs_gen = irs_gen_acc("", NULL);
	if (irs_gen && irs_nw == NULL)
		irs_nw = (*irs_gen->nw_map)(irs_gen);
#else
	irs_gen = NULL;
	irs_nw = NULL;
#endif
	i = ntohl(in);
	omask = mask;
	np = NULL;
	*line = 0;
	if (!nflag && i && irs_nw) {
		if (mask == 0) {
			switch (mask = forgemask(i)) {
			case IN_CLASSA_NET:
				subnetshift = 8;
				break;
			case IN_CLASSB_NET:
				subnetshift = 8;
				break;
			case IN_CLASSC_NET:
				subnetshift = 4;
				break;
			default:
				abort();
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (i &~ mask)
				mask = (long)mask >> subnetshift;
		}
		net = i & mask;
		addr[0] = (0xFF000000 & net) >> 24;
		addr[1] = (0x00FF0000 & net) >> 16;
		addr[2] = (0x0000FF00 & net) >> 8;
		addr[3] = (0x000000FF & net);
		for (maskbits = 32; !(mask & 1); mask >>= 1)
			maskbits--;
		np = (*irs_nw->byaddr)(irs_nw, addr, maskbits, AF_INET);
	}
	if (np != NULL)
		strncat(line, np->n_name, sizeof(line) - 1);
	else if ((i & 0x00ffffff) == 0 && (omask & 0x00ffffff) == 0)
		(void)sprintf(line, "%lu", C(i >> 24));
	else if ((i & 0x0000ffff) == 0 && (omask & 0x0000ffff) == 0)
		(void)sprintf(line, "%lu.%lu", C(i >> 24) , C(i >> 16));
	else if ((i & 0x000000ff) == 0 && (omask & 0x000000ff) == 0)
		(void)sprintf(line, "%lu.%lu.%lu",
		    C(i >> 24), C(i >> 16), C(i >> 8));
	else
		(void)sprintf(line, "%lu.%lu.%lu.%lu",
		    C(i >> 24), C(i >> 16), C(i >> 8), C(i));
	domask(line+strlen(line), i, omask);
	return (line);
}

/*
 * Print routing statistics
 */

#ifdef	defined_in_kernel_headers
static struct rtstat rtstat;
#endif
static int rtstat_mib[] = { CTL_NET, PF_ROUTE, NET_ROUTE_STATS };
static struct data_info rtstat_info = {
	"_rtstat",
	rtstat_mib, sizeof(rtstat_mib)/sizeof(*rtstat_mib),
	&rtstat, sizeof(rtstat)
};

void
rt_stats(void)
{

	if (skread("rt_stats", &rtstat_info))
		return;

	printf("routing:\n");

	printf("\t%qu bad routing redirect%s\n",
		(u_quad_t)rtstat.rts_badredirect, PLURAL(rtstat.rts_badredirect));
	printf("\t%qu dynamically created route%s\n",
		(u_quad_t)rtstat.rts_dynamic, PLURAL(rtstat.rts_dynamic));
	printf("\t%qu new gateway%s due to redirects\n",
		(u_quad_t)rtstat.rts_newgateway, PLURAL(rtstat.rts_newgateway));
	printf("\t%qu destination%s found unreachable\n",
		(u_quad_t)rtstat.rts_unreach, PLURAL(rtstat.rts_unreach));
	printf("\t%qu use%s of a wildcard route\n",
		(u_quad_t)rtstat.rts_wildcard, PLURAL(rtstat.rts_wildcard));
}

#ifdef XNS
static short ns_nullh[] = {0,0,0};
static short ns_bh[] = {-1,-1,-1};

char *
ns_print(struct sockaddr *sa)
{
	struct sockaddr_ns *sns = (struct sockaddr_ns*)sa;
	struct ns_addr work;
	union { union ns_net net_e; u_long long_e; } net;
	u_short port;
	static char mybuf[50], cport[10], chost[25];
	char *host = "";
	char *p; u_char *q;

	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.long_e == 0) {
		if (port)
			(void)sprintf(mybuf, "*.%XH", port);
		else
			(void)sprintf(mybuf, "*.*");
		return (mybuf);
	}

	if (bcmp(ns_bh, work.x_host.c_host, 6) == 0) {
		host = "any";
	} else if (bcmp(ns_nullh, work.x_host.c_host, 6) == 0) {
		host = "*";
	} else {
		q = work.x_host.c_host;
		(void)sprintf(chost, "%02x%02x%02x%02x%02x%02xH",
		    q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++)
			continue;
		host = p;
	}
	if (port)
		(void)sprintf(cport, ".%xH", htons(port));
	else
		*cport = 0;

	(void)sprintf(mybuf,"%lXH.%s%s", (u_long)ntohl(net.long_e), host,
	    cport);
	return(mybuf);
}

char *
ns_phost(struct sockaddr *sa)
{
	struct sockaddr_ns *sns = (struct sockaddr_ns *)sa;
	struct sockaddr_ns work;
	static union ns_net ns_zeronet;
	char *p;

	work = *sns;
	work.sns_addr.x_port = 0;
	work.sns_addr.x_net = ns_zeronet;

	p = ns_print((struct sockaddr *)&work);
	if (strncmp("0H.", p, 3) == 0)
		p += 3;
	return(p);
}
#endif
