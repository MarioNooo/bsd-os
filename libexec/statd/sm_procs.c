/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI sm_procs.c,v 1.3 2001/10/03 17:29:56 polk Exp
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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <rpc/rpc.h>

#include <stddef.h>		/* for queue stuff */
#include <sys/queue.h>		/* ditto */


#include <limits.h>
#include <db.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <err.h>

#include "sm_inter.h"
#include "statd.h"

#define	LOG_CALL(func, rqstp)						\
	if (d_calls)							\
		syslog(LOG_DEBUG, "%s: from %s", func,			\
		    from_addr(svc_getcaller((rqstp)->rq_xprt)));
#define	LOG_RES(st)							\
	syslog(LOG_DEBUG, st == stat_fail ?				\
	    "result: %s: %m" : "result: %s", show_state(st));

#define SIMPLE

#ifdef SIMPLE
/*
 */
typedef struct cb_entry {
	char	priv[16];			/* mon.priv */
	struct my_id	my_id;
	LIST_ENTRY(cb_entry) entries;
} CB_ENTRY;

LIST_HEAD(lentry_listhead, cb_entry);

int hn_records;

struct hn_entry {
	int recno;		/* the on-disk record number (0 based) */
	int hostname_len;
	char *hostname;
	LIST_ENTRY(hn_entry) entries;
	struct lentry_listhead cb_head;
};

LIST_HEAD(hn_listhead, hn_entry) hn_head;
struct hn_listhead *hn_headp;

/*
 * Where we keep old entries for re-use.
 */
struct hn_listhead avail_hn_head;
struct hn_listhead *avail_hn_headp;

struct hn_ondisk {
	int 	hostname_len;
	char	hostname[SM_MAXSTRLEN];
};


#else

/*
 * Our data structures are a bit complex.  There are three different databases,
 * one on-disk, two in-core.
 *
 * 1) hosts --		a simple fixed-size record array which names each host 
 *			we've agreed to notify when our state changes.  This 
 *			lives on-disk.
 *
 * 3) mon_name_tbl --	a hash table based on the mon_name,
 * 2) mon_name_tbl --	a hash table based on the mon_name,


 *There is one on-disk hostname  two in
 *
 *
 */

/*
 * Our in-core data structure, is a hash table formed from
 *
 */
typedef struct _stat_key {
	char	
} STAT_KEY;

/*
 * The on-disk array of hosts we've agreed to notify when our state changes.
 */
struct notify_ent {
	int	len;
	char 	hostname[SM_MAXSTRLEN];
};

/*
 * The in-core list of hosts we've agreed to notify.
 */
struct notify_list {
}
#endif /* SIMPLE */

char 	my_hostname[SM_MAXSTRLEN];		/* set very early by 
						   send_sm_notify() */

res	 add(mon *arg);
int	 del(mon_id *arg);
char	*from_addr(struct sockaddr_in *);
char	*show_state(int);
void	 recv_sm_notify(stat_chge *);

int
my_id_equal(struct my_id *p1, struct my_id *p2)
{
	int cmp;
	if ((p1->my_prog == p2->my_prog) && 
	    (p1->my_vers == p2->my_vers) &&
	    (p1->my_proc == p2->my_proc)) {
		cmp = strcmp(p1->my_name, p2->my_name);
		return (cmp == 0);
	}
	else
		return (FALSE);
}

/*
 * Walk through the current in-core hostname list, and return then entry
 * if it is found.
 */
struct hn_entry *
hn_lookup(char *mon_name, int *lenp)
{
	struct hn_entry *np;
	int len;

	*lenp = len = strlen(mon_name);

	if (d_cache)
		syslog(LOG_DEBUG, "lookup %s", mon_name);

	for (np = LIST_FIRST(hn_headp); 
	     np != LIST_END(hn_headp);
	     np = LIST_NEXT(np, entries)) {
		if (len == np->hostname_len &&
		    memcmp(np->hostname, mon_name, len) == 0) {
			break;
		}
	}
	return (np);
}

struct cb_entry *
cb_lookup(struct lentry_listhead *cb_headp, struct my_id *my_idp)
{
	struct cb_entry *cb;

	if (d_cache)
		syslog(LOG_DEBUG, "lookup %d/%d/%d %s", my_idp->my_prog,
			my_idp->my_vers, my_idp->my_proc, my_idp->my_name);
	for (cb = LIST_FIRST(cb_headp); 
	     cb != LIST_END(cb_headp);
	     cb = LIST_NEXT(cb, entries)) {
		if (my_id_equal(&cb->my_id, my_idp)) {
			return (cb);
		}
	}

	return (NULL);
}

/*
 * sm_stat --
 *	Check status.
 */
sm_stat_res *
sm_stat_1_svc(sm_name *arg, struct svc_req *rqstp)
{
	static sm_stat_res res;

	LOG_CALL("sm_stat_1_svc", rqstp);

	res.res_stat =
	    gethostbyname(arg->mon_name) == NULL ? stat_fail : stat_succ;
	res.state = our_state(0);

	LOG_RES(res.res_stat);
	return (&res);
}

/*
 * sm_mon --
 *	Monitor host.
 */
sm_stat_res *
sm_mon_1_svc(mon *arg, struct svc_req *rqstp)
{
	static sm_stat_res res;

	LOG_CALL("sm_mon_1_svc", rqstp);

	res.res_stat = add(arg);
	res.state = our_state(0);

	LOG_RES(res.res_stat);
	return (&res);
}

/*
 * sm_unmon --
 *	Unmonitor host.
 */
sm_stat *
sm_unmon_1_svc(mon_id *arg, struct svc_req *rqstp)
{
	static sm_stat res;

	LOG_CALL("sm_unmon_1_svc", rqstp);

	(void)del(arg);				/* !!!: No failure retrurn. */
	res.state = our_state(0);

	return (&res);
}

/*
 * sm_unmon_all --
 *	Unmonitor all hosts.
 */
sm_stat *
sm_unmon_all_1_svc(my_id *arg, struct svc_req *rqstp)
{
	static sm_stat res;

	arg = arg;				/* XXX: Shut the compiler up. */

	LOG_CALL("sm_unmon_all_1_svc", rqstp);

	/* Discard all monitor information. */
	init_statfp(1);

	res.state = our_state(0);

	return (&res);
}

/*
 * sm_simu_crash --
 *	Simulate a crash.
 */
void *
sm_simu_crash_1_svc(void *arg, struct svc_req *rqstp)
{
	static char result;
	arg = arg;				/* XXX: Shut the compiler up. */

	LOG_CALL("sm_simu_crash_1_svc", rqstp);

	/* Update our status number. */
	(void)our_state(1);

	/* Send out the SM_NOTIFY calls. */
	send_sm_notify();
		 
	return ((void *)&result);
}

/*
 * sm_notify --
 *	Accept notification of change.
 */
void *
sm_notify_1_svc(stat_chge *arg, struct svc_req *rqstp)
{
	static char result;

	LOG_CALL("sm_notify_1_svc", rqstp);

	/* Receive the SM_NOTIFY call. */
	recv_sm_notify(arg);

	return ((void *)&result);
}

/*
 * Like strncpy(), but return the length copied and don't zero pad.
 * The returned length includes the terminating null byte (if it fits).
 */
int
strncopy(char *dest, const char *src, size_t len)
{
	char *orig_dest;
	
	for (orig_dest = dest; len-- > 0 && (*(dest++) = *(src++)) != '\0';)
		if ((*(dest++) = *(src++)) == '\0') {
			return(dest - orig_dest + 1);
		}

	return(dest - orig_dest);
}

/*
 * recv_sm_notify --
 *	Handle an incoming SM_NOTIFY RPC.
 */
void
recv_sm_notify(stat_chge *arg)
{
	struct timeval timeout = {0, 0};	/* No timeout, no response. */
	status st_arg;
	CLIENT *cli;
	char name_buf[SM_MAXSTRLEN+1];
	char dummy;
	struct hn_entry *np;
	struct cb_entry *cb;
	struct lentry_listhead *cb_headp;
	int len;

	/* anyone interested in the named host? */
	if ((np = hn_lookup(arg->mon_name, &len)) == NULL)
		return;

	/* Set up constant information. */
	st_arg.mon_name = name_buf;
	memcpy(st_arg.mon_name, arg->mon_name, len);
	name_buf[len] = '\0';
	st_arg.state = arg->state;

	/* perform each call back on the list for this host */
	cb_headp = &np->cb_head;
	for (cb = LIST_FIRST(cb_headp); 
	     cb != LIST_END(cb_headp);
	     cb = LIST_NEXT(cb, entries)) {
		/* Set up per-entry information. */
		memcpy(st_arg.priv, cb->priv, sizeof(cb->priv));

		if ((cli = get_client(cb->my_id.my_name,
		    NULL, cb->my_id.my_prog, cb->my_id.my_vers)) == NULL)
			continue;
		(void)clnt_call(cli, cb->my_id.my_proc,
		    xdr_status, (caddr_t) &st_arg, xdr_void, &dummy, timeout);
		/* XXX what if we fail */
	}
}

/*
 * send_sm_notify --
 *	Notify all the registered hosts that we're starting up.
 *
 *  XXXX we assume all the notify messages get through, which will not always
 *       be correct.
 */
void
send_sm_notify()
{
	struct stat sb;
	struct timeval timeout = {0, 0};	/* no wait, no result */
	CLIENT *cli;
	struct hn_ondisk *hn_array, *lep;
	stat_chge tx_arg;
	int entries, state;
	char dummy;

	/*
	 * XXX
	 * I'm not sure what happens if we get a SM_SIMU_CRASH RPC at this
	 * point.  It's going to be strange, regardless.  Cache the state
	 * so that it doesn't change underfoot, at least.
	 */
	state = our_state(0);

	/*
	 * XXX
	 * If MAXHOSTNAMELEN > SM_MAXSTRLEN, I think we're screwed.
	 */
	if (gethostname(my_hostname, sizeof(my_hostname))) {
		syslog(LOG_ERR, "gethostbyname: %m");
		return;
	}

	/* Find out how big the file is. */
	if (fstat(fileno(statfp), &sb) != 0) {
		syslog(LOG_ERR, "%s: %m", _PATH_STATD_STATUS);
		return;
	}
	if (sb.st_size < sizeof(struct hn_ondisk))
		return;

	/* Allocate enough memory to hold it. */
	if ((hn_array = (struct hn_ondisk *)malloc(sb.st_size)) == NULL) {
		syslog(LOG_ERR, "%s: %m", _PATH_STATD_STATUS);
		return;
	}

	/* Read it in. */
	(void)rewind(statfp);
	entries = sb.st_size / sizeof(struct  hn_ondisk);
	if (fread(hn_array, sizeof(struct  hn_ondisk), entries, statfp) 
								!= entries) {
		syslog(LOG_ERR, "%s: short read", _PATH_STATD_STATUS);
		free(hn_array);
		return;
	}

#ifdef NDEF
	/*
	 * This may take some time, so we fork a child and let the parent
	 * go back to doing whatever it was doing.
	 */
	switch (fork()) {
	case -1:
		syslog(LOG_ERR, "fork: %m");
		return;
	case 0:
		break;
	default:
		free(hn_array);
		return;
	}
#endif

	for (lep = hn_array; entries > 0; ++lep, --entries) {
		if (lep->hostname_len == 0)
			continue;

		tx_arg.mon_name = my_hostname;
		tx_arg.state = our_state(0);

		if ((cli = get_client(lep->hostname, NULL, SM_PROG, 
							SM_VERS)) == NULL)
			continue;
		(void)clnt_call(cli, SM_NOTIFY,
		    xdr_status, (caddr_t) &tx_arg, xdr_void, &dummy, timeout);
	}
#ifdef NDEF
	_exit(0);
#endif
}

void
init_statfp(int discard)
{
	int fd;

	hn_records = 0;
	hn_headp = &hn_head;
	LIST_INIT(hn_headp);

	avail_hn_headp = &avail_hn_head;
	LIST_INIT(avail_hn_headp);

	if (statfp != NULL) {
		(void)fclose(statfp);
		statfp = NULL;
	}

	if (discard)
		(void)unlink(_PATH_STATD_STATUS);

	if ((fd = open(_PATH_STATD_STATUS,
	    O_RDWR | O_CREAT | (discard ? O_TRUNC : 0), S_IRUSR | S_IWUSR)) < 0)
		err(1, "%s", _PATH_STATD_STATUS);

	if ((statfp = fdopen(fd, "r+")) == NULL)
		err(1, "%s", _PATH_STATD_STATUS);
}


/*
 * add --
 *	Add a host to the list of monitored hosts.
 */
res
add(mon *arg)
{
#ifdef SIMPLE
	struct hn_entry *np;
	struct cb_entry *cb;
	int len;

	/* are we already monitoring the named host? */
	np = hn_lookup(arg->mon_id.mon_name, &len);

	/* if we hit the end, we didn't find this hostname, so add it */
	if (np == NULL) {
		struct hn_ondisk ondisk;

		if (d_cache)
			syslog(LOG_DEBUG, "add %s", arg->mon_id.mon_name);

		/* is a node available from the avail list */
		if ((np = LIST_FIRST(avail_hn_headp)) != 
						LIST_END(avail_hn_headp)) {
			LIST_REMOVE(np, entries);
		} else {
			if ((np = (struct hn_entry *)malloc(
					sizeof(struct hn_entry))) == NULL) {
				syslog(LOG_ERR, "malloc: %m");
				return (stat_fail);
			}
			np->recno = hn_records++;
			LIST_INIT(&np->cb_head);
		}

		/*
		 * If anything fails hereafter, put the node on the avail
		 * list, so the recno will get re-used.
		 */

		if ((np->hostname = (char *)malloc(len+1)) == NULL) {
			syslog(LOG_ERR, "malloc: %m");
			LIST_INSERT_HEAD(avail_hn_headp, np, entries);
			return (stat_fail);
		}
		memcpy(np->hostname, arg->mon_id.mon_name, len+1);
		np->hostname_len = len;

		/*
	 	 * update the on-disk hostname array.  These are fixed-length,
		 * zero-padded records.
		 */
		if (fseek(statfp, sizeof(struct hn_ondisk) * np->recno, 
							SEEK_SET) == -1) {
			syslog(LOG_ERR, "fseek: %m");
			free((void *)(np->hostname));
			np->hostname = NULL;
			LIST_INSERT_HEAD(avail_hn_headp, np, entries);
			return (stat_fail);
		}
		ondisk.hostname_len = np->hostname_len;
		memcpy(ondisk.hostname, np->hostname, ondisk.hostname_len);
		memset(ondisk.hostname + ondisk.hostname_len, 0, 
				sizeof(ondisk.hostname) - ondisk.hostname_len);

		if (fwrite(&ondisk, sizeof(ondisk), 1, statfp) != 1 || 
							fflush(statfp) == EOF) {
			syslog(LOG_ERR, "%s: %m", _PATH_STATD_STATUS);
			free((void *)(np->hostname));
			np->hostname = NULL;
			LIST_INSERT_HEAD(avail_hn_headp, np, entries);
			return (stat_fail);
		}
		LIST_INSERT_HEAD(hn_headp, np, entries);
	}

	/*
	 * Look for a duplicate (we don't want any).
	 */
	
	cb = cb_lookup(&np->cb_head, &arg->mon_id.my_id);

	/* if we didn't hit the end, we found a duplicate, so we're done */
	if (cb != NULL) {
		syslog(LOG_INFO, "duplicate add (%s %s %d %d %d): ",
				np->hostname, cb->my_id.my_name,
				cb->my_id.my_prog,
				cb->my_id.my_vers,
				cb->my_id.my_proc);
		return (stat_succ);
	}


	/* add it in */
	if ((cb = (struct cb_entry *)malloc(
			sizeof(struct cb_entry))) == NULL) {
		syslog(LOG_ERR, "malloc: %m");
		return (stat_fail);
	}

	/* Copy in the information. */
	memcpy(cb->priv, arg->priv, sizeof(arg->priv));

	len = strlen(arg->mon_id.my_id.my_name);
	if ((cb->my_id.my_name = (char *)malloc(len+1)) == NULL) {
		syslog(LOG_ERR, "malloc: %m");
		free(cb);
		return (stat_fail);
	}
	memcpy(cb->my_id.my_name, arg->mon_id.my_id.my_name, len+1); 
	cb->my_id.my_prog = arg->mon_id.my_id.my_prog;
	cb->my_id.my_vers = arg->mon_id.my_id.my_vers;
	cb->my_id.my_proc = arg->mon_id.my_id.my_proc;

	LIST_INSERT_HEAD(&np->cb_head, cb, entries);
#else

	if (stat_db->put(stat_db, key, data, R_NOOVERWRITE) < 0) {
		syslog(LOG_ERR, "%s: %m", _PATH_STATD_STATUS);
		return (stat_fail);
	}
#endif

	return (stat_succ);
}

/*
 * del --
 *	Delete a host from the list of monitored hosts.
 */
int
del(mon_id *arg)
{
	struct hn_entry *np;
	struct cb_entry *cb;
	struct lentry_listhead *cb_headp;
	int len;
	struct hn_ondisk ondisk;

	/*
 	 * Scan the hostname list.
	 */
	np = hn_lookup(arg->mon_name, &len);
	if (np == NULL)
		return (FALSE);
		
	/*
 	 * Scan the hosts callback list.
	 */
	cb_headp = &np->cb_head;
	cb = cb_lookup(cb_headp, &arg->my_id);
	if (cb == NULL)
		return (FALSE);

	LIST_REMOVE(cb, entries);
	free(cb->my_id.my_name);
	free(cb);

	/* not the last callback ? */
	if (LIST_FIRST(cb_headp) != LIST_END(cb_headp))
		return (TRUE);

	/*
	 * This was the last callback for this host, so delete it from the
	 * host list, and remove the entry from the on-disk notify list.
	 */

	LIST_REMOVE(np, entries);
	free((void *)(np->hostname));
	np->hostname = NULL;
	LIST_INSERT_HEAD(avail_hn_headp, np, entries);

	/*
	 * update the on-disk hostname array.  These are fixed-length,
	 * zero-padded records.  If anything fails from here-on out, we 
	 * really don't care, as the only side effect is an unneccessary
	 * notify to a un-monitored host.
	 */
	if (fseek(statfp, sizeof(struct hn_ondisk) * np->recno, 
						SEEK_SET) == -1) {
		syslog(LOG_ERR, "fseek(%s): %m", _PATH_STATD_STATUS);
	} else {
		ondisk.hostname_len = 0;
		if (fwrite(&ondisk, sizeof(ondisk), 1, statfp) != 1) {
			syslog(LOG_ERR, "fwrite(%s): %m", _PATH_STATD_STATUS);
		}
	}

	return (TRUE);
}

/*
 * our_state --
 *	Track the local server's state.
 */
int
our_state(int update)
{
	static int st = -1;
	int fd, nr;
	char buf[64];

#define	MAX_STATE	32768		/* Sheer, raging paranoia. */
	if (st == -1) {
		if ((fd = open(_PATH_STATD_STATE, O_RDONLY, 0)) >= 0) {
			nr = read(fd, buf, sizeof(buf));
			(void)close(fd);
			if (nr >= 1 &&
			    (st = atoi(buf)) > 0 && st <= MAX_STATE && !update)
				return (st);
		}
		update = 1;
	}
	if (update) {
		if ((st += 2) >= MAX_STATE)
			st = 1;
		if ((fd = open(_PATH_STATD_STATE,
		    O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
			syslog(LOG_ERR, "%s: %m", _PATH_STATD_STATE);
			exit (1);
		}
		/*
		 * RACE: the file may be left corrupted or empty, but we
		 * don't care, we'll just reinitialize.
		 */
		(void)write(fd, buf, (int)sprintf(buf, "%d\n", st));
		(void)close(fd);

		syslog(LOG_INFO,
		    "%s: status updated to %d", _PATH_STATD_STATE, st);
	}

	return (st);
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
	int len;

	if ((host = gethostbyaddr((const char *)&addr->sin_addr,
	    addr->sin_len, AF_INET)) == NULL)
		len = strncopy(hostname_buf,
		    inet_ntoa(addr->sin_addr), sizeof(hostname_buf));
	else {
		len = strncopy(hostname_buf,
		    host->h_name, sizeof(hostname_buf));
	}
	if (len == sizeof(hostname_buf))	/* force null termination ? */
		hostname_buf[sizeof(hostname_buf)-1] = '\0';
	return (hostname_buf);
}

/*
 * show_state --
 *	Display the state of a res structure.
 */
char *
show_state(int stat)
{
	switch (stat) {
	case stat_succ:
		return ("success");
	case stat_fail:
		return ("failure");
	}
	return("unknown state value");
}
