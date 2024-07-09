/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) /master/core_contrib/tcpdump/tcpdump-3.4a6/print-nfs.c,v 1.6 1999/02/02 05:05:42 jch Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#if __STDC__
struct mbuf;
struct rtentry;
#endif
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <rpc/rpc.h>

#include <ctype.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"

#include "nfsproto.h"
#include "nfsfh.h"

static void nfs_printfh(const u_int32_t *);
static void xid_map_enter(const struct rpc_msg *, const struct ip *, u_int32_t);
static u_int32_t xid_map_find(const struct rpc_msg *, const struct ip *,
    u_int32_t *);
static void interp_reply(const struct rpc_msg *, u_int32_t, u_int);

static int nfserr;		/* true if we error rather than trunc */
static int nfsvers;		/* Version number of the packet we are parsing */

/*
 * Mapping of old NFS Version 2 RPC numbers to generic numbers.
 */
int nfsv3_procid[NFS_NPROCS] = {
	NFSPROC_NULL,
	NFSPROC_GETATTR,
	NFSPROC_SETATTR,
	NFSPROC_NOOP,
	NFSPROC_LOOKUP,
	NFSPROC_READLINK,
	NFSPROC_READ,
	NFSPROC_NOOP,
	NFSPROC_WRITE,
	NFSPROC_CREATE,
	NFSPROC_REMOVE,
	NFSPROC_RENAME,
	NFSPROC_LINK,
	NFSPROC_SYMLINK,
	NFSPROC_MKDIR,
	NFSPROC_RMDIR,
	NFSPROC_READDIR,
	NFSPROC_FSSTAT,
	NFSPROC_FSINFO,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP
};

void
nfsreply_print(register const u_char *bp, u_int length,
	       register const u_char *bp2)
{
	register const struct rpc_msg *rp;
	register const struct ip *ip;
	u_int32_t proc;

	nfserr = 0;		/* assume no error */
	rp = (const struct rpc_msg *)bp;
	ip = (const struct ip *)bp2;

	if (!nflag)
		(void)printf("%s.nfs > %s.%u: reply %s %d",
			     ipaddr_string(&ip->ip_src),
			     ipaddr_string(&ip->ip_dst),
			     (u_int32_t)ntohl(rp->rm_xid),
			     ntohl(rp->rm_reply.rp_stat) == MSG_ACCEPTED?
				     "ok":"ERR",
			     length);
	else
		(void)printf("%s.%u > %s.%u: reply %s %d",
			     ipaddr_string(&ip->ip_src),
			     NFS_PORT,
			     ipaddr_string(&ip->ip_dst),
			     (u_int32_t)ntohl(rp->rm_xid),
			     ntohl(rp->rm_reply.rp_stat) == MSG_ACCEPTED?
			     	"ok":"ERR",
			     length);

	if (xid_map_find(rp, ip, &proc))
		interp_reply(rp, proc, length);
}

/*
 * Return a pointer to the first file handle in the packet.
 * If the packet was truncated, return 0.
 */
static const u_int32_t *
parsereq(register const struct rpc_msg *rp, register u_int length)
{
	register const u_int32_t *dp;
	register u_int len;

	/*
	 * find the start of the req data (if we captured it)
	 */
	dp = (u_int32_t *)&rp->rm_call.cb_cred;
	TCHECK(dp[1]);
	len = ntohl(dp[1]);
	if (len < length) {
		dp += (len + (2 * sizeof(*dp) + 3)) / sizeof(*dp);
		TCHECK(dp[1]);
		len = ntohl(dp[1]);
		if (len < length) {
			dp += (len + (2 * sizeof(*dp) + 3)) / sizeof(*dp);
			TCHECK2(dp[0], 0);
			return (dp);
		}
	}
trunc:
	return (NULL);
}

/*
 * Print out an NFS file handle and return a pointer to following word.
 * If packet was truncated, return 0.
 */
static const u_int32_t *
parsefh(register const u_int32_t *dp)
{
	register u_int32_t len;

	switch (nfsvers) {
	case NFS_VER2:
		if (dp + 8 > (u_int32_t *)snapend)
			return (NULL);
		nfs_printfh(dp);
		return (dp + 8);

	case NFS_VER3:
		if (dp + 1 > (u_int32_t *)snapend)
			return (NULL);
		len = htonl(*dp++);
		if ((u_char *)dp + len > snapend)
			return (NULL);
		nfs_printfh(dp);
		return ((u_int32_t *)((u_char *)dp + len));
	}

	return (NULL);
}

/*
 * Print out a file name and return pointer to 32-bit word past it.
 * If packet was truncated, return 0.
 */
static const u_int32_t *
parsefn(register const u_int32_t *dp)
{
	register u_int32_t len;
	register const u_char *cp;

	/* Bail if we don't have the string length */
	if ((u_char *)dp > snapend - sizeof(*dp))
		return (NULL);

	/* Fetch string length; convert to host order */
	len = *dp++;
	NTOHL(len);

	cp = (u_char *)dp;
	/* Update 32-bit pointer (NFS filenames padded to 32-bit boundaries) */
	dp += ((len + 3) & ~3) / sizeof(*dp);
	if ((u_char *)dp > snapend)
		return (NULL);
	/* XXX seems like we should be checking the length */
	putchar('"');
	(void) fn_printn(cp, len, NULL);
	putchar('"');

	return (dp);
}

/*
 * Print out file handle and file name.
 * Return pointer to 32-bit word past file name.
 * If packet was truncated (or there was some other error), return 0.
 */
static const u_int32_t *
parsefhn(register const u_int32_t *dp)
{
	dp = parsefh(dp);
	if (dp == NULL)
		return (NULL);
	putchar(' ');
	return (parsefn(dp));
}

static void
printquad(const char *string, register const u_int32_t *dp)
{
	u_int32_t high;

	high = (u_int32_t)ntohl(dp[0]);
	if (dp[0] != 0)
		(void)printf("%s0x%x%08x", 
		    string, high,
		    (u_int32_t)ntohl(dp[1]));
	else
		(void)printf("%s%u",
		    string,
		    (u_int32_t)ntohl(dp[1]));
}

#define	parsesattr(dp, v) \
	(nfsvers == NFS_VER2 ? parsesattr2(dp, v) : parsesattr3(dp, v))

static const u_int32_t *
parsesattr2(register const u_int32_t *dp, int verbose)
{
	const struct nfsv2_sattr *sap;

	sap = (const struct nfsv2_sattr *)dp;
	if (verbose) {
		TCHECK(sap->sa_size);
		printf(" %o ids %u/%u sz %u",
		    (u_int32_t)ntohl(sap->sa_mode),
		    (u_int32_t)ntohl(sap->sa_uid),
		    (u_int32_t)ntohl(sap->sa_gid),
		    (u_int32_t)ntohl(sap->sa_size));
	}
	/* print lots more stuff */
	if (verbose > 1) {
		TCHECK(sap->sa_atime);
		printf(" %u.%06u",
		    (u_int32_t)ntohl(sap->sa_atime.nfsv2_sec),
		    (u_int32_t)ntohl(sap->sa_atime.nfsv2_usec));
		TCHECK(sap->sa_mtime);
		printf(" %u.%06u",
		    (u_int32_t)ntohl(sap->sa_mtime.nfsv2_sec),
		    (u_int32_t)ntohl(sap->sa_mtime.nfsv2_usec));
	}
	return ((const u_int32_t *)(sap + 1));
trunc:
	return (NULL);
}

/*
 * Parse the sattr (v3) data
 */
static const u_int32_t *
parsesattr3(register const u_int32_t *dp, int verbose)
{

	TCHECK(dp[0]);
	if (*dp++) {
		if (verbose) {
			TCHECK(dp[0]);
			(void)printf(" %o", 
			    (u_int32_t)ntohl(*dp));
		}
		dp++;
	}
	TCHECK(dp[0]);
	if (*dp++) {
		if (verbose) {
			TCHECK(dp[0]);
			(void)printf(" uid %u", 
			    (u_int32_t)ntohl(*dp));
		}
		dp++;
	}
	TCHECK(dp[0]);
	if (*dp++) {
		if (verbose) {
			TCHECK(dp[0]);
			(void)printf(" gid %u", 
			    (u_int32_t)ntohl(*dp));
		}
		dp++;
	}
	TCHECK(dp[0]);
	if (*dp++) {
		if (verbose) {
			TCHECK2(dp[0], 2 * sizeof(*dp));
			printquad(" sz ", dp);
		}
		dp += 2;
	}
	TCHECK(dp[0]);
	switch (ntohl(*dp++)) {
	case NFSV3SATTRTIME_DONTCHANGE:
		break;
	case NFSV3SATTRTIME_TOSERVER:
		if (verbose > 1)
			(void)printf(" atime server");
		break;
	case NFSV3SATTRTIME_TOCLIENT:
		if (verbose) {
			TCHECK2(dp[0], 2 * sizeof(*dp));
			(void)printf(" atime %u.%09u",
			    (u_int32_t)ntohl(dp[0]),
			    (u_int32_t)ntohl(dp[1]));
		}
		dp += 2;
		break;
	}
	TCHECK(dp[0]);
	switch (ntohl(*dp++)) {
	case NFSV3SATTRTIME_DONTCHANGE:
		break;
	case NFSV3SATTRTIME_TOSERVER:
		if (verbose > 1)
			(void)printf(" mtime server");
		break;
	case NFSV3SATTRTIME_TOCLIENT:
		if (verbose) {
			TCHECK2(dp[0], 2 * sizeof(*dp));
			(void)printf(" mtime %u.%09u",
			    (u_int32_t)ntohl(dp[0]),
			    (u_int32_t)ntohl(dp[1]));
		}
		dp += 2;
		break;
	}

	return (dp);

trunc:
	return (NULL);
}

/*
 * Parse access field
 */
static int
parseaccess(register const u_int32_t *dp)
{
	register u_int32 access;
	TCHECK2(dp[0], sizeof(*dp));
	access = ntohl(dp[0]);
	if (access & NFSV3ACCESS_READ)
		(void)printf(" read");
	if (access & NFSV3ACCESS_LOOKUP)
		(void)printf(" lookup");
	if (access & NFSV3ACCESS_MODIFY)
		(void)printf(" modify");
	if (access & NFSV3ACCESS_EXTEND)
		(void)printf(" extend");
	if (access & NFSV3ACCESS_DELETE)
		(void)printf(" delete");
	if (access & NFSV3ACCESS_EXECUTE)
		(void)printf(" execute");

	return (1);

trunc:
	return (0);
}

static struct tok rwflag2str[] = {
	{ 0x01,	"read" },
	{ 0x02,	"write" },
	{ 0,	NULL }
};

static struct tok sync2str[] = {
	{ NFSV3WRITE_UNSTABLE,	"unstable" },
	{ NFSV3WRITE_DATASYNC,	"datasync" },
	{ NFSV3WRITE_FILESYNC,	"filesync" },
	{ 0,			NULL }
};

static struct tok type2str[] = {
	{ NFNON,	"NON" },
	{ NFREG,	"REG" },
	{ NFDIR,	"DIR" },
	{ NFBLK,	"BLK" },
	{ NFCHR,	"CHR" },
	{ NFLNK,	"LNK" },
	{ NFSOCK,	"SOCK" },
	{ NFFIFO,	"FIFO" },
	{ 0,		NULL }
};

void
nfsreq_print(register const u_char *bp, u_int length,
    register const u_char *bp2)
{
	register const struct rpc_msg *rp;
	register const struct ip *ip;
	register const u_int32_t *dp;
	register u_int32_t proc;

	nfserr = 0;		/* assume no error */
	rp = (const struct rpc_msg *)bp;
	ip = (const struct ip *)bp2;
	nfsvers = ntohl(rp->rm_call.cb_vers);
	if (!nflag)
		(void)printf("%s.%u > %s.nfs: %d v%u",
			     ipaddr_string(&ip->ip_src),
			     (u_int32_t)ntohl(rp->rm_xid),
			     ipaddr_string(&ip->ip_dst),
			     length, nfsvers);
	else
		(void)printf("%s.%u > %s.%u: %d v%u",
			     ipaddr_string(&ip->ip_src),
			     (u_int32_t)ntohl(rp->rm_xid),
			     ipaddr_string(&ip->ip_dst),
			     NFS_PORT,
			     length, nfsvers);

	switch (nfsvers) {
	case NFS_VER2:
		proc = nfsv3_procid[ntohl(rp->rm_call.cb_proc)];
		break;

	case NFS_VER3:
		proc = ntohl(rp->rm_call.cb_proc);
		break;

	default:
		return;
	}

	xid_map_enter(rp, ip, proc);	/* record proc number for later on */

	switch (proc) {
	case NFSPROC_NULL:
		printf(" null");
		return;

	case NFSPROC_GETATTR:
		printf(" getattr");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NFSPROC_SETATTR:
		printf(" setattr");
		if ((dp = parsereq(rp, length)) == NULL ||
		    (dp = parsefh(dp)) == NULL ||
		    (dp = parsesattr(dp, vflag)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			break;

		case NFS_VER3:
			TCHECK2(dp[0], sizeof(*dp));
			if (ntohl(dp[0])) {
				TCHECK2(dp[1], 2 * sizeof(*dp));
				printf(" guard %u.%09u",
				    (u_int32_t)ntohl(dp[1]),
				    (u_int32_t)ntohl(dp[2]));
			}
			return;
		}
		break;

	case NFSPROC_LOOKUP:
		printf(" lookup");
		if ((dp = parsereq(rp, length)) != NULL && parsefhn(dp) != NULL)
			return;
		break;

	case NFSPROC_ACCESS:
		printf(" access");
		if ((dp = parsereq(rp, length)) != NULL && 
		    (dp = parsefh(dp)) != NULL &&
		    parseaccess(dp) != 0)
			return;
		break;

	case NFSPROC_READLINK:
		printf(" readlink");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NFSPROC_READ:
		printf(" read");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			switch (nfsvers) {
			case NFS_VER2:
				TCHECK2(dp[0], 3 * sizeof(*dp));
				printf(" %u bytes @ %u",
				    (u_int32_t)ntohl(dp[1]),
				    (u_int32_t)ntohl(dp[0]));
				break;

			case NFS_VER3:
				TCHECK2(dp[0], 3 * sizeof(*dp));
				printf(" %u bytes",
				    (u_int32_t)ntohl(dp[2]));
				printquad(" @ ", dp);
				break;
			}
			return;
		}
		break;

	case NFSPROC_WRITE:
		printf(" write");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			switch (nfsvers) {
			case NFS_VER2:
				TCHECK2(dp[0], 4 * sizeof(*dp));
				printf(" %u bytes @ %u",
				    (u_int32_t)ntohl(dp[3]),
				    (u_int32_t)ntohl(dp[1]));
				break;

			case NFS_VER3:
				TCHECK2(dp[0], 4 * sizeof(*dp));
				printf(" %s %u bytes",
				    tok2str(sync2str, "sync-%d", (u_int32_t)ntohl(dp[3])),
				    (u_int32_t)ntohl(dp[2]));
				printquad(" @ ", dp);
				break;
			}
			return;
		}
		break;

	case NFSPROC_CREATE:
		printf(" create");
		if ((dp = parsereq(rp, length)) == NULL ||
		    (dp = parsefhn(dp)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER3:
			TCHECK(dp[0]);
			if (ntohl(dp[0]) == NFSV3CREATE_EXCLUSIVE)
				return;
			dp++;
			/* FALL THROUGH */

		case NFS_VER2:
			if (parsesattr(dp, vflag) != NULL)
				return;
			return;
		}
		break;

	case NFSPROC_MKDIR:
		printf(" mkdir");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefhn(dp)) != NULL &&
		    parsesattr(dp, vflag) != NULL)
			return;
		break;

	case NFSPROC_SYMLINK:
		printf(" symlink");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefhn(dp)) != NULL) {
			switch (nfsvers) {
			case NFS_VER3:
				if ((dp = parsesattr3(dp, vflag)) == NULL)
					goto trunc;
				fputs(" -> ", stdout);
				if (parsefn(dp) != NULL)
					return;
				break;

			case NFS_VER2:
				fputs(" -> ", stdout);
				if ((dp = parsefn(dp)) != NULL &&
				    parsesattr2(dp, vflag) != NULL)
					return;
				break;
			}
		}
		break;

	case NFSPROC_MKNOD:
		printf(" mknod");
		if ((dp = parsereq(rp, length)) != NULL && 
		    (dp = parsefhn(dp)) != NULL) {
			u_int32_t type;

			TCHECK(dp[0]);
			type = ntohl(*dp++);
			dp = parsesattr3(dp, vflag);
			printf(" %s", tok2str(type2str, "type-%u", type));
			if (dp == NULL)
				goto trunc;
			if (type == NFBLK || type == NFCHR) {
				TCHECK2(dp[0], 2 * sizeof(*dp));
				printf(" %u,%u", ntohl(dp[0]), ntohl(dp[1]));
			}
			return;
		}
		break;

	case NFSPROC_REMOVE:
		printf(" remove");
		if ((dp = parsereq(rp, length)) != NULL && parsefhn(dp) != NULL)
			return;
		break;

	case NFSPROC_RMDIR:
		printf(" rmdir");
		if ((dp = parsereq(rp, length)) != NULL && parsefhn(dp) != NULL)
			return;
		break;

	case NFSPROC_RENAME:
		printf(" rename");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefhn(dp)) != NULL) {
			fputs(" ->", stdout);
			if (parsefhn(dp) != NULL)
				return;
		}
		break;

	case NFSPROC_LINK:
		printf(" link");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			fputs(" ->", stdout);
			if (parsefhn(dp) != NULL)
				return;
		}
		break;

	case NFSPROC_READDIR:
		printf(" readdir");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			register u_int32_t len;

			switch (nfsvers) {
			case NFS_VER3:
				TCHECK2(dp[0], 5 * sizeof(*dp));
				len = (u_int32_t)ntohl(dp[4]);
				printf(" %u bytes", len);
				printquad(" @ ", dp);
				return;

			case NFS_VER2:
				TCHECK2(dp[0], 2 * sizeof(*dp));
				/*
				 * Print the offset as signed, since -1 is common,
				 * but offsets > 2^31 aren't.
				 */
				printf(" %u bytes @ %d",
				    (u_int32_t)ntohl(dp[1]),
				    (u_int32_t)htonl(dp[0]));
				return;
			}
		}
		break;

	case NFSPROC_READDIRPLUS:
		printf(" readdirplus");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			register u_int32_t len;

			TCHECK2(dp[0], 6 * sizeof(*dp));
			len = (u_int32_t)ntohl(dp[4]);

			/*
			 * Print the offset as signed, since -1 is common,
			 * but offsets > 2^31 aren't.
			 */
			printf(" %u bytes %u max", ntohl(dp[4]), ntohl(dp[5]));
			printquad(" @ ", dp);
			return;
		}
		break;

	case NFSPROC_FSSTAT:
		printf(" fsstat");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NFSPROC_FSINFO:
		printf(" fsinfo");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NFSPROC_PATHCONF:
		printf(" pathconf");
		if ((dp = parsereq(rp, length)) != NULL && parsefhn(dp) != NULL)
			return;
		break;

	case NFSPROC_COMMIT:
		printf(" commit");
		if ((dp = parsereq(rp, length)) != NULL &&
		    (dp = parsefh(dp)) != NULL) {
			TCHECK2(dp[0], 3 * sizeof(*dp));
			printf(" %u bytes",
			    (u_int32_t)ntohl(dp[2]));
			printquad(" @ ", dp);
			return;
		}
		break;

	case NQNFSPROC_GETLEASE:
		printf(" getlease");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		TCHECK2(dp[0], 2 * sizeof(*dp));
		(void)printf(" %s term %u", 
		    tok2str(rwflag2str, "lease-%d", (u_int32_t)ntohl(dp[0])),
		    (u_int32_t)ntohl(dp[1]));
		break;

	case NQNFSPROC_VACATED:
		printf(" vacated");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NQNFSPROC_EVICTED:
		printf(" evicted");
		if ((dp = parsereq(rp, length)) != NULL && parsefh(dp) != NULL)
			return;
		break;

	case NFSPROC_NOOP:
		printf(" nop");
		return;

	default:
		printf(" proc-%u", (u_int32_t)proc);
		return;
	}
trunc:
	if (!nfserr)
		fputs(" [|nfs]", stdout);
}

/*
 * Print out an NFS file handle.
 * We assume packet was not truncated before the end of the
 * file handle pointed to by dp.
 *
 * Note: new version (using portable file-handle parser) doesn't produce
 * generation number.  It probably could be made to do that, with some
 * additional hacking on the parser code.
 */
static void
nfs_printfh(register const u_int32_t *dp)
{
	my_fsid fsid;
	ino_t ino;
	char *sfsname = NULL;

	Parse_fh((caddr_t *)dp, &fsid, &ino, NULL, &sfsname, 0);

	if (sfsname) {
		/* file system ID is ASCII, not numeric, for this server OS */
		static char temp[NFS_SMALLFH+1];

		/* Make sure string is null-terminated */
		strncpy(temp, sfsname, NFS_SMALLFH);	/* XXX */
		/* Remove trailing spaces */
		sfsname = strchr(temp, ' ');
		if (sfsname)
			*sfsname = 0;

		(void)printf(" fh %s/%u", temp, (u_int32_t)ino);
	} else {
		(void)printf(" fh %u,%u/%u",
		    fsid.Fsid_dev.Major, fsid.Fsid_dev.Minor, (u_int32_t)ino);
	}
}

/*
 * Maintain a small cache of recent client.XID.server/proc pairs, to allow
 * us to match up replies with requests and thus to know how to parse
 * the reply.
 */

struct xid_map_entry {
	u_int32_t		xid;		/* transaction ID (net order) */
	struct in_addr	client;		/* client IP address (net order) */
	struct in_addr	server;		/* server IP address (net order) */
	u_int32_t		proc;		/* call proc number (host order) */
	u_int32_t		vers;		/* version number */
};

/*
 * Map entries are kept in an array that we manage as a ring;
 * new entries are always added at the tail of the ring.  Initially,
 * all the entries are zero and hence don't match anything.
 */

#define	XIDMAPSIZE	64

struct xid_map_entry xid_map[XIDMAPSIZE];

int	xid_map_next = 0;
int	xid_map_hint = 0;

static void
xid_map_enter(const struct rpc_msg *rp, const struct ip *ip, u_int32_t proc)
{
	struct xid_map_entry *xmep;

	xmep = &xid_map[xid_map_next];

	if (++xid_map_next >= XIDMAPSIZE)
		xid_map_next = 0;

	xmep->xid = rp->rm_xid;
	xmep->client = ip->ip_src;
	xmep->server = ip->ip_dst;
	xmep->proc = proc;
	xmep->vers = ntohl(rp->rm_call.cb_vers);
}

/* Returns true and sets proc success or false on failure */
static u_int32_t
xid_map_find(const struct rpc_msg *rp, const struct ip *ip, u_int32_t *proc)
{
	int i;
	struct xid_map_entry *xmep;
	u_int32_t xid = rp->rm_xid;
	u_int32_t clip = ip->ip_dst.s_addr;
	u_int32_t sip = ip->ip_src.s_addr;

	/* Start searching from where we last left off */
	i = xid_map_hint;
	do {
		xmep = &xid_map[i];
		if (xmep->xid == xid && xmep->client.s_addr == clip &&
		    xmep->server.s_addr == sip) {
			/* match */
			xid_map_hint = i;
			*proc = xmep->proc;
			nfsvers = xmep->vers;
			return (1);
		}
		if (++i >= XIDMAPSIZE)
			i = 0;
	} while (i != xid_map_hint);

	/* search failed */
	return (0);
}

/*
 * Routines for parsing reply packets
 */

/*
 * Return a pointer to the beginning of the actual results.
 * If the packet was truncated, return 0.
 */
static const u_int32_t *
parserep(register const struct rpc_msg *rp, register u_int length)
{
	register const u_int32_t *dp;
	u_int len;
	enum accept_stat astat;

	/*
	 * Portability note:
	 * Here we find the address of the ar_verf credentials.
	 * Originally, this calculation was
	 *	dp = (u_int32_t *)&rp->rm_reply.rp_acpt.ar_verf
	 * On the wire, the rp_acpt field starts immediately after
	 * the (32 bit) rp_stat field.  However, rp_acpt (which is a
	 * "struct accepted_reply") contains a "struct opaque_auth",
	 * whose internal representation contains a pointer, so on a
	 * 64-bit machine the compiler inserts 32 bits of padding
	 * before rp->rm_reply.rp_acpt.ar_verf.  So, we cannot use
	 * the internal representation to parse the on-the-wire
	 * representation.  Instead, we skip past the rp_stat field,
	 * which is an "enum" and so occupies one 32-bit word.
	 */
	dp = ((const u_int32_t *)&rp->rm_reply) + 1;
	TCHECK2(dp[0], 1);
	len = ntohl(dp[1]);
	if (len >= length)
		return (NULL);
	/*
	 * skip past the ar_verf credentials.
	 */
	dp += (len + (2*sizeof(u_int32_t) + 3)) / sizeof(u_int32_t);
	TCHECK2(dp[0], 0);

	/*
	 * now we can check the ar_stat field
	 */
	astat = ntohl(*(enum accept_stat *)dp);
	switch (astat) {

	case SUCCESS:
		break;

	case PROG_UNAVAIL:
		printf(" PROG_UNAVAIL");
		nfserr = 1;		/* suppress trunc string */
		return (NULL);

	case PROG_MISMATCH:
		printf(" PROG_MISMATCH");
		nfserr = 1;		/* suppress trunc string */
		return (NULL);

	case PROC_UNAVAIL:
		printf(" PROC_UNAVAIL");
		nfserr = 1;		/* suppress trunc string */
		return (NULL);

	case GARBAGE_ARGS:
		printf(" GARBAGE_ARGS");
		nfserr = 1;		/* suppress trunc string */
		return (NULL);

	case SYSTEM_ERR:
		printf(" SYSTEM_ERR");
		nfserr = 1;		/* suppress trunc string */
		return (NULL);

	default:
		printf(" ar_stat %d", astat);
		nfserr = 1;		/* suppress trunc string */
		return (NULL);
	}
	/* successful return */
	if ((sizeof(astat) + ((u_char *)dp)) < snapend)
		return ((u_int32_t *) (sizeof(astat) + ((char *)dp)));

trunc:
	return (NULL);
}

static struct tok err2str[] = {
	{ NFSERR_PERM,		"Operation not permitted" },
	{ NFSERR_NOENT,		"No such file or directory" },
	{ NFSERR_IO,		"Input/output error" },
	{ NFSERR_NXIO,		"Device not configured" },
	{ NFSERR_ACCES,		"Permission denied" },
	{ NFSERR_EXIST,		"File exists" },
	{ NFSERR_XDEV,		"Cross-device link" },
	{ NFSERR_NODEV,		"Operation not supported by device" },
	{ NFSERR_NOTDIR,	"Not a directory" },
	{ NFSERR_ISDIR,		"Is a directory" },
	{ NFSERR_INVAL,		"Invalid argument" },
	{ NFSERR_FBIG,		"File too large" },
	{ NFSERR_NOSPC,		"No space left on device" },
	{ NFSERR_ROFS,		"Read-only file system" },
	{ NFSERR_MLINK,		"Too many links" },
	{ NFSERR_NAMETOL,	"File name too long" },
	{ NFSERR_NOTEMPTY,	"Directory not empty" },
	{ NFSERR_DQUOT,		"Disc quota exceeded" },
	{ NFSERR_STALE,		"Stale NFS file handle" },
	{ NFSERR_REMOTE,	"Too many levels of remote in path" },
	{ NFSERR_WFLUSH,	"Write cache flushed" },
	{ NFSERR_BADHANDLE,	"Illegal NFS file handle" },
	{ NFSERR_NOT_SYNC,	"Synchronization mismatch" },
	{ NFSERR_BAD_COOKIE,	"Stale NFS cookie" },
	{ NFSERR_NOTSUPP,	"Operation not supported" },
	{ NFSERR_TOOSMALL,	"Buffer or request is too small" },
	{ NFSERR_SERVERFAULT,	"Server fault" },
	{ NFSERR_BADTYPE,	"Object type not supported" },
	{ NFSERR_JUKEBOX,	"Operation in progress" },
	{ 0,			NULL }
};

static const u_int32_t *
parsestatus(const u_int32_t *dp)
{
	register int errnum;

	TCHECK(dp[0]);
	errnum = ntohl(dp[0]);
	if (errnum != 0) {
		if (!qflag)
			printf(" ERROR: %s", tok2str(err2str, "%u", errnum));
		nfserr = 1;		/* suppress trunc string */
		return (NULL);
	}
	return (dp + 1);
trunc:
	return (NULL);
}

#define	parsefattr(dp, v)	(nfsvers == NFS_VER2 ? parsefattr2(dp, v) : \
    parsefattr3(dp, v))

static const u_int32_t *
parsefattr2(const u_int32_t *dp, int verbose)
{
	const struct nfs_fattr *fap;

	fap = (const struct nfs_fattr *)dp;
	if (verbose) {
		TCHECK(fap->fa2_size);
		printf(" %s %o ids %u/%u sz %u",
		    tok2str(type2str, " unk-ft %d",
		    (u_int32_t)ntohl(fap->fa_type)),
		    (u_int32_t)ntohl(fap->fa_mode),
		    (u_int32_t)ntohl(fap->fa_uid),
		    (u_int32_t)ntohl(fap->fa_gid),
		    (u_int32_t)ntohl(fap->fa2_size));
	}
	/* print lots more stuff */
	if (verbose > 1) {
		TCHECK(fap->fa2_fileid);
		printf(" nlink %u rdev %x fsid %x nodeid %x a/m/ctime",
		    (u_int32_t)ntohl(fap->fa_nlink),
		    (u_int32_t)ntohl(fap->fa2_rdev),
		    (u_int32_t)ntohl(fap->fa2_fsid),
		    (u_int32_t)ntohl(fap->fa2_fileid));
		TCHECK(fap->fa2_atime);
		printf(" %u.%06u",
		    (u_int32_t)ntohl(fap->fa2_atime.nfsv2_sec),
		    (u_int32_t)ntohl(fap->fa2_atime.nfsv2_usec));
		TCHECK(fap->fa2_mtime);
		printf(" %u.%06u",
		    (u_int32_t)ntohl(fap->fa2_mtime.nfsv2_sec),
		    (u_int32_t)ntohl(fap->fa2_mtime.nfsv2_usec));
		TCHECK(fap->fa2_ctime);
		printf(" %u.%06u",
		    (u_int32_t)ntohl(fap->fa2_ctime.nfsv2_sec),
		    (u_int32_t)ntohl(fap->fa2_ctime.nfsv2_usec));
	}
	return ((const u_int32_t *)(&fap->fa_un.fa_nfsv2 + 1));
trunc:
	return (NULL);
}

static const u_int32_t *
parsefattr3(const u_int32_t *dp, int verbose)
{
	const struct nfs_fattr *fap;

	fap = (const struct nfs_fattr *)dp;
	if (verbose) {
		TCHECK(fap->fa3_size);
		printf(" %s %o ids %u/%u",
		    tok2str(type2str, "unk-ft %d ",
		    (u_int32_t)ntohl(fap->fa_type)),
		    (u_int32_t)ntohl(fap->fa_mode),
		    (u_int32_t)ntohl(fap->fa_uid),
		    (u_int32_t)ntohl(fap->fa_gid));
		printquad(" sz ", (u_int32_t *)&fap->fa3_size);
	}
	/* print lots more stuff */
	if (verbose > 1) {
		TCHECK(fap->fa3_fileid);
		printquad(" used ", (u_int32_t *)&fap->fa3_used);
		printf(" rdev %x/%x",
		    (u_int32_t)ntohl(fap->fa3_rdev.specdata1),
		    (u_int32_t)ntohl(fap->fa3_rdev.specdata2));
		printquad(" fsid ", (u_int32_t *)&fap->fa3_fsid);
		printquad(" nodeid ", (u_int32_t *)&fap->fa3_fileid);
		printf(" a/m/ctime");
		TCHECK(fap->fa3_atime);
		printf(" %u.%09u",
		    (u_int32_t)ntohl(fap->fa3_atime.nfsv3_sec),
		    (u_int32_t)ntohl(fap->fa3_atime.nfsv3_nsec));
		TCHECK(fap->fa3_mtime);
		printf(" %u.%09u",
		    (u_int32_t)ntohl(fap->fa3_mtime.nfsv3_sec),
		    (u_int32_t)ntohl(fap->fa3_mtime.nfsv3_nsec));
		TCHECK(fap->fa3_ctime);
		printf(" %u.%09u",
		    (u_int32_t)ntohl(fap->fa3_ctime.nfsv3_sec),
		    (u_int32_t)ntohl(fap->fa3_ctime.nfsv3_nsec));
	}
	return ((const u_int32_t *)(&fap->fa_un.fa_nfsv3 + 1));
trunc:
	return (NULL);
}

static int
parseattrstat(const u_int32_t *dp, int verbose)
{

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	return (parsefattr(dp, verbose) != NULL);
}

static int
parsediropres(const u_int32_t *dp)
{

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	dp = parsefh(dp);
	if (dp == NULL)
		return (0);

	return (parsefattr(dp, vflag) != NULL);
}

static const u_int32_t *
parsepostopattr(const u_int32_t *dp)
{

	TCHECK2(dp[0], sizeof(*dp));
	if (!ntohl(*dp++))
		return (dp);

	return (parsefattr3(dp, vflag));

trunc:
	return (NULL);
}

static const u_int32_t *
parsewccdata(register const u_int32_t *dp, int qflag)
{
	TCHECK2(dp[0], sizeof(*dp));
	if (ntohl(*dp++)) {
		TCHECK2(dp[0], 2 * sizeof(*dp));
		printquad(" sz ", dp);
		TCHECK2(dp[2], 2 * sizeof(*dp));
		printf(" mtime %u.%09u",
		    (u_int32_t)ntohl(dp[2]),
		    (u_int32_t)ntohl(dp[3]));
		TCHECK2(dp[4], 2 * sizeof(*dp));
		printf(" ctime %u.%09u",
		    (u_int32_t)ntohl(dp[4]),
		    (u_int32_t)ntohl(dp[5]));
		dp += 6;
	}
	return (parsepostopattr(dp));
	
trunc:
	return (NULL);
}

static int
parsecreateres(const u_int32_t *dp)
{

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	TCHECK2(dp[0], sizeof(*dp));
	if (ntohl(*dp++) && (dp = parsefh(dp)) == NULL)
		return (0);

	return ((dp = parsepostopattr(dp)) != NULL &&
	    parsewccdata(dp, !qflag) != NULL);

trunc:
	return (0);
}

static int
parselinkres(const u_int32_t *dp)
{
	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	switch (nfsvers) {
	case NFS_VER2:
		break;

	case NFS_VER3:
		dp = parsepostopattr(dp);
		if (dp == NULL)
			return (0);
		break;
	}

	putchar(' ');
	return (parsefn(dp) != NULL);
}

#define	parsestatfs(dp)	(nfsvers == NFS_VER2 ? parsestatfs2(dp) : parsestatfs3(dp))

static int
parsestatfs2(const u_int32_t *dp)
{
	const struct nfs_statfs *sfsp;

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	if (!qflag) {
		sfsp = (const struct nfs_statfs *)dp;
		TCHECK(sfsp->sf_bavail);
		printf(" tsize %u bsize %u blocks %u bfree %u bavail %u",
		    (u_int32_t)ntohl(sfsp->sf_tsize),
		    (u_int32_t)ntohl(sfsp->sf_bsize),
		    (u_int32_t)ntohl(sfsp->sf_blocks),
		    (u_int32_t)ntohl(sfsp->sf_bfree),
		    (u_int32_t)ntohl(sfsp->sf_bavail));
	}

	return (1);
trunc:
	return (0);
}

static int
parsestatfs3(const u_int32_t *dp)
{
	const struct nfs_statfs *sfsp;

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);

	if (!qflag) {
		sfsp = (const struct nfs_statfs *)dp;
		TCHECK(sfsp->sf_afiles);
		printquad(" tbytes ", (u_int32_t *)&sfsp->sf_tbytes);
		printquad(" fbytes ", (u_int32_t *)&sfsp->sf_fbytes);
		printquad(" abytes ", (u_int32_t *)&sfsp->sf_abytes);
		printquad(" tfiles ", (u_int32_t *)&sfsp->sf_tfiles);
		printquad(" ffiles ", (u_int32_t *)&sfsp->sf_ffiles);
		printquad(" afiles ", (u_int32_t *)&sfsp->sf_afiles);
	}

	return (1);
trunc:
	return (0);
}

static int
parserddires(const u_int32_t *dp, int plus)
{
	int count;

	dp = parsestatus(dp);
	if (dp == NULL)
		return (0);
	if (!qflag) {
		switch (nfsvers) {
		case NFS_VER2:
			for (;;) {
				TCHECK(dp[0]);
				if (!*dp++)
					break;
				TCHECK(dp[0]);
				if (vflag)
					printf(" nodeid %u", ntohl(*dp++));
				if ((dp = parsefn(dp)) == NULL)
					goto trunc;
				if (vflag)
					printquad(" cky ", dp);
				dp++;
			}
			if (dp[2] != 0)
				printf(" eof");
			/* XXX - format the entries */
			break;

		case NFS_VER3:
			if ((dp = parsepostopattr(dp)) == NULL)
				return (0);
			if (vflag)
				printquad(" ckyvrf ", dp);
			dp += 2;
			for (;;) {
				TCHECK(dp[0]);
				if (!*dp++)
					break;
				TCHECK2(dp[0], 2 * sizeof(*dp));
				if (vflag)
					printquad(" nodeid ", dp);
				dp += 2;
				if ((dp = parsefn(dp)) == NULL)
					goto trunc;
				if (vflag)
					printquad(" cky ", dp);
				dp += 2;
				if (!plus)
					continue;
				/* The following are only for readdir plus */
				if ((dp = parsepostopattr(dp)) == NULL)
					goto trunc;
				TCHECK(dp[0]);
				if (ntohl(*dp++) && (dp = parsefh(dp)) == NULL)
					goto trunc;
			}
			TCHECK(dp[0]);
			if (ntohl(*dp++))
				printf(" eof");
			break;
		}
	}

	return (1);
trunc:
	return (0);
}

static void
interp_reply(const struct rpc_msg *rp, u_int32_t proc, u_int length)
{
	register const u_int32_t *dp;

	(void)printf(" v%u", nfsvers);
	switch (proc) {

	case NFSPROC_NULL:
		printf(" null");
		return;

	case NFSPROC_GETATTR:
		printf(" getattr");
		dp = parserep(rp, length);
		if (dp != NULL && parseattrstat(dp, vflag) != 0)
			return;
		break;

	case NFSPROC_SETATTR:
		printf(" setattr");
		dp = parserep(rp, length);
		if (dp == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parseattrstat(dp, vflag) != 0)
				return;
			break;

		case NFS_VER3:
			if ((dp = parsestatus(dp)) != NULL &&
			    parsewccdata(dp, !qflag) != NULL)
				return;
			break;
		}
		break;

	case NFSPROC_LOOKUP:
		printf(" lookup");
		if ((dp = parserep(rp, length)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parsediropres(dp) != 0)
				return;
			break;

		case NFS_VER3:
			if ((dp = parsestatus(dp)) != NULL &&
			    (dp = parsefh(dp)) != NULL &&
			    (dp = parsepostopattr(dp)) != NULL &&
			    (dp = parsepostopattr(dp)) != NULL)
				return;
			break;
		}
		break;

	case NFSPROC_ACCESS:
		printf( " access");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL &&
		    (dp = parsepostopattr(dp)) != NULL &&
		    parseaccess(dp) != 0)
			return;
		break;

	case NFSPROC_READLINK:
		printf(" readlink");
		dp = parserep(rp, length);
		if (dp != NULL && parselinkres(dp) != 0)
			return;
		break;

	case NFSPROC_READ:
		printf(" read");
		dp = parserep(rp, length);
		if (dp == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parseattrstat(dp, vflag) != 0)
				return;
			break;
		case NFS_VER3:
			if ((dp = parsestatus(dp)) != NULL &&
			    (dp = parsepostopattr(dp)) != NULL) {
				TCHECK2(dp[0], sizeof(*dp));
				(void)printf(" %u bytes",
				    (u_int32_t)ntohl(dp[0]));
				TCHECK2(dp[1], sizeof(*dp));
				if (ntohl(dp[1]))
					(void)printf(" eof");
				return;
			}			    
			break;
		}
		break;

	case NFSPROC_WRITE:
		printf(" write");
		dp = parserep(rp, length);
		if (dp == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parseattrstat(dp, vflag) != 0)
				return;
			break;

		case NFS_VER3:
			if ((dp = parsestatus(dp)) != NULL &&
			    (dp = parsewccdata(dp, !qflag)) != NULL) {
				TCHECK2(dp[0], 2 * sizeof(*dp));
				(void)printf(" %u bytes %s",
				    (u_int32_t)ntohl(dp[0]),
				    tok2str(sync2str, "stable-%d",
					(u_int32_t)ntohl(dp[1])));
				return;
			}
			break;
		}
		break;

	case NFSPROC_CREATE:
		printf(" create");
		if ((dp = parserep(rp, length)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parsediropres(dp) != 0)
				return;
			break;

		case NFS_VER3:
			if (parsecreateres(dp) != 0)
				return;
			break;
		}
		break;

	case NFSPROC_MKDIR:
		printf(" mkdir");
		if ((dp = parserep(rp, length)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parsediropres(dp) != 0)
				return;
			break;

		case NFS_VER3:
			if (parsecreateres(dp) != 0)
				return;
			break;
		}
		break;

	case NFSPROC_SYMLINK:
		printf(" symlink");
		if ((dp = parserep(rp, length)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			if (parsestatus(dp) != 0)
				return;
			break;

		case NFS_VER3:
			if (parsecreateres(dp) != 0)
				return;
			break;
		}
		break;

	case NFSPROC_MKNOD:
		printf(" mknod");
		if ((dp = parserep(rp, length)) != NULL &&
		    parsecreateres(dp) != 0)
			return;
		break;

	case NFSPROC_REMOVE:
		printf(" remove");
		if ((dp = parserep(rp, length)) == NULL ||
			(dp = parsestatus(dp)) == NULL)
			break;
		switch (nfsvers) {
		case NFS_VER2:
			break;

		case NFS_VER3:
			if (parsewccdata(dp, !qflag) != NULL)
				return;
		}
		break;

	case NFSPROC_RMDIR:
		printf(" rmdir");
		if ((dp = parserep(rp, length)) == NULL ||
		    (dp = parsestatus(dp)) == NULL)	
			break;
		switch (nfsvers) {
		case NFS_VER2:
			break;

		case NFS_VER3:
			if (parsewccdata(dp, !qflag) != NULL)
				return;
		}
		break;

	case NFSPROC_RENAME:
		printf(" rename");
		if ((dp = parserep(rp, length)) == NULL ||
		    (dp = parsestatus(dp)) == NULL)	
			break;
		switch (nfsvers) {
		case NFS_VER2:
			break;

		case NFS_VER3:
			if ((dp = parsewccdata(dp, !qflag)) != NULL &&
			    parsewccdata(dp, !qflag) != NULL)
				return;
			break;
		}
		break;

	case NFSPROC_LINK:
		printf(" link");
		if ((dp = parserep(rp, length)) == NULL ||
		    (dp = parsestatus(dp)) == NULL)	
			break;
		switch (nfsvers) {
		case NFS_VER2:
			break;
		case NFS_VER3:
			if ((dp = parsepostopattr(dp)) != NULL &&
			    parsewccdata(dp, !qflag) != NULL)
				return;
			break;
		}
		break;

	case NFSPROC_READDIR:
		printf(" readdir");
		if ((dp = parserep(rp, length)) != NULL &&
		    parserddires(dp, FALSE) != 0)
			return;
		break;

	case NFSPROC_READDIRPLUS:
		printf(" readdirplus");
		if ((dp = parserep(rp, length)) != NULL &&
		    parserddires(dp, TRUE) != 0)
			return;
		break;

	case NFSPROC_FSSTAT:
		printf(" fsstat");
		dp = parserep(rp, length);
		if (dp != NULL && parsestatfs(dp) != 0)
			return;
		break;

	case NFSPROC_FSINFO:
		printf(" fsinfo");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL &&
		    parsepostopattr(dp) != NULL)
			return;
		break;

	case NFSPROC_PATHCONF:
		printf(" pathconf");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL &&
		    parsepostopattr(dp) != NULL)
			return;
		break;

	case NFSPROC_COMMIT:
		printf(" commit");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL &&
		    parsewccdata(dp, !qflag) != NULL)
			return;
		break;

	case NQNFSPROC_GETLEASE:
		printf(" getlease");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL)
			return;
		break;

	case NQNFSPROC_VACATED:
		printf(" vacated");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL)
			return;
		break;

	case NQNFSPROC_EVICTED:
		printf(" evicted");
		if ((dp = parserep(rp, length)) != NULL &&
		    (dp = parsestatus(dp)) != NULL)
			return;
		break;

	case NFSPROC_NOOP:
		printf(" nop");
		return;

	default:
		printf(" proc-%u", proc);
		return;
	}

trunc:
	if (!nfserr)
		fputs(" [|nfs]", stdout);
}
