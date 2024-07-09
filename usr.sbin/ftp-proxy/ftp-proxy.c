/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ftp-proxy.c,v 1.11 2003/06/06 15:58:19 polk Exp
 *
 *  FTP Proxy -- transparent FTP proxy
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_nat.h>

#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define _PATH_PID	"/var/run/ftp-proxy.pid"

char pre_input_filter[] = 
"!tcp || dstaddr(%LOCAL_ADDRS%) {\n\
        next;\n\
}\n\
\n\
//\n\
// We respond to all packets coming into our service\n\
//\n\
dstport(%EXTPORT%) {\n\
	forcelocal;\n\
}\n\
//\n\
// We also need to catch the return packets\n\
//\n\
srcport(%EXTPORT%) {\n\
	forcelocal;\n\
}\n\
next;\n\
";

char pre_input_proxy[] = 
"!tcp || dstaddr(%LOCAL_ADDRS%) {\n\
        next;\n\
}\n\
\n\
//\n\
// Do not allow any packets coming in for our proxy port\n\
//\n\
dstport(%INTPORT%) {\n\
	reject;\n\
}\n\
\n\
//\n\
// Packets coming into our service get munged to our proxy port\n\
// and we respond to the packet.\n\
//\n\
dstport(%EXTPORT%) {\n\
	change dstport from %EXTPORT% to %INTPORT%;\n\
	forcelocal;\n\
}\n\
//\n\
// We also need to catch the return packets\n\
//\n\
srcport(%EXTPORT%) {\n\
	forcelocal;\n\
}\n\
next;\n\
";

char pre_output_proxy[] = 
"!tcp {\n\
        next;\n\
}\n\
\n\
//\n\
// If a packet is coming from our internal proxy port then we\n\
// need to make it look like it is coming from the external port\n\
//\n\
srcport(%INTPORT%) {\n\
	change srcport from %INTPORT% to %EXTPORT%;\n\
}\n\
next;\n\
";

#define	sin	gcc_aint_got_no_right_to_use_sin

extern int nlocs;
extern void expire(int, int);
extern int insert_entry(int, struct in_addr *, struct in_addr *, u_long);
extern int install_circuit_cache(int, u_long, char *);

static struct ifaddrs *ifa;
static struct sockaddr_in s_to, s_from;
static struct sockaddr_in sin;
static ip_nat_t nat;			/* nat entry of the control session */
static u_short extport;
static int s_master;
static int serial;
static int seriald;
static int serialn;
static int verbose;
static int cflag;
static int class;
static int expiretime;
static int highnat, lownat;
static int nonat;
static int zflag;

#define	FTP_CLIENT	1				/* FTP client */
#define	FTP_SERVER	2				/* FTP server */
#define	FTP_EITHER	(FTP_CLIENT | FTP_SERVER)	/* could be either */
#define	NAT_SESSION	0x10
#define	NAT_SERVER	(NAT_SESSION | FTP_SERVER)	/* server in our NAT */
#define	NAT_CLIENT	(NAT_SESSION | FTP_CLIENT)	/* client in our NAT */

#define	BUFFER_SIZE	4096

static int filter(char *, int *, char *, int *);
static void setfilter(char *, char *, int, char *);

static void addnat(ip_nat_t *);
static void addmap(void *, int);

static int bindsocket(struct sockaddr_in *);
static void scannats();
static void classify(struct sockaddr_in *, struct sockaddr_in *);
static void died(int s);
static void dying();
static void getnames(int, void *, void *, socklen_t);
static void removeproxy(int, char *);
static void tick(int);
static void reap(int);

int
main(int ac, char **av)
{
	FILE *fp;
	fd_set *rfd, *wfd;
	char buf_read[BUFFER_SIZE], buf_write[BUFFER_SIZE];
	char buf_in[BUFFER_SIZE], buf_out[BUFFER_SIZE];
	struct addrinfo *ai;
	struct itimerval itv;
	struct timeval tv;
	int rflag, s, useproxy;
	int priority, outpriority;
	socklen_t slen;
	int s_in, s_out, nfds, r_in, r_out, r_write, r_read, r, i;
	char hostname[64];

	if (getifaddrs(&ifa) < 0)
		err(1, NULL);

	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(sin);
	sin.sin_addr.s_addr = 0;
	extport = htons(21);

	useproxy = 0;
	verbose = 0;
	rflag = 0;
	priority = 512;
	outpriority = 512 + 1024;
	itv.it_interval.tv_sec = 1;
	itv.it_interval.tv_usec = 0;

	while ((r = getopt(ac, av, "ch:n:No:p:P:rt:vz")) != EOF) {
		switch (r) {
		case 'c':
			cflag = 1;
			break;

		case 'h':
			if ((r = getaddrinfo(optarg, NULL, NULL, &ai)) != 0)
				errx(1, "%s: %s", optarg, gai_strerror(r));
			while (ai) {
				if (ai->ai_family == sin.sin_family) {
					if (sin.sin_addr.s_addr) {
						warnx("multiple addresses found for %s, using first found", optarg);
						break;
					}
					sin.sin_addr = ((struct sockaddr_in *)ai->ai_addr)->sin_addr;
				}
				ai = ai->ai_next;
			}
			if (sin.sin_addr.s_addr == 0)
				errx(1, "%s: no acceptable address found", optarg);
			break;

		case 'N':
			nonat = 1;
			break;

		case 'n':
			i = atoi(optarg);
			if (i < 1 || i > nlocs || ((i - 1) & i))
				errx(1, "-n takes a power of 2 between 1 and %d\n", nlocs);
			nlocs = i;
			break;

		case 'o':
			outpriority = atoi(optarg);
			break;

		case 'p':
			priority = atoi(optarg);
			break;

		case 'P':
			extport = htons(atoi(optarg));
			if (extport == 0)
				errx(1, "%s: invalid port", optarg);
			break;

		case 'r':
			rflag++;
			break;

		case 't':
			i = atoi(optarg);
			if (i < 1 || i > 0xffff)
				errx(1, "-t takes a value between 1 and %d\n", 0xffff);
			itv.it_interval.tv_sec = i;
			break;

		case 'v':
			++verbose;
			break;

		case 'z':	/* Don't protect ourselves */
			++zflag;
			break;

		default:
		usage:
			fprintf(stderr, 
"Usage:	ftp-proxy [-cv] [-h host] [-n ticks] [-o priority] [-p priority]\n\t    [-P port] [-t tickrate] [proxyport]\n\tftp-proxy -r\n");
			exit(1);
		}
			
	}
	expiretime = nlocs * itv.it_interval.tv_sec;

	if (rflag) {
		if ((fp = fopen(_PATH_PID, "r")) != NULL) {
			if (fscanf(fp, "%d", &rflag) == 1)
				kill(rflag, SIGHUP);
			fclose(fp);
		}
		removeproxy(IPFW_FORWARD, "ftp-proxy-in");
		removeproxy(IPFW_FORWARD, "ftp-proxy-out");
		removeproxy(IPFW_PREINPUT, "ftp-proxy");
		exit(0);
	}
	
	if (optind != ac && optind + 1 != ac)
		goto usage;

	if (optind == ac)
		sin.sin_port = extport;
	else
		sin.sin_port = htons(atoi(av[optind]));

	if (sin.sin_port == 0)
		errx(1, "%s: invalid port", av[optind]);
		
	atexit(dying);
	scannats();
	if (highnat >= lownat && priority >= lownat)
		errx(1, "pre-input priority %d is greater the NAT priority of %d",
		    priority, lownat);

	if (extport == sin.sin_port)
		setfilter("pre-input", "ftp-proxy", priority, pre_input_filter);
	else {
		if (highnat >= lownat && outpriority <= highnat)
			errx(1, "pre-output priority %d is lower than the NAT priority of %d",
			    outpriority, highnat);
		useproxy = 1;
		setfilter("pre-input", "ftp-proxy", priority, pre_input_proxy);
		setfilter("pre-output", "ftp-proxy", outpriority, pre_output_proxy);
	}

	if (cflag) {
		serial = install_circuit_cache(0, 0xffffffff, "ftp-proxy-in");
		if (serial < 0)
			err(1, "circuit cache");
		seriald = install_circuit_cache(0, htonl(0x0000ffff), "ftp-proxy-out");
		if (seriald < 0)
			err(1, "circuit daemon cache");
	}

	s_master = bindsocket(&sin);

	if (listen(s_master, 64) < 0)
		err(1, NULL);

	signal(SIGALRM, tick);
	signal(SIGCHLD, reap);
	itv.it_value = itv.it_interval;
	setitimer(ITIMER_REAL, &itv, NULL);

	for (s = 1; s < NSIG; ++s) {
		switch (s) {
		/* case SIGABRT: don't trap this one */
		case SIGILL:
		case SIGTRAP:
		case SIGEMT:
		case SIGFPE:
		case SIGBUS:
		case SIGSEGV:
			if (zflag)
				break;
			/* FALLTHRU */
		case SIGINT:
		case SIGQUIT:
		case SIGSYS:
		case SIGPIPE:
		case SIGTERM:
		case SIGHUP:
			signal(s, died);
			break;
		}
	}

	unlink(_PATH_PID);
	if ((fp = fopen(_PATH_PID, "w")) != NULL) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}

	for (;;) {
		slen = sizeof(sin);
		s_in = accept(s_master, (struct sockaddr *)&sin, &slen);

		if (s_in < 0) {
			if (errno == EINTR)
				continue;
			err(1, NULL);
		}

		getnames(s_in, &s_from, &s_to, sizeof(s_from));

		if (verbose) {
			inet_ntop(s_from.sin_family, &s_from.sin_addr, hostname,
			    sizeof(hostname));
			printf("Connection from %s:%d\n", hostname,
			    ntohs(s_from.sin_port));
			inet_ntop(s_to.sin_family, &s_to.sin_addr, hostname,
			    sizeof(hostname));
			printf("Connection to %s:%d\n", hostname,
			    ntohs(s_to.sin_port));
		}

		switch (fork()) {
		case -1:
			err(1, NULL);
		default:
			close(s_in);
			continue;
		case 0:
			close(s_master);
			s_master = -1;
			break;
		}
		break;
	}

	s_out = bindsocket(&s_from);
	s_to.sin_port = extport;

	/*
	 * Check to see if this is a non-proxied request for an FTP
	 * server on this machine.  The NAT has already happened so we
	 * won't catch redirected servers.  Since we are non-proxied it
	 * means that WE are the ftp server on this machine (i.e., there
	 * is no FTP server).  We just blow them off in this case.
	 */

	if (!useproxy) {
		struct ifaddrs *tifa;
		for (tifa = ifa; tifa; tifa = tifa->ifa_next) {
			struct sockaddr_in *s;

			s = (struct sockaddr_in *)tifa->ifa_addr;
			if (s->sin_family == s_to.sin_family &&
			    s->sin_addr.s_addr == s_to.sin_addr.s_addr)
				break;
		}
		if (tifa) {
			if (verbose > 1)
				printf("request to local address\n");
			write(s_in, "421 Service not available\r\n", 27);
			exit(0);
		}
	}

	classify(&s_from, &s_to);

	if (connect(s_out, (struct sockaddr *)&s_to, sizeof(s_to)) < 0)
		err(1, "connect");

	if (class == FTP_EITHER)
		classify(&s_from, &s_to);

	nfds = ((s_out > s_in) ? s_out : s_in) + 1;

	rfd = FD_ALLOC(nfds);
	wfd = FD_ALLOC(nfds);

	r_read = r_write = r_in = r_out = 0;

	tv.tv_usec = 10000;
	tv.tv_sec = 0;
	for (;;) {
		FD_ZERO(rfd);
		FD_ZERO(wfd);

		if (sizeof(buf_in) > r_in)
			FD_SET(s_in, rfd);

		if (sizeof(buf_out) > r_out)
			FD_SET(s_out, rfd);

		if (r_write)
			FD_SET(s_out, wfd);

		if (r_read)
			FD_SET(s_in, wfd);

		if (r_in == 0 && r_out == 0 && r_write == 0 && r_read == 0)
			tv.tv_usec = 0;
		else if (tv.tv_usec < 1000000 / 2)
			tv.tv_usec += 10000;


		if (select(nfds, rfd, wfd, 0,
		    (tv.tv_sec || tv.tv_usec) ? &tv : NULL) < 0) {
			if (zflag)
				fprintf(stderr, "%s:%d: exiting\n",
				    __FILE__, __LINE__);
			close(s_in); close(s_out);
			err(1, "select");
		}

		if (FD_ISSET(s_in, rfd)) {
			tv.tv_usec = 0;
			r = read(s_in, buf_in + r_in, sizeof(buf_in) - r_in);
			if (r <= 0) {
				while (r_in > 0 &&
				    (r = write(s_out, buf_in, r_in)) > 0) {
					if (r < r_in)
						memcpy(buf_in, buf_in + r, r_in - r);
					r_in -= r;
				}
				close(s_in); close(s_out);
				if (zflag)
					fprintf(stderr, "%s:%d: exiting\n",
					    __FILE__, __LINE__);
				exit(0);
			}
			r_in += r;
		}
		assert(r_in >= 0 && r_in <= sizeof(buf_in));
		assert(r_write >= 0 && r_write <= sizeof(buf_write));
		if (filter(buf_in, &r_in, buf_write, &r_write))
			tv.tv_usec = 0;
		if (FD_ISSET(s_out, rfd)) {
			tv.tv_usec = 0;
			r = read(s_out, buf_out + r_out, sizeof(buf_out) - r_out);
			if (r <= 0) {
				while (r_out > 0 &&
				    (r = write(s_in, buf_out, r_out)) > 0) {
					if (r < r_out)
						memcpy(buf_out, buf_out + r, r_out - r);
					r_out -= r;
				}
				close(s_in); close(s_out);
				if (zflag)
					fprintf(stderr, "%s:%d: exiting\n",
					    __FILE__, __LINE__);
				exit(0);
			}
			r_out += r;
		}
		assert(r_out >= 0 && r_out <= sizeof(buf_out));
		assert(r_read >= 0 && r_read <= sizeof(buf_read));
		if (filter(buf_out, &r_out, buf_read, &r_read))
			tv.tv_usec = 0;
		if (r_read && FD_ISSET(s_in, wfd)) {
			tv.tv_usec = 0;
			if ((r = write(s_in, buf_read, r_read)) <= 0) {
				close(s_in); close(s_out);
				err(1, NULL);
			}
			if (r < r_read)
				memcpy(buf_read, buf_read + r, r_read - r);
			r_read -= r;
		}
		if (r_write && FD_ISSET(s_out, wfd)) {
			tv.tv_usec = 0;
			if ((r = write(s_out, buf_write, r_write)) <= 0) {
				close(s_in); close(s_out);
				err(1, NULL);
			}
			if (r < r_write)
				memcpy(buf_write, buf_write + r, r_write - r);
			r_write -= r;
		}
	}
}

static int
filter(char *buf_in, int *r_inp, char *buf_write, int *r_writep)
{
	int r_write = *r_writep;
	int r_in = *r_inp;
	int r, i, rw;
	struct sockaddr_in sin;
	ip_nat_t natt;
	char *eol, *buf, *nl;

    while (r_in > 0) {
	assert(r_write >= 0 && r_write <= BUFFER_SIZE);
	assert(r_in >= 0 && r_write <= BUFFER_SIZE);

	eol = memchr(buf_in, '\n', r_in);
	if (eol++ == NULL) {
		if (r_in == BUFFER_SIZE)
			eol = buf_in + r_in;
		else
			return (0);
	}
	r = eol - buf_in;
	assert(r >= 0 && r <= BUFFER_SIZE);

	/*
	 * If we are going to overflow our buffer then flush out what
	 * we currently have.  If we have nothing then just copy the
	 * first part of this VERY LONG LINE and flush it out.
	 */
	if (r_write + r > BUFFER_SIZE / 2) {
		if (zflag)
			fprintf(stderr, "Large buffer: %d bytes on top of %d\n", r, r_write);
		if (r_write == 0) {
			memcpy(buf_write, buf_in, BUFFER_SIZE / 2);
			*r_writep = BUFFER_SIZE / 2;
			*r_inp = r_in - BUFFER_SIZE / 2;
			return (1);
		}
		return (0);
	}

	rw = r_write;
	memcpy(buf_write + r_write, buf_in, r);
	r_write += r;

	if (verbose > 1) {
		if (strncmp(buf_in, "PASS ", 5) == 0)
			printf("%08x:%d -> %08x:%d PASS %.*s\n",
			    ntohl(s_from.sin_addr.s_addr),
			    ntohs(s_from.sin_port),
			    ntohl(s_to.sin_addr.s_addr),
			    ntohs(s_to.sin_port), r - 5,
			    "***********************************...");
		else
			printf("%08x:%d -> %08x:%d %.*s", 
			    ntohl(s_from.sin_addr.s_addr),
			    ntohs(s_from.sin_port),
			    ntohl(s_to.sin_addr.s_addr),
			    ntohs(s_to.sin_port), r, buf_in);
	}

	nl = eol;
	if (strncmp(buf_in, "PORT ", 5) == 0) {
		int c;
		buf = buf_in + 5;

		*--eol = '\0';
		if (eol[-1] == '\r')
			*--eol = '\0';

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr = c << 24;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c << 16;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c << 8;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_port = c << 8;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != '\0')
			goto out;
		sin.sin_port |= c;

		HTONL(sin.sin_addr.s_addr);

		if (sin.sin_addr.s_addr != s_from.sin_addr.s_addr) {
			printf("Wrong IP address!\n");
			goto out;
		}

		if (cflag) {
			if (verbose) {
				printf("Insert entry %08x %08x / %d -> 20\n",
					ntohl(s_from.sin_addr.s_addr),
					ntohl(s_to.sin_addr.s_addr),
					ntohs(sin.sin_port));
				sleep(1);
			}
			i = insert_entry(serial, &s_from.sin_addr, &s_to.sin_addr,
			    htonl(((u_long)sin.sin_port << 16) | 20));
		}

		if (class == NAT_CLIENT) {
			natt = nat;
			HTONL(natt.nat_external.s_addr);
			r_write = rw;		/* rewind output buffer */
			r_write += sprintf(buf_write + r_write,
			    "PORT %d,%d,%d,%d,%d,%d\r\n",
			    (natt.nat_external.s_addr >> 24) & 0xff,
			    (natt.nat_external.s_addr >> 16) & 0xff,
			    (natt.nat_external.s_addr >>  8) & 0xff,
			    (natt.nat_external.s_addr      ) & 0xff,
			    (sin.sin_port >> 8) & 0xff,
			    sin.sin_port & 0xff);
			if (verbose > 1)
				printf("rewrote: %s", buf_write + rw);
			HTONL(natt.nat_external.s_addr);

			natt.nat_rpip.p.dst =
			natt.nat_iprp.p.src =
			natt.nat_rpep.p.dst =
			natt.nat_eprp.p.src = htons(sin.sin_port);

			natt.nat_rpip.p.src =
			natt.nat_iprp.p.dst =
			natt.nat_rpep.p.src =
			natt.nat_eprp.p.dst = htons(20);

			/*
			 * Make it look like we came from the pre-input
			 * filter.  This way we will eventually time out
			 * if we are not used.
			 */
			natt.nat_direction = IPFW_PREINPUT >> 24;

			/*
			 * We give them 60 seconds to start up the session
			 */
			natt.nat_when = 60;
			addnat(&natt);
		}
	} else if (strncmp(buf_in, "227 ", 4) == 0) {
		int c;

		*--eol = '\0';
		if ((buf = strrchr(buf_in, '('/*)*/)) == NULL)
			goto out;
		if ((eol = strchr(++buf, /*(*/')')) == NULL)
			goto out;
		*eol = '\0';

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr = c << 24;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c << 16;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c << 8;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_addr.s_addr |= c;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != ',')
			goto out;
		sin.sin_port = c << 8;
		buf = eol + 1;

		c = strtol(buf, &eol, 10);
		if (c < 0 || (c == 0 && buf == eol) || c > 255 || *eol != '\0')
			goto out;
		sin.sin_port |= c;

		if (cflag) {
			HTONL(sin.sin_addr.s_addr);

			if (sin.sin_addr.s_addr != s_to.sin_addr.s_addr) {
				printf("Wrong IP address!\n");
				goto out;
			}
			if (verbose) {
				printf("Insert entry %08x %08x / %d\n",
					ntohl(s_from.sin_addr.s_addr),
					ntohl(s_to.sin_addr.s_addr),
					sin.sin_port);
				sleep(1);
			}
			i = insert_entry(seriald, &s_from.sin_addr, &s_to.sin_addr,
			    htonl((u_long)sin.sin_port));
		}
		if (class == NAT_SERVER) {
			struct {
				ip_natdef_t nd;
				ip_natservice_t ns;
			} d;

			natt = nat;
			HTONL(natt.nat_external.s_addr);
			r_write = rw;		/* rewind output buffer */
			r_write += sprintf(buf_write + r_write,
			    "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",
			    (natt.nat_external.s_addr >> 24) & 0xff,
			    (natt.nat_external.s_addr >> 16) & 0xff,
			    (natt.nat_external.s_addr >>  8) & 0xff,
			    (natt.nat_external.s_addr      ) & 0xff,
			    (sin.sin_port >> 8) & 0xff,
			    sin.sin_port & 0xff);
			HTONL(natt.nat_external.s_addr);
			if (verbose > 1)
				printf("rewrote: %s", buf_write + rw);

			d.nd.nd_nservices = 1;
			d.nd.nd_nmaps = 0;
			d.ns.ns_next = 0;
			d.ns.ns_serial = 0;
			d.ns.ns_expire = time(0) + expiretime;
			d.ns.ns_internal = natt.nat_internal;
			d.ns.ns_external = natt.nat_external;
			d.ns.ns_eport = htons(sin.sin_port);
			d.ns.ns_iport = d.ns.ns_eport;
			d.ns.ns_protocol = IPPROTO_TCP;
			addmap(&d, sizeof(d));
		}
	}

out:
	if (r_in > r) {
		memcpy(buf_in, nl, r_in - r);
		r_in -= r;
	} else
		r_in = 0;

	*r_inp = r_in;
	*r_writep = r_write;
    }
	return (1);
}

static void
setfilter(char *name, char *tag, int priority, char *template)
{
	char path[1024], buf[1024];
	FILE *fp;
	char *percent;
	int r;

	snprintf(path, sizeof(path), "/tmp/proxy.%d.ipfw", getpid());
	unlink(path);

	if ((fp = fopen(path, "w")) == NULL)
		err(1, "%s", path);

	while (*template) {
		if (*template != '%') {
			percent = strchr(template, '%');
			if (percent == NULL) {
				fprintf(fp, "%s", template);
				break;
			}
			fprintf(fp, "%.*s", percent - template, template);
			template = percent;
		}
		if ((percent = strchr(template + 1, '%')) == NULL) {
			unlink(path);
			err(1, "%s template: unmatched %%'s", name);
		}
		++percent;
		if (strncmp(template, "%LOCAL_ADDRS%", percent-template) == 0) {
			char *sep = "";
			struct ifaddrs *tifa;
			for (tifa = ifa; tifa; tifa = tifa->ifa_next) {
				struct sockaddr_in *s;

				s = (struct sockaddr_in *)tifa->ifa_addr;
				if (s->sin_family == sin.sin_family) {
					fprintf(fp, "%s%s", sep,
					    inet_ntop(s->sin_family,
					    &s->sin_addr, buf, sizeof(buf)));
					sep = ", ";
				}
			}
		} else
		if (strncmp(template, "%INTPORT%", percent-template) == 0) {
			fprintf(fp, "%d", ntohs(sin.sin_port));
		} else
		if (strncmp(template, "%EXTPORT%", percent-template) == 0) {
			fprintf(fp, "%d", ntohs(extport));
		} else {
			unlink(path);
			err(1, "%.*s: unknown variable",
			    percent-template, template);
		}
		template = percent;
	}
	fclose(fp);
    
	snprintf(buf, sizeof(buf), "ipfw %s -priority %d -tag %s -push %s",
		name, priority, tag, path);
	r = system(buf);
	if (!WIFEXITED(r) || WEXITSTATUS(r) != 0)
		errx(1, "failed to install filter");
}

static void
addnat(ip_nat_t *nat)
{
        static int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
                IPFW_PREINPUT, IPFWCTL_SYSCTL, IPFW_NAT, /*serial*/0,
                IPFWNAT_ADDNAT };

	mib[7] = serialn;
	if (sysctl(mib, 9, NULL, NULL, nat, sizeof(ip_nat_t)) < 0)
		err(1, "Adding NAT");
}

static void
classify(struct sockaddr_in *f, struct sockaddr_in *t)
{
        static int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
                IPFW_PREINPUT, IPFWCTL_LIST, IPFW_NAT, /*serial*/0,
		IPFWNAT_LOOKUP };
        size_t n, m;
        char *b;
        ipfw_filter_t *ift;

	if (nonat)
		goto no_nat;

	mib[5] = IPFWCTL_LIST;
        if (sysctl(mib, 6, NULL, &n, NULL, NULL) < 0)
                err(1, "listing NAT entries");
        if (n == 0)
		goto no_nat;

        if ((b = malloc(n)) == NULL)
                err(1, NULL);

        if (sysctl(mib, 6, b, &n, NULL, NULL) < 0)
                err(1, "getting NAT entries");
        ift = (ipfw_filter_t *)b;
        while (n > 0) {
                if (ift->type == IPFW_NAT) {
			mib[5] = IPFWCTL_SYSCTL;
			mib[7] = ift->serial;
         
			if (verbose)
				printf("Check out NAT %d\n", mib[7]);

			memset(&nat, 0, sizeof(nat));
			nat.nat_internal = f->sin_addr;
			nat.nat_iprp.p.src = f->sin_port;
			nat.nat_remote = t->sin_addr;
			nat.nat_iprp.p.dst = t->sin_port;
			nat.nat_protocol = IPPROTO_TCP;
			m = sizeof(nat);
			if (sysctl(mib, 9, &nat, &m, &nat, sizeof(nat)) < 0)
				err(1, "looking for NAT");
			if (m) {
				if (verbose)
					printf("We are a NAT client\n");
				class = NAT_CLIENT;
				serialn = ift->serial;
				free(b);
				return;
			}
         
			memset(&nat, 0, sizeof(nat));
			nat.nat_internal = t->sin_addr;
			nat.nat_iprp.p.src = t->sin_port;
			nat.nat_remote = f->sin_addr;
			nat.nat_iprp.p.dst = f->sin_port;
			nat.nat_protocol = IPPROTO_TCP;

			m = sizeof(nat);
			if (sysctl(mib, 9, &nat, &m, &nat, sizeof(nat)) < 0)
				err(1, "looking for NAT");
			if (m) {
				if (verbose)
					printf("We are a NAT server\n");
				class = NAT_SERVER;
				serialn = ift->serial;
				free(b);
				return;
			}
                }
                n -= ift->hlength;
                ift = (ipfw_filter_t *)(((char *)ift) + ift->hlength);
        }
        free(b);

no_nat:
	if (verbose)
		printf("We are a normal FTP session\n");
	class = FTP_EITHER;
	serialn = 0;
}

static void
scannats()
{
        static int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
                IPFW_PREINPUT, IPFWCTL_LIST };
        size_t n, m;
        char *b;
        ipfw_filter_t *ift;

	highnat = 0;
	lownat = 1;

	if (nonat)
		return;

        if (sysctl(mib, 6, NULL, &n, NULL, NULL) < 0)
                err(1, "listing NAT entries");
        if (n == 0)
		return;

        if ((b = malloc(n)) == NULL)
                err(1, NULL);

        if (sysctl(mib, 6, b, &n, NULL, NULL) < 0)
                err(1, "getting NAT entries");
        ift = (ipfw_filter_t *)b;
        while (n > 0) {
                if (ift->type == IPFW_NAT) {
			if (verbose > 0)
				printf("NAT found at priority %d\n",
				    ift->priority);
			if (lownat > highnat)
				lownat = highnat = ift->priority;
			else if (ift->priority > highnat)
				highnat = ift->priority;
			else if (ift->priority < lownat)
				lownat = ift->priority;
		}
                n -= ift->hlength;
                ift = (ipfw_filter_t *)(((char *)ift) + ift->hlength);
        }
        free(b);
}

static void
addmap(void *map, int length)
{
        static int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
                IPFW_PREINPUT, IPFWCTL_SYSCTL, IPFW_NAT, /*serial*/0,
                IPFWNAT_SETUP };

	mib[7] = serialn;
	if (sysctl(mib, 9, NULL, NULL, map, length) < 0)
		err(1, "Adding MAP");
}

static int
bindsocket(struct sockaddr_in *sin)
{
	int s, on;

        s = socket(AF_INET, SOCK_STREAM, 0);
        on = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
                err(1, "SO_REUSEADDR");
        on = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
                err(1, "SO_REUSEPORT");
        on = 1;
        if (setsockopt(s, SOL_SOCKET, SO_BINDANY, &on, sizeof(on)) < 0)
                err(1, "SO_BINDANY");
        if (bind(s, (struct sockaddr *)sin, sizeof(*sin)) < 0)
                err(1, "bind");
	return (s);
}

static void
died(int s)
{
	signal(s, SIG_DFL);
	if (s_master >= 0) {
		removeproxy(IPFW_FORWARD, "ftp-proxy-in");
		removeproxy(IPFW_FORWARD, "ftp-proxy-out");
		removeproxy(IPFW_PREINPUT, "ftp-proxy");
		removeproxy(IPFW_PREOUTPUT, "ftp-proxy");
	}
	if (zflag)
		abort();
	if (zflag)
		fprintf(stderr, "%s:%d: exiting\n",
		    __FILE__, __LINE__);
	exit(0);
}

static void
dying()
{
	if (zflag)
		fprintf(stderr, "%s:%d: exiting\n",
		    __FILE__, __LINE__);
	if (s_master >= 0) {
		removeproxy(IPFW_FORWARD, "ftp-proxy-in");
		removeproxy(IPFW_FORWARD, "ftp-proxy-out");
		removeproxy(IPFW_PREINPUT, "ftp-proxy");
		removeproxy(IPFW_PREOUTPUT, "ftp-proxy");
	}
}

static void
getnames(int s, void *f, void *t, socklen_t slen)
{
	if (getpeername(s, (struct sockaddr *)f, &slen) < 0)
		err(1, "getpeername");

	if (getsockname(s, (struct sockaddr *)t, &slen) < 0)
		err(1, "getsockname");
}

static void
removeproxy(int who, char *tag)
{
        static int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_IPFW,
                IPFW_PREINPUT, IPFWCTL_LIST };
	char *b;
	ipfw_filter_t *ift;
	size_t n;

	mib[4] = who;
	mib[5] = IPFWCTL_LIST;

	if (sysctl(mib, 6, NULL, &n, NULL, NULL) < 0)
		err(1, "listing ipfw entries");

	if (n == 0)
		return;

	if ((b = malloc(n)) == NULL)
		err(1, NULL);

	if (sysctl(mib, 6, b, &n, NULL, NULL) < 0)
		err(1, "getting ipfw entries");
	ift = (ipfw_filter_t *)b;
	while (n > 0) {
		if (strcmp(ift->tag, tag) == 0) {
			mib[5] = IPFWCTL_POP;
			if (sysctl(mib, 6, NULL, NULL, &ift->serial, sizeof(int)) < 0)
				err(1, "removing ipfw entry");
			break;
		}
		n -= ift->hlength;
		ift = (ipfw_filter_t *)(((char *)ift) + ift->hlength);
	}

	free(b);
}

static void
tick(int s)
{
	static int location = 0;
	if (cflag) {
		expire(serial, location);
		expire(seriald, location);
	}
	location = (location + 1) & (nlocs-1);
}

static void
reap(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}
