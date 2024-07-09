/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwnat.c,v 1.7 2000/01/20 22:53:03 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_nat.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern FILE *yyin;
extern int error;
extern void *nat_definition;
extern int nat_space;

int vflag = 0;

extern struct {
	ipfw_filter_t	ipfw;
	ip_nat_hdr_t	nh;
} s;

char nat_buf[sizeof(ip_natdef_t) +
		    sizeof(ip_natservice_t) +
		    sizeof(ip_natmap_t) +
		    sizeof(ip_natent_t)];

list(int who)
{
	extern void display_nat(int, ipfw_filter_t *, int);

	ipfw_filter_t *iftp;
	char *b;
	size_t n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_PREINPUT, IPFWCTL_PUSH };

	mib[4] = who;

	if (sysctl(mib, 6, NULL, &n, NULL, 0))
		err(1, "reading filters");

	if (n == 0)
		return(0);

	if ((b = malloc(n)) == NULL)
		err(1, NULL);

	if (sysctl(mib, 6, b, &n, NULL, 0))
		err(1, "reading filters");

	for (iftp = (ipfw_filter_t *)b; (char *)iftp < b + n;
	    iftp = (ipfw_filter_t *)IPFW_next(iftp)) {
		if (iftp->type != IPFW_NAT)
			continue;
		if (vflag)
			display_nat(who, iftp, vflag > 1);
		else
			printf("%*.*s\n", -IPFW_TAGLEN, IPFW_TAGLEN, iftp->tag);
	}
}

int
kill(int who, char *what)
{
	ipfw_filter_t *iftp;
	char *b;
	size_t n;
	int i;
	int serial;
	char tag[IPFW_TAGLEN+1];		/* last byte not always nul */
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_PREINPUT, IPFWCTL_POP };

	serial = strtol(what, &b, 0);
	if (*b || b == what)
		serial = 0;

	mib[4] = who;
	mib[5] = IPFWCTL_PUSH;
	strncpy(tag, what, IPFW_TAGLEN+1);	/* strncpy zero fills */

	if (sysctl(mib, 6, NULL, &n, NULL, 0))
		err(1, "reading filters");

	if (n == 0)
		return(0);

	if ((b = malloc(n)) == NULL)
		err(1, NULL);

	if (sysctl(mib, 6, b, &n, NULL, 0))
		err(1, "reading filters");

	for (iftp = (ipfw_filter_t *)b; (char *)iftp < b + n;
	    iftp = (ipfw_filter_t *)IPFW_next(iftp)) {
		if (iftp->type != IPFW_NAT)
			continue;
		if (serial == iftp->serial ||
		    (serial == 0 && memcmp(iftp->tag, tag, IPFW_TAGLEN)) == 0) {
			mib[5] = IPFWCTL_POP;
			if (sysctl(mib, 6, NULL, NULL, &iftp->serial,
			    sizeof(int)))
				err(1, "%s", what);
			return(1);
		}
	}
	return (0);
}

int
main(int argc, char **argv)
{
	int c;
	int lflag = 0;
	int callflag = 0;
	struct sockaddr_in sin;
	struct addrinfo *ai;
	char *killme = NULL;
	ip_natdef_t *nd;
	ip_natservice_t *ns;
	ip_natmap_t *nm;
	ip_natent_t *ne;
	size_t n;
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
		IPFW_PREINPUT, IPFWCTL_SYSCTL, IPFW_NAT, /*serial*/0,
		IPFWNAT_SETUP };


	memset(&sin, 0, sizeof(sin));
	memset(&s, 0, sizeof(s));
	s.ipfw.type = IPFW_NAT;
	s.ipfw.hlength = sizeof(s.ipfw);
	s.ipfw.length = sizeof(s.nh);
	s.ipfw.priority = 1024;

	while ((c = getopt(argc, argv, "ci:lr:s:v")) != EOF) {
		switch (c) {
		case 'c':
			callflag = 1;
			break;

		case 'i':
			s.nh.index = if_nametoindex(optarg);
			if (s.nh.index < 1)
				errx(1, "%s: no such interface", optarg);
			break;

		case 'l':
			++lflag;
			break;

		case 'r':
			killme = optarg;
			break;

		case 's':
			if ((c = getaddrinfo(optarg, NULL, NULL, &ai)) != 0)
				errx(1, "%s: %s", optarg, gai_strerror(c));
			for (; ai; ai = ai->ai_next) {
				if (ai->ai_family != PF_INET)
					continue;
				sin = *(struct sockaddr_in *)ai->ai_addr;
				break;
			}
			if (ai == NULL)
				errx(1, "%s: not an IPv4 address", optarg);
			break;
			
		case 'v':
			vflag++;
			break;

		default: usage:
			fprintf(stderr,
"ipfwnat [-cv] config-file\n"
"ipfwnat -i interface [-s address]\n"
"ipfwnat -r tag\n"
"ipfwnat [-v] -l\n");
			exit(1);
		}
	}

	if (lflag) {
		list(IPFW_PREINPUT);
		list(IPFW_CALL);
	}

	if (s.nh.index) {
		if (optind + 1 == argc)
			errx(1, "-i and config-file are mutually exclusive");
		if (killme)
			errx(1, "-i and -k are mutually exclusive");
		nd = (ip_natdef_t *)nat_buf;
		if (sin.sin_addr.s_addr) {
			ns = (ip_natservice_t *)(nd + 1);
			nm = (ip_natmap_t *)(ns + 1);
			ns->ns_internal = sin.sin_addr;
			nd->nd_nservices = 1;
		} else {
			nm = (ip_natmap_t *)(nd + 1);
			nd->nd_nservices = 0;
		}
		ne = (ip_natent_t *)(nm + 1);
		nd->nd_nmaps = 1;
		nm->nm_nentries = 1;
		nat_definition = nat_buf;
		nat_space = (char *)(ne + 1) - nat_buf;
	} else if (killme) {
		if (optind + 1 == argc)
			errx(1, "-k and config-file are mutually exclusive");
		c = kill(IPFW_PREINPUT, killme) +
		    kill(IPFW_PREOUTPUT, killme) +
		    kill(IPFW_CALL, killme);
		if (c == 0)
			errx(1, "%s: no such filter", killme);
		exit (0);
	} else if (optind + 1 == argc) {
		if ((yyin = fopen(argv[optind], "r")) == NULL)
			err(1, "%s", argv[optind]);

		yyparse();
		if (error)
			exit(error);
	} else if (lflag)
		exit (0);
	else
		goto usage;

	if (s.ipfw.tag[0] == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		sprintf(s.ipfw.tag, "nat.%d.%06d", tv.tv_sec, tv.tv_usec);
	}


	mib[4] = callflag ? IPFW_CALL : IPFW_PREINPUT;
	n = sizeof(s.ipfw.serial);
	mib[5] = IPFWCTL_SERIAL;
	if (sysctl(mib, 6, &s.ipfw.serial, &n, NULL, NULL))
		err(1, "getting serial number");
	mib[7] = s.ipfw.serial;
	mib[5] = IPFWCTL_PUSH;
	if (sysctl(mib, 6, NULL, NULL, &s, sizeof(s)))
		err(1, "pushing nat box");

	if (vflag)
		printf("Installed %snatbox at serial #%d\n",
		    callflag ? "" : "input ", s.ipfw.serial);

	if (callflag == 0) {
		s.nh.glue = s.ipfw.serial;
		mib[4] = IPFW_PREOUTPUT;
		n = sizeof(s.ipfw.serial);
		mib[5] = IPFWCTL_SERIAL;
		if (sysctl(mib, 6, &s.ipfw.serial, &n, NULL, NULL))
			err(1, "getting serial number");
		mib[5] = IPFWCTL_PUSH;
		if (sysctl(mib, 6, NULL, NULL, &s, sizeof(s)))
			err(1, "pushing nat box");
		if (vflag)
			printf("Installed output natbox at serial #%d\n",
			    s.ipfw.serial);
	}

	mib[4] = callflag ? IPFW_CALL : IPFW_PREINPUT;
	mib[5] = IPFWCTL_SYSCTL;
	if (sysctl(mib, 9, NULL, NULL, nat_definition, nat_space))
		err(1, "setting up nat box");
}
