/*	BSDI kdump.c,v 2.7 2002/01/03 00:00:53 polk Exp	*/

/*-
 * Copyright (c) 1988, 1993
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
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)kdump.c	8.5 (Berkeley) 7/11/95";
#endif /* not lint */

#include <sys/param.h>
#define KERNEL
#include <sys/errno.h>
extern int errno;	/* not declared if KERNEL is defined */
#undef KERNEL
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "ktrace.h"

int timestamp, decimal, fancy = 1, tail, maxdata;
char *tracefile = DEF_TRACEFILE;
struct ktrace_header ktr_header;

#define eqs(s1, s2)	(strcmp((s1), (s2)) == 0)

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, ktrlen, size;
	register void *m;
	int trpoints = ALL_POINTS;

	while ((ch = getopt(argc, argv, "f:dlm:nRrTt:")) != -1)
		switch (ch) {
		case 'f':
			tracefile = optarg;
			break;
		case 'd':
			decimal = 1;
			break;
		case 'l':
			tail = 1;
			break;
		case 'm':
			maxdata = atoi(optarg);
			break;
		case 'n':
			fancy = 0;
			break;
		case 'r':
			timestamp = 3;	/* relative to start of trace */
			break;
		case 'R':
			timestamp = 2;	/* relative timestamp */
			break;
		case 'T':
			timestamp = 1;
			break;
		case 't':
			trpoints = getpoints(optarg);
			if (trpoints < 0)
				errx(1, "unknown trace point in %s", optarg);
			break;
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc > 1)
		usage();

	m = (void *)malloc(size = 1025);
	if (m == NULL)
		errx(1, "%s", strerror(ENOMEM));
	if (!freopen(tracefile, "r", stdin))
		err(1, "%s", tracefile);
	while (fread_tail(&ktr_header, sizeof(struct ktrace_header), 1)) {
		if (trpoints & (1<<ktr_header.h_type))
			dumpheader(&ktr_header);
		if ((ktrlen = ktr_header.h_len) < 0)
			errx(1, "bogus length 0x%x", ktrlen);
		if (ktrlen > size) {
			m = (void *)realloc(m, ktrlen+1);
			if (m == NULL)
				errx(1, "%s", strerror(ENOMEM));
			size = ktrlen;
		}
		if (ktrlen && fread_tail(m, ktrlen, 1) == 0)
			errx(1, "data too short");
		if ((trpoints & (1<<ktr_header.h_type)) == 0)
			continue;
		switch (ktr_header.h_type) {
		case KTRACE_SYSCALL:
			ktrsyscall((struct ktrace_syscall *)m);
			break;
		case KTRACE_SYSRET:
			ktrsysret((struct ktrace_sysret *)m);
			break;
		case KTRACE_NAMEI:
			ktrnamei(m, ktrlen);
			break;
		case KTRACE_GENIO:
			ktrgenio((struct ktrace_genio *)m, ktrlen);
			break;
		case KTRACE_PSIG:
			ktrpsig((struct ktrace_psig *)m);
			break;
		case KTRACE_CSW:
			ktrcsw((struct ktrace_csw *)m);
			break;
		}
		if (tail)
			(void)fflush(stdout);
	}
}

fread_tail(buf, size, num)
	char *buf;
	int num, size;
{
	int i;

	while ((i = fread(buf, size, num, stdin)) == 0 && tail) {
		(void)sleep(1);
		clearerr(stdin);
	}
	return (i);
}

dumpheader(kth)
	struct ktrace_header *kth;
{
	static char unknown[64];
	static struct timeval prevtime, temp;
	static int gotstarttime;
	char *type;

	switch (kth->h_type) {
	case KTRACE_SYSCALL:
		type = "CALL";
		break;
	case KTRACE_SYSRET:
		type = "RET ";
		break;
	case KTRACE_NAMEI:
		type = "NAMI";
		break;
	case KTRACE_GENIO:
		type = "GIO ";
		break;
	case KTRACE_PSIG:
		type = "PSIG";
		break;
	case KTRACE_CSW:
		type = "CSW";
		break;
	default:
		(void)sprintf(unknown, "UNKNOWN(%d)", kth->h_type);
		type = unknown;
	}

	(void)printf("%6d %-8s ", kth->h_pid, kth->h_comm);
	if (timestamp) {
		if (timestamp == 3) {
			temp = kth->h_time;
			if (!gotstarttime) {
				prevtime = temp;
				gotstarttime = 1;
			}
			timevalsub(&kth->h_time, &prevtime);
		}
		if (timestamp == 2) {
			temp = kth->h_time;
			timevalsub(&kth->h_time, &prevtime);
			prevtime = temp;
		}
		(void)printf("%ld.%06ld ",
		    kth->h_time.tv_sec, kth->h_time.tv_usec);
	}
	(void)printf("%s  ", type);
}

#include <sys/syscall.h>
#define KTRACE
#include <sys/syscalls.c>
#undef KTRACE
int nsyscalls = sizeof (syscallnames) / sizeof (syscallnames[0]);

static char *ptrace_ops[] = {
	"PT_TRACE_ME",	"PT_READ_I",	"PT_READ_D",	"PT_READ_U",
	"PT_WRITE_I",	"PT_WRITE_D",	"PT_WRITE_U",	"PT_CONTINUE",
	"PT_KILL",	"PT_STEP",
};

ktrsyscall(ktr)
	register struct ktrace_syscall *ktr;
{
	register argsize = ktr->ks_argsize;
	register register_t *ap;
	char *ioctlname();

	if (ktr->ks_code >= nsyscalls || ktr->ks_code < 0)
		(void)printf("[%d]", ktr->ks_code);
	else
		(void)printf("%s", syscallnames[ktr->ks_code]);
	ap = (register_t *)((char *)ktr + sizeof(struct ktrace_syscall));
	if (argsize) {
		char c = '(';
		if (fancy) {
			switch (ktr->ks_code) {
				char *cp;

			case SYS_ioctl:
				if (decimal)
					(void)printf("(%ld", (long)*ap);
				else
					(void)printf("(%#lx", (long)*ap);
				ap++;
				argsize -= sizeof(register_t);
				if ((cp = ioctlname(*ap)) != NULL)
					(void)printf(",%s", cp);
				else {
					if (decimal)
						(void)printf(",%ld",
						    (long)*ap);
					else
						(void)printf(",%#lx ",
						    (long)*ap);
				}
				c = ',';
				ap++;
				argsize -= sizeof(register_t);
				break;

			case SYS_ptrace:
				if (*ap >= 0 && *ap <=
				    sizeof(ptrace_ops) / sizeof(ptrace_ops[0]))
					(void)printf("(%s", ptrace_ops[*ap]);
				else
					(void)printf("(%ld", (long)*ap);
				c = ',';
				ap++;
				argsize -= sizeof(register_t);
				break;

			case SYS_sigaction:
				printf("%c", c);
				ktrpsigname((long)*ap);
				c = ',';
				ap++;
				argsize -= sizeof(register_t);
				break;
			}
		}
		while (argsize) {
			if (decimal)
				(void)printf("%c%ld", c, (long)*ap);
			else
				(void)printf("%c%#lx", c, (long)*ap);
			c = ',';
			ap++;
			argsize -= sizeof(register_t);
		}
		(void)putchar(')');
	}
	(void)putchar('\n');
}

ktrsysret(ktr)
	struct ktrace_sysret *ktr;
{
	register int ret = ktr->ksr_retval;
	register int error = ktr->ksr_error;
	register int code = ktr->ksr_code;

	if (code >= nsyscalls || code < 0)
		(void)printf("[%d] ", code);
	else
		(void)printf("%s ", syscallnames[code]);

	if (error == 0) {
		if (fancy) {
			(void)printf("%d", ret);
			if (ret < 0 || ret > 9)
				(void)printf("/%#x", ret);
		} else {
			if (decimal)
				(void)printf("%d", ret);
			else
				(void)printf("%#x", ret);
		}
	} else if (error == ERESTART)
		(void)printf("RESTART");
	else if (error == EJUSTRETURN)
		(void)printf("JUSTRETURN");
	else {
		(void)printf("-1 errno %d", ktr->ksr_error);
		if (fancy)
			(void)printf(" %s", strerror(ktr->ksr_error));
	}
	(void)putchar('\n');
}

ktrnamei(cp, len) 
	char *cp;
{
	(void)printf("\"%.*s\"\n", len, cp);
}

ktrgenio(ktr, len)
	struct ktrace_genio *ktr;
{
	register int datalen = len - sizeof (struct ktrace_genio);
	register char *dp = (char *)ktr + sizeof (struct ktrace_genio);
	register char *cp;
	register int col = 0;
	register width;
	char visbuf[5];
	static screenwidth = 0;

	if (screenwidth == 0) {
		struct winsize ws;

		if (fancy && ioctl(fileno(stderr), TIOCGWINSZ, &ws) != -1 &&
		    ws.ws_col > 8)
			screenwidth = ws.ws_col;
		else
			screenwidth = 80;
	}
	printf("fd %d %s %d bytes\n", ktr->kg_fd,
		ktr->kg_rw == UIO_READ ? "read" : "wrote", datalen);
	if (maxdata && datalen > maxdata)
		datalen = maxdata;
	(void)printf("       \"");
	col = 8;
	for (; datalen > 0; datalen--, dp++) {
		(void) vis(visbuf, *dp, VIS_CSTYLE, *(dp+1));
		cp = visbuf;
		/*
		 * Keep track of printables and
		 * space chars (like fold(1)).
		 */
		if (col == 0) {
			(void)putchar('\t');
			col = 8;
		}
		switch(*cp) {
		case '\n':
			col = 0;
			(void)putchar('\n');
			continue;
		case '\t':
			width = 8 - (col&07);
			break;
		default:
			width = strlen(cp);
		}
		if (col + width > (screenwidth-2)) {
			(void)printf("\\\n\t");
			col = 8;
		}
		col += width;
		do {
			(void)putchar(*cp++);
		} while (*cp);
	}
	if (col == 0)
		(void)printf("       ");
	(void)printf("\"\n");
}

ktrpsigname(signo)
	u_int signo;
{
	if (signo < NSIG)
		(void)printf("SIG%s", sys_signame[signo]);
	else
		(void)printf("SIG%d", signo);
}

ktrpsig(psig)
	struct ktrace_psig *psig;
{
	ktrpsigname(psig->kp_signo);
	if (psig->kp_action == SIG_DFL)
		(void)printf(" SIG_DFL\n");
	else
		(void)printf(" caught handler=0x%lx mask=0x%x code=0x%x\n",
		    (u_long)psig->kp_action, psig->kp_mask, psig->kp_code);
}

ktrcsw(cs)
	struct ktrace_csw *cs;
{
	(void)printf("%s %s\n", cs->csw_out ? "stop" : "resume",
	    cs->csw_user ? "user" : "kernel");
}

usage()
{
	(void)fprintf(stderr,
	    "usage: kdump [-dnlRT] [-f trfile] [-m maxdata] [-t [cnis]]\n");
	exit(1);
}
