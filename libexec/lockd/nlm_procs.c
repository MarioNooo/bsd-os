/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI nlm_procs.c,v 1.4 2001/10/03 17:29:55 polk Exp
 */
/*
 * Copyright (c) 1995
 *	A.R. Gordon (andrew.gordon@net-tel.co.uk).  All rights reserved.
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
 *	This product includes software developed for the FreeBSD project
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREW GORDON AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <sys/ucred.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <rpc/rpc.h>

#include <stddef.h>
#include <sys/queue.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include "nlm_prot.h"
#include "lockd.h"
#include "nfs/nfs_lock.h"



#define	LOG_CALL(func, rqstp)						\
	if (d_calls)							\
		syslog(LOG_DEBUG, "%s: from %s", func,			\
		    from_addr(svc_getcaller((rqstp)->rq_xprt)));
#define	LOG_CALL_ST(func, rqstp, st)					\
	if (d_calls)							\
		syslog(LOG_DEBUG, "%s: from %s: %s", func,		\
		    from_addr(svc_getcaller((rqstp)->rq_xprt)),		\
		    show_state(st));
#define	LOG_CALL_4ST(func, rqstp, st)					\
	if (d_calls)							\
		syslog(LOG_DEBUG, "%s: from %s: %s", func,		\
		    from_addr(svc_getcaller((rqstp)->rq_xprt)),		\
		    show_4state(st));
#define	LOG_RES(st)							\
	if (d_calls)							\
		syslog(LOG_DEBUG, st == nlm_denied ?			\
		    "result: %s: %m" : "result: %s", show_state(st));
#define	LOG_4RES(st)							\
	if (d_calls)							\
		syslog(LOG_DEBUG, st == NLM4_DENIED ?			\
		    "result: %s: %m" : "result: %s", show_4state(st));
#define	LOG_PCNFS_RES(st)						\
	if (d_calls)							\
		syslog(LOG_DEBUG, st == nlm_denied ?			\
		    "result: %s: %m" : "result: %s", show_state(st));
#define	LOG_PCNFS_4RES(st)						\
	if (d_calls)							\
		syslog(LOG_DEBUG, st == NLM4_DENIED ?			\
		    "result: %s: %m" : "result: %s", show_4state(st));

#define MALLOC(N)	((N *)malloc(sizeof(N)))
static char hex[] = "0123456789abcdef";

static void  show_key(DBT *);
static void  show_fh(char *, netobj *);
static void  show_lock(struct nlm4_lock *);
static void  transmit_4result(int, nlm4_res *, struct svc_req *);
static void  transmit_result(int, nlm_res *, struct svc_req *);

/*
 * Functions for NLM locking (i.e., monitored locking, with statd involved
 * to ensure reclaim of locks after a crash of the "stateless" server.  These
 * all come in three flavours: nlm_xxx(), nlm_xxx_msg(), and nlm_xxx_res().
 *
 * The first (nlm_xxx()) are standard RPCs with argument and result.  The
 * second (nlm_xxx_msg()) calls implement exactly the same functions, but
 * use two pseudo-RPCs (one in each direction).  These calls aren't standard
 * use of the RPC protocol in that they do not return a result at all  (This
 * is quite different from returning a void result).  The effect of this is
 * to make the nlm_xxx_msg() calls simple unacknowledged datagrams, requiring
 * higher-level code to perform retries.
 *
 * Despite the disadvantages of the nlm_xxx_msg() approach (some of which
 * are documented in the comments to get_client()), this is the interface
 * used by all current commercial NFS implementations [Solaris, SCO, AIX
 * etc.].  This is presumed to be because these allow implementations to
 * continue using the standard RPC libraries, while avoiding the standard
 * block-until-result nature of the library interface.
 *
 * No client implementations have been identified so far that make use
 * of the true RPC version (early SunOS releases would be a likely candidate
 * for testing).
 *
 * The third (nlm_xxx_res()) are client-side pseudo-RPCs for results.  Note
 * that for the client there are only nlm_xxx_msg() versions of each call,
 * since the 'real RPC' version returns the results in the RPC result, and
 * so the client does not normally receive incoming RPCs.
 *
 * The exception to this is nlm_granted(), which is genuinely an RPC call from
 * the server to the client -- a 'call-back' in normal procedure call terms.
 *
 * The first group are NLM versions 1-3, the second group are NLM version 4.
 * The naming conventions are the same.
 */

/*
 * This wrapper makes calling nfslockdreq() easier.
 */
#define my_nfslockdreq(_req, _op, _owner, _owner_ret, _saddrp, _fh, _fh_len, \
				_offset, _len, _type, _pid) \
	(_req.op = (_op), \
	_req.owner = (_owner), \
	_req.owner_ret = (_owner_ret), \
	_req.saddr = *(_saddrp), \
	_req.fh = (_fh), \
	_req.fh_len = (_fh_len), \
	_req.offset = (_offset), \
	_req.len = (_len), \
	_req.type = (_type), \
	_req.pid = (_pid), \
	nfslockdreq(LOCKD_REQ_VERSION, &_req))


#define RETRY_INTERVAL	10		/* 10 seconds */
int	alarm_running = 0;
unsigned nlm_clock;	/* bumped on every SIGALARM */

enum nlmvers { nlmvers1, nlmvers4 };

struct blocked_l {
	unsigned	timer;
	char		*host;
	int		host_len;
	void		*oh;
	size_t		oh_len;
	enum nlmvers	nlmvers;
	int		async;
	struct lockd_req req;
	struct owner_l	*ownerp;
	netobj		cookie;
	LIST_ENTRY(blocked_l) next;
};

LIST_HEAD(blocked_l_head, blocked_l) bhead;

/* head up the list of locks blocked waiting for a lock */
struct blocked_l_head *bheadp = &bhead;

/* head up the list of locks waiting for the client to be informed of status */
struct blocked_l_head whead, *wheadp = &whead;

/*
 *
 * We link all the owner's onto a linked list which hangs off the 
 * entry in the host DB.
 */

LIST_HEAD(owner_l_head, owner_l);

struct owner_l {
	int			owner;
	struct blocked_l	*queue;
	int			pid;
	LIST_ENTRY(owner_l)	next;
};

struct owner_key {
	int		pid;
	int		host_len;
	char		host[LM_MAXSTRLEN+1];
};

/*
 * We want a length that understands the variable length host field.
 */
#define OWNER_KEY_SIZE(owner, len) \
	((int)((char *)&(owner).host - (char *)&(owner)) + (len) + 1)

int
in_grace_period()
{
	if (time(NULL) <= grace_end) {
		return TRUE;
	} else {
		check_grace = FALSE;
		return FALSE;
	}
}

int
extract_cred(struct svc_req *rqstp, struct ucred *ucred)
{
	struct authunix_parms *unix_cred;
	int *au_gid_p, *au_stop;
	gid_t *uc_gid_p, *uc_stop;
	gid_t gid;

	switch (rqstp->rq_cred.oa_flavor) {
		case AUTH_UNIX:
			unix_cred =
			    (struct authunix_parms *)rqstp->rq_clntcred;
			ucred->cr_uid = unix_cred->aup_uid;

			/*
			 * the gid has it's own entry in the authunix_parm
			 * structure, but ucred just uses the first entry in
			 * the array.  The authunix_parm gid entry and the
			 * first entry in the group list should be the same,
			 * but what happens if it isn't?  The for loop
			 * below deals with it, but it may be overly paranoid
			 * and thus non-optimal.
			 */
			ucred->cr_groups[0] = gid = unix_cred->aup_gid;
			if (unix_cred->aup_len < 1) {
				ucred->cr_ngroups = 1;
				return (0);
			}

			/*
			 * Copy over as many group entries as will fit.
			 * Skip the gid which we already copied above.
			 */
			uc_gid_p = &ucred->cr_groups[1];
			uc_stop = &ucred->cr_groups[NGROUPS - 1];
			au_gid_p = unix_cred->aup_gids;
			au_stop = &unix_cred->aup_gids[unix_cred->aup_len - 1];
			while (uc_gid_p <= uc_stop && au_gid_p <= au_stop) {
				if ((*uc_gid_p = *au_gid_p++) != gid) {
					uc_gid_p++;
					continue;
				}
			}
			ucred->cr_ngroups = uc_gid_p - ucred->cr_groups;
		break;
		case AUTH_NULL:
		default:        /* return weak authentication error */
			svcerr_weakauth(rqstp->rq_xprt);
			return (-1);
	}
	return (0);
}

void
ownerdb_del(
	DBT 		*key,
	struct owner_l	*ownerp
)
{
	int ret;

	ret = ownerdb->del(ownerdb, key, 0);
	if (ret == 0) {			/* deleted from db, expected result */
		LIST_REMOVE(ownerp, next);
		free(ownerp);
		if (d_cache) {
			syslog(LOG_DEBUG, "deleted owner %7d/%s", 
				((struct owner_key *)key->data)->pid, 
				((struct owner_key *)key->data)->host);
			dump_ownerdb();
		}
	} else if (ret == -1) { 	/* delete failed for some reason */
		/*
		 * The owner descriptor is left as -1, which will
		 * cause an owner descriptor to be
		 * allocated the next time this owner pid/host pair
		 * is used.  If the owner pid/host pair
		 * is never used, well then, we've just used some memory 
		 * we'll never free -- which is better then
		 * exiting... I think.
		 */
		syslog(LOG_ERR, "ownerdb: %m");
	} else if (ret == 1) { 	/* could not find entry ... we are confused */
		syslog(LOG_ERR, "ownerdb: lost owner, exiting");
		exit(-1);
	} else {		/* should never get here */
		syslog(LOG_ERR, "ownerdb: del returned (%d)", ret);
	}
}

struct owner_l *
owner_get(
	char	*caller_name,
	int	pid
)
{
	DBT key, data;
	int result;
	struct owner_key owner_k;
	int host_len;

	memset((void *)&owner_k, '\0', (char *)owner_k.host - (char *)&owner_k);
	host_len = strlen(caller_name);
	owner_k.pid = pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, caller_name, host_len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, host_len);

	if (d_cache) {
		syslog(LOG_DEBUG, "lookup owner %7d/%s", 
			owner_k.pid, owner_k.host);
		show_key(&key);
	}

	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb: %m, exiting");
			exit(-1);
		case 1:
			if (d_cache)
				syslog(LOG_DEBUG, "didn't find owner %7d/%s", 
					owner_k.pid, owner_k.host);

			/* not in our database, so nothing to unlock */
			return (NULL);
		case 0:
			return (*(struct owner_l **)data.data);
		default:
			syslog(LOG_ERR, "ownerdb: get returned (%d), exiting",
							result);
			exit(-1);
	}
}
/*
 * Clear a lock on a file.  Nobody really cares if we succeed, as they all
 * have to report success regardless.
 */
void
clearlock(
	struct svc_req	*rqstp,
	char		*caller_name,
	int		pid,
	void            *fh,
        size_t          fh_len,
        u_quad_t        offset,
        u_quad_t        len
)
{
	struct lockd_req req;
	struct owner_l *ownerp;
	DBT key, data;
	int result;
	struct owner_key owner_k;
	int host_len;

	/*
	 * Get the owner descripter already allocated for this owner 
	 * handle, and pass it in the lockd request.
	 */

	memset((void *)&owner_k, '\0', (char *)owner_k.host - (char *)&owner_k);
	host_len = strlen(caller_name);
	owner_k.pid = pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, caller_name, host_len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, host_len);

	if (d_cache) {
		syslog(LOG_DEBUG, "lookup owner %7d/%s", 
			owner_k.pid, owner_k.host);
		show_key(&key);
	}

	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb: %m, exiting");
			exit(-1);
		case 1:
			if (d_cache)
				syslog(LOG_DEBUG, "didn't find owner %7d/%s", 
					owner_k.pid, owner_k.host);

			/* not in our database, so nothing to unlock */
			return;
		case 0:
			ownerp = *(struct owner_l **)data.data;
			break;
		default:
			syslog(LOG_ERR, "ownerdb: get returned (%d), exiting",
							result);
			exit(-1);
	}
	if (extract_cred(rqstp, &req.cred) < 0) {
		return;
	}

	/*
	 * An unlock clears any queues we might be waiting on.
	 */
	if (ownerp->queue) {
		LIST_REMOVE(ownerp->queue, next);
		free(ownerp->queue);
		ownerp->queue = NULL;
	}
	req.owner_rel_ok = 1;
	if (my_nfslockdreq(req, F_UNLCK, ownerp->owner, &ownerp->owner,
	    (struct sockaddr *)svc_getcaller(rqstp->rq_xprt),
	    fh, fh_len, offset, len, F_UNLCK, ownerp->pid) == 0) {

		/*
		 * Our unlock succeeded, and if owner is -1, then no more
		 * locks exist for this owner, and it was deallocated, so
		 * remove it from our owner database.
		 */
		if (ownerp->owner == -1) {
			ownerdb_del(&key, ownerp);
		}
	}
}

/*
 * Request the local status monitor to notify us if it is informed the
 * requested host's status changes.
 *
 */
int
sm_mon(
	char *host
)
{
	int ret;
        struct sm_stat_res clnt_res;

	mon_host.mon_id.mon_name = host;
	if ((ret = callrpc("localhost", SM_PROG, SM_VERS, SM_MON, 
			(xdrproc_t) xdr_mon, (caddr_t) &mon_host,
			(xdrproc_t) xdr_sm_stat_res, 
			(caddr_t) &clnt_res)) != 0) {
		syslog(LOG_ERR, "rpc to local status monitor failed : %s",
						clnt_sperrno(ret));
		return (-1);
	}

	if (clnt_res.res_stat == stat_fail) {
		syslog(LOG_ERR, "local status monitor refused to monitor %s",
						host);
		return (-1);
	}
	
	return 0;
}

/*
 * Find the head of the owner list for the named host (based on caller_name).
 */
struct owner_l_head *
owner_headp_get(
	char			*host,
	int			host_len,
	int			create		/* create if it doesn't exist */
)
{
	struct owner_l_head	*headp;
	DBT 			key, data;
	int 			result;

	/*
	 * Get the head pointer for the host list.
	 * If this is a new host, add it to our database.
	 */

	if (d_cache)
		syslog(LOG_DEBUG, "owner_headp_get(%s)", host);

	key.data = host;
	key.size = host_len + 1;
	switch (result = hostdb->get(hostdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "hostdb->get: %m, exiting");
			exit(-1);
		case 1:
			if (! create)
				return NULL;

			if (sm_mon(host) < 0)
				return NULL;

			if ((headp = MALLOC(struct owner_l_head)) == NULL)
				return NULL;

			LIST_INIT(headp);
			data.data = (void *)&headp;
			data.size = sizeof(headp);
			switch (result = hostdb->put(hostdb, &key, &data, 
							R_NOOVERWRITE)) {
				case -1:
					syslog(LOG_ERR, 
						"hostdb->put: %m, exiting");
					exit(-1);
				case 1:
					syslog(LOG_ERR, 
					    "hostdb->put: duplicate, exiting");
					exit(-1);
				case 0:
					return headp;
				default:
					syslog(LOG_ERR, "hostdb->put: returned %d, exiting",
						result);
					exit(-1);
			}
			break;
		case 0:
			return (*(struct owner_l_head **)data.data);
		default:
			syslog(LOG_ERR, "hostdb->get return %d, exiting",
									result);
			exit(-1);
	}
}

/*
 * Search for an entry in the blocked queue...
 */
struct blocked_l *
queue_search(
	struct blocked_l_head *headp,
	struct lockd_req *req
)
{
	struct blocked_l *p;

	/* XXX we ignore the owner handles... */
	for (p = LIST_FIRST(headp);
	     p != LIST_END(headp);
	     p = LIST_NEXT(p, next)) {
		if (p->req.owner == req->owner &&
		    p->req.offset == req->offset &&
		    p->req.len == req->len &&
		    p->req.fh_len == req->fh_len &&
		    memcmp(p->req.fh, req->fh, req->fh_len) == 0 &&
		    p->req.type == req->type) {
			return (p); 	/* found it */
		}
	}
	return (NULL); 	/* not in the blocked queue */
}

/*
 *  Queue a lock request into the blocked list.
 */
int
blocked_enqueue(
	enum nlmvers	nlmvers,
	int		async,
	char		*host,
	int		host_len,
	void		*oh,
	size_t		oh_len,
	struct owner_l *ownerp,
	netobj		*cookie,
	struct lockd_req *req
)
{
	char *buf;
	struct blocked_l *p;

	/* XXX paranoa ... this should never happen */
	if (ownerp->owner == -1) {
		syslog(LOG_DEBUG, "bad owner");
		return -1;
	}
	/*
	 * If this request is already queued, we're done.
	 */
	if (queue_search(bheadp, req) != NULL)
		return 0;

	/* malloc one buffer to hold everything */
	buf = (char *)malloc(sizeof(struct blocked_l) + req->fh_len + oh_len + 
				cookie->n_len + host_len + 1);
	if (buf == NULL)
		return -1;

	p = (struct blocked_l *)buf;
	buf += sizeof(struct blocked_l);
	p->req = *req;
	p->req.owner = ownerp->owner;
	p->req.fh = buf;
	buf += req->fh_len;
	memcpy(p->req.fh, req->fh, req->fh_len);
	if (oh_len == 0) {
		p->oh = NULL;
	} else {
		p->oh = buf;
		buf += oh_len;
		memcpy(p->oh, oh, oh_len);
	}
	p->oh_len = oh_len;

	if (cookie->n_len == 0)
		p->cookie.n_bytes = NULL;
	else {
		p->cookie.n_bytes = buf;
		buf += cookie->n_len;
		memcpy(p->cookie.n_bytes, cookie->n_bytes, cookie->n_len);
	}
	p->cookie.n_len = cookie->n_len;
	p->host = buf;
	memcpy(p->host, host, host_len+1);
	p->host_len = host_len;
	p->nlmvers = nlmvers;
	p->async = async;
	p->ownerp = ownerp;
	ownerp->queue = p;
	LIST_INSERT_HEAD(bheadp, p, next);
#ifdef NDEF
	if (! alarm_running) {
		alarm_running = 1;
		alarm(1);
	}
#endif
	return 0;
}

void
alarm_handler(
	int sig
)
{
	nlm_clock++;
	if (alarm_running == 1)
		alarm(1);
}


void
owner_unlock(
	struct blocked_l *p
)
{
	p->req.op = F_UNLCK;
	*(p->req.owner_ret) = 0;
	p->req.owner_rel_ok = p->ownerp->queue == NULL;
	if (nfslockdreq(LOCKD_REQ_VERSION, &p->req) == 0) {

		/*
		 * Our unlock succeeded, and if owner is -1, then no more
		 * locks exist for this owner, and it was deallocated, so
		 * remove it from our owner database.
		 */
		if (*(p->req.owner_ret) == -1) {
			struct owner_key owner_k;
			DBT key;

			memset((void *)&owner_k, '\0', 
				(char *)owner_k.host - (char *)&owner_k);
			owner_k.pid = p->req.pid;
			owner_k.host_len = p->host_len;
			memcpy((void *)owner_k.host, p->host, p->host_len+1);
			key.data = &owner_k;
			key.size = OWNER_KEY_SIZE(owner_k, p->host_len);

			ownerdb_del(&key, p->ownerp);
		}
	}
}

void
send_granted(
	struct blocked_l *p
)
{
	nlm4_res	*res;
	CLIENT		*cli;

	/* XXX short cut to local host */
	if (strcasecmp(my_hostname, p->host) == 0) {
		lock_answer(p->req.pid, &p->cookie, NLM4_GRANTED, NULL,
								NLM4_VERS);
		return;
	}

	cli = get_client(NULL, (struct sockaddr_in *)&p->req.saddr,
						NLM_PROG, NLM_VERS);
	if (cli == NULL) {
		if (p->ownerp->queue != NULL) {
			p->ownerp->queue = NULL; 
			LIST_REMOVE(p, next);
		}
		owner_unlock(p);
		free(p);
		return;
	}

	if (p->nlmvers == nlmvers4) {
		nlm4_testargs	args;

		args.alock.fh.n_bytes = p->req.fh;
		args.alock.fh.n_len = p->req.fh_len;
		args.alock.oh.n_bytes = p->oh;
		args.alock.oh.n_len = p->oh_len;
		args.alock.l_offset = p->req.offset;
		args.alock.l_len = p->req.len;
		args.alock.svid = p->req.pid;
		args.alock.caller_name = p->host;
		args.cookie = p->cookie;
		args.exclusive = (p->req.type == F_WRLCK);

		if (! p->async) {
			/* XXX should enqueue this for later send via
			 * nlm_after(), or single threaded servers
			 * could get wedged.
			 */
			res = nlmproc4_granted_4(&args, cli);
		} else {
			/* XXX not implemented yet */
			res = NULL;
		}
	}
	else {
		nlm_testargs	args;

		args.alock.fh.n_bytes = p->req.fh;
		args.alock.fh.n_len = p->req.fh_len;
		args.alock.oh.n_bytes = p->oh;
		args.alock.oh.n_len = p->oh_len;
		args.alock.l_offset = (unsigned)p->req.offset;
		args.alock.l_len = (unsigned)p->req.len;
		args.alock.svid = p->req.pid;
		args.alock.caller_name = p->host;
		args.cookie = p->cookie;
		args.exclusive = (p->req.type == F_WRLCK);

		if (! p->async) {
			res = (nlm4_res *)nlm_granted_1(&args, cli);
		} else {
			/* XXX not implemented yet */
			res = NULL;
		}
	}

	if (res == NULL) {
		/* timed out, retry later */
		if (p->ownerp->queue == NULL) {
			p->ownerp->queue = p;
			LIST_INSERT_HEAD(wheadp, p, next);
		}
		p->timer = nlm_clock + RETRY_INTERVAL;
	} else {
		/* they now know about the lock request status */
		if (p->ownerp->queue != NULL) {
			p->ownerp->queue = NULL;
			LIST_REMOVE(p, next);
		}
		if (res->stat.stat != NLM4_GRANTED) {
			owner_unlock(p);
		}
		free(p);
	}
}

void
delete_owner(
	struct owner_l	*ownerp,
	char		*host,
	size_t		host_len
)
{
	struct owner_key owner_k;
	DBT key;

	memset((void *)&owner_k, '\0', 
			(char *)owner_k.host - (char *)&owner_k);
	owner_k.pid = ownerp->pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, host, host_len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, host_len);
	ownerdb_del(&key, ownerp);
}

void
blocked_try(struct blocked_l *p)
{
	int owner;
	
	p->req.op = F_SETLK;
	p->req.owner_ret = &owner;
	p->req.owner_rel_ok = FALSE;	/* don't release owner numbers */
	if (nfslockdreq(LOCKD_REQ_VERSION, &p->req) == 0) {
		send_granted(p);
	} else if (errno != EAGAIN) {
		if (owner != -1)
			syslog(LOG_ERR, "kernel didn't release nfs owner %d",
				owner);
		/*
		 * no way to report a failure (as per the NLM protocol),
		 * so just drop it.
		 */
		delete_owner(p->ownerp, p->host, p->host_len);
	}
}

/*
 * Test if a file has a lock which would block us.
 */
nlm4_stats
getlock(
	struct svc_req	*rqstp,
	char		*caller_name,
	int		pid,
	void            *fh,
        size_t          fh_len,
        u_quad_t        offset,
        u_quad_t        len,
        int             type,
	int		async,
	int		*pid_ret
)
{
	struct lockd_req req;
	struct owner_l *ownerp;
	struct owner_key owner_k;
	DBT key, data;
	int result;
	int host_len;
	int owner;

	/*
	 * if we can extract a cred use it, otherwise map it to a user
	 * with minimal permissions.  If we get an error the NLM_TEST routines,
	 * we can only return NLM_GRANTED anyway (idiotic protocol)...
	 */
	if (extract_cred(rqstp, &req.cred) < 0) {
		req.cred.cr_uid = nobody;
		req.cred.cr_groups[0] = nogroup;
		req.cred.cr_ngroups = 1;
	}

	/*
	 * If we have an owner descripter already allocated for this owner 
	 * handle, get it, and pass it in the lockd request.
	 */

	memset((void *)&owner_k, '\0', (char *)owner_k.host - (char *)&owner_k);
	host_len = strlen(caller_name);
	owner_k.pid = pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, caller_name, host_len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, host_len);
	
	if (d_cache) {
		syslog(LOG_DEBUG, "lookup owner %7d/%s", 
			owner_k.pid, owner_k.host);
		show_key(&key);
	}

	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb->get: %m, exiting");
			exit(-1);
		case 1:
			if (d_cache)
				syslog(LOG_DEBUG, "didn't find owner %7d/%s", 
					owner_k.pid, owner_k.host);
			owner = -1;
			break;
		case 0:
			ownerp = *((struct owner_l **)data.data);
			owner = ownerp->owner;

			/*
			 * If this owner is on some queue, remove it from it,
			 * as this new request supplants the old one.
			 */
			if (ownerp->queue != NULL) {
				LIST_REMOVE(ownerp->queue, next);
				free(ownerp->queue);
				ownerp->queue = NULL;
			}
			break;
		default:
			syslog(LOG_ERR, "ownerdb->get return %d, exiting",
									result);
			exit(-1);
	}

	req.owner_rel_ok = FALSE;
	if (my_nfslockdreq(req, F_GETLK, owner, pid_ret,
	    (struct sockaddr *)svc_getcaller(rqstp->rq_xprt),
	    fh, fh_len, offset, len, type, pid) != 0) {
		return NLM4_GRANTED;
	}
	if (*pid_ret == -1) {
		*pid_ret = 0;
		return NLM4_GRANTED;
	}

	return NLM4_DENIED;
}

/*
 * Set a lock on a file.
 */
nlm4_stats
setlock(
	struct svc_req	*rqstp,
	char		*caller_name,
	int		pid,
	void            *fh,
        size_t          fh_len,
	void            *oh,
        size_t          oh_len,
        u_quad_t        offset,
        u_quad_t        len,
        int             type,
	int		block,
	enum nlmvers	nlmvers,
	netobj		*cookie,
	int		async
)
{
	struct lockd_req req;
	struct owner_l *ownerp;
	struct owner_l_head *headp;
	struct owner_key owner_k;
	int new_owner;
	DBT key, data;
	int result;
	int host_len;

	if (extract_cred(rqstp, &req.cred) < 0) {
		return NLM4_FAILED;
	}

	/*
	 * If we have an owner descripter already allocated for this owner 
	 * handle, get it, and pass it in the lockd request.
	 */

	memset((void *)&owner_k, '\0', (char *)owner_k.host - (char *)&owner_k);
	host_len = strlen(caller_name);
	owner_k.pid = pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, caller_name, host_len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, host_len);
	
	if (d_cache) {
		syslog(LOG_DEBUG, "lookup owner %7d/%s", 
			owner_k.pid, owner_k.host);
		show_key(&key);
	}

	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb->get: %m, exiting");
			exit(-1);
		case 1:
			if (d_cache)
				syslog(LOG_DEBUG, "didn't find owner %7d/%s", 
					owner_k.pid, owner_k.host);
			new_owner = TRUE;
			headp = owner_headp_get(caller_name, host_len, TRUE);
			if (headp == NULL) {
				return (NLM4_DENIED_NOLOCKS);
			}
			if ((ownerp = MALLOC(struct owner_l)) == NULL) {
				return (NLM4_DENIED_NOLOCKS);
			}
			ownerp->owner = -1;
			ownerp->pid = pid;
			ownerp->queue = NULL; 
			LIST_INSERT_HEAD(headp, ownerp, next);
			data.data = (void *)&ownerp;
			data.size = sizeof(ownerp);
			switch (result = ownerdb->put(ownerdb, &key, &data, 
							R_NOOVERWRITE)) {
				case -1:
					if (errno == ENOMEM) {
						syslog(LOG_ERR, 
						    "ownerdb->put: %m");
						LIST_REMOVE(ownerp, next);
						free(ownerp);
						return (NLM4_DENIED_NOLOCKS);
					} else {
						syslog(LOG_ERR, 
						   "ownerdb->put: %m, exiting");
						exit(-1);
					}
				case 1:
					syslog(LOG_ERR, 
					    "ownerdb->put: duplicate, exiting");
					exit(-1);
				case 0:
					if (d_cache)
						syslog(LOG_DEBUG,
							"added owner %7d/%s", 
							owner_k.pid, 
							owner_k.host);
					break;
				default:
					syslog(LOG_ERR, "ownerdb->put: returned %d, exiting",
						result);
					exit(-1);
			}
			break;
		case 0:
			new_owner = FALSE;
			ownerp = *((struct owner_l **)data.data);

			/*
			 * If this owner is on some queue, remove it from it,
			 * as this new request supplants the old one.
			 */
			if (ownerp->queue != NULL) {
				LIST_REMOVE(ownerp->queue, next);
				free(ownerp->queue);
				ownerp->queue = NULL;
			}
			break;
		default:
			syslog(LOG_ERR, "ownerdb->get return %d, exiting",
									result);
			exit(-1);
	}

	/*
	 * Since we cannot undo a lock (since we don't know how it might
	 * have combined previous locks), we set the lock *after* having
	 * already done everything else that might fail.
	 *
	 * This means that if the lock fails, and this was a new owner, we 
	 * need to undo some stuff.
	 */
	req.owner_rel_ok = ownerp->queue == NULL && !block;
	if (my_nfslockdreq(req, F_SETLK, ownerp->owner, &ownerp->owner,
	    (struct sockaddr *)svc_getcaller(rqstp->rq_xprt),
	    fh, fh_len, offset, len, type, ownerp->pid) != 0) {
		if (errno == EAGAIN && block &&
		    blocked_enqueue(nlmvers, async, caller_name, host_len, 
				oh, oh_len, ownerp, cookie, &req) >= 0) {
			return NLM4_BLOCKED;
		}
		if (ownerp->owner == -1) {	/* kernel released the owner */
			ownerdb_del(&key, ownerp);
		}
		return NLM4_DENIED;
	}

	return NLM4_GRANTED;
}

void *
nlm_nsm_callback_1_svc(status *statusp, struct svc_req *rqstp)
{
	struct owner_l_head	*headp;
	struct owner_l		*ownerp;
	struct owner_l		*next;
	struct lockd_req	req;
	DBT			key;
	struct owner_key 	owner_k;
	int			len;
	int			ret;

	LOG_CALL("nlm_nsm_callback_1", rqstp);

	len = strlen(statusp->mon_name);
	if ((headp = owner_headp_get(statusp->mon_name, len, FALSE)) == NULL)
		return (0);

	/*
	 * set the key.  Make sure all bytes get set, and don't count any
	 * of the round-up bytes at the end of the struct owner_key.
	 */
	memset((void *)&owner_k, '\0', (char *)owner_k.host - (char *)&owner_k);
	owner_k.host_len = len;
	memcpy((void *)owner_k.host, statusp->mon_name, len+1);
	key.data = &owner_k;
	key.size = OWNER_KEY_SIZE(owner_k, len);

	/*
	 * unlock all files for each owner
	 *  specify a null file handle with op = F_UNLCK and the owner
	 */
	memset((void *)&req, 0, sizeof (req));
	req.op = F_UNLCK;
	for (ownerp = LIST_FIRST(headp);
	    ownerp != LIST_END(NULL); 
	    ownerp = next) {
		req.owner = ownerp->owner;
		if (nfslockdreq(LOCKD_REQ_VERSION, &req)) {
			syslog(LOG_ERR, "nfslockdreq forced unlock failed: %m");
			return (0);
		}

		owner_k.pid = ownerp->pid;
		next = LIST_NEXT(ownerp, next);
		ownerdb_del(&key, ownerp);
	}
	if (LIST_FIRST(headp) != LIST_END(NUL))
		return (0);


	key.data = statusp->mon_name;
	key.size = len + 1;
	ret = hostdb->del(hostdb, &key, 0);
	if (ret == 0) {			/* deleted from db, expected result */
		free(ownerp);
	} else if (ret == -1) { 	/* delete failed for some reason */
		syslog(LOG_ERR, "hostdb: %m");
	} else if (ret == 1) { 	/* could not find entry ... we are confused */
		syslog(LOG_ERR, "hostdb: lost host, exiting");
		exit(-1);
	} else {		/* should never get here */
		syslog(LOG_ERR, "hostdb: del returned (%d)", ret);
	}
	return (0);
}

/******************************
 * NLM Version 1-3.
 ******************************/
/*
 * nlm_test --
 *	Test whether a specified lock would be granted if requested.
 */
nlm_testres *
nlm_test_1_svc(nlm_testargs *arg, struct svc_req *rqstp)
{
	static nlm_testres res;

	LOG_CALL("nlm_test_1_svc", rqstp);

	res.cookie = arg->cookie;
	res.stat.stat = nlm_granted;

	if (d_args) {
		syslog(LOG_DEBUG, "exclusive: %lu;\n", (u_long)arg->exclusive);
		/*
		 * XXX
		 * The Protocols for X/Open Interworking: XNFS, Version 3 says
		 * that the nlm_lock structure has an element uppid, not svid.
		 * The SMI nlm_prot.x file:
		 *	@(#)nlm_prot.x 2.1 88/08/01 4.0 RPCSRC
		 *	@(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro
		 * disagrees.
		 */
		syslog(LOG_DEBUG,
		    "test: caller: %s; uppid: %lu; offset: %lu; len %lu\n",
		    arg->alock.caller_name, (u_long)arg->alock.svid,
		    arg->alock.l_offset, arg->alock.l_len);
		show_fh("file", &arg->alock.fh);
		show_netobj("owner", &arg->alock.oh);
		show_netobj("cookie", &arg->cookie);
	}

	if (check_grace && in_grace_period())
		res.stat.stat = nlm_denied_grace_period;
	else {
		res.stat.stat = getlock(rqstp, 
		    arg->alock.caller_name, arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len,
		    arg->exclusive ? F_WRLCK : F_RDLCK, FALSE,
		    &res.stat.nlm_testrply_u.holder.svid);
		if (res.stat.stat == nlm_denied) {
			res.stat.nlm_testrply_u.holder.exclusive = 1;
			res.stat.nlm_testrply_u.holder.oh.n_bytes = NULL;
			res.stat.nlm_testrply_u.holder.oh.n_len = 0;
			res.stat.nlm_testrply_u.holder.l_offset = 0;
			res.stat.nlm_testrply_u.holder.l_len = 0;
		}
	}

	LOG_RES(res.stat.stat);
	return (&res);
}

/*
 * nlm_test_msg --
 *	Test lock message.
 */
void *
nlm_test_msg_1_svc(nlm_testargs *arg, struct svc_req *rqstp)
{
	static char dummy;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	CLIENT *cli;

	/*
	 * nlm_test has different result type to the other operations, so
	 * can't use transmit_result() in this case.
	 */
	if ((cli = get_client(NULL,
	    svc_getcaller(rqstp->rq_xprt), NLM_PROG, NLM_VERS)) == NULL)
		return (NULL);

	/*
	 * clnt_call() will always fail (with timeout) as we are calling it
	 * with timeout 0 as a hack to issue a datagram without expecting a
	 * result.
	 */
	(void)clnt_call(cli, NLM_TEST_RES, xdr_nlm_testres,
	    (caddr_t) nlm_test_1_svc(arg, rqstp), xdr_void, &dummy, timeout);
	return (NULL);
}

/*
 * nlm_test_res --
 *	Accept result from earlier nlm_test_msg() call.
 */
void *
nlm_test_res_1_svc(nlm_testres *arg, struct svc_req *rqstp)
{
	LOG_CALL_ST("nlm_test_res_1_svc", rqstp, arg->stat.stat);
	(void)lock_answer(-1, &arg->cookie, arg->stat.stat, 
			&arg->stat.nlm_testrply_u.holder.svid, NLM_VERS);

	return (NULL);
}

/*
 * nlm_lock  --
 *	Establish a lock.
 */
nlm_res *
nlm_lock_1_svc(nlm_lockargs *arg, struct svc_req *rqstp)
{
	static nlm_res res;

	LOG_CALL("nlm_lock_1_svc", rqstp);
	if (d_args) {
		syslog(LOG_DEBUG,
		    "block: %lu; exclusive: %lu; reclaim: %lu; state: %lu\n",
		    (u_long)arg->block, (u_long)arg->exclusive,
		    (u_long)arg->reclaim, (u_long)arg->state);
		/*
		 * XXX
		 * The Protocols for X/Open Interworking: XNFS, Version 3 says
		 * that the nlm_lock structure has an element uppid, not svid.
		 * The SMI nlm_prot.x file:
		 *	@(#)nlm_prot.x 2.1 88/08/01 4.0 RPCSRC
		 *	@(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro
		 * disagrees.
		 */
		syslog(LOG_DEBUG,
		    "lock: caller: %s; uppid: %lu; offset: %lu; len %lu\n",
		    arg->alock.caller_name, (u_long)arg->alock.svid,
		    arg->alock.l_offset, arg->alock.l_len);
		show_fh("file", &arg->alock.fh);
		show_netobj("owner", &arg->alock.oh);
		show_netobj("cookie", &arg->cookie);
	}

	res.cookie = arg->cookie;
	if (check_grace && !arg->reclaim && in_grace_period())
		res.stat.stat = nlm_denied_grace_period;
	else {
		res.stat.stat = setlock(rqstp,
		    arg->alock.caller_name, arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.oh.n_bytes, arg->alock.oh.n_len,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len,
		    arg->exclusive ? F_WRLCK : F_RDLCK,
		    arg->block, nlmvers1, &arg->cookie, FALSE);
	}

	LOG_RES(res.stat.stat);
	return (&res);
}

/*
 * nlm_lock_msg --
 *	Lock message.
 */
void *
nlm_lock_msg_1_svc(nlm_lockargs *arg, struct svc_req *rqstp)
{
	nlm_res *res;

	res = nlm_lock_1_svc(arg, rqstp);
	transmit_result(NLM_LOCK_RES, res, rqstp);
	return (NULL);
}

/*
 * nlm_lock_res --
 *	Accept result from earlier nlm_lock_msg() call.
 */
void *
nlm_lock_res_1_svc(nlm_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_ST("nlm_lock_res_1_svc", rqstp, arg->stat.stat);

	(void)lock_answer(-1, &arg->cookie, arg->stat.stat, NULL, NLM_VERS);

	return (NULL);
}

nlm_stats
lock_cancel(
	char		*caller_name,
	int		pid,
	void		*fh,
	int		fh_len,
	u_quad_t	offset,
	u_quad_t	len,
	int		exclusive
)
{
	nlm_stats stat;
	struct owner_l	*ownerp;

	ownerp = owner_get(caller_name, pid);
	if (ownerp != NULL) {
		struct blocked_l *p;
		struct lockd_req req;

		req.owner = ownerp->owner;
		req.offset = offset;
		req.len = len;
		req.fh = fh;
		req.fh_len = fh_len;
		req.type = exclusive ? F_WRLCK : F_RDLCK;

		if ((p = queue_search(bheadp, &req)) != NULL) {
			/*
			 * the lock hasn't been granted yet, so it's easy to
			 * cancel.
			 */
			p->ownerp->queue = NULL; 
			LIST_REMOVE(p, next);
			free(p);
			stat = nlm_granted;
		} else {
			/*
			 * XXX if we're on the waiting queue, then refuse to
			 *     let them cancel, as we're just waiting to inform
			 *     them they have it.
			 */
			stat = nlm_denied;
		}
	} else {
		stat = nlm_denied;
	}
	return (stat);
}

/*
 * nlm_cancel --
 *	Cancel a blocked lock request.
 */
nlm_res *
nlm_cancel_1_svc(nlm_cancargs *arg, struct svc_req *rqstp)
{
	static nlm_res res;

	LOG_CALL("nlm_cancel_1_svc", rqstp);

	res.cookie = arg->cookie;
	res.stat.stat = lock_cancel(arg->alock.caller_name,
			arg->alock.svid,
			arg->alock.fh.n_bytes,
			arg->alock.fh.n_len,
			(u_quad_t)arg->alock.l_offset,
			(u_quad_t)arg->alock.l_len,
			arg->exclusive);

	LOG_RES(res.stat.stat);
	return (&res);
}

/*
 * nlm_cancel_msg --
 *	Cancel message.
 */
void *
nlm_cancel_msg_1_svc(nlm_cancargs *arg, struct svc_req *rqstp)
{
	transmit_result(NLM_CANCEL_RES, nlm_cancel_1_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm_cancel_res --
 *	Accept result from earlier nlm_cancel_msg() call.
 */
void *
nlm_cancel_res_1_svc(nlm_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_ST("nlm_cancel_res_1_svc", rqstp, arg->stat.stat);

	return (NULL);
}

/*
 * nlm_unlock --
 *	Release an existing lock.
 */
nlm_res *
nlm_unlock_1_svc(nlm_unlockargs *arg, struct svc_req *rqstp)
{
	static nlm_res res;
	
	LOG_CALL("nlm_unlock_1_svc", rqstp);

	clearlock(rqstp,
		    arg->alock.caller_name, arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the only two legal return codes are LCK_GRANTED and
	 * LCK_DENIED_GRACE_PERIOD.  I see no reason to ever implement
	 * the latter.
	 */
	res.cookie = arg->cookie;
	res.stat.stat = nlm_granted;

	LOG_RES(res.stat.stat);
	return (&res);
}

/*
 * nlm_unlock_msg --
 *	Unlock message.
 */
void *
nlm_unlock_msg_1_svc(nlm_unlockargs *arg, struct svc_req *rqstp)
{
	transmit_result(NLM_UNLOCK_RES, nlm_unlock_1_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm_unlock_res --
 *	Accept result from earlier nlm_unlock_msg() call.
 */
void *
nlm_unlock_res_1_svc(nlm_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_ST("nlm_unlock_res_1_svc", rqstp, arg->stat.stat);

	return (NULL);
}

/*
 * nlm_granted --
 *	Receive notification that formerly blocked lock now granted.
 */
nlm_res *
nlm_granted_1_svc(nlm_testargs *arg, struct svc_req *rqstp)
{
	static nlm_res res;

	LOG_CALL("nlm_granted_1_svc", rqstp);

	if (lock_answer(arg->alock.svid, &arg->cookie, nlm_granted, NULL, 
								NLM_VERS) == 0)
		res.stat.stat = nlm_granted; 
	else
		res.stat.stat = nlm_denied; 

	res.cookie = arg->cookie;

	LOG_RES(res.stat.stat);
	return (&res);
}

/*
 * nlm_granted_msg --
 *	Granted message.
 */
void *
nlm_granted_msg_1_svc(nlm_testargs *arg, struct svc_req *rqstp)
{
	transmit_result(NLM_GRANTED_RES, nlm_granted_1_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm_granted_res --
 *	Accept result from earlier nlm_granted_msg() call.
 */
void *
nlm_granted_res_1_svc(nlm_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_ST("nlm_granted_res_1_svc", rqstp, arg->stat.stat);

	return (NULL);
}

/*
 * Calls for PCNFS locking (AKA non-monitored locking, no involvement of
 * statd).  Blocking locks are not supported -- the client is expected
 * to retry if required.
 *
 * These are all genuine RPCs -- no nlm_xxx_msg() nonsense here.
 */
/*
 * nlm_share --
 *	Establish a DOS-style lock.
 */
nlm_shareres *
nlm_share_3_svc(nlm_shareargs *arg, struct svc_req *rqstp)
{
	static nlm_shareres res;

	LOG_CALL("nlm_share_3_svc", rqstp);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the cookie and sequence fields are ignored.
	 */
	res.cookie = arg->cookie;
	if (check_grace && in_grace_period())
		res.stat = nlm_denied_grace_period;
	else if (setlock(rqstp, arg->share.caller_name, 0,
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    NULL, (size_t)0,
		    (u_quad_t)0, (u_quad_t)0,
			arg->share.mode == fsm_DR ? F_RDLCK : F_WRLCK, 
			FALSE, nlmvers1, &arg->cookie, FALSE) == NLM4_GRANTED)
		res.stat = nlm_granted;
	else
		res.stat = nlm_denied;

	res.sequence = 0;

	LOG_PCNFS_RES(res.stat);
	return (&res);
}

/*
 * nlm_unshare --
 *	Release a DOS-style lock.
 */
nlm_shareres *
nlm_unshare_3_svc(nlm_shareargs *arg, struct svc_req *rqstp)
{
	static nlm_shareres res;

	LOG_CALL("nlm_unshare_3_svc", rqstp);

	clearlock(rqstp,
		    arg->share.caller_name, 0,
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    (u_quad_t)0, (u_quad_t)0);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the cookie and sequence fields are ignored.
	 *
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the only two legal return codes are LCK_GRANTED and
	 * LCK_DENIED_GRACE_PERIOD.  I see no reason to ever implement
	 * the latter.
	 */
	res.cookie = arg->cookie;
	res.stat = nlm_granted;
	res.sequence = 0;

	LOG_PCNFS_RES(res.stat);
	return (&res);
}

/*
 * nlm_nm_lock --
 *	Non-monitored version of nlm_lock().
 */
nlm_res *
nlm_nm_lock_3_svc(nlm_lockargs *arg, struct svc_req *rqstp)
{
	/*
	 * These locks are in the same style as the standard nlm_lock, but
	 * statd should not be called to establish a monitor for the client
	 * machine, as that machine is declared not to be running a statd,
	 * and so would not respond to the statd protocol.
	 */
	return (nlm_lock_1_svc(arg, rqstp));
}

/*
 * nlm_free_all --
 *	Release all locks held by a named client.
 */
void *
nlm_free_all_3_svc(nlm_notify *arg, struct svc_req *rqstp)
{
	arg = arg;				/* XXX: Shut the compiler up. */

	LOG_CALL("nlm_free_all_3_svc", rqstp);

	/*
	 * XXX
	 * Potential denial-of-service security problem here.  The locks to be
	 * released are specified by a host name, independent of the address
	 * from which the request has arrived.  Should probably be rejected if
	 * the named host has been using monitored locks.
	 *
	 * UNIMPLEMENTED
	 * (void)nfs1unlockall(arg->name);
	 */

	return (NULL);
}

/*
 * transmit_result --
 *
 *	Transmit result for nlm_xxx_msg pseudo-RPCs.
 */
static void
transmit_result(int opcode, nlm_res *result, struct svc_req *rqstp)
{
	static char dummy;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	CLIENT *cli;

	if ((cli = get_client(NULL,
	    svc_getcaller(rqstp->rq_xprt), NLM_PROG, NLM_VERS)) == NULL)
		return;

	(void)clnt_call(cli,
	    opcode, xdr_nlm_res, (caddr_t) result, xdr_void, &dummy, timeout);
}

/******************************
 * NLM Version 4.
 ******************************/
/*
 * nlm4_test --
 *	Test lock.
 */
nlm4_testres *
nlmproc4_test_4_svc(nlm4_testargs *arg, struct svc_req *rqstp)
{
	static nlm4_testres res;

	LOG_CALL("nlmproc4_test_4_svc", rqstp);

	/*
	 * XXX
	 * We just return that the lock will be granted.  If we need to
	 * actually do the test at some point, it will require additional
	 * system calls.
	 */
	res.cookie = arg->cookie;

	if (d_args) {
		syslog(LOG_DEBUG, "exclusive: %lu;\n", (u_long)arg->exclusive);
		/*
		 * XXX
		 * The Protocols for X/Open Interworking: XNFS, Version 3 says
		 * that the nlm_lock structure has an element uppid, not svid.
		 * The SMI nlm_prot.x file:
		 *	@(#)nlm_prot.x 2.1 88/08/01 4.0 RPCSRC
		 *	@(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro
		 * disagrees.
		 */
		syslog(LOG_DEBUG,
		    "test: caller: %s; uppid: %lu; offset: %qu; len %qu\n",
		    arg->alock.caller_name, (u_long)arg->alock.svid,
		    arg->alock.l_offset, arg->alock.l_len);
		show_fh("file", &arg->alock.fh);
		show_netobj("owner", &arg->alock.oh);
		show_netobj("cookie", &arg->cookie);
	}

	if (check_grace && in_grace_period())
		res.stat.stat = nlm_denied_grace_period;
	else {
		res.stat.stat = getlock(rqstp, 
		    arg->alock.caller_name, arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.l_offset, arg->alock.l_len,
		    arg->exclusive ? F_WRLCK : F_RDLCK, FALSE,
		    (int *)&res.stat.nlm4_testrply_u.holder.svid);
		if (res.stat.stat == nlm_denied) {
			res.stat.nlm4_testrply_u.holder.exclusive = 1;
			res.stat.nlm4_testrply_u.holder.oh.n_bytes = NULL;
			res.stat.nlm4_testrply_u.holder.oh.n_len = 0;
			res.stat.nlm4_testrply_u.holder.l_offset = 0;
			res.stat.nlm4_testrply_u.holder.l_len = 0;
		}
	}

	LOG_4RES(res.stat.stat);
	return (&res);
}

/*
 * nlm4_test_msg --
 *	Test lock message.
 */
void *
nlmproc4_test_msg_4_svc(nlm4_testargs *arg, struct svc_req *rqstp)
{
	static char dummy;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	CLIENT *cli;

	/*
	 * nlm4_test has a different result type to the other operations, so
	 * can't use transmit_4result() in this case.
	 */
	if ((cli = get_client(NULL,
	    svc_getcaller(rqstp->rq_xprt), NLM_PROG, NLM4_VERS)) == NULL)
		return (NULL);

	(void)clnt_call(cli, NLMPROC4_TEST_RES, xdr_nlm4_testres,
	    (caddr_t) nlmproc4_test_4_svc(arg, rqstp), xdr_void, &dummy, timeout);
	return (NULL);
}

/*
 * nlm4_test_res --
 *	Accept results of earlier nlm4_test_msg() call.
 */
void *
nlmproc4_test_res_4_svc(nlm4_testres *arg, struct svc_req *rqstp)
{
	LOG_CALL_4ST("nlmproc4_test_res_4_svc", rqstp, arg->stat.stat);

	(void)lock_answer(-1, &arg->cookie, arg->stat.stat,
			(int *)&arg->stat.nlm4_testrply_u.holder.svid,
			NLM4_VERS);

	return (NULL);
}

/*
 * nlm4_lock --
 *	Establish a lock.
 */
nlm4_res *
nlmproc4_lock_4_svc(nlm4_lockargs *arg, struct svc_req *rqstp)
{
	static nlm4_res res;

	LOG_CALL("nlmproc4_lock_4_svc", rqstp);
	if (d_args)
		show_lock(&arg->alock);

	res.cookie = arg->cookie;
	if (check_grace && !arg->reclaim && in_grace_period())
		res.stat.stat = NLM4_DENIED_GRACE_PERIOD;
	else {
		res.stat.stat = setlock(rqstp, arg->alock.caller_name,
		    arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.oh.n_bytes, arg->alock.oh.n_len,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len,
			arg->exclusive ? F_WRLCK : F_RDLCK,
			arg->block, nlmvers4, &arg->cookie, FALSE);
	}

	LOG_4RES(res.stat.stat);
	return (&res);
}

/*
 * nlm4_lock_msg --
 *	Establish a lock message.
 */
void *
nlmproc4_lock_msg_4_svc(nlm4_lockargs *arg, struct svc_req *rqstp)
{
	nlm4_res *res;

	res = nlmproc4_lock_4_svc(arg, rqstp);

	if (res->stat.stat == NLM4_GRANTED)
		transmit_4result(NLMPROC4_LOCK_RES, res, rqstp);
	
	return (NULL);
}

/*
 * nlm4_lock_res --
 *	Accept result from earlier nlm4_lock_msg() call.
 */
void *
nlmproc4_lock_res_4_svc(nlm4_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_4ST("nlmproc4_lock_res_4_svc", rqstp, arg->stat.stat);

	(void)lock_answer(-1, &arg->cookie, arg->stat.stat, NULL, NLM4_VERS);

	return (NULL);
}

/*
 * nlm4_cancel --
 *	Cancel a blocked lock request.
 */
nlm4_res *
nlmproc4_cancel_4_svc(nlm4_cancargs *arg, struct svc_req *rqstp)
{
	static nlm4_res	res;

	LOG_CALL("nlmproc4_cancel_4_svc", rqstp);

	if (d_args)
		show_lock(&arg->alock);

	res.cookie = arg->cookie;
	res.stat.stat = (nlm4_stats)lock_cancel(arg->alock.caller_name,
			arg->alock.svid,
			arg->alock.fh.n_bytes,
			arg->alock.fh.n_len,
			arg->alock.l_offset,
			arg->alock.l_len,
			arg->exclusive);

	LOG_4RES(res.stat.stat);

	return ((nlm4_res *)&res);
}

/*
 * nlm4_cancel_msg --
 *	Cancel a blocked lock request message.
 */
void *
nlmproc4_cancel_msg_4_svc(nlm4_cancargs *arg, struct svc_req *rqstp)
{
	transmit_4result(NLMPROC4_CANCEL_RES,
	    nlmproc4_cancel_4_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm4_cancel_res --
 *	Accept result from earlier nlm4_cancel_msg() call.
 */
void *
nlmproc4_cancel_res_4_svc(nlm4_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_4ST("nlmproc4_cancel_res_4_svc", rqstp, arg->stat.stat);

	return (NULL);
}

/*
 * nlm4_unlock --
 *	Release an existing lock.
 */
nlm4_res *
nlmproc4_unlock_4_svc(nlm4_unlockargs *arg, struct svc_req *rqstp)
{
	static nlm4_res res;

	LOG_CALL("nlmproc4_unlock_4_svc", rqstp);

	if (d_args) {
		show_netobj("cookie", &arg->cookie);
		show_lock(&arg->alock);
	}

	clearlock(rqstp,
		    arg->alock.caller_name, arg->alock.svid,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    (u_quad_t)0, (u_quad_t)0);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the only two legal return codes are NLM4_GRANTED and
	 * NLM4_DENIED_GRACE_PERIOD.  I see no reason to ever implement
	 * the latter.
	 */
	res.cookie = arg->cookie;
	res.stat.stat = NLM4_GRANTED;

	LOG_4RES(res.stat.stat);
	return (&res);
}

/*
 * nlm4_unlock_msg --
 *	Release an existing lock message.
 */
void *
nlmproc4_unlock_msg_4_svc(nlm4_unlockargs *arg, struct svc_req *rqstp)
{
	transmit_4result(NLMPROC4_UNLOCK_RES,
	    nlmproc4_unlock_4_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm4_unlock_res --
 *	Accept result from earlier nlm4_unlock_msg() call.
 */
void *
nlmproc4_unlock_res_4_svc(nlm4_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_4ST("nlmproc4_unlock_res_4_svc", rqstp, arg->stat.stat);

	return (NULL);
}

/*
 * nlm4_granted --
 *	Receive notification that formerly blocked lock now granted.
 */
nlm4_res *
nlmproc4_granted_4_svc(nlm4_testargs *arg, struct svc_req *rqstp)
{
	static nlm4_res res;

	LOG_CALL("nlmproc4_granted_4_svc", rqstp);

	if (lock_answer(arg->alock.svid, &arg->cookie, NLM4_GRANTED, NULL, 
								NLM4_VERS) == 0)
		res.stat.stat = NLM4_GRANTED;
	else
		res.stat.stat = NLM4_DENIED;

	res.cookie = arg->cookie;

	LOG_4RES(res.stat.stat);
	return (&res);
}

/*
 * nlm4_granted_msg --
 *	Granted message.
 */
void *
nlmproc4_granted_msg_4_svc(nlm4_testargs *arg, struct svc_req *rqstp)
{
	transmit_4result(NLMPROC4_GRANTED_RES,
	    nlmproc4_granted_4_svc(arg, rqstp), rqstp);
	return (NULL);
}

/*
 * nlm4_granted_res --
 *	Accept result from earlier nlm4_granted_msg() call.
 */
void *
nlmproc4_granted_res_4_svc(nlm4_res *arg, struct svc_req *rqstp)
{
	LOG_CALL_4ST("nlmproc4_granted_res_4_svc", rqstp, arg->stat.stat);

	/* XXX should really move the message off a retry queue now, 
	 *     rather then earlier
	 */

	return (NULL);
}

/*
 * nlmproc4_nm_lock_4_svc --
 *	Non-monitored version of nlm4_lock().
 */
nlm4_res *
nlmproc4_nm_lock_4_svc(nlm4_lockargs *arg, struct svc_req *rqstp)
{
	/*
	 * These locks are in the same style as the standard nlm4_lock, but
	 * statd should not be called to establish a monitor for the client
	 * machine, as that machine is declared not to be running a statd,
	 * and so would not respond to the statd protocol.
	 */
	return (nlmproc4_lock_4_svc(arg, rqstp));
}

/*
 * nlmproc4_free_all_4_svc --
 *	Release all locks held by a named client.
 */
void *
nlmproc4_free_all_4_svc(nlm4_notify *arg, struct svc_req *rqstp)
{
	arg = arg;				/* XXX: Shut the compiler up. */
	
	LOG_CALL("nlmproc4_free_all_4_svc", rqstp);

	/*
	 * XXX
	 * Potential denial-of-service security problem here.  The locks to be
	 * released are specified by a host name, independent of the address
	 * from which the request has arrived.  Should probably be rejected if
	 * the named host has been using monitored locks.
	 *
	 * UNIMPLEMENTED:
	 * (void)nfs1unlockall(arg->name);
	 */

	return (NULL);
}

/*
 * nlmproc4_null_4_svc --
 *
 * XXX
 * The Protocols for X/Open Interworking: XNFS, Version 3 describes a call
 * named nlmproc3_null.
 * The SMI nlm_prot.x file:
 *	@(#)nlm_prot.x 2.1 88/08/01 4.0 RPCSRC
 *	@(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro
 * disagrees.
 */
void *
nlmproc4_null_4_svc(void *arg, struct svc_req *rqstp)
{
	arg = arg;				/* XXX: Shut the compiler up. */
	rqstp = rqstp;

	return (NULL);
}

/*
 * nlmproc4_share_4_svc --
 *	Establish a DOS-style lock.
 */
nlm4_shareres *
nlmproc4_share_4_svc(nlm4_shareargs *arg, struct svc_req *rqstp)
{
	static nlm4_shareres res;

	LOG_CALL("nlmproc4_share_4_svc", rqstp);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the cookie and sequence fields are ignored.
	 */
	res.cookie = arg->cookie;
	if (check_grace && in_grace_period())
		res.stat = NLM4_DENIED_GRACE_PERIOD;
	else {
		res.stat = setlock(rqstp, arg->share.caller_name, 0,
				arg->share.fh.n_bytes, 
				arg->share.fh.n_len,
				NULL, (size_t)0,
				(u_quad_t)0, (u_quad_t)0,
				arg->share.mode == fsm_DR ? F_RDLCK : F_WRLCK,
				FALSE, nlmvers4, &arg->cookie, FALSE);
	}

	res.sequence = 0;

	LOG_PCNFS_4RES(res.stat);
	return (&res);
}

/*
 * nlmproc4_unshare_4_svc --
 *	Release a DOS-style lock.
 */
nlm4_shareres *
nlmproc4_unshare_4_svc(nlm4_shareargs *arg, struct svc_req *rqstp)
{
	static nlm4_shareres res;

	LOG_CALL("nlmproc4_unshare_4_svc", rqstp);

	clearlock(rqstp,
		    arg->share.caller_name, 0,
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    (u_quad_t)0, (u_quad_t)0);

	/*
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the cookie and sequence fields are ignored.
	 *
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says
	 * that the only two legal return codes are NLM4_GRANTED and
	 * NLM4_DENIED_GRACE_PERIOD.  I see no reason to ever implement
	 * the latter.
	 */
	res.cookie = arg->cookie;
	res.stat = NLM4_GRANTED;
	res.sequence = 0;

	LOG_PCNFS_4RES(res.stat);
	return (&res);
}

/*
 * transmit_4result --
 *	Transmit result for nlm_xxx_msg_4_svc pseudo-RPCs.
 */
static void
transmit_4result(int opcode, nlm4_res *result, struct svc_req *rqstp)
{
	static char dummy;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	CLIENT *cli;

	if ((cli = get_client(NULL,
	    svc_getcaller(rqstp->rq_xprt), NLM_PROG, NLM4_VERS)) == NULL)
		return;

	(void)clnt_call(cli,
	    opcode, xdr_nlm4_res, (caddr_t) result, xdr_void, &dummy, timeout);
}

/*
 * from_addr --
 *	Return the hostname from a source address.
 */
char *
from_addr(struct sockaddr_in *addr)
{
	struct hostent *host;
	static char hostname_buf[1024];

	if ((host = gethostbyaddr((const char *)&addr->sin_addr,
	    addr->sin_len, AF_INET)) == NULL)
		(void)strncpy(hostname_buf,
		    inet_ntoa(addr->sin_addr), sizeof(hostname_buf) - 1);
	else {
		(void)strncpy(hostname_buf,
		    host->h_name, sizeof(hostname_buf) - 1);
	}
	hostname_buf[sizeof(hostname_buf) - 1] = '\0';
	return (hostname_buf);
}

void
dump_hostdb()
{
	DBT			key, data;
	int			result;
	struct owner_l		*ownerp;
	struct owner_l_head	*headp;

	syslog(LOG_DEBUG, "dumping hostdb\n");
	result = hostdb->seq(hostdb, &key, &data, R_FIRST);
	while (result == 0) {
		headp = *(struct owner_l_head **)data.data;

		syslog(LOG_DEBUG, "  host: %.*s\n", key.size, key.data);
		for (ownerp = LIST_FIRST(headp);
		    ownerp != LIST_END(NULL); 
		    ownerp = LIST_NEXT(ownerp, next)) {
			syslog(LOG_DEBUG, "    owner: %d; pid: %d\n",
					ownerp->owner, ownerp->pid);
		}

		result = hostdb->seq(hostdb, &key, &data, R_NEXT);
	}
	if (result == -1) {
		syslog(LOG_DEBUG, "hostdb: %m");
	}
}

void
show_queue(
	struct blocked_l *p
)
{
	syslog(LOG_DEBUG,"     q pid/host: %d/%s", p->req.pid, p->host);
}

void
dump_ownerdb()
{
	DBT			key, data;
	int			result;
	struct owner_l		*ownerp;
	struct owner_key	owner_k;

	syslog(LOG_DEBUG, "dumping ownerdb\n");
	result = ownerdb->seq(ownerdb, &key, &data, R_FIRST);
	while (result == 0) {

		memcpy((void *)&owner_k, key.data, key.size);
		ownerp = *(struct owner_l **)data.data;

		syslog(LOG_DEBUG, "  key pid/host: %11d/%.*s\n", 
				owner_k.pid, owner_k.host_len, owner_k.host);
		syslog(LOG_DEBUG, "  data pid/owner: %7d/%7d\n", 
				ownerp->pid, ownerp->owner);
		if (ownerp->queue) {
			show_queue(ownerp->queue);
		}

		result = ownerdb->seq(ownerdb, &key, &data, R_NEXT);
	}
	if (result == -1) {
		syslog(LOG_DEBUG, "hostdb: %m");
	}
}

/*
 * show_lock --
 *	Display a NLM version 4 lock cookie.
 */
static void
show_lock(struct nlm4_lock *lp)
{
	char *p, *t, buf[8192];

	t = buf;
	for (p = lp->caller_name; *p != '\0'; ++p)
		if (isprint(*p))
			*t++ = *p;
		else {
			*t++ = '\\';
			*t++ = hex[(u_int8_t)(*p & 0xf0) >> 4];
			*t++ = hex[*p & 0x0f];
		}
	*t = '\0';
	syslog(LOG_DEBUG, "caller: len: %d; %s\n", (int)(p - lp->caller_name),
									buf);

	show_fh("file", &lp->fh);
	show_netobj("owner", &lp->oh);

	syslog(LOG_DEBUG, "pid: %ld; offset: 0x%qx; len: 0x%qx\n",
	    (long)lp->svid, lp->l_offset, lp->l_len);
}

/*
 * show_fh --
 *	Display a file handle cookie.
 */
static void
show_fh(char *msg, netobj *cookie)
{
	struct fid *fidp;
	fsid_t *fsidp;
	size_t len;
	u_int8_t *p, *t, buf[NFS_SMALLFH*3+1];

	fsidp = (fsid_t *)cookie->n_bytes;
	fidp = (struct fid *)((u_int8_t *)cookie->n_bytes + sizeof(fsid_t));

#ifdef LOCAL_FH
	/* this can only be done if we know it is one of our file handles.
	 */
	for (t = buf, p = (u_int8_t *)fidp->fid_data,
	    len = fidp->fid_len; len > 0; ++p, --len) {
		*t++ = '\\';
		*t++ = hex[(*p & 0xf0) >> 4];
		*t++ = hex[*p & 0x0f];
	}
#else
	for (t = buf, p = (u_int8_t *)cookie->n_bytes,
	    len = cookie->n_len;
	    len > 0; ++p, --len) {
		*t++ = '\\';
		*t++ = hex[(*p & 0xf0) >> 4];
		*t++ = hex[*p & 0x0f];
	}
#endif
	*t = '\0';

	syslog(LOG_DEBUG, "%s: fsid_t [%d][%d]; fid_len %d; fid_data %s",
	    msg, (int)fsidp->val[0], (int)fsidp->val[1], fidp->fid_len, buf);
}

void
show_key(DBT *key)
{
	char *p, *s, *t, buf[8192];

	t = buf;
	for (p = key->data, s = key->data + key->size; p < s; ++p)
		if (isprint(*p))
			*t++ = *p;
		else {
			*t++ = '\\';
			*t++ = hex[(u_int8_t)(*p & 0xf0) >> 4];
			*t++ = hex[*p & 0x0f];
		}
	*t = '\0';
	syslog(LOG_DEBUG, "key: %s", buf);
}

/*
 * show_netobj --
 *	Display a netobj cookie.
 */
void
show_netobj(char *msg, netobj *cookie)
{
	u_int len;
	char *p, *t, buf[8192];

	t = buf;
	for (p = cookie->n_bytes, len = cookie->n_len; len--;)
		if (0 /*isprint(*p) */)
			*t++ = *p++;
		else {
			*t++ = '\\';
			*t++ = hex[(u_int8_t)(*p & 0xf0) >> 4];
			*t++ = hex[*p & 0x0f];
			++p;
		}
	*t = '\0';

	syslog(LOG_DEBUG, "%s: len: %lu; %s\n", msg, cookie->n_len, buf);
}

/*
 * show_state --
 *	Display the state of an nlm_res structure.
 */
char *
show_state(int stat)
{
	switch (stat) {
	case nlm_granted:
		return ("granted");
	case nlm_denied:
		return ("denied");
	case nlm_denied_nolocks:
		return ("denied_nolocks");
	case nlm_blocked:
		return ("denied_blocked");
	case nlm_denied_grace_period:
		return ("denied_grace_period");
	case nlm_deadlck:
		return ("denied_deadlock");
	}
	return("unknown state value");
}

/*
 * show_4state --
 *	Display the state of an nlm4_stats structure.
 */
char *
show_4state(nlm4_stats stat)
{
	switch (stat) {
	case NLM4_GRANTED:
		return ("granted");
	case NLM4_DENIED:
		return ("denied");
	case NLM4_DENIED_NOLOCKS:
		return ("denied_resource");
	case NLM4_BLOCKED:
		return ("denied_blocked");
	case NLM4_DENIED_GRACE_PERIOD:
		return ("denied_grace_period");
	case NLM4_DEADLCK:
		return ("denied_deadlock");
	case NLM4_ROFS:
		return ("denied_rofs");
	case NLM4_STALE_FH:
		return ("denied_stale_fh");
	case NLM4_FBIG:
		return ("denied_fbig");
	case NLM4_FAILED:
		return ("denied_unknown");
	}
	return("unknown state value");
}

/*
 * This gets called after handling every rpc.
 */
void
nlm_after()
{
	struct blocked_l *p, *next;

	/*
	 * for each lock request we haven't gotten the lock for yet.
	 * XXXX This polling mode is a hack for connectathon.
	 */
	for (p = LIST_FIRST(bheadp); p != LIST_END(NULL); p = next) {
		next = LIST_NEXT(p, next);
		blocked_try(p);
	}
#ifdef NDEF
	if (LIST_FIRST(wheadp) == LIST_END(NULL) &&
	    LIST_FIRST(bheadp) == LIST_END(NULL))
		alarm_running = 0;
	else if (alarm_running == 0) {
		alarm_running = 1;
		alarm(1);
	}
#endif
}


/*
 * This gets called anytime select() returns from any interrupt after we've
 * intered the service loop -- svc_runi().
 */
int
nlm_interrupt()
{
	struct blocked_l *p, *next;

	nlm_clock++;

	/*
	 * For each request waiting for a retry timer on sending a granted
	 * message.
	 */
	for (p = LIST_FIRST(wheadp); p != LIST_END(NULL); p = next) {
		next = LIST_NEXT(p, next);
		if (p->timer < nlm_clock)	/* timer expired ? */
			send_granted(p);
	}

	nlm_after();

	return 0;
}

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

nlm_res *
nlm_granted_1(nlm_testargs *argp, CLIENT *clnt)
{
	static nlm_res clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, NLM_GRANTED,
		(xdrproc_t) xdr_nlm_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

nlm4_res *
nlmproc4_granted_4(nlm4_testargs *argp, CLIENT *clnt)
{
	static nlm4_res clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, NLMPROC4_GRANTED,
		(xdrproc_t) xdr_nlm4_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

