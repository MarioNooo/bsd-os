/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw.c,v 1.12 2002/12/20 17:20:48 prb Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <net/bpf.h>

#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipfw.h"

#ifdef	PHOENIX
#define	_PATH_FILTERS	"/ipfw"
#define	_PATH_TMPIPFW	"kovzha:/folk/rich/Firewall/ipfw.%d"
#define	FORCE_DONT_SAVE		1	/* Force -dontsave was specified */
#define	NO_DISPLAY_SUPPORT	1	/* Don't support display_* routines */
#else
#define	_PATH_FILTERS	"/var/run/ipfw"
#define	_PATH_TMPIPFW	"/tmp/ipfw.%d"
#endif

typedef u_long flag_t[3];

#define	DATASZ	(8 * 1024 * 32)

static char _buf[DATASZ];
static char *buf = _buf;

static int priority = 0;
static int dontsave = 0;
static char tag[IPFW_TAGLEN+1];

static char *append_filter(char *, char *, char *, char *, int *, int *);
static int cat(char *);
static int callfilter(char *);
#ifndef	NO_DISPLAY_SUPPORT
extern void display_circuitcache(ipfw_filter_t *);
extern void display_nat(int, ipfw_filter_t *, int);
extern void display_throttle(int, ipfw_filter_t *, int);
extern void display_flow(int, ipfw_filter_t *, int);
extern void display_cache(int, ipfw_filter_t *, int);
#endif

struct {
	char	*name;
	int	ib;
	char	*flags;
} filternames[] = {
	{ "input",	IPFW_INPUT, },
	{ "output",	IPFW_OUTPUT, },
	{ "forward",	IPFW_FORWARD, },
	{ "pre-input",	IPFW_PREINPUT, },
	{ "pre-output",	IPFW_PREOUTPUT, },
	{ "call",	IPFW_CALL, },
	{ "rate",	IPFW_RATE, },
	{ "input6",	IPFW_INPUT6, "-v6" },
	{ "output6",	IPFW_OUTPUT6, "-v6" },
	{ "forward6",	IPFW_FORWARD6, "-v6" },
	{ "pre-input6",	IPFW_PREINPUT6, "-v6" },
	{ "pre-output6",IPFW_PREOUTPUT6, "-v6" },
	{ NULL,		0 },
};

char dashes[] = "----------------------------------------";
static int header = 1;
static char *version_flags;

#ifdef	PHOENIX
#define	main	__ipfwcmd

int main(int, char **);
int ipfwcmd(int, char **);

int uniqID = 0;
int ipfwCmdError;
SEM_ID syncSemIpfwCmd; /* For synchronizing with ipfw command */
SEM_ID syncSemIpfwCmp; /* For synchronizing with compiler */
extern STATUS ipfwcmp(int, char**);
extern int ipfwCmpError;
extern int ipfwAsmError;
extern void ipfwCmdExit(int);
extern SL_LIST *ipfwMemBlockList;
extern int totalMallocs;
extern int totalFrees;

void
ipfwCmdReInit()
{
	buf = _buf + BUF_OFFSET;
	dontsave = 0;
	header = 1;

	/*
	 * Initialize the memory block usage list
	 */
	ipfwMemBlockList = sllCreate(); 
	totalMallocs = totalFrees = 0;
}

void
cleanMallocList()
{
	SL_NODE *pNode;

	pNode = sllGet(ipfwMemBlockList);
	while(pNode != NULL) {
#if 0
		printf("Buffer needs cleaning: 0x%x\n",
		    (int)((IPFW_MEM_BLK_NODE *)pNode)->buf);
#endif
		free((void *)((IPFW_MEM_BLK_NODE *)pNode)->buf);
		free(pNode);
		pNode = sllGet(ipfwMemBlockList);
	}
	sllDelete(ipfwMemBlockList);
	printf("totalMallocs: %d\n", totalMallocs);
	printf("totalFrees: %d\n", totalFrees);
}


int
ipfw(char * args)
{
	int status = 0;
	int argc = 0;
	char *argv[10]; /* REVISIT - how big should this be? */

	argv[argc++] = "ipfw";
	argv[argc++] = strtok(args, " ");

	/*
	 * Reinit globals each time we are called
	 */

	ipfwCmdReInit();

	while ((argv[argc] = strtok(NULL, " ")) != NULL)
		++argc;

	/*
	 * ipfw sysctl routines call splnet() which takes the network 
	 * semaphore. Consequently, we can't call ipfwcmd() from the shell
	 * since the shell is not breakable (I guess). Anyway, it doesn't
	 * work unless you spawn a separate task. So, we create a semaphore
	 * spawn the ipfwcmd task and wait for it to finish. Note that we
	 * spawn the new task at priority 100. This just seems like a
	 * "reasonable" priority level. ipfwcmp and ipfwasm will each be
	 * spawned below at priority levels 99 and 98 respectivley so
	 * that everything synchronizes properly. 
	 */

	syncSemIpfwCmd = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

	if (taskSpawn("ipfwcmd", 100, 0, 20000, (FUNCPTR)ipfwcmd,
	   argc, (int)argv, 0, 0, 0, 0, 0, 0, 0, 0) != ERROR) {
		semTake(syncSemIpfwCmd, WAIT_FOREVER);

		/* Check the exit status of the compiler task. */

		if (ipfwCmdError) {
			printf("ERROR: ipfwCmdError - \n");
			status = ERROR;
		}
	} else {
		printf("ERROR: ipfwcmd could not be spawned\n");
		status = ERROR;
	}

	semDelete(syncSemIpfwCmd);
	cleanMallocList();
	return(status);
}

int
ipfwcmd(int ac, char **av)
{
	int r;

Vinod -- Is it correct that you always want to call ipfwCmdExit when ever
this code exits?

Also, you will want to make sure that err() and errx() call ipfwCmdExit
too, right?

	r = main(ac, av);
	ipfwCmdExit(r);
}

int
getindex()
{
	return (++uniqID);
}

#else

#define	getindex()	getpid()

#endif

int
main(int ac, char **av)
{
	int o, serial;
	ipfw_filter_t ift, *iftp;
	size_t n;
	char *ext, *ebuf = buf;

        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_INPUT,
	    0, 0 };

#define	what	mib[4]
#define	cmd	mib[5]

#ifdef	PHOENIX
#define	MIB	(mib + 4)
#define	MIBLEN	0
#else
#define	MIB	mib
#define	MIBLEN	4
#endif

	memset(tag, 0, sizeof(tag));
	priority = 0;

	if (ac < 1) {
		fprintf(stderr, "usage: ipfw filter [-command options ...]\n");
		return (1);
	}

	if (av[1] == NULL || (av[2] == NULL && strcmp(av[1], "-list") == 0)) {
		char *argv[4];
		int ret = 0;

		argv[0] = NULL;
		argv[2] = av[1];
		argv[3] = NULL;
		for (o = 0; filternames[o].name != NULL; ++o) {
			argv[1] = filternames[o].name;
			ret |= main(argv[2] ? 3 : 2, argv);
		}
		return (ret);
	}

	for (o = 0; ; ++o) {
		if (filternames[o].name == NULL)
			errx(1, "%s: unknown filter", av[1]);
		if (strcmp(filternames[o].name, av[1]) == 0) {
			what = filternames[o].ib;
			version_flags = filternames[o].flags;
			break;
		}
	}

	if (ac == 2) {
		char *b;

		cmd = IPFWCTL_LIST;
		if (sysctl(MIB, MIBLEN + 2, NULL, &n, NULL, 0))
			err(1, "reading filters");

		if (n == 0)
			return(0);

		if ((b = malloc(n)) == NULL)
			err(1, NULL);

		if (sysctl(MIB, MIBLEN + 2, b, &n, NULL, 0))
			err(1, "reading filters");

		if (header) {
			header = 0;
			if (av[0] == NULL)
				printf("%-10s ", "filter");
			printf("%6s %*s %-10s %s\n",
			    "serial", -IPFW_TAGLEN, "tag", "type",
			    "priority");
			if (av[0] == NULL)
				printf("%.10s ", dashes);
			printf("%.6s %.*s %.10s %.8s\n",
				dashes,
				IPFW_TAGLEN, dashes,
				dashes, dashes);
		}
		for (iftp = (ipfw_filter_t *)b; (char *)iftp < b + n;
		    iftp = (ipfw_filter_t *)IPFW_next(iftp)) {
			if (av[0] == NULL)
				printf("%-10s ", av[1]);
			printf("%6d %*.*s %-10s %d\n",
			    iftp->serial,
			    -IPFW_TAGLEN, IPFW_TAGLEN, iftp->tag,
			    iftp->type == IPFW_BPF ? "BPF" :
			    iftp->type == IPFW_CIRCUIT ? "circuit" :
			    iftp->type == IPFW_FLOW ? "flow" :
			    iftp->type == IPFW_NAT ? "nat" :
			    iftp->type == IPFW_THROTTLE ? "throttle" :
			    iftp->type == IPFW_CACHE ? "cache" :
			    iftp->type == IPFW_ECHOCHK ? "echochk" :
			    "unknown",
			    iftp->priority);
		}
		free(b);
		return(0);
	}


	for (o = 2; o < ac; ++o) {
		if (strcmp(av[o], "-push") == 0) {
			if (av[++o] == NULL)
				errx(1, "-push requires at least one filter");
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-insert") == 0) {
			if (av[++o] == NULL || av[++o] == NULL || av[o][0] == '-')
				errx(1, "-insert requires a filter number and a filter");
			if (what != IPFW_CALL)
				errx(1, "filters can only be inserted on the call filter chain");
		} else if (strcmp(av[o], "-serialpush") == 0) {
			if (av[++o] == NULL || av[++o] == NULL)
				errx(1, "-serialpush requires at least one serial number and one filter");
			while (av[++o] && av[o][0] != '-')
				if (av[++o] == NULL)
					errx(1, "-serialpush requires a serial number for every filter");
		} else if (strcmp(av[o], "-move") == 0) {
			if (av[++o] == NULL || av[++o] == NULL || av[o][0] == '-')
				errx(1, "-move requires two serial numbers");
		} else if (strcmp(av[o], "-pop") == 0) {
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-popall") == 0) {
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-replace") == 0) {
			if (av[++o] == NULL)
				errx(1, "-replace requires at least one filter");
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-replaceall") == 0) {
			if (av[++o] == NULL)
				errx(1, "-replaceall requires at least one filter");
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-list") == 0) {
			;
		} else if (strcmp(av[o], "-dontsave") == 0) {
			;
		} else if (strcmp(av[o], "-serial") == 0) {
			;
		} else if (strcmp(av[o], "-stats") == 0) {
			;
		} else if (strcmp(av[o], "-display") == 0) {
			while (av[++o] && av[o][0] != '-')
				;
		} else if (strcmp(av[o], "-output") == 0) {
			if (av[++o] == NULL)
				errx(1, "-output requires a filename");
		} else if (strcmp(av[o], "-priority") == 0) {
			if (av[++o] == NULL)
				errx(1, "-priority requires a priority level");
		} else if (strcmp(av[o], "-tag") == 0) {
			if (av[++o] == NULL)
				errx(1, "-tag requires a tag name");
		} else
			errx(1, "%s: unknown command", av[o]);
	}

#define	FLUSH_FILTER \
	if (ebuf > buf) { \
		cmd = IPFWCTL_PUSH; \
		if (sysctl(MIB, MIBLEN + 2, NULL, NULL, buf, ebuf - buf)) \
			err(1, NULL); \
	} \
	ebuf = buf = _buf;

	ebuf = buf = _buf;
	for (o = 2; o < ac; ++o) {
		if (strcmp(av[o], "-list") == 0) {
			char *b;

			cmd = IPFWCTL_PUSH;
			if (sysctl(MIB, MIBLEN + 2, NULL, &n, NULL, 0))
				err(1, "reading filters");

			if (n == 0)
				return(0);

			if ((b = malloc(n)) == NULL)
				err(1, NULL);

			if (sysctl(MIB, MIBLEN + 2, b, &n, NULL, 0))
				err(1, "reading filters");

			if (header) {
				header = 0;
				if (av[0] == NULL)
					printf("%-10s ", "filter");
				printf("%6s %*s %-10s %6s %s\n",
				    "serial", -IPFW_TAGLEN, "tag", "type",
				    "length", "priority");
				if (av[0] == NULL)
					printf("%.10s ", dashes);
				printf("%.6s %.*s %.10s %.6s %.8s\n",
					dashes,
					IPFW_TAGLEN, dashes,
					dashes, dashes, dashes);
			}
			for (iftp = (ipfw_filter_t *)b; (char *)iftp < b + n;
			    iftp = (ipfw_filter_t *)IPFW_next(iftp)) {
				if (av[0] == NULL)
					printf("%-10s ", av[1]);
				printf("%6d %*.*s %-10s %6d %d\n",
				    iftp->serial,
				    -IPFW_TAGLEN, IPFW_TAGLEN, iftp->tag,
				    iftp->type == IPFW_BPF ? "BPF" :
				    iftp->type == IPFW_CIRCUIT ? "circuit" :
				    iftp->type == IPFW_FLOW ? "flow" :
				    iftp->type == IPFW_NAT ? "nat" :
				    iftp->type == IPFW_THROTTLE ? "throttle" :
				    iftp->type == IPFW_CACHE ? "cache" :
			    	    iftp->type == IPFW_ECHOCHK ? "echochk" :
				    "unknown",
				    iftp->length, iftp->priority);
			}
			free(b);
		} else if (strcmp(av[o], "-push") == 0) {
			while (av[++o] && av[o][0] != '-')
				ebuf = append_filter(av[o], ebuf,
				    _buf + sizeof(_buf), NULL, mib, NULL);
		} else if (strcmp(av[o], "-insert") == 0) {
			ebuf = append_filter(av[o], ebuf, _buf + sizeof(_buf),
				NULL, mib, &serial);
			FLUSH_FILTER
			cmd = IPFWCTL_INSERT;
			mib[6] = strtol(av[o+1], &ext, 0);
			if (mib[6] < 0 || ext == av[o+1] || *ext)
				errx(1, "%s: invalid filter number", av[0]);
			if (sysctl(MIB, MIBLEN + 3, NULL, NULL, &serial, sizeof(serial)))
				err(1, "inserting filter %d", mib[6]);
			o += 2;
		} else if (strcmp(av[o], "-serialpush") == 0) {
			while (av[++o] && av[o][0] != '-') {
				ebuf = append_filter(av[o], ebuf,
				    _buf + sizeof(_buf), av[o+1], mib, NULL);
				++o;
			}
		} else if (strcmp(av[o], "-pop") == 0) {
			FLUSH_FILTER
			if (av[o+1] == NULL || av[o+1][0] == '-') {
				*(int *)ebuf = 0;
				ebuf = ebuf + sizeof(int);
			} else while (av[++o] && av[o][0] != '-') {
				*(int *)ebuf = strtol(av[o], 0, 0);
				ebuf = ebuf + sizeof(int);
			}
			cmd = IPFWCTL_POP;
			if (sysctl(MIB, MIBLEN + 2, NULL, NULL, buf, ebuf - buf))
				err(1, NULL);
			ebuf = buf = _buf;
		} else if (strcmp(av[o], "-popall") == 0) {
			FLUSH_FILTER
			if (av[o+1] == NULL || av[o+1][0] == '-') {
				*(int *)ebuf = 0;
				ebuf = ebuf + sizeof(int);
			} else while (av[++o] && av[o][0] != '-') {
				*(int *)ebuf = strtol(av[o], 0, 0);
				ebuf = ebuf + sizeof(int);
			}
			cmd = IPFWCTL_POPALL;
			if (sysctl(MIB, MIBLEN + 2, NULL, NULL, buf, ebuf - buf))
				err(1, NULL);
			ebuf = buf = _buf;
		} else if (strcmp(av[o], "-replace") == 0) {
			memset(&ift, 0, sizeof(ift));
			n = sizeof(ift);
			cmd = IPFWCTL_PUSH;
			if (sysctl(MIB, MIBLEN + 2, &ift, &n, NULL, NULL))
				if (errno != ENOMEM)
					err(1, NULL);

			while (av[++o] && av[o][0] != '-')
				ebuf = append_filter(av[o], ebuf,
				    _buf + sizeof(_buf), NULL, mib, NULL);

			if (ift.serial) {
				FLUSH_FILTER
				*(int *)ebuf = ift.serial;
				ebuf = ebuf + sizeof(int);
				cmd = IPFWCTL_POP;
				if (sysctl(MIB, MIBLEN + 2, NULL, NULL, buf, ebuf - buf))
					err(1, NULL);
				ebuf = buf = _buf;
			}
		} else if (strcmp(av[o], "-replaceall") == 0) {
			memset(&ift, 0, sizeof(ift));
			n = sizeof(ift);
			cmd = IPFWCTL_PUSH;
			if (sysctl(MIB, MIBLEN + 2, &ift, &n, NULL, NULL))
				if (errno != ENOMEM)
					err(1, NULL);

			while (av[++o] && av[o][0] != '-')
				ebuf = append_filter(av[o], ebuf,
				    _buf + sizeof(_buf), NULL, mib, NULL);

			if (ift.serial) {
				FLUSH_FILTER
				*(int *)ebuf = ift.serial;
				ebuf = ebuf + sizeof(int);
				cmd = IPFWCTL_POPALL;
				if (sysctl(MIB, MIBLEN + 2, NULL, NULL, buf, ebuf - buf))
					err(1, NULL);
				ebuf = buf = _buf;
			}
		} else if (strcmp(av[o], "-serial") == 0) {
        		cmd = IPFWCTL_SERIAL;
			n = sizeof(serial);
			if (sysctl(MIB, MIBLEN + 2, &serial, &n, NULL, NULL))
				err(1, "getting serial number");
			printf("%d\n", serial);
		} else if (strcmp(av[o], "-output") == 0) {
			struct ipfw_header ih;
			FILE *fp;
			char *b;

			cmd = IPFWCTL_PUSH;
			if (sysctl(MIB, MIBLEN + 2, NULL, &n, NULL, 0))
				err(1, "reading filters");

			if ((b = malloc(n)) == NULL)
				err(1, NULL);

			if (sysctl(MIB, MIBLEN + 2, b, &n, NULL, 0))
				err(1, "reading filters");

			if (strcmp(av[++o], "-") == 0)
				fp = stdout;
			else if ((fp = fopen(av[o], "w")) == NULL)
				err(1, av[o]);

			memset(&ih, 0, sizeof(ih));
			ih.magic = IPFW_MAGIC;
			if (fwrite(&ih, sizeof(ih), 1, fp) != 1 ||
			    fwrite(b, n, 1, fp) != 1)
				errx(1, av[o]);
			if (fp != stdout)
				fclose(fp);
			free(b);
		} else if (strcmp(av[o], "-display") == 0) {
			char path[1024];
			char *b;
			int hdr;

			hdr = (av[o+1] == NULL || av[o+1][0] == '-') ? 1 : 0;
			if (!hdr && av[o+2] && av[o+2][0] != '-')
				hdr = 1;

			cmd = IPFWCTL_PUSH;
			if (sysctl(MIB, MIBLEN + 2, NULL, &n, NULL, 0))
				err(1, "reading filters");

			if (n == 0)
				return(0);

			if ((b = malloc(n)) == NULL)
				err(1, NULL);

			if (sysctl(MIB, MIBLEN + 2, b, &n, NULL, 0))
				err(1, "reading filters");

			if (av[o+1] == NULL || av[o+1][0] == '-') {
				for (iftp = (ipfw_filter_t *)b;
				    (char *)iftp < b + n;
				    iftp = (ipfw_filter_t *)IPFW_next(iftp)) {
					snprintf(path, sizeof(path),
					    "%s/%d", _PATH_FILTERS, iftp->serial);
					if (hdr) {
						if (hdr++ > 1)
							printf("\n\n");
						printf("%.*s\n", printf("Filter %d\n", iftp->serial), dashes);
					}
#ifndef	NO_DISPLAY_SUPPORT
					if (iftp->type == IPFW_CIRCUIT)
						display_circuitcache(iftp);
					else if (iftp->type == IPFW_NAT)
						display_nat(what, iftp, 0);
					else if (iftp->type == IPFW_THROTTLE)
						display_throttle(what, iftp, 0);
					else if (iftp->type == IPFW_FLOW)
						display_flow(what, iftp, 0);
					else if (iftp->type == IPFW_CACHE)
						display_cache(what, iftp, 0);
					else
#endif
					if (cat(path) < 0)
						fprintf(stderr, "%d: content not found\n", iftp->serial);
				}
			} else while (av[o+1] != NULL && av[o+1][0] != '-') {
				serial = atoi(av[++o]);
				for (iftp = (ipfw_filter_t *)b;
				    (char *)iftp < b + n;
				    iftp = (ipfw_filter_t *)IPFW_next(iftp))
					if (serial == 0 ||
					    iftp->serial == serial)
						break;
				if ((char *)iftp < b + n) {
					snprintf(path, sizeof(path),
					    "%s/%d", _PATH_FILTERS, iftp->serial);
					if (hdr) {
						if (hdr++ > 1)
							printf("\n\n");
						printf("%.*s\n", printf("Filter %d\n", iftp->serial), dashes);
					}
#ifndef	NO_DISPLAY_SUPPORT
					if (iftp->type == IPFW_CIRCUIT)
						display_circuitcache(iftp);
					else if (iftp->type == IPFW_NAT)
						display_nat(what, iftp, 0);
					else if (iftp->type == IPFW_THROTTLE)
						display_throttle(what, iftp, 0);
					else if (iftp->type == IPFW_FLOW)
						display_flow(what, iftp, 0);
					else if (iftp->type == IPFW_CACHE)
						display_cache(what, iftp, 0);
					else
#endif
					if (cat(path) < 0)
						fprintf(stderr, "%d: content not found\n", iftp->serial);
				} else
					fprintf(stderr, "%d: no such filter\n", iftp->serial);
			}

		} else if (strcmp(av[o], "-stats") == 0) {
			ipfw_stats_t s;

			cmd = IPFWCTL_STATS;
			n = sizeof(s);
			if (sysctl(MIB, MIBLEN + 2, &s, &n, NULL, 0))
				err(1, NULL);
			printf("%s filter statistics:\n", av[1]);
			printf("%10qd packets rejected\n",
			    s.dropped + s.denied);
			printf("\t%10qd reported\n", s.denied);
			printf("%10qd packets accepted\n",
			    s.accepted + s.reported);
			printf("\t%10qd reported\n", s.reported);
			printf("%10qd errors while reporting\n",
			    s.reportfailed);
			printf("%10qd unknown disposition\n",
			    s.unknown);
		} else if (strcmp(av[o], "-move") == 0) {
			int a[2];
			a[0] = strtol(av[++o], 0, 0);
			a[1] = strtol(av[++o], 0, 0);
			cmd = IPFWCTL_MOVE;
			if (sysctl(MIB, MIBLEN + 2, NULL, NULL, a, sizeof(a)))
				err(1, "IPFWCTL_MOVE");
		} else if (strcmp(av[o], "-priority") == 0) {
			priority = strtol(av[++o], 0, 0);
		} else if (strcmp(av[o], "-dontsave") == 0) {
			dontsave = 1;
		} else if (strcmp(av[o], "-tag") == 0) {
			strncpy(tag, av[++o], sizeof(tag));
			tag[IPFW_TAGLEN] = '\0';
		} else
			errx(1, "%s: unknown command", av[o]);
	}
	FLUSH_FILTER

	return(0);
}


static char *
append_filter(char *name, char *buf, char *ebuf, char *serialbuf, int *mib, int *serialp)
{
	int c, i, n, fd;
	char *ext, ifname[128];	/* plenty long for an interface name */
	char data[8192];
	FILE *fp;
	struct ipfw_header ih;
	ipfw_filter_t *ift;
	int serial;

	if (serialbuf) {
		serial = strtol(serialbuf, &ext, 0);
		if (ext == serialbuf || *ext)
			errx(1, "%s: invalid serial number", serialbuf);
	} else {
		cmd = IPFWCTL_SERIAL;
		n = sizeof(serial);
		if (sysctl(MIB, MIBLEN + 2, &serial, &n, NULL, NULL))
			err(1, "getting serial number");
	}
	if (serialp)
		*serialp = serial;


	if (ebuf - buf < sizeof(ipfw_filter_t))
		errx(1, "filter(s) to large");

#ifndef	FORCE_DONT_SAVE
	if (!dontsave) {
		snprintf(ifname, sizeof(ifname), "%s/%d", _PATH_FILTERS, serial);
		if (geteuid() == 0)
			mkdir(_PATH_FILTERS, S_IRWXU);
		if ((fd = open(ifname, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR)) < 0)
			err(1, "%s", ifname);
	} else
#endif
		fd = -1;
	if ((fp = fopen(name, "r")) == NULL)
		err(1, "%s", name);
	if (fd >= 0) {
		while ((i = fread(data, 1, sizeof(data), fp)) > 0)
			if (write(fd, data, i) != i)
				err(1, "%s", ifname);
		close(fd);
	}

	if ((ext = strrchr(name, '.')) != NULL && strcmp(ext, ".ipfw") == 0) {
#ifdef	PHOENIX
		int argc = 0;
		char *argv[8];
#endif
		char *cmd;
		sprintf(ifname, _PATH_TMPIPFW, getindex());

#ifndef	PHOENIX
		fclose(fp);
		if (version_flags == NULL)
			version_flags = "";

		cmd = malloc(strlen("ipfwcmp -o") + strlen(ifname) +
		    strlen(version_flags) +
		    strlen(name) + 5);
		if (cmd == NULL)
			err(1, NULL);
		sprintf(cmd, "ipfwcmp %s -o %s %s",
		   version_flags, ifname, name);
		if (system(cmd)) {
			unlink(ifname);
			errx(1, "%s: could not compile", name);
		}
#else
		/*
                 * Build up an argv array a'la unix since this is what ipfwcmp
                 * expects. Also, ipfwcmp calls unix getopt which also expects
                 * this format.
                 */
                argv[argc++] = "ipfwcmp";
#if 0
    	        argv[argc++] = "-s";
#endif
		if (version_flags)
			argv[argc++] = version_flags;
    	        argv[argc++] = "-o";
	        argv[argc++] = ifname;
	        argv[argc++] = name;
	        argv[argc] = NULL;

                /*
		 * Create a semaphore for synchronization on task completion.
		 */

                syncSemIpfwCmp = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

                /*
		 * Spawn the compiler and wait for it to finish.
		 */

	        if (taskSpawn("ipfwcmp", 99, 0, 7000, (FUNCPTR)ipfwcmp,
		    argc, (int)argv, 0, 0, 0, 0, 0, 0, 0, 0) != ERROR) {
                	semTake(syncSemIpfwCmp, WAIT_FOREVER);

			/*
			 * Check the exit status of the compiler task.
			 */
			if (ipfwAsmError)
				errx(1, "ERROR: ipfwAsmError - ipfw exiting");

			if (ipfwCmpError)
				errx(1, "ERROR: ipfwCmpError - ipfw exiting");
                } else
			errx(1, "ERROR - could not spawn task ipfwcmp");
         
		/*
                 * We only need the semaphore once for each compilation of
                 * a filter. So, let's delete it now.
                 */
   
                semDelete(syncSemIpfwCmp);
#endif
		fp = fopen(ifname, "r");
#ifndef	PHOENIX
		unlink(ifname);
#endif
		if (fp == NULL)
			err(1, "%s (compiled from %s)", ifname, name);
	} else
		rewind(fp);

	if (fread(&ih, sizeof(ih), 1, fp) != 1) {
		if (feof(fp))
			errx(1, "%s: not a BSD/OS IP Filter", name);
		else
			err(1, "%s", name);
	}

	if (ih.magic != IPFW_MAGIC)
		errx(1, "%s: not a BSD/OS IP Filter", name);

	switch (ih.type) {

	case IPFW_BPF: {
		struct bpf_insn *p;

		ift = (ipfw_filter_t *)buf;
		ift->type = ih.type;
		ift->length = ih.icnt * sizeof(struct bpf_insn);
		ift->serial = serial;
		ift->hlength = sizeof(*ift);
		ift->priority = priority;
		memcpy(ift->tag, tag, IPFW_TAGLEN);

		p = (struct bpf_insn *)(ift + 1);
		buf = (char *)IPFW_next(ift);

		if (buf > ebuf)
			errx(1, "filter(s) to large");

		if (fread(p, ift->length, 1, fp) != 1) {
			if (feof(fp))
				errx(1, "%s: truncated file", name);
			else
				err(1, "%s", name);
		}


		while (fread(&c, sizeof(int), 1, fp) == 1) {
			if (c >= ih.icnt)
				errx(1, "pc %d out of range (%d)", c, ih.icnt);

			i = 0;
			do {
				if (fread(ifname+i, sizeof(int), 1, fp) != 1) {
					if (feof(fp))
						errx(1, "%s: truncated file",
						    name);
					else
						err(1, "%s", name);
				}
				i += sizeof(int);
			} while (i < sizeof(ifname) && ifname[i-1] != '\0');

			if (i >= sizeof(ifname))
				errx(1, "%s: interface name too long", name);

			if (*ifname == '*') {
				if ((i = callfilter(ifname+1)) < 0)
					errx(1, "%s: no such filter", ifname+1);
			} else if ((i = if_nametoindex(ifname)) <= 0)
				errx(1, "%s: no such interface", ifname);
			p[c].k = i;
		}
	    }

	case IPFW_CIRCUIT:
	case 0:
		while (fread(buf, sizeof(*ift), 1, fp) == 1) {
			ift = (ipfw_filter_t *)buf;
			buf = (char *)IPFW_next(ift);
			if (buf > ebuf)
				errx(1, "filter(s) to large");
			if (IPFW_len(ift) &&
			    fread(ift + 1, IPFW_len(ift), 1, fp) != 1) {
				if (feof(fp))
					errx(1, "%s: truncated file", name);
				else
					err(1, "%s", name);
			}
		}
		break;

	default:
		errx(1, "%s: unknown filter type", name);
	}
	if (ferror(fp))
		err(1, "%s", name);
	fclose(fp);
	return (buf);
}

static int
callfilter(char *filter)
{
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW, IPFW_CALL,
	    IPFWCTL_CALLFILTERS };
	ipfw_call_t *ic;

	static size_t n = 0xffffffff;
	static char *b = NULL;

	if (n == 0xffffffff) {
		if (sysctl(MIB, MIBLEN + 2, NULL, &n, NULL, 0))
			err(1, "reading call filter list");

		if (n == 0)
			return(0);

		if ((b = malloc(n)) == NULL)
			err(1, NULL);

		if (sysctl(MIB, MIBLEN + 2, b, &n, NULL, 0))
			err(1, "reading filters");
	}

	for (ic = (ipfw_call_t *)b; (char *)ic < b + n; ++ic) {
		if (strncmp(filter, ic->tag, IPFW_TAGLEN) == 0) {
			return (ic->index | ((int)ic->generation << 16));
		}
	}
	return (-1);
}

static int
cat(char *path)
{
	int fd, r, w, n;
	char buf[8192];

	if ((fd = open(path, O_RDONLY, 0)) < 0)
		return (fd);
	fflush(stdout);
	while ((r = read(fd, buf, sizeof(buf))) > 0) {
		for (w = 0; w < r; w += n)
			if ((n = write(1, buf + w, r - w)) <= 0) {
				warn(NULL);
				break;
			}
	}
	close(fd);
	return (0);
	
}
