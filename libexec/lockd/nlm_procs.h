/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI nlm_procs.h,v 1.1 1998/03/19 20:18:11 don Exp
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
#include <limits.h>
#include <db.h>
#include <stdlib.h>

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
	syslog(LOG_DEBUG, st == nlm_denied ?				\
	    "result: %s: %m" : "result: %s", show_state(st));
#define	LOG_4RES(st)							\
	syslog(LOG_DEBUG, st == NLM4_DENIED ?				\
	    "result: %s: %m" : "result: %s", show_4state(st));
#define	LOG_PCNFS_RES(st)						\
	syslog(LOG_DEBUG, st == nlm_denied ?				\
	    "result: %s: %m" : "result: %s", show_state(st));
#define	LOG_PCNFS_4RES(st)						\
	syslog(LOG_DEBUG, st == NLM4_DENIED ?				\
	    "result: %s: %m" : "result: %s", show_4state(st));

#define MALLOC(N)	((N *)malloc(sizeof(N)))
static char hex[] = "0123456789abcdef";

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
#define my_nfslockdreq(_req, _op, _ownerp, _saddrp, _fh, _fh_len, \
				_offset, _len, _type) \
	(_req.op = (_op), \
	_req.owner = *(_ownerp), \
	_req.owner_ret = (_ownerp), \
	_req.saddr = *(_saddrp), \
	_req.fh = (_fh), \
	_req.fh_len = (_fh_len), \
	_req.offset = (_offset), \
	_req.len = (_len), \
	_req.type = (_type), \
	nfslockdreq(LOCKD_REQ_VERSION, &_req, sizeof(_req)))


/*
 *
 * We link all the owner's onto a linked list which hangs off the 
 * entry in the host DB.
 */

LIST_HEAD(owner_l_head, owner_l);

struct owner_l {
	int		owner;
	int		pid;
	LIST_ENTRY(owner_l) next;
};

struct owner_key {
	int		pid;
	int		host_len;
	char		host[LM_MAXSTRLEN+1];
};

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
			au_gid_p = unix_cred->aup_gids;
			au_stop = &ucred->cr_groups[NGROUPS - 1];
			uc_stop = &unix_cred->aup_gids[unix_cred->aup_len - 1];
			uc_gid_p = ucred->cr_groups;
			while (uc_gid_p <= uc_stop && au_gid_p <= au_stop) {
				if ((*uc_gid_p++ = *au_gid_p) != gid) {
					au_gid_p++;
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

/*
 * We just use the pid plus the first byte of the hostname as the hash.
 */
u_int32_t
ownerhash(
	const void *data,
	size_t len
)
{
	register struct owner_key *owner_k = (struct owner_key *)data;

	return ((u_int32_t)owner_k->host[0] + (u_int32_t)owner_k->pid);
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
	} else if (ret == -1) { 	/* delete failed for some reason */
		/*
		 * The owner descriptor is left as -1, which will
		 * cause an owner descriptor to be
		 * allocated the next time this owner
		 * handle is used.  If the owner
		 * handle is never used, well then,
		 * we've just used some memory we'll
		 * never free -- which is better then
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

/*
 * Clear a lock on a file.  Nobody really cares if we succeed, as they all
 * have to report success regardless.
 */
void
clearlock(
	struct svc_req *rqstp,
	void            *fh,
        size_t          fh_len,
        void            *oh,
        size_t          oh_len,
        u_quad_t        offset,
        u_quad_t        len
)
{
	struct lockd_req req;
	struct owner_l *ownerp;
	DBT key, data;
	int result;

	/*
	 * Get the owner descripter already allocated for this owner 
	 * handle, and pass it in the lockd request.
	 */

	key.data = oh;
	key.size = oh_len;
	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb: %m, exiting");
			exit(-1);
		case 1:
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
	if (my_nfslockdreq(req, F_UNLCK, &ownerp->owner,
	    (struct sockaddr *)svc_getcaller(rqstp->rq_xprt),
	    fh, fh_len, offset, len, F_UNLCK) == 0) {

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
	key.size = host_len;
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
 * Set a lock on a file.
 */
nlm4_stats
setlock(
	struct svc_req	*rqstp,
	char		*caller_name,
	void            *fh,
        size_t          fh_len,
	int		pid,
        u_quad_t        offset,
        u_quad_t        len,
        int             type
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

	host_len = strlen(caller_name);
	owner_k.pid = pid;
	owner_k.host_len = host_len;
	memcpy((void *)owner_k.host, caller_name, host_len+1);
	key.data = &owner_k;
	key.size = sizeof (owner_k) - sizeof(owner_k.host) + host_len;
	
	switch (result = ownerdb->get(ownerdb, &key, &data, 0)) {
		case -1:
			syslog(LOG_ERR, "ownerdb->get: %m, exiting");
			exit(-1);
		case 1:
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
	if (my_nfslockdreq(req, F_SETLK, &ownerp->owner,
	    (struct sockaddr *)svc_getcaller(rqstp->rq_xprt),
	    fh, fh_len, offset, len, type) != 0) {
		if (new_owner) {
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

	owner_k.host_len = len;
	memcpy((void *)owner_k.host, statusp->mon_name, len+1);
	key.data = &owner_k;
	key.size = sizeof (owner_k) - sizeof(owner_k.host) + len;

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
		if (nfslockdreq(LOCKD_REQ_VERSION, &req, sizeof(req))) {
			syslog(LOG_ERR, "nfslockdreq forced unlock failed: %m");
			return (0);
		}

		owner_k.pid = ownerp->pid;
		next = LIST_NEXT(ownerp, next);
		ownerdb_del(&key, ownerp);
	}
	if (LIST_FIRST(headp) != LIST_END(NUL))
		return (0);


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

	/*
	 * Copy the cookie from the argument into the result.  Note that this
	 * is slightly hazardous, as the structure contains a pointer to a
	 * malloc()ed buffer that will get freed by the caller.  However, the
	 * main function transmits the result before freeing the argument
	 * so it is in fact safe.
	 *
	 * XXX
	 * The Protocols for X/Open Interworking: XNFS, Version 3 says that
	 * the nlm_testres structure has an element test_stat, not stat.
	 * The SMI nlm_prot.x file:
	 *	@(#)nlm_prot.x 2.1 88/08/01 4.0 RPCSRC
	 *	@(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro
	 * disagrees.
	 *
	 * XXX
	 * We return that the lock will be granted.  If we need to actually do
	 * the test at some point, it will require additional system calls.
	 */
	res.cookie = arg->cookie;
	res.stat.stat = nlm_granted;

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
	    nlm_test_1_svc(arg, rqstp), xdr_void, &dummy, timeout);
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

	/*
	 * XXX
	 * Blocking locks (LCK_BLOCKED) not implemented.
	 */
	res.cookie = arg->cookie;
	if (check_grace && !arg->reclaim && in_grace_period())
		res.stat.stat = nlm_denied_grace_period;
	else if (setlock(rqstp, arg->alock.caller_name,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.uppid,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len,
		    arg->exclusive ? F_WRLCK : F_RDLCK) == NLM4_GRANTED)
		res.stat.stat = nlm_granted;
	else
		res.stat.stat = nlm_denied;

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
	transmit_result(NLM_LOCK_RES, nlm_lock_1_svc(arg, rqstp), rqstp);
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

	lock_answer(&arg->cookie, arg->stat.stat, NLM_VERS);

	return (NULL);
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

	/*
	 * Grace period (LCK_DENIED_GRACE_PERIOD) not implemented, and I see
	 * no reason to ever do so.
	 *
	 * XXX
	 * Blocking locks (LCK_BLOCKED) not implemented.  Since we currently
	 * never return 'nlm_blocked', there can never be a lock to cancel,
	 * so this call always fails.
	 */
	res.cookie = arg->cookie;
	res.stat.stat = nlm_denied;
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
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.oh.n_bytes, arg->alock.oh.n_len,
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

	lock_answer(&arg->cookie, nlm_granted, NLM_VERS);

	res.cookie = arg->cookie;
	res.stat.stat = nlm_granted; 

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
	else if (setlock(rqstp, arg->share.caller_name,
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    arg->alock.uppid,
		    (u_quad_t)0, (u_quad_t)0,
			arg->share.mode == fsm_DR ? F_RDLCK : F_WRLCK) ==
								NLM4_GRANTED)
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

	clearlock( rqstp,
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    arg->share.oh.n_bytes, arg->share.oh.n_len,
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
	    opcode, xdr_nlm_res, result, xdr_void, &dummy, timeout);
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
	res.stat.stat = NLM4_GRANTED;

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

	(void)clnt_call(cli, NLMPROC4_TEST_MSG, xdr_nlm4_testres,
	    nlmproc4_test_4_svc(arg, rqstp), xdr_void, &dummy, timeout);
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

	/*
	 * XXX
	 * Blocking locks (NLM4_BLOCKED) not implemented.
	 */
	res.cookie = arg->cookie;
	if (check_grace && !arg->reclaim && in_grace_period())
		res.stat.stat = NLM4_DENIED_GRACE_PERIOD;
	else {
		res.stat.stat = setlock(rqstp, arg->alock.caller_name,
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.uppid,
		    (u_quad_t)arg->alock.l_offset, (u_quad_t)arg->alock.l_len,
			arg->exclusive ? F_WRLCK : F_RDLCK);
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
	transmit_4result(NLMPROC4_LOCK_MSG,
	    nlmproc4_lock_4_svc(arg, rqstp), rqstp);
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

	lock_answer(&arg->cookie, arg->stat.stat, NLM4_VERS);

	return (NULL);
}

/*
 * nlm4_cancel --
 *	Cancel a blocked lock request.
 */
nlm4_res *
nlmproc4_cancel_4_svc(nlm4_cancargs *arg, struct svc_req *rqstp)
{
	static nlm4_res res;

	LOG_CALL("nlmproc4_cancel_4_svc", rqstp);

	if (d_args)
		show_lock(&arg->alock);

	/*
	 * Grace period (NLM4_DENIED_GRACE_PERIOD) not implemented, and I see
	 * no reason to ever do so.
	 *
	 * XXX
	 * Blocking locks (NLM4_BLOCKED) not implemented.  Since we currently
	 * never return 'NLM4_BLOCKED', there can never be a lock to cancel,
	 * so this call always fails.
	 */
	res.cookie = arg->cookie;
	res.stat.stat = NLM4_DENIED;
	return (NULL);
}

/*
 * nlm4_cancel_msg --
 *	Cancel a blocked lock request message.
 */
void *
nlmproc4_cancel_msg_4_svc(nlm4_cancargs *arg, struct svc_req *rqstp)
{
	transmit_4result(NLMPROC4_CANCEL_MSG,
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
		    arg->alock.fh.n_bytes, arg->alock.fh.n_len,
		    arg->alock.oh.n_bytes, arg->alock.oh.n_len,
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
	transmit_4result(NLMPROC4_UNLOCK_MSG,
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

	lock_answer(&arg->cookie, NLM4_GRANTED, NLM4_VERS);

	res.cookie = arg->cookie;
	res.stat.stat = NLM4_GRANTED;

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
	transmit_4result(NLMPROC4_GRANTED_MSG,
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
		res.stat = setlock(rqstp, arg->share.caller_name,
				arg->share.fh.n_bytes, 
				arg->share.fh.n_len,
				0,
				(u_quad_t)0, (u_quad_t)0,
				arg->share.mode == fsm_DR ? F_RDLCK : F_WRLCK);
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
		    arg->share.fh.n_bytes, arg->share.fh.n_len,
		    arg->share.oh.n_bytes, arg->share.oh.n_len,
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
	    opcode, xdr_nlm4_res, result, xdr_void, &dummy, timeout);
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

	syslog(LOG_DEBUG, "pid: %ld; offset: %qu; len: %qu\n",
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
	u_int8_t *p, *t, buf[64];

	fsidp = (fsid_t *)cookie->n_bytes;
	fidp = (struct fid *)((u_int8_t *)cookie->n_bytes + sizeof(fsid_t));

	for (t = buf, p = (u_int8_t *)fidp->fid_data,
	    len = fidp->fid_len; len > 0; ++p, --len) {
		*t++ = '\\';
		*t++ = hex[(*p & 0xf0) >> 4];
		*t++ = hex[*p & 0x0f];
	}
	*t = '\0';

	syslog(LOG_DEBUG, "%s: fsid_t [%d][%d]; fid_len %d; fid_data %s",
	    msg, (int)fsidp->val[0], (int)fsidp->val[1], fidp->fid_len, buf);
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
