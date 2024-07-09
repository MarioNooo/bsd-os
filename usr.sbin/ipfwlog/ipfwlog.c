/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfwlog.c,v 1.12 2002/05/06 20:29:59 chrisk Exp
 */
#define	IPFW
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ipfw.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define	SYSLOG_NAMES
#include <syslog.h>
#include <unistd.h>

#define	_PATH_PID	"/var/run/ipfwlog.pid"

int nflag = 0;
int cflag = 0;
FILE *lfp, _usesyslog, *usesyslog = &_usesyslog;
char *logfile;
int log_prio = LOG_NOTICE;
int log_facility = LOG_USER;

void
cracksyslog(char *facility)
{
	char *priority;
	int i;

    	if ((priority = strchr(facility, '.')) != NULL)
		*priority++ = '\0';
	if (facility && *facility) {
		for (i = 0; facilitynames[i].c_name; ++i)
			if (strcmp(facility, facilitynames[i].c_name) == 0)
				break;
		if (facilitynames[i].c_name == NULL)
			errx(1, "%s: unknown facility", facility);
		log_facility = facilitynames[i].c_val;
	}
	if (priority && *priority) {
		for (i = 0; prioritynames[i].c_name; ++i)
			if (strcmp(priority, prioritynames[i].c_name) == 0)
				break;
		if (prioritynames[i].c_name == NULL)
			errx(1, "%s: unknown priority", priority);
		log_prio = prioritynames[i].c_val;
	}
}

void
log(char *fmt, ...)
{
	static char logbuf[8192];
	static int index = 0;
	char *eol;
	int i;

	va_list ap;

	va_start(ap, fmt);
	index += vsnprintf(logbuf + index, sizeof(logbuf) - index, fmt, ap);
	va_end(ap);

	if (index >= sizeof(logbuf)) {
		index = sizeof(logbuf) - 1;
		logbuf[index] = '\0';
	}
	for (i = 0; eol = strchr(logbuf + i, '\n'); i = (eol - logbuf) + 1) {
		if (lfp == usesyslog)
			syslog(log_prio, "%.*s", eol - (logbuf+i), logbuf+i);
		else if (lfp != NULL) {
			fwrite(logbuf+i, 1, eol - (logbuf+i) + 1, lfp);
			fflush(lfp);
		} else {
			fwrite(logbuf+i, 1, eol - (logbuf+i) + 1, stdout);
			fflush(stdout);
		}
	}
	if (i >= index)
		index = 0;
	else if (i > 0) {
		memcpy(logbuf, logbuf + i, index - i);
		index -= i;
	}
}

void
sighup(int s)
{
	if (lfp == usesyslog)
		return;
	if (lfp != stdout) {
		if (lfp)
			fclose(lfp);
		if ((lfp = fopen(logfile, "a")) == NULL)
			err(1, "%s", logfile);
	}
}

void
sigterm(int s)
{
	if (lfp && lfp != usesyslog)
		fclose(lfp);
	exit(0);
}

main(int ac, char **av)
{
	int fd;
	FILE *fp, *pfp;
	struct sockaddr sa;
	unsigned char buf[16*1024];
	unsigned char *b, *bb;
	struct ip *ip;
	struct udphdr *udp;
	struct tcphdr *tcp;
	struct icmp *icmp;
	ipfw_hdr_t *ih;
	int n, nn, nb, ni, rcvbuf, kflag;
	int i, s;
	int code;
	int raw;
	ipfw_data_t id;
	int dmode, xflag, amod3;
	size_t sizes[16];

	id.mask = 0;
	id.code = 0;
	s = 0;
	nn = 0;
	raw = 0;
	rcvbuf = 0;
	fp = NULL;
	lfp = stdout;
	dmode = 0;
	kflag = 0;
	xflag = 0;

	while ((n = getopt(ac, av, "b:cdfIikL:l:m:nOoRr:x")) != EOF) {
		switch (n) {
		case 'd':
			dmode = 1;
			break;

		case 'b':
			id.code |= (strtol(optarg, 0, 0) & 0xff) << 16;
			id.mask |= id.code;
			break;

		case 'c':
			cflag++;
			break;

		case 'k':
			kflag = 1;
			break;
		case 'L':
			cracksyslog(optarg);
			openlog(NULL, LOG_PID, log_facility);
			lfp = usesyslog;
			break;

		case 'l':
			if (lfp && lfp != stdout)
				errx(1, "-l and -L are mutually exclusive");
			logfile = optarg;
			lfp = NULL;
			signal(SIGHUP, sighup);
			break;

		case 'm':
			id.mask |= (strtol(optarg, 0, 0) & 0xff) << 16;
			break;

		case 'n':
			nflag = 1;
			break;

		case 'R':
			raw = 1;
			break;

		case 'r':
			rcvbuf = strtol(optarg, 0, 0);
			break;

		case 'x':
			xflag = 1;
			break;

		case 'f':
			id.code &= ~IPFW_FILTER;
			id.code |= IPFW_FORWARD;
			id.mask |= IPFW_FILTER;
			break;
		case 'i':
			id.code &= ~IPFW_FILTER;
			id.code |= IPFW_INPUT;
			id.mask |= IPFW_FILTER;
			break;
		case 'o':
			id.code &= ~IPFW_FILTER;
			id.code |= IPFW_OUTPUT;
			id.mask |= IPFW_FILTER;
			break;
		case 'I':
			id.code &= ~IPFW_FILTER;
			id.code |= IPFW_PREINPUT;
			id.mask |= IPFW_FILTER;
			break;
		case 'O':
			id.code &= ~IPFW_FILTER;
			id.code |= IPFW_PREOUTPUT;
			id.mask |= IPFW_FILTER;
			break;
		default: usage:
			fprintf(stderr, "Usage: ipfwlog [-b bits] [-m mask] [-r rcvsize] [-cifoIO]\n");
			exit(1);
		}
	}

	if (lfp == usesyslog && logfile)
		errx(1, "-l and -L are mutually exclusive");
	if (lfp == usesyslog && raw)
		errx(1, "-R and -L are mutually exclusive");
	if (lfp == usesyslog && kflag)
		errx(1, "-k and -L are mutually exclusive");

	switch (ac - optind) {
	case 0:
		if (kflag)
			break;

		if ((fd = socket(PF_INET, SOCK_RAW, IPFW_PROTO)) < 0)
			err(1, "socket(PF_INET, SOCK_RAW, 0)");

		if (rcvbuf && setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf,
		    sizeof(n)) < 0)
			err(1, "setsockopt(RCVBUF)");

		if (setsockopt(fd, IPFW_PROTO, IPFW_SELECT, &id, sizeof(id))< 0)
			err(1, "setsockopt");

		memset(&sa, 0, sizeof(sa));
		sa.sa_len = 2;
		sa.sa_family = AF_INET;

		if (bind(fd, &sa, 2))
			err(1, "bind");
		break;
	case 1:
		if ((fp = fopen(av[optind], "r")) == NULL)
			err(1, "%s", av[optind]);
		break;
	default:
		goto usage;
	}

	if (kflag && (dmode || lfp == NULL))
		goto usage;

	if (lfp == NULL)
		sighup(1);

	if (kflag) {
		if ((pfp = fopen(_PATH_PID, "r")) != NULL) {
			if (fscanf(pfp, "%d", &n) == 1)
				kill(n, SIGHUP); 
			fclose(pfp);
		}
		if (fp == NULL)
			exit(0);
	}

	if (dmode) {
		if (lfp == stdout)
			errx(1, "-d requires a log file be specified (-l) or syslog be used (-L)");

		daemon(0, 0);
		unlink(_PATH_PID);
		pfp = fopen(_PATH_PID, "w");
		fprintf(pfp, "%d\n", getpid());
		fclose(pfp);
	}

	signal(SIGTERM, sigterm);
	signal(SIGINT, sigterm);

	while (fflush(stdout),
	    (nn = (fp ? fread(buf, 1, sizeof(ipfw_hdr_t), fp)
#ifdef	USE_RECVMFROM
		      : recvmfrom(fd, buf, sizeof(buf), 0, 0, 0, 0))) > 0) {
#else
		      : read(fd, buf, sizeof(buf)))) > 0) {
#endif
		struct tm *t;

		bb = buf;

		while (nn > 0) {
			if (nn < sizeof(ipfw_hdr_t)) {
				if (!raw) {
					log("Short packet (%d):", nn);
					for (i = 0; i < nn; ++i)
						log(" %02x", bb[i]);
					log("\n");
					if (fp)
						errx(1, "out of sync.  exiting.");
				}
				nn = 0;
				continue;
			}
			ih = (ipfw_hdr_t *)bb;
			if (ih->version != IPFW_VER_1_0) {
				if (!raw) {
					log("Unknown packet version ($d):",
					    nn);
					for (i = 0; i < sizeof(ipfw_hdr_t); ++i)
						log(" %02x", bb[i]);
					log("\n");
					if (fp)
						errx(1, "out of sync.  exiting.");
				}
				nn = 0;
				continue;
			}
			if (fp)
				/*
				 * Read the rest of the packet from the
				 * raw file.
				 */
				nn += fread(buf + sizeof(ipfw_hdr_t), 1,
					ih->length - sizeof(ipfw_hdr_t), fp);
			if (raw) {
				if (nn < ih->hdrlen) {
					nn = 0;
					continue;
				}
				if (nn < ih->length)
					ih->length = nn;
				fwrite(bb, ih->length, 1, lfp);
				bb += ih->length;
				nn += ih->length;
				continue;
			}
			b = bb + ih->hdrlen;

			code = ih->code & ~IPFW_SIZE;
			if (ih->length < nn)
				n = ih->length;
			else
				n = nn;
			bb += n;
			nn -= n;

			if ((code & IPFW_CONTROL) && cflag == 0)
				continue;

			t = localtime(&ih->when.tv_sec);
			log("%02d/%02d/%02d %02d:%02d:%02d ",
			    t->tm_year % 100, t->tm_mon+1, t->tm_mday,
			    t->tm_hour, t->tm_min, t->tm_sec);

			if (code & IPFW_ACCEPT)
				log(" ");
			else if (code & IPFW_CONTROL)
				log("c");
			else
				log("!");

			switch (code & (IPFW_FORWARD|IPFW_INPUT|IPFW_OUTPUT|IPFW_PREOUTPUT|IPFW_PREINPUT)) {
			case IPFW_FORWARD:	log("f"); break;
			case IPFW_PREINPUT:	log("I"); break;
			case IPFW_INPUT:	log("i"); break;
			case IPFW_PREOUTPUT:	log("O"); break;
			case IPFW_OUTPUT:	log("o"); break;
			default:		log("?"); break;
			}
#if 1
			log(" %02x", (code >> 16) & 0xff);
#endif
			switch (*b >> 4) {
			case 4:
				ip_print(b, n);
				break;
			case 6:
				ip6_print(b, n);
				break;
			case 0:
				if (code & IPFW_CONTROL)
					ipfw_print(b, n);
				break;
			default:
				log("IPv%d", (*b >> 4));
				break;
			}
			if (xflag) {
				char sbuf[18];

				sbuf[0] = 0;
				for (i = 0; i < n; ++i) {
					if ((i & 0xf) == 0) {
						if (sbuf[0])
							log("  %s", sbuf);
						memset(sbuf, 0, sizeof(sbuf));
						sbuf[0] = isprint(*b) ? *b : '.';
						log("\n\t%02x", *b++);
					} else if ((i & 0xf) == 8) {
						sbuf[8] = ' ';
						sbuf[9] = isprint(*b) ? *b : '.';
						log("  %02x", *b++);
					} else if (i & 0x8) {
						sbuf[(i & 0xf) + 1] = isprint(*b) ? *b : '.';
						log(" %02x", *b++);
					} else {
						sbuf[i & 0xf] = isprint(*b) ? *b : '.';
						log(" %02x", *b++);
					}
				}
				if (sbuf[0]) {
					i &= 0xf;
					if (i < 8) {
						++i;
						log(" ");
					}
					while (i++ < 0x10)
						log("   ");
					log("  %s", sbuf);
				}
			}
			log("\n");
		}
	}
}
