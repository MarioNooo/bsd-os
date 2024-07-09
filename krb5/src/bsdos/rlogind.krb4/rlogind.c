/*	BSDI rlogind.c,v 2.20 2000/11/17 19:56:33 richards Exp	*/

/*-
 * Copyright (c) 1983, 1988, 1989, 1993
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
"@(#) Copyright (c) 1983, 1988, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rlogind.c	8.2 (Berkeley) 4/28/95";
#endif /* not lint */

/*
 * remote login server:
 *	\0
 *	remuser\0
 *	locuser\0
 *	terminal_type/speed\0
 *	data
 */

#define	FD_SETSIZE	16		/* don't need many bits for select */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <utmp.h>
#include "pathnames.h"

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif

#ifdef KERBEROS
#include <kerberosIV/krb.h>
#include "krb.h"

#ifdef	CRYPT
#define	SECURE_MESSAGE "This rlogin session is using DES encryption for all transmissions.\r\n"

static Key_schedule	schedule;
#endif	/* CRYPT */

static AUTH_DAT	*kdata;
static KTEXT	ticket;
static u_char	auth_buf[sizeof(AUTH_DAT)];
static u_char	tick_buf[sizeof(KTEXT_ST)];

static int	doencrypt, retval, use_kerberos, vacuous;

#define		ARGSTR			"alnkvx"
#else
#define		ARGSTR			"aln"
#endif

char	*env[2];
#define	NMAX 30
char	lusername[NMAX+1], rusername[NMAX+1];
static	char term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */
int	keepalive = 1;
int	check_all = 0;

struct	passwd *pwd;

void	doit __P((int, struct sockaddr *));
int	control __P((int, char *, int));
void	protocol __P((int, int));
void	cleanup __P((int));
void	fatal __P((int, char *, int));
int	do_rlogin __P((struct sockaddr *));
void	getstr __P((char *, int, char *));
void	setup_term __P((int));
#ifdef KERBEROS
int	do_krb_login __P((struct sockaddr *));
#ifdef CRYPT
void	des_save_key __P((C_Block, Key_schedule));
#endif
#endif
void	usage __P((void));
int	local_domain __P((char *));
char	*topdomain __P((char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int __check_rhosts_file;
	struct sockaddr_storage from;
	int ch, fromlen, on;

	openlog(NULL, LOG_PID | LOG_CONS, LOG_AUTH);

	opterr = 0;
	while ((ch = getopt(argc, argv, ARGSTR)) != EOF)
		switch (ch) {
		case 'a':
			check_all = 1;
			break;
		case 'l':
			__check_rhosts_file = 0;
			break;
		case 'n':
			keepalive = 0;
			break;
#ifdef KERBEROS
		case 'k':
			use_kerberos = 1;
			break;
		case 'v':
			vacuous = 1;
			break;
		case 'x':
			doencrypt = 1;
			break;
#endif
		case '?':
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

#ifdef KERBEROS
	if (use_kerberos && vacuous) {
		usage();
		fatal(STDERR_FILENO, "only one of -k and -v allowed", 0);
	}
#endif

	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR,"Can't get peer name of remote host: %m");
		fatal(STDERR_FILENO, "Can't get peer name of remote host", 1);
	}
	if (((struct sockaddr *)&from)->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6;
		struct sockaddr_in sin;

		sin6 = (struct sockaddr_in6 *)&from;
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_len = sizeof(struct sockaddr_in);
			sin.sin_port = sin6->sin6_port;
			memcpy(&sin.sin_addr, &sin6->sin6_addr.s6_addr[12],
				sizeof(struct in_addr));
			memcpy(&from, &sin, sin.sin_len);
		}
	}
#ifdef KERBEROS
	if (use_kerberos &&
	    (((struct sockaddr *)&from)->sa_family == AF_INET6))
		fatal(STDERR_FILENO, "Kerberos is not supported for IPv6", 0);
#endif
	on = 1;
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	if (((struct sockaddr*)&from)->sa_family == AF_INET) {
		on = IPTOS_LOWDELAY;
		if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&on,
				sizeof(int)) < 0) {
			syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
		}
	}
	doit(0, (struct sockaddr *)&from);
	/*NOTREACHED*/
	exit(1);
}

int	child;
int	netf;
char	line[MAXPATHLEN];
int	confirmed;

struct winsize win = { 0, 0, 0, 0 };


void
doit(f, fromp)
	int f;
	struct sockaddr *fromp;
{
	int len, master, pid, on = 1;
	int authenticated = 0;
	register struct hostent *hp;
	char hostname[MAXHOSTNAMELEN + 64];	/* hostname + [ IP addr ] */
	char ut_hostname[UT_HOSTSIZE];
	char num_hostname[NI_MAXHOST];
	char str_hostname[NI_MAXHOST];
	char c;
	u_short *portp;

	alarm(60);
	read(f, &c, 1);

	if (c != 0)
		exit(1);
#ifdef KERBEROS
	if (vacuous)
		fatal(f, "Remote host requires Kerberos authentication", 0);
	if (use_kerberos && krb_configured() == KFAILURE)
		fatal(f, "Kerberos is not configured", 0);
#ifndef	CRYPT
	if (doencrypt)
		fatal(f, "Encryption is not available", 0);
#endif
#endif

	alarm(0);
	switch (fromp->sa_family) {
	case AF_INET:
		portp = &((struct sockaddr_in *)fromp)->sin_port;
		break;
	case AF_INET6:
		portp = &((struct sockaddr_in6 *)fromp)->sin6_port;
		break;
	default:
		fatal(f, "Unsupported address family", 0);
	}
	if (getnameinfo(fromp, fromp->sa_len, str_hostname,
	    sizeof(str_hostname), NULL, 0, 0) == 0)
		len = strlen(num_hostname);
	else
		len = 0;

	if (getnameinfo(fromp, fromp->sa_len, num_hostname,
	    sizeof(num_hostname), NULL, 0, NI_NUMERICHOST) != 0)
		strcpy(num_hostname, "(invalid)");

	if (len > 0 && len < MAXHOSTNAMELEN)
		(void)snprintf(hostname, sizeof(hostname), "%s[%s]",
		    str_hostname, num_hostname);
	else
		(void)strncpy(hostname, num_hostname, sizeof(hostname));

	if (len > 0 && len <= sizeof(ut_hostname))
		(void)strncpy(ut_hostname, str_hostname, sizeof(ut_hostname));
	else
		(void)strncpy(ut_hostname, num_hostname, sizeof(ut_hostname));

#ifdef KERBEROS
	if (use_kerberos) {
		retval = do_krb_login(fromp);
		if (retval == 0)
			authenticated++;
		else if (retval > 0) {
			syslog(LOG_AUTH|LOG_NOTICE,
			    "Kerberos auth failure from %s: %s",
			    hostname, krb_err_txt[retval]);
			fatal(f, krb_err_txt[retval], 0);
		}
		write(f, &c, 1);
		confirmed = 1;		/* we sent the null! */
	} else 
#endif
		{
		if (fromp->sa_family != AF_INET ||
		    ntohs(*portp) >= IPPORT_RESERVED ||
		    ntohs(*portp) < IPPORT_RESERVED/2) {
			syslog(LOG_NOTICE, "Connection from %s on illegal port",
				num_hostname);
			fatal(f, "Permission denied", 0);
		}

#ifdef IP_OPTIONS
		{
		/*
		 * Length of output buffer needs to be at least:
		 *      strlen("strict routed via: ")
		 *      strlen("xxx.xxx.xxx.xxx")
		 *      max_IP_addr * strlen(" xxx.xxx.xxx.xxx")
		 * 19 + 15 + (40-4)/4 * 16 = 177
		 */
		char lbuf[256], *lp;
		struct ip_opts optbuf;
		u_char *cp;
		int optsize = sizeof(optbuf), ipproto, optlen;
		struct protoent *ip;
		if ((ip = getprotobyname("ip")) != NULL)
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
#ifdef KERBEROS
		if (!use_kerberos)
#endif
		if (getsockopt(0, ipproto, IP_OPTIONS,
		    (char *)&optbuf, &optsize) == 0 && optsize != 0) {
			lp = lbuf;
			optsize -= sizeof(optbuf.ip_dst);
			for (cp = (u_char *)optbuf.ip_opts; optsize > 0; ) {
				switch (*cp) {

				case IPOPT_LSRR:
				case IPOPT_SSRR:
					optlen = cp[1];
					lp += sprintf(lp, "%srouted via: %s",
					   (*cp == IPOPT_SSRR) ? "strict " : "",
					    inet_ntoa(optbuf.ip_dst));
					cp += 3;
					if (optlen > optsize)
						optlen = optsize;
					optsize -= optlen;
					optlen -= 3;
					while (optlen > 0) {
						lp += sprintf(lp,
						    " %u.%u.%u.%u",
						    cp[0], cp[1], cp[2], cp[3]);
						cp += 4;
						optlen -= 4;
					}
					break;

				case IPOPT_EOL:
					optsize = 0;	/* break out of for() */
					break;

				default:
					optlen = (*cp == IPOPT_NOP) ?
					    1 : *(cp+1);
					cp += optlen;
					optsize -= optlen;
					break;
				}
			}
			if (lp != lbuf) {
				syslog(LOG_AUTH|LOG_NOTICE, "IP Source Route "
				    "option on connection apparently from %s, "
				    "connection dropped: %s", hostname, lbuf);
				exit(1);
			}
		}
		}
#endif
		if (do_rlogin(fromp) == 0)
			authenticated++;
	}
	if (confirmed == 0) {
		write(f, "", 1);
		confirmed = 1;		/* we sent the null! */
	}
#ifdef	CRYPT
	if (doencrypt)
		(void) des_write(f, SECURE_MESSAGE, sizeof(SECURE_MESSAGE) - 1);
#endif
	netf = f;

	if (lusername[0] == '-') {
		syslog(LOG_WARNING, "user name of ``%s'' from %s",
		    lusername, hostname);
		fatal(f, "user names must not start with a -", 0);
	}

	pid = forkpty(&master, line, NULL, &win);
	if (pid < 0) {
		if (errno == ENOENT)
			fatal(f, "Out of ptys", 0);
		else
			fatal(f, "Forkpty", 1);
	}
	if (pid == 0) {
		if (f > 2)	/* f should always be 0, but... */
			(void) close(f);
		setup_term(0);
		if (authenticated) {
#ifdef KERBEROS
			if (use_kerberos && (pwd->pw_uid == 0))
				syslog(LOG_INFO|LOG_AUTH,
				    "ROOT Kerberos login from %s.%s@%s on %s\n",
				    kdata->pname, kdata->pinst, kdata->prealm,
				    hostname);
#endif

			execle(_PATH_LOGIN, "login", "-p", "-B",
			    "-h", ut_hostname, "-f", "--", lusername, NULL, env);
		} else
			execle(_PATH_LOGIN, "login", "-p", "-B",
			    "-h", ut_hostname, "--", lusername, NULL, env);
		fatal(STDERR_FILENO, _PATH_LOGIN, 1);
		/*NOTREACHED*/
	}
#ifdef	CRYPT
	/*
	 * If encrypted, don't turn on NBIO or the des read/write
	 * routines will croak.
	 */

	if (!doencrypt)
#endif
		ioctl(f, FIONBIO, &on);
	ioctl(master, FIONBIO, &on);
	ioctl(master, TIOCPKT, &on);
	signal(SIGCHLD, cleanup);
	protocol(f, master);
	signal(SIGCHLD, SIG_IGN);
	cleanup(0);
}

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
int
control(pty, cp, n)
	int pty;
	char *cp;
	int n;
{
	struct winsize w;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */
	memmove(&w, cp+4, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	(void)ioctl(pty, TIOCSWINSZ, &w);
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
void
protocol(f, p)
	register int f, p;
{
	char pibuf[1024+1], fibuf[1024], *pbp, *fbp;
	register pcc = 0, fcc = 0;
	int cc, nfd, n;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	if (f > p)
		nfd = f + 1;
	else
		nfd = p + 1;
	if (nfd > FD_SETSIZE) {
		syslog(LOG_ERR, "select mask too small, increase FD_SETSIZE");
		fatal(f, "internal error (select mask too small)", 0);
	}
	for (;;) {
		fd_set ibits, obits, ebits, *omask;

		FD_ZERO(&ebits);
		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		omask = (fd_set *)NULL;
		if (fcc) {
			FD_SET(p, &obits);
			omask = &obits;
		} else
			FD_SET(f, &ibits);
		if (pcc >= 0)
			if (pcc) {
				FD_SET(f, &obits);
				omask = &obits;
			} else
				FD_SET(p, &ibits);
		FD_SET(p, &ebits);
		if ((n = select(nfd, &ibits, omask, &ebits, 0)) < 0) {
			if (errno == EINTR)
				continue;
			fatal(f, "select", 1);
		}
		if (n == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (FD_ISSET(p, &ebits)) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					FD_CLR(p, &ibits);
				}
			}
		}
		if (FD_ISSET(f, &ibits)) {
#ifdef	CRYPT
			if (doencrypt)
				fcc = des_read(f, fibuf, sizeof(fibuf));
			else
#endif
				fcc = read(f, fibuf, sizeof(fibuf));
			if (fcc < 0 && errno == EWOULDBLOCK)
				fcc = 0;
			else {
				register char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								bcopy(cp+n, cp, left);
							fcc -= n;
							goto top; /* n^2 */
						}
					}
				FD_SET(p, &obits);		/* try write */
			}
		}

		if (FD_ISSET(p, &obits) && fcc > 0) {
			cc = write(p, fbp, fcc);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (FD_ISSET(p, &ibits)) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else if (pcc <= 0)
				break;
			else if (pibuf[0] == 0) {
				pbp++, pcc--;
#ifdef	CRYPT
				if (!doencrypt)
#endif
					FD_SET(f, &obits);	/* try write */
			} else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
				}
				pcc = 0;
			}
		}
		if ((FD_ISSET(f, &obits)) && pcc > 0) {
#ifdef	CRYPT
			if (doencrypt)
				cc = des_write(f, pbp, pcc);
			else
#endif
				cc = write(f, pbp, pcc);
			if (cc < 0 && errno == EWOULDBLOCK) {
				/*
				 * This happens when we try write after read
				 * from p, but some old kernels balk at large
				 * writes even when select returns true.
				 */
				if (!FD_ISSET(p, &ibits))
					sleep(5);
				continue;
			}
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

void
cleanup(signo)
	int signo;
{
	char *p;

	p = line + sizeof(_PATH_DEV) - 1;
	if (logout(p))
		logwtmp(p, "", "");
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	*p = 'p';
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	shutdown(netf, 2);
	exit(1);
}

void
fatal(f, msg, syserr)
	int f;
	char *msg;
	int syserr;
{
	int len;
	char buf[BUFSIZ], *bp = buf;

	/*
	 * Prepend binary one to message if we haven't sent
	 * the magic null as confirmation.
	 */
	if (!confirmed)
		*bp++ = '\01';		/* error indicator */
	if (syserr)
		len = sprintf(bp, "rlogind: %s: %s.\r\n",
		    msg, strerror(errno));
	else
		len = sprintf(bp, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, bp + len - buf);
	exit(1);
}

int
do_rlogin(dest)
	struct sockaddr *dest;
{
	u_char *addr;

	getstr(rusername, sizeof(rusername), "remuser too long");
	getstr(lusername, sizeof(lusername), "locuser too long");
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type too long");

	pwd = getpwnam(lusername);
	if (pwd == NULL)
		return (-1);
	/*
	 * Silly check for root, but maybe pigs can sing...
	 */
	if (pwd->pw_uid == 0)
		return (-1);

	/* XXX why don't we syslog() failure? */
	switch (dest->sa_family) {
	case AF_INET:
		addr = (u_char *)&((struct sockaddr_in *)dest)->sin_addr;
		break;
	case AF_INET6:
		addr = (u_char *)&((struct sockaddr_in6 *)dest)->sin6_addr;
		break;
	default:
		return (-1);
	}
	return (iruserok_af(addr, pwd->pw_uid == 0, rusername,
	    lusername, dest->sa_family));
}

void
getstr(buf, cnt, errmsg)
	char *buf;
	int cnt;
	char *errmsg;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		if (--cnt < 0)
			fatal(STDOUT_FILENO, errmsg, 0);
		*buf++ = c;
	} while (c != 0);
}

void
setup_term(fd)
	int fd;
{
	register char *cp = index(term+ENVSIZE, '/');
	char *speed;
	struct termios tt;

	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = index(speed, '/');
		if (cp)
			*cp++ = '\0';
		tcgetattr(fd, &tt);
		cfsetspeed(&tt, atoi(speed));
		tcsetattr(fd, TCSAFLUSH, &tt);
	}

	env[0] = term;
	env[1] = 0;
}

#define	VERSION_SIZE	9

#ifdef KERBEROS
/*
 * Do the remote kerberos login to the named host with the
 * given inet address
 *
 * Return 0 on valid authorization
 * Return -1 on valid authentication, no authorization
 * Return >0 for error conditions
 */
int
do_krb_login(dest)
	struct sockaddr *dest;
{
	int rc;
	char instance[INST_SZ], version[VERSION_SIZE];
	long authopts = 0L;	/* !mutual */
#ifdef	CRYPT
	struct sockaddr_in faddr;
#endif	/* CRYPT */

	kdata = (AUTH_DAT *) auth_buf;
	ticket = (KTEXT) tick_buf;

	k_getsockinst(0, instance);

#ifdef	CRYPT
	if (doencrypt) {
		rc = sizeof(faddr);
		if (getsockname(0, (struct sockaddr *)&faddr, &rc))
			return (-1);
		authopts = KOPT_DO_MUTUAL;
		rc = krb_recvauth(
			authopts, 0,
			ticket, "rcmd",
			instance, (struct sockaddr_in *)dest, &faddr,
			kdata, "", schedule, version);
		 des_save_key(kdata->session, schedule);

	} else
#endif
		rc = krb_recvauth(
			authopts, 0,
			ticket, "rcmd",
			instance, (struct sockaddr_in *)dest,
			(struct sockaddr_in *) 0,
			kdata, "", (struct des_ks_struct *) 0, version);

	if (rc != KSUCCESS)
		return (rc);

	getstr(lusername, sizeof(lusername), "locuser");
	/* get the "cmd" in the rcmd protocol */
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type");

	pwd = getpwnam(lusername);
	if (pwd == NULL)
		return (-1);

	/* returns nonzero for no access */
	if (kuserok(kdata, lusername) != 0)
		return (-1);
	
	return (0);

}
#endif

void
usage()
{
#ifdef KERBEROS
#ifdef CRYPT
	syslog(LOG_ERR, "usage: rlogind [-alnx] [-k|-v]");
#else
	syslog(LOG_ERR, "usage: rlogind [-aln] [-k|-v]");
#endif
#else
	syslog(LOG_ERR, "usage: rlogind [-aln]");
#endif
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
int
local_domain(h)
	char *h;
{
	char localhost[MAXHOSTNAMELEN];
	char *p1, *p2;

	localhost[0] = 0;
	(void) gethostname(localhost, sizeof(localhost));
	p1 = topdomain(localhost);
	p2 = topdomain(h);
	if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
		return (1);
	return (0);
}

char *
topdomain(h)
	char *h;
{
	register char *p;
	char *maybe = NULL;
	int dots = 0;

	for (p = h + strlen(h); p >= h; p--) {
		if (*p == '.') {
			if (++dots == 2)
				return (p);
			maybe = p;
		}
	}
	return (maybe);
}
