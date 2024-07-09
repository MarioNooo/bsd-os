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
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/* Many bug fixes are from Jim Guyton <guyton@rand-unix> */

/*
 * TFTP User Program -- Command Interface.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define	TIMEOUT		5		/* secs between rexmt's */

struct	sockaddr_in6 peeraddr;
int	f = -1;
short   port;
int	trace;
int	verbose;
int	silent;
int	justcheck;
int	connected;
char	mode[32];
char	line[200];
int	margc;
char	*margv[20];
char	*prompt = "tftp";
jmp_buf	toplevel;
void	intr();

void	get __P((int, char **));
void	help __P((int, char **));
void	modecmd __P((int, char **));
void	put __P((int, char **));
void	quit __P((int, char **));
void	setascii __P((int, char **));
void	setbinary __P((int, char **));
void	setpeer __P((int, char **));
void	setrexmt __P((int, char **));
void	settimeout __P((int, char **));
void	settrace __P((int, char **));
void	setverbose __P((int, char **));
void	status __P((int, char **));

static __dead void command __P((void));

static void getusage __P((char *));
static void makeargv __P((void));
static void putusage __P((char *));
static void settftpmode __P((char *));

#define HELPINDENT (sizeof("connect"))

struct cmd {
	char	*name;
	char	*help;
	void	(*handler) __P((int, char **));
};

char	vhelp[] = "toggle verbose mode";
char	thelp[] = "toggle packet tracing";
char	chelp[] = "connect to remote tftp";
char	qhelp[] = "exit tftp";
char	hhelp[] = "print help information";
char	shelp[] = "send file";
char	rhelp[] = "receive file";
char	mhelp[] = "set file transfer mode";
char	sthelp[] = "show current status";
char	xhelp[] = "set per-packet retransmission timeout";
char	ihelp[] = "set total retransmission timeout";
char    ashelp[] = "set mode to netascii";
char    bnhelp[] = "set mode to octet";

struct cmd cmdtab[] = {
	{ "connect",	chelp,		setpeer },
	{ "mode",       mhelp,          modecmd },
	{ "put",	shelp,		put },
	{ "get",	rhelp,		get },
	{ "quit",	qhelp,		quit },
	{ "verbose",	vhelp,		setverbose },
	{ "trace",	thelp,		settrace },
	{ "status",	sthelp,		status },
	{ "binary",     bnhelp,         setbinary },
	{ "ascii",      ashelp,         setascii },
	{ "rexmt",	xhelp,		setrexmt },
	{ "timeout",	ihelp,		settimeout },
	{ "?",		hhelp,		help },
	{ 0 }
};

struct	cmd *getcmd();
char	*tail();
char	*index();
char	*rindex();

#define SA(addr) ((struct sockaddr *)(addr))
#define SIN(addr) ((struct sockaddr_in *)(addr))

static void
usage(void)
{
	fprintf(stderr, "usage: connect [[-es] -r file] host-name [port]\n");
	exit(1);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c, error;
	char *readfile;

	readfile = NULL;
	strcpy(mode, "netascii");
	signal(SIGINT, intr);

	while ((c = getopt(argc, argv, "esr:")) != EOF) {
		switch (c) {
		case 'r':
			readfile = optarg;
			break;
		case 's':
			silent = 1;
			break;
		case 'e':
			justcheck = 1;
			break;
		default:
			usage();
		}
	}
	
	if (!readfile && (silent || justcheck))
		usage();

	if (argc > 1) {
		if (setjmp(toplevel) != 0)
			exit(0);
		setpeer(argc - optind + 1, argv + optind - 1);
	}
	if (readfile) {
		settftpmode("octet");
		SIN(&peeraddr)->sin_port = port;
		error = recvfile(1, readfile, mode);
		exit (error);
	}

	if (setjmp(toplevel) != 0)
		(void)putchar('\n');
	command();
}

char    hostname[NI_MAXHOST];

set_peeraddr(name)
	char *name;
{
	struct addrinfo hints, *ai, *ai2;
	int aierr;
	struct sockaddr_in6 sin6;

	connected = 0;
	bzero((char *)&hints, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_CANONNAME;

	if (aierr = getaddrinfo(name, NULL, &hints, &ai)) {
		fprintf(stderr, "tftp: %s: %s\n", name, gai_strerror(aierr));
		return;
	}
	if (f >= 0) {
		close(f);
		f = -1;
	}
	for (ai2 = ai; ai; ai = ai->ai_next) {
		f = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (f < 0) {
			if (ai->ai_next)
				continue;
			perror("tftp: socket");
			break;
		}
		bzero((char *)&sin6, sizeof(sin6));
		SA(&sin6)->sa_family = ai->ai_family;
		SA(&sin6)->sa_len = ai->ai_addrlen;
		if (bind(f, SA(&sin6), SA(&sin6)->sa_len) < 0) {
			close(f);
			f = -1;
			if (ai->ai_next)
				continue;
			perror("tftp: bind");
			break;
		}
		bcopy(ai->ai_addr, &peeraddr, ai->ai_addrlen);
		strncpy(hostname, ai->ai_canonname, sizeof(hostname));
		hostname[sizeof(hostname)-1] = '\0';
		connected = 1;
		break;
	}
	freeaddrinfo(ai2);
	return(connected ? 0 : 1);
}

void
setpeer(argc, argv)
	int argc;
	char *argv[];
{
	char *p;
	struct addrinfo hints, *ai;
	int aierr;

#define	PREFIX	"Connect "
	if (argc < 2) {
		(void)strcpy(line, PREFIX);
		printf("(to) ");
		if (fgets(&line[sizeof(PREFIX) - 1],
		    sizeof(line) - (sizeof(PREFIX) - 1), stdin) == NULL) {
			printf("\n");
			goto usage;
		}
		if (line[sizeof(PREFIX) - 1] == '\n')
			goto usage;
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc > 3) {
usage:		fprintf(stderr, "usage: connect host-name [port]\n");
		return;
	}
	if (set_peeraddr(argv[1]))
		return;

	bzero((char *)&hints, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	p = (argc == 3) ? argv[2] : "tftp";
	if (aierr = getaddrinfo(NULL, p, &hints, &ai)) {
		fprintf(stderr, "tftp: %s: %s\n", p, gai_strerror(aierr));
		connected = 0;
	} else {
		port = SIN(ai->ai_addr)->sin_port;
		freeaddrinfo(ai);
	}
}

struct	modes {
	char *m_name;
	char *m_mode;
} modes[] = {
	{ "ascii",	"netascii" },
	{ "netascii",   "netascii" },
	{ "binary",     "octet" },
	{ "image",      "octet" },
	{ "octet",     "octet" },
/*      { "mail",       "mail" },       */
	{ 0,		0 }
};

void
modecmd(argc, argv)
	int argc;
	char *argv[];
{
	register struct modes *p;
	char *sep;

	if (argc < 2) {
		printf("Using %s mode to transfer files.\n", mode);
		return;
	}
	if (argc == 2) {
		for (p = modes; p->m_name; p++)
			if (strcmp(argv[1], p->m_name) == 0)
				break;
		if (p->m_name) {
			settftpmode(p->m_mode);
			return;
		}
		printf("%s: unknown mode\n", argv[1]);
		/* drop through and print usage message */
	}

	printf("usage: %s [", argv[0]);
	sep = " ";
	for (p = modes; p->m_name; p++) {
		printf("%s%s", sep, p->m_name);
		if (*sep == ' ')
			sep = " | ";
	}
	printf(" ]\n");
	return;
}

void
setbinary(argc, argv)
	int argc;
	char *argv[];
{      

	settftpmode("octet");
}

void
setascii(argc, argv)
	int argc;
	char *argv[];
{

	settftpmode("netascii");
}

static void
settftpmode(newmode)
	char *newmode;
{
	strcpy(mode, newmode);
	if (verbose)
		printf("mode set to %s\n", mode);
}


/*
 * Send file(s).
 */
void
put(argc, argv)
	int argc;
	char *argv[];
{
	int fd;
	register int n;
	register char *cp, *p, *targ;

	if (argc < 2) {
		strcpy(line, "send ");
		printf("(file) ");
		fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		putusage(argv[0]);
		return;
	}
	targ = argv[argc - 1];
	if (index(argv[argc - 1], ':')) {
		char *cp;

		for (n = 1; n < argc - 1; n++)
			if (index(argv[n], ':')) {
				putusage(argv[0]);
				return;
			}
		cp = argv[argc - 1];
		targ = rindex(cp, ':');	/* use last ':', for numeric IPv6 */
		*targ++ = 0;
		if (set_peeraddr(cp))
			return;
	}
	if (!connected) {
		printf("No target machine specified.\n");
		return;
	}
	if (argc < 4) {
		cp = argc == 2 ? tail(targ) : argv[1];
		fd = open(cp, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(cp);
			return;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				cp, hostname, targ, mode);
		SIN(&peeraddr)->sin_port = port;
		sendfile(fd, targ, mode);
		return;
	}
				/* this assumes the target is a directory */
				/* on a remote unix system.  hmmmm.  */
	cp = index(targ, '\0'); 
	*cp++ = '/';
	for (n = 1; n < argc - 1; n++) {
		strcpy(cp, tail(argv[n]));
		fd = open(argv[n], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(argv[n]);
			continue;
		}
		if (verbose)
			printf("putting %s to %s:%s [%s]\n",
				argv[n], hostname, targ, mode);
		SIN(&peeraddr)->sin_port = port;
		sendfile(fd, targ, mode);
	}
}

static void
putusage(s)
	char *s;
{
	fprintf(stderr, "usage: %s file ... host:target, or\n", s);
	fprintf(stderr, "       %s file ... target (when already connected)\n", s);
}

/*
 * Receive file(s).
 */
void
get(argc, argv)
	int argc;
	char *argv[];
{
	int fd;
	register int n;
	register char *cp, *p;
	char *src;

	if (argc < 2) {
		strcpy(line, "get ");
		printf("(files) ");
		fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		getusage(argv[0]);
		return;
	}
	if (!connected) {
		for (n = 1; n < argc ; n++)
			if (index(argv[n], ':') == 0) {
				getusage(argv[0]);
				return;
			}
	}
	for (n = 1; n < argc ; n++) {
		src = rindex(argv[n], ':'); /* use last ':', for numeric IPv6 */
		if (src == NULL)
			src = argv[n];
		else {
			*src++ = 0;
			if (set_peeraddr(argv[n]))
				return;
		}
		if (argc < 4) {
			cp = argc == 3 ? argv[2] : tail(src);
			fd = creat(cp, 0644);
			if (fd < 0) {
				fprintf(stderr, "tftp: "); perror(cp);
				return;
			}
			if (verbose)
				printf("getting from %s:%s to %s [%s]\n",
					hostname, src, cp, mode);
			SIN(&peeraddr)->sin_port = port;
			(void) recvfile(fd, src, mode);
			break;
		}
		cp = tail(src);         /* new .. jdg */
		fd = creat(cp, 0644);
		if (fd < 0) {
			fprintf(stderr, "tftp: "); perror(cp);
			continue;
		}
		if (verbose)
			printf("getting from %s:%s to %s [%s]\n",
				hostname, src, cp, mode);
		SIN(&peeraddr)->sin_port = port;
		(void) recvfile(fd, src, mode);
	}
}

static void
getusage(s)
	char *s;
{
	printf("usage: %s host:file host:file ... file, or\n", s);
	printf("       %s file file ... file if connected\n", s);
}

int	rexmtval = TIMEOUT;

void
setrexmt(argc, argv)
	int argc;
	char *argv[];
{
	int t;
	char *p;

	if (argc < 2) {
		strcpy(line, "Rexmt-timeout ");
		printf("(value) ");
		fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		printf("%s: bad value\n", argv[1]);
	else
		rexmtval = t;
}

int	maxtimeout = 5 * TIMEOUT;

void
settimeout(argc, argv)
	int argc;
	char *argv[];
{
	int t;
	char *p;

	if (argc < 2) {
		strcpy(line, "Maximum-timeout ");
		printf("(value) ");
		fgets(&line[strlen(line)], sizeof(line) - strlen(line), stdin);
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		printf("usage: %s value\n", argv[0]);
		return;
	}
	t = atoi(argv[1]);
	if (t < 0)
		printf("%s: bad value\n", argv[1]);
	else
		maxtimeout = t;
}

void
status(argc, argv)
	int argc;
	char *argv[];
{
	if (connected)
		printf("Connected to %s.\n", hostname);
	else
		printf("Not connected.\n");
	printf("Mode: %s Verbose: %s Tracing: %s\n", mode,
		verbose ? "on" : "off", trace ? "on" : "off");
	printf("Rexmt-interval: %d seconds, Max-timeout: %d seconds\n",
		rexmtval, maxtimeout);
}

void
intr()
{

	signal(SIGALRM, SIG_IGN);
	alarm(0);
	longjmp(toplevel, -1);
}

char *
tail(filename)
	char *filename;
{
	register char *s;
	
	while (*filename) {
		s = rindex(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return (s + 1);
		*s = '\0';
	}
	return (filename);
}

/*
 * Command parser.
 */
static __dead void
command()
{
	register struct cmd *c;
	char *p;

	for (;;) {
		printf("%s> ", prompt);
		if (fgets(line, sizeof(line), stdin) == NULL) {
			if (feof(stdin)) {
				exit(0);
			} else {
				continue;
			}
		}
		if (line[0] == 0)
			continue;
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		makeargv();
		if (margc == 0)
			continue;
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			printf("?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			printf("?Invalid command\n");
			continue;
		}
		(*c->handler)(margc, margv);
	}
}

struct cmd *
getcmd(name)
	register char *name;
{
	register char *p, *q;
	register struct cmd *c, *found;
	register int nmatches, longest;

	longest = 0;
	nmatches = 0;
	found = 0;
	for (c = cmdtab; (p = c->name) != NULL; c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return (c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return ((struct cmd *)-1);
	return (found);
}

/*
 * Slice a string up into argc/argv.
 */
static void
makeargv()
{
	register char *cp;
	register char **argp = margv;

	margc = 0;
	for (cp = line; *cp;) {
		while (isspace(*cp))
			cp++;
		if (*cp == '\0')
			break;
		*argp++ = cp;
		margc += 1;
		while (*cp != '\0' && !isspace(*cp))
			cp++;
		if (*cp == '\0')
			break;
		*cp++ = '\0';
	}
	*argp++ = 0;
}

void
quit(argc, argv)
	int argc;
	char *argv[];
{

	exit(0);
}

/*
 * Help command.
 */
void
help(argc, argv)
	int argc;
	char *argv[];
{
	register struct cmd *c;

	if (argc == 1) {
		printf("Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c->name; c++)
			printf("%-*s\t%s\n", (int)HELPINDENT, c->name, c->help);
		return;
	}
	while (--argc > 0) {
		register char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			printf("?Ambiguous help command %s\n", arg);
		else if (c == (struct cmd *)0)
			printf("?Invalid help command %s\n", arg);
		else
			printf("%s\n", c->help);
	}
}

void
settrace(argc, argv)
	int argc;
	char **argv;
{
	trace = !trace;
	printf("Packet tracing %s.\n", trace ? "on" : "off");
}

void
setverbose(argc, argv)
	int argc;
	char **argv;
{
	verbose = !verbose;
	printf("Verbose mode %s.\n", verbose ? "on" : "off");
}
