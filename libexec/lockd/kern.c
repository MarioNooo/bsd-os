/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI kern.c,v 1.4 2001/10/03 17:29:55 polk Exp
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "nlm_prot.h"
#include "nfs/nfs_lock.h"

#include "lockd.h"

/* Lock request owner. */
typedef struct __owner {
	pid_t	 pid;				/* Process ID. */
	time_t	 tod;				/* Time-of-day. */
} OWNER;
static OWNER owner;

static char hostname[MAXHOSTNAMELEN + 1];	/* Hostname. */

int	lock_request(LOCKD_MSG *);
int	test_request(LOCKD_MSG *);
void	show(LOCKD_MSG *);
int	unlock_request(LOCKD_MSG *);

void
client_cleanup(sig, code)
	int sig;
	int code;
{
	(void)seteuid(0);
	(void)unlink(_PATH_LCKFIFO);
	exit(-1);
}

/*
 * client_request --
 *	Loop around messages from the kernel, forwarding them off to
 *	NLM servers.
 */
pid_t
client_request(void)
{
	LOCKD_MSG msg;
	fd_set rdset;
	int fd, nr, ret;
	pid_t child;

	/* Recreate the NLM fifo. */
	(void)unlink(_PATH_LCKFIFO);
	(void)umask(S_IXGRP|S_IXOTH);
	if (mkfifo(_PATH_LCKFIFO, S_IWUSR | S_IRUSR)) {
		syslog(LOG_ERR, "mkfifo: %s: %m", _PATH_LCKFIFO);
		exit (1);
	}

	/*
	 * Create a separate process, the client code is really a separate
	 * daemon that shares a lot of code.
	 */
	switch (child = fork()) {
	case -1:
		err(1, "fork");
	case 0:
		break;
	default:
		return (child);
	}

	signal(SIGHUP, client_cleanup);
	signal(SIGTERM, client_cleanup);

	/* Setup. */
	(void)time(&owner.tod);
	owner.pid = getpid();
	(void)gethostname(hostname, sizeof(hostname) - 1);

	/* Open the fifo for reading. */
	if ((fd = open(_PATH_LCKFIFO, O_RDONLY | O_NONBLOCK)) < 0)
		syslog(LOG_ERR, "open: %s: %m", _PATH_LCKFIFO);

	/* drop our root priviledges */
	(void)seteuid(daemon_uid);

	/* Set up the select. */
	FD_ZERO(&rdset);
	FD_SET(fd, &rdset);

	for (;;) {
		/* Wait for contact... fifo's return EAGAIN when read with 
		 * no data
		 */
		(void)select(fd + 1, &rdset, NULL, NULL, NULL);

		/* Read the fixed length message. */
		if ((nr = read(fd, &msg, sizeof(msg))) == sizeof(msg)) {
			if (d_args)
				show(&msg);

			if (msg.version != LOCKD_MSG_VERSION) {
				syslog(LOG_ERR,
				    "unknown msg type: %d", msg.version);
			}
			/*
			 * Send it to the NLM server and don't grant the lock
			 * if we fail for any reason.
			 */
			switch (msg.fl.l_type) {
			case F_RDLCK:
			case F_WRLCK:
				if (msg.getlk)
					ret = test_request(&msg);
				else
					ret = lock_request(&msg);
				break;
			case F_UNLCK:
				ret = unlock_request(&msg);
				break;
			default:
				ret = 1;
				syslog(LOG_ERR,
				    "unknown lock type: %d", msg.fl.l_type);
				break;
			}
			if (ret) {
				struct lockd_ans ans;

				ans.msg_ident = msg.msg_ident;
				ans.errno = EHOSTUNREACH;

				if (nfslockdans(LOCKD_ANS_VERSION, &ans)) {
					syslog((errno == EPIPE ? LOG_INFO : 
						LOG_ERR), "process %lu: %m",
						(u_long)msg.msg_ident.pid);
				}
			}
		} else if (nr == -1) {
			if (errno != EAGAIN) {
				syslog(LOG_ERR, "read: %s: %m", _PATH_LCKFIFO);
				goto err;
			}
		} else if (nr != 0) {
			syslog(LOG_ERR,
			    "%s: discard %d bytes", _PATH_LCKFIFO, nr);
		}
	}

	/* Reached only on error. */
err:
	(void)seteuid(0);
	(void)unlink(_PATH_LCKFIFO);
	_exit (1);
}

void
set_auth(
	CLIENT *cl,
	struct ucred *ucred
)
{
        if (cl->cl_auth != NULL)
                cl->cl_auth->ah_ops->ah_destroy(cl->cl_auth);
        cl->cl_auth = authunix_create(hostname,
                        ucred->cr_uid,
                        ucred->cr_groups[0],
                        ucred->cr_ngroups-1,
                        &ucred->cr_groups[1]);
}


/*
 * test_request --
 *	Convert a lock LOCKD_MSG into an NLM request, and send it off.
 */
int
test_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "test request: %s: %s to %s",
		    msg->nfsv3 ? "V4" : "V1/3",
		    msg->fl.l_type == F_WRLCK ? "write" : "read",
		    from_addr((struct sockaddr_in *)&msg->addr));

	if (msg->nfsv3) {
		struct nlm4_testargs arg4;

		arg4.cookie.n_bytes = (char *)&msg->msg_ident;
		arg4.cookie.n_len = sizeof(msg->msg_ident);
		arg4.exclusive = msg->fl.l_type == F_WRLCK ? 1 : 0;
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->fh;
		arg4.alock.fh.n_len = msg->fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->msg_ident.pid;
		arg4.alock.l_offset = msg->fl.l_start;
		arg4.alock.l_len = msg->fl.l_len;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM4_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_TEST_MSG,
		    xdr_nlm4_testargs, (caddr_t) &arg4, xdr_void, &dummy, timeout);
	} else {
		struct nlm_testargs arg;

		arg.cookie.n_bytes = (char *)&msg->msg_ident;
		arg.cookie.n_len = sizeof(msg->msg_ident);
		arg.exclusive = msg->fl.l_type == F_WRLCK ? 1 : 0;
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->fh;
		arg.alock.fh.n_len = msg->fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->msg_ident.pid;
		arg.alock.l_offset = msg->fl.l_start;
		arg.alock.l_len = msg->fl.l_len;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_TEST_MSG,
		    xdr_nlm_testargs, (caddr_t) &arg, xdr_void, &dummy, timeout);
	}
	return (0);
}

/*
 * lock_request --
 *	Convert a lock LOCKD_MSG into an NLM request, and send it off.
 */
int
lock_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct nlm4_lockargs arg4;
	struct nlm_lockargs arg;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "lock request: %s: %s to %s",
		    msg->nfsv3 ? "V4" : "V1/3",
		    msg->fl.l_type == F_WRLCK ? "write" : "read",
		    from_addr((struct sockaddr_in *)&msg->addr));

	if (msg->nfsv3) {
		arg4.cookie.n_bytes = (char *)&msg->msg_ident;
		arg4.cookie.n_len = sizeof(msg->msg_ident);
		arg4.block = msg->wait ? 1 : 0;
		arg4.exclusive = msg->fl.l_type == F_WRLCK ? 1 : 0;
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->fh;
		arg4.alock.fh.n_len = msg->fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->msg_ident.pid;
		arg4.alock.l_offset = msg->fl.l_start;
		arg4.alock.l_len = msg->fl.l_len;
		arg4.reclaim = 0;
		arg4.state = nsm_state;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM4_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_LOCK_MSG,
		    xdr_nlm4_lockargs, (caddr_t) &arg4, xdr_void, &dummy, timeout);
	} else {
		arg.cookie.n_bytes = (char *)&msg->msg_ident;
		arg.cookie.n_len = sizeof(msg->msg_ident);
		arg.block = msg->wait ? 1 : 0;
		arg.exclusive = msg->fl.l_type == F_WRLCK ? 1 : 0;
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->fh;
		arg.alock.fh.n_len = msg->fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->msg_ident.pid;
		arg.alock.l_offset = msg->fl.l_start;
		arg.alock.l_len = msg->fl.l_len;
		arg.reclaim = 0;
		arg.state = nsm_state;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_LOCK_MSG,
		    xdr_nlm_lockargs, (caddr_t) &arg, xdr_void, &dummy, timeout);
	}
	return (0);
}

/*
 * unlock_request --
 *	Convert an unlock LOCKD_MSG into an NLM request, and send it off.
 */
int
unlock_request(LOCKD_MSG *msg)
{
	CLIENT *cli;
	struct nlm4_unlockargs arg4;
	struct nlm_unlockargs arg;
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	char dummy;

	if (d_calls)
		syslog(LOG_DEBUG, "unlock request: %s: to %s",
		    msg->nfsv3 ? "V4" : "V1/3",
		    from_addr((struct sockaddr_in *)&msg->addr));

	if (msg->nfsv3) {
		arg4.cookie.n_bytes = (char *)&msg->msg_ident;
		arg4.cookie.n_len = sizeof(msg->msg_ident);
		arg4.alock.caller_name = hostname;
		arg4.alock.fh.n_bytes = (char *)&msg->fh;
		arg4.alock.fh.n_len = msg->fh_len;
		arg4.alock.oh.n_bytes = (char *)&owner;
		arg4.alock.oh.n_len = sizeof(owner);
		arg4.alock.svid = msg->msg_ident.pid;
		arg4.alock.l_offset = msg->fl.l_start;
		arg4.alock.l_len = msg->fl.l_len;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM4_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_UNLOCK_MSG,
		    xdr_nlm4_unlockargs, (caddr_t) &arg4, xdr_void, &dummy, timeout);
	} else {
		arg.cookie.n_bytes = (char *)&msg->msg_ident;
		arg.cookie.n_len = sizeof(msg->msg_ident);
		arg.alock.caller_name = hostname;
		arg.alock.fh.n_bytes = (char *)&msg->fh;
		arg.alock.fh.n_len = msg->fh_len;
		arg.alock.oh.n_bytes = (char *)&owner;
		arg.alock.oh.n_len = sizeof(owner);
		arg.alock.svid = msg->msg_ident.pid;
		arg.alock.l_offset = msg->fl.l_start;
		arg.alock.l_len = msg->fl.l_len;

		if ((cli = get_client(NULL,
		    (struct sockaddr_in *)&msg->addr,
		    NLM_PROG, NLM_VERS)) == NULL)
			return (1);

		set_auth(cli, &msg->cred);
		(void)clnt_call(cli, NLM_UNLOCK_MSG,
		    xdr_nlm_unlockargs, (caddr_t) &arg, xdr_void, &dummy, timeout);
	}

	return (0);
}

int
lock_answer(int pid, netobj *netcookie, int result, int *pid_p, int version)
{
	struct lockd_ans ans;

	if (netcookie->n_len != sizeof(ans.msg_ident)) {
		if (pid == -1) {	/* we're screwed */
			syslog(LOG_ERR, "inedible nlm cookie");
			return -1;
		}
		ans.msg_ident.pid = pid;
		ans.msg_ident.msg_seq = -1;
	} else {
		memcpy(&ans.msg_ident, netcookie->n_bytes, 
							sizeof(ans.msg_ident));
	}

	if (d_calls)
		syslog(LOG_DEBUG, "lock answer: pid %lu: %s",
		    ans.msg_ident.pid, version == NLM4_VERS ?
		    show_4state(result) : show_state(result));

	ans.set_getlk_pid = 0;
	if (version == NLM4_VERS)
		switch (result) {
		case NLM4_GRANTED:
			ans.errno = 0;
			break;
		default:
			ans.errno = EACCES;
			break;
		case NLM4_DENIED:
			if (pid_p == NULL)
				ans.errno = EACCES;
			else {
				/* this is an answer to a nlm_test msg */
				ans.set_getlk_pid = 1;
				ans.getlk_pid = *pid_p;
				ans.errno = 0;
			}
			break;
		case NLM4_DENIED_NOLOCKS:
			ans.errno = EAGAIN;
			break;
		case NLM4_BLOCKED:
			return -1;
			/* NOTREACHED */
		case NLM4_DENIED_GRACE_PERIOD:
			ans.errno = EAGAIN;
			break;
		case NLM4_DEADLCK:
			ans.errno = EDEADLK;
			break;
		case NLM4_ROFS:
			ans.errno = EROFS;
			break;
		case NLM4_STALE_FH:
			ans.errno = ESTALE;
			break;
		case NLM4_FBIG:
			ans.errno = EFBIG;
			break;
		case NLM4_FAILED:
			ans.errno = EACCES;
			break;
		}
	else
		switch (result) {
		case nlm_granted:
			ans.errno = 0;
			break;
		default:
			ans.errno = EACCES;
			break;
		case nlm_denied:
			if (pid_p == NULL)
				ans.errno = EACCES;
			else {
				/* this is an answer to a nlm_test msg */
				ans.set_getlk_pid = 1;
				ans.getlk_pid = *pid_p;
				ans.errno = 0;
			}
			break;
		case nlm_denied_nolocks:
			ans.errno = EAGAIN;
			break;
		case nlm_blocked:
			return -1;
			/* NOTREACHED */
		case nlm_denied_grace_period:
			ans.errno = EAGAIN;
			break;
		case nlm_deadlck:
			ans.errno = EDEADLK;
			break;
		}

	if (nfslockdans(LOCKD_ANS_VERSION, &ans)) {
		syslog(((errno == EPIPE || errno == ESRCH) ? 
			LOG_INFO : LOG_ERR), 
			"process %lu: %m", (u_long)ans.msg_ident.pid);
		return -1;
	}
	return 0;
}

/*
 * show --
 *	Display the contents of a kernel LOCKD_MSG structure.
 */
void
show(LOCKD_MSG *mp)
{
	static char hex[] = "0123456789abcdef";
	struct fid *fidp;
	fsid_t *fsidp;
	size_t len;
	u_int8_t *p, *t, buf[NFS_SMALLFH*3+1];

	printf("process ID: %lu\n", (long)mp->msg_ident.pid);

	fsidp = (fsid_t *)&mp->fh;
	fidp = (struct fid *)((u_int8_t *)&mp->fh + sizeof(fsid_t));

	for (t = buf, p = (u_int8_t *)mp->fh,
	    len = mp->fh_len;
	    len > 0; ++p, --len) {
		*t++ = '\\';
		*t++ = hex[(*p & 0xf0) >> 4];
		*t++ = hex[*p & 0x0f];
	}
	*t = '\0';

	printf("fh_len %d, fh %s\n", mp->fh_len, buf);

	/* Show flock structure. */
	printf("start %qu; len %qu; pid %lu; type %d; whence %d\n",
	    mp->fl.l_start, mp->fl.l_len, (u_long)mp->fl.l_pid,
	    mp->fl.l_type, mp->fl.l_whence);

	/* Show wait flag. */
	printf("wait was %s\n", mp->wait ? "set" : "not set");
}
