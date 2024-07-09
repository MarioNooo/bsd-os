/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI gettyd.c,v 1.30 2001/12/01 00:44:06 chrisk Exp
 */
#include <sys/ioctl.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/tty.h>
#include <sys/types.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>
#include <sys/resource.h>

#include "daemon.h"
#include "phone.h"

extern int ttysetdev(char *, uid_t, gid_t);
extern int logout(char *);
extern void logwtmp(char *, char *, char *);
int recvcred(int, struct fcred *);
void process(void);
void reap(void);
void match_request(struct request *re);
fd_set *mk_fd_set(void);
void free_fd_set(fd_set *);
int uucp_uid = 0;
int uucp_gid = 0;

#define	MODEM_MODE	(O_RDWR|O_NONBLOCK)
#define	MAXTIMEO	(60 * 60 * 24)	/* One day, for testing */
#define	ALRMOUT		5
#define	MAXSPEED	INT_MAX		/* "infinite" baud rate */

#define	DEFMODE		(S_IRUSR | S_IWUSR)
#define	UUCPMODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

typedef struct {
	FILE	*fp;
	char	*original;
	char	string[256];
} expandnumber_t;

/* #define	real_noisy_debug */

static char *expandnumber(expandnumber_t *);
static int became_available = 0;

char *states[] = {
	"UNINITIALIZED",
	"SETUP",
	"CONDITION",
	"WATCH",
	"WAIT",
	"INCOMING",
	"IDLE",
	"WATCHDEATH",
	"DIAL",
	"CONNECT",
	"HANGUP",
	"HUNG",
	"ENDING",
	"KILLING",
	"WAITIN",
	"WATCHIN",
	"UUCPLOCKED",
	"SINGLEUSER",
	"REVOKEDELAY"
};
#define	Nstates	(sizeof(states)/sizeof(states[0]))

int uu_lock(char *);
int uu_unlock(char *);

void *fd_set_list = NULL;

char *statename(int s)
{
	static char buf[32];

	s &= ~LE_AVAILABLE;

	if (s >= 0 && s < Nstates)
		return(states[s]);
	sprintf(buf, "unknown state %x", s);
	return(buf);
}

#define	EXITSTATUS(s)	(WIFEXITED(s) ? WEXITSTATUS(s) : -1)

void
set_state(struct linent *le, int s)
{
	switch (le->le_state) {
	case LE_INCOMING:	/* Done with an incomming call */
		info("%s:state=INCOMING:time=%ld:duration=%ld:",
		    le->le_name, le->le_start, time(NULL) - le->le_start);
		break;
	case LE_CONNECT:	/* Done with an outgoing call */
		info("%s:state=OUTGOING:time=%ld:duration=%ld:login=%.*s:ruid=%d:uid=%d%s%s:",
		    le->le_name, le->le_start, time(NULL) - le->le_start,
		    sizeof(le->le_fcred.fc_login),
		    le->le_fcred.fc_login,
		    le->le_fcred.fc_ruid,
		    le->le_fcred.fc_uid,
		    le->le_number ? ":number=" : "",
		    le->le_number ? le->le_number : "");
		break;
	default:
		break;
	}
	/*
	 * terminate the utmp entry for the line if we are switching from
	 * a state that might have it set to a state that should not have
	 * it set.
	 */
	switch (le->le_state) {
	case LE_INCOMING:
		if (s != le->le_state) {
			/*
			 * We need to reset ownership of things after
			 * the user logs out.
			 */
			ttysetdev(le->le_name, 0, 0);
		}
		/** FALL THRU **/
	case LE_CONNECT:
	case LE_DIAL:
	case LE_WATCH:
		switch(s) {
		case LE_INCOMING:
		case LE_CONNECT:
		case LE_DIAL:
			break;
		default:
			if (logout(le->le_name))
				logwtmp(le->le_name, "", "");
			break;
		}
	default:
		break;
	}

	/*
	 * flag if any lines become available
	 */
	if ((s & LE_AVAILABLE) && (le->le_state & LE_AVAILABLE) == 0)
		became_available = 1;

	if (le->le_state != s)
		dprintf("%s: going to state %s", le->le_name, statename(s));
	le->le_state = s;
}

char *dialerconf[] = { _PATH_TTYS_CONF, _PATH_TTYS_LOCAL, 0 };

static struct linent *le_root = 0;
static struct request *re_root = 0;
static int callind = 0;
static int on_the_way_down = 0;
static int single_user = 0;
static int limit_select = 0;
int numberfds;
static int soket;
sigset_t blockedsigs;
fd_set *read_set;
fd_set *write_set;
fd_set *close_set;
int debug = 0;
int daemon_mode = 1;
int verbose = 0;
char *logfile = _PATH_GETTYDLOG;
char *actfile = _PATH_GETTYDACT;

static void (*callist[8])();

struct event {
	void		(*func)(void *);
	struct event	**who;
	struct timeval	  when;
	struct event	 *next;
};

struct event *events;

#define	WRITES(fd,str)	write(fd, str, sizeof(str) - 1)

void
close_modem(struct linent *le)
{
	if (le->le_mfd >= 0 ) {
		close(le->le_mfd);
		FD_CLR(le->le_mfd, read_set);
		FD_CLR(le->le_mfd, write_set);
		FD_SET(le->le_mfd, close_set);
		le->le_mfd = -1;
	}
}

void
addhandler(void (*func)())
{
	if (callind < sizeof(callist) / sizeof(callist[0]))
		callist[callind++] = func;
	alarm(ALRMOUT);
}

void
callhandlers()
{
	while (callind)
		(*callist[--callind])();
}

struct event *
schedule_event(void *lre, void (*func)(), double dwhen)
{
	struct event *nev = malloc(sizeof(struct event));
	struct event *ev;
	struct timeval when;

	if (nev == 0) {
		warning("Out of memory trying to allocate event structure.");
		return(nev);
	}

	gettimeofday(&when, 0);
	when.tv_sec += (time_t)dwhen;
	when.tv_usec += (time_t)((dwhen - (int)dwhen) * 1000000);
	if (when.tv_usec >= 1000000) {
		when.tv_usec -= 1000000;
		when.tv_sec += 1;
	}

	nev->func = func;
	if ((nev->who = (struct event **)lre) != NULL)
		*(struct event **)lre = nev;

	nev->when = when;

	if (!events || timercmp(&(events->when), &when, >)) {
		nev->next = events;
		events = nev;
		return(nev);
	}

	ev = events;

	while (ev->next && timercmp(&(ev->next->when), &when, <=))
		ev = ev->next;

	nev->next = ev->next;
	ev->next = nev;
	return(nev);
}

void
delete_event(struct event *ev)
{
	struct event *e;

	if (ev == NULL)
		return;
	if (ev->who && *(ev->who) == ev)
		*(ev->who) = 0;

	if (ev == events)
		events = ev->next;
	else {
		for (e = events; e && e->next != ev; e = e->next)
			;

		if (e->next == ev)
			e->next = ev->next;
	}
	free(ev);
}

void
call_events(struct timeval now)
{
	struct event *ev;

	while ((ev = events) && timercmp(&(ev->when), &now, <=)) {
		events = ev->next;
		if (ev->who && *(ev->who) == ev)
			*(ev->who) = 0;
		ev->func(ev->who);
		free(ev);
	}
}

struct linent *
getlinent(char *name)
{
	struct linent *le;

	for (le = le_root; le; le = le->le_next) {
		if (strcmp(le->le_name, name) == 0)
			break;
	}
	return (le);
}

struct linent *
getlinentbypid(pid_t pid)
{
	struct linent *le;

	for (le = le_root; le; le = le->le_next) {
		if (le->le_pid == pid)
			break;
	}
	return (le);
}

int
addlinent(ttydesc_t *te, int slot)
{
	struct linent *le;

	if ((le = getlinent(te->ty_name)) != NULL) {
		if (te->ty_cap) {
			if (le->le_cap && le->le_cap != te->ty_cap)
				cap_free(le->le_cap);
			le->le_cap = te->ty_cap;
			te->ty_cap = NULL;
		}

		le->le_delete = 0;
		if (le->le_slot != slot) {
			warning("%s: changed from slot %d to %d\n",
			    le->le_name, le->le_slot, slot);
			le->le_slot = slot;
		}
		if (te->ty_getty == NULL ||
		    strcmp(te->ty_getty, le->le_getty) != 0) {
			char *t = nstrdup(te->ty_getty);
			if (t) {
				free(le->le_getty);
				le->le_getty = t;
			} else {
				del_session(le);
				return (-1);
			}
			if (le->le_getty_argv)
				free(le->le_getty_argv);
			le->le_getty_argv = construct_argv(le->le_getty, 0);
		}
		le->le_oldstatus = le->le_status;
		le->le_status = te->ty_status;
		return (0);
	}

	if ((le = malloc(sizeof(struct linent))) == NULL)
		return (-1);
	memset(le, 0, sizeof(struct linent));

	le->le_status = te->ty_status;
	le->le_slot = slot;
	set_state(le, LE_UNINITIALIZED);
	le->le_mfd = -1;
	le->le_rfd = -1;
	le->le_sfd = -1;

	if ((le->le_name = nstrdup(te->ty_name)) == NULL) {
		del_session(le);
		return (-1);
	}
	if ((le->le_getty = nstrdup(te->ty_getty)) == NULL) {
		del_session(le);
		return (-1);
	}

	le->le_getty_argv = construct_argv(le->le_getty, 0);

	if (le->le_getty_argv == 0) {
		del_session(le);
		return (-1);
	}

	le->le_cap = te->ty_cap;
	te->ty_cap = NULL;

	if (le_root == 0) {
		le->le_prev = le;
		le_root = le;
	} else {
		le->le_prev = le_root->le_prev;
		le->le_prev->le_next = le;
		le_root->le_prev = le;
	}
	return (0);
}

void
new_session(struct linent *le)
{
	int s;
	cap_t *ct;

	if (on_the_way_down) {
		warning("%s: trying to start up again", le->le_name);
		del_session(le);
		return;
	}

	switch (le->le_state) {
	case LE_UNINITIALIZED:
		break;
	case LE_SINGLEUSER:
	case LE_UUCPLOCKED:
		end_session(le);
		break;
	default:
#ifdef	clean_up_after_flow_errors_but_we_should_not_have_to
		end_session(le);
#endif
		break;
	}

	/*
	 * We can only start a new session if we are in the unitialized
	 * state.  It is common to try and start a session for the ending
	 * state, so don't complain about it.
	 */
	switch (le->le_state) {
	case LE_UNINITIALIZED:
		break;
	case LE_ENDING:
		return;
	default:
		warning("%s: trying to restart from state %s",
			le->le_name, statename(le->le_state));
		return;
	}

	/*
	 * It is possible we have an outstanding event to get us here.
	 * We don't want to get here twice, so delete any outstanding events.
	 */
	delete_event(le->le_event);

	/*
	 * Check to make sure we are not repeating too quickly.
	 * If we get here COMMAND_COUNT times in COMMAND_SPACE seconds
	 * then we sleep for COMMAND_SLEEP seconds.
	 */
	for (s = 1; s < COMMAND_COUNT; ++s)
		le->le_times[s-1] = le->le_times[s];
	le->le_times[COMMAND_COUNT-1] = time(NULL);

	if (le->le_times[COMMAND_COUNT-1] - le->le_times[0] < COMMAND_SPACE) {
		warning("%s: sessions repeating too quickly", le->le_name);
		schedule_event(le, new_session, COMMAND_SLEEP);
		return;
	}

	if (le->le_cap == NULL) {
		ttydesc_t *ty = getttydescbyname(le->le_name);
		if (ty) {
			le->le_cap = ty->ty_cap;
			ty->ty_cap = NULL;
		}
	}

	if ((ct = getent_session(le, RESOURCE_MANAGER)) == NULL ||
	    strcmp(ct->value, MANAGER_GETTYD) != 0)
		return;

	if ((le->le_status & (TD_OUT|TD_IN)) == 0)
		return;
		
	le->le_uulocked = 0;

	if ((ct = getent_session(le, RESOURCE_UUCP)) &&
	    strcasecmp(ct->value, "off") != 0)
		le->le_uucp = 1;
	else
		le->le_uucp = 0;


	if (single_user && (le->le_status & TD_OUT) == 0) {
		/*
		 * Do not revoke a dialin only line while in single
		 * user mode.  This is mainly to make sure we do not
		 * revoke the console from /etc/rc.  The line will get
		 * revoked later when we come through here again after
		 * going to multi user mode.
		 */
		set_state(le, LE_SETUP);
		new_session2(le);
		return;
	}

	if (le->le_uucp && (le->le_status & TD_OUT)) {
		devchall(le->le_name, uucp_uid, uucp_gid, UUCPMODE);
		if (uu_lock(le->le_name) == 0) {
			devrevoke(le->le_name, uucp_uid, uucp_gid, UUCPMODE);
			uu_unlock(le->le_name);
		} else {
			set_state(le, LE_UUCPLOCKED);
			schedule_event(le, check_uucplock, 5.0);
			return;
		}
	} else
		devrevoke(le->le_name, 0, 0, DEFMODE);

	set_state(le, LE_REVOKEDELAY);
	schedule_event(le, new_session2, 1.0);	/* hold DTR down 1 second */
}

void
new_session2(struct linent *le)
{
	argv_t *avt;
	cap_t *wt, *ct, *cp, *lp;
	char **pv, **vv;
	int sv[2];
	char buf[sizeof(_PATH_DEV "fd/xxxxxxxx")];
	char name[256];
	fd_set *passfds;

	if (le->le_state == LE_REVOKEDELAY)
		set_state(le, LE_SETUP);

	if (le->le_state != LE_SETUP) {
		warning("%s: entered new_session2 in state %s",
			le->le_name, statename(le->le_state));
		return;
	}


	if ((ct = getent_session(le, RESOURCE_COST)) != NULL)
		le->le_cost = cap_max(ct);
	else
		le->le_cost = 0;

	ct = wt = 0;

	if (le->le_status & TD_IN) {
		wt = getent_session(le, RESOURCE_WATCHER);
		ct = getent_session(le, RESOURCE_CONDITION);
	} else
		ct = getent_session(le, RESOURCE_HANGUP);

	if (wt && ct) {
		warning("%s: has both watcher and condition set", le->le_name);
		ct = 0;
	}

	if ((le->le_status & TD_IN) == 0 && ct == NULL) {
		set_state(le, LE_IDLE);
		return;
	}

	if (single_user && ((le->le_status & TD_OUT) == 0 || wt != NULL)) {
		set_state(le, LE_SINGLEUSER);
		return;
	}

	if ((le->le_mfd = devopen(le->le_name, MODEM_MODE)) >= 0)
		close(le->le_mfd);
	le->le_mfd = devopen(le->le_name, MODEM_MODE);

	if (le->le_mfd < 0) {
		warning("%s: cannot open: %s",
			le->le_name, strerror(errno));
		cap_free(le->le_cap);
		le->le_cap = NULL;
		set_state(le, LE_UNINITIALIZED);
		return;
	}

	if (ct || wt) {
		if (!ct)
			ct = wt;
		sprintf(buf, _PATH_DEV "fd/%d", le->le_mfd);
		avt = start_argv(ct->values[0], "-f", buf, 0);
		if (le->le_status & TD_IN)
			add_argv(avt, "-" RESOURCE_DIALIN);
		if (le->le_status & TD_OUT)
			add_argv(avt, "-" RESOURCE_DIALOUT);

		if ((cp = getent_session(le, RESOURCE_ARGUMENTS)) != NULL)
			for (pv = cp->values; *pv; ++pv) {
				snprintf(name, sizeof(name), "-%s", *pv);
				if (strcmp(*pv, RESOURCE_LINE) == 0) {
					add_argv(avt, name);
					add_argv(avt, le->le_name);
#ifdef dont_make_the_user_specify_that_the_command_should_be_sent
				} else if (strcmp(*pv, RESOURCE_COMMAND) == 0) {
					add_argv(avt, name);
					add_argv(avt, le->le_getty);
#endif
				} else if ((lp = getent_session(le, *pv)) &&
				    lp->value) {
					add_argv(avt, name);
					for (vv = lp->values; *vv; ++vv)
						add_argv(avt, *vv);
				}
			}
		if (!avt) {
			warning("%s: cannot create arglist: %s",
				le->le_name, strerror(errno));
			close_modem(le);
			cap_free(le->le_cap);
			le->le_cap = NULL;
			set_state(le, LE_UNINITIALIZED);
			return;
		}
		if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
			free_argv(avt);
			warning("%s: cannot create socket pair: %s",
				le->le_name, strerror(errno));
			close_modem(le);
			cap_free(le->le_cap);
			le->le_cap = NULL;
			set_state(le, LE_UNINITIALIZED);
			return;
		}
		if (fcntl(sv[0], F_SETFL, O_NONBLOCK) < 0) {
			close(sv[0]);
			close(sv[1]);
			free_argv(avt);
			warning("%s: cannot set socket pair nonblocking: %s",
				le->le_name, strerror(errno));
			close_modem(le);
			cap_free(le->le_cap);
			le->le_cap = NULL;
			set_state(le, LE_UNINITIALIZED);
			return;
		}

		fb_init(&le->le_sfb, sv[0]);

		passfds = mk_fd_set();
		FD_SET(le->le_mfd, passfds);

		infocall(le->le_name, avt);
		le->le_pid = callscript(sv[1], passfds, wt ? le->le_mfd : -1, avt); 
		free_fd_set(passfds);
		free_argv(avt);
		close(sv[1]);
		if (le->le_pid <= 0) {
			clear_session(le);
			schedule_event(le, new_session, 5.0);
		} else {
			if (status(le, TD_BIDIR))
				set_state(le, wt ? LE_WATCH : LE_CONDITION);
			else if (status(le, TD_IN))
				set_state(le, wt ? LE_WATCHIN : LE_CONDITION);
			else
				set_state(le, LE_HANGUP);
			FD_SET(le->le_sfd, read_set);
		}
	} else {
		le->le_pid = 0;
		FD_SET(le->le_mfd, write_set);
		if (status(le, TD_IN))
			set_state(le, LE_WAITIN);
		else
			set_state(le, LE_WAIT);
	}
	if (le->le_uucp)
		devchall(le->le_name, uucp_uid, uucp_gid, UUCPMODE);
}

cap_t *
getent_session(struct linent *le, char *name)
{
	cap_t *c = le->le_cap;

	while (c) {
		if (strcmp(c->name, name) == 0)
			return (c->value ? c : 0);
		c = c->next;
	}
	return (0);
}

struct linent *
findmatch(cap_t *c, int *busy, int super_user)
{
	struct linent *use = NULL;	/* line we expect to use */
	struct linent *le;
	int maxspeed = 0, ms;
	char *name = NULL;
	char **av;
	cap_t *tc;
	cap_t *v;
	cap_t *sc;	/* speed entry of request */
	cap_t *lc;	/* line entry of request */

	for (sc = c; sc; sc = sc->next)
		if (strcmp(sc->name, RESOURCE_DTE) == 0)
			break;

	for (lc = c; lc; lc = lc->next)
		if (strcmp(lc->name, RESOURCE_LINE) == 0) {
			/*
			 * strip off any preceeding /dev/ and
			 * make sure the result is not the empty string
			 */
			name = lc->values[0];
			if (strncmp(name, _PATH_DEV, sizeof(_PATH_DEV)-1) == 0)
				name += 5;
			if (*name == '\0')
				name = NULL;
			break;
		}

	for (le = le_root; le; le = le->le_next) {
		/*
		 * Don't even look at non-dialout lines
		 */
		if ((le->le_status & TD_OUT) == NULL)
			continue;

		/*
		 * If we are doing uucp locking on the line then
		 * check to see if this line is already locked.
		 * The uu_lock() function returns success if we
		 * already have the lock so check if we think
		 * we have it first.
		 */
		if (le->le_uucp) {
			if (le->le_uulocked || uu_lock(le->le_name) < 0) {
				dprintf("%s: UUCP locked", le->le_name);
				continue;
			} else
				le->le_uulocked = 1;
		}

		/*
		 * If we have asked for a specific line, only match that line!
		 */
		if (name && (strcmp(le->le_name, name) != 0)) {
#ifdef	real_noisy_debug
			dprintf("%s: does not match ``%s''", le->le_name, name);
#endif
next_entry:
			if (le->le_uulocked) {
				uu_unlock(le->le_name);
				le->le_uulocked = 0;
			}
			continue;
		}

		/*
		 * If we have already found a potential line, we need to
		 * compare it to the current line.  We always select a line
		 * of lower cost.  If two lines have the same cost, we either
		 * select the fastest line (if the user did not specify a
		 * speed), or we select the slowest line that is at least as
		 * fast as the user asked for.  If the all things are equal,
		 * we take the first one found.
		 */
		if (use) {
			if (use->le_cost < le->le_cost) {
				dprintf("%s: is more expensive than %s",
				    le->le_name, use->le_name);
				goto next_entry;
			}
			if (use->le_cost == le->le_cost) {
				v = getent_session(le, RESOURCE_DTE);
				if (v != NULL) {
					if (sc && !cap_check(sc, v)) {
						dprintf("%s: does not support "
						    "reqeusted speed(s)",
						    le->le_name);
						goto next_entry;
					}
					if ((ms = cap_max(v)) == 0)
						ms = MAXSPEED;
				} else if (sc) {
					dprintf("%s: does not support "
					    "reqeusted speed(s)", le->le_name);
					goto next_entry;
				} else
					ms = 0;
				if (sc && ms >= maxspeed) {
					dprintf("%s: no better speed match "
					    "than %s",
					    le->le_name, use->le_name);
					goto next_entry;
				}
				if (sc == NULL && ms <= maxspeed) {
					dprintf("%s: no faster speed than %s",
					    le->le_name, use->le_name);
					goto next_entry;
				}
			}
		}

		/*
		 * go through each of the entries specifed by the user
		 * and make sure they are satisfied.
		 */
		for (tc = c; tc; tc = tc->next) {
			if (tc == lc)
				continue;
			if ((v = getent_session(le, tc->name)) == NULL) {
				/*
				 * If a line does not specify types of calls
				 * it can make then all types are allowed
				 * likewise with phone numbers
				 * and with user names...
				 * and with groups...
				 */
				if (strcmp(tc->name, RESOURCE_CALLTYPE) == 0 ||
#ifdef	DONT_RESTRICT_ON_NUMBER
				    strcmp(tc->name, RESOURCE_NUMBER) == 0 ||
#endif
				    strcmp(tc->name, RESOURCE_USERS) == 0 ||
				    strcmp(tc->name, RESOURCE_GROUPS) == 0)
					continue;
				break;
			}
			/*
			 * Both numbers and calltypes require that
			 * if specified on the line, *all* the values
			 * specified by the user must be supported
			 * by the line
			 */
			if (strcmp(tc->name, RESOURCE_CALLTYPE) == 0 ||
			    strcmp(tc->name, RESOURCE_NUMBER) == 0) {
				if (!cap_verify(tc, v))
					break;
			} else if (super_user &&
			    (strcmp(tc->name, RESOURCE_USERS) == 0 ||
			     strcmp(tc->name, RESOURCE_GROUPS) == 0))
				/* super-user need not check user/group */;
			else if (!cap_check(tc, v))
				break;
		}
		if (tc) {
			/*
			 * Don't bother to tell us about lines that
			 * don't match because of the phone number.
			 */
			if (strcmp(tc->name, RESOURCE_NUMBER))
				dprintf("%s: failed on %s", le->le_name,
				    tc->name);
			goto next_entry;
		}

		if ((v = getent_session(le, RESOURCE_REQUIRED)) != NULL) {
			for (av = v->values; *av; ++av) {
				for (tc = c; tc; tc = tc->next)
					if (strcmp(tc->name, *av) == 0)
						break;
				if (tc == 0)
					break;
			}
			if (*av) {
				dprintf("%s: requires %s", le->le_name, *av);
				goto next_entry;
			}
		}

		if (!available(le)) {
#ifdef	real_noisy_debug
			dprintf("%s: not available", le->le_name);
#endif
			++*busy;
			goto next_entry;
		}

		if (use && use->le_uulocked) {
			uu_unlock(use->le_name);
			use->le_uulocked = 0;
		}
		dprintf("%s: possible line", le->le_name);
		use = le;

		/*
		 * a :dte-speeds: with no value supports infinite speeds
		 */
		if ((v = getent_session(use, RESOURCE_DTE)) != NULL) {
			if ((maxspeed = cap_max(v)) <= 0)
				maxspeed = MAXSPEED;
		} else
			maxspeed = 0;
	}
	if (use)
		use->le_req = c;
	
	return (use);
}

void
hung_session(struct linent *le)
{
	set_state(le, LE_HUNG);
}

void
kill_session(struct linent *le)
{
	if (le->le_pid && kill(le->le_pid, 0) == 0) {
		killpg(le->le_pid, SIGKILL);
		kill(le->le_pid, SIGKILL);
		set_state(le, LE_KILLING);
		schedule_event(le, hung_session, 30.0);
	} else {
		reap();
		if (le->le_pid) {
			warning("%s: Process ID %d has vanished!",
			    le->le_name, le->le_pid);
			le->le_pid = 0;
			clear_session(le);
			new_session(le);
		}
	}
}

void
end_session(struct linent *le)
{
	if (le->le_pid) {
		if (kill(le->le_pid, 0) == 0) {
			killpg(le->le_pid, SIGTERM);
			kill(le->le_pid, SIGTERM);
			set_state(le, LE_ENDING);
			schedule_event(le, kill_session, 30.0);
			return;
		} else {
			warning("%s: Process ID %d has vanished!",
			    le->le_name, le->le_pid);
			le->le_pid = 0;
		}
	}
	clear_session(le);
}

/*
 * Check to see if the uucp lock for this line is still being held.
 * If so, set up to check again in 5 seconds.
 */
void
check_uucplock(struct linent *le)
{
	if (uu_lock(le->le_name) < 0) {
		schedule_event(le, check_uucplock, 5.0);
		return;
	}
	uu_unlock(le->le_name);
	clear_session(le);
	new_session(le);
}

void
clear_session(struct linent *le)
{
	if (le->le_uulocked) {
		uu_unlock(le->le_name);
		le->le_uulocked = 0;
	}

	fb_close(&le->le_rfb);
	fb_close(&le->le_sfb);
	close_modem(le);
	if (le->le_state != LE_UNINITIALIZED)
		devrevoke(le->le_name, 0, 0, DEFMODE);

	cap_free(le->le_cap);
	cap_free(le->le_req);
	delete_event(le->le_event);

	le->le_cap = NULL;
	le->le_req = NULL;
	le->le_event = NULL;
	set_state(le, LE_UNINITIALIZED);
	memset(&le->le_fcred, 0, sizeof(le->le_fcred));

	if (le->le_delete) {
		del_session(le);
		return;
	}
	le->le_start = 0;
	if (le->le_number) {
		free(le->le_number);
		le->le_number = NULL;
	}
}

void
terminate()
{
	struct linent *le;
	struct linent *next;
	sigset_t ss;

	if (on_the_way_down)
		return;

	dprintf("shutting down");
	setsig(SIGHUP, SIG_IGN);
	setsig(SIGTERM, SIG_DFL);

	(void) sigemptyset(&ss);
	(void) sigaddset(&blockedsigs, SIGTERM);
	(void) sigdelset(&blockedsigs, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &ss, 0);

	on_the_way_down = 1;

	for (le = le_root; le; le = next) {
		next = le->le_next;
		le->le_delete = 1;
		end_session(le);
	}

	alarm(10);
}

void
del_session(struct linent *le)
{
	le->le_state = 0;

	if (le->le_state != LE_UNINITIALIZED)
		end_session(le);

	if (le == le_root) {
		if ((le_root = le->le_next) != NULL)
			le->le_next->le_prev = le->le_prev;
	} else if (le->le_prev) {
		if ((le->le_prev->le_next = le->le_next) != NULL)
			le->le_next->le_prev = le->le_prev;
		if (le_root->le_prev == le)
			le_root->le_prev = le->le_prev;
	}

	if (le->le_name)
		free(le->le_name);
	if (le->le_getty)
		free(le->le_getty);
	if (le->le_getty_argv)
		free(le->le_getty_argv);
	free(le);
}

void
reap()
{
	pid_t pid;
	struct linent *le;
	int rstatus;
	double timo = 1.0;

	while ((pid = waitpid(-1, &rstatus, WNOHANG)) > 0 ||
	    (pid == -1 && errno == EINTR)) {
		if ((le = getlinentbypid(pid)) != NULL) {
			/* XXX - should look at exit status here */

			le->le_pid = 0;
			if (le->le_delete) {
				del_session(le);
				return;
			}
			switch (le->le_state) {
			case LE_CONDITION:
				fb_close(&le->le_sfb);
				/*
				 * Flush the raw input queue on the device
				 * The condition script may not have read
				 * all the data from the modem.
				 */
				rstatus = FREAD;
				ioctl(le->le_mfd, TIOCFLUSH, &rstatus);
				FD_SET(le->le_mfd, write_set);
				if (status(le, TD_IN))
					set_state(le, LE_WAITIN);
				else
					set_state(le, LE_WAIT);
				break;
			case LE_WATCH:
			case LE_WATCHIN:
				clear_session(le);
				if (EXITSTATUS(rstatus))
				    warning("%s: watcher died with status %d",
					le->le_name, EXITSTATUS(rstatus));
				schedule_event(le, new_session, timo);
				break;
			case LE_WATCHDEATH:
				sendmessage(le, "line cleared.");
				delete_event(le->le_event);
				dialcall(le);
				break;
			case LE_INCOMING:
				clear_session(le);
				schedule_event(le, new_session, timo);
				break;
			case LE_ENDING:
			case LE_KILLING:
			case LE_HUNG:
				clear_session(le);
				new_session(le);
				break;
			case LE_HANGUP:
				fb_close(&le->le_sfb);
				close_modem(le);
				set_state(le, LE_IDLE);
				break;
			case LE_DIAL:
				fb_close(&le->le_sfb);
				if (EXITSTATUS(rstatus)) {
					senderror(le, "dialer exited with %d",
					    EXITSTATUS(rstatus));
					clear_session(le);
					new_session(le);
					break;
				}
				WRITES(le->le_rfd, "fd\n");
				if (fcntl(le->le_mfd, F_SETFL, 0) < 0)
					warning("setting non-block for %s: %s",
						le->le_name, strerror(errno));
				sendfd(le->le_rfd, le->le_mfd);
				close_modem(le);
				set_state(le, LE_CONNECT);
				le->le_start = time(NULL);
				info("%s:state=OUTGOING:time=%ld:login=%.*s:ruid=%d:uid=%d%s%s:",
				    le->le_name, le->le_start,
				    sizeof(le->le_fcred.fc_login),
				    le->le_fcred.fc_login,
				    le->le_fcred.fc_ruid,
				    le->le_fcred.fc_uid,
				    le->le_number ? ":number=" : "",
				    le->le_number ? le->le_number : "");
				break;
			default:
				warning("Reaped child of state %s",
				    statename(le->le_state));
			}
		}
	}
}

void
readlinents()
{
	ttydesc_t *te;
	struct linent *le, *nle;
	int slot = 0;

	/*
	 * First mark all the entires as deleted.
	 * The addlinent function will clear this bit when
	 * it finds the entry.
	 */

	for (le = le_root; le; le = le->le_next)
		le->le_delete = 1;

	setttydesc();
	while ((te = getttydesc()) != NULL)
		addlinent(te, ++slot);
	endttydesc();

	for (le = le_root; le; le = nle) {
		nle = le->le_next;
		if (le->le_delete) {
			del_session(le);
			continue;
		}
		/*
		 * The only situation we are interested in is when
		 * we either change to one of our states (dialout or bidir)
		 * or we change away from one of our states.
		 */
		if (transto(le, TD_OUT))
			switch (le->le_state) {
			case LE_UNINITIALIZED:
				new_session(le);
				break;
			case LE_DIAL:
			case LE_CONNECT:
				break;
			default:
				end_session(le);
				new_session(le);
				break;
			}
		else if (transto(le, TD_BIDIR))
			switch (le->le_state) {
			case LE_UNINITIALIZED:
				new_session(le);
				break;
			case LE_IDLE:
			case LE_HANGUP:
			case LE_WAITIN:
			case LE_WATCHIN:
				end_session(le);
				new_session(le);
				break;
			default:
				break;
			}
		else if (transto(le, TD_IN))
			switch (le->le_state) {
			case LE_UNINITIALIZED:
				new_session(le);
				break;
			case LE_IDLE:
			case LE_HANGUP:
			case LE_DIAL:
			case LE_WAIT:
			case LE_WATCH:
				end_session(le);
				new_session(le);
				break;
			default:
				break;
			}
		else if (transfrom(le,TD_OUT) || transfrom(le,TD_BIDIR)
		   || transfrom(le,TD_IN)) {
			devrevoke(le->le_name, 0, 0, DEFMODE);
			end_session(le);
		} else if ((le->le_state & ~LE_AVAILABLE) == LE_SINGLEUSER &&
		    single_user == 0)
			new_session(le);
	}
}

void
failedio()
{
	int i;
	struct timeval tv;
	fd_set *rs, *ws;

	rs = mk_fd_set();
	ws = mk_fd_set();
	for (i = 0; i < numberfds; ++i) {
		FD_NZERO(numberfds, rs);
		FD_NZERO(numberfds, ws);
		if (FD_ISSET(i, read_set))
			FD_SET(i, rs);
		else if (FD_ISSET(i, write_set))
			FD_SET(i, ws);
		else
			continue;
		errno = 0;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		if (select(numberfds, rs, ws, 0, &tv) < 0 && errno == EBADF)
			failedfd(i);
	}
	free_fd_set(rs);
	free_fd_set(ws);
}

void
failedfd(int fd)
{
	struct linent *le;
	struct request *re;

	FD_CLR(fd, read_set);
	FD_CLR(fd, write_set);
	FD_SET(fd, close_set);

	if (fd == soket) {
		warning("Failure on control socket.");
		exit(1);
	}
	for (le = le_root; le; le = le->le_next)
		if (le->le_rfd == fd || le->le_sfd == fd
				     || le->le_mfd == fd)
			break;
	if (le) {
		close_modem(le);
		fb_close(&le->le_sfb);
		fb_close(&le->le_rfb);

		end_session(le);
		new_session(le);
		return;
	}

	for (re = re_root; re; re = re->re_next)
		if (re->re_socket == fd)
			break;
	if (re) {
		del_request(re);
		return;
	}
}

void
dochild()
{
#ifdef	reap_children_only_on_signal
	addhandler(reap);
#endif
}

void
dohup()
{
	addhandler(readlinents);
}

void
dopipe()
{
	addhandler(failedio);
}

void
doterm()
{
	addhandler(terminate);
}

void
doalrm()
{
	if (on_the_way_down)
		exit(1);
	/* nothing to do */
}

void
del_request(struct request *re)
{
	struct request *lre;

	delete_event(re->re_event);
	cap_free(re->re_num);
	fb_close(&re->re_fb);

	if (re == re_root)
		re_root = re->re_next;
	else {
		for (lre = re_root; lre; lre = lre->re_next)
			if (lre->re_next == re)
				break;
		if (lre)
			lre->re_next = re->re_next;
	}
	cap_free(re->re_user);
	free(re);
}

/*
 * process the request for status from the user
 */
void
do_status(struct request *re)
{
	cap_t *c, *cp;
	int doall = 0;		/* display *all* lines */
	int check = 0;		/* user specified which lines */
	int mdms = 1;		/* display modem control status */
	struct linent *le;
	struct termios tio;
	int i, fd, state;
	char buf[1024];
	char nbuf[MAXPATHLEN];
	char *comma;

	if ((c = cp = cap_make(re->re_data, NULL)) != NULL) {
		while (c) {
			if (strcmp(c->name, "doall") == 0)
				doall = c->value ? 1 : 0;
			else if (strcmp(c->name, "mdmctl") == 0)
				mdms = c->value ? 1 : 0;
			else
				check = 1;
			c = c->next;
		}
	}

	for (le = le_root; le; le = le->le_next) {
		if (check) {
			for (c = cp; c; c = c->next)
				if (strcmp(le->le_name, c->name) == 0)
					break;
			if (c == NULL || c->value == NULL)
				continue;
		} else if (doall == 0 && (le->le_status & ~TD_SECURE) == 0 &&
		    le->le_state == LE_UNINITIALIZED)
			continue;

		snprintf(buf, sizeof(buf), "%s:status=%s:", le->le_name,
		    statename(le->le_state));

		switch (le->le_state) {
		case LE_CONNECT:
		case LE_INCOMING:
			if (le->le_start == 0)
				break;
			i = strlen(buf);
			snprintf(buf + i, sizeof(buf) - i, "duration=%d:",
				time(NULL) - le->le_start);
			break;
		default:
			break;
		}
		if (le->le_state & LE_AVAILABLE)
			strncat(buf, "available:", sizeof(buf));
		if (le->le_status & TD_SECURE)
			strncat(buf, "secure:", sizeof(buf));
		if (le->le_status & TD_INIT)
			strncat(buf, "manager=init:", sizeof(buf));
		else if (le->le_status & (TD_OUT|TD_IN))
			strncat(buf, "manager=gettyd:", sizeof(buf));
		if (le->le_status & TD_OUT)
			strncat(buf, "dialout:", sizeof(buf));
		if (le->le_status & TD_IN)
			strncat(buf, "dialin:", sizeof(buf));
		if (le->le_fcred.fc_ucred.cr_ref) {
			i = strlen(buf);
			snprintf(buf + i, sizeof(buf) - i,
			    "login=%.*s:ruid=%d:uid=%d:",
			    sizeof(le->le_fcred.fc_login),
			    le->le_fcred.fc_login,
			    le->le_fcred.fc_ruid,
			    le->le_fcred.fc_uid);
		}
		if (le->le_pid) {
			i = strlen(buf);
			snprintf(buf+i, sizeof(buf)-i, "pid=%d:", le->le_pid);
		}
		if (mdms) {
			comma = "";
			i = strlen(buf);

			if ((fd = le->le_mfd) < 0) {
				snprintf(nbuf, sizeof(nbuf), _PATH_DEV "%s",
				    le->le_name);
				fd = open(nbuf, O_NONBLOCK);
			}
			if (fd < 0) {
				if (errno != ENOENT)
					snprintf(buf+i, sizeof(buf)-i,
					    "openerror=%s:", strerror(errno));
				goto mdms_out;
			}

			if (ioctl(fd, TIOCMGET, &state) < 0) {
				if (errno != ENOTTY)
					snprintf(buf+i, sizeof(buf)-i,
					    "mdmctl=%s:", strerror(errno));
				goto mdms_done;
			}
			if (state == 0)
				goto mdms_done;

			snprintf(buf+i, sizeof(buf)-i,
			    "mdmctl=");

			if (state & TIOCM_LE) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "LE");
				comma = ",";
			}
			if (state & TIOCM_CD) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "CD");
				comma = ",";
			}
			if (state & TIOCM_RI) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "RI");
				comma = ",";
			}
			if (state & TIOCM_DTR) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "DTR");
				comma = ",";
			}
			if (state & TIOCM_DSR) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "DSR");
				comma = ",";
			}
			if (state & TIOCM_RTS) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "RTS");
				comma = ",";
			}
			if (state & TIOCM_CTS) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "CTS");
				comma = ",";
			}
			if (state & TIOCM_ST) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "ST");
				comma = ",";
			}
			if (state & TIOCM_SR) {
				i = strlen(buf);
				snprintf(buf+i, sizeof(buf)-i,
				    "%s%s", comma, "SR");
				comma = ",";
			}
			i = strlen(buf);
			snprintf(buf+i, sizeof(buf)-i, ":");

mdms_done:
			if (tcgetattr(fd, &tio) < 0)
				goto mdms_done2;

			i = strlen(buf);
			if (cfgetispeed(&tio) != cfgetospeed(&tio))
				snprintf(buf+i, sizeof(buf)-i,
				    "ispeed=%d:ospeed=%d:",
				    cfgetispeed(&tio), cfgetospeed(&tio));
			else
				snprintf(buf+i, sizeof(buf)-i,
				    "speed=%d:", cfgetispeed(&tio));
mdms_done2:
			if (fd != le->le_mfd)
				close(fd);
mdms_out:		;
		}
		strncat(buf, "\n", sizeof(buf));
		write(re->re_socket, buf, strlen(buf));
	}
	cap_free(cp);
}

/*
 * process the request for status from the user
 */
void
do_reset(struct request *re)
{
	cap_t *c, *cp;
	struct linent *le;

	if (re->re_fcred.fc_uid) {
		WRITES(re->re_socket, "error:permission denied.\n");
		return;
	}

	if ((c = cp = cap_make(re->re_data, NULL)) != NULL) {
		while (c) {
			if (strcmp(c->name, RESOURCE_VERBOSE) == 0)
				verbose = c->value != NULL &&
				    strcasecmp(c->values[0], "off") != 0;
			else if (strcmp(c->name, RESOURCE_DEBUG) == 0)
				debug = c->value != NULL &&
				    strcasecmp(c->values[0], "off") != 0;
			else if (c->value == NULL)
				/* this entry is marked absent */;
			else if (strcmp(c->name, RESOURCE_REREAD) == 0)
				readlinents();
			else if (strcmp(c->name, RESOURCE_SHUTDOWN) == 0)
				on_the_way_down = 1;
			else if (strcmp(c->name, RESOURCE_DIE) == 0)
				terminate();
			else if (strcmp(c->name, RESOURCE_MULTIUSER) == 0) {
				single_user = 0;
				readlinents();
			} else
				for (le = le_root; le; le = le->le_next)
					if (strcmp(c->name, le->le_name) == 0) {
						end_session(le);
						new_session(le);
						break;
					}
			c = c->next;
		}
		cap_free(cp);
	}
}

void
busy_request(struct request *re)
{
	WRITES(re->re_socket, "error:no available lines.\n");
	del_request(re);
	return;
}

/*
 * process the request from the user
 */
void
do_request(struct request *re)
{
	int i;
	cap_t *r;
	cap_t *a;
	cap_t *c;
	cap_t *d;
	cap_t *num = 0;
	char *nv = 0;
	char **av;
	char **bv;
	char *p;
	struct passwd *pwd;
	struct group *grp;
	char ugname[128];

	re->re_fcred.fc_ucred.cr_ref = 1;	/* mark creditials as valid */

	info("request:%s:time=%ld:login=%.*s:ruid=%d:uid=%d:", re->re_data,
	    time(NULL),
	    sizeof(re->re_fcred.fc_login), re->re_fcred.fc_login,
	    re->re_fcred.fc_ruid,
	    re->re_fcred.fc_uid);

	if (strncasecmp(re->re_data, RESOURCE_STATUS ":",
	    sizeof(RESOURCE_STATUS)) == 0) {
		switch(fork()) {
		case -1:
			snprintf(ugname, sizeof(ugname), "error: fork: %s\n",
			    strerror(errno));
			write(re->re_socket, ugname, strlen(ugname));
			break;
		case 0:
			/* Do the status in blocking mode */
			fcntl(re->re_socket, F_SETFL, 0);
			do_status(re);
			exit(0);
		default:
			break;
		}
		del_request(re);
		return;
	}

	if (strncasecmp(re->re_data, RESOURCE_RESET ":",
	    sizeof(RESOURCE_RESET)) == 0) {
		do_reset(re);
		del_request(re);
		return;
	}

	if (strncasecmp(re->re_data, RESOURCE_REQUEST ":",
	    sizeof(RESOURCE_REQUEST))) {
		WRITES(re->re_socket, "error:invalid request.\n");
		del_request(re);
		return;
	}

	/*
	 * Handle the degenerate request of just "request:"
	 */
	if (strcmp(re->re_data, RESOURCE_REQUEST ":") == 0)
		c = NULL;
	else if (!(c = cap_make(re->re_data, _EMPTYCAP)) || c == _EMPTYCAP) {
		WRITES(re->re_socket, "error:failed to parse request.\n");
		del_request(re);
		return;
	} 
	for (d = c; d; d = d->next)
		if (strcmp(d->name, RESOURCE_DESTINATION) == 0)
			break;

	if (d) {
		/*
		 * XXX - we could read up multiple destinations, but it
		 * would not make too much sense (we only allow one
		 * number field, for instance).
		 */
		if (d->value == NULL || (p = getttycapbuf(d->value)) == NULL) {
			WRITES(re->re_socket, "error:bad destination.\n");
			del_request(re);
			return;
		}
		if (p != _EMPTYVALUE) {
			if (!(c = cap_make(p, c)) || c == _EMPTYCAP) {
				free(p);
				WRITES(re->re_socket,
				    "error:failed to parse destination.\n");
				del_request(re);
				return;
			}
			free(p);
		}
	}


	for (d = c; d; d = d->next) {
		if (strcmp(d->name, RESOURCE_CALLTYPE) == 0 ||
		    strcmp(d->name, RESOURCE_USERS) == 0 ||
		    strcmp(d->name, RESOURCE_GROUPS) == 0) {
			WRITES(re->re_socket,
			    "error:privileged field specified\n");
clean_up:
			free(nv);
			cap_free(c);
			del_request(re);
			return;
		}
	}

	pwd = getpwuid(re->re_fcred.fc_ruid);
	cap_add(&c, RESOURCE_USERS, pwd ? pwd->pw_name : DEF_USER, CM_NORMAL);
	for (d = c; d; d = d->next)
		if (strcmp(d->name, RESOURCE_USERS) == 0)
			break;
	if (!d) {
out_of_mem:
		WRITES(re->re_socket, "error:out of memory\n");
		goto clean_up;
	}
	snprintf(ugname, sizeof(ugname), RESOURCE_USER, d->values[0]);
	re->re_user = getttycap(ugname);
	if (re->re_user == 0)
		goto out_of_mem;

	cap_add(&c, RESOURCE_GROUPS, 0, CM_NORMAL);
	for (d = c; d; d = d->next)
		if (strcmp(d->name, RESOURCE_GROUPS) == 0)
			break;
	if (!d)
		goto out_of_mem;

	if (d->value && d->value != _EMPTYVALUE)
		free(d->value);
	d->value = 0;
	free(d->values);

	d->values = malloc(sizeof(char *) * (re->re_fcred.fc_ngroups + 2));
	if (d->values == NULL)
		goto out_of_mem;
	memset(d->values, 0, sizeof(char *) * (re->re_fcred.fc_ngroups + 2));

	grp = getgrgid(re->re_fcred.fc_rgid);
	if (strsplice(&d->value, grp ? grp->gr_name : DEF_GROUP) == NULL)
		goto out_of_mem;

	for (i = 0; i < re->re_fcred.fc_ngroups; ++i) {
		grp = getgrgid(re->re_fcred.fc_groups[i]);
		if (strsplice(&d->value, grp ? grp->gr_name : DEF_GROUP) ==NULL)
			goto out_of_mem;
	}

	av = d->values;
	p = d->value;
	while (*p) {
		*av++ = p;
		while (*p && *p != '\377')
			++p;
		if (*p)
			*p++ = '\0';
	}
	*av = 0;
	for (av = d->values; *av; ++av) {
		if (*av != _EMPTYVALUE)
			for (bv = av + 1; *bv; ++bv)
				if (strcmp(*av, *bv) == 0)
					*bv = _EMPTYVALUE;
	}

	for (av = d->values; *av; ++av) {
		while (*av == _EMPTYVALUE) {
			bv = av;
			while (*bv++)
				bv[-1] = *bv;
		}
		if (*av == NULL)
			break;
		snprintf(ugname, sizeof(ugname), RESOURCE_GROUP, *av);
		cap_merge(&re->re_user, getttycap(ugname), CM_PERMISSIONS);
		if (re->re_user == 0)
			goto out_of_mem;
	}
	if (re->re_user == _EMPTYCAP)
		re->re_user = 0;
#ifndef	real_noisy_debug
	if (debug) {
		cap_t *c;
		dprintf("user restrictions:");
		for (c = re->re_user; c; c = c->next) {
			if (c->value && c->value != _EMPTYVALUE) {
				char **av;
				char buf[1024];
				buf[0] = 0;
				for (av = c->values; *av; ++av) {
					if (buf[0])
						strcat(buf, " | ");
					strcat(buf, *av);
				}
				dprintf("	:%s=%s:", c->name, buf);
			} else
				dprintf("	:%s:", c->name);
		}
	}
#endif

	for (r = re->re_user; r; r = r->next)
		if (strcmp(r->name, RESOURCE_RESTRICT) == 0)
			break;
	for (a = re->re_user; a; a = a->next)
		if (strcmp(a->name, RESOURCE_ALLOW) == 0)
			break;

	for (num = c; num && strcmp(num->name,RESOURCE_NUMBER); num = num->next)
		;
	if (num) {
		char **nums;
		int cnt = 1;		/* null at the end */

		/*
		 * create calltype entry to match numbers
		 */

		cap_add(&c, RESOURCE_CALLTYPE, 0, CM_NORMAL);
		for (d = c; d; d = d->next)
			if (strcmp(d->name, RESOURCE_CALLTYPE) == 0)
				break;

		if (!d)
			goto out_of_mem;

		if (d->value && d->value != _EMPTYVALUE)
			free(d->value);
		d->value = 0;
		free(d->values);
		d->values = 0;

		/*
		 * classify each number given.
		 * the entries in RESOURCE_CALLTYPE will correspond
		 * to the entries in RESOURCE_NUMBER.
		 */
		nv = 0;
		for (nums = num->values; nums && *nums; ++nums) {
			char *p;
			char *nm;
			char n[MAXCALLTYPESIZE];
			expandnumber_t e;

			memset(&e, 0, sizeof(e));
			e.original = *nums;

			while ((nm = expandnumber(&e)) != NULL) {
				++cnt;
				p = process_number(nm, n, sizeof(n), 0);
				if (p == 0 ||
				    (p = phone_variable("NUMBER")) == NULL) {
					WRITES(re->re_socket,
					    "error:invalid number\n");
					while (expandnumber(&e))
						;
					goto clean_up;
				}
				dprintf("%s maps to %s (%s)", nm, p, n);
				if (checknumber(p, r, a) < 0) {
					dprintf("cannot use number %s", p);
					WRITES(re->re_socket,
					    "error:invalid special sequence\n");
					while (expandnumber(&e))
						;
					goto clean_up;
				}
				if (strsplice(&(d->value), n) == NULL ||
				    strsplice(&nv, p) == NULL) {
					while (expandnumber(&e))
						;
					goto out_of_mem;
				}
			}
		}

		if (cnt == 1) {
			dprintf("expanded request yields no numbers");
			WRITES(re->re_socket,
			    "error:expanded request yields no numbers\n");
			goto clean_up;
		}


		if ((d->values = malloc(sizeof(char *) * cnt)) == NULL)
			goto out_of_mem;
		memset(d->values, 0, sizeof(char *) * cnt);

		if ((av = malloc(sizeof(char *) * cnt)) == NULL) {
			free(d->values);
			goto out_of_mem;
		}
		memset(av, 0, sizeof(char *) * cnt);

		free(num->values);
		num->values = av;

		free(num->value);
		num->value = nv;

		av = d->values;
		while (*p) {
			*av++ = p;
			while (*p && *p != '\377')
				++p;
			if (*p)
				*p++ = '\0';
		}
		*av = 0;

		av = num->values;
		p = num->value;
		while (*p) {
			*av++ = p;
			while (*p && *p != '\377')
				++p;
			if (*p)
				*p++ = '\0';
		}
		*av = 0;
	} else {
		cap_add(&c, RESOURCE_CALLTYPE, "DIRECT", CM_NORMAL);
		dprintf("DIRECT call");
	}

	num = c;

	if (re->re_user == _EMPTYCAP)
		re->re_user = 0;

	d = re->re_user;

	/*
	 * d has the restrictions on the user
	 * c has the request from the user
	 */
	while (d && c) {
		i = strcmp(d->name, c->name);
		if (i == 0 && d->value && d->value != _EMPTYVALUE &&
		    !cap_verify(c, d)) {
			dprintf("Failed on %s", d->name);
perm_deny:
			WRITES(re->re_socket, "error:permission denied.\n");
			cap_free(num);
			del_request(re);
			return;
		}
		if (i <= 0)
			d = d->next;
		if (i >= 0)
			c = c->next;
	}

	/*
	 * Now check to make sure the required fields were specified.
	 */
	for (d = re->re_user; d; d = d->next)
		if (strcmp(d->name, RESOURCE_REQUIRED) == 0)
			break;

	if (d) {
		for (av = d->values; *av; ++av) {
			for (c = num; c; c = c->next)
				if (strcmp(c->name, *av) == 0)
					break;
			if (c == 0)
				goto perm_deny;
		}
	}

#ifdef	real_noisy_debug
	if (debug) {
		dprintf("final request parameters:");
		for (c = num; c; c = c->next) {
			if (c->value && c->value != _EMPTYVALUE) {
				char **av;
				char buf[1024];
				buf[0] = 0;
				for (av = c->values; *av; ++av) {
					if (buf[0])
						strcat(buf, " | ");
					strcat(buf, *av);
				}
				dprintf("	:%s=%s:", c->name, buf);
			} else
				dprintf("	:%s:", c->name);
		}
	}
#endif
	
	re->re_num = num;
	re->re_timeout = 60;	/* XXX */
	schedule_event(re, busy_request, (double)re->re_timeout);
	match_request(re);
}

void
match_request(struct request *re)
{
	struct linent *le;
	int busy = 0;

	le = findmatch(re->re_num, &busy, re->re_fcred.fc_ruid == 0);

	if (le == NULL && busy) {
		WRITES(re->re_socket, "message:waiting for line to become available\n");
		return;
	}

	delete_event(re->re_event);

	if (le == NULL) {
		WRITES(re->re_socket, "error:no available lines.\n");
		del_request(re);
		return;
	}
	re->re_num = NULL;

	fb_init(&le->le_rfb, re->re_socket);
	re->re_socket = -1;
	le->le_fcred = re->re_fcred;
	del_request(re);

	if (le->le_state == LE_WATCH) {
		close_modem(le);
		fb_close(&le->le_sfb);
		if (le->le_pid) {
			killpg(le->le_pid, SIGKILL);
			kill(le->le_pid, SIGKILL);
		}
		
		sendmessage(le, "waiting for line to clear.");
		set_state(le, LE_WATCHDEATH);
		schedule_event(le, watch_timeout, 30.0);
	} else if (le->le_state == LE_WAIT) {
		close_modem(le);
		dialcall(le);
	} else if (le->le_state == LE_IDLE)
		dialcall(le);
	else {
		senderror(le, "dialer in invalid state: %s",
				statename(le->le_state));
		clear_session(le);
		fb_close(&le->le_rfb);
		cap_free(le->le_req);
		le->le_req = NULL;
	}
}

static char *
expandnumber(expandnumber_t *e)
{
	char *cp, *hst;

	if (e->original == NULL)
		return(NULL);
	if (e->original[0] != '@') {
		cp = e->original;
		e->original = NULL;
		return(cp);
	}
	if (e->fp == NULL && (e->fp = fopen(_PATH_PHONES, "r")) == NULL) {
		warning("%s: %s", _PATH_PHONES, strerror(errno));
		return(NULL);
	}
	while (fgets(e->string, sizeof(e->string), e->fp)) {
		for (cp = e->string; *cp && isspace(*cp); cp++)
			;
		if ((hst = strchr(cp, '\n')) != NULL)
			*hst = 0;
		if ((hst = strchr(cp, '\r')) != NULL)
			*hst = 0;
		/*
		 * if it's a comment or end of line, skip on
		 */
		if (*cp == '\0' || *cp == '#')
			continue;

		/*
		 * now skip past host name and terminate
		 */
		for (hst = cp; *cp && !isspace(*cp); cp++)
			;
		if (*cp == '\0') {
			warning("%s: bad line: %s", _PATH_PHONES, e->string);
			continue;
		}
		*cp++ = '\0';

		/*
		 * this this our host?
		 */
		if (strcmp(hst, e->original+1) != 0)
			continue;               /* doesn't match */

		/*
		 * now skip white space between host and phone
		 */
		while (*cp && isspace(*cp))
			cp++;
		if (*cp == '\0') {
			warning("%s: bad line: %s", _PATH_PHONES, e->string);
			continue;
		}
		return(cp);
	}
	fclose(e->fp);
	return(NULL);
}

void
watch_timeout(struct linent *le)
{
	if (le->le_pid && kill(le->le_pid, 0) == 0) {
		killpg(le->le_pid, SIGKILL);
		kill(le->le_pid, SIGKILL);
		senderror(le, "timeout waiting for line to clear");
		set_state(le, LE_KILLING);
		schedule_event(le, hung_session, 30.0);
		return;
	}
	le->le_pid = 0;
	dialcall(le);
}

void
dialcall(struct linent *le)
{
	argv_t *avt;
	char buf[sizeof(_PATH_DEV "fd/xxxxxxxx")];
	cap_t *ct, *mt, *lt, *nt, *rt;
	char *p;
	char **pv;
	int i;
	int sv[2];
	char name[256];

	set_state(le, LE_DIAL);

	/*
	 * If we do not request numbers then we should not try and dial
	 */
	nt = getent_session(le, RESOURCE_NUMBER);

	if ((ct = getent_session(le, RESOURCE_DIALER)) == NULL && nt != NULL) {
		senderror(le, "no dialer available");
		clear_session(le);
		return;
	}

	if (le->le_mfd >= 0)
		close_modem(le);
	else if ((le->le_mfd = devopen(le->le_name, MODEM_MODE)) >= 0)
		close(le->le_mfd);
	if ((le->le_mfd = devopen(le->le_name, MODEM_MODE)) < 0) {
		senderror(le, "%s: %s", le->le_name, strerror(errno));
		clear_session(le);
		return;
	}
	sprintf(buf, _PATH_DEV "fd/%d", le->le_mfd);

	if (ct == NULL) {
		/*
		 * We need to set the baud rate ourself... grumble
		 */
		p = 0;
		if ((lt = getent_session(le, RESOURCE_DTE)) != NULL) {
			speed_t baud = 0;
			for (mt = le->le_req; mt; mt = mt->next)
				if (strcmp(lt->name, mt->name) == 0) {
					if ((p = cap_check(lt, mt)) != NULL)
						baud = strtol(p, 0, 10);
					break;
				}
			if (baud) {
				struct termios t;

				if (tcgetattr(le->le_mfd, &t) >= 0) {
					cfsetospeed(&t, baud);
					cfsetispeed(&t, baud);
					tcsetattr(le->le_mfd, TCSANOW, &t);
				} else
					warning("%s: tcgetattr: %s\n",
					    le->le_name, strerror(errno));
			}
		}
		WRITES(le->le_rfd, "fd\n");
		if (fcntl(le->le_mfd, F_SETFL, 0) < 0)
			warning("%s: setting non-block: %s",
				le->le_name, strerror(errno));
		sendfd(le->le_rfd, le->le_mfd);
		close_modem(le);
		set_state(le, LE_CONNECT);
		le->le_start = time(NULL);
		info("%s:state=OUTGOING:time=%ld:login=%.*s:ruid=%d:uid=%d:",
		    le->le_name, le->le_start,
		    sizeof(le->le_fcred.fc_login),
		    le->le_fcred.fc_login,
		    le->le_fcred.fc_ruid,
		    le->le_fcred.fc_uid);
		return;
	}

	avt = start_argv(ct->values[0], "-f", buf,
	    "-" RESOURCE_LINE, le->le_name, 0);

	if (!avt) {
		senderror(le, "%s: %s", le->le_name, strerror(errno));
		clear_session(le);
		return;
	}

	rt = cap_look(le->le_cap, RESOURCE_ARGUMENTS);

	for (lt = le->le_cap; lt; lt = lt->next) {
		if (strcmp(lt->name, RESOURCE_LINE) == 0)
			continue;
		for (mt = le->le_req; mt; mt = mt->next) {
			if (strcmp(lt->name, mt->name) == 0) {
				/*
				 * give the dialer all the numbers to try
				 */
				if (strcmp(lt->name, RESOURCE_NUMBER) == 0) {
					snprintf(name, sizeof(name),
					    "-%s", lt->name);
					add_argv(avt, name);
					for (pv = mt->values; *pv; ++pv) {
						mapspecial(le, *pv);
						add_argv(avt, *pv);
					}
				} else if ((p = cap_check(lt, mt)) != NULL) {
					snprintf(name, sizeof(name),
					    "-%s", lt->name);
					add_argv(avt, name);
					if (*p)
						add_argv(avt, p);
				}
				break;
			}
		}
		if (mt || rt == NULL)
			continue;
		for (pv = rt->values; *pv; ++pv)
			if (!strcmp(lt->name, *pv)) {
				if (lt->value == NULL)
					break;
				snprintf(name, sizeof(name),
				    "-%s", lt->name);
				add_argv(avt, name);
				for (pv = lt->values; *pv; ++pv)
					add_argv(avt, *pv);
				break;
			}
	}

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
		senderror(le, "%s: creating socketpair: %s",
		    le->le_name, strerror(errno));
		free_argv(avt);
		clear_session(le);
		return;
	}
	if (fcntl(sv[0], F_SETFL, O_NONBLOCK) < 0) {
		close(sv[0]);
		close(sv[1]);
		senderror(le, "%s: setting socketpair to noblocking: %s",
		    le->le_name, strerror(errno));
		free_argv(avt);
		clear_session(le);
		return;
	}

	fb_init(&le->le_sfb, sv[0]);

	infocall(le->le_name, avt);

	switch (le->le_pid = fork()) {
	case -1:
		free_argv(avt);
		le->le_pid = 0;
		clear_session(le);
		return;
	default:
		close(sv[1]);
		FD_SET(sv[0], read_set);
		free_argv(avt);

		return;
	case 0:
		setpgid(0, getpid());
		dup2(sv[1], 0);
		dup2(sv[1], 1);
		dup2(sv[1], 2);
		for (i = 3; i < numberfds; ++i)
			if (i != le->le_mfd)
				close(i);
		break;
	}

	ioctl(le->le_mfd, TIOCSCTTY, (char *)NULL);

	/*
	 * dialer tty01 /dev/fd/xx
	 */
	execscript(avt);
}

/*
 * Map special sequences to what ever the modem wants
 * Each sequence can map into only as many characters as its name
 * (since this is inline expansion)
 */
void
mapspecial(struct linent *le, char *pn)
{
	char *e;
	cap_t *map;
	char **av;
	char *m;
	char *p;

	map = getent_session(le, RESOURCE_MAP);

	p = pn;
	while (*pn) {
		if (*pn == '<') {
			if ((e = strchr(pn, '>')) == NULL) {
				*p++ = *pn++;
				continue;
			}
			++e;
			m = "";
			if (map && (av = map->values)) {
				for (; *av; ++av) {
					if (strncasecmp(*av, pn, e - pn))
						continue;
					m = &(*av)[e-pn];
					break;
				}
				if (strlen(m) > e - pn) {
					warning("%s: mapping %s too long",
					    le->le_name, *av);
					m = "";
				}
			}
			while ((*p = *m++) != NULL)
				++p;
			pn = e;
		} else
			*p++ = *pn++;
	}
	*p = 0;
}

/*
 * Check ``pn'' to make sure there are no special sequences
 * defined in ``r''.  If ``a'' is defined then only allow the
 * sequences defined in ``a''.
 */
int
checknumber(char *pn, cap_t *r, cap_t *a)
{
	char *opn = pn;
	char *e;
	char **av;

	while ((pn = strchr(pn, '<')) != NULL) {
		if ((e = strchr(pn, '>')) == NULL) {
			warn("invalid phone number: %s", opn);
			return(-1);
		}
		++e;
		if (r && (av = r->values)) {
			for (; *av; ++av) {
				if (strncasecmp(*av, pn, e - pn) == 0 &&
				    (*av)[e-pn] == 0)
					return(-1);
			}
		}
		if (a && (av = a->values)) {
			for (; *av; ++av) {
				if (strncasecmp(*av, pn, e - pn) == 0 &&
				    (*av)[e-pn] == 0)
					break;
			}
			if (!*av)
				return(-1);
		}
		pn = e;
	}
	return(0);
}

void
inp_request(struct request *re)
{
	if (re->re_fcred.fc_ucred.cr_ref == 0) {
		if (recvcred(re->re_socket, &re->re_fcred) < 0) {
			warning("no credentials returned: %s", strerror(errno));
			del_request(re);
			return;
		}
		re->re_fcred.fc_ucred.cr_ref = 1;
		return;
	}

	switch (fb_read(&re->re_fb, '\n')) {
	case 1:
		do_request(re);
		break;
	case 0:
		break;
	case -1:
		/*
		 * hmm, they closed it.  Just go away.
		 */
		del_request(re);
		break;
	case -2:
		warning("Too much data from request.");
		del_request(re);
		break;
	case -3:
		warning("Reading from request: %s", strerror(errno));
		del_request(re);
		break;
	}
}

/*
 * Read data from dialer or watcher program.
 */
void
inps_session(struct linent *le)
{
	char *ibuf;

	ibuf = le->le_sfb.fb_buffer;

	switch (fb_read(&le->le_sfb, '\n')) {
	case 1:	/* got a line */
		switch (le->le_state) {
		case LE_DIAL:
			if (strncmp(ibuf, "AD_NUMBER:", 10) == 0) {
				if (le->le_number)
					free(le->le_number);
				le->le_number = strdup(ibuf+10);
			} else if (strncmp(ibuf, "AD_ERROR:", 9) == 0)
				senderror(le, ibuf + 9);
			else
				sendmessage(le, ibuf);
			break;
		case LE_WATCH:
		case LE_WATCHIN:
			if (strcmp(ibuf, "AD_ANSWER") == 0) {
				close_modem(le);
				fb_close(&le->le_sfb);
				if (le->le_uucp && uu_lock(le->le_name) < 0)
					warning("%s: could not get uucp lock for incoming call", le->le_name);
				else
					le->le_uulocked = le->le_uucp;

				le->le_start = time(NULL);
				info("%s:state=INCOMING:time=%ld:",
				    le->le_name, le->le_start);
				set_state(le, LE_INCOMING);
			} else
				warning("%s: watcher: %s", le->le_name, ibuf);
			break;
		case LE_HANGUP:
			warning("%s: hangup: %s", le->le_name, ibuf);
			break;
		case LE_CONDITION:
			warning("%s: condition: %s", le->le_name, ibuf);
			break;
		default:
			warning("%s: %s: %s", le->le_name,
				statename(le->le_state), ibuf);
			break;
		}
		break;
	case 0:	/* no line yet */
		break;
	case -1: /* line closed */
		switch (le->le_state) {
		case LE_WATCH:
		case LE_WATCHIN:
			close_modem(le);
			fb_close(&le->le_sfb);
			if (le->le_uucp && uu_lock(le->le_name) < 0)
				warning("%s: could not get uucp lock for incoming call", le->le_name);
			else
				le->le_uulocked = le->le_uucp;
			le->le_start = time(NULL);
			info("%s:state=INCOMING:time=%ld:",
			    le->le_name, le->le_start);
			set_state(le, LE_INCOMING);
			break;
		default:
			fb_close(&le->le_sfb);
			dprintf("%s: line to script closed", le->le_name);
			clear_session(le);
			new_session(le);
			break;
		}
		break;
	case -2: /* too much data */
		fb_close(&le->le_sfb);
		warning("%s: too much data from script", le->le_name);
		clear_session(le);
		new_session(le);
		break;
	case -3: /* error in reading */
		warning("%s: while reading from script: %s",
		    le->le_name, strerror(errno));
		fb_close(&le->le_sfb);
		clear_session(le);
		new_session(le);
		break;
	}
}

/*
 * Read data from client program (after the request has been made)
 */
void
inpr_session(struct linent *le)
{
	/*
	 * Currently we ignore all data from the client.
	 */
	switch (fb_read(&le->le_rfb, '\n')) {
	case 1:	/* got a line */
		break;
	case 0:	/* no line yet */
		break;
	case -1: /* line closed */
		failedfd(le->le_rfd);
		break;
	case -2: /* too much data */
		break;
	case -3: /* error in reading */
		break;
	}
}

void
start_getty(struct linent *le)
{
	int i;

	FD_CLR(le->le_mfd, write_set);
	FD_SET(le->le_mfd, close_set);

	if (single_user) {
		close_modem(le);
		set_state(le, LE_SINGLEUSER);
		schedule_event(le, new_session, 10.0);
	}

	if (le->le_uucp) {
		if (uu_lock(le->le_name) < 0) {
			/*
			 * Someone else has the line, start checking
			 * every five seconds to see if the lock
			 * has been freed.
			 */
			close_modem(le);
			set_state(le, LE_UUCPLOCKED);
			schedule_event(le, check_uucplock, 5.0);
			return;
		} else
			le->le_uulocked = 1;
	}

	le->le_start = time(NULL);
	info("%s:state=INCOMING:time=%ld:", le->le_name, le->le_start);
			
	set_state(le, LE_INCOMING);

	switch(le->le_pid = fork()) {
	case -1:
		stall("%s: cannot fork: %s",
			le->le_name, strerror(errno));
		le->le_pid = 0;
		clear_session(le);
		new_session(le);
		return;
	case 0:
		dup2(le->le_mfd, 0);
		dup2(le->le_mfd, 1);
		dup2(le->le_mfd, 2);
		for (i = 3; i < numberfds; ++i)
			close(i);
		(void) sigemptyset(&blockedsigs);
		sigprocmask(SIG_SETMASK, &blockedsigs, 0);
		for (i = 0; i < NSIG; ++i)
			setsig(i, SIG_DFL);
		setsid();
		fcntl(0, F_SETFL, 0);
		ioctl(0, TIOCSCTTY, 0);
		execv(le->le_getty_argv[0], le->le_getty_argv);
		stall("%s: %s: %s", le->le_name, le->le_getty_argv[0],
			strerror(errno));
		_exit(127);
	default:
		close_modem(le);
		break;
	}
}

void
handle_request()
{
	struct request *re;
	int a;

	if ((a = accept(soket, 0, 0)) < 0) {
		warning("accept failed: %s", strerror(errno));
		return;
	}

	if (fcntl(a, F_SETFL, O_NONBLOCK) < 0) {
		warning("setting to nonblocking: %s", strerror(errno));
		close(a);
		return;
	}

	if ((re = malloc(sizeof(struct request))) == NULL) {
		warning("Allocating request structure: %s", strerror(errno));
		close(a);
		return;
	}

	memset(re, 0, sizeof(struct request));

	fb_init(&re->re_fb, a);
	re_root = re;

	FD_SET(re->re_socket, read_set);
}

void
process()
{
	fd_set *rs;
	fd_set *ws;
	fd_set *xs;
	int r;
	struct timeval tv;
	struct linent *le;
	struct request *re;
	int oerrno;
#define	NTRIES	10
	static struct {
		time_t	when;
		fd_set	*rs;
		fd_set	*ws;
		fd_set	*xs;
		int	r;
	} tsa, sa[NTRIES] = { { 0,}, };

	rs = mk_fd_set();
	ws = mk_fd_set();
	xs = mk_fd_set();
	for (r = 0; r < NTRIES; ++r) {
		sa[r].rs = mk_fd_set();
		sa[r].ws = mk_fd_set();
		sa[r].xs = mk_fd_set();
	}

	(void) sigemptyset(&blockedsigs);
	(void) sigaddset(&blockedsigs, SIGHUP);
	(void) sigaddset(&blockedsigs, SIGPIPE);
	(void) sigaddset(&blockedsigs, SIGTERM);
	(void) sigaddset(&blockedsigs, SIGCHLD);

	sigprocmask(SIG_BLOCK, &blockedsigs, 0);

	setsig(SIGCHLD, dochild);
	setsig(SIGHUP, dohup);
	setsig(SIGTERM, doterm);
	setsig(SIGPIPE, dopipe);
	setsig(SIGALRM, doalrm);

	for (;;) {
		gettimeofday(&tv, 0);
		call_events(tv);
		/*
		 * if lines became available, check for outstanding requests
		 */
		if (became_available)
			for (re = re_root; re;) {
				struct request *nre = re->re_next;
				if (re->re_event) {
					WRITES(re->re_socket, "message:checking available lines\n");
					match_request(re);
				}
				re = nre;
			}
		became_available = 0;

		if (events) {
			if (events->when.tv_usec < tv.tv_usec) {
				tv.tv_usec = events->when.tv_usec + 1000000
					     - tv.tv_usec;
				tv.tv_sec = events->when.tv_sec - tv.tv_sec - 1;
			} else {
				tv.tv_usec = events->when.tv_usec - tv.tv_usec;
				tv.tv_sec = events->when.tv_sec - tv.tv_sec;
			}
		}
		/*
		 * There is a small race between unblocking signals
		 * and calling select.  Since any pending signal's handler
		 * will be called before sigprocmask() returns, we can
		 * just set the timeout for select to 0 if we notice that
		 * we have any signal handlers to call (so we can poll
		 * the file descriptors as well).
		 * We still have to deal with the possiblity that a signal
		 * will come in after the if() but before the select(), which
		 * means we still must have to have an upper bound.
		 * 
		 * One side note, we often will get here and tv will have
		 * the time of day in it.  This will always be greater than
		 * our maximum timeout (unless you want to wait for over
		 * 25 years)
		 *
		 * One final cheese hack is that when signals come in, we
		 * set an alarm for a short period of time, this way, if
		 * we do miss a signal, the select() will return shortly
		 * anyhow.
		 */
		if (tv.tv_sec >= MAXTIMEO) {
			tv.tv_sec = MAXTIMEO;
			tv.tv_usec = 0;
		}

		FD_NZERO(numberfds, close_set);
		sigprocmask(SIG_UNBLOCK, &blockedsigs, 0);
		if (callind) {
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		}
		FD_NCOPY(numberfds, read_set, rs);
		FD_NCOPY(numberfds, write_set, ws);
		FD_NCOPY(numberfds, rs, xs);
		r = select(numberfds, rs, ws, xs, &tv);
		oerrno = errno;
		sigprocmask(SIG_BLOCK, &blockedsigs, 0);
		alarm(0);
		if (limit_select) {
			int i;
			tsa = sa[0];
			for (i = 0; i < NTRIES-1; ++i)
				sa[i] = sa[i+1];
			sa[NTRIES-1] = tsa;
			sa[NTRIES-1].when = time(NULL);
			FD_NCOPY(numberfds, rs, sa[NTRIES-1].rs);
			FD_NCOPY(numberfds, ws, sa[NTRIES-1].ws);
			FD_NCOPY(numberfds, xs, sa[NTRIES-1].xs);
			sa[NTRIES-1].r = r;
			if (sa[NTRIES-1].when - sa[0].when < 2) {
				warning("select looping too quickly");
				for (i = 0; i < NTRIES; ++i)
					warning("%d: %08x %08x %08x %d\n",
					    sa[i].when,
					    *(int *)(sa[i].rs),
					    *(int *)(sa[i].ws),
					    *(int *)(sa[i].xs),
					    sa[i].r);
				sleep(2);
			}
		}

		errno = oerrno;
		if (r < 0) {
			switch (errno) {
			case EINTR:
				break;
			case EBADF:
				failedio();
				break;
			case EINVAL:
			default:
				warning("select fails: %s", strerror(errno));
				break;
			}
		}
		callhandlers();
		reap();
		if (le_root == NULL)
			exit(0);

		if (r <= 0)
			continue;

		if (FD_ISSET(soket, rs)) {
			FD_CLR(soket, rs);
			handle_request();
		}
		le = le_root;
		while (le) {
			if (le->le_mfd >= 0 && FD_ISSET(le->le_mfd, ws) &&
			    FD_ISSET(le->le_mfd, write_set)) {
				FD_CLR(le->le_mfd, ws);
				start_getty(le);
				le = le_root;
			} else
			if (le->le_rfd >= 0 && FD_ISSET(le->le_rfd, rs) &&
			    FD_ISSET(le->le_rfd, read_set)) {
				FD_CLR(le->le_rfd, rs);
				inpr_session(le);
				le = le_root;
			} else
			if (le->le_sfd >= 0 && FD_ISSET(le->le_sfd, rs) &&
			    FD_ISSET(le->le_sfd, read_set)) {
				FD_CLR(le->le_sfd, rs);
				inps_session(le);
				le = le_root;
			} else
				le = le->le_next;
		}
		re = re_root;
		while (re) {
			if (re->re_socket >= 0 && FD_ISSET(re->re_socket, rs)){
				FD_CLR(re->re_socket, rs);
				inp_request(re);
				re = re_root;
			} else
				re = re->re_next;
		}
		/*
		 * scan through the filedescriptors looking for ones
		 * we did not process, careful to not check filedescriptors
		 * we closed during processing.
		 * normally we should find no filedescriptors in this state
		 */
		for (r = 0; r < numberfds; ++r) {
			if (FD_ISSET(r, close_set))
				continue;
			if (FD_ISSET(r, rs)) {
				dprintf("dangling read fd %d (closed)\n", r);
				close(r);
				FD_CLR(r, read_set);
			}
			if (FD_ISSET(r, ws)) {
				dprintf("dangling write fd %d (closed)\n", r);
				close(r);
				FD_CLR(r, write_set);
			}
			if (FD_ISSET(r, xs))
				dprintf("dangling except fd %d\n", r);
		}
	}
	free_fd_set(rs);
	free_fd_set(ws);
	free_fd_set(xs);
	for (r = 0; r < NTRIES; r++) {
		free_fd_set(sa[r].rs);
		free_fd_set(sa[r].ws);
		free_fd_set(sa[r].xs);
	}
}

int
recvcred(int soket, struct fcred *fc)
{
	u_char _cmsg[sizeof(struct cmsghdr) + sizeof(struct fcred)];
	struct cmsghdr *cmsg = (struct cmsghdr *)_cmsg;
	struct msghdr msg;

	memset(_cmsg, 0, sizeof(_cmsg));
	memset(&msg, 0, sizeof(msg));
	msg.msg_control = (caddr_t)cmsg;
	msg.msg_controllen = sizeof(_cmsg);

	if (recvmsg(soket, &msg, 0) < 0)
		return(-1);
	if (cmsg->cmsg_len - sizeof(struct cmsghdr) < sizeof(struct fcred))
		return(-1);
	if (cmsg->cmsg_type != SCM_CREDS) {
		warning("control message not SCM_CREDS: %d", cmsg->cmsg_type);
		return(-1);
	}
	*fc = *(struct fcred *)CMSG_DATA(cmsg);
	return(0);
}

fd_set *
mk_fd_set()
{
	fd_set *fs;

        if (fd_set_list == NULL) {
		if ((fs = FD_ALLOC(numberfds)) == NULL) {
			warning("allocating fd_set: %s", strerror(errno));
			exit(1);
		}
	}
	else {
		fs = (fd_set *)fd_set_list;
		fd_set_list = *(void **)fd_set_list;
	}

	FD_NZERO(numberfds, fs);
	return(fs);
}

void
free_fd_set(fd_set *fs)
{
	*(void **)fs = fd_set_list;
	fd_set_list = (void *)fs;
}

void
main(int ac, char **av)
{
	struct sockaddr_un sun;
	struct sockaddr *sa = (struct sockaddr *)&sun;
	struct passwd *pwd;
	struct group *grp;
	struct rlimit file_limit;
	int one = 1;
	int c;
	int backlog = 64;
	FILE *fp;
	char *rulefile = _PATH_RULESET;

	close(STDIN_FILENO);

	if ((pwd = getpwnam("uucp")) != NULL) {
		uucp_uid = pwd->pw_uid;
		uucp_gid = pwd->pw_gid;
	}
	if ((grp = getgrnam("uucp")) != NULL)
		uucp_gid = grp->gr_gid;
	

	while ((c = getopt(ac, av, "a:b:dvl:r:sS")) != EOF)
		switch (c) {
		case 'a':
			actfile = optarg;
			break;
		case 'b':
			backlog = strtol(optarg, 0, 0);
			if (backlog < 1)
				errx(1, "%s: invalid backlog", backlog);
			break;
		case 'd':
			debug = 1;
			daemon_mode = 0;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'r':
			rulefile = optarg;
			break;
		case 's':
			single_user = 1;
			break;
		case 'S':		/* to remain undocumented */
			limit_select = 1;
			break;
		default:
			fprintf(stderr, "usage: gettyd [-dsSv] [-r rulefile]\n");
			exit(1);
		}

	if (parse_rule_file(rulefile) < 0)
		err(1, rulefile);

	if (getrlimit(RLIMIT_NOFILE, &file_limit) || file_limit.rlim_cur < 0)
		err(1, "file limit");

	numberfds = file_limit.rlim_cur;

	read_set = mk_fd_set();
	write_set = mk_fd_set();
	close_set = mk_fd_set();

	if ((soket = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0)
		err(1, "socket");

	sun.sun_family = PF_LOCAL;
	strcpy(sun.sun_path, _PATH_DIALER);

	if (!(bind(soket, sa, sizeof(sun)) == 0 ||
	     (errno == EADDRINUSE && connect(soket, sa, sizeof(sun)) < 0 &&
	      errno == ECONNREFUSED && unlink(sun.sun_path) == 0 &&
	      bind(soket, sa, sizeof(sun)) == 0)))
		err(1, "%s", sun.sun_path);

	/* Make socket available to everyone */
	chmod(sun.sun_path, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

	if (setsockopt(soket, 0, LOCAL_CREDS, &one, sizeof(one)) < 0)
		err(1, "Cannot require credentials");

	listen(soket, backlog);

	FD_SET(soket, read_set);

	if (daemon_mode) {
		if (daemon(0, 1) == -1)
			err(1, "entering daemon mode");
		if ((c = open(_PATH_DEVNULL, O_RDWR, 0)) >= 0) {
			if (STDOUT_FILENO != soket)
				dup2(c, STDOUT_FILENO);
			if (STDERR_FILENO != soket)
				dup2(c, STDERR_FILENO);
			if (c != STDOUT_FILENO && c != STDERR_FILENO)
				close(c);
		}
		openlog("gettyd", LOG_PID|LOG_CONS, LOG_DAEMON);
	}
	if ((fp = fopen(_PATH_VARRUN "gettyd.pid", "w")) != NULL) {
		fprintf(fp, "%d\n", getpid());
		fclose(fp);
	}

	readlinents();

	process();
}
