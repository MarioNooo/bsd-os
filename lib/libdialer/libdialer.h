/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI libdialer.h,v 1.9 1998/04/13 23:05:18 chrisk Exp
 */
#ifndef	__LIBDIALER_H__
#define	__LIBDIALER_H__

#define	_PATH_DIALER		"/var/run/dialer"
#define	_PATH_GETTYDLOG		"/var/log/gettyd.log"
#define	_PATH_PHONES		"/etc/phones"
#define	_PATH_RULESET		"/etc/dialer.rules"
#define	_PATH_TTYS_CONF		"/etc/ttys.conf"
#define	_PATH_TTYS_LOCAL	_PATH_TTYS_CONF ".local"

/*
 * Message types available on /var/run/dialer
 */
#define	RESOURCE_REQUEST	"request"	/* requests start with this */
#define	RESOURCE_RESET		"reset"		/* reset the daemon */
#define	RESOURCE_STATUS		"status"	/* request status from daemon */

/*
 * Resources which define what happens on this line
 */
#define	RESOURCE_MANAGER	"manager"	/* controlling resource */
#define	MANAGER_INIT		"init"		/* init manages this line */
#define	MANAGER_GETTYD		"gettyd"	/* gettyd manages this line */

/*
 * Entries associated with permissions/restrictions
 */
#define	DEF_GROUP		"nobody"	/* default group if unknown */
#define	DEF_USER		"nobody"	/* default user if unknown */
#define	RESOURCE_GROUP		"group-%s"	/* group restrictions */
#define	RESOURCE_GROUPS		"groups"	/* who can use this line */
#define	RESOURCE_REQUIRED	"required"	/* list of requried fields */
#define	RESOURCE_USER		"user-%s"	/* user restrictions */
#define	RESOURCE_USERS		"users"		/* who can use this line */

/*
 * Resources describing scripts
 * Only makes sense in /etc/ttys.conf database.
 * Only one of "condition" and "watcher" should be defined for any line.
 */
#define	RESOURCE_CONDITION	"condition"	/* condition line for dialin */
#define	RESOURCE_DIALER		"dialer"	/* dail a number */
#define	RESOURCE_HANGUP		"hangup"	/* condition line for dialout */
#define	RESOURCE_WATCHER	"watcher"	/* watch for incoming call */

/*
 * Various resources which have special meaning to gettyd(8)
 */
#define	RESOURCE_ALLOW		"allow"		/* what specials you can use */
#define	RESOURCE_ARGUMENTS	"arguments"	/* arguments sent to scripts */
#define	RESOURCE_CALLTYPE	"calltype"	/* type of call */
#define	RESOURCE_COST		"cost"		/* cost of line */
#define	RESOURCE_DESTINATION	"destination"	/* where you are trying to go */
#define	RESOURCE_DIALIN		"dialin"	/* this line can do dialin */
#define	RESOURCE_DIALOUT	"dialout"	/* this line can do dialout */
#define	RESOURCE_DCE		"dce-speeds"	/* DCE speeds of modem */
#define	RESOURCE_DTE		"dte-speeds"	/* DTE speeds of line */
#define	RESOURCE_LINE		"line"		/* Line to use */
#define	RESOURCE_MAP		"map"		/* how to map specials */
#define	RESOURCE_NUMBER		"number"	/* number to call */
#define	RESOURCE_RESTRICT	"restrict"	/* what specials you cant use */
#define	RESOURCE_SPEED		"speed"		/* modulation speed of line */
#define	RESOURCE_UUCP		"uucplocking"	/* need to do uucp locking */

/*
 * Other resources which have well known meanings
 */
#define	RESOURCE_COMMAND	"command"	/* the "getty" command */
#define	RESOURCE_TERM		"term"		/* the terminal type ($TERM) */
#define	RESOURCE_AUTH		"auth"		/* authentication mode */
#define	RESOURCE_WINDOW		"window"	/* the window manager command */
#define	RESOURCE_SECURE		"secure"	/* allow uid of 0 to login */

/*
 * Resources used by the reset command
 */
#define	RESOURCE_SHUTDOWN	"shutdown"	/* system is shutting down */
#define	RESOURCE_REREAD		"reread"	/* re-read /etc/ttys */
#define	RESOURCE_DIE		"die"		/* shut down now */
#define	RESOURCE_MULTIUSER	"multiuser"	/* allow logins */
#define	RESOURCE_VERBOSE	"verbose"	/* turn on verbose messages */
#define	RESOURCE_DEBUG		"debug"		/* turn on debug messages */

#define RESOURCE_STTYMODES	"stty-modes"	/* additional stty modes */
#define RESOURCE_FLOWCONTROL    "flowcontrol"	/* type of flow control */
#define RESOURCE_MODEMCONTROL   "modemcontrol"	/* enable modem control */

#define VALUE_FLOWCONTROL_V	"cts_oflow rts_iflow -ixon -ixoff"
#define VALUE_SWFLOWCONTROL_V	"-cts_oflow -rts_iflow ixon ixoff"
#define VALUE_NOFLOWCONTROL_V	"-cts_oflow -rts_iflow -ixon -ixoff"
#define VALUE_MODEMCONTROL_V	"-clocal"	/* modem control */
#define VALUE_NOMODEMCONTROL_V	"clocal"	/* no modem control */


#define	C_PIPE	0x0001		/* do special processing of | */
#define	C_SPACE	0x0002		/* do speical processing of spaces */

#define	CM_NORMAL	0	/* first entry found is used */
#define	CM_UNION	'+'	/* union same name entries */
#define	CM_INTERSECTION	'*'	/* intersect same name entries */
#define	CM_PERMISSIONS	'%'	/* do "permissions" style mergeing */

#define MAXFDS  64		/* Can pass up to 64 fds per message */

typedef struct {
	int		argc;		/* number of elements in list */
	int		max_argc;	/* number of elements allocated */
	char		**argv;		/* the argument list */
} argv_t;

typedef struct cap_t {
    	struct cap_t	*next;
	char		*name;		/* Name of capability */
	char		*value;		/* Value.  NULL -> not there */
	char		**values;	/* Value.  NULL -> not there */
} cap_t;

typedef struct {
	char	*ty_name;	/* terminal device name */
	cap_t	*ty_cap;	/* all the capabilities for the line */

	/*
	 * init needs ty_status, so we fill it in as a convienence for
	 * all our callers
	 */
#define	TD_IN		0x01	/* enable logins/dialin */
#define	TD_OUT		0x02	/* allow dialout */
#define	TD_INIT		0x04	/* have init control this line */
#define	TD_SECURE	0x80	/* secure line */
	int	ty_status;	/* status flags */

	/*
	 * the following are only used by init, and even there the
	 * use has been depricated.
	 */
	char	*ty_getty;	/* value of :command=...: field (for init) */
	char	*ty_window;	/* value of :window=...: field (for init) */
} ttydesc_t;

extern char _EMPTYVALUE[];		/* Value of BOOLEAN only capabilities */

#define	_EMPTYCAP	((cap_t *)_EMPTYVALUE)

ttydesc_t *getttydesc(void);
ttydesc_t *getttydescbyname(const char *);
int setttydesc(void);
int endttydesc(void);
cap_t *getttycap(char *);
char *getttycapbuf(char *);

cap_t *cap_make(char *, cap_t *);
cap_t *cap_look(cap_t *, char *);
char *cap_check(cap_t *, cap_t *);
int cap_add(cap_t **, char *, char *, int);
int cap_verify(cap_t *, cap_t *);
int cap_max(cap_t *);
void cap_free(cap_t *);
void cap_merge(cap_t **, cap_t *, int);
char *cap_dump(cap_t *, char *, int);
void cap_strrestore(char *, int);
char *cap_strencode(int, int);

argv_t *start_argv(char *, ...);
char *add_argv(argv_t *, char *);
char *add_argvn(argv_t *, char *, int);
void free_argv(argv_t *);

int recvfd(int);
int sendfd(int, int);
int recvfds(int, int *, int *);
int sendfds(int, int *, int);

int connect_daemon(cap_t *, uid_t, gid_t);
int connect_modem(int *, char **);
int dialer_status(char *);
int dialer_reset(char *);

#endif
