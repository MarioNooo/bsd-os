/*	BSDI rwhod.c,v 2.5 1996/08/23 16:50:18 dab Exp	*/

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
static char sccsid[] = "@(#)rwhod.c	8.1 (Berkeley) 6/6/93 plus MULTICAST 1.2";
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <protocols/rwhod.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <utmp.h>

/*
 * This version of Berkeley's rwhod has been modified to use IP multicast
 * datagrams, under control of new command-line options:
 *
 *	rwhod -m	causes rwhod to use IP multicast (instead of
 *			broadcast or unicast) on all interfaces that have
 *			the IFF_MULTICAST flag set in their "ifnet" structs
 *			(excluding the loopback interface).  The multicast
 *			reports are sent with a time-to-live of 1, to prevent
 *			forwarding beyond the directly-connected subnet(s).
 *
 *	rwhod -M <ttl>	causes rwhod to send IP multicast datagrams with a
 *			time-to-live of <ttl>, via a SINGLE interface rather
 *			than all interfaces.  <ttl> must be between 0 and
 *			MAX_MULTICAST_SCOPE, defined below.  Note that "-M 1"
 *			is different than "-m", in that "-M 1" specifies
 *			transmission on one interface only; the two modes
 *			are exclusive.
 *
 *
 * When "-m" is used, the program accepts multicast rwhod reports from all
 * multicast-capable interfaces.  If a <ttl> argument is given, it accepts
 * multicast reports from only one interface, the one on which reports are
 * sent (which may be controlled via the host's routing table).  Regardless
 * of options, the program accepts broadcast or unicast reports from
 * all interfaces.  Thus, this program will hear the reports of old,
 * non-multicasting rwhods, but, if multicasting is used, those old rwhods
 * won't hear the reports generated by this program.
 *
 *                  -- Steve Deering, Stanford University, February 1989
 */

#define NO_MULTICAST		0	  /* multicast modes */
#define PER_INTERFACE_MULTICAST	1
#define SCOPED_MULTICAST	2

#define MAX_MULTICAST_SCOPE	32	  /* "site-wide", by convention */

#define INADDR_WHOD_GROUP (u_long)0xe0000103      /* 224.0.1.3 */
					  /* (belongs in protocols/rwhod.h) */

int			multicast_mode  = NO_MULTICAST;
int			multicast_scope;
struct sockaddr_in	multicast_addr;

/*
 * Alarm interval. Don't forget to change the down time check in ruptime
 * if this is changed.
 */
#define AL_INTERVAL (3 * 60)

char	myname[MAXHOSTNAMELEN];

/*
 * We communicate with each neighbor in a list constructed at the time we're
 * started up.  Neighbors are currently directly connected via a hardware
 * interface.
 */
struct	neighbor {
	struct	neighbor *n_next;
	char	*n_name;		/* interface name */
	struct	sockaddr *n_addr;	/* who to send to */
	int	n_flags;		/* should forward?, interface flags */
};

struct	neighbor *neighbors;
struct	whod mywd;
struct	servent *sp;
int	s, utmpf;

#define	WHDRSIZE	(sizeof(mywd) - sizeof(mywd.wd_we))

int	 configure __P((int));
void	 getboottime __P((int));
void	 onalrm __P((int));
void	 quit __P((char *));
int	 verify __P((char *));

#ifdef DEBUG
char	*interval __P((int, const char *));
void	prsend __P((int, const void *, int, int, const struct sockaddr *, int));
#define	sendto prsend
#endif

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, on, debug;
	char *cp;
	struct stat st;
	struct sockaddr_in from, sin;
	char path[64];

	if (getuid()) {
		fprintf(stderr, "rwhod: not super user\n");
		exit(1);
	}
	debug = 0;
	while ((ch = getopt(argc, argv, "dmM:")) != EOF) {
		switch (ch) {

		case 'd':
			debug = 1;
			break;

		case 'm':
			multicast_mode = PER_INTERFACE_MULTICAST;
			break;

		case 'M':
			multicast_mode  = SCOPED_MULTICAST;
			multicast_scope = strtol(optarg, &cp, 10);
			if (cp == optarg || *cp ||
			    (u_int)multicast_scope > MAX_MULTICAST_SCOPE) {
				fprintf(stderr,
				    "rwhod: ttl must not exceed %u\n",
				    MAX_MULTICAST_SCOPE);
				exit(1);
			}
			break;

		default:
			goto usage;
		}
	}
	if (optind < argc) {
usage:
		fprintf(stderr, "usage: rwhod [ -d ] [ -m | -M ttl ]\n");
		exit(1);
	}

	/* from now on, all errors go via syslog only */
	openlog("rwhod", LOG_PID, debug ? LOG_DAEMON|LOG_PERROR : LOG_DAEMON);
	sp = getservbyname("who", "udp");
	if (sp == NULL) {
		fprintf(stderr, "rwhod: udp/who: unknown service\n");
		quit("udp/who: unknown service");
	}
	if (!debug)
		daemon(1, 0);
	if (chdir(_PATH_RWHODIR) < 0) {
		quit(_PATH_RWHODIR ": %m");
	}

	/* XXX catching SIGHUP no longer has any function */
	(void) signal(SIGHUP, getboottime);

	/*
	 * Establish host name as returned by system.
	 */
	if (gethostname(myname, sizeof(myname) - 1) < 0) {
		quit("gethostname: %m");
	}
	if ((cp = strchr(myname, '.')) != NULL)
		*cp = '\0';
	strncpy(mywd.wd_hostname, myname, sizeof(mywd.wd_hostname) - 1);
	utmpf = open(_PATH_UTMP, O_RDONLY|O_CREAT, 0644);
	if (utmpf < 0) {
		quit(_PATH_UTMP ": %m");
	}
	getboottime(0);
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		quit("socket: %m");
	}
	on = 1;
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		quit("setsockopt SO_BROADCAST: %m");
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	multicast_addr.sin_family = AF_INET;
	sin.sin_port = sp->s_port;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		quit("bind: %m");
	}
	if (!configure(s))
		exit(1);
	signal(SIGALRM, onalrm);
	onalrm(0);
	for (;;) {
		struct whod wd;
		int cc, whod, len = sizeof(from);

		cc = recvfrom(s, (char *)&wd, sizeof(struct whod), 0,
			(struct sockaddr *)&from, &len);
		if (cc <= 0) {
			if (cc < 0 && errno != EINTR)
				syslog(LOG_WARNING, "recv: %m");
			continue;
		}
		if (from.sin_port != sp->s_port) {
			syslog(LOG_WARNING, "%d: bad from port",
				ntohs(from.sin_port));
			continue;
		}
		if (wd.wd_vers != WHODVERSION)
			continue;
		if (wd.wd_type != WHODTYPE_STATUS)
			continue;
		/* Blindly force NULL termination of wd.wd_hostname */
		wd.wd_hostname[sizeof(wd.wd_hostname)-1] = '\0';
		if (!verify(wd.wd_hostname)) {
			syslog(LOG_WARNING, "malformed host name from %x",
				from.sin_addr);
			continue;
		}
		(void) snprintf(path, sizeof(path), "whod.%s", wd.wd_hostname);
		/*
		 * Rather than truncating and growing the file each time,
		 * use ftruncate if size is less than previous size.
		 */
		whod = open(path, O_WRONLY | O_CREAT, 0644);
		if (whod < 0) {
			syslog(LOG_WARNING, "%s: %m", path);
			continue;
		}
#if BYTE_ORDER != BIG_ENDIAN
		{
			int i, n = (cc - WHDRSIZE)/sizeof(struct whoent);
			struct whoent *we;

			/* undo header byte swapping before writing to file */
			wd.wd_sendtime = ntohl(wd.wd_sendtime);
			for (i = 0; i < 3; i++)
				wd.wd_loadav[i] = ntohl(wd.wd_loadav[i]);
			wd.wd_boottime = ntohl(wd.wd_boottime);
			we = wd.wd_we;
			for (i = 0; i < n; i++) {
				we->we_idle = ntohl(we->we_idle);
				we->we_utmp.out_time =
				    ntohl(we->we_utmp.out_time);
				we++;
			}
		}
#endif
		(void) time((time_t *)&wd.wd_recvtime);
		(void) write(whod, (char *)&wd, cc);
		if (fstat(whod, &st) < 0 || st.st_size > cc)
			ftruncate(whod, cc);
		(void) close(whod);
	}
}

/*
 * Check out host name for unprintables
 * and other funnies before allowing a file
 * to be created.  Sorry, but blanks aren't allowed.
 */
int
verify(name)
	register char *name;
{
	register int size = 0;

	while (*name) {
		if (!isascii(*name) || !(isalnum(*name) || ispunct(*name)))
			return (0);
		name++, size++;
	}
	return (size > 0);
}

int	utmptime;
int	utmpent;
int	utmpsize = 0;
struct	utmp *utmp;

void
onalrm(signo)
	int signo;
{
	register struct neighbor *np;
	register struct whoent *we = mywd.wd_we, *wlast;
	register int i;
	struct stat stb;
	double avenrun[3];
	time_t now;
	int cc;

	now = time(NULL);
	(void) fstat(utmpf, &stb);
	if ((stb.st_mtime != utmptime) || (stb.st_size > utmpsize)) {
		utmptime = stb.st_mtime;
		if (stb.st_size > utmpsize) {
			utmpsize = stb.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (utmp == NULL) {
				fprintf(stderr, "rwhod: malloc failed\n");
				utmpsize = 0;
				goto done;
			}
		}
		(void) lseek(utmpf, (off_t)0, L_SET);
		cc = read(utmpf, (char *)utmp, stb.st_size);
		if (cc < 0) {
			fprintf(stderr, "rwhod: %s: %s\n",
			    _PATH_UTMP, strerror(errno));
			goto done;
		}
		wlast = &mywd.wd_we[1024 / sizeof(struct whoent) - 1];
		utmpent = cc / sizeof(struct utmp);
		for (i = 0; i < utmpent; i++)
			if (utmp[i].ut_name[0]) {
				memcpy(we->we_utmp.out_line, utmp[i].ut_line,
				   sizeof(utmp[i].ut_line));
				memcpy(we->we_utmp.out_name, utmp[i].ut_name,
				   sizeof(utmp[i].ut_name));
				we->we_utmp.out_time = htonl(utmp[i].ut_time);
				if (we >= wlast)
					break;
				we++;
			}
		utmpent = we - mywd.wd_we;
	}

	/*
	 * The test on utmpent looks silly---after all, if no one is
	 * logged on, why worry about efficiency?---but is useful on
	 * (e.g.) compute servers.
	 */
	if (utmpent && chdir(_PATH_DEV)) {
		quit("chdir(" _PATH_DEV "): %m");
	}
	we = mywd.wd_we;
	for (i = 0; i < utmpent; i++) {
		if (stat(we->we_utmp.out_line, &stb) >= 0)
			we->we_idle = htonl(now - stb.st_atime);
		we++;
	}
	(void)getloadavg(avenrun, sizeof(avenrun)/sizeof(avenrun[0]));
	for (i = 0; i < 3; i++)
		mywd.wd_loadav[i] = htonl((u_long)(avenrun[i] * 100));
	cc = (char *)we - (char *)&mywd;
	mywd.wd_sendtime = htonl(time(0));
	mywd.wd_vers = WHODVERSION;
	mywd.wd_type = WHODTYPE_STATUS;
	if (multicast_mode == SCOPED_MULTICAST)
		(void)sendto(s, (char *)&mywd, cc, 0,
		    (struct sockaddr *)&multicast_addr, sizeof multicast_addr);
	else for (np = neighbors; np != NULL; np = np->n_next) {
		if (multicast_mode == PER_INTERFACE_MULTICAST &&
		    np->n_flags & IFF_MULTICAST) {
			/*
			 * Select the outgoing interface for the multicast.
			 */
			if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
			    &(((struct sockaddr_in *)np->n_addr)->sin_addr),
			    sizeof(struct in_addr)) < 0) {
				quit("setsockopt IP_MULTICAST_IF: %m");
			}
			(void)sendto(s, (char *)&mywd, cc, 0,
			    (struct sockaddr *)&multicast_addr,
			    sizeof multicast_addr);
		} else
			(void)sendto(s, (char *)&mywd, cc, 0,
			    np->n_addr, np->n_addr->sa_len);
	}
	if (utmpent && chdir(_PATH_RWHODIR)) {
		quit("chdir(" _PATH_RWHODIR "): %m");
	}
done:
	(void) alarm(AL_INTERVAL);
}

void
getboottime(signo)
	int signo;
{
	int mib[2];
	size_t size;
	struct timeval tm;

	mib[0] = CTL_KERN;
	mib[1] = KERN_BOOTTIME;
	size = sizeof(tm);
	if (sysctl(mib, 2, &tm, &size, NULL, 0) == -1) {
		quit("cannot get boottime: %m");
	}
	mywd.wd_boottime = htonl(tm.tv_sec);
}

void
quit(msg)
	char *msg;
{
	syslog(LOG_ERR, msg);
	exit(1);
}

/*
 * Figure out device configuration and select
 * networks which deserve status information.
 */
int
configure(s)
	int s;
{
	register struct neighbor *np;
	struct sockaddr *naddr;
	struct ifaddrs *ifaddrs, *ifa;

	if (multicast_mode == SCOPED_MULTICAST) {
		struct ip_mreq mreq;
		unsigned char ttl;

		mreq.imr_multiaddr.s_addr = htonl(INADDR_WHOD_GROUP);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			    &mreq, sizeof mreq) < 0) {
			syslog(LOG_ERR, "setsockopt IP_ADD_MEMBERSHIP: %m");
			return (0);
		}
		ttl = multicast_scope;
		if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
			    &ttl, sizeof ttl) < 0) {
			syslog(LOG_ERR, "setsockopt IP_MULTICAST_TTL: %m");
			return (0);
		}
		multicast_addr.sin_addr.s_addr = htonl(INADDR_WHOD_GROUP);
		multicast_addr.sin_port = sp->s_port;
		return (1);
	}

	if (getifaddrs(&ifaddrs) < 0)
		quit("getting interface addresses");
	
	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET ||
		    (ifa->ifa_flags & IFF_UP) == 0 ||
		    (ifa->ifa_flags &
		    (IFF_BROADCAST|IFF_POINTOPOINT|IFF_MULTICAST)) == 0 ||
		    ifa->ifa_dstaddr == NULL)
			continue;
#define IPADDR_SA(x) ((struct sockaddr_in *)(x))->sin_addr.s_addr
#define PORT_SA(x) ((struct sockaddr_in *)(x))->sin_port
		if (multicast_mode == PER_INTERFACE_MULTICAST &&
		    (ifa->ifa_flags & IFF_MULTICAST) &&
		    !(ifa->ifa_flags & IFF_LOOPBACK)) {
			struct ip_mreq mreq;

			/* used only to select interface */
			if (ifa->ifa_flags & IFF_POINTOPOINT)
				naddr = ifa->ifa_dstaddr;
			else
				naddr = ifa->ifa_addr;
			mreq.imr_multiaddr.s_addr = htonl(INADDR_WHOD_GROUP);
			mreq.imr_interface.s_addr = IPADDR_SA(naddr);
			if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						&mreq, sizeof(mreq)) < 0) {
				syslog(LOG_ERR,
				    "setsockopt IP_ADD_MEMBERSHIP: %m");
				continue;
			}
			multicast_addr.sin_addr.s_addr
						= htonl(INADDR_WHOD_GROUP);
			multicast_addr.sin_port = sp->s_port;
		} else {
			for (np = neighbors; np != NULL; np = np->n_next)
				if (strcmp(ifa->ifa_name, np->n_name) == 0 &&
				    IPADDR_SA(np->n_addr) == IPADDR_SA(ifa->ifa_dstaddr))
					break;
			if (np != NULL)
				continue;
			naddr = ifa->ifa_dstaddr;
		}
		np = (struct neighbor *)malloc(sizeof(*np) +
		    ifa->ifa_dstaddr->sa_len + strlen(ifa->ifa_name) + 1);
		if (np == NULL)
			quit("malloc of neighbor structure");
		np->n_flags = ifa->ifa_flags;
		np->n_addr = (struct sockaddr *)(np + 1);
		np->n_name = naddr->sa_len + (char *)np->n_addr;
		np->n_next = neighbors;
		neighbors = np;
		memcpy(np->n_addr, naddr, naddr->sa_len);
		PORT_SA(np->n_addr) = sp->s_port;
		strcpy(np->n_name, ifa->ifa_name);
	}
	free(ifaddrs);
	return (1);
}

#ifdef DEBUG
int
prsend(s, buf, cc0, flags, to, tolen)
	int s;
	const void *buf;
	int cc0, flags;
	const struct sockaddr *to;
	int tolen;
{
	register struct whod *w = (struct whod *)buf;
	register struct whoent *we;
	register int cc = cc0;
	struct sockaddr_in *sin = (struct sockaddr_in *)to;

	printf("sendto %lx.%d\n", ntohl(sin->sin_addr.s_addr),
	    ntohs(sin->sin_port));
	printf("hostname %s %s\n", w->wd_hostname,
	   interval(ntohl(w->wd_sendtime) - ntohl(w->wd_boottime), "  up"));
	printf("load %4.2f, %4.2f, %4.2f\n",
	    ntohl(w->wd_loadav[0]) / 100.0, ntohl(w->wd_loadav[1]) / 100.0,
	    ntohl(w->wd_loadav[2]) / 100.0);
	cc -= WHDRSIZE;
	for (we = w->wd_we, cc /= sizeof(struct whoent); cc > 0; cc--, we++) {
		time_t t = ntohl(we->we_utmp.out_time);
		printf("%-8.8s %s:%s %.12s",
			we->we_utmp.out_name,
			w->wd_hostname, we->we_utmp.out_line,
			ctime(&t)+4);
		we->we_idle = ntohl(we->we_idle) / 60;
		if (we->we_idle) {
			if (we->we_idle >= 100*60)
				we->we_idle = 100*60 - 1;
			if (we->we_idle >= 60)
				printf(" %2d", we->we_idle / 60);
			else
				printf("   ");
			printf(":%02d", we->we_idle % 60);
		}
		printf("\n");
	}
	return (cc0);
}

char *
interval(time, updown)
	int time;
	const char *updown;
{
	static char resbuf[32];
	int days, hours, minutes;

	if (time < 0 || time > 3*30*24*60*60) {
		(void) snprintf(resbuf, sizeof(resbuf), "   %s ??:??", updown);
		return (resbuf);
	}
	minutes = (time + 59) / 60;		/* round to minutes */
	hours = minutes / 60; minutes %= 60;
	days = hours / 24; hours %= 24;
	if (days)
		(void) snprintf(resbuf, sizeof(resbuf), "%s %2d+%02d:%02d",
		    updown, days, hours, minutes);
	else
		(void) snprintf(resbuf, sizeof(resbuf), "%s    %2d:%02d",
		    updown, hours, minutes);
	return (resbuf);
}
#endif
