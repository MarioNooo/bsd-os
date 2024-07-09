/*	BSDI rcmd.c,v 2.11 2002/04/04 17:24:51 chrisk Exp	*/

/* Parts of this file are taken from... */
/*
 * Copyright (C) 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */  

/*
 * Copyright (c) 1983, 1993, 1994
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rcmd.c	8.3 (Berkeley) 3/26/94";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int __ivaliduser __P((FILE *, u_long, const char *, const char *));
int __ivaliduser_af __P((FILE *,const void *, const char *, const char *,
	int, int));
int __ivaliduser_sa __P((FILE *, const struct sockaddr *, int, const char *,
	const char *));
static int __icheckhost_sa __P((const struct sockaddr *, int, const char *));


extern char *__progname;

int
rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	u_short rport;
	const char *locuser, *remuser, *cmd;
	int *fd2p;
{
	return(rcmd_af(ahost, rport, locuser, remuser, cmd, fd2p, AF_INET));
}

int
rcmd_af(ahost, rport, locuser, remuser, cmd, fd2p, af)
	char **ahost;
	u_short rport;
	const char *locuser, *remuser, *cmd;
	int *fd2p;
{
	struct addrinfo hints, *res, *ai;
	struct sockaddr_storage from;
	struct sockaddr *fromp = (struct sockaddr *)&from;
	struct sockaddr_in sin;
	fd_set reads;
	long oldmask;
	pid_t pid;
	int s, p, lport, timo, error;
	char c;
	char num[8];
	static char cname[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
	int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	int niflags = NI_NUMERICHOST;
#endif
	char paddr[NI_MAXHOST];

	pid = getpid();
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	(void)snprintf(num, sizeof(num), "%d", ntohs(rport));
	error = getaddrinfo(*ahost, num, &hints, &res);
	if (error) {
		(void)fprintf(stderr, "%s: rcmd: %s\n",
		    __progname, gai_strerror(error));
		return(-1);
	}
	if (res->ai_canonname && strlen(res->ai_canonname) < sizeof(cname)-1)
		strncpy(cname, res->ai_canonname, sizeof(cname));
	else
		strncpy(cname, *ahost, sizeof(cname));
	*ahost = cname;

	ai = res;
	if (getnameinfo(ai->ai_addr, ai->ai_addrlen, paddr,
	    sizeof(paddr), NULL, 0, niflags) != 0)
		strcpy(paddr, "?");

	oldmask = sigblock(sigmask(SIGURG));
	for (timo = 1, lport = IPPORT_RESERVED - 1;;) {
		s = rresvport_af(&lport, ai->ai_family);
		if (s < 0) {
			if (errno == EAGAIN)
				(void)fprintf(stderr,
				    "%s: socket: All ports in use\n",
				    __progname);
			else
				(void)fprintf(stderr, "%s: socket: %s\n",
				    __progname, strerror(errno));
			sigsetmask(oldmask);
			freeaddrinfo(res);
			return (-1);
		}
		fcntl(s, F_SETOWN, pid);
		if (connect(s, ai->ai_addr, ai->ai_addrlen) >= 0)
			break;
		error = errno;
		(void)close(s);
		if (error == EADDRINUSE) {
			lport--;
			continue;
		}
		if (error == ECONNREFUSED && timo <= 16) {
			(void)sleep(timo);
			timo *= 2;
			continue;
		}
		if (ai->ai_next != NULL) {
			(void)fprintf(stderr, "%s: connect to address %s: %s\n",
			    __progname, paddr, strerror(error));
			ai = ai->ai_next;
			if (getnameinfo(ai->ai_addr, ai->ai_addrlen, paddr,
			    sizeof(paddr), NULL, 0, niflags) != 0)
				strncpy(paddr, "?", sizeof(paddr));
			(void)fprintf(stderr, "Trying %s...\n", paddr);
			continue;
		}
		(void)fprintf(stderr, "%s: %s\n", cname, strerror(error));
		sigsetmask(oldmask);
		freeaddrinfo(res);
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport_af(&lport, ai->ai_family), s3;
		int len = ai->ai_addrlen;

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void)snprintf(num, sizeof(num), "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			(void)fprintf(stderr,
			    "%s: write (setting up stderr): %s\n",
			    __progname, strerror(errno));
			(void)close(s2);
			goto bad;
		}
	again:
		FD_ZERO(&reads);
		FD_SET(s, &reads);
		FD_SET(s2, &reads);
		errno = 0;
		if (select(MAX(s, s2) + 1, &reads, 0, 0, 0) < 1 ||
		    !FD_ISSET(s2, &reads)) {
			if (errno != 0)
				(void)fprintf(stderr,
				    "%s: select (setting up stderr): %s\n",
				    __progname, strerror(errno));
			else
				(void)fprintf(stderr,
				"select: protocol failure in circuit setup\n");
			(void)close(s2);
			goto bad;
		}
		s3 = accept(s2, fromp, &len);
		switch (((struct sockaddr *)&from)->sa_family) {
		case AF_INET:
			p = ntohs(((struct sockaddr_in *)&from)->sin_port);
			break;
		case AF_INET6:
			p = ntohs(((struct sockaddr_in6 *)&from)->sin6_port);
			break;
		default:
			p = 0;	/* will cause an error */
		}
		if (p == 20) {	/* avoid FTP bounce attack */
			close(s3);
			goto again;
		}
		(void)close(s2);
		if (s3 < 0) {
			(void)fprintf(stderr,
			    "%s: accept: %s\n", __progname, strerror(errno));
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		if (p >= IPPORT_RESERVED || p < IPPORT_RESERVED / 2) {
			(void)fprintf(stderr,
			    "%s: protocol failure in circuit setup.\n",
			    __progname);
			goto bad2;
		}
	}
	(void)write(s, locuser, strlen(locuser)+1);
	(void)write(s, remuser, strlen(remuser)+1);
	(void)write(s, cmd, strlen(cmd)+1);
	errno = 0;
	if (read(s, &c, 1) != 1) {
		(void)fprintf(stderr,
		    "%s: %s: %s\n", __progname, *ahost, 
		    errno ? strerror(errno) : "connection lost during setup");
		goto bad2;
	}
	if (c == 0) {
		sigsetmask(oldmask);
		freeaddrinfo(res);
		return (s);
	}
	(void)fprintf(stderr, "%s: %s: ", __progname, *ahost);
	do {
		errno = 0;
		if (read(s, &c, 1) != 1) {
			(void)fprintf(stderr, "\n%s: %s: %s\n", __progname,
			    *ahost, errno ? strerror(errno) :
			    "connection lost during setup");
			break;
		}
		(void)fputc(c, stderr);
	} while (c != '\n');
bad2:
	if (lport)
		(void)close(*fd2p);
bad:
	(void)close(s);
	sigsetmask(oldmask);
	freeaddrinfo(res);
	return (-1);
}

int
rresvport_af(alport, family)
	int *alport;
	int family;
{
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	int s;
	u_short *sport;

	memset(&ss, 0, sizeof(ss));
	sa = (struct sockaddr *)&ss;
	switch (family) {
	case AF_INET:
		sa->sa_len = sizeof(struct sockaddr_in);
		sport = &((struct sockaddr_in *)&ss)->sin_port;
		break;
	case AF_INET6:
		sa->sa_len = sizeof(struct sockaddr_in6);
		sport = &((struct sockaddr_in6 *)&ss)->sin6_port;
		break;
	default:
		errno = EAFNOSUPPORT;
		return (-1);
	}
	sa->sa_family = family;
	s = socket(family, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		*sport = htons((u_short)*alport);
		if (bind(s, sa, sa->sa_len) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void)close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			(void)close(s);
			errno = EAGAIN;		/* close */
			return (-1);
		}
	}
}

int
rresvport(alport)
	int *alport;
{
	return(rresvport_af(alport, AF_INET));
}

extern int __check_rhosts_file;
char *__rcmd_errstr;

int
ruserok(rhost, superuser, ruser, luser)
	const char *rhost, *ruser, *luser;
	int superuser;
{
	struct hostent *hp;
	u_long addr;
	char **ap;

	if ((hp = gethostbyname(rhost)) == NULL)
		return (-1);
	for (ap = hp->h_addr_list; *ap; ++ap) {
		bcopy(*ap, &addr, sizeof(addr));
		if (iruserok(addr, superuser, ruser, luser) == 0)
			return (0);
	}
	return (-1);
}

/*
 * New .rhosts strategy: We are passed an ip address. We spin through
 * hosts.equiv and .rhosts looking for a match. When the .rhosts only
 * has ip addresses, we don't have to trust a nameserver.  When it
 * contains hostnames, we spin through the list of addresses the nameserver
 * gives us and look for a match.
 *
 * Returns 0 if ok, -1 if not ok.
 */
int
iruserok(raddr, superuser, ruser, luser)
	u_long raddr;
	int superuser;
	const char *ruser, *luser;
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);
	memcpy(&sin.sin_addr, &raddr, sizeof(sin.sin_addr));
	return(iruserok_sa((struct sockaddr *)&sin, sin.sin_len, superuser,
	    ruser, luser));
}

int
iruserok_af(raddr, superuser, ruser, luser, af)
	const void *raddr;
	int superuser;
	const char *ruser, *luser;
	int af;
{
	struct sockaddr *sa = NULL;
	struct sockaddr_in *sin = NULL;
	struct sockaddr_in6 *sin6 = NULL;
	struct sockaddr_storage ss;

	memset(&ss, 0, sizeof(ss));
	switch (af) {
	case AF_INET:
		sin = (struct sockaddr_in *)&ss;
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		memcpy(&sin->sin_addr, raddr, sizeof(sin->sin_addr));
		break;

	case AF_INET6:
		/* you will loose scope info */
		sin6 = (struct sockaddr_in6 *)&ss;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		memcpy(&sin6->sin6_addr, raddr, sizeof(sin6->sin6_addr));
		break;

	default:
		return -1;
	}
	sa = (struct sockaddr *)&ss;
	return(iruserok_sa(sa, sa->sa_len, superuser, ruser, luser));
}

int
iruserok_sa(ra, rlen, superuser, ruser, luser)
	const void *ra;
	int rlen;
	int superuser;
	const char *ruser, *luser;
{
	register char *cp;
	struct stat sb_lstat, sb_fstat;
	struct passwd *pwd;
	FILE *hostf;
	uid_t uid;
	int first, stat_rval;
	char pbuf[MAXPATHLEN];
	const struct sockaddr *raddr;
	struct sockaddr_storage ss;

	/* Avoid any problems with alignment */
	if (rlen > sizeof(ss))
		return(-1);
	memcpy(&ss, ra, rlen);
	raddr = (struct sockaddr *)&ss;

	first = 1;
	hostf = superuser ? NULL : fopen(_PATH_HEQUIV, "r");
again:
	if (hostf) {
		if (__ivaliduser_sa(hostf, raddr, rlen, luser, ruser) == 0) {
			(void)fclose(hostf);
			return (0);
		}
		(void)fclose(hostf);
	}
	if (first == 1 && (__check_rhosts_file || superuser)) {
		first = 0;
		if ((pwd = getpwnam(luser)) == NULL)
			return (-1);

		/*
		 * XXX
		 * Don't use snprintf(3), this code has to run in places
		 * where it doesn't (yet) exist.
		 */
#define	RHOSTS_NAME	"/.rhosts"
		if (strlen(pwd->pw_dir) +
		    (sizeof(RHOSTS_NAME) - 1) + 1 > sizeof(pbuf))
			return (-1);
		(void)sprintf(pbuf, "%s%s", pwd->pw_dir, RHOSTS_NAME);

		/*
		 * Change effective uid while opening and stat'ing .rhosts.
		 * If root and reading an NFS mounted file system that isn't
		 * mapping root's ID, we won't be able to read files that
		 * aren't world accessible, and .rhosts files are supposed to
		 * be protected read/write owner only.  Also, check that the
		 * file we opened is the one that we stat'ed -- there's a race.
		 */
		uid = geteuid();
		(void)seteuid(pwd->pw_uid);
		if ((hostf = fopen(pbuf, "r")) != NULL) {
			stat_rval = lstat(pbuf, &sb_lstat);
			if (fstat(fileno(hostf), &sb_fstat) || 
			    sb_lstat.st_dev != sb_fstat.st_dev ||
			    sb_lstat.st_ino != sb_fstat.st_ino)
				stat_rval = -1;
		}
		(void)seteuid(uid);

		if (hostf == NULL)
			return (-1);
		/*
		 * If not a regular file, or is owned by someone other than
		 * user or root or if writeable by anyone but the owner, quit.
		 */
		cp = NULL;
		if (stat_rval < 0)
			cp = ".rhosts stat failed";
		else if (!S_ISREG(sb_lstat.st_mode))
			cp = ".rhosts not regular file";
		else if (sb_fstat.st_uid && sb_fstat.st_uid != pwd->pw_uid)
			cp = "bad .rhosts owner";
		else if (sb_fstat.st_mode & (S_IWGRP|S_IWOTH))
			cp = ".rhosts writeable by other than owner";
		/* If there were any problems, quit. */
		if (cp) {
			__rcmd_errstr = cp;
			(void)fclose(hostf);
			return (-1);
		}
		goto again;
	}
	return (-1);
}

/*
 * XXX Don't make static, used by lpd(8).
 * XXX Should document this or move it into lpd's sources.
 * XXX Should change signature to make raddr a struct in_addr.
 *
 * Returns 0 if ok, -1 if not ok.
 */
int
__ivaliduser(hostf, raddr, luser, ruser)
	FILE *hostf;
	u_long raddr;
	const char *luser, *ruser;
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(struct sockaddr_in);
	memcpy(&sin.sin_addr, &raddr, sizeof(sin.sin_addr));
	return __ivaliduser_sa(hostf, (struct sockaddr *)&sin, sin.sin_len,
		luser, ruser);
}

int
__ivaliduser_sa(hostf, raddr, rlen, luser, ruser)
	FILE *hostf;
	const struct sockaddr *raddr;
	int rlen;
	const char *luser, *ruser;
{
	register char *user, *p;
	int ch;
	char buf[MAXHOSTNAMELEN + 128];		/* host + login */

	while (fgets(buf, sizeof(buf), hostf)) {
		p = buf;
		/* Skip lines that are too long. */
		if (strchr(p, '\n') == NULL) {
			while ((ch = getc(hostf)) != '\n' && ch != EOF);
			continue;
		}
		if (*p == '#')
			continue;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' &&
			    *p != '\t' && *p != '\0')
				p++;
		} else
			user = p;
		*p = '\0';
		if (__icheckhost_sa(raddr, rlen, buf) &&
		    strcmp(ruser, *user ? user : luser) == 0) {
			return (0);
		}
	}
	return (-1);
}

/*
 * Returns "true" if match, 0 if no match.
 */
static int
__icheckhost_sa(raddr, rlen, lhost)
	const struct sockaddr *raddr;
	int rlen;
	const char *lhost;
{
	register struct hostent *hp;
	struct in_addr laddr;
	register char **pp;
	struct addrinfo hints, *res0, *res;
	int match;
	char h1[NI_MAXHOST];
	struct sockaddr_storage ss;
#ifdef NI_WITHSCOPEID
	const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflags = NI_NUMERICHOST;
#endif


	/* Normalize *raddr: convert to numeric name and back to sockaddr */
	if (getnameinfo(raddr, rlen, h1, sizeof(h1), NULL, 0, niflags) != 0)
		return(0);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = raddr->sa_family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST;
	if (getaddrinfo(h1, NULL, &hints, &res0) != 0)
		return(0);
	bcopy(res0->ai_addr, &ss, res0->ai_addrlen);
	freeaddrinfo(res0);
	
	/* Now, get the list of addresses associated with "lhost" */
	hints.ai_flags = 0;
	if (getaddrinfo(lhost, NULL, &hints, &res0) != 0)
		return (0);

	/* Spin through the addresses. */
	match = 0;
	for (res = res0; res; res = res->ai_next) {
		if (res->ai_family != raddr->sa_family)
			continue;
		if (res->ai_addrlen != rlen)
			continue;
		if (bcmp(res->ai_addr, &ss, rlen) == 0) {
			match = 1;
			break;
		}
	}
	freeaddrinfo(res0);
	return (match);
}
