/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI daemon.h,v 1.7 1997/03/05 20:34:06 prb Exp
 */
#include <libdialer.h>

#define	_PATH_GETTYDLOG	"/var/log/gettyd.log"
#define	_PATH_GETTYDACT	"/var/account/gettyd"

#define	MAXCALLTYPESIZE		64		/* maximum call type length */

typedef struct {
	int		 fb_fd;
	char		*fb_buffer;
	char	    	*fb_tail;
	int		 fb_bufsize;
} fb_data_t;

int fb_init(fb_data_t *, int);
void fb_close(fb_data_t *);
int fb_read(fb_data_t *, char);

#define	COMMAND_COUNT	5		/* N commands cause repeat message */
#define	COMMAND_SPACE	10		/* minimum time for N commands */
#define	COMMAND_SLEEP	30.0		/* sleep N seconds when repeating */

struct linent {
	struct event	 *le_event;	/* Some event waiting, must be first */
	struct linent	 *le_next;
	struct linent	 *le_prev;
	char		 *le_name;	/* Name (device) of line */
	char		 *le_getty;	/* Getty entry from /etc/ttys */
	char		**le_getty_argv;
	int		  le_status;	/* Status from /etc/ttys */
	int		  le_oldstatus;	/* Previous Status */
	unsigned int	  le_state:12;	/* Internal state */
	int		  le_delete:1;	/* Line should be deleted */
	int		  le_uucp:1;	/* Need to support uucp locking */
	int		  le_uulocked:1;/* Holds a uucp locks */
	pid_t		  le_pid;	/* PID (if any) of running session */
	int		  le_mfd;
	fb_data_t	  le_rfb;	/* File descriptor with client */
	fb_data_t	  le_sfb;	/* Output from dialer/watcher */
#define	le_rfd		  le_rfb.fb_fd
#define	le_sfd		  le_sfb.fb_fd
    	int		  le_cost;	/* "Cost" of this line */
	int		  le_slot;
	struct fcred	  le_fcred;	/* Credentials of user using line */
	cap_t		 *le_cap;	/* List of capabilities. */
	cap_t		 *le_req;	/* List of requested capabilities. */
	time_t		  le_times[COMMAND_COUNT];
	char		 *le_number;	/* what number was really dialed */
	time_t		  le_start;	/* when the session started */
					/* time of last N commands issued */
};

#define	LE_AVAILABLE	0x800	/* This line is available */

#define	LE_UNINITIALIZED 0x000	/* Ground state */
#define	LE_SETUP	0x001	/* Setup state */
#define	LE_CONDITION	0x002	/* Conditioning state */
#define	LE_WATCH	0x803	/* Watching state (can dial out) */
#define	LE_WAIT		0x804	/* Wait for modem (can dial out) */
#define	LE_INCOMING	0x005	/* Incoming session active */
#define	LE_IDLE		0x806	/* Wait for dialout request (can dial out) */
#define	LE_WATCHDEATH	0x007	/* Waiting for watcher to die to dialout */
#define	LE_DIAL		0x008	/* Dialing out */
#define	LE_CONNECT	0x009	/* Outgoing connection made */
#define	LE_HANGUP	0x00a	/* hanging up modem */
#define	LE_HUNG		0x00b	/* Line is hung (process did not die) */
#define	LE_ENDING	0x00c	/* We are waiting to die */
#define	LE_KILLING	0x00d	/* We won't die, try killing ourself */
#define	LE_WAITIN	0x00e	/* Wait for modem (can't dial out) */
#define	LE_WATCHIN	0x00f	/* Watching state (can't dial out) */
#define	LE_UUCPLOCKED	0x010	/* Some external program has line locked */
#define	LE_SINGLEUSER	0x011	/* Line waiting for multi user mode */
#define	LE_REVOKEDELAY	0x012	/* Holding DTR down after a revoke */

/*
 * transto	- transition from some other state to the state X
 * transfrom	- transition to some other state from the state X
 * available	- if the line is available for dialout
 */
#define	TD_BIDIR	(TD_IN|TD_OUT)		/* short hand notation */
#define	transto(le,X)	((le->le_oldstatus & TD_BIDIR) != X && \
			 (le->le_status & TD_BIDIR) == X)
#define	transfrom(le,X)	((le->le_oldstatus & TD_BIDIR) == X && \
			 (le->le_status & TD_BIDIR) != X)
#define	status(le,X)	((le->le_status & TD_BIDIR) == X)
#define	available(le)	(le->le_state & LE_AVAILABLE)

struct request {
	struct event	*re_event;	/* Some event waiting, must be first */
	struct request	*re_next;
	fb_data_t	 re_fb;		/* socket to request */
	struct fcred	 re_fcred;
	cap_t		 *re_user;	/* Restrictions on this user */
	cap_t		*re_num;	/* the number(s) to dial */
	time_t		 re_timeout;	/* how long to wait for busy */
#define	re_socket	 re_fb.fb_fd
#define	re_data	 	 re_fb.fb_buffer
#define	re_tail	 	 re_fb.fb_tail
};

int debug;
int verbose;
int daemon_mode;
char *logfile;
char *actfile;

pid_t callscript(int, fd_set *, int, argv_t *);
void check_uucplock(struct linent *);
int checknumber(char *, cap_t *, cap_t *);
void clear_session(struct linent *);
char **construct_argv(char *, ...);
void del_request(struct request *);
void del_session(struct linent *);
int devopen(char *, int);
int devrevoke(char *, uid_t, gid_t, mode_t);
void devchall(char *, uid_t, gid_t, mode_t);
void dialcall(struct linent *);
void dprintf(char *, ...);
void end_session(struct linent *);
void execscript(argv_t *);
void failedfd(int);
void failedio();
cap_t *getent_session(struct linent *, char *name);
void info(char *, ...);
void infocall(char *, argv_t *);
void mapspecial(struct linent *, char *);
void new_session(struct linent *);
void new_session2(struct linent *);
char *nstrdup(char *);
void senderror(struct linent *, char *, ...);
int sendfd(int, int);
void sendmessage(struct linent *, char *, ...);
void setsig(int, void (*func)());
void stall(char *, ...);
char *strsplice(char **, char *);
void terminated();
void warning(char *, ...);
void watch_timeout(struct linent *);
