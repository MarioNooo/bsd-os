/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)tftp.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/* Many bug fixes are from Jim Guyton <guyton@rand-unix> */

/*
 * TFTP User Program -- Protocol Machines
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "extern.h"
#include "tftpsubs.h"

#define SA(addr) ((struct sockaddr *)(addr))

extern	int errno;

extern  struct sockaddr_in6 peeraddr;	/* filled in by set_peeraddr() */
extern  int     f;			/* the opened socket */
extern  int     trace;
extern  int     verbose;
extern  int     silent;
extern  int     justcheck;
extern  int     rexmtval;
extern  int     maxtimeout;

#define PKTSIZE    SEGSIZE+4
char    ackbuf[PKTSIZE];
volatile unsigned long amount;
char	*direction;
int	timeout;
jmp_buf	toplevel;
jmp_buf	timeoutbuf;

static void info __P((int));
static void nak __P((int));
static int makerequest __P((int, const char *, struct tftphdr *, const char *));
static void printstats __P((const char *, unsigned long));
static void startclock __P((void));
static void stopclock __P((void));
static void timer __P((int));
static void tpacket __P((const char *, struct tftphdr *, int));

/*
 * Send the requested file.
 */
void
sendfile(fd, name, mode)
	int fd;
	char *name;
	char *mode;
{
	register struct tftphdr *ap;	   /* data and ack packets */
	struct tftphdr *r_init(), *dp;
	register int n;
	volatile u_short block;
	volatile int size, convert;
	struct sockaddr_in6 from;
	int firsttrip;
	int fromlen;
	FILE *file;

	startclock();		/* start stat's clock */
	dp = r_init();		/* reset fillbuf/read-ahead code */
	ap = (struct tftphdr *)ackbuf;
	file = fdopen(fd, "r");
	convert = !strcmp(mode, "netascii");
	block = 0;
	firsttrip = 1;
	amount = 0;
	direction = "Sent";

	signal(SIGALRM, timer);
	signal(SIGINFO, info);
	do {
		if (firsttrip) {
			size = makerequest(WRQ, name, dp, mode) - 4;
			firsttrip = 0;
		} else {
		/*	size = read(fd, dp->th_data, SEGSIZE);	 */
			size = readit(file, &dp, convert);
			if (size < 0) {
				nak(errno + 100);
				break;
			}
			dp->th_opcode = htons((u_short)DATA);
			dp->th_block = htons((u_short)block);
		}
		timeout = 0;
		(void) setjmp(timeoutbuf);
send_data:
		if (trace)
			tpacket("sent", dp, size + 4);
		n = sendto(f, dp, size + 4, 0,
		    SA(&peeraddr), SA(&peeraddr)->sa_len);
		if (n != size + 4) {
			perror("tftp: sendto");
			goto abort;
		}
		read_ahead(file, convert);
		for ( ; ; ) {
			alarm(rexmtval);
			do {
				fromlen = sizeof(from);
				n = recvfrom(f, ackbuf, sizeof(ackbuf), 0,
				    SA(&from), &fromlen);
			} while (n <= 0);
			alarm(0);
			if (n < 0) {
				perror("tftp: recvfrom");
				goto abort;
			}
			peeraddr.sin6_port = from.sin6_port;	/* added */
			if (trace)
				tpacket("received", ap, n);
			/* should verify packet came from server */
			ap->th_opcode = ntohs(ap->th_opcode);
			ap->th_block = ntohs(ap->th_block);
			if (ap->th_opcode == ERROR) {
				fprintf(stderr, "Error code %d: %s\n", ap->th_code,
					ap->th_msg);
				goto abort;
			}
			if (ap->th_opcode == ACK) {
				int j;

				if (ap->th_block == block) {
					break;
				}
				/* On an error, try to synchronize
				 * both sides.
				 */
				j = synchnet(f);
				if (j && trace) {
					fprintf(stderr, "discarded %d packets\n",
							j);
				}
				if (ap->th_block == (block-1)) {
					goto send_data;
				}
			}
		}
		if (block > 0)
			amount += size;
		block++;
	} while (size == SEGSIZE || block == 1);
abort:
	fclose(file);
	stopclock();
	if (amount > 0)
		printstats(direction, amount);
	signal(SIGINFO, SIG_IGN);
}

/*
 * Receive a file.
 */
int
recvfile(fd, name, mode)
	int fd;
	char *name;
	char *mode;
{
	register struct tftphdr *ap;
	struct tftphdr *dp, *w_init();
	register int n;
	volatile u_short block;
	volatile int size, firsttrip;
	struct sockaddr_in6 from;
	int fromlen;
	FILE *file;
	volatile int convert;		/* true if converting crlf -> lf */
	int error = 0;

	startclock();
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	file = fdopen(fd, "w");
	convert = !strcmp(mode, "netascii");
	block = 1;
	firsttrip = 1;
	amount = 0;
	direction = "Received";

	signal(SIGALRM, timer);
	signal(SIGINFO, info);
	do {
		if (firsttrip) {
			size = makerequest(RRQ, name, ap, mode);
			firsttrip = 0;
		} else {
			ap->th_opcode = htons((u_short)ACK);
			ap->th_block = htons((u_short)(block));
			size = 4;
			block++;
		}
		timeout = 0;
		(void) setjmp(timeoutbuf);
send_ack:
		if (trace)
			tpacket("sent", ap, size);
		if (sendto(f, ackbuf, size, 0, SA(&peeraddr),
		    SA(&peeraddr)->sa_len) != size) {
			alarm(0);
			perror("tftp: sendto");
			error = 1;
			goto abort;
		}
		write_behind(file, convert);
		for ( ; ; ) {
			alarm(rexmtval);
			do  {
				fromlen = sizeof(from);
				n = recvfrom(f, dp, PKTSIZE, 0,
				    SA(&from), &fromlen);
			} while (n <= 0);
			alarm(0);
			if (n < 0) {
				perror("tftp: recvfrom");
				error = 1;
				goto abort;
			}
			peeraddr.sin6_port = from.sin6_port;	/* added */
			if (trace)
				tpacket("received", dp, n);
			/* should verify client address */
			dp->th_opcode = ntohs(dp->th_opcode);
			dp->th_block = ntohs(dp->th_block);
			if (dp->th_opcode == ERROR) {
				fprintf(stderr, "Error code %d: %s\n", dp->th_code,
					dp->th_msg);
				error = 1;
				goto abort;
			}
			if (dp->th_opcode == DATA) {
				int j;

				if (dp->th_block == block) {
					break;		/* have next packet */
				}
				/* On an error, try to synchronize
				 * both sides.
				 */
				error = 1;
				j = synchnet(f);
				if (j && trace) {
					fprintf(stderr, "discarded %d packets\n", j);
				}
				if (dp->th_block == (block-1)) {
					goto send_ack;	/* resend ack */
				}
			}
		}
	/*	size = write(fd, dp->th_data, n - 4); */
			/* If we got here, the file is readable */
		if (justcheck)
			exit(0);
		size = writeit(file, &dp, n - 4, convert);
		if (size < 0) {
			nak(errno + 100);
			break;
		}
		amount += size;
	} while (size == SEGSIZE);
abort:						/* ok to ack, since user */
	ap->th_opcode = htons((u_short)ACK);	/* has seen err msg */
	ap->th_block = htons((u_short)block);
	(void) sendto(f, ackbuf, 4, 0, SA(&peeraddr),
	    SA(&peeraddr)->sa_len);
	write_behind(file, convert);		/* flush last buffer */
	fclose(file);
	stopclock();
	if (amount > 0)
		printstats(direction, amount);
	signal(SIGINFO, SIG_IGN);
	return error;
}

static int
makerequest(request, name, tp, mode)
	int request;
	const char *name;
	struct tftphdr *tp;
	const char *mode;
{
	register char *cp;

	tp->th_opcode = htons((u_short)request);
	cp = tp->th_stuff;
	strcpy(cp, name);
	cp += strlen(name);
	*cp++ = '\0';
	strcpy(cp, mode);
	cp += strlen(mode);
	*cp++ = '\0';
	return (cp - (char *)tp);
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
static void
nak(error)
	int error;
{
	register struct errmsg *pe;
	register struct tftphdr *tp;
	int length;
	char *strerror();

	tp = (struct tftphdr *)ackbuf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->th_code = EUNDEF;
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg) + 4;
	if (trace)
		tpacket("sent", tp, length);
	if (sendto(f, ackbuf, length, 0, SA(&peeraddr),
	    SA(&peeraddr)->sa_len) != length)
		perror("nak");
}

static void
tpacket(s, tp, n)
	const char *s;
	struct tftphdr *tp;
	int n;
{
	static char *opcodes[] =
	   { "#0", "RRQ", "WRQ", "DATA", "ACK", "ERROR" };
	register char *cp, *file;
	u_short op = ntohs(tp->th_opcode);
	char *index();

	if (op < RRQ || op > ERROR)
		fprintf(stderr, "%s opcode=%x ", s, op);
	else
		fprintf(stderr, "%s %s ", s, opcodes[op]);
	switch (op) {

	case RRQ:
	case WRQ:
		n -= 2;
		file = cp = tp->th_stuff;
		cp = index(cp, '\0');
		fprintf(stderr, "<file=%s, mode=%s>\n", file, cp + 1);
		break;

	case DATA:
		fprintf(stderr, "<block=%d, %d bytes>\n", ntohs(tp->th_block), n - 4);
		break;

	case ACK:
		fprintf(stderr, "<block=%d>\n", ntohs(tp->th_block));
		break;

	case ERROR:
		fprintf(stderr, "<code=%d, msg=%s>\n", ntohs(tp->th_code), tp->th_msg);
		break;
	}
}

struct timeval tstart;
struct timeval tstop;

static void
startclock()
{

	(void)gettimeofday(&tstart, NULL);
}

static void
stopclock()
{

	(void)gettimeofday(&tstop, NULL);
}

static void
printstats(direction, amount)
	const char *direction;
	unsigned long amount;
{
	double delta;
	if (silent)
		return;
			/* compute delta in 1/10's second units */
	delta = ((tstop.tv_sec*10.)+(tstop.tv_usec/100000)) -
		((tstart.tv_sec*10.)+(tstart.tv_usec/100000));
	delta = delta/10.;      /* back to seconds */
	fprintf(stderr, "%s %d bytes in %.1f seconds", direction, amount, delta);
	if (verbose)
		fprintf(stderr, " [%.0f bits/sec]", (amount*8.)/delta);
	putc('\n', stderr);
}

static void
timer(sig)
	int sig;
{

	timeout += rexmtval;
	if (timeout >= maxtimeout) {
		fprintf(stderr, "Transfer timed out.\n");
		longjmp(toplevel, -1);
	}
	longjmp(timeoutbuf, 1);
}

static void
info(sig)
	int sig;
{
	stopclock();
	printstats(direction, amount);
}
