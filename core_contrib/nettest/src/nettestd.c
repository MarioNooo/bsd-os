static char USMID[] = "@(#)tcp/usr/etc/nettest/nettestd.c	80.5	11/03/92 17:12:29";

/*
 * Copyright 1992 Cray Research, Inc.
 * All Rights Reserved.
 */
/*
 * Permission to use, copy, modify and distribute this software, in
 * source and binary forms, and its documentation, without fee is
 * hereby granted, provided that:  1) the above copyright notice and
 * this permission notice appear in all source copies of this
 * software and its supporting documentation; 2) distributions
 * including binaries display the following acknowledgement:  ``This
 * product includes software developed by Cray Research, Inc.'' in
 * the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of
 * this software; 3) the name Cray Research, Inc. may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission; 4) the USMID revision line and
 * binary copyright notice are retained without modification in all
 * source and binary copies of this software; 5) the software is
 * redistributed only as part of a bundled package and not as a
 * separate product (except that it may be redistibuted separately if
 * if no fee is charged); and 6) this software is not renamed in any
 * way and is referred to as Nettest.
 *
 * THIS SOFTWARE IS PROVIDED AS IS AND CRAY RESEARCH, INC.
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL CRAY RESEARCH, INC. BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
char copyright[] =
"@(#) Copyright 1992 Cray Research, Inc.\n\
 All rights reserved.\n";

#include "nettest.h"

#include <errno.h>
#include <signal.h>
#ifdef	WAIT3CODE
#include <sys/wait.h>
#endif
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef	CRAY2
#include <sys/sysmacros.h>
#endif
#include <netinet/tcp.h>
#ifndef	NO_ISO
#include <netiso/iso.h>
#include <netiso/tp_user.h>
#include <netinet/in.h>
#endif	/* NO_ISO */
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/termios.h>

#ifdef	WAIT3CODE
void
dochild(sig)
	int sig;
{
	int pid;

	while ((pid = wait3(0, WNOHANG, 0)) > 0)
		;
}
#else
#define	dochild	SIG_IGN
#endif

int dflag;
int	ipoptions = 0;
int	mesghdr = 0;
#define	debug(x)	if(dflag>1)fprintf x
int verbose;
int daemon_mode = 0;		/* are we running in daemon mode? */

#ifdef	TCP_WINSHIFT
int	winshift;
int	usewinshift;
#endif

#define D_DEFAULT	0
#define	D_PIPE		1
#define	D_LOCAL		2
#define	D_INET		3
#define	D_FILE		4
#define D_ISO		5
#define D_INET6 	6

int domain = D_DEFAULT;
int type = SOCK_STREAM;

union {
	struct sockaddr		d_gen;
	struct sockaddr_un	d_local;
	struct sockaddr_in	d_inet;
#ifndef NO_ISO
	struct sockaddr_iso	d_iso;
#endif /* NO_ISO */
#ifdef INET6
	struct sockaddr_in6	d_inet6;
#endif /* INET6 */
} name;
int namesize;

typedef int (*rfunc_type)(int, void *, size_t, int);
rfunc_type rfunc = (rfunc_type)read;

void usage(void);
void error(char *);
int data_stream(int, int);
void do_dgram(int);
void do_stream(int);
int get_port(char *, int);

int
main(argc, argv)
int argc;
char **argv;
{
	register int	s, s2, mode, dev1, dev2;
	char		*portname = 0;
	short		port = PORTNUMBER;
	int		on = 1;
	int		c;
	char		*f1 = NULL, *f2 = NULL;
#ifndef NO_ISO
	/* OSI transport selector */
	union {
		int	port;
		char	data[sizeof(int)];
	} portnumber;
#endif /* NO_ISO */
	char		buf[256], buf2[256];
	extern int	optind;
	extern char	*optarg;

	while ((c = getopt(argc, argv, "bdimp:s:vV")) != EOF) {
		switch(c) {
		case 'b':	/* run as a background daemon */
			if (verbose) {
				fprintf(stderr,
				    "-v flag ignored when -b flag is used\n");
				verbose = 0;
			}
			daemon_mode++;
			break;
		case 'd':	/* turn on socket level debugging */
			++dflag;
			break;
		case 'i':
			ipoptions++;
			break;
		case 'm':	/* use recvmsg() instead of recvfrom() */
#ifdef CMSG_DATA
			++mesghdr;
#else
			printf("'m' flag not supported\n");
#endif
			break;
		case 'p':	/* specify the protocol to use */
			if (!strcmp(optarg, "local") ||
			    !strcmp(optarg, "unix")) {
				domain = D_LOCAL;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "locald") ||
			    !strcmp(optarg, "unixd")) {
				domain = D_LOCAL;
				type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "iso")) {
#ifndef	NO_ISO
				domain = D_ISO;
				type = SOCK_SEQPACKET;
#else
				printf("Unsupported protocol: %s\n", optarg);
				usage();
#endif /* NO_ISO */
			} else if (!strcmp(optarg, "tcp")) {
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "udp")) {
				type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "tcp4")) {
				domain = D_INET;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "udp4")) {
				domain = D_INET;
				type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "tcp6")) {
				domain = D_INET6;
				type = SOCK_STREAM;
			} else if (!strcmp(optarg, "udp6")) {
				domain = D_INET6;
				type = SOCK_DGRAM;
			} else if (!strcmp(optarg, "file")) {
				domain = D_FILE;
			} else if (!strcmp(optarg, "pipe")) {
#ifdef	NAMEDPIPES
				domain = D_PIPE;
#else
				printf("Unsupported protocol: %s\n", optarg);
				usage();
#endif	/* NAMEDPIPES */
			} else {
				printf("Unknown protocol: %s\n", optarg);
				usage();
			}
			break;

		case 's':	/* set the default window shift value */
#ifdef	TCP_WINSHIFT
			usewinshift++;
			winshift = atoi(optarg);
			if (winshift < -1 || winshift > 14) {
				fprintf(stderr, "window shift (-s) must be beteen -1 and 14\n");
				usage();
			}
#else
			fprintf(stderr, "window shift option not supported\n");
			usage();
#endif

		case 'v':	/* print out errors in sequenced data */
			if (daemon_mode) {
				fprintf(stderr,
				    "-v flag ignored when -b flag is used\n");
				verbose = 0;
			} else {
				++verbose;
			}
			break;

		case 'V':	/* print out version & copyright info */
			printf("%s\n%s", &USMID[4], &copyright[4]);
			exit(0);

		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

#ifdef	TCP_WINSHIFT
	if (usewinshift && domain && ((domain != D_INET && domain != D_INET6) ||
	    (type != SOCK_STREAM))) {
		fprintf(stderr,
		    "nettestd: -s option ignored (only valid for tcp)\n");
		usewinshift = 0;
	}
#endif
	if (mesghdr && (type != SOCK_DGRAM)) {
		fprintf(stderr, "nettestd: -m option ignored (only "
		    "valid for udp and locald)\n");
		mesghdr = 0;
	}
	if (ipoptions && domain && (domain != D_INET && domain != D_INET6) &&
	    (type != SOCK_DGRAM)) {
		fprintf(stderr,
		    "nettestd: -i option ignored (only valid for udp)\n");
		ipoptions = 0;
	}

	if (domain == D_FILE) {
		if (argc != 2)
			usage();
		f1 = *argv++;
		f2 = *argv++;
		argc -= 2;
	} else if (argc > 1)
		usage();
	else if (argc == 1) {
		portname = *argv++;
		argc--;
#ifndef NO_ISO
		port = atoi(portname);
#endif
	}

#ifndef	_PATH_TTY
# define _PATH_TTY "/dev/tty"
#endif
#ifndef	TIOCNOTTY
# ifdef	TIOCSCTTY
#  define TIOCNOTTY TIOCSCTTY
# else
#  ifdef TCSETCTTY
#   define TIOCNOTTY TCSETCTTY
#  endif
# endif
#endif
	if (daemon_mode) {
#ifdef notdef
		if (setpgrp() < 0)
			perror("setpgrp");
		if ((c = open(_PATH_TTY, O_RDWR)) >= 0) {
			(void)ioctl(c, TIOCNOTTY, (char *)0);
			(void)close(c);
		}
		switch (fork()) {
		default:	/* in the parent */
			exit(0);
		case -1:	/* fork failed */
			perror("nettestd: fork");
			exit(1);
		case 0:		/* in the child */
			break;
		}
		for (c = getdtablesize(); c > 0; --c)
			(void)close(c);
#endif
		if (daemon(1, 0) < 0)
			perror("nettestd: fork");
		for (c = getdtablesize(); c > 2; --c)
			(void)close(c);
	}

	switch (domain) {
#ifdef	NAMEDPIPES
	case D_PIPE:
		if (portname == 0)
			portname = PIPENAME;
		mode = S_IFIFO|0666;
		dev1 = dev2 = 0;
		sprintf(buf, "%sR", portname);
		sprintf(buf2, "%sW", portname);
		umask(0);
		for(;;) {
			unlink(buf);
			if (mknod(buf, mode, dev1) < 0) {
				error("mknod");
				exit(1);
			}
			unlink(buf2);
			if (mknod(buf2, mode, dev2) < 0) {
				error("mknod");
				goto err1;
			}
			if ((s2 = open(buf2, O_RDONLY)) < 0) {
				error(buf2);
				goto err2;
			}
			if ((s = open(buf, O_WRONLY)) < 0) {
				error(buf);
				close(s2);
			err2:	unlink(buf2);
			err1:	unlink(buf);
				exit(1);
			}
			data_stream(s2, s);
			close(s2);
			close(s);
		}
		break;
#endif	/* NAMEDPIPES */
	case D_FILE:
		for(;;) {
			s = open(f1, 0);
			if (s < 0) {
				error(f1);
				exit(1);
			}
			s2 = open(f2, 1);
			if (s2 < 0) {
				error(f2);
				exit(1);
			}
			data_stream(s, s2);
			close(s2);
			close(s);
			sleep(1);
		}
		break;

	case D_LOCAL:
		if (portname == 0)
			portname = (type == SOCK_DGRAM) ? LOCALDPORT : LOCALPORT;
		name.d_local.sun_family = AF_LOCAL;
		strcpy(name.d_local.sun_path, portname);
		namesize = sizeof(name.d_local) - sizeof(name.d_local.sun_path)
				+ strlen(name.d_local.sun_path);
		(void) unlink(portname);
		goto dosock;
		break;

	case D_DEFAULT:
#ifdef INET6
	case D_INET6:
		bzero(&name.d_inet6, sizeof(name.d_inet6));
		name.d_inet6.sin6_family = AF_INET6;
		port = get_port(portname, type);
		if (port <= 0)
			exit(1);
		name.d_inet6.sin6_port = htons(port);
		namesize = name.d_inet6.sin6_len = sizeof(name.d_inet6);
		goto dosock;
		break;
#endif /* INET6 */

	case D_INET:
		name.d_inet.sin_family = AF_INET;
		port = get_port(portname, type);
		if (port <= 0)
			exit(1);
		name.d_inet.sin_port = htons(port);
#if	!defined(CRAY) || defined(s_addr)
		name.d_inet.sin_addr.s_addr = 0;
#else
		name.d_inet.sin_addr = 0;
#endif
		namesize = sizeof(name.d_inet);
		goto dosock;
		break;

#ifndef NO_ISO
	case D_ISO:

		name.d_iso.siso_len = sizeof(struct sockaddr_iso);
		name.d_iso.siso_family = AF_ISO;
		name.d_iso.siso_tlen = 2;
		if ((port <= 0) || (port > 65535)) {
			fprintf(stderr, "bad ISO port number\n");
			exit(1);
		}
		portnumber.port = htons(port);
		bcopy(&(portnumber.data[(sizeof(int)-2)]),
						TSEL(&(name.d_iso)), 2);
		namesize = sizeof(name.d_iso);

#endif /* NO_ISO */
	dosock:
		if ((s = socket(name.d_gen.sa_family, type, 0)) < 0) {
#ifdef INET6
			if (errno == EPROTONOSUPPORT && domain == D_DEFAULT &&
			     name.d_gen.sa_family == AF_INET6) {
				name.d_gen.sa_family = AF_INET;
				namesize = sizeof(name.d_inet);
				s = socket(name.d_gen.sa_family, type, 0);
			}
			if (s < 0)
#endif /* INET6 */
			{
				error("socket");
				exit(1);
			}
		}
		if (dflag && setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on)) < 0)
			error("setsockopt - SO_DEBUG");
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#if defined(IP_RECVOPTS) || defined(IP_RECVRETOPTS) || defined(IP_RECVDSTADDR)
		if (ipoptions)
			if(
# if	defined(IP_RECVOPTS)
				setsockopt(s, IPPROTO_IP,
					IP_RECVOPTS, &on, sizeof(on)) < 0
# endif
# if	defined(IP_RECVRETOPTS)
			    ||	setsockopt(s, IPPROTO_IP,
					IP_RECVRETOPTS, &on, sizeof(on)) < 0
# endif
# if	defined(IP_RECVDSTADDR)
			    ||	setsockopt(s, IPPROTO_IP,
					IP_RECVDSTADDR, &on, sizeof(on)) < 0
# endif
			    )
			error("setsockopt (IP_OPTIONS)");
#endif
		if (bind(s, &name.d_gen, namesize) < 0) {
			error("bind");
			exit(1);
		}
		if (type == SOCK_DGRAM)
			do_dgram(s);
		else
			do_stream(s);
		/*NOTREACHED*/
		break;
	}
	exit(0);
}

int
get_port(name, type)
	char *name;
	int type;
{
	struct servent *sp;
	int port;
	       
	sp = getservbyname((name == NULL ? "nettest" : name),
	    (type == SOCK_DGRAM ? "udp" : "tcp"));
	if (sp)
		return(ntohs(sp->s_port));
	if (name == NULL)
		return (PORTNUMBER);
	port = atoi(name);
	if (port <= 0 || port >= 0xffff) {
		fprintf(stderr, "bad port: %s\n", name);
		return (-1);
	}
	return (port);
}

void
do_stream(s)
register int s;
{
	register int		i, s2;
#ifndef NO_ISO
	struct sockaddr_iso	isoname;/*ZAP*/
#endif /* NO_ISO */

#ifdef	TCP_WINSHIFT
	if (usewinshift) {
		if (setsockopt(s, IPPROTO_TCP, TCP_WINSHIFT, &winshift,
						sizeof(winshift)) < 0)
			error("setsockopt - TCP_WINSHIFT");
	}
#endif
	listen(s, 5);

	signal(SIGCHLD, dochild);
	for (;;) {
		namesize = sizeof(name);
		s2 = accept(s, &name.d_gen, &namesize);
		if (s2 < 0) {
			extern int errno;
			if (errno == EINTR)
				continue;
			error("accept");
		} else {
			if ((i = fork()) == 0) {
				close(s);
				i = data_stream(s2, s2);
				shutdown(s2, 2);
				exit(i);
			} else if (i < 0)
				error("fork");
			close(s2);
		}
	}
}


#ifndef	CRAY
#define	VRFY() { \
		register int j, k; \
		register long *ldp = (long *)(data + (offset&~0x7)); \
		register int len = t + (offset&0x7); \
		for (j = 0; j < len/8; j++) { \
			k = (ntohl(*ldp++) != hival); \
			if ((ntohl(*ldp++) != loval) || k) { \
				printf("expected %8lx%8lx, got %8x%8x\n", \
					hival, loval, ntohl(*(ldp-2)), \
					ntohl(*(ldp-1))); \
				hival = ntohl(*(ldp-2)); \
				loval = ntohl(*(ldp-1)); \
			} \
			if (++loval == 0) \
				++hival; \
		} \
		if ((len&0x7) && (offset+t) >= chunksize) { \
			*(ldp-(chunksize/8)) = *ldp; \
			*(ldp-(chunksize/8)+1) = *(ldp+1); \
		} \
	}
#else
#define	VRFY() { \
		register int j; \
		register long *ldp = (long *)(data + (offset&~0x7)); \
		register int len = t + (offset&0x7); \
		for (j = 0; j < len/8; j++) { \
			if (*ldp++ != loval) { \
				printf("expected %16x, got %16x\n", \
					loval, *(ldp-1)); \
				loval = *(ldp-1); \
			} \
			++loval; \
		} \
		if ((len&0x7) && ((offset+t) >= chunksize)) { \
			*(ldp-(chunksize/8)) = *ldp; \
		} \
	}
#endif

int
data_stream(in, out)
int in, out;
{
	register int	i, t, offset = 0;
	register char	*cp, *data;
	char		buf[128], *malloc();
	int		chunks = 0, chunksize = 0, fullbuf = 0, kbufsize = 0;
	int		tos = 0, nodelay = 0, seqdata = 0, waitall = 0;
	register unsigned long hival, loval;

#ifndef NO_ISO
	/* read ISO CR - 0 bytes of data */
	if (domain == D_ISO) {
		if ((i = read(in, buf, sizeof(buf))) != 0) {
			fprintf(stderr, "read(ISO CR) failed\n");
			exit(1);
		}
	}
#endif /* NO_ISO */
	for (cp = buf; ; ) {
		i = read(in, cp, 1);
		if (i != 1) {
			if (i < 0)
				error("nettestd: read");
			else
				fprintf(stderr, "nettestd: Read returned %d, expected 1\n", i);
			exit(1);
		}
		if (*cp == '\n')
			break;
		cp++;
	}
	*cp = '\0';
	sscanf(buf, "%d %d %d %d %d %d %d %d", &chunks, &chunksize, &fullbuf,
				&kbufsize, &tos, &nodelay, &seqdata, &waitall);
	/*
	 * If fullbuf is set, allocate a buffer twice as big.  This
	 * is so that we can always read a full size buffer, from
	 * the offset of the last read.  This keeps the data in
	 * the first chunksize consistent in case the remote side
	 * is trying to verify the contents.
	 */
	data = malloc(fullbuf ? 2*chunksize : chunksize);
	if (data == NULL) {
		sprintf(buf, "0 malloc() failed\n");
		write(out, buf, strlen(buf));
		return(1);
	}
	strcpy(buf, "1");
	if (kbufsize) {
#ifdef	SO_SNDBUF
		if ((setsockopt(out, SOL_SOCKET, SO_SNDBUF, &kbufsize,
						sizeof(kbufsize)) < 0) ||
		    (setsockopt(in, SOL_SOCKET, SO_RCVBUF, &kbufsize,
						sizeof(kbufsize)) < 0))
#endif
			strcat(buf, " Cannot set buffers sizes.");
	}
	if (tos) {
#ifdef	IP_TOS
		if (setsockopt(out, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0)
#endif
			strcat(buf, " Cannot set TOS bits.");
	}
	if (nodelay) {
#ifdef	TCP_NODELAY
		if (setsockopt(out, IPPROTO_TCP, TCP_NODELAY, &nodelay,
							sizeof(nodelay)) < 0)
#endif
			strcat(buf, " Cannot set TCP_NODELAY.");
	}
	if (waitall) {
#ifdef	MSG_WAITALL
		waitall = MSG_WAITALL;
		rfunc = recv;
#else
		strcat(buf, " MSG_WAITALL not supported.");
		waitall = 0;
#endif
	}
	strcat(buf, " \n");
	write(out, buf, strlen(buf));
	hival = loval = 0;
	for (i = 0; i < chunks || offset; i++) {
		if ((t = (*rfunc)(in, data + offset, chunksize, waitall)) < 0) {
			sprintf(buf, "server: read #%d.%d", i+1, chunksize);
			goto bad;
		}
		if (t == 0) {
			fprintf(stderr, "server: EOF on read, block # %d\n", i);
			break;
		}
		if (verbose && seqdata)
			VRFY();
/*@*/		debug((stderr, "server: %d: read %d\n", i, t));
		if (fullbuf) {
			offset += t;
			if (offset >= chunksize)
				offset -= chunksize;
			else
				--i;
		} else while (t != chunksize) {
			register int t2;
			t2 = (*rfunc)(in, data+t, chunksize-t, waitall);
			if (verbose && seqdata)
				VRFY();
			if (t2 < 0) {
				sprintf(buf, "server: read #%d.%d",
							i+1, chunksize-t);
				goto bad;
			}
			if (t2 == 0) {
				fprintf(stderr, "server: EOF on read, block # %d\n", i);
				break;
			}
			t += t2;
/*@*/			debug((stderr, "server: %d: partial read %d (%d)\n", i, t2, t));
		}
	}

	hival = loval = 0;
	for (i = 0; i < chunks; i++) {
		if (seqdata) {
			register int j;
			register long *ldp = (long *)data;
			for (j = 0; j < chunksize/8; j++) {
#ifndef	CRAY
				*ldp++ = htonl(hival);
				*ldp++ = htonl(loval);
				if (++loval == 0)
					++hival;
#else
				*ldp++ = loval++;
#endif
			}
		}
		if ((t = write(out, data, chunksize)) < 0) {
			sprintf(buf, "server: write #%d", i+1);
			goto bad;
		}
		if (t != chunksize)
			fprintf(stderr, "server: write: %d vs %d\n", t, chunksize);
/*@*/		else
/*@*/			debug((stderr, "server: %d: write %d\n", i, t));
	}
#ifndef	NO_ISO
	/* read ISO sync-up data */
	if (domain == D_ISO) {
		if ((i = read(in, buf, sizeof(buf))) == 4) {
			if (strncmp(buf, "DONE", 4))
				fprintf(stderr,
					"OSI server got wrong sync-up data\n");
		} else
			fprintf(stderr,
				"OSI server got wrong size sync-up (%d)\n", i);
	}
#endif /* NO_ISO */
	free(data);
	return(0);
bad:
	error(buf);
	free(data);
	return(1);
}

#define	MAXSIZE	(64*1024)

void
do_dgram(s)
int s;
{
	register int		t;
	char			*cp, *data;
	char			*inet_ntoa(), *malloc();
	register char		*errmsg;
#ifdef	CMSG_DATA
	struct msghdr		inmsg;
	struct iovec		iov;
	char			control[3*(sizeof(struct cmsghdr)+40)];
#endif /* CMSG_DATA */

	data = malloc(MAXSIZE);
	if (data == NULL) {
		fprintf(stderr, "no malloc\n");
		shutdown(s, 2);
		exit(1);
	}
#ifdef CMSG_DATA
	if(mesghdr) {
		iov.iov_base = data;
		iov.iov_len = MAXSIZE;
		inmsg.msg_iov = &iov;
		inmsg.msg_iovlen = 1;
		inmsg.msg_name = (caddr_t)&name.d_inet;
		inmsg.msg_control = (caddr_t)control;
		inmsg.msg_flags = 0;
		errmsg = "recvmsg";
	} else
#endif /* CMSG_DATA */
		errmsg = "recvfrom";

	for (;;) {
#ifdef CMSG_DATA
		if (mesghdr) {
			inmsg.msg_namelen = sizeof(name.d_inet);
			inmsg.msg_controllen = sizeof(control);
			t = recvmsg(s, &inmsg, 0);
		} else
#endif
		{
			namesize = sizeof(name);
			t = recvfrom(s, data, MAXSIZE, 0, &name.d_gen,
				 &namesize);
		}
		if (t < 0) {
			error(errmsg);
			continue;
		}
		if (domain == D_INET) {
			cp = inet_ntoa(name.d_inet.sin_addr);
			printf("got %d bytes from %s\n", t, cp);
#ifdef INET6
		} else if (domain == D_INET6) {
			char addrbuf[64];
			(const char *)cp = inet_ntop(name.d_inet6.sin6_family,
			    &name.d_inet6.sin6_addr, addrbuf, sizeof(addrbuf));
			printf("Got %d bytes from %s\n", t, cp ? cp : "<unknown>");
#endif /* INET6 */
		} else
			printf("got %d bytes\n", t);
	}
}

void
usage()
{
	fprintf(stderr,
		"Usage: nettestd [-b] [-d] [-v] [-s val] [-p tcp] [port]\n"
		"       nettestd [-b] [-d] [-i] [-m] -p udp [port]\n"
#ifdef INET6
		"       nettestd [-b] [-d] [-v] [-s val] -p tcp6 [port]\n"
		"       nettestd [-b] [-d] [-i] [-m] -p udp6 [port]\n"
#endif
#ifndef	NO_ISO
		"       nettestd [-b] [-d] [-v] -p iso [port]\n"
#endif
		"       nettestd [-b] [-d] [-v] -p local|pipe [filename]\n"
		"       nettestd [-b] [-d] [-m] -p locald [filename]\n"
		"       nettestd [-b] [-d] [-v] -p file readfile writefile\n"
		"       nettestd -V\n");
	exit(1);
}

void
error(string)
char *string;
{
	if (daemon_mode)
		syslog(LOG_ERR, "nettestd: %s %m", string);
	else
		perror(string);
}
