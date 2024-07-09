/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_proxy.c,v 1.7 2003/07/08 21:53:18 polk Exp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/termios.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libdialer.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dial.h"
#include "pathnames.h"

int Dial(int, int *);
void w(int, char *);
void sf(char *, ...);
void f(char *, ...);
char * printable(char *str);
int expect(int, char *, int);
void getsyscap(char *, char *);
static void initialmodes(int, struct termios *, char *);

#define	STRSIZE	128

dial_t dial;
int debug;

main(int ac, char **av)
{
	char buf[8192];
	char *b, *fail;
	int dfd, fd;
	int c;
	char *sysfile = _PATH_SYS;
	cap_t *cap;
	cap_t *cp;
	int sv[2];
	struct termios t;

	setbuf(stdout, NULL);

	while ((c = getopt(ac, av, "s:x")) != EOF)
		switch (c) {
		case 's':
			sysfile = optarg;
			break;
		case 'x':
			debug = 1;
			break;
		usage:
		default:
			f("Usage: login_proxy [-x] [-s sysfile] system\n");
		}

	if (getuid())
		debug = 0;

	if (optind + 1 != ac)
		goto usage;

	getsyscap(av[optind], sysfile);
	for (c = 0; c < 10; ++c) {
		if (dial.S[c] && strlen(dial.S[c]) >= STRSIZE)
			f("send string too long");
		if (dial.E[c] && strlen(dial.E[c]) >= STRSIZE)
			f("expect string too long");
		if (dial.F[c] && strlen(dial.F[c]) >= STRSIZE)
			f("failure string too long");
	}

	/*
	 * Suck up request from upstream
	 */
	b = buf;
	while (b < buf + sizeof(buf)-1 && read(0, b, 1) == 1)
		if (*b++ == '\n')
			break;

	if (b == buf || strncmp(buf, "request:", 8) ||
	    b[-1] != '\n' || b[-2] != ':')
		f("incomplete request");
	b[-1] = '\0';

	if ((cap = cap_make(buf, (cap_t *)_EMPTYCAP)) == NULL)
		f("invalid request");

	cp = cap_look(cap, "direct");

	if (cp) {
		dfd = 0;
		cp = cap_look(cap, RESOURCE_LINE);
		if (cp == NULL)
			f("no device specified");
		if ((fd = open((b = cp->value), O_RDWR | O_EXCL)) < 0)
			sf(cp->value);
		initialmodes(fd, &t, cp->value);
		cp = cap_look(cap, "speed");
		if (cp)
			cfsetspeed(&t, strtol(cp->value, 0, 0));
		cp = cap_look(cap, "stty-modes");
		if (cp && (cfsetterm(&t, cp->value, &fail) != 0)) {
			sprintf(buf, "cfsetterm: %s: bad value:", fail);
			f(buf);
		}
		if (tcsetattr(fd, TCSANOW, &t) < 0)
			sf(b);
	} else {
		/*
		 * First connect to a daemon (or another proxy)
		 * Wait for the daemon to dial
		 */
		dfd = connect_daemon(cap, (uid_t)-1, (gid_t)-1);	

		if ((fd = Dial(0, &dfd)) < 0)
			exit(1);
		initialmodes(fd, &t, "modem");
		if (tcsetattr(fd, TCSANOW, &t) < 0)
			sf(b);
	}

	for (c = 0; c < 10; ++c) {
		int t = dial.F[c] ? 1 : 0;
		if (dial.S[c]) {
			if (debug)
				printf("Send: <%s>\n", printable(dial.S[c]));
			w(fd, dial.S[c]);
		}
		while (dial.E[c]) {
			if (expect(fd, dial.E[c], dial.T[c]) == 0)
				break;
			if (t-- <= 0)
				f("expected: ``%s''", printable(dial.E[c]));
			if (dial.F[c]) {
				w(fd, dial.F[c]);
				if (debug)
					printf("Send F: <%s>\n",
					    printable(dial.F[c]));
			}
		}
	}

	/*
	 * Forward up the file descriptors and get out of the way
	 */
	write(0, "daemon\n", 7);
	sv[0] = fd;
	sv[1] = dfd;
	sendfds(0, sv, 2);
	exit(0);
}

static expect_interrupted;

/* Timeout signal handler */
static void
expalarm()
{
	expect_interrupted = 1;
}

static void
initialmodes(fd, t, name )
	int fd;
	struct termios *t;
	char *name;
{
	if (tcgetattr(fd, t) < 0)
		sf(name);
	t->c_iflag &= IXON | IXOFF;
	t->c_iflag |= IGNBRK | IGNPAR;
	t->c_oflag = 0;
	t->c_cflag &= CCTS_OFLOW | CRTS_IFLOW | NOCLOCAL;
	t->c_cflag |= CLOCAL | CS8 | CREAD | HUPCL;
	t->c_lflag = 0;
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 0;
}

/*
 * Look up for an expected string or until the timeout is expired
 * Returns 0 if successfull.
 */
int
expect(fd, str, timo)
	int fd;
	char *str;
	int timo;
{
	struct sigaction act, oact;
	char buf[STRSIZE];
	int nchars, expchars;
	char c;

	if (timo < 0)
		timo = 15;
	if (debug)
		printf("Expect <%s> (%d sec)\nGot: <", printable(str), timo);
	nchars = 0;
	expchars = strlen(str);
	expect_interrupted = 0;
	act.sa_handler = expalarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oact);
	alarm(timo);

	do {
		if (read(fd, &c, 1) != 1 || expect_interrupted) {
			alarm(0);
			sigaction(SIGALRM, &oact, 0);
			if (debug)
				printf("> FAILED\n");
			return (1);
		}
		c &= 0177;
		if (debug) {
			if (c < 040)
				printf("^%c", c | 0100);
			else if (c == 0177)
				printf("^?");
			else
				printf("%c", c);
		}
		buf[nchars++] = c;
		if (nchars > expchars)
			bcopy(buf+1, buf, --nchars);
	} while (nchars < expchars || bcmp(buf, str, expchars));
	alarm(0);
	sigaction(SIGALRM, &oact, 0);
	if (debug)
		printf("> OK\n");
	return (0);
}

int
Dial(int up, int *dfdp)
{
	char buf[8192];
	char *b;
	int sv[2];
	int n;

	for (;;) {
		b = buf;
		while (b < buf + sizeof(buf)-1 && read(*dfdp, b, 1) == 1)
			if (*b++ == '\n')
				break;
		if (b == buf)
			return(-1);
		if (strncmp(buf, "fd\n", b - buf) == 0) {
			return(recvfd(*dfdp));
		} else if (strncmp(buf, "daemon\n", b - buf) == 0) {
			n = 2;
			if (recvfds(*dfdp, sv, &n) < 0)
				return(-1);
			close(*dfdp);
			*dfdp = sv[1];
			return(sv[0]);
		} else {
			write(up, buf, b - buf);
		}
	}
}

void
w(int fd, char *str)
{
	write(fd, str, strlen(str));
}

void
f(char *fmt, ...)
{
	va_list ap;
	FILE *fp;

	fp = fdopen(0, "w");

	fprintf(fp, "error:");
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fprintf(fp, "\n");
	exit(1);
}

void
sf(char *fmt, ...)
{
	va_list ap;
	FILE *fp;
	int e;

	e = errno;
	fp = fdopen(0, "w");

	fprintf(fp, "error:");
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fprintf(fp, ":%s\n", strerror(e));
	exit(1);
}

/*
 * Make string suitable to print out
 */
char *
printable(str)
	char *str;
{
	static char buf[STRSIZE*4+1];
	char *p;
	int c;

	p = buf;

	while (c = *str++) {
		if (p >= buf + sizeof(buf) - 5)
			break;
		c &= 0377;
		if (c < 040) {
			*p++ = '^';
			*p++ = c | 0100;
		} else if (c == 0177) {
			*p++ = '^';
			*p++ = '?';
		} else if (c & 0200) {
			*p++ = '\\';
			*p++ = '0' + (c >> 6);
			*p++ = '0' + ((c >> 3) & 07);
			*p++ = '0' + (c & 07);
		} else
			*p++ = c;
	}
	*p = 0;
	return (buf);
}
