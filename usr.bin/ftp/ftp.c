/*	BSDI ftp.c,v 2.17 2002/01/10 20:44:05 dab Exp	*/

/*
 * Copyright (c) 1985, 1989, 1993, 1994
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
static char sccsid[] = "@(#)ftp.c	8.6 (Berkeley) 10/27/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <varargs.h>

#include "ftp_var.h"

#if FASTCTO
#include <setjmp.h>
#endif /* FASTCTO */

extern int h_errno;

typedef union {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
} su;
su hisctladdr;
su data_addr;
su myctladdr;
int	data = -1;
int	abrtflag = 0;
jmp_buf	ptabort;
int	ptabflg;
int	ptflag = 0;
off_t	restart_point = 0;


FILE	*cin, *cout;

extern int xferbufsize;
extern char *xferbuf;

int satoeport __P((char *, int, struct sockaddr *, int));
int osatoeport __P((char *, int, struct sockaddr *, int));
static void ai_unmapped __P((struct addrinfo *));

#if IPSEC
extern void *control_request, *data_request;
extern int control_requestlen, data_requestlen;
#endif /* IPSEC */

#ifdef FASTCTO
static jmp_buf timeout_env;

static void
timeout_handler(i)
	int i;
{
	longjmp(timeout_env, i);
}
#endif /* FASTCTO */

char *
hookup(host, port)
	char *host;
	char *port;
{
	int i;
	struct addrinfo req, *ai, *ai2;
	int s, len, tos;
	static char hostnamebuf[80];
	char buf1[46], buf2[16];
	int family, found = 0;

	memset(&req, 0, sizeof(struct addrinfo));

	req.ai_socktype = SOCK_STREAM;
	req.ai_flags |= AI_CANONNAME;

	pswitch(!proxy);
	family = connected ? hisctladdr.sa.sa_family : 0;
	pswitch(!proxy);

	if (!port)
		port = "ftp";

	if (i = getaddrinfo(host, port, &req, &ai)) {
		fprintf(stderr, "ftp: getaddrinfo: %s.%s: %s\n",
		    host, port, gai_strerror(i));
		return NULL;
	}

#ifdef FASTCTO
	signal(SIGALRM, &timeout_handler);
#endif /* FASTCTO */

	for (ai2 = ai, s = -1; ai; ai = ai->ai_next) {
		/*
		 * Make sure that ai_addr is NOT an IPv4 mapped address.
		 * IPv4 mapped address complicate too many things in FTP
		 * protocol handling, as FTP protocol is defined differently
		 * between IPv4 and IPv6.
		 */
		ai_unmapped(ai);

		if (family && (ai->ai_family != family))
			continue;
		found++;
		if (getnameinfo(ai->ai_addr, ai->ai_addrlen, buf1,
		    sizeof(buf1), buf2, sizeof(buf2),
		    NI_NUMERICHOST|NI_NUMERICSERV|NI_NOFQDN)) {
			printf("ftp: getnameinfo() failed!\n");
			continue;
		}

		printf("Trying %s.%s...\n", buf1, buf2);

		if (s >= 0)
			close(s);

		if ((s = socket(ai->ai_family, ai->ai_socktype,
		    ai->ai_protocol)) < 0) {
			perror("ftp: socket");
			continue;
		}

#if IPSEC
		if (control_request && setsockopt(s, SOL_SOCKET,
		    SO_SECURITY_REQUEST, control_request,
		    control_requestlen) < 0)
			fprintf(stderr, "ftp: setsockopt(..., SO_SECURITY_"
			    "REQUEST, control_request, ...): %s(%d)\n",
			    strerror(errno), errno);
#endif /* IPSEC */

#if FASTCTO
		if (setjmp(timeout_env)) {
			fprintf(stderr, "ftp: Connection timed out\n");
			continue;
		}

		alarm(FASTCTO);
#endif /* FASTCTO */

		if (connect(s, ai->ai_addr, ai->ai_addrlen) < 0) {
#ifdef FASTCTO
			alarm(0);
#endif /* FASTCTO */
			perror("ftp: connect");
			errno = 0;
			close(s);
			s = -1;
			continue;
		}

#ifdef FASTCTO
		alarm(0);
#endif /* FASTCTO */
		memcpy(&hisctladdr, ai->ai_addr, ai->ai_addrlen);
		if (ai->ai_canonname)
			strcpy(hostname = hostnamebuf, ai->ai_canonname);
		else
			strcpy(hostname = hostnamebuf, host);
		break;
	}

#ifdef FASTCTO
	signal(SIGALRM, SIG_IGN);
#endif /* FASTCTO */
	if (family && !found) {
		if (ai2->ai_next)
			fprintf(stderr, "No %s address found for %s\n",
			    (family == AF_INET) ? "IPv4" : "IPv6", host);
		else
			fprintf(stderr, "%s is not an %s address\n",
			    host, (family == AF_INET) ? "IPv4" : "IPv6");
	}

	freeaddrinfo(ai2);

	if (s < 0)
		return NULL;

	howport = 0;
	len = sizeof (myctladdr);
	if (getsockname(s, (struct sockaddr *)&myctladdr, &len) < 0) {
		warn("getsockname");
		code = -1;
		goto bad;
	}
#ifdef IP_TOS
	if (myctladdr.sa.sa_family == AF_INET) {
	tos = IPTOS_LOWDELAY;
	if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int)) < 0)
		warn("setsockopt TOS (ignored)");
	}
#endif
	cin = fdopen(s, "r");
	cout = fdopen(s, "w");
	if (cin == NULL || cout == NULL) {
		warnx("fdopen failed.");
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
	if (verbose)
		printf("Connected to %s.\n", hostname);
	if (getreply(0) > 2) { 	/* read startup message from server */
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on))
		< 0 && debug) {
			warn("setsockopt");
		}
	}
#endif /* SO_OOBINLINE */

	return (hostname);
bad:
	(void) close(s);
	return ((char *)0);
}

int
login(host)
	char *host;
{
	char tmp[80];
	char *user, *pass, *acct;
	int n, aflag = 0;

	user = pass = acct = 0;
	if (ruserpass(host, &user, &pass, &acct) < 0) {
		code = -1;
		return (0);
	}
	while (user == NULL) {
		char *myname = getlogin();

		if (myname == NULL) {
			struct passwd *pp = getpwuid(getuid());

			if (pp != NULL)
				myname = pp->pw_name;
		}
		if (myname)
			printf("Name (%s:%s): ", host, myname);
		else
			printf("Name (%s): ", host);
		if (fgets(tmp, sizeof(tmp) - 1, stdin) == NULL ||
		    tmp[0] == '\n') {
			if (feof(stdin))
				printf("\n");
			clearerr(stdin);
			user = myname;
		} else {
			if ((user = strchr(tmp, '\n')) != NULL)
				*user = '\0';
			user = tmp;
		}
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = getpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		acct = getpass("Account:");
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		warnx("Login failed.");
		return (0);
	}
	if (!aflag && acct != NULL)
		(void) command("ACCT %s", acct);
	if (proxy)
		return (1);
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

void
cmdabort()
{

	alarmtimer(0);
	printf("\n");
	(void) fflush(stdout);
	abrtflag++;
	if (ptflag)
		longjmp(ptabort,1);
}

/*VARARGS*/
int
command(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;
	int r;
	sig_t oldintr;

	abrtflag = 0;
	if (debug) {
		printf("---> ");
		va_start(ap);
		fmt = va_arg(ap, char *);
		if (strncmp("PASS ", fmt, 5) == 0)
			printf("PASS XXXX");
		else
			vfprintf(stdout, fmt, ap);
		va_end(ap);
		printf("\n");
		(void) fflush(stdout);
	}
	if (cout == NULL) {
		warn("No control connection for command");
		code = -1;
		return (0);
	}
	oldintr = signal(SIGINT, cmdabort);
	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(cout, fmt, ap);
	va_end(ap);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)(SIGINT);
	(void) signal(SIGINT, oldintr);
	return (r);
}

char reply_string[BUFSIZ];		/* last line of previous reply */

int
getreply(expecteof)
	int expecteof;
{
	int c, n;
	int dig;
	int originalcode = 0, continuation = 0;
	sig_t oldintr;
	int pflag = 0;
	char *cp, *pt = pasv;

	oldintr = signal(SIGINT, cmdabort);
	for (;;) {
		dig = n = code = 0;
		cp = reply_string;
		while ((c = getc(cin)) != '\n') {
			if (c == IAC) {     /* handle telnet commands */
				switch (c = getc(cin)) {
				case WILL:
					c = getc(cin);
					fprintf(cout, "%c%c%c", IAC, DONT, c);
					(void) fflush(cout);
					break;
				case DO:
					c = getc(cin);
					fprintf(cout, "%c%c%c", IAC, WONT, c);
					(void) fflush(cout);
					break;
				case WONT:
				case DONT:
					/*
					 * Don't reply to these since a
					 * simple minded server can go
					 * into a loop (such as ours)
					 */
				default:
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
					(void) signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (verbose) {
					printf("421 Service not available, remote server has closed connection\n");
					(void) fflush(stdout);
				}
				code = 421;
				return (4);
			}
			if (c != '\r' && (verbose > 0 ||
			    (verbose > -1 && n == '5' && dig > 4))) {
				if (proxflag &&
				   (dig == 1 || dig == 5 && verbose == 0))
					printf("%s:",hostname);
				(void) putchar(c);
			}
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (n == 0)
				n = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		}
		if (verbose > 0 || verbose > -1 && n == '5') {
			(void) putchar(c);
			(void) fflush (stdout);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (n != '1')
			cpend = 0;
		(void) signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabort && oldintr != SIG_IGN)
			(*oldintr)(SIGINT);
		return (n - '0');
	}
}

int
empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (long) sec;
	t.tv_usec = 0;
	return (select(32, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	sendabort;

void
abortsend()
{

	alarmtimer(0);
	mflag = 0;
	abrtflag = 0;
	printf("\nsend aborted\nwaiting for remote to finish abort\n");
	(void) fflush(stdout);
	longjmp(sendabort, 1);
}

void
sendrequest(cmd, local, remote, printnames)
	char *cmd, *local, *remote;
	int printnames;
{
	struct stat st;
	int c, d;
	FILE *fin, *dout = 0, *popen();
	int (*closefunc) __P((FILE *));
	sig_t oldinti, oldintr, oldintp;
	volatile off_t hashbytes = mark;
	char *lmode, *bufp;
	int oprogress;

	direction = "sent";
	bytes = 0;
	filesize = -1;
	oprogress = progress;
	if (verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
	}
	if (proxy) {
		proxtrans(cmd, local, remote);
		return;
	}
	if (curtype != type)
		changetype(type, 0);
	closefunc = NULL;
	oldinti = NULL;
	oldintr = NULL;
	oldintp = NULL;
	lmode = "w";
	if (setjmp(sendabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT,oldintr);
		if (oldintp)
			(void) signal(SIGPIPE,oldintp);
		if (oldinti)
			(void) signal(SIGINFO,oldinti);
		code = -1;
		goto cleanupsend;
	}
	oldintr = signal(SIGINT, abortsend);
	oldinti = signal(SIGINFO, psummary);
	if (strcmp(local, "-") == 0) {
		fin = stdin;
		progress = 0;
	} else if (*local == '|') {
		oldintp = signal(SIGPIPE,SIG_IGN);
		fin = popen(local + 1, "r");
		if (fin == NULL) {
			warn("%s", local + 1);
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGPIPE, oldintp);
			(void) signal(SIGINFO, oldinti);
			code = -1;
			goto cleanupsend;
		}
		progress = 0;
		closefunc = pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			warn("local: %s", local);
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGINFO, oldinti);
			code = -1;
			goto cleanupsend;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stdout, "%s: not a plain file.\n", local);
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGINFO, oldinti);
			fclose(fin);
			code = -1;
			goto cleanupsend;
		}
		filesize = st.st_size;
	}
	if (initconn(lmode)) {
		(void) signal(SIGINT, oldintr);
		(void) signal(SIGINFO, oldinti);
		if (oldintp)
			(void) signal(SIGPIPE, oldintp);
		code = -1;
		if (closefunc != NULL)
			(*closefunc)(fin);
		goto cleanupsend;
	}
	if (setjmp(sendabort))
		goto abort;

	if (restart_point &&
	    (strcmp(cmd, "STOR") == 0 || strcmp(cmd, "APPE") == 0)) {
		int rc;

		switch (curtype) {
		case TYPE_A:
			rc = fseek(fin, (long) restart_point, SEEK_SET);
			break;
		case TYPE_I:
		case TYPE_L:
			rc = lseek(fileno(fin), restart_point, SEEK_SET);
			break;
		}
		if (rc < 0) {
			warn("local: %s", local);
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			goto cleanupsend;
		}
		if (command("REST %ld", (long) restart_point)
			!= CONTINUE) {
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			goto cleanupsend;
		}
		restart_point = 0;
		lmode = "r+w";
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			(void)signal(SIGINFO, oldinti);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			goto cleanupsend;
		}
	} else
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			(void)signal(SIGINFO, oldinti);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			goto cleanupsend;
		}
	dout = dataconn(lmode);
	if (dout == NULL)
		goto abort;
	progressmeter(-1);
	setup_xferbuf(fileno(fin), fileno(dout), SO_SNDBUF);
	oldintp = signal(SIGPIPE, SIG_IGN);
	switch (curtype) {

	case TYPE_I:
	case TYPE_L:
		errno = d = 0;
		while ((c = read(fileno(fin), xferbuf, xferbufsize)) > 0) {
			bytes += c;
			for (bufp = xferbuf; c > 0; c -= d, bufp += d)
				if ((d = write(fileno(dout), bufp, c)) <= 0)
					break;
			if (hash && (!progress || filesize < 0)) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += mark;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && (!progress || filesize < 0) && bytes > 0) {
			if (bytes < mark)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			warn("local: %s", local);
		if (d < 0) {
			if (errno != EPIPE)
				warn("netout");
			bytes = -1;
		}
		break;

	case TYPE_A:
		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				while (hash && (!progress || filesize < 0) &&
				    (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += mark;
				}
				if (ferror(dout))
					break;
				(void) putc('\r', dout);
				bytes++;
			}
			(void) putc(c, dout);
			bytes++;
	/*		if (c == '\r') {			  	*/
	/*		(void)	putc('\0', dout);  // this violates rfc */
	/*			bytes++;				*/
	/*		}                          			*/	
		}
		if (hash && (!progress || filesize < 0)) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(fin))
			warn("local: %s", local);
		if (ferror(dout)) {
			if (errno != EPIPE)
				warn("netout");
			bytes = -1;
		}
		break;
	}
	progressmeter(1);
	if (closefunc != NULL)
		(*closefunc)(fin);
	(void) fclose(dout);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	(void)signal(SIGINFO, oldinti);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (bytes > 0)
		ptransfer(0);
	goto cleanupsend;
abort:
	(void) signal(SIGINT, oldintr);
	(void)signal(SIGINFO, oldinti);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (!cpend) {
		code = -1;
		return;
	}
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (dout)
		(void) fclose(dout);
	(void) getreply(0);
	code = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	if (bytes > 0)
		ptransfer(0);
cleanupsend:
	progress = oprogress;
	restart_point = 0;
}

jmp_buf	recvabort;

void
abortrecv()
{

	alarmtimer(0);
	mflag = 0;
	abrtflag = 0;
	printf("\nreceive aborted\nwaiting for remote to finish abort\n");
	(void) fflush(stdout);
	longjmp(recvabort, 1);
}

void
recvrequest(cmd, local, remote, lmode, printnames)
	char *cmd, *local, *remote, *lmode;
	int printnames;
{
	FILE *fout, *din = 0;
	int (*closefunc) __P((FILE *));
	sig_t oldinti, oldintr, oldintp;
	int c, d, is_retr, tcrflag, bare_lfs = 0;
	volatile off_t hashbytes = mark;
	time_t mtime;
	struct timeval tval[2];
	int oprogress;
	int opreserve;

	direction = "received";
	bytes = 0;
	filesize = -1;
	oprogress = progress;
	opreserve = preserve;
	is_retr = strcmp(cmd, "RETR") == 0;
	if (is_retr && verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
	}
	if (proxy && is_retr) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldinti = NULL;
	oldintp = NULL;
	tcrflag = !crflag && is_retr;
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT, oldintr);
		if (oldinti)
			(void)signal(SIGINFO, oldinti);
		oprogress = progress;
		preserve = opreserve;
		code = -1;
		return;
	}
	oldintr = signal(SIGINT, abortrecv);
	oldinti = signal(SIGINFO, psummary);
	if (strcmp(local, "-") && *local != '|') {
		if (access(local, 2) < 0) {
			char *dir = strrchr(local, '/');

			if (errno != ENOENT && errno != EACCES) {
				warn("local: %s", local);
				(void) signal(SIGINT, oldintr);
				(void) signal(SIGINFO, oldinti);
				code = -1;
				return;
			}
			/*
			 * Check whether the directory is writable.  Special
			 * case for file in root directory.
			 */
			if (dir == local)
				dir = NULL;
			if (dir != NULL)
				*dir = 0;
			d = access(dir ? local : ".", 2);
			if (dir != NULL)
				*dir = '/';
			if (d < 0) {
				warn("local: %s", local);
				(void) signal(SIGINT, oldintr);
				(void) signal(SIGINFO, oldinti);
				code = -1;
				return;
			}
			if (!runique && errno == EACCES &&
			    chmod(local, 0600) < 0) {
				warn("local: %s", local);
				(void) signal(SIGINT, oldintr);
				(void) signal(SIGINT, oldintr);
				(void) signal(SIGINFO, oldinti);
				code = -1;
				return;
			}
			if (runique && errno == EACCES &&
			   (local = gunique(local)) == NULL) {
				(void) signal(SIGINT, oldintr);
				(void) signal(SIGINFO, oldinti);
				code = -1;
				return;
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGINFO, oldinti);
			code = -1;
			return;
		}
	}
	if (!is_retr) {
		if (curtype != TYPE_A)
			changetype(TYPE_A, 0);
	} else {
		if (curtype != type)
			changetype(type, 0);
		filesize = remotesize(remote, 0);
	}
	if (initconn("r")) {
		(void) signal(SIGINT, oldintr);
		(void) signal(SIGINFO, oldinti);
		code = -1;
		return;
	}
	if (setjmp(recvabort))
		goto abort;
	if (is_retr && restart_point &&
	    command("REST %ld", (long) restart_point) != CONTINUE)
		return;
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGINFO, oldinti);
			return;
		}
	} else {
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGINFO, oldinti);
			return;
		}
	}
	din = dataconn("r");
	if (din == NULL)
		goto abort;
	if (strcmp(local, "-") == 0) {
		fout = stdout;
		progress = 0;
		preserve = 0;
	} else if (*local == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = popen(local + 1, "w");
		if (fout == NULL) {
			warn("%s", local+1);
			goto abort;
		}
		progress = 0;
		preserve = 0;
		closefunc = pclose;
	} else {
		fout = fopen(local, lmode);
		if (fout == NULL) {
			warn("local: %s", local);
			goto abort;
		}
		closefunc = fclose;
	}
	setup_xferbuf(fileno(fout), fileno(din), SO_RCVBUF);
	progressmeter(-1);
	switch (curtype) {

	case TYPE_I:
	case TYPE_L:
		if (restart_point &&
		    lseek(fileno(fout), restart_point, SEEK_SET) < 0) {
			warn("local: %s", local);
			if (closefunc != NULL)
				(*closefunc)(fout);
			progress = oprogress;
			preserve = opreserve;
			return;
		}
		errno = d = 0;
		while ((c = read(fileno(din), xferbuf, xferbufsize)) > 0) {
			if ((d = write(fileno(fout), xferbuf, c)) != c)
				break;
			bytes += c;
			if (hash && (!progress || filesize < 0)) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += mark;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && (!progress || filesize < 0) && bytes > 0) {
			if (bytes < mark)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0) {
			if (errno != EPIPE)
				warn("netin");
			bytes = -1;
		}
		if (d < c) {
			if (d < 0)
				warn("local: %s", local);
			else
				warnx("%s: short write", local);
		}
		break;

	case TYPE_A:
		if (restart_point) {
			int i, n, ch;

			if (fseek(fout, 0L, SEEK_SET) < 0)
				goto done;
			n = restart_point;
			for (i = 0; i++ < n;) {
				if ((ch = getc(fout)) == EOF)
					goto done;
				if (ch == '\n')
					i++;
			}
			if (fseek(fout, 0L, SEEK_CUR) < 0) {
done:
				warn("local: %s", local);
				if (closefunc != NULL)
					(*closefunc)(fout);
				progress = oprogress;
				preserve = opreserve;
				return;
			}
		}
		while ((c = getc(din)) != EOF) {
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				while (hash && (!progress || filesize < 0) &&
				    (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += mark;
				}
				bytes++;
				if ((c = getc(din)) != '\n' || tcrflag) {
					if (ferror(fout))
						goto break2;
					(void) putc('\r', fout);
					if (c == '\0') {
						bytes++;
						goto contin2;
					}
					if (c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, fout);
			bytes++;
	contin2:	;
		}
break2:
		if (bare_lfs) {
			printf("WARNING! %d bare linefeeds received in ASCII mode\n", bare_lfs);
			printf("File may not have transferred correctly.\n");
		}
		if (hash && (!progress || filesize < 0)) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(din)) {
			if (errno != EPIPE)
				warn("netin");
			bytes = -1;
		}
		if (ferror(fout))
			warn("local: %s", local);
		break;
	}
	progressmeter(1);
	progress = oprogress;
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) signal(SIGINT, oldintr);
	(void) signal(SIGINFO, oldinti);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	(void) fclose(din);
	(void) getreply(0);
	if (bytes > 0 && is_retr)
		ptransfer(0);

	if (preserve && remote && (closefunc == fclose)) {
		mtime = remotemodtime(remote, 0);
		if (mtime != -1) {
			(void)gettimeofday(&tval[0], NULL);
			tval[1].tv_sec = mtime;
			tval[1].tv_usec = 0;
			if (utimes(local, tval) < 0)
				warn("Can't change modification time"
				    " on %s to %s", local,
				    asctime(localtime(&mtime)));
		} else
			printf("MDTM command failed\n");
	}
	preserve = opreserve;
	return;
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	progress = oprogress;
	preserve = opreserve;
	if (oldintp)
		(void) signal(SIGPIPE, oldintr);
	(void) signal(SIGINT, SIG_IGN);
	if (!cpend) {
		code = -1;
		(void) signal(SIGINT, oldintr);
		(void) signal(SIGINFO, oldinti);
		return;
	}

	abort_remote(din);
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		(void) fclose(din);
	if (bytes > 0)
		ptransfer(0);
	(void) signal(SIGINT, oldintr);
	(void) signal(SIGINFO, oldinti);
}

#define BADPORT	-1
#define	NOPORT	-2	


#define PASV_PORT	1
#define EPSV_EPRT	2
#define SPASV_SPORT	3
#define LPSV_LPRT	4

struct hprt {
	char *name;
	int val;
} hprt[] = {
	{ "PORT",	PASV_PORT },
	{ "EPRT",	EPSV_EPRT },
	{ "SPORT",	SPASV_SPORT },
	{ "LPRT",	LPSV_LPRT },
	{ "UNDEF",	0 },
	{ "<unknown>",	-1 }
};

void
set_howport(how)
	char *how;
{
	struct hprt *hp;

	if (how == NULL) {
		for (hp = hprt; hp->val >= 0; hp++)
			if (howport == hp->val)
				break;
		printf("type is set to %s\n", hp->name);
		return;
	}

	for (hp = hprt; hp->val >= 0; hp++) {
		if (strncasecmp(how, hp->name, strlen(how)) == 0) {
			if (hp->val == PASV_PORT &&
			    ((hisctladdr.sa.sa_family != AF_INET) ||
			    (myctladdr.sa.sa_family != AF_INET)))
				printf("PORT is only valid with "
				    "IPv4 connections\n");
			else {
				printf("type is set to %s\n", hp->name);
				howport = hp->val;
			}
			return;
		}
	}
	printf("Bad type %s.  Valid types:", how);
	for (hp = hprt; hp->val >= 0; hp++)
		printf(" %s", hp->name);
	printf("\n");
}


pasv_command(is_proxy)
	int is_proxy;
{
	int result = ERROR;

	switch (howport) {
	case 0:
	case PASV_PORT:
		if ((hisctladdr.sa.sa_family == AF_INET) &&
		    (myctladdr.sa.sa_family == AF_INET)) {
			result = command("PASV");
			if ((result == COMPLETE) &&
			    !porttosa(&data_addr, pasv, &hisctladdr)) {
				howport = 1;
				break;
			}
		}
		if (howport)
			break;
		/* FALL THROUGH */
	case EPSV_EPRT:
		result = command("EPSV");
		if (result == COMPLETE) {
			char *c, *c2;

			if ((c = strchr(reply_string, '(')) &&
			    (c2 = strchr(++c, ')'))) {
				*c2 = 0;
				if (!eporttosa(&data_addr, c,
				    &hisctladdr, 1)) {
					howport = 2;
					if (!is_proxy && (passivemode != 2) &&
					    (command("EPSV ALL") ==
					    COMPLETE))
						passivemode = 2;
					break;
				}
			}
		}
		if (howport)
			break;
		/* FALL THROUGH */
	case SPASV_SPORT:
		if (!is_proxy) {
			result = command("SPASV");
			if ((result == COMPLETE) &&
			    !sporttosa(&data_addr, pasv, &hisctladdr)) {
				howport = 3;
				break;
			}
			if (howport)
				break;
		}
		/* FALL THROUGH */
	case LPSV_LPRT:
		result = command("LPSV");
		if ((result == COMPLETE) &&
		    !lporttosa(&data_addr, pasv, &hisctladdr)) {
			howport = 4;
			break;
		}
		/* FALL THROUGH */
	default:
		break;
	}

	return (result);
}

port_command(is_proxy)
	int is_proxy;
{
	int result;
	static int (*eport_func)
	    __P((char *, int, struct sockaddr *, int)) = satoeport;

	char buffer[1+1+1+NI_MAXHOST+1+NI_MAXSERV+1+1];

	switch (howport) {
	case 0:
		if (is_proxy)
			return (ERROR);
		/* FALL THROUGH */
	case PASV_PORT:
		if ((hisctladdr.sa.sa_family == AF_INET) &&
		    (myctladdr.sa.sa_family == AF_INET)) {
			if (is_proxy)
				result = command("PORT %s", pasv);
			else {
				if (satoport(buffer, &data_addr)) {
					printf("Can't convert address for "
					    "transmission\n");
					return (BADPORT);
				}
				result = command("PORT %s", buffer);
			}
			if (result == COMPLETE) {
				howport = 1;
				return (COMPLETE);
			}
			if (result == ERROR) {
				if (code == 530)
					return (ERROR);
				if (sendport == -1 && howport)
					return (NOPORT);
			}
		}
		if (howport)
			return (ERROR);
		eport_func = satoeport;
		/* FALL THROUGH */
	case EPSV_EPRT:
		if (eport_func(buffer, sizeof(buffer),
		    (struct sockaddr *)&data_addr, 0)) {
			printf(
			    "Can't convert address for transmission\n");
			return (BADPORT);
		}

		result = command("EPRT %s", buffer);
		if (result == COMPLETE) {
			howport = 2;
			return (COMPLETE);
		}
		if (result == ERROR) {
			if (code == 530)
				return (ERROR);
			if (sendport == -1 && howport)
				return (NOPORT);
		}
		if (howport)
			return (ERROR);
		/* FALL THROUGH */
	case SPASV_SPORT:
		if (satosport(buffer, &data_addr)) {
			printf(
			    "Can't convert address for transmission\n");
			return (BADPORT);
		}
		result = command("SPORT %s", buffer);
		if (result == ERROR) {
			if (code == 530)
				return (ERROR);
			if (sendport == -1 && howport)
				return (NOPORT);
		} else if (result == COMPLETE) {
			howport = 3;
			return (COMPLETE);
		}
		if (howport)
			return (ERROR);
		/* FALL THROUGH */
	case LPSV_LPRT:
		if (is_proxy)
			result = command("LPRT %s", pasv);
		else {
			if (satolport(buffer, &data_addr)) {
				printf("Can't convert address for "
				    "transmission\n");
				return (BADPORT);
			}
			result = command("LPRT %s", buffer);
		}
		if (result == ERROR) {
			if (code == 530)
				return (ERROR);
			if (sendport == -1 && howport)
				return (NOPORT);
		} else if (result == COMPLETE) {
			howport = 4;
			return (COMPLETE);
		}
		if (howport)
			return (ERROR);
	default:
		return 1;
	}
}

void
set_dataopts(s, mode, family)
	int s;
	char *mode;
	int family;
{
	int on;

#ifdef IP_TOS
	on = IPTOS_THROUGHPUT;
	if (family == AF_INET &&
	    setsockopt(data, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		perror("ftp: setsockopt TOS (ignored)");
#endif

#ifdef SO_SNDBUF
	on = 65536;
	if (setsockopt(s, SOL_SOCKET, (*mode == 'w') ? SO_SNDBUF : SO_RCVBUF,
            (char *)&on, sizeof on) < 0)
		fprintf(stderr, "ftp: setsockopt %s (ignored): %s\n",
                    (*mode == 'w') ? "SO_SNDBUF" : "SO_RCVBUF",
		    strerror(errno));
#endif
}

/*
 * Need to start a listen on the data channel before we send the command,
 * otherwise the server's connect may fail.
 */
int
initconn(lmode)
	char *lmode;
{
	int len, tmpno = 0;
	int on = 1;

	if (passivemode && !passivefailed) {
		data = socket(hisctladdr.sa.sa_family, SOCK_STREAM, 0);
		if (data < 0) {
			perror("ftp: socket");
			return(1);
		}
		if ((options & SO_DEBUG) &&
		    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on,
			       sizeof (on)) < 0)
			perror("ftp: setsockopt (ignored)");
#if IPSEC
                if (data_request && setsockopt(data, SOL_SOCKET, SO_SECURITY_REQUEST,
		    data_request, data_requestlen) < 0)
			fprintf(stderr, "ftp: setsockopt(..., "
			    "SO_SECURITY_REQUEST, data_request, ...): %s(%d)\n",
			    strerror(errno), errno);
#endif /* IPSEC */
		if (pasv_command(0) != COMPLETE) {
			if (code == 500 || code == 502) {
				passivefailed = 1;
				fprintf(stderr, "ftp: passive mode failed, "
				    "using PORT mode for this connection\n");
				goto noport;
			}
			goto bad;
		}
		if (connect(data, (struct sockaddr *)&data_addr,
		    data_addr.sa.sa_len) < 0)
		{
			perror("ftp: connect");
			goto bad;
		}
		set_dataopts(data, lmode, data_addr.sa.sa_family);
		return(0);
	}

noport:
	memcpy(&data_addr, &myctladdr, myctladdr.sa.sa_len);
	if (sendport)
		switch(myctladdr.sa.sa_family) {
		case AF_INET:
			data_addr.sin.sin_port = 0;
			break;
		case AF_INET6:
			data_addr.sin6.sin6_port = 0;
			break;
		default:
			printf("Don't know how to handle af#%d addresses!",
			    myctladdr.sa.sa_family);
			break;
		}
	if (data != -1)
		close(data);

	data = socket(data_addr.sa.sa_family, SOCK_STREAM, 0);
	if (data < 0) {
		warn("socket");
		if (tmpno)
			sendport = 1;
		return (1);
	}
#if IPSEC
	if (data_request && setsockopt(data, SOL_SOCKET, SO_SECURITY_REQUEST,
	    data_request, data_requestlen) < 0)
		fprintf(stderr, "ftp: setsockopt(..., SO_SECURITY_REQUEST, "
		    "data_request, ...): %s(%d)\n", strerror(errno), errno);
#endif /* IPSEC */
	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			warn("setsockopt (reuse address)");
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr,
	    data_addr.sa.sa_len) < 0)
	{
		warn("bind");
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		warn("setsockopt (ignored)");
	len = sizeof (data_addr);
	if (getsockname(data, (struct sockaddr *)&data_addr, &len) < 0) {
		warn("getsockname");
		goto bad;
	}
	if (listen(data, 1) < 0)
		warn("listen");
	if (sendport) {
		switch (port_command(0)) {
		case COMPLETE:
			return (0);
		case ERROR:
			return (-1);
		case BADPORT:
			goto bad;
		case NOPORT:
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
	}
	if (tmpno)
		sendport = 1;
	set_dataopts(data, lmode, data_addr.sa.sa_family);
	return (0);
bad:
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

FILE *
dataconn(lmode)
	char *lmode;
{
	su from;
	int s, fromlen = sizeof (from);

	if (passivemode && !passivefailed)
		return (fdopen(data, lmode));

	s = accept(data, (struct sockaddr *) &from, &fromlen);
	if (s < 0) {
		warn("accept");
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
	set_dataopts(s, lmode, from.sa.sa_family);

	return (fdopen(data, lmode));
}

void
psummary(notused)
	int notused;
{

	if (bytes > 0)
		ptransfer(1);
}

void
psabort()
{

	alarmtimer(0);
	abrtflag++;
}

void
pswitch(flag)
	int flag;
{
	sig_t oldintr;
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
	        su mctl;
	        su hctl;
		FILE *in;
		FILE *out;
		int tpe;
		int curtpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
	} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	} else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	memcpy(&ip->hctl, &hisctladdr, hisctladdr.sa.sa_len);
	memcpy(&hisctladdr, &op->hctl, op->hctl.sa.sa_len);
	memcpy(&ip->mctl, &myctladdr, myctladdr.sa.sa_len);
	memcpy(&myctladdr, &op->mctl, op->mctl.sa.sa_len);
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
	type = op->tpe;
	ip->curtpe = curtype;
	curtype = op->curtpe;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
	(void) signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)(SIGINT);
	}
}

void
abortpt()
{

	alarmtimer(0);
	printf("\n");
	(void) fflush(stdout);
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	longjmp(ptabort, 1);
}

void
proxtrans(cmd, local, remote)
	char *cmd, *local, *remote;
{
	sig_t oldintr = NULL;
	int secndflag = 0, prox_type, nfnd;
	char *cmd2;
	struct fd_set mask;

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if ((prox_type = type) == 0) {
		if (unix_server && unix_proxy)
			prox_type = TYPE_I;
		else
			prox_type = TYPE_A;
	}
	if (curtype != prox_type)
		changetype(prox_type, 1);
	if (pasv_command(1) != COMPLETE) {
		printf("proxy server does not support third party transfers.\n");
		return;
	}
	pswitch(0);
	if (!connected) {
		printf("No primary connection\n");
		pswitch(1);
		code = -1;
		return;
	}
	if (curtype != prox_type)
		changetype(prox_type, 1);
	if (port_command(1) != COMPLETE) {
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
	oldintr = signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote) != PRELIM) {
		(void) signal(SIGINT, oldintr);
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	pswitch(1);
	ptflag = 0;
	printf("local: %s remote: %s\n", local, remote);
	return;
abort:
	(void) signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			if (cpend)
				abort_remote((FILE *) NULL);
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
		(void) signal(SIGINT, oldintr);
		return;
	}
	if (cpend)
		abort_remote((FILE *) NULL);
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			if (cpend)
				abort_remote((FILE *) NULL);
			pswitch(1);
			if (ptabflg)
				code = -1;
			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend)
		abort_remote((FILE *) NULL);
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask, 10)) <= 0) {
			if (nfnd < 0) {
				warn("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	pswitch(1);
	if (ptabflg)
		code = -1;
	(void) signal(SIGINT, oldintr);
}

void
reset(argc, argv)
	int argc;
	char *argv[];
{
	struct fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,0)) < 0) {
			warn("reset");
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			(void) getreply(0);
		}
	}
}

char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp = strrchr(local, '/');
	int d, count=0;
	char ext = '1';

	if (cp)
		*cp = '\0';
	d = access(cp ? local : ".", 2);
	if (cp)
		*cp = '/';
	if (d < 0) {
		warn("local: %s", local);
		return ((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			printf("runique: can't find unique file name.\n");
			return ((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return (new);
}

void
abort_remote(din)
	FILE *din;
{
	int nfnd;
	struct fd_set mask;

	setup_xferbuf(-1, fileno(din), SO_RCVBUF);
	/*
	 * send IAC in urgent mode instead of DM because 4.3BSD places oob mark
	 * after urgent byte rather than before as is protocol now
	 */
	sprintf(xferbuf, "%c%c%c", IAC, IP, IAC);
	if (send(fileno(cout), xferbuf, 3, MSG_OOB) != 3)
		warn("abort");
	fprintf(cout,"%cABOR\r\n", DM);
	(void) fflush(cout);
	FD_ZERO(&mask);
	FD_SET(fileno(cin), &mask);
	if (din) {
		FD_SET(fileno(din), &mask);
	}
	if ((nfnd = empty(&mask, 10)) <= 0) {
		if (nfnd < 0) {
			warn("abort");
		}
		if (ptabflg)
			code = -1;
		lostpeer();
	}
	if (din && FD_ISSET(fileno(din), &mask)) {
		while (read(fileno(din), xferbuf, xferbufsize) > 0)
			/* LOOP */;
	}
	if (getreply(0) == ERROR && code == 552) {
		/* 552 needed for nic style abort */
		(void) getreply(0);
	}
	(void) getreply(0);
}

static void
ai_unmapped(ai)
	struct addrinfo *ai;
{
	struct sockaddr_in6 *sin6;
	struct sockaddr_in sin;

	if (ai->ai_family != AF_INET6)
		return;
	if (ai->ai_addrlen != sizeof(struct sockaddr_in6) ||
	    sizeof(sin) > ai->ai_addrlen)
		return;
	sin6 = (struct sockaddr_in6 *)ai->ai_addr;
	if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);
	memcpy(&sin.sin_addr, &sin6->sin6_addr.s6_addr[12],
	    sizeof(sin.sin_addr));
	sin.sin_port = sin6->sin6_port;

	ai->ai_family = AF_INET;
	memcpy(ai->ai_addr, &sin, sin.sin_len);
	ai->ai_addrlen = sin.sin_len;
}
