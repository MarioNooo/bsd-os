/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI statd.h,v 1.1 1998/03/19 20:20:23 don Exp
 */

extern int	d_cache;		/* Show cacheing behavior. */
extern int	d_calls;		/* Show requests/replies. */

extern DB *	stat_db;

extern FILE	*statfp;		/* Stable-storage status file. */
#define	_PATH_STATD_STATE	"/var/db/statd.state"
#define	_PATH_STATD_STATUS	"/var/db/statd.status"
#define _PATH_STATD_PID		_PATH_VARRUN "statd.pid"

/* Exported functions. */
CLIENT *get_client(char *, struct sockaddr_in *, int, int);
void	init_statfp(int);
int	our_state(int);
void	send_sm_notify(void);
