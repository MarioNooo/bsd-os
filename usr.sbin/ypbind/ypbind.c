/*
 * Copyright (c) 1992/3 Theo de Raadt <deraadt@fsa.ca>
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef LINT
static char rcsid[] = "=Id: ypbind.c,v 1.15.4.1 1995/08/26 12:56:07 davidg Exp =";
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <netdb.h>
#include <string.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_rmt.h>
#include <unistd.h>
#include <stdlib.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#ifndef BINDINGDIR
#define BINDINGDIR "/var/yp/binding"
#endif

#ifndef YPBINDLOCK
#define YPBINDLOCK "/var/run/ypbind.lock"
#endif

struct _dom_binding {
	struct _dom_binding *dom_pnext;
	char dom_domain[YPMAXDOMAIN + 1];
	struct sockaddr_in dom_server_addr;
	long int dom_vers;
	int dom_lockfd;
	int dom_alive;
	int dom_broadcast_pid;
	int dom_pipe_fds[2];
	int dom_default;
};

#define READFD ypdb->dom_pipe_fds[0]
#define WRITEFD ypdb->dom_pipe_fds[1]
#define BROADFD broad_domain->dom_pipe_fds[1]

extern bool_t xdr_domainname(), xdr_ypbind_resp();
extern bool_t xdr_ypreq_key(), xdr_ypresp_val();
extern bool_t xdr_ypbind_setdom();

void	checkwork __P((void));
void	*ypbindproc_null_2 __P((SVCXPRT *, void *, CLIENT *));
void	*ypbindproc_setdom_2 __P((SVCXPRT *, struct ypbind_setdom *, CLIENT *));
void	rpc_received __P((char *, struct sockaddr_in *, int ));
void	broadcast __P((struct _dom_binding *));
int	ping __P((struct _dom_binding *));
int	tell_parent __P((char *, struct sockaddr_in *));
void	handle_children __P(( struct _dom_binding * ));
void	reaper __P((int));
void	terminate __P((int));
void	yp_restricted_mode __P((char *));
int	verify __P((struct in_addr));
int	rpc_getdtablesize __P((void));

char *domainname;
struct _dom_binding *ypbindlist;
static struct _dom_binding *broad_domain;

#define YPSET_NO	0
#define YPSET_LOCAL	1
#define YPSET_ALL	2
int ypsetmode = YPSET_NO;
int ypsecuremode = 0;

/*
 * Special restricted mode variables: when in restricted mode, only the
 * specified restricted_domain will be bound, and only the servers listed
 * in restricted_addrs will be used for binding.
 */
#define RESTRICTED_SERVERS 10
int yp_restricted = 0;
struct in_addr restricted_addrs[RESTRICTED_SERVERS];

/* No more than MAX_CHILDREN child broadcasters at a time. */
#ifndef MAX_CHILDREN
#define MAX_CHILDREN 5
#endif
/* No more than MAX_DOMAINS simultaneous domains */
#ifndef MAX_DOMAINS
#define MAX_DOMAINS 200
#endif
/* RPC timeout value */
#ifndef FAIL_THRESHOLD
#define FAIL_THRESHOLD 10
#endif

/* Number of times to fish for a response froma particular set of hosts */
#ifndef MAX_RETRIES
#define MAX_RETRIES 30
#endif

int retries = 0;
int children = 0;
int domains = 0;
int yplockfd;
fd_set fdsr;

SVCXPRT *udptransp, *tcptransp;

void *
ypbindproc_null_2(transp, argp, clnt)
SVCXPRT *transp;
void *argp;
CLIENT *clnt;
{
	static char res;

	bzero((char *)&res, sizeof(res));
	return (void *)&res;
}

struct ypbind_resp *
ypbindproc_domain_2(transp, argp, clnt)
SVCXPRT *transp;
char *argp;
CLIENT *clnt;
{
	static struct ypbind_resp res;
	struct _dom_binding *ypdb;
	char path[MAXPATHLEN];

	bzero((char *)&res, sizeof res);
	res.ypbind_status = YPBIND_FAIL_VAL;
	res.ypbind_respbody.ypbind_error = YPBIND_ERR_NOSERV;

	for(ypdb=ypbindlist; ypdb; ypdb=ypdb->dom_pnext) {
		if( strcmp(ypdb->dom_domain, argp) == 0)
			break;
		}

	if(ypdb==NULL) {
		if (yp_restricted) {
			syslog(LOG_NOTICE, "Running in restricted mode -- request to bind domain \"%s\" rejected.\n", argp);
			return &res;
		}

		if (domains >= MAX_DOMAINS) {
			syslog(LOG_WARNING, "domain limit (%d) exceeded",
							MAX_DOMAINS);
			res.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
			return &res;
		}
		ypdb = (struct _dom_binding *)malloc(sizeof *ypdb);
		if (ypdb == NULL) {
			syslog(LOG_WARNING, "malloc: %s", strerror(errno));
			res.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
			return &res;
		}
		bzero((char *)ypdb, sizeof *ypdb);
		strncpy(ypdb->dom_domain, argp, sizeof ypdb->dom_domain);
		ypdb->dom_vers = YPVERS;
		ypdb->dom_alive = 0;
		ypdb->dom_default = 0;
		ypdb->dom_lockfd = -1;
		sprintf(path, "%s/%s.%ld", BINDINGDIR,
					ypdb->dom_domain, ypdb->dom_vers);
		unlink(path);
		ypdb->dom_pnext = ypbindlist;
		ypbindlist = ypdb;
		domains++;
	}

	if (ping(ypdb)) {
		return &res;
	}

	res.ypbind_status = YPBIND_SUCC_VAL;
	res.ypbind_respbody.ypbind_error = 0; /* Success */
	res.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr.s_addr =
		ypdb->dom_server_addr.sin_addr.s_addr;
	res.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port =
		ypdb->dom_server_addr.sin_port;
	/*printf("domain %s at %s/%d\n", ypdb->dom_domain,
		inet_ntoa(ypdb->dom_server_addr.sin_addr),
		ntohs(ypdb->dom_server_addr.sin_port));*/
	return &res;
}

void *
ypbindproc_setdom_2(transp, argp, clnt)
SVCXPRT *transp;
struct ypbind_setdom *argp;
CLIENT *clnt;
{
	struct sockaddr_in *fromsin, bindsin;

	fromsin = svc_getcaller(transp);

	switch(ypsetmode) {
	case YPSET_LOCAL:
		if( fromsin->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
			svcerr_noprog(transp);
			return;
		}
		break;
	case YPSET_ALL:
		break;
	case YPSET_NO:
	default:
		svcerr_noprog(transp);
		return;
	}

	if(ntohs(fromsin->sin_port) >= IPPORT_RESERVED) {
		svcerr_noprog(transp);
		return;
	}

	if(argp->ypsetdom_vers != YPVERS) {
		svcerr_noprog(transp);
		return;
	}

	bzero((char *)&bindsin, sizeof bindsin);
	bindsin.sin_family = AF_INET;
	bindsin.sin_addr.s_addr = argp->ypsetdom_addr.s_addr;
	bindsin.sin_port = argp->ypsetdom_port;
	rpc_received(argp->ypsetdom_domain, &bindsin, 1);

	return;
}

static void
ypbindprog_2(rqstp, transp)
struct svc_req *rqstp;
register SVCXPRT *transp;
{
	union {
		char ypbindproc_domain_2_arg[MAXHOSTNAMELEN];
		struct ypbind_setdom ypbindproc_setdom_2_arg;
	} argument;
	struct authunix_parms *creds;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	switch (rqstp->rq_proc) {
	case YPBINDPROC_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) ypbindproc_null_2;
		break;

	case YPBINDPROC_DOMAIN:
		xdr_argument = xdr_domainname;
		xdr_result = xdr_ypbind_resp;
		local = (char *(*)()) ypbindproc_domain_2;
		break;

	case YPBINDPROC_SETDOM:
		switch(rqstp->rq_cred.oa_flavor) {
		case AUTH_UNIX:
			creds = (struct authunix_parms *)rqstp->rq_clntcred;
			if( creds->aup_uid != 0) {
				svcerr_auth(transp, AUTH_BADCRED);
				return;
			}
			break;
		default:
			svcerr_auth(transp, AUTH_TOOWEAK);
			return;
		}

		xdr_argument = xdr_ypbind_setdom;
		xdr_result = xdr_void;
		local = (char *(*)()) ypbindproc_setdom_2;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(transp, &argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	return;
}

/* Jack the reaper */
void reaper(sig)
int sig;
{
	int st;

	while(wait3(&st, WNOHANG, NULL) > 0)
		children--;
}

void terminate(sig)
int sig;
{
	struct _dom_binding *ypdb;
	char path[MAXPATHLEN];

	for(ypdb=ypbindlist; ypdb; ypdb=ypdb->dom_pnext) {
		close(ypdb->dom_lockfd);
		if (ypdb->dom_broadcast_pid)
			kill(ypdb->dom_broadcast_pid, SIGINT);
		sprintf(path, "%s/%s.%ld", BINDINGDIR,
			ypdb->dom_domain, ypdb->dom_vers);
		unlink(path);
	}
	close(yplockfd);
	unlink(YPBINDLOCK);
	pmap_unset(YPBINDPROG, YPBINDVERS);
	exit(0);
}

int
main(argc, argv)
int argc;
char **argv;
{
	char path[MAXPATHLEN];
	struct timeval tv;
	int i;
	DIR *dird;
	struct dirent *dirp;
	struct _dom_binding *ypdb;

	/* Check that another ypbind isn't already running. */
	if ((yplockfd = (open(YPBINDLOCK, O_RDONLY|O_CREAT, 0444))) == -1) {
		perror(YPBINDLOCK);
		exit(1);
	}

	if(flock(yplockfd, LOCK_EX|LOCK_NB) == -1 && errno == EWOULDBLOCK) {
		fprintf (stderr, "Another ypbind is already running. Aborting.\n");
		exit(1);
	}

	/* XXX domainname will be overriden if we use restricted mode */
	yp_get_default_domain(&domainname);
	if( domainname[0] == '\0') {
		fprintf(stderr, "domainname not set. Aborting.\n");
		exit(1);
	}

	for(i=1; i<argc; i++) {
		if( strcmp("-ypset", argv[i]) == 0)
			ypsetmode = YPSET_ALL;
		else if (strcmp("-ypsetme", argv[i]) == 0)
		        ypsetmode = YPSET_LOCAL;
		else if (strcmp("-s", argv[i]) == 0)
		        ypsecuremode++;
		else if (strcmp("-S", argv[i]) == 0 && argc > i)
			yp_restricted_mode(argv[i+1]);
	}

	/* blow away everything in BINDINGDIR (if it exists) */

	if ((dird = opendir(BINDINGDIR)) != NULL) {
		char path[MAXPATHLEN];
		while ((dirp = readdir(dird)) != NULL)
			if (strcmp(dirp->d_name, ".") &&
			    strcmp(dirp->d_name, "..")) {
				sprintf(path,"%s/%s",BINDINGDIR,dirp->d_name);
				unlink(path);
			}
		closedir(dird);
	}

#ifdef DAEMON
	if (daemon(0,0)) {
		perror("fork");
		exit(1);
	}
#endif

	pmap_unset(YPBINDPROG, YPBINDVERS);

	udptransp = svcudp_create(RPC_ANYSOCK);
	if (udptransp == NULL) {
		fprintf(stderr, "cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(udptransp, YPBINDPROG, YPBINDVERS, ypbindprog_2,
	    IPPROTO_UDP)) {
		fprintf(stderr, "unable to register (YPBINDPROG, YPBINDVERS, udp).\n");
		exit(1);
	}

	tcptransp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (tcptransp == NULL) {
		fprintf(stderr, "cannot create tcp service.\n");
		exit(1);
	}

	if (!svc_register(tcptransp, YPBINDPROG, YPBINDVERS, ypbindprog_2,
	    IPPROTO_TCP)) {
		fprintf(stderr, "unable to register (YPBINDPROG, YPBINDVERS, tcp).\n");
		exit(1);
	}

	/* build initial domain binding, make it "unsuccessful" */
	ypbindlist = (struct _dom_binding *)malloc(sizeof *ypbindlist);
	if (ypbindlist == NULL) {
		perror("malloc");
		exit(1);
	}
	bzero((char *)ypbindlist, sizeof *ypbindlist);
	strncpy(ypbindlist->dom_domain, domainname, sizeof ypbindlist->dom_domain);
	ypbindlist->dom_vers = YPVERS;
	ypbindlist->dom_alive = 0;
	ypbindlist->dom_lockfd = -1;
	ypbindlist->dom_default = 1;
	domains++;

	signal(SIGCHLD, reaper);
	signal(SIGTERM, terminate);

	openlog(argv[0], LOG_PID, LOG_DAEMON);

	/* Kick off the default domain */
	broadcast(ypbindlist);

	while(1) {
		fdsr = svc_fdset;

		tv.tv_sec = 60;
		tv.tv_usec = 0;

		switch(select(rpc_dtablesize(), &fdsr, NULL, NULL, &tv)) {
		case 0:
			checkwork();
			break;
		case -1:
			if (errno != EINTR)
				syslog(LOG_WARNING, "select: %s", strerror(errno));
			break;
		default:
			for(ypdb=ypbindlist; ypdb; ypdb=ypdb->dom_pnext) {
				if (READFD > 0 && FD_ISSET(READFD, &fdsr)) {
					handle_children(ypdb);
					if (children == (MAX_CHILDREN - 1))
						checkwork();
				}
			}
			svc_getreqset(&fdsr);
			break;
		}
	}
}

void
checkwork()
{
	struct _dom_binding *ypdb;

	for(ypdb=ypbindlist; ypdb; ypdb=ypdb->dom_pnext)
		ping(ypdb);
}

/* The clnt_broadcast() callback mechanism sucks. */

/*
 * Receive results from broadcaster. Don't worry about passing
 * bogus info to rpc_received() -- it can handle it. Note that we
 * must be sure to invalidate the dom_pipe_fds descriptors here:
 * since descriptors can be re-used, we have to make sure we
 * don't mistake one of the RPC descriptors for one of the pipes.
 * What's weird is that forgetting to invalidate the pipe descriptors
 * doesn't always result in an error (otherwise I would have caught
 * the mistake much sooner), even though logically it should.
 */
void handle_children(ypdb)
struct _dom_binding *ypdb;
{
	char buf[YPMAXDOMAIN + 1];
	struct sockaddr_in addr;

	if (read(READFD, &buf, sizeof(buf)) < 0)
		syslog(LOG_WARNING, "could not read from child: %s", strerror(errno));
	if (read(READFD, &addr, sizeof(struct sockaddr_in)) < 0)
		syslog(LOG_WARNING, "could not read from child: %s", strerror(errno));

	close(READFD);
	FD_CLR(READFD, &fdsr);
	FD_CLR(READFD, &svc_fdset);
	READFD = WRITEFD = -1;
	rpc_received((char *)&buf, &addr, 0);
}

/*
 * Send our dying words back to our parent before we perish.
 */
int
tell_parent(dom, addr)
char *dom;
struct sockaddr_in *addr;
{
	char buf[YPMAXDOMAIN + 1];
	struct timeval timeout;
	fd_set fds;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	sprintf(buf, "%s", broad_domain->dom_domain);
	if (write(BROADFD, &buf, sizeof(buf)) < 0)
		return(1);

	/*
	 * Stay in sync with parent: wait for it to read our first
	 * message before sending the second.
	 */

	FD_ZERO(&fds);
	FD_SET(BROADFD, &fds);
	if (select(FD_SETSIZE, NULL, &fds, NULL, &timeout) == -1)
		return(1);
	if (FD_ISSET(BROADFD, &fds)) {
		if (write(BROADFD, addr, sizeof(struct sockaddr_in)) < 0)
			return(1);
	} else {
		return(1);
	}

	close(BROADFD);
	return (0);
}

bool_t broadcast_result(out, addr)
bool_t *out;
struct sockaddr_in *addr;
{
	if (retries >= MAX_RETRIES) {
		bzero((char *)addr, sizeof(struct sockaddr_in));
		if (tell_parent(broad_domain->dom_domain, addr))
			syslog(LOG_WARNING, "lost connection to parent");
		return TRUE;
	}

	if (yp_restricted && verify(addr->sin_addr)) {
		retries++;
		syslog(LOG_NOTICE, "NIS server at %s not in restricted mode access list -- rejecting.\n",inet_ntoa(addr->sin_addr));
		return FALSE;
	} else {
		if (tell_parent(broad_domain->dom_domain, addr))
			syslog(LOG_WARNING, "lost connection to parent");
		return TRUE;
	}
}

/*
 * The right way to send RPC broadcasts.
 * Use the clnt_broadcast() RPC service. Unfortunately, clnt_broadcast()
 * blocks while waiting for replies, so we have to fork off seperate
 * broadcaster processes that do the waiting and then transmit their
 * results back to the parent for processing. We also have to remember
 * to save the name of the domain we're trying to bind in a global
 * variable since clnt_broadcast() provides no way to pass things to
 * the 'eachresult' callback function.
 */
void
broadcast(ypdb)
struct _dom_binding *ypdb;
{
	bool_t out = FALSE;
	enum clnt_stat stat;

	if (children >= MAX_CHILDREN || ypdb->dom_broadcast_pid)
		return;

	if (pipe(ypdb->dom_pipe_fds) < 0) {
		syslog(LOG_WARNING, "pipe: %s",strerror(errno));
		return;
	}

	if (ypdb->dom_vers = -1 && (long)ypdb->dom_server_addr.sin_addr.s_addr)
		syslog(LOG_WARNING, "NIS server [%s] for domain \"%s\" not responding",
		inet_ntoa(ypdb->dom_server_addr.sin_addr), ypdb->dom_domain);

	broad_domain = ypdb;
	flock(ypdb->dom_lockfd, LOCK_UN);

	switch((ypdb->dom_broadcast_pid = fork())) {
	case 0:
		close(READFD);
		break;
	case -1:
		syslog(LOG_WARNING, "fork: %s", strerror(errno));
		close(READFD);
		close(WRITEFD);
		return;
	default:
		close(WRITEFD);
		FD_SET(READFD, &svc_fdset);
		children++;
		return;
	}

	/* Release all locks before doing anything else. */
	while(ypbindlist) {
		close(ypbindlist->dom_lockfd);
		ypbindlist = ypbindlist->dom_pnext;
	}
	close(yplockfd);

	retries = 0;

	stat = clnt_broadcast(YPPROG, YPVERS, YPPROC_DOMAIN_NONACK,
	    xdr_domainname, (char *)ypdb->dom_domain, xdr_bool, (char *)&out,
	    broadcast_result);

	if (stat != RPC_SUCCESS) {
		bzero((char *)&ypdb->dom_server_addr,
						sizeof(struct sockaddr_in));
		if (tell_parent(ypdb->dom_domain, &ypdb->dom_server_addr))
			syslog(LOG_WARNING, "lost connection to parent");
	}

	exit(0);
}

/*
 * The right way to check if a server is alive.
 * Attempt to get a client handle pointing to the server and send a
 * YPPROC_DOMAIN. If we can't get a handle or we get a reply of FALSE,
 * we invalidate this binding entry and send out a broadcast to try to
 * establish a new binding. Note that we treat non-default domains
 * specially: once bound, we keep tabs on our server, but if it
 * goes away and fails to respond after one round of broadcasting, we
 * abandon it until a client specifically references it again. We make
 * every effort to keep our default domain bound, however, since we
 * need it to keep the system on its feet.
 */
int
ping(ypdb)
struct _dom_binding *ypdb;
{
	bool_t out;
	struct timeval interval, timeout;
	enum clnt_stat stat;
	int rpcsock = RPC_ANYSOCK;
	CLIENT *client_handle;
	time_t t;

	interval.tv_sec = FAIL_THRESHOLD;
	interval.tv_usec = 0;
	timeout.tv_sec = FAIL_THRESHOLD;
	timeout.tv_usec = 0;

	if (ypdb->dom_broadcast_pid)
		return(1);

	if ((client_handle = clntudp_bufcreate(&ypdb->dom_server_addr,
		YPPROG, YPVERS, interval, &rpcsock, RPCSMALLMSGSIZE,
		RPCSMALLMSGSIZE)) == (CLIENT *)NULL) {
		/* Can't get a handle: we're dead. */
		ypdb->dom_alive = 0;
		ypdb->dom_vers = -1;
		broadcast(ypdb);
		return(1);
	}

	if ((stat = clnt_call(client_handle, YPPROC_DOMAIN,
		xdr_domainname, (char *)ypdb->dom_domain, xdr_bool,
		(char *)&out, timeout)) != RPC_SUCCESS || out == FALSE) {
		ypdb->dom_alive = 0;
		ypdb->dom_vers = -1;
		clnt_destroy(client_handle);
		broadcast(ypdb);
		return(1);
	}

	clnt_destroy(client_handle);
	return(0);
}

void rpc_received(dom, raddrp, force)
char *dom;
struct sockaddr_in *raddrp;
int force;
{
	struct _dom_binding *ypdb, *prev = NULL;
	struct iovec iov[2];
	struct ypbind_resp ybr;
	char path[MAXPATHLEN];
	int fd;

	/*printf("returned from %s/%d about %s\n", inet_ntoa(raddrp->sin_addr),
	       ntohs(raddrp->sin_port), dom);*/

	if(dom==NULL)
		return;

	for(ypdb=ypbindlist; ypdb; ypdb=ypdb->dom_pnext) {
		if( strcmp(ypdb->dom_domain, dom) == 0)
			break;
		prev = ypdb;
	}

	if (ypdb && force) {
		if (ypdb->dom_broadcast_pid) {
			kill(ypdb->dom_broadcast_pid, SIGINT);
			close(READFD);
			FD_CLR(READFD, &fdsr);
			FD_CLR(READFD, &svc_fdset);
			READFD = WRITEFD = -1;
		}
	}

	/* if in secure mode, check originating port number */
	if ((ypsecuremode && (ntohs(raddrp->sin_port) >= IPPORT_RESERVED))) {
	    syslog(LOG_WARNING, "Rejected NIS server on [%s/%d] for domain %s.",
		   inet_ntoa(raddrp->sin_addr), ntohs(raddrp->sin_port),
		   dom);
	    if (ypdb != NULL) {
		ypdb->dom_broadcast_pid = 0;
		ypdb->dom_alive = 0;
	    }
	    return;
	}

	if (raddrp->sin_addr.s_addr == (long)0) {
		switch(ypdb->dom_default) {
		case 0:
			if (prev == NULL)
				ypbindlist = ypdb->dom_pnext;
			else
				prev->dom_pnext = ypdb->dom_pnext;
			sprintf(path, "%s/%s.%ld", BINDINGDIR,
				ypdb->dom_domain, YPVERS);
			close(ypdb->dom_lockfd);
			unlink(path);
			free(ypdb);
			domains--;
			return;
		case 1:
			ypdb->dom_broadcast_pid = 0;
			ypdb->dom_alive = 0;
			broadcast(ypdb);
			return;
		default:
			break;
		}
	}

	if(ypdb==NULL) {
		if (force == 0)
			return;
		ypdb = (struct _dom_binding *)malloc(sizeof *ypdb);
		if (ypdb == NULL) {
			syslog(LOG_WARNING, "malloc: %s", strerror(errno));
			return;
		}
		bzero((char *)ypdb, sizeof *ypdb);
		strncpy(ypdb->dom_domain, dom, sizeof ypdb->dom_domain);
		ypdb->dom_lockfd = -1;
		ypdb->dom_default = 0;
		ypdb->dom_pnext = ypbindlist;
		ypbindlist = ypdb;
	}

	/* We've recovered from a crash: inform the world. */
	if (ypdb->dom_vers = -1 && ypdb->dom_server_addr.sin_addr.s_addr)
		syslog(LOG_WARNING, "NIS server [%s] for domain \"%s\" OK",
		inet_ntoa(raddrp->sin_addr), ypdb->dom_domain);

	bcopy((char *)raddrp, (char *)&ypdb->dom_server_addr,
		sizeof ypdb->dom_server_addr);

	ypdb->dom_vers = YPVERS;
	ypdb->dom_alive = 1;
	ypdb->dom_broadcast_pid = 0;

	if(ypdb->dom_lockfd != -1)
		close(ypdb->dom_lockfd);

	sprintf(path, "%s/%s.%ld", BINDINGDIR,
		ypdb->dom_domain, ypdb->dom_vers);
#ifdef O_SHLOCK
	if( (fd=open(path, O_CREAT|O_SHLOCK|O_RDWR|O_TRUNC, 0644)) == -1) {
		(void)mkdir(BINDINGDIR, 0755);
		if( (fd=open(path, O_CREAT|O_SHLOCK|O_RDWR|O_TRUNC, 0644)) == -1)
			return;
	}
#else
	if( (fd=open(path, O_CREAT|O_RDWR|O_TRUNC, 0644)) == -1) {
		(void)mkdir(BINDINGDIR, 0755);
		if( (fd=open(path, O_CREAT|O_RDWR|O_TRUNC, 0644)) == -1)
			return;
	}
	flock(fd, LOCK_SH);
#endif

	/*
	 * ok, if BINDINGDIR exists, and we can create the binding file,
	 * then write to it..
	 */
	ypdb->dom_lockfd = fd;

	iov[0].iov_base = (caddr_t)&(udptransp->xp_port);
	iov[0].iov_len = sizeof udptransp->xp_port;
	iov[1].iov_base = (caddr_t)&ybr;
	iov[1].iov_len = sizeof ybr;

	bzero(&ybr, sizeof ybr);
	ybr.ypbind_status = YPBIND_SUCC_VAL;
	ybr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr = raddrp->sin_addr;
	ybr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port = raddrp->sin_port;

	if( writev(ypdb->dom_lockfd, iov, 2) != iov[0].iov_len + iov[1].iov_len) {
		syslog(LOG_WARNING, "write: %s", strerror(errno));
		close(ypdb->dom_lockfd);
		ypdb->dom_lockfd = -1;
		return;
	}
}

/*
 * Check address against list of allowed servers. Return 0 if okay,
 * 1 if not matched.
 */
int
verify(addr)
struct in_addr addr;
{
	int i;

	for (i = 0; i < RESTRICTED_SERVERS; i++)
		if (!bcmp((char *)&addr, (char *)&restricted_addrs[i],
			sizeof(struct in_addr)))
			return(0);

	return(1);
}

/*
 * Try to set restricted mode. We default to normal mode if we can't
 * resolve the specified hostnames.
 */
void
yp_restricted_mode(args)
char *args;
{
	struct hostent *h;
	int i = 0;
	char *s;

	/* Find the restricted domain. */
	if ((s = strsep(&args, ",")) == NULL)
		return;
	domainname = s;

	/* Get the addresses of the servers. */
	while ((s = strsep(&args, ",")) != NULL && i < RESTRICTED_SERVERS) {
		if ((h = gethostbyname(s)) == NULL)
			return;
		bcopy ((char *)h->h_addr_list[0], (char *)&restricted_addrs[i],
			sizeof(struct in_addr));
	i++;
	}

	/* ypset and ypsetme not allowed with restricted mode */
	ypsetmode = YPSET_NO;
	
	yp_restricted = 1;
	return;
}

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

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)rpc_dtablesize.c 1.2 87/08/11 Copyr 1987 Sun Micro";*/
/*static char *sccsid = "from: @(#)rpc_dtablesize.c	2.1 88/07/29 4.0 RPCSRC";*/
static char *rcsid = "rpc_dtablesize.c,v 1.1 1994/08/07 18:36:02 wollman Exp";
#endif

/*
 * Cache the result of getdtablesize(), so we don't have to do an
 * expensive system call every time.
 */
/*
 * XXX In FreeBSD 2.x, you can have the maximum number of open file
 * descriptors be greater than FD_SETSIZE (which us 256 by default).
 * This can lead to many RPC functions getting back an EINVAL from
 * select() and bombing all over the place.
 *
 * You can apparently get select() to handle values larger than 256
 * by patching the kernel, but most people aren't likely to know
 * that. Clamping this function at 256 is a kludge, but it'll have to
 * do until select()'s descriptor table size can be adjusted dynamically.
 */
rpc_dtablesize()
{
	static int size;

	if (size == 0) {
		size = getdtablesize();
		if (size > FD_SETSIZE)
			size = FD_SETSIZE;
	}
	return (size);
}