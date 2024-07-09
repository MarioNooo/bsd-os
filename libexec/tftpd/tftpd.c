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
static char sccsid[] = "@(#)tftpd.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton
 * <guyton@rand-unix>.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "tftpsubs.h"

#define	TIMEOUT		5
#define	PKTSIZE	SEGSIZE+4

#define SA(s)	((struct sockaddr *)(s))
#define	SIN(s)	((struct sockaddr_in *)(s))
#define	SIN6(s)	((struct sockaddr_in6 *)(s))

/*
 * Null-terminated directory prefix list for absolute pathname requests and
 * search list for relative pathname requests.
 */
struct dirlist {
	char	*name;
	int	len;
	TAILQ_ENTRY(dirlist)	chain;
};

struct formats {
	char	*f_mode;
	int	(*f_validate) __P((char **, int));
	void	(*f_send) __P((struct formats *));
	void	(*f_recv) __P((struct formats *));
	int	f_convert;
};

struct errmsg {
	int	e_code;
	char	*e_msg;
};

enum dest_rc { GOOD, BAD, BCAST };

union packet {
	struct	tftphdr header;
	char	buf[PKTSIZE];
};

static union packet ackbuf;
static struct	sockaddr_in6 addr;
static union packet buf;
static TAILQ_HEAD(dirhead, dirlist) dirs;
static FILE	*file;
static struct	sockaddr_in6 from;
static int	logging;
static int	maxtimeout = 5*TIMEOUT;
static int	peer;
static int	rexmtval = TIMEOUT;
static int	suppress_naks;
static int	tftpfd;
static int	timeout;
static jmp_buf	timeoutbuf;

static char	*errtomsg __P((int));
static void	justquit __P((int));
static void	nak __P((int));
static void	recvfile __P((struct formats *));
static void	sendfile __P((struct formats *));
static void	tftp __P((struct tftphdr*, int));
static void	timer __P((int));
static int	validate_access __P((char **, int));
static enum	dest_rc verify_dest __P((struct tftphdr *, int, struct sockaddr *,
		    struct sockaddr *));
static char	*verifyhost __P((struct sockaddr *));
static void	fail __P((const char *, ...));

static struct errmsg errmsgs[] = {
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

static struct formats formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail, 1 },
#endif
	{ 0 }
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct msghdr msghdr;
	struct iovec iov;
	struct tftphdr *tp;
	struct cmsghdr *cp;
	int n;
	int ch, on;
	char cbuf[1024];

	TAILQ_INIT(&dirs);
	
	openlog("tftpd", LOG_PID | LOG_NDELAY, LOG_FTP);

	if (getuid() == 0)
		fail("cannot run as root");

	while ((ch = getopt(argc, argv, "ln")) != EOF) {
		switch (ch) {
		case 'l':
			logging = 1;
			break;
		case 'n':
			suppress_naks = 1;
			break;
		default:
			fail("unknown command-line option");
		}
	}
	argc -= optind;
	argv += optind;

	/* Get list of directory prefixes. */
	for (; argc != 0; argc--) {
		struct dirlist *dirp;

		if (*argv[argc - 1] != '/') 
			fail("illegal relative path in commandline: %s", 
			    argv[argc - 1]);

		dirp = (struct dirlist *)calloc(1, sizeof(*dirp));
		if (dirp == NULL)
			fail("calloc: %m");

		dirp->name = argv[argc - 1];
		dirp->len = strlen(dirp->name);
		TAILQ_INSERT_HEAD(&dirs, dirp, chain);
	}
	if (TAILQ_FIRST(&dirs) == NULL) 
		fail("at least one command-line directory argument required");

	on = 1;
	if (ioctl(0, FIONBIO, &on) < 0)
		fail("ioctl(FIONBIO): %m");
	if (setsockopt(0, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof(on)) < 0) {
		int oerrno = errno;
		if (setsockopt(0, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on,
		    sizeof(on)) < 0) {
			errno = oerrno;
			fail("setsockopt(IPPROTO_IP, IP_RECVDSTADDR): %m");
		}
	}

	memset(&msghdr, 0, sizeof(msghdr));
	msghdr.msg_name = (caddr_t)&from;
	msghdr.msg_namelen = sizeof(from);
	msghdr.msg_iov = &iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = cbuf;
	msghdr.msg_controllen = sizeof(cbuf);

	iov.iov_base = buf.buf;
	iov.iov_len = sizeof(buf);

	n = recvmsg(0, &msghdr, 0);
	if (n < 0)
		fail("recvfrom: %m");
	tftpfd = -1;

	/*
	 * Now that we have read the message out of the UDP
	 * socket, we fork and exit.  Thus, inetd will go back
	 * to listening to the tftp port, and the next request
	 * to come in will start up a new instance of tftpd.
	 *
	 * We do this so that inetd can run tftpd in "wait" mode.
	 * The problem with tftpd running in "nowait" mode is that
	 * inetd may get one or more successful "selects" on the
	 * tftp port before we do our receive, so more than one
	 * instance of tftpd may be started up.  Worse, if tftpd
	 * break before doing the above "recvfrom", inetd would
	 * spawn endless instances, clogging the system.
	 */
	{
		int pid;
		int i;

		for (i = 1; i < 20; i++) {
		    pid = fork();
		    if (pid < 0) {
				sleep(i);
				/*
				 * flush out to most recently sent request.
				 *
				 * This may drop some request, but those
				 * will be resent by the clients when
				 * they timeout.  The positive effect of
				 * this flush is to (try to) prevent more
				 * than one tftpd being started up to service
				 * a single request from a single client.
				 */
				msghdr.msg_namelen = sizeof(from);
				msghdr.msg_controllen = sizeof(cbuf);
				i = recvmsg(0, &msghdr, 0);
				if (i > 0) {
					n = i;
				}
		    } else {
				break;
		    }
		}
		if (pid < 0)
			fail("fork: %m");
		else if (pid != 0) 
			exit(0);
	}

	memset(&addr, 0, sizeof(addr));
	SA(&addr)->sa_family = SA(&from)->sa_family;
	SA(&addr)->sa_len = SA(&from)->sa_len;

#define	ENOUGH_CMSG(cp, size)	((cp)->cmsg_len >= ((size) + sizeof(*cp)))
	if (msghdr.msg_controllen >= sizeof(*cp) &&
	    !(msghdr.msg_flags & MSG_CTRUNC)) {
	
		for (cp = CMSG_FIRSTHDR(&msghdr);
		     cp && cp->cmsg_len >= sizeof(*cp);
		     cp = CMSG_NXTHDR(&msghdr, cp)) {
			if (cp->cmsg_level == IPPROTO_IP &&
			    cp->cmsg_type == IP_RECVDSTADDR &&
			    ENOUGH_CMSG(cp, sizeof(struct in_addr))) {
				struct in6_addr *s6;
				if (SA(&addr)->sa_family == AF_INET) {
					SIN(&addr)->sin_addr = 
					    *(struct in_addr *)CMSG_DATA(cp);
					break;
				}
				/* Map V4 address to V6 format */
				s6 = &SIN6(&addr)->sin6_addr;
				s6->s6_addr[10] = s6->s6_addr[11] = 0xff;
				*(u_int32_t *)&s6->s6_addr[12] =
				    ((struct in_addr *)CMSG_DATA(cp))->s_addr;
				break;
			} else if (cp->cmsg_level == IPPROTO_IPV6 &&
			    cp->cmsg_type == IPV6_PKTINFO &&
			    ENOUGH_CMSG(cp, sizeof(struct in6_pktinfo))) {
				SIN6(&addr)->sin6_family = AF_INET6;
				SIN6(&addr)->sin6_len =
				    sizeof(struct sockaddr_in6);
				SIN6(&addr)->sin6_addr = ((struct in6_pktinfo *)
				    CMSG_DATA(cp))->ipi6_addr;
				SIN6(&addr)->sin6_scope_id =
				    SIN6(&from)->sin6_scope_id;
				break;
			}
		}
	}

	tp = &buf.header;
	NTOHS(tp->th_opcode);
	switch (verify_dest(tp, n, SA(&addr), SA(&from))) {
	case BAD:
		/* Ignore the request */
		exit(1);

	case BCAST:
		/* Do not nak a broadcast request */
		suppress_naks = 1;
		break;

	case GOOD:
		break;
	}

	alarm(0);
	close(0);
	close(1);
	peer = socket(SA(&addr)->sa_family, SOCK_DGRAM, 0);
	if (peer < 0)
		fail("socket: %m");
	if (bind(peer, SA(&addr), SA(&addr)->sa_len) < 0)
		fail("bind: %m");
	if (connect(peer, SA(&from), SA(&from)->sa_len) < 0) 
		fail("connect: %m");
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

/*
 * Handle initial connection protocol.
 */
void
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	struct formats *pf;
	int first = 1, ecode;
	char *cp;
	char *filename, *mode;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf.buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(&filename, tp->th_opcode);
	if (logging) {
		char hbuf[MAXHOSTNAMELEN];

		(void)snprintf(hbuf, sizeof(hbuf), "%s", verifyhost(SA(&addr)));

		syslog(LOG_INFO, "%s -> %s: %s request for %s: %s",
		       verifyhost(SA(&from)),
		       hbuf,
		       tp->th_opcode == WRQ ? "write" : "read",
		       filename, errtomsg(ecode));
	}
	if (ecode) {
		/*
		 * Avoid storms of naks to a RRQ broadcast for a relative
		 * bootfile pathname from a diskless Sun.
		 */
		if (suppress_naks && *filename != '/' && ecode == ENOTFOUND)
			exit(0);
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}


/*
 * Validate file access.  Since we have no uid or gid, for now require
 * file to exist and be publicly readable/writable.
 * We use the arguments from inetd as prefix search path.
 */
int
validate_access(filep, mode)
	char **filep;
	int mode;
{
	struct dirlist *dirp;
	struct stat stbuf;
	int fd;
	char *filename = *filep;
	static char pathname[MAXPATHLEN];

	/*
	 * Prevent tricksters from getting around the directory restrictions
	 */
	if ((*filename == '\0') || strstr(filename, "/../") 
	    || ( !strncmp(filename, "../", 3)))
		return (EACCESS);

	/* If absolute filename, make it relative */
	if (*filename == '/' && *++filename == '\0')
		return (EACCESS);

	/*
	 * If the file exists in one of the directories, stop searching.
	 * If it exists but isn't accessable, change the error code
	 * to give an indication that the file exists.
	 */
	for (dirp = dirs.tqh_first; dirp != NULL; 
	    dirp = dirp->chain.tqe_next) {
		(void)snprintf(pathname, sizeof(pathname), "%s/%s",
		    dirp->name, filename);
		if (stat(pathname, &stbuf) == 0 &&
		    (stbuf.st_mode & S_IFMT) == S_IFREG) {
			if (mode == RRQ) {
				if ((stbuf.st_mode & S_IROTH) == 0)
					return (EACCESS);
			} else {
				if ((stbuf.st_mode & S_IWOTH) == 0)
					return (EACCESS);
			}
			break;
		}
	}
	if (dirp == NULL)
		return (ENOTFOUND);

	*filep = filename = pathname;
	fd = open(filename, mode == RRQ ? O_RDONLY : (O_WRONLY|O_TRUNC));
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		return (errno + 100);
	}
	return (0);
}

void
timer(int signo)
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
void
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *r_init();
	struct tftphdr *ap;    /* ack packet */
	int size, n;
	volatile u_short block;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = &ackbuf.header;
	block = 1;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void)setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf.buf, sizeof(ackbuf), 0);
			alarm(0);
			if (n < 0) {
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			NTOHS(ap->th_opcode);
			NTOHS(ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;

			if (ap->th_opcode == ACK) {
				if (ap->th_block == block)
					break;
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1))
					goto send_data;
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

void
justquit(int signo)
{
	exit(0);
}


/*
 * Receive a file.
 */
void
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *w_init();
	struct tftphdr *ap;    /* ack buffer */
	int n, size;
	volatile u_short block;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = &ackbuf.header;
	block = 0;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf.buf, sizeof *ap, 0) != sizeof *ap) {
			syslog(LOG_ERR, "tftpd: write: %m");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "tftpd: read: %m");
				goto abort;
			}
			NTOHS(dp->th_opcode);
			NTOHS(dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf.buf, sizeof *ap, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf.buf, sizeof(buf), 0);
					/* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf.buf, sizeof *ap, 0);
					/* resend final ack */
	}
abort:
	return;
}

static char *
errtomsg(error)
	int error;
{
	struct errmsg *pe;
	static char ebuf[20];

	if (error == 0)
		return "success";
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			return pe->e_msg;
	(void)snprintf(ebuf, sizeof(ebuf), "error %d", error);
	return ebuf;
}

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
	struct tftphdr *tp;
	struct errmsg *pe;
	int length;

	tp = &buf.header;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf.buf, length, 0) != length)
		syslog(LOG_ERR, "nak: %m");
}

static char *
verifyhost(fromp)
	struct sockaddr *fromp;
{
	static char nbuf[NI_MAXHOST];

	if (getnameinfo(fromp, fromp->sa_len, nbuf, sizeof(nbuf), NULL, 0, 0))
		inet_ntop(fromp->sa_family, fromp->sa_family == AF_INET6 ?
		    (char *)&SIN6(fromp)->sin6_addr :
		    (char *)&SIN(fromp)->sin_addr,
		    nbuf, sizeof(nbuf));
	return (nbuf);
}

static enum dest_rc
verify_dest(tp, size, addr, from)
	struct tftphdr *tp;
	int size;
	struct sockaddr *addr;
	struct sockaddr *from;
{
	struct ifaddrs *ifaddrs, *ifap;
	struct sockaddr_in addr4, from4;
	u_long laddr;
	enum dest_rc rc;
	char *fromaddr;

	rc = GOOD;
	laddr = INADDR_ANY;

	/*
	 * RFC1123 says that we should not respond to broadcast
	 * requests, we extend that to multicast addresses.  We
	 * provide an exception for net-booting Sun workstations by
	 * allowing broadcasts to the local-wire and subnet broadcast
	 * address if they orignate on the local network.  Later
	 * checks will restrict these requests based on filename.
	 */
	if (addr->sa_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&SIN6(addr)->sin6_addr)) {
		/* Translate mapped address into AF_INET address */

		memset(&addr4, 0, sizeof(addr4));
		addr4.sin_len = sizeof(addr4);
		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr =
		    *(u_int32_t *)&SIN6(addr)->sin6_addr.s6_addr[12];
		addr = SA(&addr4);
		memset(&from4, 0, sizeof(from4));
		from4.sin_len = sizeof(from4);
		from4.sin_family = AF_INET;
		from4.sin_addr.s_addr =
		    *(u_int32_t *)&SIN6(from)->sin6_addr.s6_addr[12];
		from = SA(&from4);
	}
	if (addr->sa_family == AF_INET6) {
		if (IN6_IS_ADDR_UNSPECIFIED(&SIN6(addr)->sin6_addr) ||
		    IN6_IS_ADDR_MULTICAST(&SIN6(addr)->sin6_addr)) {
			char nbuf1[INET6_ADDRSTRLEN], nbuf2[INET6_ADDRSTRLEN];

		Broadcast6:
			syslog(LOG_WARNING, "ignoring %s request to %s from %s",
			    IN6_IS_ADDR_MULTICAST(&SIN6(addr)->sin6_addr) ?
			    "multicast" : "broadcast",
			    inet_ntop(AF_INET6, &SIN6(addr)->sin6_addr,
				nbuf2, sizeof(nbuf2)),
			    inet_ntop(AF_INET6, &SIN6(from)->sin6_addr,
				nbuf1, sizeof(nbuf1)));
			return (BAD);
		}
	} else {
		laddr = ntohl(SIN(addr)->sin_addr.s_addr);

		if (laddr == INADDR_ANY || IN_MULTICAST(laddr)) {

		Broadcast:
			fromaddr = strdup(inet_ntoa(SIN(from)->sin_addr));
			syslog(LOG_WARNING, "ignoring %s request to %s from %s",
			    IN_MULTICAST(laddr) ? "multicast" : "broadcast",
			    inet_ntoa(SIN(addr)->sin_addr),
			    fromaddr);
			free(fromaddr);
			return (BAD);
		}
	}

	/* Get interface list and compare against broadcast addresses */
	if (getifaddrs(&ifaddrs) < 0) {
		syslog(LOG_WARNING, "getifaddrs: %m, continuing");
		return (rc);
	}
	for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {
		if (ifap->ifa_addr == NULL ||
		    ifap->ifa_broadaddr == NULL ||
		    (ifap->ifa_flags & IFF_BROADCAST) == 0)
			continue;
		if (SA(ifap->ifa_broadaddr)->sa_family !=
		    SA(addr)->sa_family)
			continue;
		if (SA(addr)->sa_family == AF_INET6) {
			if (IN6_ARE_ADDR_EQUAL(&SIN6(addr)->sin6_addr,
			    &SIN6(ifap->ifa_broadaddr)->sin6_addr)) {
				freeifaddrs(ifaddrs);
				goto Broadcast6;
			}
		} else if (laddr == INADDR_BROADCAST) {
			/* 
			 * Local-wire broadcast, check for origination
			 * on this network.
			 */
			if ((SIN(ifap->ifa_addr)->sin_addr.s_addr ^
			    SIN(from)->sin_addr.s_addr) &
			    SIN(ifap->ifa_netmask)->sin_addr.s_addr)
			    continue;
			rc = BCAST;
			break;
		} else if (SIN(ifap->ifa_broadaddr)->sin_addr.s_addr ==
		    SIN(addr)->sin_addr.s_addr) {
			/*
			 * Subnet broadcast, check for origination on
			 * this network.
			 */
			if ((SIN(ifap->ifa_addr)->sin_addr.s_addr ^
			    SIN(from)->sin_addr.s_addr) &
			    SIN(ifap->ifa_netmask)->sin_addr.s_addr) {
				freeifaddrs(ifaddrs);
				goto Broadcast;
			}
			rc = BCAST;
			break;
		}
	}
	freeifaddrs(ifaddrs);

	/* Check for a local-wire broadcast w/o a local address match */
	if (laddr == INADDR_BROADCAST && rc != BCAST)
		goto Broadcast;

	/* 
	 * If this is a broadcast, only accept it if it appears to be
	 * a Sun booting.
	 */
	if (rc == BCAST) {
		int ipaddr_len;
		char *cp, *fn;
		char ipaddr[9];

		/* Only allow broadcast reads */
		if (tp->th_opcode != RRQ)
			goto Broadcast;

		ipaddr_len = snprintf(ipaddr, sizeof(ipaddr), "%08X", 
		    ntohl(SIN(from)->sin_addr.s_addr));

		/* 
		 * Verify that the initial portion of the file name is
		 * the IP address in host byte order translated into
		 * upper case hexidecimal.
		 */
		cp = fn = tp->th_stuff;
		while (cp < (char *)tp + size) {
			if (*cp == '\0')
				break;
			if (*cp == '/')
				fn = cp + 1;
			cp++;
		}
		if (*cp != '\0' || strncmp(fn, ipaddr, ipaddr_len) != 0)
			goto Broadcast;
	}	

	return (rc);
}

/*
 * Syslog fail message and exit.
 * Note that we need to drain the packet that caused inetd to start us,
 * otherwise inetd will start an infinite stream of tftpd's...
 */
static void
#if __STDC__
fail(const char *fmt, ...)
#else
fail(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	struct msghdr msghdr;
	char cbuf[1024];
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);

	if (tftpfd >= 0) {
		memset(&msghdr, 0, sizeof(msghdr));
		msghdr.msg_name = NULL;
		msghdr.msg_namelen = 0;
		msghdr.msg_iov = NULL;
		msghdr.msg_iovlen = 0;
		msghdr.msg_control = cbuf;
		msghdr.msg_controllen = sizeof(cbuf);

		recvmsg(tftpfd, &msghdr, 0);
	}

	exit(1);
}

