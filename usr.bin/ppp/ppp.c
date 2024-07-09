/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp.c,v 2.42 2003/07/08 21:58:20 polk Exp
 */

/*
 * Establish a PPP connection
 *
 * ppp  - attach dial-in connection if called as a login shell
 * ppp -d [systemid]
 *      - dial-out dodaemon mode (calls on dropped packets)
 * ppp systemid
 *      - call the specified system explicitly
 *
 * Additional parameters:
 *      -s sysfile      - use sysfile instead of /etc/ppp.sys
 *      -x              - turn on debugging of dialing
 *      -b              - put self in background after connecting
 */

#include "ppp.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <net/slip.h>

#include <arpa/inet.h>

#include <err.h>
#include <login_cap.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <ttyent.h>
#include <utmp.h>
#include <pwd.h>

#include "pbuf.h"
#include "ppp_proto.h"
#include "ppp_var.h"

extern void logwtmp(char *, char *, char *);

char *printable(char *);
int default_down(session_t *);
int default_init(session_t *);
int execute(session_t *, int, char *, ...);
int expect(int, char *, int);
int linkscript(session_t *, char *, int);
int loginscript(session_t *, char *);
void cleanup(int);
void updateutmp(session_t *, char *);
void updatewtmp(session_t *);
void updatelastlog(session_t *);
void waitdrop(char *);
int fttyslot(int);

static void expalarm(void);

int background = 0;
int dodaemon = 0;
int daemon_pass = 0;
char *sysfile = NULL;
jmp_buf wt_env;
int slip_hangup = 0;

int amup = 0;
static ppp_t _ppp;
int dialin = 0;
int async = 1;

session_t *current_session;
static int busymsg = 0;
uid_t suid = -1;
gid_t sgid = -1;

int
main(ac, av)
	int ac;
	char **av;
{
	session_t *S;
	char	*_sysname;
#define	sysname	_sysname
	extern int optind;
	extern char *optarg;
	extern char *getlogin();
	int ld;
	pid_t pid;
	int needwtmp;

	needwtmp = 0;
	srandom(getpid() ^ time(NULL));

	openlog(NULL, LOG_PID, LOG_DAEMON);

	/*
	 * scripts must run as root.  Note that if the -s option is
	 * envoked then suid will be set to the user instead.
	 * in theory this means the only scripts a user will be
	 * able to get to run as root are those set in ppp.sys.
	 */
	suid = geteuid();

	for (;;) {
		switch (getopt(ac, av, "bdis:xX:")) {
		default:
		usage:
			fprintf(stderr,
			    "usage: ppp [-s sysfile] [-bdix] [-X level]"
			    " [systemname]\n");
			exit(1);
		case 'b':
			background++;
			continue;

		case 'd':
			dodaemon++;
			continue;

		case 'i':
			++dialin;
			continue;

		case 's':
			suid = getuid();
			sgid = getgid();
			sysfile = optarg;
			continue;

		case 'x':
			debug |= D_XDEB;
			continue;

		case 'X':
			if (setdebug(optarg))
				errx(1, "%s: invalid debug option", optarg);
			continue;

		case EOF:
			break;
		}
		break;
	}

	if (optind < ac - 1)
		goto usage;

	/* Check permissions */
	if (dodaemon && getuid() != 0)
		errx(1, "only superuser can run in dodaemon mode");

	setbuf(stdout, NULL);

	/* Get the system name */
	if (ac > optind)
		sysname = av[optind];
	else if (dodaemon)
		sysname = 0;
	else {
		sysname = getlogin();
		dialin++;
		if (sysname == 0)
			errx(1, "can't get the login name");
	}
	if (dodaemon && dialin)
		errx(1, "dodaemon mode not allowed for dialin lines");

	/*
	 * if we came in as ppp_direct we will need to write a wtmp
	 * entry, once we know what to write
	 */
	if (dialin && strcmp(sysname, "ppp_direct") == 0)
		needwtmp = 1;

	/*
	 * Daemon mode... look out for the dropped packets!
	 */
	if (dodaemon && sysname == 0)
		/*
		 * Scan the system file for all automatic dial-outs
		 * and do forks for every system name.
		 */
		errx(1, "dodaemon w/o sysname IS NOT IMPLEMENTED (yet)");

	S = getsyscap(sysname, sysfile);
	current_session = S;
#undef	sysname

	if (S->flags & F_SLIP)
		getslcap(S);
	else {
		S->private = &_ppp;
		_ppp.ppp_flags |= PPP_ASYNC;	/* XXX - bletch */
		_ppp.ppp_fd = -1;
		_ppp.ppp_session = S;
		getpppcap(S);
	}

	if (S->device && *S->device != '/')
		async = 0;

#if 0
	if (!dialin && debug && getuid() != 0)
		errx(1, "only superuser can debug");
#endif

	if ((S->flags & F_SLIP) && dodaemon)
		errx(1, "slip does not support daemon mode");

	if (dialin && (S->flags & F_DIALIN) == 0)
		errx(1, "dial in is not allowed for %s", S->sysname);

	if (!dialin && (S->flags & F_DIALOUT) == 0)
		errx(1, "dial out is not allowed for %s", S->sysname);

	if (async && dodaemon && S->interface == -1 &&
	    (S->flags & F_IMMEDIATE) == 0 && dodaemon)
		errx(1, "%s: interface number (in) required for non-immediate"
			"dodaemon mode.", S->sysname);
	if (async && S->interface == -1 && !dialin &&
	    (S->flags & F_IMMEDIATE) == 0)
		errx(1, "%s: dynamic interface allocation is not supported\n"
			"\ton demand dial connections.", S->sysname);

	if ((S->flags & F_SLIP) && S->linkinit == 0)
		errx(1, "link-init script required for SLIP");
	if ((S->flags & F_SLIP) && S->linkdown == 0)
		errx(1, "down script required for SLIP");

	if (dialin) {
		extern FILE *logfile;
		mode_t omask;

		omask = umask(077);
		logfile = fopen(_PATH_PPPDEBUG, "a");
		umask(omask);
		if (logfile == NULL)
			err(1, "opening ppp.debug log file");
		setbuf(logfile, NULL);
	}

	/*
	 * If we in dodaemon mode and we need to wait
	 * for a packet we need to setup the ifname.
	 */
	if (dodaemon && (S->flags & F_IMMEDIATE) == 0) {
		if (async)
			snprintf(S->ifname, sizeof(S->ifname), "%s%d",
			    (S->flags & F_SLIP) ? "sl" : "ppp", S->interface);
		else
			strncpy(S->ifname, S->device, sizeof(S->ifname));
		_ppp.ppp_ifname = S->ifname;
		_ppp.ppp_dl.sdl_index = 0;
	}

	/* Dial-in? Just set the line discipline and wait */

daemon_loop:
	if (dialin) {
		syslog(LOG_INFO, "dialin: %s\n", S->sysname);

		if (!async)
			goto start_protocol;

		S->fd = 0;

		/* Set working line parameters */
		set_lparms(S);
		ioctl(S->fd, TIOCEXCL, 0);
		/* Let the other end know it is okay to send requests. */
		printf("%s Ready.\n", (S->flags & F_SLIP) ? "SLIP" : "PPP");
		goto start_protocol;
	}

	/*
	 * Hunt for an available device
	 */
	(void) signal(SIGHUP, cleanup);
	(void) signal(SIGINT, cleanup);
	(void) signal(SIGTERM, cleanup);

	/*
	 * Wait for a dropped packet at the interface
	 */
	if (dodaemon && (S->flags & F_IMMEDIATE) == 0) {
		amup = 1;
		S->pifname[0] = '\0';
		_ppp.ppp_pifname = S->pifname;
		_ppp.ppp_flags |= PPP_DEMAND; 
		if (startup(S, S->sysname)) {
			dodaemon = 0;
			goto timeout;
		}
		ppp_openskt();
		ppp_clrstate(S->private, PPP_IPREJECT);
		/*
		 * If it's the first pass in daemon mode and remote address
		 * is not known then go ahead and connect to get the addresses.
		 * Otherwise, wait for a packet.
		 */
		if (daemon_pass == 0 && ppp_ipcp_getaddr(&_ppp,DST_ADDR) == 0)
			daemon_pass++;
		else
			waitdrop(S->ifname);
	}

	if (async) {
		char *reason;

		/*
		 * Dial the remote side.  Collect and zombied proxies
		 * we may have created along the way.
		 */
		while ((reason = Dial(S)) != NULL) {
			while (waitpid(-1, NULL, WNOHANG) > 0)
				;
			syslog(LOG_INFO, "%s: %s\n", S->sysname, reason);
			uprintf("Failed for %s: %s\n", S->sysname, reason);
			if (busymsg++ == 0)
				syslog(LOG_INFO, "%s: all ports busy\n",
				    S->sysname);
			if (dodaemon) {
				uprintf("All ports busy...\n");
				sleep(60);
				goto daemon_loop;
			}
			errx(1, "all ports busy");
		}
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
		busymsg = 0;
		uprintf("Connected...\n");
	}
		
	/*
	 * Set SLIP or PPP line discipline
	 */
start_protocol:
	if (async) {
		if (S->flags & F_SLIP)
			(void) signal(SIGHUP, SIG_IGN);

		uprintf("Set %s line disc.\n",
		    (S->flags & F_SLIP) ? "SLIP" : "PPP");

		ld = (S->flags & F_SLIP) ? SLIPDISC : PPPDISC;
		if (ioctl(S->fd, TIOCSETD, &ld) < 0) {
			disconnect(S);
			err(1, "setting line discipline for %s", S->sysname);
		}

		if (ioctl(S->fd, (S->flags & F_SLIP) ? SLIOCGUNIT : PPPIOCSUNIT,
		    &S->interface) < 0) {
			switch (errno) {
			case ENXIO:
				warnx("no interface");
				break;
			case EBUSY:
				warnx("interface is busy");
				break;
			default:
				warn("%s: assigning %s unit", S->sysname,
				    (S->flags & F_SLIP) ? "SLIP" : "PPP");
				break;
			}
			disconnect(S);

			if (!dialin && dodaemon) {
				if (S->nextdst == NULL || *S->nextdst == NULL)
					sleep(60);
				goto daemon_loop;
			}
			exit(1);
		}
	}

	/*
	 * At this point the following should be true:
	 *   1) We know the interface number.
	 *   2) PPP or SLIP line discipline is set.
	 *
	 * If this is a dialup connection now is the time to set the address
	 * on the interface in case the other end wants us to do the
	 * IP address assignment.  So we would like the interface to be
	 * down at this point -- I'm not sure that that will be true.
	 */

	if (async)
		snprintf(S->ifname, sizeof(S->ifname), "%s%d",
		    (S->flags & F_SLIP) ? "sl" : "ppp", S->interface);
	else
		strncpy(S->ifname, S->device, sizeof(S->ifname));
	_ppp.ppp_ifname = S->ifname;
	_ppp.ppp_sysname = S->sysname;
	S->pifname[0] = '\0';
	_ppp.ppp_pifname = S->pifname;
	_ppp.ppp_dl.sdl_index = 0;

	if (async)
		updateutmp(S, "");

	/*
	 * Wait for the session to come up.
	 * Needs to happen after interface goes up.
	 */
	if (S->flags & F_SLIP) {
		/* SLIP just starts up */;
		ppp_openskt();
		if ((!(dodaemon && (S->flags & F_IMMEDIATE) == 0)) &&
		    startup(S, S->sysname))
			goto timeout;
	} else if (ppp_start(S->private) == 0) {
		warn("while waiting for session to start");
		syslog(LOG_INFO, "%s: did not negotiate session on %s\n",
		    S->sysname, S->ifname);
		if (S->linkfailed)
			loginscript(S, S->linkfailed);
		else if (!dodaemon && !S->linkdown)
			default_down(S);
		goto timeout;
	}

	if (async) {
		updateutmp(S, 0);
		if (needwtmp) {
			needwtmp = 0;
			if (!(S->flags & F_NOLASTLOG))
				updatelastlog(S);
			updatewtmp(S);
		}
	}

	syslog(LOG_INFO, "%s: connected on %s%s%s\n", S->sysname, S->ifname,
		S->pifname[0] ? "/" : "", S->pifname[0] ? S->pifname : "");

	/*
	 * Run link up script.
	 */
	amup = 1;
	if (S->linkup && linkscript(S, S->linkup, 1)) {
		warnx("%s: up script failed", S->sysname);
		syslog(LOG_WARNING, "%s: up script failed\n", S->sysname);
	} else {
		/*
		 * wait until end of session
		 */
		if (background) {
			switch (pid = fork()) {
			case -1:
				warnx("failed to fork while attempting to put"
				    "the session in the background");
				syslog(LOG_WARNING, "%s: couldn't fork: %m\n",
				    S->sysname);
				break;
			default:
				exit(0);		/* Parent exits. */
			case 0:
				/*
				 * Child continues in background.
				 */
				uprintf("Continue in background.\n");
				setsid();
				chdir("/");
				break;
			}
		}

		if (S->flags & F_SLIP)
			(void) ioctl(S->fd, SLIOCWEOS, 0);
		else
			ppp_waitforeos(S->private);
	}

	uprintf("Restore & close line\n");
timeout:
	disconnect(S);
	syslog(LOG_INFO, "%s: end of session\n", S->sysname);

	if (dodaemon)
		goto daemon_loop;

	/*
	 * Run link down script.
	 */
	if (amup && S->linkdown && linkscript(S, S->linkdown, 0)) {
		warnx("%s: down script failed", S->sysname);
		syslog(LOG_WARNING, "%s: down script failed\n", S->sysname);
	} else if (amup && !S->linkdown)
		default_down(S);

	exit(0);
}

int
startup(session_t *S, char * _sysname)
{
	char dst[SLIP_ADDRLEN];

	S->sysname = _sysname;	/* passed in from pap/chap */
	if (async && S->fd >= 0)
		updateutmp(S, "");

	dprintf(D_DEBUG, "startup(%s%s%s, %s)\n", S->ifname,
	    S->pifname[0] ? "/" : "", S->pifname[0] ? S->pifname : "",
	    _sysname);

	if (S->linkinit && loginscript(S, S->linkinit)) {
		warnx("%s: link-init script failed", S->sysname);
		syslog(LOG_WARNING, "%s: link-init script failed\n",
		    S->sysname);
		if (S->linkfailed)
			loginscript(S, S->linkfailed);
		else if (!S->linkdown)
			default_down(S);
		return(1);
	} else if (!S->linkinit && default_init(S)) {
		warnx("%s: default link-init script failed", S->sysname);
		syslog(LOG_WARNING, "%s: default link-init script failed\n",
		    S->sysname);
		if (S->linkfailed)
			loginscript(S, S->linkfailed);
		else if (!S->linkdown)
			default_down(S);
		return(1);
	}

	/*
	 * Load interface and PPP parameters
	 */
	if (S->flags & F_SLIP) {
		struct in_addr ia;

		memset(dst, 0, sizeof(dst));
		snprintf(dst, SLIP_ADDRLEN, "\r\n");

		memset(&ia, 0, sizeof(ia));
		if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, SRC_ADDR)) != NULL)
			snprintf(&dst[strlen(dst)], SLIP_ADDRLEN - strlen(dst),
			    "Server address is %s\n", inet_ntoa(ia));
		else
			warn("failed to get local IP address for %s%s%s",
			    S->ifname, S->pifname[0] ? "/" : "",
			    S->pifname[0] ? S->pifname : "");

		memset(&ia, 0, sizeof(ia));
		if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, DST_ADDR)) != NULL)
			snprintf(&dst[strlen(dst)], SLIP_ADDRLEN - strlen(dst),
			    "Your address is %s\n", inet_ntoa(ia));
		else
			warn("failed to get remote IP address for %s%s%s",
			    S->ifname, S->pifname[0] ? "/" : "",
			    S->pifname[0] ? S->pifname : "");

		ioctl(S->fd, SLIOCSENDADDR, dst);
	}
	return(0);
}

static int expect_interrupted;

/* Timeout signal handler */
static void
expalarm()
{
	expect_interrupted = 1;
}

/*
 * Look up for an expected string or until the timeout is expired
 * Returns 0 if successfull.
 */
int
expect(fd, str, timo)
	int fd;
	char *str;
	int timo;
{
	struct sigaction act, oact;
	char buf[128];
	int nchars, expchars;
	char c;

	uprintf("Expect <%s> (%d sec)\nGot: <", printable(str), timo);
	nchars = 0;
	expchars = strlen(str);
	expect_interrupted = 0;
	act.sa_handler = expalarm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oact);
	alarm(timo);

	do {
		if (read(fd, &c, 1) != 1 || expect_interrupted) {
			alarm(0);
			sigaction(SIGALRM, &oact, 0);
			uprintf("> FAILED\n");
			return (1);
		}
		c &= 0177;
		if (debug & D_XDEB) {
			if (c < 040)
				uprintf("^%c", c | 0100);
			else if (c == 0177)
				uprintf("^?");
			else
				uprintf("%c", c);
		}
		buf[nchars++] = c;
		if (nchars > expchars)
			bcopy(buf+1, buf, --nchars);
	} while (nchars < expchars || bcmp(buf, str, expchars));
	alarm(0);
	sigaction(SIGALRM, &oact, 0);
	uprintf("> OK\n");
	return (0);
}

/*
 * Make string suitable to print out
 */
char *
printable(str)
	char *str;
{
	static char buf[128];
	char *p = buf;
	register int c;

	while ((c = *str++) != NULL) {
		c &= 0377;
		if (c < 040) {
			*p++ = '^';
			*p++ = c | 0100;
		} else if (c == 0177) {
			*p++ = '^';
			*p++ = '?';
		} else if (c & 0200) {
			*p++ = '\\';
			*p++ = '0' + (c >> 6);
			*p++ = '0' + ((c >> 3) & 07);
			*p++ = '0' + (c & 07);
		} else
			*p++ = c;
	}
	*p = 0;
	return (buf);
}

/*
 * Cleanup and exit after a signal.
 */
void
cleanup(int sig)
{
	session_t *S = current_session;	/* yuch */
	char *signame;

	switch (sig) {
	case SIGHUP:
		signame = "SIGHUP"; break;
	case SIGINT:
		signame = "SIGINT"; break;
	case SIGTERM:
		signame = "SIGTERM"; break;
	default:
		signame = "unknown"; break;
	}
	syslog(LOG_INFO, "%s: end of session, caught %s\n",
	    S->sysname, signame);
	if ((S->flags & F_SLIP) == 0 && S->private)
		ppp_shutdown(S->private);

	if (amup && S->linkdown != 0)
		linkscript(S, S->linkdown, 0);
	else if (amup)
		default_down(S);
	disconnect(S);
	exit(1);
}

/*
 * Run link up/down script.
 */

int
linkscript(session_t *S, char *path, int up)
{
	char dst[24], src[24];
	struct in_addr ia;
	int r;
	char *ifname;

	if (!up && pif_isup(&_ppp))
		return(0);
	ifname = S->pifname[0] ? S->pifname : S->ifname;

	dst[0] = 0;
	src[0] = 0;

	memset(&ia, 0, sizeof(ia));
	if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, SRC_ADDR)) != NULL)
		strncpy(src, inet_ntoa(ia), sizeof(src));
	else
		warn("failed to get source IP address for %s", ifname);

	if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, DST_ADDR)) != NULL )
		strncpy(dst, inet_ntoa(ia), sizeof(dst));
	else
		warn("failed to get remote IP address for %s", ifname);

	src[sizeof(src)-1] = 0;
	dst[sizeof(dst)-1] = 0;

	uprintf("Running \"%s %s %s %s %s\" . . . ", 
	    path, S->sysname, ifname, src, dst);
	/*
	 * XXX - there should be a timeout
	 */
	r = execute(S, 1, path, S->sysname, ifname, src, dst, 0);
	uprintf("done.\n");
	return(r);
}

/*
 * "down script" when none is provided
 */
int
default_down(session_t *S)
{
	char src[24];
	struct in_addr ia;
	int r;
	char *ifname;

	if (pif_isup(&_ppp))
		return(0);

	ifname = S->pifname[0] ? S->pifname : S->ifname;

	src[0] = 0;

	memset(&ia, 0, sizeof(ia));
	ia.s_addr = ppp_ipcp_getaddr(&_ppp, SRC_ADDR);
	strncpy(src, inet_ntoa(ia), sizeof(src));
	src[sizeof(src)-1] = 0;

	uprintf("Running \"%s %s %s %s %s %s %s %s\" . . . ", 
	    _PATH_IFCONFIG, ifname, "-alias", src, "down",
	    "-link0", "-link1", "-link2", 0);
	r = execute(S, 0, _PATH_IFCONFIG, ifname, "-alias", src, "down",
	    "-link0", "-link1", "-link2", 0);
	uprintf("done.\n");
	return(r);
}

/*
 * "init script" when none is provided
 */

#define	IFNAME(s) ((s)->pifname[0] ? (s)->pifname : (s)->ifname)

int
default_init(session_t *S)
{
	char src[24];
	char dst[24];
	struct in_addr ia;
	int r;
	ppp_t *ppp = (ppp_t *)S->private;

	if ((S->flags & F_SLIP) != 0 || !(ppp->ppp_us || ppp->ppp_them))
		return(0);

	src[0] = 0;

	memset(&ia, 0, sizeof(ia));
	ia.s_addr = ppp->ppp_us;
	strncpy(src, inet_ntoa(ia), sizeof(src));
	src[sizeof(src)-1] = 0;
	ia.s_addr = ppp->ppp_them;
	strncpy(dst, inet_ntoa(ia), sizeof(src));
	src[sizeof(dst)-1] = 0;

	uprintf("Running \"%s %s %s %s\" . . . ", 
	    _PATH_IFCONFIG, IFNAME(S), src, dst);
	r = execute(S, 0, _PATH_IFCONFIG, IFNAME(S), src, dst, 0);
	uprintf("done.\n");
	return(r);
}

/*
 * Run link up/down script.
 */
int
loginscript(session_t *S, char *path)
{
	int r;

	uprintf("Running \"%s %s %s\" . . . ", path, S->sysname, IFNAME(S));
	/*
	 * XXX - there should be a timeout
	 */
	r = execute(S, 1, path, S->sysname, IFNAME(S), 0);
	uprintf("done.\n");
	return(r);
}

void
updateutmp(session_t *S, char *str)
{
	struct utmp ut;
	struct in_addr ia;
	int fd;
	int tty;

	if (str == 0) {
		memset(&ia, 0, sizeof(ia));
		if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, DST_ADDR)) == 0) {
			warn("could not get remote IP address");
			return;
		}
		str = inet_ntoa(ia);
	}

	if ((tty = fttyslot(S->fd)) <= 0) {
		syslog(LOG_WARNING, "getting tty slot: %m\n");
		return;
	}
	if ((fd = open(_PATH_UTMP, O_RDWR)) < 0) {
		syslog(LOG_WARNING, "opening " _PATH_UTMP ": %m\n");
		return;
	}

	(void)lseek(fd, (off_t)(tty * sizeof(struct utmp)), L_SET);
	if (read(fd, &ut, sizeof(struct utmp)) != sizeof(struct utmp))
		memset(&ut, 0, sizeof(struct utmp));
	if (ut.ut_line[0] == 0) {
		char *t = ttyname(S->fd);
		if (t && (t = strrchr(t, '/')))
			++t;
		if (t)
			strncpy(ut.ut_line, t, sizeof(ut.ut_line));
	}
	if (ut.ut_name[0] == NULL || ut.ut_time == NULL)
		ut.ut_time = time(NULL);
	if (dialin)
		strncpy(ut.ut_name, S->sysname, sizeof(ut.ut_name));
	else {
		ut.ut_name[0] = '*';
		strncpy(ut.ut_name+1, S->sysname, sizeof(ut.ut_name)-1);
	}
	ut.ut_host[0] = 0;
	if (S->ifname[0]) {
		strncat(ut.ut_host, S->ifname, UT_HOSTSIZE);
		strncat(ut.ut_host, ":", UT_HOSTSIZE);
	}
	strncat(ut.ut_host, str, UT_HOSTSIZE);
	(void)lseek(fd, (off_t)(tty * sizeof(struct utmp)), L_SET);
	(void)write(fd, &ut, sizeof(struct utmp));
	(void)close(fd);
}

void
updatewtmp(session_t *S)
{
	struct in_addr ia;
	char *str;
	char *t;

	if ((t = ttyname(S->fd)) == NULL) {
		syslog(LOG_WARNING, "could not get tty name");
		return;
	}

	if ((t = strrchr(t, '/')) != NULL)
		++t;
		
	memset(&ia, 0, sizeof(ia));
	if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, DST_ADDR)) == 0) {
		syslog(LOG_WARNING, "could not get remote IP address");
		str = "";
	} else
		str = inet_ntoa(ia);

	logwtmp(t, S->sysname, str);
}

void
updatelastlog(session_t *S)
{
	struct in_addr ia;
	struct passwd *pwd;
	struct lastlog ll;
	char *hostname;
	char *tty;
	int fd;

	if ((pwd = getpwnam(S->sysname)) == NULL) {
		syslog(LOG_NOTICE,
		    "no passwd entry for %s, so no lastlog entry",S->sysname);
		return;
	}

	if ((tty = ttyname(S->fd)) == NULL) {
		syslog(LOG_WARNING, "could not get tty name");
		return;
	}

	if ((tty = strrchr(tty, '/')) != NULL)
		++tty;

	memset((void *)&ia, 0, sizeof(ia));
	if ((ia.s_addr = ppp_ipcp_getaddr(&_ppp, DST_ADDR)) == 0) {
		syslog(LOG_WARNING, "could not get remote IP address");
		hostname = NULL;
	} else
		hostname = inet_ntoa(ia);

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0) {
		(void)lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), L_SET);
		memset((void *)&ll, 0, sizeof(ll));
		(void)time(&ll.ll_time);
		(void)strncpy(ll.ll_line, tty, sizeof(ll.ll_line));
		if (hostname)
			(void)strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		(void)write(fd, (char *)&ll, sizeof(ll));
		(void)close(fd);
	}

}

/*
 * The code below is derived from lib/libc/stdlib/system.c
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 */

#define	MAXARGS	64
#define	MAXOPTS	(MAXARGS - 4)

int
execute(session_t *S, int useso, char *script, ...)
{
	va_list ap;
	int pstat;
	pid_t pid;
	int omask;
	sig_t intsave, quitsave;
	char *argv[MAXARGS];
    	char **av = argv;
	char **sov;

	if (S->linkoptions) {
		sov = S->linkopts;
		while (sov < &S->linkopts[MAXOPTS-1] &&
		    (*sov = strsep(&S->linkoptions, " \t")) != NULL)
			if (**sov != '\0')
				++sov;
		*sov = 0;
		S->linkoptions = 0;
	}

    	if ((*av = strrchr(script, '/')) != NULL)
		++*av;
	else
		*av = script;
	++av;

	if (useso) {
		sov = S->linkopts;
		while (av < &argv[MAXARGS-2] && (*av = *sov++))
			++av;
	}

	va_start(ap, script);
	while (av < &argv[MAXARGS-2] && (*av = va_arg(ap, char *)))
		++av;
	va_end(ap);

	*av = 0;

	omask = sigblock(sigmask(SIGCHLD));

	switch (pid = vfork()) {
	case -1:			/* error */
		(void)sigsetmask(omask);
		warn("%s", script);
		return(-1);
	case 0:				/* child */
		/*
		 * If we are running as root or not as the user,
		 * nuke the environment and swith to the "daemon"
		 * class.  This set the environment to just the PATH.
		 */
		if (suid == 0 || suid != getuid()) {
			extern char **environ;
			*environ = NULL;
			setclasscontext("daemon", LOGIN_SETALL);
		}
		if (sgid != -1)
			setgid(sgid);
		if (suid != -1)
			setuid(suid);
		(void)sigsetmask(omask);
		execv(script, argv);
		_exit(127);
	}

	intsave = signal(SIGINT, SIG_IGN);
	quitsave = signal(SIGQUIT, SIG_IGN);
	pid = waitpid(pid, &pstat, 0);
	(void)sigsetmask(omask);
	(void)signal(SIGINT, intsave);
	(void)signal(SIGQUIT, quitsave);

	return (pid == -1 ? -1 : WIFEXITED(pstat) ? WEXITSTATUS(pstat) : -1);
}

/*
 * Wait for a dropped packet on a ppp interface
 */
void
waitdrop(ifname)
	char *ifname;
{
	struct ifreq ifr;

	if (debug)
		printf("Wait for a packet on %s...", ifname);
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(skt, PPPIOCWAIT, (caddr_t)&ifr) < 0)
		err(1, "%s: waiting for packet on interface", ifname);
	if (debug)
		printf(" Got it.\n");
}

int
fttyslot(fd)
	int fd;
{
	register struct ttyent *ttyp;
	register int slot;
	register char *p;
	char *name;

	setttyent();
	if ((name = ttyname(fd)) != NULL) {
		if ((p = strrchr(name, '/')) != NULL)
			++p;
		else
			p = name;
		for (slot = 1; (ttyp = getttyent()) != NULL; ++slot)
			if (!strcmp(ttyp->ty_name, p)) {
				endttyent();
				return(slot);
			}
	}
	endttyent();
	return (0);
}
