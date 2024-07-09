/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI lockd.h,v 1.1 1998/03/19 20:18:10 don Exp
 */

#include <sys/param.h>			/* for MAXHOSTNAMELEN */
#include <sys/types.h>
#include <limits.h>
#include <db.h>


extern int nsm_state;			/* NSM state. */

extern DB *ownerdb;			/* the owner DB handle */
extern DB *hostdb;			/* the host DB handle */
extern int check_grace;			/* check the grace period ? */
extern time_t grace_end;		/* End of grace period. */
extern char my_hostname[MAXHOSTNAMELEN+1];	/* null terminated host name */
extern struct mon mon_host;
extern gid_t nogroup;
extern uid_t nobody; 
extern uid_t daemon_uid; 

/* Debugging flags. */
extern int d_args;			/* Show arguments. */
extern int d_cache;			/* Show cacheing behavior. */
extern int d_calls;			/* Show requests/replies. */
extern int d_bg;			/* Show background activites */

/* Exported functions. */
pid_t	client_request(void);
char   *from_addr(struct sockaddr_in *);
CLIENT *get_client(char *, struct sockaddr_in *, int, int);
int	lock_answer(int, netobj *, int, int *, int);
void	show_netobj(char *, netobj *);
char   *show_4state(nlm4_stats);
char   *show_state(int);
void	dump_hostdb();
void	dump_ownerdb();
void	alarm_handler(int sig);

/*
 * XXX
 * No prototype for callrpc() in BSD/OS include files!?
 */
enum clnt_stat callrpc(char *, u_long,
    u_long, u_long, xdrproc_t, void *, xdrproc_t, void *);

/*
 * XXX
 * Kernel functions.
 */
int nfs1lock(nlm_lockargs *, struct sockaddr *);
int nfs1shlock(nlm_shareargs *, struct sockaddr *);
int nfs1shunlock(nlm_shareargs *, struct sockaddr *);
int nfs1unlock(nlm_unlockargs *, struct sockaddr *);
int nfs1unlockall(char *);
int nfs4lock(nlm4_lockargs *, struct sockaddr *);
int nfs4shlock(nlm4_shareargs *, struct sockaddr *);
int nfs4shunlock(nlm4_shareargs *, struct sockaddr *);
int nfs4unlock(nlm4_unlockargs *, struct sockaddr *);
int nfslockd(pid_t, int);
