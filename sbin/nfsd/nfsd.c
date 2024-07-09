/*	BSDI nfsd.c,v 2.8 2003/08/20 23:46:56 polk Exp	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
"@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)nfsd.c	8.9 (Berkeley) 3/29/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#ifdef ISO
#include <netiso/iso.h>
#endif
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#ifdef NFSKERB
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef	IP_OPTIONS
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

/* Global defs */
#ifdef DEBUG
#define	syslog(e, s)	fprintf(stderr,(s))
int	debug = 1;
#else
int	debug = 0;
#endif
int	privport;

struct	nfsd_srvargs nsd;

#ifdef NFSKERB
char		lnam[ANAME_SZ];
KTEXT_ST	kt;
AUTH_DAT	kauth;
char		inst[INST_SZ];
struct nfsrpc_fullblock kin, kout;
struct nfsrpc_fullverf kverf;
NFSKERBKEY_T	kivec;
struct timeval	ktv;
NFSKERBKEYSCHED_T kerb_keysched;
#endif

#ifdef IP_OPTIONS
int	clobber_ip_options __P((int, struct sockaddr_in *));
#endif
int	nfsport __P((const char *));
void	nonfs __P((int));
void	reapchild __P((int));
void	usage __P((void));

/*
 * Nfs server daemon mostly just a user context for nfssvc()
 *
 * 1 - do file descriptor and signal cleanup
 * 2 - fork the nfsd(s)
 * 3 - create server socket(s)
 * 4 - register socket with portmap
 *
 * For connectionless protocols, just pass the socket into the kernel via.
 * nfssvc().
 * For connection based sockets, loop doing accepts. When you get a new
 * socket from accept, pass the msgsock into the kernel via. nfssvc().
 * The arguments are:
 *	-c - support iso cltp clients
 *	-r - reregister with portmapper
 *	-t - support tcp nfs clients
 *	-u - support udp nfs clients
 * followed by "n" which is the number of nfsds' to fork off
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct nfsd_args nfsdargs;
	struct sockaddr_in inetaddr, inetpeer;
#ifdef ISO
	struct sockaddr_iso isoaddr, isopeer;
#endif
	struct timeval ktv;
	fd_set ready, sockbits;
	int ch, connect_type_cnt, i, len, maxsock, msgsock;
	int nfsdcnt, nfssvc_flag, on, reregister, sock, tcpflag, tcpsock;
#ifdef ISO
	int cltpflag;
#ifdef notyet
	int tp4cnt, tp4flag, tp4sock;
	int tpipcnt, tpipflag, tpipsock;
#endif
#endif
	int udpflag;
	int port;

#define	MAXNFSDCNT	20
#define	DEFNFSDCNT	 4
	nfsdcnt = DEFNFSDCNT;
	reregister = 0;
	tcpsock = tcpflag = 0;
	udpflag = 0;
#ifdef ISO
	cltpflag = 0;
#ifdef notyet
	tp4sock = tp4cnt = tp4flag = 0;
	tpipsock = tpipcnt = tpipflag = 0;
#define	GETOPT	"ciPn:prtu"
#define	USAGE	"[-ciPprtu] [-n num_servers]"
#else /* notyet */
#define	GETOPT	"cn:prtu"
#define	USAGE	"[-cprtu] [-n num_servers]"
#endif /* notyet */
#else /* ISO */
#define	GETOPT	"n:prtu"
#define	USAGE	"[-prtu] [-n num_servers]"
#endif /* ISO */
	while ((ch = getopt(argc, argv, GETOPT)) != EOF)
		switch (ch) {
		case 'n':
			nfsdcnt = atoi(optarg);
			if (nfsdcnt < 1 || nfsdcnt > MAXNFSDCNT) {
				warnx("nfsd count %d; reset to %d", nfsdcnt,
				    DEFNFSDCNT);
				nfsdcnt = DEFNFSDCNT;
			}
			break;
		case 'p':
			privport = 1;
			break;
		case 'r':
			reregister = 1;
			break;
		case 't':
			tcpflag = 1;
			break;
		case 'u':
			udpflag = 1;
			break;
#ifdef ISO
		case 'c':
			cltpflag = 1;
			break;
#ifdef notyet
		case 'i':
			tp4cnt = 1;
			break;
		case 'P':
			tpipcnt = 1;
			break;
#endif /* notyet */
#endif /* ISO */
		default:
		case '?':
			usage();
		};
	argv += optind;
	argc -= optind;

	/*
	 * XXX
	 * Backward compatibility, trailing number is the count of daemons.
	 */
	if (argc > 1)
		usage();
	if (argc == 1) {
		nfsdcnt = atoi(argv[0]);
		if (nfsdcnt < 1 || nfsdcnt > MAXNFSDCNT) {
			warnx("nfsd count %d; reset to %d", DEFNFSDCNT);
			nfsdcnt = DEFNFSDCNT;
		}
	}

	if (debug == 0) {
		daemon(0, 0);
		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGSYS, nonfs);
		(void)signal(SIGTERM, SIG_IGN);
	}
	(void)signal(SIGCHLD, reapchild);

	if (reregister) {
		if (udpflag &&
		    (!pmap_set(RPCPROG_NFS, 2, IPPROTO_UDP, NFS_PORT) ||
		     !pmap_set(RPCPROG_NFS, 3, IPPROTO_UDP, NFS_PORT)))
			err(1, "can't register with portmap for UDP.");
		if (tcpflag &&
		    (!pmap_set(RPCPROG_NFS, 2, IPPROTO_TCP, NFS_PORT) ||
		     !pmap_set(RPCPROG_NFS, 3, IPPROTO_TCP, NFS_PORT)))
			err(1, "can't register with portmap for TCP.");
		exit(0);
	}
	openlog("nfsd", LOG_PID, LOG_DAEMON);

	for (i = 0; i < nfsdcnt; i++) {
		switch (fork()) {
		case -1:
			syslog(LOG_ERR, "fork: %m");
			exit (1);
		case 0:
			break;
		default:
			continue;
		}

		setproctitle("nfsd-server");
		nfssvc_flag = NFSSVC_NFSD;
		nsd.nsd_nfsd = NULL;
#ifdef NFSKERB
		if (sizeof (struct nfsrpc_fullverf) != RPCX_FULLVERF ||
		    sizeof (struct nfsrpc_fullblock) != RPCX_FULLBLOCK)
		    syslog(LOG_ERR, "Yikes NFSKERB structs not packed!");
		nsd.nsd_authstr = (u_char *)&kt;
		nsd.nsd_authlen = sizeof (kt);
		nsd.nsd_verfstr = (u_char *)&kverf;
		nsd.nsd_verflen = sizeof (kverf);
#endif
		while (nfssvc(nfssvc_flag, &nsd) < 0) {
			if (errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc: %m");
				exit(1);
			}
			nfssvc_flag = NFSSVC_NFSD | NFSSVC_AUTHINFAIL;
#ifdef NFSKERB
			/*
			 * Get the Kerberos ticket out of the authenticator
			 * verify it and convert the principal name to a user
			 * name. The user name is then converted to a set of
			 * user credentials via the password and group file.
			 * Finally, decrypt the timestamp and validate it.
			 * For more info see the IETF Draft "Authentication
			 * in ONC RPC".
			 */
			kt.length = ntohl(kt.length);
			if (gettimeofday(&ktv, (struct timezone *)0) == 0 &&
			    kt.length > 0 && kt.length <=
			    (RPCAUTH_MAXSIZ - 3 * NFSX_UNSIGNED)) {
			    kin.w1 = NFS_KERBW1(kt);
			    kt.mbz = 0;
			    (void)strcpy(inst, "*");
			    if (krb_rd_req(&kt, NFS_KERBSRV,
				inst, nsd.nsd_haddr, &kauth, "") == RD_AP_OK &&
				krb_kntoln(&kauth, lnam) == KSUCCESS &&
				(pwd = getpwnam(lnam)) != NULL) {
				cr = &nsd.nsd_cr;
				cr->cr_uid = pwd->pw_uid;
				cr->cr_groups[0] = pwd->pw_gid;
				cr->cr_ngroups = 1;
				setgrent();
				while ((grp = getgrent()) != NULL) {
					if (grp->gr_gid == cr->cr_groups[0])
						continue;
					for (cpp = grp->gr_mem;
					    *cpp != NULL; ++cpp)
						if (!strcmp(*cpp, lnam))
							break;
					if (*cpp == NULL)
						continue;
					cr->cr_groups[cr->cr_ngroups++]
					    = grp->gr_gid;
					if (cr->cr_ngroups == NGROUPS)
						break;
				}
				endgrent();

				/*
				 * Get the timestamp verifier out of the
				 * authenticator and verifier strings.
				 */
				kin.t1 = kverf.t1;
				kin.t2 = kverf.t2;
				kin.w2 = kverf.w2;
				bzero((caddr_t)kivec, sizeof (kivec));
				bcopy((caddr_t)kauth.session,
				    (caddr_t)nsd.nsd_key,sizeof(kauth.session));

				/*
				 * Decrypt the timestamp verifier in CBC mode.
				 */
				XXX

				/*
				 * Validate the timestamp verifier, to
				 * check that the session key is ok.
				 */
				nsd.nsd_timestamp.tv_sec = ntohl(kout.t1);
				nsd.nsd_timestamp.tv_usec = ntohl(kout.t2);
				nsd.nsd_ttl = ntohl(kout.w1);
				if ((nsd.nsd_ttl - 1) == ntohl(kout.w2))
				    nfssvc_flag = NFSSVC_NFSD | NFSSVC_AUTHIN;
			}
#endif /* NFSKERB */
		}
		exit(0);
	}

	/* If we are serving udp, set up the socket. */
	if (udpflag) {
		port = nfsport("udp");
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create udp socket: %m");
			exit(1);
		}
		memset(&inetaddr, 0, sizeof(inetaddr));
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(port);
		inetaddr.sin_len = sizeof(inetaddr);
		if (bind(sock,
		    (struct sockaddr *)&inetaddr, sizeof(inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind udp addr (port %d): %m",
			    port);
			exit(1);
		}
		/* XXX pmap_set error reason prints to stderr & is lost */
		if (!pmap_set(RPCPROG_NFS, 2, IPPROTO_UDP, port) ||
		    !pmap_set(RPCPROG_NFS, 3, IPPROTO_UDP, port)) {
			syslog(LOG_ERR, "can't register with udp portmap");
			exit(1);
		}
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add UDP socket: %m");
			exit(1);
		}
		(void)close(sock);
	}

#ifdef ISO
	/* If we are serving cltp, set up the socket. */
	if (cltpflag) {
		port = nfsport("cltp");
		if ((sock = socket(AF_ISO, SOCK_DGRAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create cltp socket: %m");
			exit(1);
		}
		memset(&isoaddr, 0, sizeof(isoaddr));
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		cp = TSEL(&isoaddr);
		*cp++ = port >> 8;
		*cp = port & 0xff;
		isoaddr.siso_len = sizeof(isoaddr);
		if (bind(sock,
		    (struct sockaddr *)&isoaddr, sizeof(isoaddr)) < 0) {
			syslog(LOG_ERR, "can't bind cltp addr: %m");
			exit(1);
		}
#ifdef notyet
		/*
		 * I changed IPPROTO_UDP to ISOPROTO_CLTP ... but this
		 * will never work; we need an address family.  XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, ISOPROTO_CLTP, port)) {
			syslog(LOG_ERR, "can't register with cltp portmap");
			exit(1);
		}
#endif /* notyet */
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add CLTP socket: %m");
			exit(1);
		}
		close(sock);
	}
#endif /* ISO */

	/* Now set up the master server socket waiting for tcp connections. */
	on = 1;
	FD_ZERO(&sockbits);
	connect_type_cnt = 0;
	maxsock = 0;
	if (tcpflag) {
		port = nfsport("tcp");
		if ((tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create tcp socket: %m");
			exit(1);
		}
		if (setsockopt(tcpsock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		memset(&inetaddr, 0, sizeof(inetaddr));
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(port);
		inetaddr.sin_len = sizeof(inetaddr);
		if (bind(tcpsock,
		    (struct sockaddr *)&inetaddr, sizeof (inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tcp addr: %m");
			exit(1);
		}
		if (listen(tcpsock, 5) < 0) {
			syslog(LOG_ERR, "listen failed: %m");
			exit(1);
		}
		if (!pmap_set(RPCPROG_NFS, 2, IPPROTO_TCP, port) ||
		    !pmap_set(RPCPROG_NFS, 3, IPPROTO_TCP, port)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			exit(1);
		}
		FD_SET(tcpsock, &sockbits);
		maxsock = tcpsock;
		connect_type_cnt++;
	}

#ifdef notyet
	/* Now set up the master server socket waiting for tp4 connections. */
	if (tp4flag) {
		port = nfsport("tp4");
		if ((tp4sock = socket(AF_ISO, SOCK_SEQPACKET, 0)) < 0) {
			syslog(LOG_ERR, "can't create tp4 socket: %m");
			exit(1);
		}
		if (setsockopt(tp4sock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		memset(&isoaddr, 0, sizeof(isoaddr));
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		cp = TSEL(&isoaddr);
		*cp++ = port >> 8;
		*cp = port & 0xff;
		isoaddr.siso_len = sizeof(isoaddr);
		if (bind(tp4sock,
		    (struct sockaddr *)&isoaddr, sizeof (isoaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tp4 addr: %m");
			exit(1);
		}
		if (listen(tp4sock, 5) < 0) {
			syslog(LOG_ERR, "listen failed: %m");
			exit(1);
		}
		/*
		 * XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, ISOPROTO_TP4, port)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			exit(1);
		}
		FD_SET(tp4sock, &sockbits);
		maxsock = tp4sock;
		connect_type_cnt++;
	}

	/* Now set up the master server socket waiting for tpip connections. */
	/* ??? this says tpip but seems to be using RDP? */
	if (tpipflag) {
		port = nfsport("tpip");
		if ((tpipsock = socket(AF_INET, SOCK_SEQPACKET, 0)) < 0) {
			syslog(LOG_ERR, "can't create tpip socket: %m");
			exit(1);
		}
		if (setsockopt(tpipsock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		memset(&inetaddr, 0, sizeof(inetaddr));
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(port);
		inetaddr.sin_len = sizeof(inetaddr);
		if (bind(tpipsock,
		    (struct sockaddr *)&inetaddr, sizeof (inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tpip addr: %m");
			exit(1);
		}
		if (listen(tpipsock, 5) < 0) {
			syslog(LOG_ERR, "listen failed: %m");
			exit(1);
		}
		/*
		 * XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, IPPROTO_TCP, port)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			exit(1);
		}
		FD_SET(tpipsock, &sockbits);
		maxsock = tpipsock;
		connect_type_cnt++;
	}
#endif /* notyet */

	if (connect_type_cnt == 0)
		exit(0);

	setproctitle("nfsd-master");

	/*
	 * Loop forever accepting connections and passing the sockets
	 * into the kernel for the mounts.
	 */
	for (;;) {
		ready = sockbits;
		if (connect_type_cnt > 1) {
			if (select(maxsock + 1,
			    &ready, NULL, NULL, NULL) < 1) {
				syslog(LOG_ERR, "select failed: %m");
				exit(1);
			}
		}
		if (tcpflag && FD_ISSET(tcpsock, &ready)) {
			len = sizeof(inetpeer);
			if ((msgsock = accept(tcpsock,
			    (struct sockaddr *)&inetpeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
#ifdef IP_OPTIONS
			if (clobber_ip_options(msgsock, &inetpeer))
				continue;
#endif
			memset(inetpeer.sin_zero, 0, sizeof(inetpeer.sin_zero));
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inetpeer;
			nfsdargs.namelen = sizeof(inetpeer);
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
#ifdef notyet
		if (tp4flag && FD_ISSET(tp4sock, &ready)) {
			len = sizeof(isopeer);
			if ((msgsock = accept(tp4sock,
			    (struct sockaddr *)&isopeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&isopeer;
			nfsdargs.namelen = len;
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
		if (tpipflag && FD_ISSET(tpipsock, &ready)) {
			len = sizeof(inetpeer);
			if ((msgsock = accept(tpipsock,
			    (struct sockaddr *)&inetpeer, &len)) < 0) {
				syslog(LOG_ERR, "Accept failed: %m");
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR, "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inetpeer;
			nfsdargs.namelen = len;
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
#endif /* notyet */
	}
}

#ifdef IP_OPTIONS
int
clobber_ip_options(s, fromp)
	int s;
	struct sockaddr_in *fromp;
{
	int optsize, optlen;
	char *lp;
	u_char *cp;
	struct protoent *ip;
	struct ip_opts optbuf;
	static int ipproto;
	/*
	 * Length of output buffer needs to be at least:
	 *      strlen("strict routed via: ")
	 *      strlen("xxx.xxx.xxx.xxx")
	 *      max_IP_addr * strlen(" xxx.xxx.xxx.xxx")
	 * 19 + 15 + (40-4)/4 * 16 = 177
	 */
	char lbuf[256];

	if (ipproto == 0) {
		if ((ip = getprotobyname("ip")) != NULL)
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
	}
	optsize = sizeof optbuf;
	if (getsockopt(s, ipproto, IP_OPTIONS, (char *)&optbuf,
	    &optsize) == 0 && optsize != 0) {
		lp = lbuf;
		optsize -= sizeof(optbuf.ip_dst);
		for (cp = (u_char *)optbuf.ip_opts; optsize > 0; ) {
			switch (*cp) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				optlen = cp[1];
				sprintf(lp, "%srouted via: %s",
				    (*cp == IPOPT_SSRR) ? "strict " : "",
				    inet_ntoa(optbuf.ip_dst));
				lp += strlen(lp);
				cp += 3;
				if (optlen > optsize)
					optlen = optsize;
				optsize -= optlen;
				optlen -= 3;
				while (optlen > 0) {
					sprintf(lp, " %u.%u.%u.%u",
					    cp[0], cp[1], cp[2], cp[3]);
					lp += strlen(lp);
					cp += 4;
					optlen -= 4;
				}
				break;

			case IPOPT_EOL:
				optsize = 0;    /* break out of for() loop */
				break;

			default:
				optlen = (*cp == IPOPT_NOP) ? 1 : *(cp+1);
				cp += optlen;
				optsize -= optlen;
				break;
			}
		}
		if (lp != lbuf) {
			syslog(LOG_NOTICE, "IP Source Route option "
			    "on connection apparently from %s, connection "
			    "dropped: %s", inet_ntoa(fromp->sin_addr), lbuf);
			shutdown(s, 2);
			close(s);
			return (-1);
		}
	}
	return (0);
}
#endif

int
nfsport(proto)
	const char *proto;
{
	struct servent *sp;

	if ((sp = getservbyname("nfs", proto)) != NULL)
		return (ntohs(sp->s_port));
	if (privport) {
		syslog(LOG_ERR,
		    "getservbyname: no privileged nfs/%s service", proto);
		exit(1);
	}
	return (NFS_PORT);
}

void
usage()
{
	(void)fprintf(stderr, "usage: nfsd %s\n", USAGE);
	exit(1);
}

void
nonfs(signo)
	int signo;
{
	syslog(LOG_ERR, "missing system call: NFS not available.");
}

void
reapchild(signo)
	int signo;
{

	while (wait3(NULL, WNOHANG, NULL) > 0);
}
