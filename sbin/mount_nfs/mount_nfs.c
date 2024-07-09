/* BSDI mount_nfs.c,v 2.10 2003/08/20 23:46:18 polk Exp */
/*
 * Copyright (c) 1992, 1993, 1994
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
"@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mount_nfs.c	8.11 (Berkeley) 5/4/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#ifdef ISO
#include <netiso/iso.h>
#endif

#ifdef NFSKERB
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#define KERNEL
#include <nfs/nfs.h>
#undef KERNEL
#include <nfs/nqnfs.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "mntopts.h"

#define	DEF_RETRY	10000
int retrycnt = DEF_RETRY;

#ifdef NFSKERB
char inst[INST_SZ];
char realm[REALM_SZ];
struct {
	u_long		kind;
	KTEXT_ST	kt;
} ktick;
struct nfsrpc_nickverf kverf;
struct nfsrpc_fullblock kin, kout;
NFSKERBKEY_T kivec;
CREDENTIALS kcr;
struct timeval ktv;
NFSKERBKEYSCHED_T kerb_keysched;
#endif

#define	BGRND	1
#define	ISBGRND	2
int opflags;

int nfsproto = IPPROTO_UDP;
int mnttcp_ok = 1;

#define	ALTF_BG		0x01
#define	ALTF_MNTUDP	0x02
#define ALTF_SEQPACKET	0x04
#define ALTF_TCP	0x08
#define ALTF_NFSV2	0x10
int altflags;
int nfsargs_flags;

u_short tport;
char *deadthresh_s;
char *leaseterm_s;
char *maxgrouplist_s;
char *port_s;
char *readahead_s;
char *readdirsize_s;
char *realm_s;
char *retrans_s;
char *retrycnt_s;
char *rsize_s;
char *timeo_s;
char *wsize_s;

struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_FORCE,
	MOPT_UPDATE,
	{ "bg", 0, ALTF_BG, NULL, &altflags },
	{ "conn", 1, NFSMNT_NOCONN, NULL, &nfsargs_flags },
	{ "deadthresh", 0, NFSMNT_DEADTHRESH, &deadthresh_s, &nfsargs_flags },
	{ "dumbtimer", 0, NFSMNT_DUMBTIMR, NULL, &nfsargs_flags },
	{ "intr", 0, NFSMNT_INT, NULL, &nfsargs_flags },
#ifdef	NFSKERB
	{ "kerb", 0, NFSMNT_KERB, NULL, &nfsargs_flags },
#endif
	{ "leaseterm", 0, NFSMNT_LEASETERM, &leaseterm_s, &nfsargs_flags },
	{ "maxgrps", 0, NFSMNT_MAXGRPS, &maxgrouplist_s, &nfsargs_flags },
	{ "mntudp", 0, ALTF_MNTUDP, NULL, &altflags },
	{ "nfsv3", 0, NFSMNT_NFSV3, NULL, &nfsargs_flags },
	{ "noconn", 0, NFSMNT_NOCONN, NULL, &nfsargs_flags },
	{ "nqnfs", 0, NFSMNT_NQNFS | NFSMNT_NFSV3, NULL, &nfsargs_flags },
#ifdef	ISO
	{ "packet", 0, ALTF_SEQPACKET, NULL, &altflags, },
#endif
	{ "port", 0, 0, &port_s },
	{ "rdirplus", 0, NFSMNT_RDIRPLUS, NULL, &nfsargs_flags },
	{ "readahead", 0, NFSMNT_READAHEAD, &readahead_s, &nfsargs_flags },
	{ "readdirsize", 0, NFSMNT_READDIRSIZE, &readdirsize_s, &nfsargs_flags },
#ifdef	NFSKERB
	{ "realm", 0, NULL, &realm_s, },
#endif
	{ "resvport", 0, NFSMNT_RESVPORT, NULL, &nfsargs_flags },
	{ "retrans", 0, NFSMNT_RETRANS, &retrans_s, &nfsargs_flags },
	{ "retry", 0, 0, &retrycnt_s },
	{ "rsize", 0, NFSMNT_RSIZE, &rsize_s, &nfsargs_flags },
#ifdef ISO
	{ "seqpacket", 0, ALTF_SEQPACKET, NULL, &altflags },
#endif
	{ "soft", 0, NFSMNT_SOFT, NULL, &nfsargs_flags },
	{ "tcp", 0, ALTF_TCP, NULL, &altflags },
	{ "timeo", 0, NFSMNT_TIMEO, &timeo_s, &nfsargs_flags },
	{ "wsize", 0, NFSMNT_WSIZE, &wsize_s, &nfsargs_flags },
	{ "nfsv2", 0, ALTF_NFSV2, NULL, &altflags },
	{ NULL }
};

struct nfs_args nfsdefargs = {
	NFS_ARGSVERSION,
	(struct sockaddr *)0,
	sizeof (struct sockaddr_in),
	SOCK_DGRAM,
	0,
	(u_char *)0,
	0,
	0,
	NFS_WSIZE,
	NFS_RSIZE,
	NFS_READDIRSIZE,
	10,
	NFS_RETRANS,
	NFS_MAXGRPS,
	NFS_DEFRAHEAD,
	NQ_DEFLEASE,
	NQ_DEADTHRESH,
	(char *)0,
};

struct nfhret {
	u_long		stat;
	long		vers;
	long		auth;
	long		fhsize;
	u_char		nfh[NFSX_V3FHMAX];
};

int	getnfsargs __P((char *, struct nfs_args *));
#ifdef ISO
struct	iso_addr *iso_addr __P((const char *));
#endif
void	set_rpc_maxgrouplist __P((int));
__dead	void usage __P((void));
int	xdr_dir __P((XDR *, char *));
int	xdr_fh __P((XDR *, struct nfhret *));
long	lget __P((char *, char *, long, long));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct nfs_args *nfsargsp, nfsargs;
	struct nfsd_cargs ncd;
	long lval;
	int c, error, mntflags, i, nfssvc_flag;
	char *name, *p, *spec;
#ifdef NFSKERB
	uid_t last_ruid;

	error = 0;
	last_ruid = -1;
	(void)strcpy(realm, KRB_REALM);
	if (sizeof (struct nfsrpc_nickverf) != RPCX_NICKVERF ||
	    sizeof (struct nfsrpc_fullblock) != RPCX_FULLBLOCK ||
	    ((char *)&ktick.kt) - ((char *)&ktick) != NFSX_UNSIGNED ||
	    ((char *)ktick.kt.dat) - ((char *)&ktick) != 2 * NFSX_UNSIGNED)
		fprintf(stderr, "Yikes! NFSKERB structs not packed!!\n");
#endif /* NFSKERB */
	retrycnt = DEF_RETRY;

	mntflags = 0;
	nfsargs = nfsdefargs;
	nfsargsp = &nfsargs;
	while ((c = getopt(argc, argv,
	    "23a:bcdD:g:I:iKL:lm:o:PpqR:r:sTt:w:x:U")) != EOF)
		switch (c) {
		case '2':
			altflags |= ALTF_NFSV2;
			break;
		case '3':
			nfsargsp->flags |= NFSMNT_NFSV3;
			break;
		case 'a':
			nfsargsp->readahead = lget(optarg, "-a", 0, LONG_MAX);
			nfsargsp->flags |= NFSMNT_READAHEAD;
			break;
		case 'b':
			opflags |= BGRND;
			break;
		case 'c':
			nfsargsp->flags |= NFSMNT_NOCONN;
			break;
		case 'D':
			nfsargsp->deadthresh = lget(optarg, "-D", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_DEADTHRESH;
			break;
		case 'd':
			nfsargsp->flags |= NFSMNT_DUMBTIMR;
			break;
		case 'g':
			nfsargsp->maxgrouplist =
			    lget(optarg, "-g", 1, LONG_MAX);
			set_rpc_maxgrouplist(nfsargsp->maxgrouplist);
			nfsargsp->flags |= NFSMNT_MAXGRPS;
			break;
		case 'I':
			nfsargsp->readdirsize = lget(optarg, "-I", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_READDIRSIZE;
			break;
		case 'i':
			nfsargsp->flags |= NFSMNT_INT;
			break;
#ifdef NFSKERB
		case 'K':
			nfsargsp->flags |= NFSMNT_KERB;
			break;
#endif
		case 'L':
			nfsargsp->leaseterm = lget(optarg, "-L", 2, LONG_MAX);
			nfsargsp->flags |= NFSMNT_LEASETERM;
			break;
		case 'l':
			nfsargsp->flags |= NFSMNT_RDIRPLUS;
			break;
#ifdef NFSKERB
		case 'm':
			(void)strncpy(realm, optarg, REALM_SZ - 1);
			realm[REALM_SZ - 1] = '\0';
			break;
#endif
		case 'o':
			getmntopts(optarg, mopts, &mntflags);
			if (altflags & ALTF_BG)
				opflags |= BGRND;
			if (altflags & ALTF_MNTUDP)
				mnttcp_ok = 0;
#ifdef ISO
			if (altflags & ALTF_SEQPACKET)
				nfsargsp->sotype = SOCK_SEQPACKET;
#endif
			if (altflags & ALTF_TCP) {
				nfsargsp->sotype = SOCK_STREAM;
				nfsproto = IPPROTO_TCP;
			}
			altflags &= ALTF_NFSV2;
			break;
		case 'P':
			nfsargsp->flags |= NFSMNT_RESVPORT;
			break;
#ifdef ISO
		case 'p':
			nfsargsp->sotype = SOCK_SEQPACKET;
			break;
#endif
		case 'q':
			nfsargsp->flags |= (NFSMNT_NQNFS | NFSMNT_NFSV3);
			break;
		case 'R':
			retrycnt = lget(optarg, "-R", 1, LONG_MAX);
			break;
		case 'r':
			nfsargsp->rsize = lget(optarg, "-r", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_RSIZE;
			break;
		case 's':
			nfsargsp->flags |= NFSMNT_SOFT;
			break;
		case 'T':
			nfsargsp->sotype = SOCK_STREAM;
			nfsproto = IPPROTO_TCP;
			break;
		case 't':
			nfsargsp->timeo = lget(optarg, "-t", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_TIMEO;
			break;
		case 'U':
			mnttcp_ok = 0;
			break;
		case 'w':
			nfsargsp->wsize = lget(optarg, "-w", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_WSIZE;
			break;
		case 'x':
			nfsargsp->retrans = lget(optarg, "-x", 1, LONG_MAX);
			nfsargsp->flags |= NFSMNT_RETRANS;
			break;
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		usage();
		/* NOTREACHED */
	}

	spec = *argv++;
	name = *argv;

	nfsargsp->flags |= nfsargs_flags;

	if (deadthresh_s != NULL)
		nfsargsp->deadthresh =
		    lget(deadthresh_s, "deadthresh", 1, LONG_MAX);
	if (leaseterm_s != NULL)
		nfsargsp->leaseterm =
		    lget(leaseterm_s, "leaseterm", 1, LONG_MAX);
	if (maxgrouplist_s != NULL) {
		nfsargsp->maxgrouplist =
		    lget(maxgrouplist_s, "maxgrouplist", 1, LONG_MAX);
		set_rpc_maxgrouplist(nfsargsp->maxgrouplist);
	}
	if (port_s != NULL)
		tport = lget(port_s, "port", 1, (long)USHRT_MAX);
	if (readahead_s != NULL)
		nfsargsp->readahead =
		    lget(readahead_s, "readahead", 1, LONG_MAX);
	if (readdirsize_s != NULL)
		nfsargsp->readdirsize =
		    lget(readdirsize_s, "readdirsize", 1, LONG_MAX);
#ifdef	NFSKERB
	if (realm_s != NULL) {
		(void)strncpy(realm, realm_s, REALM_SZ - 1);
		realm[REALM_SZ - 1] = '\0';
	}
#endif
	if (retrans_s != NULL)
		nfsargsp->retrans =
		    lget(retrans_s, "retrans", 1, LONG_MAX);
	if (retrycnt_s != NULL)
		retrycnt = lget(retrycnt_s, "retrycnt", 1, LONG_MAX);
	if (rsize_s != NULL)
		nfsargsp->rsize = lget(rsize_s, "rsize", 1, LONG_MAX);
	if (timeo_s != NULL)
		nfsargsp->timeo = lget(timeo_s, "timeo", 1, LONG_MAX);
	if (wsize_s != NULL)
		nfsargsp->wsize = lget(wsize_s, "wsize", 1, LONG_MAX);

	if (!getnfsargs(spec, nfsargsp))
		exit(1);

	if (mount("nfs", name, mntflags, nfsargsp))
		if (errno == ENODEV)
			errx(1, "%s on %s: filesystem type not configured",
				spec, name);
		else
			err(1, "%s on %s", spec, name);

	if (nfsargsp->flags & (NFSMNT_NQNFS | NFSMNT_KERB)) {
		if ((opflags & ISBGRND) == 0) {
			if (i = fork()) {
				if (i == -1)
					err(1, "nqnfs 1");
				exit(0);
			}
			(void) setsid();
			(void) close(STDIN_FILENO);
			(void) close(STDOUT_FILENO);
			(void) close(STDERR_FILENO);
			(void) chdir("/");
		}
		openlog("mount_nfs:", LOG_PID, LOG_DAEMON);
		nfssvc_flag = NFSSVC_MNTD;
		ncd.ncd_dirp = name;
		while (nfssvc(nfssvc_flag, (caddr_t)&ncd) < 0) {
			if (errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc err %m");
				continue;
			}
			nfssvc_flag =
			    NFSSVC_MNTD | NFSSVC_GOTAUTH | NFSSVC_AUTHINFAIL;
#ifdef NFSKERB
			/*
			 * Set up as ncd_authuid for the kerberos call.
			 * Must set ruid to ncd_authuid and reset the
			 * ticket name iff ncd_authuid is not the same
			 * as last time, so that the right ticket file
			 * is found.
			 * Get the Kerberos credential structure so that
			 * we have the seesion key and get a ticket for
			 * this uid.
			 * For more info see the IETF Draft "Authentication
			 * in ONC RPC".
			 */
			if (ncd.ncd_authuid != last_ruid) {
				krb_set_tkt_string("");
				last_ruid = ncd.ncd_authuid;
			}
			setreuid(ncd.ncd_authuid, 0);
			kret = krb_get_cred(NFS_KERBSRV, inst, realm, &kcr);
			if (kret == RET_NOTKT) {
		            kret = get_ad_tkt(NFS_KERBSRV, inst, realm,
				DEFAULT_TKT_LIFE);
			    if (kret == KSUCCESS)
				kret = krb_get_cred(NFS_KERBSRV, inst, realm,
				    &kcr);
			}
			if (kret == KSUCCESS)
			    kret = krb_mk_req(&ktick.kt, NFS_KERBSRV, inst,
				realm, 0);

			/*
			 * Fill in the AKN_FULLNAME authenticator and verfier.
			 * Along with the Kerberos ticket, we need to build
			 * the timestamp verifier and encrypt it in CBC mode.
			 */
			if (kret == KSUCCESS &&
			    ktick.kt.length <= (RPCAUTH_MAXSIZ-3*NFSX_UNSIGNED)
			    && gettimeofday(&ktv, (struct timezone *)0) == 0) {
			    ncd.ncd_authtype = RPCAUTH_KERB4;
			    ncd.ncd_authstr = (u_char *)&ktick;
			    ncd.ncd_authlen = nfsm_rndup(ktick.kt.length) +
				3 * NFSX_UNSIGNED;
			    ncd.ncd_verfstr = (u_char *)&kverf;
			    ncd.ncd_verflen = sizeof (kverf);
			    memmove(ncd.ncd_key, kcr.session,
				sizeof (kcr.session));
			    kin.t1 = htonl(ktv.tv_sec);
			    kin.t2 = htonl(ktv.tv_usec);
			    kin.w1 = htonl(NFS_KERBTTL);
			    kin.w2 = htonl(NFS_KERBTTL - 1);
			    bzero((caddr_t)kivec, sizeof (kivec));

			    /*
			     * Encrypt kin in CBC mode using the session
			     * key in kcr.
			     */
			    XXX

			    /*
			     * Finally, fill the timestamp verifier into the
			     * authenticator and verifier.
			     */
			    ktick.kind = htonl(RPCAKN_FULLNAME);
			    kverf.kind = htonl(RPCAKN_FULLNAME);
			    NFS_KERBW1(ktick.kt) = kout.w1;
			    ktick.kt.length = htonl(ktick.kt.length);
			    kverf.verf.t1 = kout.t1;
			    kverf.verf.t2 = kout.t2;
			    kverf.verf.w2 = kout.w2;
			    nfssvc_flag = NFSSVC_MNTD | NFSSVC_GOTAUTH;
			}
			setreuid(0, 0);
#endif /* NFSKERB */
		}
	}
	exit(0);
}

int
getnfsargs(spec, nfsargsp)
	char *spec;
	struct nfs_args *nfsargsp;
{
	static struct nfhret nfhret;
	static struct sockaddr_in saddr;
	static char nam[MNAMELEN + 1];
	CLIENT *clp;
	struct hostent *hp;
	struct timeval pertry, try;
#ifdef ISO
	static struct sockaddr_iso isoaddr;
	struct iso_addr *isop;
	int isoflag = 0;
#endif
	enum clnt_stat clnt_stat;
	int so = RPC_ANYSOCK, i, nfsvers, mntvers;
#ifdef	NFSKERB
	char *cp;
#endif
	char *delimp, *hostp;

	strncpy(nam, spec, MNAMELEN);
	nam[MNAMELEN] = '\0';
	if ((delimp = strchr(spec, '@')) != NULL) {
		hostp = delimp + 1;
	} else if ((delimp = strchr(spec, ':')) != NULL) {
		hostp = spec;
		spec = delimp + 1;
	} else {
		warnx("no <host>:<dirpath> or <dirpath>@<host> spec");
		return (0);
	}
	*delimp = '\0';
	/*
	 * DUMB!! Until the mount protocol works on iso transport, we must
	 * supply both an iso and an inet address for the host.
	 */
#ifdef ISO
	if (!strncmp(hostp, "iso=", 4)) {
		u_short isoport;

		hostp += 4;
		isoflag++;
		if ((delimp = strchr(hostp, '+')) == NULL) {
			warnx("no iso+inet address");
			return (0);
		}
		*delimp = '\0';
		if ((isop = iso_addr(hostp)) == NULL) {
			warnx("bad ISO address");
			return (0);
		}
		memset(&isoaddr, 0, sizeof (isoaddr));
		memmove(&isoaddr.siso_addr, isop, sizeof (struct iso_addr));
		isoaddr.siso_len = sizeof (isoaddr);
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		isoport = htons(NFS_PORT);
		memmove(TSEL(&isoaddr), &isoport, isoaddr.siso_tlen);
		hostp = delimp + 1;
	}
#endif /* ISO */

	/*
	 * Handle an internet host address and reverse resolve it if
	 * doing Kerberos.
	 */
	if (isdigit(*hostp)) {
		if ((saddr.sin_addr.s_addr = inet_addr(hostp)) == -1) {
			warnx("bad net address %s", hostp);
			return (0);
		}
	} else {
		if ((hp = gethostbyname(hostp)) == NULL) {
			warnx("can't get net id for host");
			return (0);
		}
		memmove(&saddr.sin_addr, hp->h_addr, hp->h_length);
        }
#ifdef NFSKERB
	if ((nfsargsp->flags & NFSMNT_KERB)) {
		if ((hp = gethostbyaddr((char *)&saddr.sin_addr.s_addr,
		    sizeof (u_long), AF_INET)) == (struct hostent *)0) {
			warnx("can't reverse resolve net address");
			return (0);
		}
		memmove(&saddr.sin_addr, hp->h_addr, hp->h_length);
		strncpy(inst, hp->h_name, INST_SZ);
		inst[INST_SZ - 1] = '\0';
		if (cp = strchr(inst, '.'))
			*cp = '\0';
	}
#endif /* NFSKERB */

again:
	if (altflags & ALTF_NFSV2) {
		nfsvers = 2;
		mntvers = 1;
	} else {
		nfsvers = 3;
		mntvers = 3;
	}

	nfhret.stat = EACCES;	/* Mark not yet successful */
	while (retrycnt > 0) {
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(PMAPPORT);
		if (tport == 0 &&
		    (tport = pmap_getport(&saddr,
		    RPCPROG_NFS, nfsvers, nfsproto)) == 0) {
			if ((opflags & ISBGRND) == 0)
				clnt_pcreateerror("NFS Portmap");
		} else {
			saddr.sin_port = 0;
			pertry.tv_sec = 10;
			pertry.tv_usec = 0;
			clp = NULL;
			if (mnttcp_ok && nfsargsp->sotype == SOCK_STREAM)
				clp = clnttcp_create(&saddr,
				    RPCPROG_MNT, mntvers, &so, 0, 0);
			if (clp == NULL)
				clp = clntudp_create(&saddr,
				    RPCPROG_MNT, mntvers, pertry, &so);
			if (clp == NULL) {
				if ((opflags & ISBGRND) == 0)
					clnt_pcreateerror("Cannot MNT PRC");
			} else {
				clp->cl_auth = authunix_create_default();
				try.tv_sec = 10;
				try.tv_usec = 0;
				if (nfsargsp->flags & NFSMNT_KERB)
				    nfhret.auth = RPCAUTH_KERB4;
				else
				    nfhret.auth = RPCAUTH_UNIX;
				nfhret.vers = mntvers;
				clnt_stat = clnt_call(clp, RPCMNT_MOUNT,
				    xdr_dir, spec, xdr_fh, (caddr_t) &nfhret,
				    try);
				if (clnt_stat != RPC_SUCCESS) {
					if (clnt_stat == RPC_PROGVERSMISMATCH &&
					    !(nfsargsp->flags & NFSMNT_NFSV3) &&
					    !(altflags & ALTF_NFSV2)) {
						altflags |= ALTF_NFSV2;
						goto again;
					}
					if ((opflags & ISBGRND) == 0)
						warnx("%s", clnt_sperror(clp,
						    "bad MNT RPC"));
				} else {
					auth_destroy(clp->cl_auth);
					clnt_destroy(clp);
					retrycnt = 0;
				}
			}
		}
		if (--retrycnt > 0) {
			if (opflags & BGRND) {
				opflags &= ~BGRND;
				if (i = fork()) {
					if (i == -1)
						err(1, "nqnfs 2");
					exit(0);
				}
				(void) setsid();
				(void) close(STDIN_FILENO);
				(void) close(STDOUT_FILENO);
				(void) close(STDERR_FILENO);
				(void) chdir("/");
				opflags |= ISBGRND;
			}
			sleep(60);
		}
	}
	if (nfhret.stat) {
		if (opflags & ISBGRND)
			exit(1);
		warnx("can't access %s: %s", spec, strerror(nfhret.stat));
		return (0);
	}
	saddr.sin_port = htons(tport);
#ifdef ISO
	if (isoflag) {
		nfsargsp->addr = (struct sockaddr *) &isoaddr;
		nfsargsp->addrlen = sizeof (isoaddr);
	} else
#endif /* ISO */
	{
		nfsargsp->addr = (struct sockaddr *) &saddr;
		nfsargsp->addrlen = sizeof (saddr);
	}
	nfsargsp->fh = nfhret.nfh;
	nfsargsp->fhsize = nfhret.fhsize;
	nfsargsp->hostname = nam;
	if (mntvers == 3)
		nfsargsp->flags |= NFSMNT_NFSV3;
	return (1);
}

/*
 * xdr routines for mount rpc's
 */
int
xdr_dir(xdrsp, dirp)
	XDR *xdrsp;
	char *dirp;
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

int
xdr_fh(xdrsp, np)
	XDR *xdrsp;
	register struct nfhret *np;
{
	register int i;
	long auth, authcnt, authfnd = 0;

	if (!xdr_u_long(xdrsp, &np->stat))
		return (0);
	if (np->stat)
		return (1);
	switch (np->vers) {
	case 1:
		np->fhsize = NFSX_V2FH;
		return (xdr_opaque(xdrsp, (caddr_t)np->nfh, NFSX_V2FH));
	case 3:
		if (!xdr_long(xdrsp, &np->fhsize))
			return (0);
		if (np->fhsize <= 0 || np->fhsize > NFSX_V3FHMAX)
			return (0);
		if (!xdr_opaque(xdrsp, (caddr_t)np->nfh, np->fhsize))
			return (0);
		if (!xdr_long(xdrsp, &authcnt))
			return (0);
		for (i = 0; i < authcnt; i++) {
			if (!xdr_long(xdrsp, &auth))
				return (0);
			if (auth == np->auth)
				authfnd++;
		}
		/*
		 * Some servers, such as DEC's OSF/1 return a nil authenticator
		 * list to indicate RPCAUTH_UNIX.
		 */
		if (!authfnd && (authcnt > 0 || np->auth != RPCAUTH_UNIX))
			np->stat = EAUTH;
		return (1);
	};
	return (0);
}

__dead void
usage()
{
	(void)fprintf(stderr, "usage: mount_nfs %s\n%s\n%s\n%s\n",
"[-bcdiKklMPqsT] [-a maxreadahead] [-D deadthresh]",
"\t[-g maxgroups] [-L leaseterm] [-m realm] [-o options] [-R retrycnt]",
"\t[-r readsize] [-t timeout] [-w writesize] [-x retrans]",
"\trhost:path node");
	exit(1);
}

long
lget(arg, name, min, max)
	char *arg, *name;
	long min, max;
{
	long lval;
	char *ep;

	errno = 0;
	lval = strtol(arg, &ep, 10);
	if (lval < min || (max != 0 && lval > max) ||
	    (lval == LONG_MIN || lval == LONG_MAX) && errno == ERANGE) {
		errno = ERANGE;
		err(1, "%s: %s", name, arg);
	}
	if (arg[0] == '\0' || *ep != '\0')
		errx(1, "%s: illegal numeric value: %s", name, arg);
	return (lval);
}
