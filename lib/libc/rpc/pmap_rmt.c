/* @(#)pmap_rmt.c	2.2 88/08/01 4.0 RPCSRC */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)pmap_rmt.c 1.21 87/08/27 Copyr 1984 Sun Micro";
#endif

/*
 * pmap_rmt.c
 * Client interface to pmap rpc service.
 * remote call and broadcast service
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <errno.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_rmt.h>

#include <arpa/inet.h>

#define MAX_BROADCAST_SIZE 1400

extern int errno;
static struct timeval timeout = { 3, 0 };


/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
*/
enum clnt_stat
pmap_rmtcall(addr, prog, vers, proc, xdrargs, argsp, xdrres, resp, tout, port_ptr)
	struct sockaddr_in *addr;
	u_long prog, vers, proc;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	u_long *port_ptr;
{
	int socket = -1;
	register CLIENT *client;
	struct rmtcallargs a;
	struct rmtcallres r;
	enum clnt_stat stat;

	addr->sin_port = htons(PMAPPORT);
	client = clntudp_create(addr, PMAPPROG, PMAPVERS, timeout, &socket);
	if (client != (CLIENT *)NULL) {
		a.prog = prog;
		a.vers = vers;
		a.proc = proc;
		a.args_ptr = argsp;
		a.xdr_args = xdrargs;
		r.port_ptr = port_ptr;
		r.results_ptr = resp;
		r.xdr_results = xdrres;
		stat = CLNT_CALL(client, PMAPPROC_CALLIT, xdr_rmtcall_args,
		    (caddr_t) &a, xdr_rmtcallres, (caddr_t) &r, tout);
		CLNT_DESTROY(client);
	} else {
		stat = RPC_FAILED;
	}
	(void)close(socket);
	addr->sin_port = 0;
	return (stat);
}


/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR *xdrs;
	register struct rmtcallargs *cap;
{
	u_int lenposition, argposition, position;

	if (xdr_u_long(xdrs, &(cap->prog)) &&
	    xdr_u_long(xdrs, &(cap->vers)) &&
	    xdr_u_long(xdrs, &(cap->proc))) {
		lenposition = XDR_GETPOS(xdrs);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		argposition = XDR_GETPOS(xdrs);
		if (! (*(cap->xdr_args))(xdrs, cap->args_ptr))
		    return (FALSE);
		position = XDR_GETPOS(xdrs);
		cap->arglen = (u_long)position - (u_long)argposition;
		XDR_SETPOS(xdrs, lenposition);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		XDR_SETPOS(xdrs, position);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
bool_t
xdr_rmtcallres(xdrs, crp)
	register XDR *xdrs;
	register struct rmtcallres *crp;
{
	caddr_t port_ptr;

	port_ptr = (caddr_t)crp->port_ptr;
	if (xdr_reference(xdrs, &port_ptr, sizeof (u_long),
	    xdr_u_long) && xdr_u_long(xdrs, &crp->resultslen)) {
		crp->port_ptr = (u_long *)port_ptr;
		return ((*(crp->xdr_results))(xdrs, crp->results_ptr));
	}
	return (FALSE);
}


/*
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial 
 * routines which only support udp/ip .
 */

static int
compar(v1, v2)
	const void *v1;
	const void *v2;
{
	const struct in_addr *a1 = v1;
	const struct in_addr *a2 = v2;

	if (a1->s_addr == a2->s_addr)
		return (0);
	if (a1->s_addr > a2->s_addr)
		return (1);
	return (-1);
}

static int
getbroadcastnets(addrs)
	struct in_addr **addrs;
{
	struct ifaddrs *ifa0, *ifa;
	struct in_addr *alist, *dp, *ep, *sp;
	int n_addrs;

	n_addrs = 0;

	if (getifaddrs(&ifa0)) {
		perror("broadcast: getifaddrs");
		return (0);
	}

	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET ||
		    ifa->ifa_broadaddr == NULL ||
		    ifa->ifa_broadaddr->sa_family != AF_INET ||
		    (ifa->ifa_flags & (IFF_BROADCAST|IFF_UP)) !=
			(IFF_BROADCAST|IFF_UP))
			continue;
		n_addrs++;
	}

	if (n_addrs == 0)
		return (0);
	if ((alist = malloc(n_addrs * sizeof(struct in_addr))) == NULL) {
		freeifaddrs(ifa0);
		return (0);
	}

	ep = alist;
	for (ifa = ifa0; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET ||
		    ifa->ifa_broadaddr == NULL ||
		    ifa->ifa_broadaddr->sa_family != AF_INET ||
		    (ifa->ifa_flags & (IFF_BROADCAST|IFF_UP)) !=
		    (IFF_BROADCAST|IFF_UP))
			continue;
		*ep++ = ((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr;
	}
	freeifaddrs(ifa0);

	qsort(alist, n_addrs, sizeof(*alist), compar);

	dp = sp = alist;
	while (++sp < ep) {
		if (sp->s_addr == dp->s_addr)
			continue;
		if (++dp != sp)
			dp->s_addr = sp->s_addr;
	}

	*addrs = alist;
	return (dp - alist + 1);
}

typedef bool_t (*resultproc_t)();

enum clnt_stat 
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	enum clnt_stat stat;
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, fromlen, nets;
	register int sock;
	int on = 1;
#ifdef FD_SETSIZE
	fd_set mask;
	fd_set readfds;
#else
	int readfds;
	register int mask;
#endif /* def FD_SETSIZE */
	register int i;
	bool_t done = FALSE;
	register u_long xid;
	u_long port;
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct in_addr *addrs;
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	struct timeval t; 
	char outbuf[MAX_BROADCAST_SIZE], inbuf[UDPMSGSIZE];

	addrs = NULL;

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("Cannot create socket for broadcast rpc");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#ifdef SO_BROADCAST
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		perror("Cannot set socket option SO_BROADCAST");
		stat = RPC_CANTSEND;
		goto done_broad;
	}
#endif /* def SO_BROADCAST */
#ifdef FD_SETSIZE
	FD_ZERO(&mask);
	FD_SET(sock, &mask);
#else
	mask = (1 << sock);
#endif /* def FD_SETSIZE */
	nets = getbroadcastnets(&addrs);
	bzero((char *)&baddr, sizeof (baddr));
	baddr.sin_len = sizeof(baddr);
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(PMAPPORT);
	baddr.sin_addr.s_addr = htonl(INADDR_ANY);
/*	baddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); */
	(void)gettimeofday(&t, (struct timezone *)0);
	msg.rm_xid = xid = getpid() ^ t.tv_sec ^ t.tv_usec;
	t.tv_usec = 0;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = xargs;
	a.args_ptr = argsp;
	r.port_ptr = &port;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	xdrmem_create(xdrs, outbuf, MAX_BROADCAST_SIZE, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);
	xdr_destroy(xdrs);
	/*
	 * Basic loop: broadcast a packet and wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
		for (i = 0; i < nets; i++) {
			baddr.sin_addr = addrs[i];
			if (sendto(sock, outbuf, outlen, 0,
				(struct sockaddr *)&baddr,
				sizeof (struct sockaddr)) != outlen) {
				perror("Cannot send broadcast packet");
				stat = RPC_CANTSEND;
				goto done_broad;
			}
		}
		if (eachresult == NULL) {
			stat = RPC_SUCCESS;
			goto done_broad;
		}
	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.where = (caddr_t)&r;
                msg.acpted_rply.ar_results.proc = xdr_rmtcallres;
		readfds = mask;
		switch (select(_rpc_dtablesize(), &readfds, NULL, 
			       NULL, &t)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			continue;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			perror("Broadcast select problem");
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */
	try_again:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(sock, inbuf, UDPMSGSIZE, 0,
			(struct sockaddr *)&raddr, &fromlen);
		if (inlen < 0) {
			if (errno == EINTR)
				goto try_again;
			perror("Cannot receive reply to broadcast");
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_long))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, (u_int)inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short)port);
				done = (*eachresult)(resultsp, &raddr);
			}
			/* otherwise, we just ignore the errors ... */
		} else {
#ifdef notdef
			/* some kind of deserialization problem ... */
			if (msg.rm_xid == xid)
				fprintf(stderr, "Broadcast deserialization problem");
			/* otherwise, just random garbage */
#endif
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		(void)(*xresults)(xdrs, resultsp);
		xdr_destroy(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
done_broad:
	(void)close(sock);
	AUTH_DESTROY(unix_auth);
	if (addrs != NULL)
		free(addrs);
	return (stat);
}

