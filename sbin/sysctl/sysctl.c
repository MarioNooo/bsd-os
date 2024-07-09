/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI sysctl.c,v 2.37 2003/05/10 00:00:50 giff Exp
 */

/*
 * Copyright (c) 1993
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
static char copyright[] =
"@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)sysctl.c	8.5 (Berkeley) 5/9/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/gmon.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/license.h>
#include <sys/evcnt.h>
#include <vm/vm_param.h>
#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_fddi.h>
#include <net/if_802_11.h>
#include <net/if_token.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/pim6_var.h>
#include <netinet6/ipsec.h>

#include <netkey/key_var.h>

#include <netkey/key_var.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "license.h"
#include "extern.h"

struct ctlname topname[] = CTL_NAMES;
struct ctlname kernname[] = CTL_KERN_NAMES;
struct ctlname vmname[] = CTL_VM_NAMES;
struct ctlname netname[] = CTL_NET_NAMES;
struct ctlname hwname[] = CTL_HW_NAMES;
struct ctlname username[] = CTL_USER_NAMES;
struct ctlname *debugname;
struct ctlname *statname;
struct ctlname statdata[] = STAT_NAMES;
struct ctlname *vfsname;
#ifdef CTL_MACHDEP_NAMES
struct ctlname machdepname[] = CTL_MACHDEP_NAMES;
#endif
char names[BUFSIZ];
int lastused;

struct list {
	struct	ctlname *list;
	int	size;
	int	dup6;
};
struct list toplist = { topname, CTL_MAXID };
struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ kernname, KERN_MAXID },	/* CTL_KERN */
	{ vmname, VM_MAXID },		/* CTL_VM */
	{ 0, 0 },			/* CTL_VFS */
	{ netname, NET_MAXID },		/* CTL_NET */
	{ 0, 0 },			/* CTL_DEBUG */
	{ hwname, HW_MAXID },		/* CTL_HW */
#ifdef CTL_MACHDEP_NAMES
	{ machdepname, CPU_MAXID },	/* CTL_MACHDEP */
#else
	{ 0, 0 },			/* CTL_MACHDEP */
#endif
	{ username, USER_MAXID },	/* CTL_USER */
	{ 0, 0 },			/* CTL_STAT */
};

static char *th[] = {
	"first", "second", "third", "fourth", "fifth", "sixth", "seventh"
};
#define	TH_FIRST	th[0]
#define	TH_SECOND	th[1]
#define	TH_THIRD	th[2]
#define	TH_FOURTH	th[3]
#define	TH_FIFTH	th[4]

int	flags, nflag, wflag;
int	have_seen_INET4;

struct ctlname vfs_generic_name[] = VFS_GENERIC_NAMES;
struct list vfslist = { vfs_generic_name, VFS_MAXID };
struct list statlist = { statdata, STAT_MAXID };

/*
 * Variables requiring special processing.
 */
#define	CLOCK		0x00000001
#define	BOOTTIME	0x00000002
#define	CONSDEV		0x00000004
#define	SLOWHZ		0x00000008	/* convert seconds to/from PR_SLOWHZ */
#define	ISUINT		0x00000010	/* print as unsigned int not int */
#define	RTPROF		0x00000020	/* print RT profile structure */

#define	FLAG_A		0x01	/* The A Flag was given */
#define	FLAG_a		0x02	/* The a Flag was given */
#define	FLAG_s		0x04	/* The s Flag was given */

void	debuginit __P((void));
void	statinit __P((void));
int	findname __P((char *, char *, char **, struct list *));
void	linkinit __P((void));
void	listall __P((char *, struct list *));
int	parse __P((char *, int));
int	sysctl_inet __P((char *, char **, int *, int, int *, int *));
int	sysctl_stat __P((char *, char **, int *, int, int *, int *));
int	sysctl_interface __P((char *, char **, int, int *, int, int *, 
	    int *));
int	sysctl_link __P((char *, char **, int *, int, int *, int *));
int	sysctl_key __P((char *, char **, int *, int, int *));
int	sysctl_route __P((char *, char **, int *, int, int *));
int	sysctl_socket __P((char *, char **, int *, int *));
void	usage __P((void));
void	vfsinit __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	int ch, eval, lvl1;

	while ((ch = getopt(argc, argv, "Aansw")) != EOF) {
		switch (ch) {
		case 'A':
			flags |= FLAG_A;
			break;
		case 'a':
			flags |= FLAG_a;
			break;
		case 's':
			flags |= FLAG_s;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * We never attempt to return the name paramter since it is the
	 * name we use for mib[1] and it just makes things look complicated
	 */
	statdata[STAT_NAME].ctl_name = 0;
	statdata[STAT_NAME].ctl_type = 0;

	if (argc == 0 && flags & (FLAG_A | FLAG_a)) {
		debuginit();
		linkinit();
		vfsinit();
		statinit();
		for (lvl1 = 1; lvl1 < CTL_MAXID; lvl1++)
			listall(topname[lvl1].ctl_name, &secondlevel[lvl1]);
		exit(0);
	}
	if (argc == 0)
		usage();
	for (eval = 0; *argv != NULL; ++argv) {
		have_seen_INET4 = 0;
		if (parse(*argv, 1))
			eval = 1;
	}
	exit(eval);
}

/*
 * List all variables known to the system.
 */
void
listall(prefix, lp)
	char *prefix;
	struct list *lp;
{
	int lvl2;
	char name[BUFSIZ];

	if (lp->list == 0)
		return;
	for (lvl2 = 0; lvl2 < lp->size; lvl2++) {
		if (lp->list[lvl2].ctl_name == 0)
			continue;
		(void)snprintf(name, sizeof(name),
		    "%s.%s", prefix, lp->list[lvl2].ctl_name);	
		(void)parse(name, flags);
	}
}

/*
 * Parse a name into a MIB entry.  Lookup and print out the MIB entry if it
 * exists.  Set a new value if requested.
 */
int
parse(string, flags)
	char *string;
	int flags;
{
	license_t lic, nlic;
	struct list *lp;
	struct vfsconf vfc;
	size_t size;
	u_int intval;
	u_quad_t quadval;
	void *newval;
	int indx, len, newsize, special, state, type;
	int mib[CTL_MAXNAME];
	char *cp, *bufp, buf[BUFSIZ];

	newval = NULL;
	newsize = special = 0;

	bufp = buf;
	snprintf(buf, BUFSIZ, "%s", string);
	if ((cp = strchr(string, '=')) != NULL) {
		if (!wflag)
			errx(2, "must specify -w to set variables");
		*strchr(buf, '=') = '\0';
		*cp++ = '\0';
		while (isspace(*cp))
			cp++;
		newval = cp;
		newsize = strlen(cp);
	}
	if ((indx = findname(string, "top", &bufp, &toplist)) == -1)
		return (1);
	mib[0] = indx;
	if (indx == CTL_VFS)
		vfsinit();
	if (indx == CTL_DEBUG)
		debuginit();
	if (indx == CTL_STAT)
		statinit();
	lp = &secondlevel[indx];
	if (lp->list == 0) {
		warnx("%s: class is not implemented", topname[indx].ctl_name);
		return (1);
	}
	if (bufp == NULL) {
		listall(topname[indx].ctl_name, lp);
		return (0);
	}
	if ((indx = findname(string, TH_SECOND, &bufp, lp)) == -1)
		return (1);
	mib[1] = indx;
	type = lp->list[indx].ctl_type;
	len = 2;
	switch (mib[0]) {
	case CTL_KERN:
		switch (mib[1]) {
		case KERN_PROF:
			mib[2] = GPROF_STATE;
			size = sizeof(state);
			if (sysctl(mib, 3, &state, &size, NULL, 0) < 0) {
				if (!(flags & FLAG_A))
					return (0);
				if (!nflag)
					fprintf(stderr, "%s: ", string);
				warnx("this kernel not compiled for profiling");
				return (1);
			}
			if (!nflag)
				printf("%s: %s\n", string,
				    state == GMON_PROF_OFF ? "off" : "running");
			return (0);
		case KERN_VNODE:
		case KERN_FILES:
			if (!(flags & FLAG_A))
				return (0);
			warnx("use pstat to view %s information", string);
			return (1);
		case KERN_PROC:
			if (!(flags & FLAG_A))
				return (0);
			warnx("use ps to view %s information", string);
			return (1);
		case KERN_CLOCKRATE:
			special |= CLOCK;
			break;
		case KERN_BOOTTIME:
			special |= BOOTTIME;
			break;
		case KERN_SYSVIPC:
			if (!(flags & FLAG_A))
				return (0);
			warnx("use ipcs to view %s information", string);
			return (1);
		case KERN_LICENSE:
			size = sizeof(lic);
			if (newval) {
				if ((errno =
				    license_decode(newval, &nlic)) != 0) {
					if (!nflag)
						fprintf(stderr, "%s: ", string);
					warnx("invalid license: %s", newval);
					return (1);
				}
			}
			if (sysctl(mib, 2, &lic, &size,
			    newval ? &nlic : NULL, newval ? size : 0) < 0) {
				if (!(flags & FLAG_A))
					return (0);
				if (!nflag)
					fprintf(stderr, "%s: ", string);
				if (errno == EOPNOTSUPP)
					warnx("this kernel not compiled for licenses");
				else if (errno == EINVAL)
					warnx("invalid license: %s", newval);
				else
					warn(NULL);
				return (1);
			}
			if (!nflag)
				printf("%s: ", string);
			printf("%s", license_encode(&lic));
			if (newval)
				printf(" -> %s", (char *)newval);
			printf("\n");
			return (0);
		case KERN_HOSTID:
			special |= ISUINT;
			break;
		}
		break;
	case CTL_HW:
		switch (mib[1]) {
		case HW_PHYSMEM:
			special |= ISUINT;
			break;
		case HW_USERMEM:
			special |= ISUINT;
			break;
		}
		break;
	case CTL_VM:
		switch (mib[1]) {
		case VM_LOADAVG: {
			double loads[3];

			getloadavg(loads, 3);
			if (!nflag)
				printf("%s: ", string);
			printf("%.2f %.2f %.2f\n",
			    loads[0], loads[1], loads[2]);
			return (0);
		}
		case VM_TOTAL:
		case VM_CNT:
		case VM_SWAPSTATS:
			if (!(flags & FLAG_A))
				return (0);
			warnx("Use vmstat or systat to view %s information",
			    string);
			return (1);
		default:
			break;
		}
		break;
	case CTL_NET:
		switch (mib[1]) {
		case PF_UNSPEC:		/* socket layer */
			len = sysctl_socket(string, &bufp, mib, &type);
			if (len >= 0)
				goto doit;
			return (0);
		case PF_INET:
		case PF_INET6:
			len = sysctl_inet(string,
			    &bufp, mib, flags & FLAG_A, &type, &special);
			if (len >= 0)
				goto doit;
			return (0);
		case PF_ROUTE:
			len = sysctl_route(string,
			    &bufp, mib, flags & FLAG_A, &type);
			if (len >= 0)
				goto doit;
			return (0);
		case PF_LINK:
			linkinit();
			len = sysctl_link(string,
			    &bufp, mib, flags & FLAG_A, &type, NULL);
			if (len >= 0)
				goto doit;
			return (0);
		case PF_KEY:
			len = sysctl_key(string, &bufp, mib, flags, &type);
			if (len >= 0)
				goto doit;
			return (0);
		}
		if (!(flags & FLAG_A))
			return (0);
		warnx("Use netstat to view %s information", string);
		return (1);
	case CTL_DEBUG:
		mib[2] = CTL_DEBUG_VALUE;
		len = 3;
		break;
	case CTL_MACHDEP:
#ifdef CPU_CONSDEV
		if (mib[1] == CPU_CONSDEV)
			special |= CONSDEV;
#endif
		break;
	case CTL_VFS:
		/*
		 * vfs.generic is a special case:
		 * If no third level, then assume we're here due to listall()
		 * and print all but first two special case variables.
		 */
		if (mib[1] == VFS_GENERIC) {
			if (bufp == NULL) {
				listall(string, &vfslist);
				return (-1);
			}
			if ((indx =
			    findname(string, TH_THIRD, &bufp, &vfslist)) == -1)
				return (-1);
			mib[2] = indx;
			type = vfs_generic_name[indx].ctl_type;
			len = 3;
			goto doit;
		}
		mib[3] = mib[1];
		mib[1] = VFS_GENERIC;
		mib[2] = VFS_CONF;
		len = 4;
		size = sizeof(vfc);
		if (sysctl(mib, 4, &vfc, &size, NULL, (size_t)0) < 0) {
			warn("vfs print");
			return (1);
		}
		if (flags == 0 && vfc.vfc_refcount == 0)
			return (0);
		if (!nflag)
			printf("%s has %d mounted instance%s\n",
			    string, vfc.vfc_refcount,
			    vfc.vfc_refcount != 1 ? "s" : "");
		else
			printf("%d\n", vfc.vfc_refcount);
		return (0);
	case CTL_USER:
		break;
	case CTL_STAT:
		len = sysctl_stat(string,
		    &bufp, mib, flags & FLAG_A, &type, &special);
		if (len >= 0)
			goto doit;
		return (0);
	default:
		warnx("illegal top level value: %d", mib[0]);
		return (1);
	}
doit:
	if (bufp) {
		warnx("name %s in %s is unknown", bufp, string);
		return (1);
	}
	if (newval) {
		switch (type) {
		case CTLTYPE_INT:
			intval = strtoul(newval, &cp, 0);
			/*
			 * Check that value is not empty and
			 * has at most one suffix character.
			 */
			if (cp == newval || (cp[0] != '\0' && cp[1] != '\0')) {
				warnx("invalid integer value %s", newval);
				return (1);
			}
			switch (*cp) {
			case '\0':
				break;
			case 'k':
			case 'K':
				intval *= 1024;
				break;
			case 'm':
			case 'M':
				intval *= 1048576;
				break;
			default:
				warnx("invalid integer value %s", newval);
				return (1);
			}
			newval = &intval;
			newsize = sizeof(intval);
			break;
		case CTLTYPE_QUAD:
			quadval = strtouq(newval, &cp, 0);
			/*
			 * Check that value is not empty and
			 * has at most one suffix character.
			 */
			if (cp == newval || (cp[0] != '\0' && cp[1] != '\0')) {
				warnx("invalid integer value %s", newval);
				return (1);
			}
			switch (*cp) {
			case '\0':
				break;
			case 'k':
			case 'K':
				quadval *= 1024;
				break;
			case 'm':
			case 'M':
				quadval *= 1048576;
				break;
			default:
				warnx("invalid integer value %s", newval);
				return (1);
			}
			newval = &quadval;
			newsize = sizeof(quadval);
			break;
		}
	}
	size = BUFSIZ;
	/* for SLOWHZ, convert any new value from seconds to PR_SLOWHZ */
	if (special & SLOWHZ && newsize && *(int *) newval)
		*(int *)newval *= PR_SLOWHZ;
	if (sysctl(mib, len, buf, &size, newsize ? newval : 0, newsize) == -1) {
		if (!(flags & FLAG_A))
			return (0);
		switch (errno) {
		case EOPNOTSUPP:
			warnx("%s: value is not available", string);
			return (1);
		case ENOTDIR:
			warnx("%s: specification is incomplete", string);
			return (1);
		case ENOMEM:
			warnx("%s: type is unknown", string);
			return (1);
		default:
			warn("%s", string);
			return (1);
		}
	}
	if (special & CLOCK) {
		struct clockinfo *clkp = (struct clockinfo *)buf;

		if (!nflag)
			printf("%s: ", string);
		printf(
		    "hz = %d, tick = %d, profhz = %d, stathz = %d\n",
		    clkp->hz, clkp->tick, clkp->profhz, clkp->stathz);
		return (0);
	}
	if (special & BOOTTIME) {
		struct timeval *btp = (struct timeval *)buf;
		time_t then;

		if (!nflag) {
			then = btp->tv_sec;
			printf("%s = %s", string, ctime(&then));
		} else
			printf("%ld\n", btp->tv_sec);
		return (0);
	}
	if (special & CONSDEV) {
		dev_t dev = *(dev_t *)buf;
		char *p;

		if (!nflag) {
			if ((p = devname(dev, S_IFCHR)) == NULL)
				p = "??";
			printf("%s = %s\n", string, p);
		} else
			printf("0x%lx\n", dev);
		return (0);
	}
	if (special & SLOWHZ) {
		/* convert values to seconds */
		*(int *)buf /= PR_SLOWHZ;
		if (newsize && *(int *) newval)
			*(int *)newval /= PR_SLOWHZ;
	}
	if (special & RTPROF) {
		static unsigned int cps = -1;
		double scale;
		char *scales;
		static int cpsmib[] = { CTL_KERN, KERN_CYCLES_PER_SECOND, };
		rt_profile_t *rtp = (rt_profile_t *)buf;

		if (cps == -1) {
			if (flags & FLAG_s)
				cps = 0;
			else {
				size = sizeof(cps);
				if (sysctl(cpsmib, 2, &cps, &size, NULL, 0) < 0)
					cps = 0;
			}
		}

		if (!nflag)
			printf("%s: ", string);
		if (rtp->count == 0)
			printf("0 times\n");
		else if (cps) {
			scale = cps;
			scales = "s";
			if (rtp->min / scale < 0.000001) {
				scale = cps / 1000000;
				scales = "us";
			} else if (rtp->min / scale < 0.01) {
				scale = cps / 1000;
				scales = "ms";
			}
			printf("%u times @ %.3f/%.3f/%.3f %s\n",
			    rtp->count,
			    rtp->min / scale,
			    rtp->sum / (rtp->count * scale),
			    rtp->max / scale, scales);
		} else
			printf("%u times @ %qd/%qd/%qd cycles\n",
			    rtp->count,
			    rtp->min, rtp->sum / rtp->count, rtp->max);
		return (0);
	}
	switch (type) {
	case CTLTYPE_INT:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			if (special & ISUINT)
				printf("%u\n", *(u_int *)buf);
			else
				printf("%d\n", *(int *)buf);
		} else {
			if (!nflag)
				printf("%s: ", string);
			if (special & ISUINT)
				printf("%u -> %u\n", *(u_int *)buf,
				    *(u_int *)newval);
			else
				printf("%d -> %d\n", *(int *)buf,
				    *(int *)newval);
		}
		return (0);
	case CTLTYPE_STRING:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%s\n", buf);
		} else {
			if (!nflag)
				printf("%s: %s -> ", string, buf);
			printf("%s\n", (char *)newval);
		}
		return (0);
	case CTLTYPE_QUAD:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%qd\n", *(quad_t *)buf);
		} else {
			if (!nflag)
				printf("%s: %qd -> ", string, *(quad_t *)buf);
			printf("%qd\n", *(quad_t *)newval);
		}
		return (0);
	case CTLTYPE_STRUCT:
		if (flags == 0) {
			warnx("%s: unknown structure returned", string);
			return (1);
		}
		return (0);
	default:
	case CTLTYPE_NODE:
		if (flags == 0) {
			warnx("%s: unknown type returned", string);
			return (1);
		}
		return (0);
	}
	/* NOTREACHED */
}

/*
 * Initialize the set of debugging names
 */
void
debuginit()
{
	size_t size;
	int mib[3], loc, i;
	int ctl_debug_maxid;

	if (secondlevel[CTL_DEBUG].list != 0)
		return;
	mib[0] = CTL_DEBUG;
	mib[1] = CTL_DEBUG_MAXID;
	mib[2] = CTL_DEBUG_NAME;
	size = sizeof(int);
	if (sysctl(mib, 3, &ctl_debug_maxid, &size, NULL, 0) == -1)
		return;
	if ((debugname = malloc(ctl_debug_maxid * sizeof(struct ctlname))) == 0)
		return;
	secondlevel[CTL_DEBUG].list = debugname;
	secondlevel[CTL_DEBUG].size = ctl_debug_maxid;
	for (loc = lastused, i = 0; i < ctl_debug_maxid; i++) {
		mib[1] = i;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		debugname[i].ctl_name = &names[loc];
		debugname[i].ctl_type = CTLTYPE_INT;
		loc += size;
	}
	lastused = loc;
}

/*
 * Initialize the set of debugging names
 */
void
statinit()
{
	size_t size;
	int mib[3], loc, i;
	int ctl_stat_maxid;

	if (secondlevel[CTL_STAT].list != 0)
		return;
	mib[0] = CTL_STAT;
	mib[1] = STAT_ENTRIES;
	size = sizeof(int);
	if (sysctl(mib, 2, &ctl_stat_maxid, &size, NULL, 0) == -1)
		return;
	if ((statname = malloc(ctl_stat_maxid * sizeof(struct ctlname))) == 0)
		return;
	secondlevel[CTL_STAT].list = statname;
	secondlevel[CTL_STAT].size = ctl_stat_maxid;
	for (loc = lastused, i = 0; i < ctl_stat_maxid; i++) {
		mib[1] = i;
		mib[2] = STAT_NAME;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		statname[i].ctl_name = &names[loc];
		statname[i].ctl_type = CTLTYPE_NODE;
		loc += size;
	}
	lastused = loc;
}

/*
 * Initialize the set of filesystem names
 * Allocalte slot 0 for the generic identifier
 */
void
vfsinit()
{
	struct vfsconf vfc;
	size_t buflen;
	int mib[4], maxtypenum, cnt, loc, size;

	if (secondlevel[CTL_VFS].list != 0)
		return;
	mib[0] = CTL_VFS;
	mib[1] = VFS_GENERIC;
	mib[2] = VFS_MAXTYPENUM;
	buflen = 4;
	if (sysctl(mib, 3, &maxtypenum, &buflen, NULL, (size_t)0) < 0)
		return;
	maxtypenum++;
	if ((vfsname = malloc(maxtypenum * sizeof(*vfsname))) == 0)
		return;
	memset(vfsname, 0, maxtypenum * sizeof(*vfsname));
	loc = lastused;
	strcat(&names[loc], "generic");
	vfsname[0].ctl_name = &names[loc];
	vfsname[0].ctl_type = CTLTYPE_NODE;
	size = strlen(vfsname[0].ctl_name) + 1;
	loc += size;

	mib[2] = VFS_CONF;
	buflen = sizeof(vfc);
	for (cnt = 1; cnt < maxtypenum; cnt++) {
		mib[3] = cnt;
		if (sysctl(mib, 4, &vfc, &buflen, NULL, (size_t)0) < 0) {
			if (errno == EOPNOTSUPP)
				continue;
			warn("vfsinit");
			free(vfsname);
			return;
		}
		strcat(&names[loc], vfc.vfc_name);
		vfsname[cnt].ctl_name = &names[loc];
		vfsname[cnt].ctl_type = CTLTYPE_INT;
		size = strlen(vfc.vfc_name) + 1;
		loc += size;
	}
	lastused = loc;
	secondlevel[CTL_VFS].list = vfsname;
	secondlevel[CTL_VFS].size = maxtypenum;
	return;
}

struct ctlname sockname[] = SOCTL_NAMES;
struct list socklist = { sockname, SOCTL_MAXID };

/*
 * handle socket requests
 */
/* ARGSUSED4 */
int
sysctl_socket(string, bufpp, mib, typep)
	char *string;
	char **bufpp;
	int mib[];
	int *typep;
{
	int indx;

	if (*bufpp == NULL) {
		listall(string, &socklist);
		return (-1);
	}
	if ((indx = findname(string, TH_THIRD, bufpp, &socklist)) == -1)
		return (-1);
	mib[2] = indx;
	*typep = sockname[indx].ctl_type;
	return (3);
}

struct ctlname inetname[] = CTL_IPPROTO_NAMES;
struct ctlname ipname[] = IPCTL_NAMES;
struct ctlname icmpname[] = ICMPCTL_NAMES;
struct ctlname igmpname[] = IGMPCTL_NAMES;
struct ctlname tcpname[] = TCPCTL_NAMES;
struct ctlname udpname[] = UDPCTL_NAMES;
struct ctlname ipv6name[] = IPV6CTL_NAMES;
struct ctlname ipsecname[] = IPSECCTL_NAMES;
struct ctlname icmpv6name[] = ICMPV6CTL_NAMES;
struct list inetlist = { inetname, IPPROTO_MAXID };
struct list inetvars[] = {
	{ ipname, IPCTL_MAXID },	/* ip */
	{ icmpname, ICMPCTL_MAXID },	/* icmp */
	{ igmpname, IGMPCTL_MAXID },	/* igmp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
/*41*/	{ ipv6name, IPV6CTL_MAXID },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
/*50*/	{ ipsecname, IPSECCTL_MAXID },	/* esp - just for backward compat */
	{ ipsecname, IPSECCTL_MAXID },	/* ah */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ icmpv6name, ICMPV6CTL_MAXID },
};

struct ctlname inet6name[] = CTL_IPV6PROTO_NAMES;
struct ctlname ipsec6name[] = IPSEC6CTL_NAMES;
struct ctlname pim6name[] = PIM6CTL_NAMES;
struct list inet6list = { inet6name, IPV6PROTO_MAXID };
struct list inet6vars[] = {
	{ ipv6name, IPCTL_MAXID },	/* ipv6 */
	{ icmpname, ICMPCTL_MAXID },	/* icmpv4 */
	{ igmpname, IGMPCTL_MAXID },	/* igmpv4 */
	{ 0, 0 },
	{ ipname, IPCTL_MAXID },	/* ipv4 */
	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ ipv6name, IPV6CTL_MAXID },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
/*50*/	{ ipsec6name, IPSECCTL_MAXID },	/* esp6 - just for backward compat */
	{ ipsec6name, IPSECCTL_MAXID },	/* ah6 */
	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },	{ 0, 0 },
	{ 0, 0 },	{ 0, 0 },
	{ icmpv6name, ICMPV6CTL_MAXID },
	{ 0, 0 },
/*60*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*70*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*80*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*90*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*100*/	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ pim6name, PIM6CTL_MAXID },	/* pim6 */
};

/*
 * handle internet requests
 */
int
sysctl_inet(string, bufpp, mib, flags, typep, specialp)
	char *string;
	char **bufpp;
	int mib[];
	int flags;
	int *typep;
	int *specialp;
{
	struct list *lp;
	int indx;
	struct list *list;
	int varslen;
	struct list *vars;

	if (mib[1] == PF_INET6) {
		list = &inet6list;
		vars = inet6vars;
		varslen = sizeof(inet6vars)/sizeof(inet6vars[0]);
	} else {
		list = &inetlist;
		vars = inetvars;
		varslen = sizeof(inetvars)/sizeof(inetvars[0]);
		have_seen_INET4 = 1;
	}
	if (*bufpp == NULL) {
#if 0 /* XXX */
		if (mib[1] == PF_INET6 && have_seen_INET4)
			return (-1);
#endif /* XXX */
		listall(string, list);
		return (-1);
	}
	if ((indx = findname(string, TH_THIRD, bufpp, list)) == -1)
		return (-1);
	mib[2] = indx;
	if (indx <= varslen && vars[indx].list != NULL)
		lp = &vars[indx];
	else if (!flags)
		return (-1);
	else {
		warnx("%s: no variables defined for this protocol", string);
		return (-1);
	}
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, TH_FOURTH, bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	switch (mib[2]) {
	case IPPROTO_TCP:
		switch (mib[3]) {
		case TCPCTL_KEEPIDLE:
		case TCPCTL_KEEPINTVL:
		case TCPCTL_MAXPERSISTIDLE:
		case TCPCTL_CONNTIMEO:
			*specialp |= SLOWHZ;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return (4);
}

struct ctlname routename[] = CTL_NET_ROUTE_NAMES;
struct list routelist = { routename, NET_ROUTE_MAXID };

struct ctlname arpname[] = ARPCTL_NAMES;
struct list arplist = { arpname, ARPCTL_MAXID };

/*
 * handle route requests
 */
int
sysctl_route(string, bufpp, mib, flags, typep)
	char *string;
	char **bufpp;
	int mib[];
	int flags;
	int *typep;
{
	int indx;

	if (*bufpp == NULL) {
		listall(string, &routelist);
		return (-1);
	}

	if ((indx = findname(string, TH_THIRD, bufpp, &routelist)) == -1)
		return (-1);

	if (indx == NET_ROUTE_TABLE) {
		if (flags & FLAG_A)
			warnx("Use netstat to view %s information", string);
		return (-1);
	}

	mib[2] = indx;
	if (indx != NET_ROUTE_ARP) {
		*typep = routename[indx].ctl_type;
		return (3);
	}

	if (*bufpp == NULL) {
		listall(string, &arplist);
		return (-1);
	}

	if ((indx = findname(string, TH_FOURTH, bufpp, &arplist)) == -1)
		return (-1);

	mib[3] = indx;
	*typep = arpname[indx].ctl_type;
	return (4);
}

static struct list linklist = { NULL, 0 };

static struct ctlname linktypename[] = CTL_LINK_TYPE_NAMES;
static struct list linktypelist = { linktypename,
    sizeof(linktypename) / sizeof(linktypename[0]) };

#define CTL_LINKPROTO_MAXID	1
static struct ctlname linkprotoname[] = { { "numif", CTLTYPE_INT } };
static struct list linkvars[] = {
	{ 0, 0 },			/* 0x00 */
	{ 0, 0 },			/* 0x01 */
	{ 0, 0 },			/* 0x02 */
	{ 0, 0 },			/* 0x03 */
	{ 0, 0 },			/* 0x04 */
	{ 0, 0 },			/* 0x05 */
	{ 0, 0 },			/* 0x06 */
	{ 0, 0 },			/* 0x07 */
	{ 0, 0 },			/* 0x08 */
	{ 0, 0 },			/* 0x09 */
	{ 0, 0 },			/* 0x0a */
	{ 0, 0 },			/* 0x0b */
	{ 0, 0 },			/* 0x0c */
	{ 0, 0 },			/* 0x0d */
	{ 0, 0 },			/* 0x0e */
	{ 0, 0 },			/* 0x0f */
	{ 0, 0 },			/* 0x10 */
	{ 0, 0 },			/* 0x11 */
	{ 0, 0 },			/* 0x10 */
	{ 0, 0 },			/* 0x13 */
	{ 0, 0 },			/* 0x14 */
	{ 0, 0 },			/* 0x15 */
	{ 0, 0 },			/* 0x16 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x17 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x18 */
	{ 0, 0 },			/* 0x19 */
	{ 0, 0 },			/* 0x1a */
	{ 0, 0 },			/* 0x1b */
	{ 0, 0 },			/* 0x1c */
	{ 0, 0 },			/* 0x1d */
	{ 0, 0 },			/* 0x1e */
	{ 0, 0 },			/* 0x1f */
	{ 0, 0 },			/* 0x20 */
	{ 0, 0 },			/* 0x21 */
	{ 0, 0 },			/* 0x22 */
	{ 0, 0 },			/* 0x23 */
	{ 0, 0 },			/* 0x24 */
	{ 0, 0 },			/* 0x25 */
	{ 0, 0 },			/* 0x26 */
	{ 0, 0 },			/* 0x27 */
	{ 0, 0 },			/* 0x28 */
	{ 0, 0 },			/* 0x29 */
	{ 0, 0 },			/* 0x2a */
	{ 0, 0 },			/* 0x2b */
	{ 0, 0 },			/* 0x2c */
	{ 0, 0 },			/* 0x2d */
	{ 0, 0 },			/* 0x2e */
	{ 0, 0 },			/* 0x2f */
	{ 0, 0 },			/* 0x30 */
	{ 0, 0 },			/* 0x31 */
	{ 0, 0 },			/* 0x32 */
	{ 0, 0 },			/* 0x33 */
	{ 0, 0 },			/* 0x34 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x35 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 0x36 */

	{ 0, 0 },			/* 55 */
	{ 0, 0 },			/* 56 */
	{ 0, 0 },			/* 57 */
	{ 0, 0 },			/* 58 */
	{ 0, 0 },			/* 59 */

	{ 0, 0 },			/* 60 */
	{ 0, 0 },			/* 61 */
	{ 0, 0 },			/* 62 */
	{ 0, 0 },			/* 63 */
	{ 0, 0 },			/* 64 */
	{ 0, 0 },			/* 65 */
	{ 0, 0 },			/* 66 */
	{ 0, 0 },			/* 67 */
	{ 0, 0 },			/* 68 */
	{ 0, 0 },			/* 69 */
	{ 0, 0 },			/* 70 */
	{ 0, 0 },			/* 71 */
	{ 0, 0 },			/* 72 */
	{ 0, 0 },			/* 73 */
	{ 0, 0 },			/* 74 */
	{ 0, 0 },			/* 75 */
	{ 0, 0 },			/* 76 */
	{ 0, 0 },			/* 77 */
	{ 0, 0 },			/* 78 */
	{ 0, 0 },			/* 79 */
	{ 0, 0 },			/* 80 */
	{ 0, 0 },			/* 81 */
	{ 0, 0 },			/* 82 */
	{ 0, 0 },			/* 83 */
	{ 0, 0 },			/* 84 */
	{ 0, 0 },			/* 85 */
	{ 0, 0 },			/* 86 */
	{ 0, 0 },			/* 87 */
	{ 0, 0 },			/* 88 */
	{ 0, 0 },			/* 89 */
	{ 0, 0 },			/* 90 */
	{ 0, 0 },			/* 91 */
	{ 0, 0 },			/* 92 */
	{ 0, 0 },			/* 93 */
	{ 0, 0 },			/* 94 */
	{ 0, 0 },			/* 95 */
	{ 0, 0 },			/* 96 */
	{ 0, 0 },			/* 97 */
	{ 0, 0 },			/* 98 */
	{ 0, 0 },			/* 99 */
	{ 0, 0 },			/* 100 */
	{ 0, 0 },			/* 101 */
	{ 0, 0 },			/* 102 */
	{ 0, 0 },			/* 103 */
	{ 0, 0 },			/* 104 */
	{ 0, 0 },			/* 105 */
	{ 0, 0 },			/* 106 */
	{ 0, 0 },			/* 107 */
	{ 0, 0 },			/* 108 */
	{ 0, 0 },			/* 109 */
	{ 0, 0 },			/* 110 */
	{ 0, 0 },			/* 111 */
	{ 0, 0 },			/* 112 */
	{ 0, 0 },			/* 113 */
	{ 0, 0 },			/* 114 */
	{ 0, 0 },			/* 115 */
	{ 0, 0 },			/* 116 */
	{ 0, 0 },			/* 117 */
	{ 0, 0 },			/* 118 */
	{ 0, 0 },			/* 119 */
	{ 0, 0 },			/* 120 */
	{ 0, 0 },			/* 121 */
	{ 0, 0 },			/* 122 */
	{ 0, 0 },			/* 123 */
	{ 0, 0 },			/* 124 */
	{ 0, 0 },			/* 125 */
	{ 0, 0 },			/* 126 */
	{ 0, 0 },			/* 127 */
	{ 0, 0 },			/* 128 */
	{ 0, 0 },			/* 129 */
	{ 0, 0 },			/* 130 */
	{ 0, 0 },			/* 131 */
	{ 0, 0 },			/* 132 */
	{ 0, 0 },			/* 133 */
	{ 0, 0 },			/* 134 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 135 */
	{ 0, 0 },			/* 136 */
	{ 0, 0 },			/* 137 */
	{ 0, 0 },			/* 138 */
	{ 0, 0 },			/* 139 */
	{ 0, 0 },			/* 140 */
	{ 0, 0 },			/* 141 */
	{ 0, 0 },			/* 142 */
	{ 0, 0 },			/* 143 */
	{ 0, 0 },			/* 144 */
	{ 0, 0 },			/* 145 */
	{ 0, 0 },			/* 146 */
	{ 0, 0 },			/* 147 */
	{ 0, 0 },			/* 148 */
	{ 0, 0 },			/* 149 */
	{ 0, 0 },			/* 150 */
	{ 0, 0 },			/* 151 */
	{ 0, 0 },			/* 152 */
	{ 0, 0 },			/* 153 */
	{ 0, 0 },			/* 154 */
	{ 0, 0 },			/* 155 */
	{ 0, 0 },			/* 156 */
	{ 0, 0 },			/* 157 */
	{ 0, 0 },			/* 158 */
	{ 0, 0 },			/* 159 */
	{ 0, 0 },			/* 160 */
	{ 0, 0 },			/* 161 */
	{ 0, 0 },			/* 162 */
	{ 0, 0 },			/* 163 */
	{ 0, 0 },			/* 164 */
	{ 0, 0 },			/* 165 */
	{ 0, 0 },			/* 166 */
	{ 0, 0 },			/* 167 */
	{ 0, 0 },			/* 168 */
	{ 0, 0 },			/* 169 */
	{ 0, 0 },			/* 170 */
	{ 0, 0 },			/* 171 */
	{ 0, 0 },			/* 172 */
	{ 0, 0 },			/* 173 */
	{ 0, 0 },			/* 174 */
	{ 0, 0 },			/* 175 */
	{ 0, 0 },			/* 176 */
	{ 0, 0 },			/* 177 */
	{ 0, 0 },			/* 178 */
	{ 0, 0 },			/* 179 */
	{ 0, 0 },			/* 180 */
	{ 0, 0 },			/* 181 */
	{ 0, 0 },			/* 182 */
	{ 0, 0 },			/* 183 */
	{ 0, 0 },			/* 184 */
	{ 0, 0 },			/* 185 */
	{ 0, 0 },			/* 186 */
	{ 0, 0 },			/* 187 */
	{ 0, 0 },			/* 188 */
	{ 0, 0 },			/* 189 */
	{ 0, 0 },			/* 190 */
	{ 0, 0 },			/* 191 */
	{ 0, 0 },			/* 192 */
	{ 0, 0 },			/* 193 */
	{ 0, 0 },			/* 194 */
	{ 0, 0 },			/* 195 */
	{ 0, 0 },			/* 196 */
	{ 0, 0 },			/* 197 */
	{ 0, 0 },			/* 198 */
	{ 0, 0 },			/* 199 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 200 */
	{ 0, 0 },			/* 201 */
	{ 0, 0 },			/* 202 */
	{ 0, 0 },			/* 203 */
	{ 0, 0 },			/* 204 */
	{ 0, 0 },			/* 205 */
	{ 0, 0 },			/* 206 */
	{ 0, 0 },			/* 207 */
	{ 0, 0 },			/* 208 */
	{ 0, 0 },			/* 209 */
	{ 0, 0 },			/* 210 */
	{ 0, 0 },			/* 211 */
	{ 0, 0 },			/* 212 */
	{ 0, 0 },			/* 213 */
	{ 0, 0 },			/* 214 */
	{ 0, 0 },			/* 215 */
	{ 0, 0 },			/* 216 */
	{ 0, 0 },			/* 217 */
	{ 0, 0 },			/* 218 */
	{ 0, 0 },			/* 219 */
	{ 0, 0 },			/* 220 */
	{ 0, 0 },			/* 221 */
	{ 0, 0 },			/* 222 */
	{ 0, 0 },			/* 223 */
	{ 0, 0 },			/* 224 */
	{ 0, 0 },			/* 225 */
	{ 0, 0 },			/* 226 */
	{ 0, 0 },			/* 227 */
	{ 0, 0 },			/* 228 */
	{ 0, 0 },			/* 229 */
	{ 0, 0 },			/* 230 */
	{ 0, 0 },			/* 231 */
	{ 0, 0 },			/* 232 */
	{ 0, 0 },			/* 233 */
	{ 0, 0 },			/* 234 */
	{ 0, 0 },			/* 235 */
	{ 0, 0 },			/* 236 */
	{ 0, 0 },			/* 237 */
	{ 0, 0 },			/* 238 */
	{ 0, 0 },			/* 239 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 240 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 241 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 242 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 243 */
	{ linkprotoname, CTL_LINKPROTO_MAXID },	/* 244 */
	{ 0, 0 },			/* 245 */
	{ 0, 0 },			/* 246 */
	{ 0, 0 },			/* 247 */
	{ 0, 0 },			/* 248 */
	{ 0, 0 },			/* 249 */
	{ 0, 0 },			/* 250 */
	{ 0, 0 },			/* 251 */
	{ 0, 0 },			/* 252 */
	{ 0, 0 },			/* 253 */
	{ 0, 0 },			/* 254 */
	{ 0, 0 },			/* 255 */
};

/* Interface specific */

static struct ctlname linkifname[] = CTL_LINK_NAMES;
static struct list linkiflist = { linkifname,
    sizeof(linkifname) / sizeof(linkifname[0]) };

/* Interface specific generic */

static struct ctlname linkgenname[] = LINK_GENERIC_NAMES;
static struct list linkgenlist = { linkgenname,
    sizeof(linkgenname) / sizeof(linkgenname[0]) };

/* Interface specific link type */

static struct ctlname ethertypename[] = ETHERCTL_NAMES;
static struct list ethertypelist = { ethertypename,
    sizeof(ethertypename) / sizeof(ethertypename[0]) };

static struct ctlname etherifname[IFT_ETHER + 1];
static struct list etheriflist = { etherifname,
    sizeof(etherifname) / sizeof(etherifname[0]) } ;

static struct ctlname fdditypename[] = FDDICTL_NAMES;
static struct list fdditypelist = { fdditypename,
    sizeof(fdditypename) / sizeof(fdditypename[0]) };

static struct ctlname fddiifname[IFT_FDDI + 1];
static struct list fddiiflist = { fddiifname,
    sizeof(fddiifname) / sizeof(fddiifname[0]) } ;

static struct ctlname tokentypename[] = TOKENCTL_NAMES;
static struct list tokentypelist = { tokentypename,
    sizeof(tokentypename) / sizeof(tokentypename[0]) };

static struct ctlname tokenifname[IFT_ISO88025 + 1];
static struct list tokeniflist = { tokenifname,
    sizeof(tokenifname) / sizeof(tokenifname[0]) } ;

static struct ctlname ieee80211typename[] = IEEE_802_11_NAMES;
static struct list ieee80211typelist = { ieee80211typename,
    sizeof(ieee80211typename) / sizeof(ieee80211typename[0]) };

static struct ctlname ieee80211ifname[IFT_IEEE80211 + 1];
static struct list ieee80211iflist = { ieee80211ifname,
    sizeof(ieee80211ifname) / sizeof(ieee80211ifname[0]) } ;

/* Interface specific HW type */

/* Interface specific Proto type */

static struct ctlname linknetname[] = CTL_LINK_NET_NAMES;
static struct list linknetlist = { linknetname,
    sizeof(linknetname) / sizeof(linknetname[0]) } ;

static struct ctlname inetifgenname[] = IPIFCTL_GEN_NAMES;
static struct list inetifgenlist = { inetifgenname,
    sizeof(inetifgenname) / sizeof(inetifgenname[0]) } ;

struct linkif {
	char	*if_name;		/* Pointer to interface name */
	struct list *if_linklist;	/* List for link type */
	struct list *if_namelist;	/* List of vars for link type */
};
static struct linkif *ifs;

void
linkinit()
{
	struct linkif *lif;
	struct ifaddrs *ifa, *ifa0;
	struct sockaddr_dl *sdl;
	struct ctlname *lnp;
	int max_if;
	int i, *types;

	/* XXX - Should we rescan each time to find new interfaces? */
	if (linklist.list != NULL)
		return;

	if ((types = calloc(linktypelist.size + 1, sizeof(types[0]))) == NULL)
		return;

	etherifname[IFT_ETHER].ctl_name = "ether";
	etherifname[IFT_ETHER].ctl_type = CTLTYPE_NODE;
	fddiifname[IFT_FDDI].ctl_name = "fddi";
	fddiifname[IFT_FDDI].ctl_type = CTLTYPE_NODE;
	tokenifname[IFT_ISO88025].ctl_name = "iso88025";
	tokenifname[IFT_ISO88025].ctl_type = CTLTYPE_NODE;
	ieee80211ifname[IFT_IEEE80211].ctl_name = "ieee80211";
	ieee80211ifname[IFT_IEEE80211].ctl_type = CTLTYPE_NODE;
	    
	max_if = 0;
	if (getifaddrs(&ifa0) == 0)
		for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
			if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr)
			    == NULL || ifa->ifa_addr->sa_family != AF_LINK)
				continue;
			if (sdl->sdl_index > max_if)
				max_if = sdl->sdl_index;
			if (sdl->sdl_type < linktypelist.size)
				types[sdl->sdl_type] = 1;
		}

	max_if++;
	if ((ifs = (struct linkif *)calloc(max_if, sizeof(*ifs))) == NULL) {
		free(types);
		return;
	}

	if ((lnp = (struct ctlname *)malloc(max_if * sizeof(*lnp))) == NULL) {
		free(types);
		return;
	}
	linklist.list = lnp;
	linklist.size = max_if;

	lnp->ctl_name = "generic";
	lnp->ctl_type = CTLTYPE_NODE;
	
	if (ifa0 == NULL) {
		free(types);
		return;
	}

	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if ((sdl = (struct sockaddr_dl *)ifa->ifa_addr) == NULL ||
		    ifa->ifa_addr->sa_family != AF_LINK) 
			continue;

		/* Initialize per interface info */
		lif = &ifs[sdl->sdl_index];
		lif->if_name = ifa->ifa_name;
		switch (sdl->sdl_type) {
		case IFT_ETHER:
			lif->if_linklist = &etheriflist;
			lif->if_namelist = &ethertypelist;
			break;
		case IFT_FDDI:
			lif->if_linklist = &fddiiflist;
			lif->if_namelist = &fdditypelist;
			break;
		case IFT_ISO88025:
			lif->if_linklist = &tokeniflist;
			lif->if_namelist = &tokentypelist;
			break;
		case IFT_IEEE80211:
			lif->if_linklist = &ieee80211iflist;
			lif->if_namelist = &ieee80211typelist;
			break;
		}

		/* Initialize interface name lookup */
		lnp++;
		lnp->ctl_name = ifa->ifa_name;
		lnp->ctl_type = CTLTYPE_NODE;
	}

	for (i = 0; i < linktypelist.size; i++)
		if (!types[i])
			linktypelist.list[i].ctl_name = NULL;

	free(types);
}

/*
 * handle link-level net requests
 * XXX this is a placeholder
 * For now, just handle
 *	net.link_layer.generic.{ppp,pif}.numif
 */
int
sysctl_link(string, bufpp, mib, flags, typep, specialp)
	char *string;
	char **bufpp;
	int mib[];
	int flags;
	int *typep;
	int *specialp;
{
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &linklist);
		return (-1);
	}

	if ((indx = findname(string, TH_THIRD, bufpp, &linklist)) == -1)
		return (-1);
	mib[2] = indx;
	/* Is it "generic" or a specific interface */
	if (indx == 0)
		lp = &linktypelist;
	else if (indx > 0)
		return (sysctl_interface(string, bufpp, indx, mib, flags, typep,
		    specialp));
	else if (!flags)
		return (-1);
	else {
		warnx("%s: no variables defined for this interface", string);
		return (-1);
	}

	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	/* look up I/F type */
	if ((indx = findname(string, TH_FOURTH, bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	if (indx <= sizeof(linktypename) / sizeof(linktypename[0]) &&
	    linkvars[indx].list != NULL)
		lp = &linkvars[indx];
	else if (!flags)
		return (-1);
	else {
		warnx("%s: no variables defined for this interface type",
		    string);
		return (-1);
	}

	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	/* look up variable name */
	if ((indx = findname(string, TH_FIFTH, bufpp, lp)) == -1)
		return (-1);
	mib[4] = indx;
	*typep = lp->list[indx].ctl_type;
	return (5);
}

int
sysctl_interface(string, bufpp, ifindx, mib, lflags, typep, specialp)
	char *string;
	char **bufpp;
	int ifindx;
	int mib[];
	int lflags;
	int *typep;
	int *specialp;
{
	struct linkif *lif;
	struct list *lp;
	struct in_addr inaddr;
	int i, indx, *mp;
	char *octet, name[BUFSIZ];

	lp = &linkiflist;
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}

	mp = &mib[3];
	lif = &ifs[ifindx];

	if ((indx = findname(string, th[mp - mib], bufpp, lp)) == -1)
		return (-1);
	*mp++ = indx;

	switch (indx) {
	case CTL_LINK_GENERIC:
		lp = &linkgenlist;
		break;

	case CTL_LINK_LINKTYPE:
		lp = lif->if_linklist;
		if (lp == NULL) {
			return (-1);
		}
		if (*bufpp == NULL) {
			listall(string, lp);
			return (-1);
		}

		if ((indx = findname(string, th[mp - mib], bufpp, lp)) == -1)
			return (-1);
		*mp++ = indx;
		lp = lif->if_namelist;
		break;

	case CTL_LINK_HWTYPE:
		lp = NULL;
		break;

	case CTL_LINK_PROTOTYPE:
		lp = &linknetlist;
		if (*bufpp == NULL) {
			listall(string, lp);
			return (-1);
		}
		if ((indx = findname(string, th[mp - mib], bufpp, lp)) == -1)
			return (-1);
		*mp++ = indx;
		switch (indx) {
		case AF_INET:
			if (*bufpp == NULL) {
				/* XXX - Need to find all IP addresses */
				inaddr.s_addr = INADDR_ANY;
				(void)sprintf(name, "%s.%s", string,
				    inet_ntoa(inaddr));
				(void)parse(name, flags);
				return (-1);
			}
			*name = 0;
			for (i = 0; i < 4; i++) {
				if ((octet = strsep(bufpp, ".")) == NULL) {
					warnx("%s: incomplete specification", 
					    string);
					return (-1);
				}
				if (strlen(octet) > 3) {
					warnx("%s: invalid octet: %s", string,
					    octet);
					return(-1);
				}
				if (*name != 0)
					strcat(name, ".");
				strcat(name, octet);
			}
			switch (inet_pton(indx, name, &inaddr)) {
			case 0:
				warnx("%s: invalid address: %s", string, name);
				return (-1);
			case -1:
				warn("%s: paring address %s:", string, name);
				return (-1);
			}
			/* XXX - No address specific definitions yet */
			if (inaddr.s_addr != INADDR_ANY) {
				warnx("%s: invalid address: %s", string, name);
				return (-1);
			}
			lp = &inetifgenlist;
			break;
		}
		break;
	}

	if (lp == NULL) {
		return (-1);
	}

	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}

	if ((indx = findname(string, th[mp - mib], bufpp, lp)) == -1)
		return (-1);
	*mp++ = indx;

	switch (lp->list[indx].ctl_type) {
	case CTLTYPE_STRUCT:
		if (!(flags & FLAG_A))
			break;
		warnx("Use netstat to view %s information", string);
		return (-1);

	}
	return (mp - mib);
}

/*
 * handle key requests
 */
struct ctlname keynames[] = KEYCTL_NAMES;
struct list keylist = { keynames, KEYCTL_MAXID };

int
sysctl_key(string, bufpp, mib, lflags, typep)
	char *string;
	char **bufpp;
	int mib[];
	int lflags;
	int *typep;
{
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &keylist);
		return (-1);
	}

	if ((indx = findname(string, TH_THIRD, bufpp, &keylist)) == -1)
		return (-1);
	mib[2] = indx;
	lp = &keylist;
	*typep = lp->list[indx].ctl_type;
	return (3);
}


/*
 * handle stat requests
 */
int
sysctl_stat(string, bufpp, mib, flags, typep, specialp)
	char *string;
	char **bufpp;
	int mib[];
	int flags;
	int *typep;
	int *specialp;
{
	int indx;

	if (*bufpp == NULL) {
		listall(string, &statlist);
		return (-1);
	}

	if ((indx = findname(string, TH_THIRD, bufpp, &statlist)) == -1)
		return (-1);
	mib[2] = indx;
	*typep = statdata[indx].ctl_type;
	if (mib[2] == STAT_PROF)
		*specialp |= RTPROF;

	return (3);
}

/*
 * Scan a list of names searching for a particular name.
 */
int
findname(string, level, bufp, namelist)
	char *string;
	char *level;
	char **bufp;
	struct list *namelist;
{
	char *name;
	int i;

	if (namelist->list == 0 || (name = strsep(bufp, ".")) == NULL) {
		warnx("%s: incomplete specification", string);
		return (-1);
	}
	for (i = 0; i < namelist->size; i++)
		if (namelist->list[i].ctl_name != NULL &&
		    strcmp(name, namelist->list[i].ctl_name) == 0)
			break;
	if (i == namelist->size) {
		warnx("%s level name %s in %s is invalid", level, name, string);
		return (-1);
	}
	return (i);
}

void
usage()
{

	(void)fprintf(stderr, "usage:\t%s\n\t%s\n\t%s\n\t%s\n",
	    "sysctl [-sn] variable ...",
	    "sysctl [-sn] -w variable=value ...",
	    "sysctl [-sn] -a",
	    "sysctl [-sn] -A");
	exit(1);
}
