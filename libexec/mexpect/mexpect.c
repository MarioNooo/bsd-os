/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI mexpect.c,v 1.3 1997/09/11 16:27:56 prb Exp
 */
#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void output(unsigned char *, int);
void sendit(char *, char *);
void timeout();

#define	MAX_STR_SIZE	1024

int debug = 0;
int send = 0;
int cnt = 0;
int wfd = 1;
int rfd = 0;
int fbaud = 0;
char *rstring = 0;
struct termios tio;

jmp_buf again;

struct sigaction sa = { timeout, };

void
main(int ac, char **av)
{
	extern char *optarg;
	extern int optind;
	extern char *__progname;

	/*
	 * The static items here are to keep -Wall happy.
	 * This would be bad if you recursed on main(),
	 * but doing that would be bad anyhow.
	 */
	static regex_t *pats;
	regmatch_t pmatch;
	unsigned char *start, *stop, buf[MAX_STR_SIZE * 2];
	char *pattern;
	static char *sendbuf;
	char c;
	int e, flag, i, match, sendlen;
	static int zflag;
	static long timo;

	sendbuf = NULL;
	timo = 0;
	zflag = 0;
	pats = NULL;	/* For -Wall */
	sigaction(SIGALRM, &sa, 0);

	while ((c = getopt(ac, av, "b:c:df:r:st:z")) != EOF) {
		switch (c) {
		case 'b':
			fbaud = strtol(optarg, 0, 0);
			break;
		case 'c':
			cnt = strtol(optarg, 0, 0);
			break;
		case 'd':
			debug = 1;
			break;
		case 'f':
			if ((wfd = rfd = open(optarg, O_RDWR)) < 0)
				err(1, optarg);
			break;
		case 'r':
			rstring = optarg;
			break;
		case 's':
			send = 1;
			break;
		case 't':
			timo = strtol(optarg, 0, 0);
			break;
		case 'z':
			zflag = 1;
			break;
		usage:
		default:
			fprintf(stderr, "Usage: %s [-s] [-d] [-z] "
					"[-b baud] "
					"[-c cnt] [-r resend] "
					"[-f device] "
					"[-t timeout] send "
					"[expect ...]\n", __progname);
			exit(1);
		}
	}

	ac -= optind;
    	av += optind;

	if (ac < 1)
		goto usage;

	pattern = *av++;
	--ac;

	if (ac > 0) {
		if ((pats = (regex_t *)malloc(sizeof(regex_t) * ac)) == 0)
			err(1, "allocating memory");
		for (i = 0; i < ac; ++i)
			if (e = regcomp(pats+i, av[i], REG_BASIC)) {
				regerror(e, pats+i, (char *)buf, sizeof(buf));
				errx(1, "%s: %s", av[i], buf);
			}
		
	}
	if (tcgetattr(wfd, &tio) < 0)
		cfsetspeed(&tio, 115200);

	if (ac > 0) {
		sendbuf = strdup(pattern);

		if (!sendbuf)
			err(1, "unable to allocate memory");
	}

	setjmp(again);

	if (timo)
		alarm(timo);
	sendit(pattern, sendbuf);

    	if (ac < 1) {
		if (send) {
			printf("MINDEX=0\n");
			printf("MSTRING=''\n");
		}
		exit(0);
	}

	sendlen = strlen(sendbuf);

	if (debug) {
		fprintf(stderr, "Read: `");
		++debug;
	}

    	start = stop = buf;

    	match = zflag;

	for(;;) {
		if (read(rfd, stop, 1) != 1) {
			if (debug)
				fprintf(stderr, "'\n");
			exit(0);
		}
		if (debug)
			if (*stop < ' ' || *stop > '~')
				switch (*stop) {
				case '\r':
					fprintf(stderr, "\\r");
					break;
				case '\n':
					fprintf(stderr, "\\n");
					break;
				default:
					fprintf(stderr, "\\%03o", *stop);
					break;
				}
			else if (*stop == '\\')
				fprintf(stderr, "\\\\");
			else
				fputc(*stop, stderr);

		/*
		 * Ignore the echoing of the string we sent.
		 */
		if (match) {
			*++stop = 0;
			if (sendlen <= stop - start &&
			    strncmp(sendbuf, (char *)stop-sendlen, sendlen) == 0) {
				if (debug)
					fprintf(stderr, "' TOSSED\nRead: `");
				match = 0;
				start = stop = buf;
			}
		} else {
			if (*stop == '\n' || *stop == '\r') {
				flag = REG_NOTEOL;
				flag = 0;
				*stop = 0;
				stop = start;
			} else {
				flag = REG_NOTEOL;
				*++stop = 0;
			}
			for (i = 0; i < ac; ++i) {
				if (!(e = regexec(pats+i, (char *)start, 1,
				    &pmatch, flag))) {
					if (debug)
						fprintf(stderr,
						    "' MATCHED (%d)\n", i);
					if (send) {
						printf("MINDEX=%d\n", i);
						printf("MSTRING=");
						output(start + pmatch.rm_so,
						   (pmatch.rm_eo-pmatch.rm_so));
						printf("\n");
					}
					exit(0);
				}
			}
		}
		if (stop < start)
			start = stop;

		if (stop >= buf + MAX_STR_SIZE)
			++start;
		if (start >= buf + MAX_STR_SIZE) {
			memcpy(buf, start, stop - start);
			stop = buf + (stop - start);
			start = buf;
		}
	}
}

void
sendit(char *p, char *start)
{
	struct timeval tv;

	if (debug)
		fprintf(stderr, "Send: ``%s''\n", p);

	tv.tv_sec = 0;

	while (*p) {
		if (fbaud < cfgetospeed(&tio) && fbaud != 0) {
			/*
			 * We are faking a baud rate.  Delay the
			 * difference in time for sending a single
			 * character.
			 */
			tv.tv_usec = ((1000000 / fbaud) -
			    (1000000 / cfgetospeed(&tio))) * 10;
			tv.tv_sec = 0;
			while (tv.tv_usec > 1000000) {
				tv.tv_sec++;
				tv.tv_usec -= 1000000;
			}
			select(0, 0, 0, 0, &tv);
		}
		switch (*p) {
		case '\\':
			switch (*++p) {
			case 'n':
				if (start)
					*start++ = '\n';
				write(wfd, "\n", 1);
				break;
			case 'r':
				if (start)
					*start++ = '\r';
				write(wfd, "\r", 1);
				break;
			case '#':
				tcsendbreak(wfd, 0);
				break;
			default:
				if (start)
					*start++ = p[0];
				write(wfd, p, 1);
				break;
			}
			break;
		default:
			if (start)
				*start++ = p[0];
			write(wfd, p, 1);
			break;
		}
		++p;
	}
    	if (start)
		*start = 0;
}

void
timeout()
{
	if (debug > 1)
		fprintf(stderr, "' TIMEOUT\n");

	if (--cnt > 0) {
		if (rstring)
			sendit(rstring, 0);
		longjmp(again, 1);
	}
	if (send) {
		printf("MINDEX=-1\n");
		printf("MSTRING='TIMEOUT'\n");
	}
	exit(2);
}

void
output(unsigned char *s, int n)
{
	putchar('\'');
    	while (n-- && *s) {
		switch (*s) {
		case '\'':
			printf("'\"'\"'");
			break;
		default:
			putchar(*s);
		}
		++s;
	}
	putchar('\'');
}
