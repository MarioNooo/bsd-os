/*	BSDI main.c,v 2.16 2002/01/02 23:01:09 dab Exp	*/
/*
 * Copyright (c) 1983, 1988, 1993
 *	Regents of the University of California.  All rights reserved.
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
char copyright[] =
"@(#) Copyright (c) 1983, 1988, 1993\n\
	Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	8.4 (Berkeley) 3/1/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

struct protox {
	struct	data_info *pr_data;	/* index into nlist of cb head */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	void	(*pr_cblocks)(char *, struct data_info *);	/* control blocks printing routine */
	void	(*pr_stats)(char *);	/* statistics printing routine */
	void	(*pr_istats)(char *);	/* per/if statistics printing routine */
	char	*pr_name;		/* well-known name */
};

static struct protox inetprotox[] = {
	{ NULL,		1,	NULL,		ip_stats,	NULL, "ip" },
	{ NULL,		1,	NULL,		ip6_stats,	NULL, "ip6" },
	{ NULL,		1,	NULL,		icmp_stats,	NULL, "icmp" },
	{ NULL,		1,	NULL,		icmp6_stats,	NULL, "icmp6" },
	{ NULL,		1,	NULL,		igmp_stats,	NULL, "igmp" },
	{ &tcb_info,	1,	inetprotopr,	tcp_stats,	NULL, "tcp" },
	{ &udb_info,	1,	inetprotopr,	udp_stats,	NULL, "udp" },
	{ NULL,		1,	NULL,		ipsec_stats,	NULL, "ipsec" },
#if 0
	{ NULL,		1,	NULL,		ESP_stats,	NULL, "esp" },
	{ NULL,		1,	NULL,		AH_stats,	NULL, "ah" },
#endif
	{ NULL,		0,	NULL,		NULL,		NULL, NULL }
};

#ifdef	XNS
static struct protox nsprotox[] = {
	{ &idp_info,	1,	nsprotopr,	idp_stats,	NULL, "idp" },
	{ &idp_info,	1,	nsprotopr,	spp_stats,	NULL, "spp" },
	{ NULL,		1,	NULL,		nserr_stats,	NULL, "ns_err"},
	{ NULL,		0,	NULL,		NULL,		NULL, NULL }
};
#endif

#ifdef	ISO
static struct protox isoprotox[] = {
	{ &tp_info,	1,	iso_protopr,	tp_stats,	NULL, "tp" },
	{ &cltp_info,	1,	iso_protopr,	cltp_stats,	NULL, "cltp" },
	{ NULL,		1,	NULL,		clnp_stats,	NULL, "clnp"},
	{ NULL,		1,	NULL,		esis_stats,	NULL, "esis"},
	{ NULL,		0,	NULL,		NULL,		NULL, NULL }
};
#endif

static struct protox *protoprotox[] = {
	inetprotox,
#ifdef XNS
	nsprotox,
#endif
#ifdef ISO
	isoprotox,
#endif
	NULL };
static struct protox *pflag;		/* show given protocol */

static void print_family __P((struct protox *));
static void usage __P((void));
static struct protox *name2protox __P((char *));
static struct protox *knownname __P((char *));
static char *prog;		/* program name */

kvm_t *kvmd;

int	Aflag;		/* show addresses of protocol control block */
int	aflag;		/* show all sockets (including servers) */
int	bflag;		/* display bytes instead of packets */
int	fflag;		/* address family */
int	gflag;		/* show group (multicast) routing or stats */
int	iflag;		/* show interfaces */
int	kflag;		/* use kmem (vs sysctl). */
int	mflag;		/* show memory stats */
int	nflag;		/* show addresses numerically */
int	Oflag;		/* backward compatibility */
int	Pflag;		/* parsable output */
int	rflag;		/* show routing tables (or routing stats) */
int	sflag;		/* show protocol statistics */
int	vflag;		/* verbose (may be repeated) */

int	interval;	/* repeat interval for i/f stats */

char	*interface;	/* desired i/f for stats, or NULL for all i/fs */

char	*filler = " ";	/* filler for empty columns */

int	resolve_quietly;	/* Quietly ignore non-existant symbols */


int
main(int argc, char *argv[])
{
	int ch, dflag;
	char *cp;
	char *nlistf = NULL, *memf = NULL;
	char buf[_POSIX2_LINE_MAX];

	if ((cp = strrchr(argv[0], '/')))
		prog = cp + 1;
	else
		prog = argv[0];
	fflag = AF_UNSPEC;
	aflag = dflag = 0;

	while ((ch = getopt(argc, argv, "Aabdf:gI:ikM:mN:nOPp:rstuvw:")) != EOF)
		switch(ch) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			if (strcasecmp(optarg, "ns") == 0)
#ifdef XNS
				fflag = AF_NS;
#else
				errx(1, "unsupported address family: %s", optarg);
#endif
			else if (strcasecmp(optarg, "inet") == 0)
				fflag = AF_INET;
			else if (strcasecmp(optarg, "unix") == 0
				 || strcasecmp(optarg, "local") == 0)
				fflag = AF_LOCAL;
			else if (strcasecmp(optarg, "iso") == 0)
#ifdef ISO
				fflag = AF_ISO;
#else
				errx(1, "unsupported address family: %s", optarg);
#endif
			else if (strcasecmp(optarg, "inet6") == 0)
				fflag = AF_INET6;
			else
				errx(1, "unknown address family: %s", optarg);
			break;
		case 'g':
			gflag = 1;
			break;
		case 'I':
			interface = optarg;
			iflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'M':
			kflag = 1;
			memf = optarg;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'O':
			Oflag = 1;
			break;
		case 'P':
			Pflag = 1;
			filler = "-";
			break;
		case 'p':
			if ((pflag = name2protox(optarg)) == NULL)
				errx(1,
				    "unknown or uninstrumented protocol: %s",
				     optarg);
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'u':
			fflag = AF_LOCAL;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			interval = atoi(optarg);
			iflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		if (isdigit(**argv)) {
			interval = atoi(*argv);
			if (interval <= 0)
				usage();
			++argv;
			iflag = 1;
		}
		if (*argv) {
			kflag = 1;
			nlistf = *argv;
			if (*++argv)
				memf = *argv;
		}
	}
#endif

	if (rflag + iflag + (gflag & Oflag) > 1)
		usage();

	if (Oflag && pflag != NULL && !sflag)
		sflag = 1;

	if (sflag && interval != 0)
		usage();

	if (dflag) {
		if (!iflag || !Oflag)
			usage();
		if (!vflag)
			vflag = 1;
	}

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());

	if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL)
		errx(1, "kvm_openfiles: %s", buf);

	if (mflag) {
		mbpr();
		exit(0);
	}

	if (pflag != NULL) {
		if (sflag) {
			(*pflag->pr_stats)(pflag->pr_name);
			if (iflag) {
				if (pflag->pr_istats == NULL)
					(void)errx(1,
					    "no ifstats routine for %s",
					    pflag->pr_name);
				(*pflag->pr_istats)(interface);
			} else {
				if (pflag->pr_stats == NULL)
					(void)errx(1,
					    "no stats routine for %s",
					    pflag->pr_name);
				(*pflag->pr_stats)(pflag->pr_name);
			}
		} else {
			if (pflag->pr_cblocks == NULL)
				(void)errx(1, "no print routine for %s", pflag->pr_name);
			(*pflag->pr_cblocks)(pflag->pr_name, pflag->pr_data);
		}
		exit(0);
	}

	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	sethostent(1);
	setnetent(1);
	setprotoent(1);
	setservent(1);

	if (iflag) {
		if (fflag != AF_UNSPEC)
			goto protostat;
		if (!Oflag && gflag) {
			if (sflag)
				usage();
			if_print_multi();
		} else if (interval != 0)
			if_print_interval(interval);
		else if (sflag)
			if_stats();
		else
			if_print();
		exit(0);
	}

	if (rflag) {
		if (!Oflag && gflag) {
			if (fflag != AF_UNSPEC && fflag != AF_INET &&
			    fflag != AF_INET6)
				errx(1, "%s does not support multicast routing",
				    af2name(fflag));

			if (sflag) {
				mrt_stats();
				mrt6_stats();
			} else {
				mroutepr();
				mroute6pr();
			}
			exit(0);
		}
		if (sflag)
			rt_stats();
		else
			routepr();
		exit(0);
	}

	if (Oflag && gflag) {
		if (sflag) {
			mrt_stats();
			mrt6_stats();
		} else {
			mroutepr();
			mroute6pr();
		}
		exit(0);
	}

protostat:
	if (fflag == AF_UNSPEC)
		resolve_quietly = 1;

	if (fflag == AF_INET || fflag == AF_INET6 || fflag == AF_UNSPEC)
		print_family(inetprotox);

#ifdef XNS
	if (fflag == AF_NS || fflag == AF_UNSPEC)
		print_family(nsprotox);
#endif

#ifdef ISO
	if (fflag == AF_ISO || fflag == AF_UNSPEC)
		print_family(isoprotox);
#endif

	if ((fflag == AF_LOCAL || fflag == AF_UNSPEC) && !sflag)
		unixpr();

	exit(0);
}

/*
 * Print out protocol statistics or control blocks (per sflag).
 * If the interface was not specifically requested, and the symbol
 * is not in the namelist, ignore this one.
 */
static void
print_family(struct protox *tpp)
{
	struct protox *tp;

	for (tp = tpp; tp->pr_name != NULL; tp++)
		if (sflag) {
			if (iflag) {
				if (tp->pr_istats)
					tp->pr_istats(interface);
			} else if (tp->pr_stats != NULL)
				tp->pr_stats(tp->pr_name);
		} else {
			if (tp->pr_cblocks != NULL)
				tp->pr_cblocks(tp->pr_name, tp->pr_data);
		}
}

/*
 * Read kernel memory, return 0 on success.
 */
int
kread(u_long addr, void *buf, size_t size)
{
	ssize_t rc;

	if ((rc = kvm_read(kvmd, addr, buf, size)) == (ssize_t)size)
		return (0);

	if (rc < 0)
		warnx("kvm_read %s", kvm_geterr(kvmd));
	else
		warnx("kread: short read (read %u, expected %u)",
		    rc, size);
	return(-1);
}


/* Given the passed data structure, get the value from the kernel */
int
skread(char *name, struct data_info *dip)
{
	size_t size;

	/* 
	 * If reading kmem is indicated, or there is no sysctl for
	 * this object, then we go kernel diving.
	 */
	if (kflag || dip->di_mib == NULL || dip->di_miblen == 0) {
		if (!dip->di_resolved)
			resolve(name, dip, NULL);
		if (dip->di_off == 0) {
			if (!resolve_quietly)
				warnx("%s not supported by this system", name);
			return (-1);
		}
		return (kread(dip->di_off, dip->di_ptr, dip->di_size));
	}

	size = dip->di_size;
	if (sysctl(dip->di_mib, dip->di_miblen, dip->di_ptr, &size, NULL, 0)
	    < 0) {
		if (!resolve_quietly ||
		    (errno != ENOPROTOOPT && errno != EOPNOTSUPP))
			warn("%s: sysctl read %s", name, dip->di_name + 1);
		return (-1);
	}
	if (size < dip->di_size)
		memset(dip->di_ptr + size, 0, dip->di_size - size);

	return (0);
}

#include <stdarg.h>

/* Resolve the nlist entries in the passed data structures */
void
resolve(char *name, ...)
{
	va_list ap;
	struct nlist nlist[10], *nlp, *nllp;
	struct data_info *dip;
#ifdef	notdef
	char buf[_POSIX2_LINE_MAX];

	if (kvmd == NULL &&
	    (kvmd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, buf)) == NULL)
		errx(1, "kvm_openfiles: %s", buf);
#endif	/* notdef */

	memset(nlist, 0, sizeof(nlist));
	va_start(ap, name);
	for (nlp = nlist; (dip = va_arg(ap, struct data_info *)) != NULL; 
	    nlp++)
		nlp->n_name = dip->di_name;
	(nllp = nlp)->n_name = "";
	va_end(ap);
	
	if (kvm_nlist(kvmd, nlist) < 0)
		err(1, "resolve %s kvm_nlist: %s", name, kvm_geterr(kvmd));
        
	va_start(ap, name);
	for (nlp = nlist; nlp < nllp; nlp++) {
		dip = va_arg(ap, struct data_info *);
		dip->di_off = nlp->n_value;
		dip->di_resolved = 1;
	}
	va_end(ap);
}

/*
 * Find the protox for the given "well-known" name.
 */
static struct protox *
knownname(char *name)
{
	struct protox **tpp, *tp;

	for (tpp = protoprotox; *tpp; tpp++)
		for (tp = *tpp; tp->pr_name; tp++)
			if (strcasecmp(tp->pr_name, name) == 0)
				return (tp);
	return (NULL);
}

/*
 * Find the protox corresponding to name.
 */
static struct protox *
name2protox(char *name)
{
	struct protox *tp;
	char **alias;			/* alias from p->aliases */
	struct protoent *p;

	/*
	 * Try to find the name in the list of "well-known" names. If that
	 * fails, check if name is an alias for an Internet protocol.
	 */
	if ((tp = knownname(name)))
		return (tp);

	setprotoent(1);			/* make protocol lookup cheaper */
	while ((p = getprotoent())) {
		/* assert: name not same as p->name */
		for (alias = p->p_aliases; *alias; alias++)
			if (strcasecmp(name, *alias) == 0) {
				endprotoent();
				return (knownname(p->p_name));
			}
	}
	endprotoent();
	return (NULL);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s "
	    "[-Aknv] [-f address_family] [-p protocol] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-s [-Aknv] [-f address_family] [-p protocol] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s -m [-k] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-i [-bknv] [-I interface] [-f address_family] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-ig [-knv] [-I interface] [-f address_family] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-is [-bknv] [-I interface] [-f address_family] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-i -w wait [-bkv] [-I interface] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-r [-knv] [-f address_family] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s "
	    "-rg [-knv] [-f address_family] "
	    "[-M core] [-N system]\n", prog);
	(void)fprintf(stderr, "       %s -rs [-g] "
	    "[-M core] [-N system]\n", prog);
	exit(1);
}

char *
af2name(int family)
{
#define	PRUNK	"Protocol Family %3u"
	static char unknown[sizeof(PRUNK)];

	switch (family) {
	case AF_INET:
		return ("Internet");
	case AF_INET6:
		return ("Internet v6");
	case AF_NS:
		return ("XNS/IPX");
	case AF_ISO:
		return ("ISO");
	case AF_CCITT:
		return ("X.25");
	case AF_LINK:
		return ("Link layer");
	case AF_ROUTE:
		return ("Routing socket");
	case AF_LOCAL:
		return ("Local domain");
	}
	(void)snprintf(unknown, sizeof(unknown), PRUNK, family);
	return (unknown);
}
