/*
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI syslogd.c,v 2.21 2001/12/04 18:51:13 chrisk Exp
 */

/*
 * Copyright (c) 1983, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)syslogd.c	8.3 (Berkeley) 4/4/94";
#endif /* not lint */

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 * more extensive changes by Eric Allman (again)
 */

#include <sys/param.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/msgbuf.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>
#include <vis.h>

#include "pathnames.h"
#include "md5.h"

#define SYSLOG_NAMES
#include <sys/syslog.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#define	MAXMSG		MSG_BSIZE	/* Maximum message size we will receive */
#define	MAXLINE		1024		/* Maximum message size we will forward */
#define	BUFFER_SIZE	(MAXMSG * 8)
#define DEFUPRI		(LOG_USER|LOG_NOTICE)
#define DEFSPRI		(LOG_KERN|LOG_CRIT)
#define TIMERINTVL	30		/* interval for checking flush, mark */
#define	TTY_TIMEOUT	(5*60)		/* Timeout writing to ttys */
#define	TSTMP_SIZE	16		/* Length of time stamp, including nul */
#define	MD5_SIZE	16		/* Size of an MD5 digest */
#define	WHITESPACE	" \t"		/* For parsing whitespace */
#define	MARK_MSG	"-- MARK --"
#define	RESTART_MSG	": restart"

/* Default paths */
static const char *path_log = _PATH_LOG;
static const char *path_klog = _PATH_KLOG;
static const char *path_logconf = _PATH_LOGCONF;
static const char *path_logpid = _PATH_LOGPID;
static const char *path_console = _PATH_CONSOLE;
static const char *path_utmp = _PATH_UTMP;
static const char *path_messages = "/var/log/messages";

#define	dprintf(x)		do { if (Debug) printf x; } while (0)

/* Flags to logmsg(). */

#define IGN_CONS	0x001	/* don't print on console */
#define SYNC_FILE	0x002	/* do fsync on file after printing */
#define ADDDATE		0x004	/* add a date to the message */
#define MARK		0x008	/* this message is a mark */

/* Types of actions */
/* Order is important, entries are inserted in list by type */
enum f_types {
	F_UNUSED,		/* unused entry */
	F_FILE,			/* regular file */
	F_WALL,			/* everyone logged on */
	F_CONSOLE,		/* console terminal */
	F_TTY,			/* terminal */
	F_USER,			/* a logged in user */
	F_FORW			/* remote machine */
};
static const char *TypeNames[] = {
	"UNUSED",
	"FILE",
	"WALL",
	"CONSOLE",
	"TTY",
	"USER",
	"FORW"
};


/*
 * This structure represents the files that will have log
 * copies printed.
 */

struct filed {
	int	f_file;			/* file descriptor */
	enum	f_types f_type;		/* entry type, see below */
	time_t	f_time;			/* time this was last written */
	int	f_warning;		/* Warning message issued */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	const	char *f_name;		/* User, host or file name */
	struct	sockaddr_in f_addr;	/* Address for F_FORW */
	char	f_prevdigest[MD5_SIZE];		/* digest of last message logged */
	char	f_lasttime[TSTMP_SIZE];		/* time of last occurrence */
	char	*f_prevhost;			/* host from which recd. */
	int	f_prevpri;			/* pri of f_prevline */
	u_int	f_prevlen;			/* length of f_prevline */
	u_int	f_prevcount;			/* repetition cnt of prevline */
	u_int	f_repeatcount;			/* number of "repeated" msgs */
	TAILQ_ENTRY(filed) f_link;
};
static TAILQ_HEAD(filed_head, filed) Files;
#define	FILED_LIST(fhp, f) \
	for ((f) = TAILQ_FIRST(fhp); (f) != TAILQ_END(fhp); \
	       (f) = TAILQ_NEXT(f, f_link))

/*
 * Intervals at which we flush out "message repeated" messages,
 * in seconds after previous message is logged.  After each flush,
 * we move to the next interval until we reach the largest.
 */
static int repeatinterval[] = { 30, 120, 600 };	/* # of secs before flush */
#define	MAXREPEAT ((sizeof(repeatinterval) / sizeof(repeatinterval[0])) - 1)
#define	REPEATTIME(f)	((f)->f_time + repeatinterval[(f)->f_repeatcount])
#define	BACKOFF(f)	{ if (++(f)->f_repeatcount > MAXREPEAT) \
				 (f)->f_repeatcount = MAXREPEAT; \
			}

static int	Debug;			/* debug flag */
static char	*LocalHostName;		/* our hostname */
static const	char *LocalDomain;	/* our local domain name */
static const	char *progname;		/* Our program name */
static int	inet_fd;		/* Internet datagram socket */
static int	LogPort;		/* port number for INET connections */
static int	MarkInterval = 20 * 60;	/* interval between marks in seconds */
static int	MarkSeq = 0;		/* mark sequence number */
static time_t	TimeNow;		/* Current time */
static sigset_t	block_set;		/* Used to block signals while logging */
static int	nflag;			/* Do not look up host names */
static CODE	wildcode;		/* Used for detecting wildcards */

static void	cfline __P((struct filed_head *, char *, int));
static const	char *cvthname __P((struct sockaddr_in *));
static CODE	*decode __P((const char *, CODE *));
static void	die __P((int));
static void	debug_filed __P((struct filed *));
static void	domark __P((int));
static void	filed_init __P((void));
static void	fprintlog __P((struct filed *, int, const char *, 
		    const char *, size_t));
static void	logerror __P((int, const char *, ...));
static void	logmsg __P((int, const char *, const char *, size_t,
		    const char *, int));
static void	printline __P((struct msghdr *, size_t));
static void	printsys __P((char *, size_t));
static void	read_config __P((int));
static void	reapchild __P((int));
static void	usage __P((void));
static void	wallmsg __P((struct filed *, struct iovec *, int));

extern char	*ttymsg __P((struct iovec *, int, char *, int));

int
main(int argc, char *argv[])
{
	struct itimerval itv;
	struct servent *sp;
	struct sigaction sigact;
	struct sockaddr_un slocal;
	struct sockaddr_in sinet;
	struct msghdr msghdr;
	struct iovec iov;
	fd_set modelfds;
	sigset_t oset;
	size_t len;
	int i;
	int ch;
	int klog_fd;
	int iflag;
	int lflag;
	int local_fd;
	int max_fd;
	int pid_fd;
	char *p;
	char line[MAXMSG + 1];
	u_char cmsg[CMSG_SPACE(sizeof(struct fcred))];


	iflag = lflag = 0;

	if ((p = strrchr(argv[0], '/')) != NULL)
		progname = p + 1;
	else
		progname = argv[0];

	while ((ch = getopt(argc, argv, "df:ilm:np:")) != EOF)
		switch(ch) {
		case 'd':		/* debug */
			Debug++;
			break;
		case 'f':		/* configuration file */
			path_logconf = optarg;
			break;
		case 'i':		/* insecure, don't log credentials */
			iflag = 1;
			break;
		case 'l':		/* local connections only */
			lflag = 1;
			break;
		case 'm':		/* mark interval */
			MarkInterval = atoi(optarg) * 60;
			break;
		case 'n':		/* do not lookup hostnames */
			nflag = 1;
			break;
		case 'p':		/* path */
			path_log = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		usage();

	if (Debug)
		(void)setvbuf(stdout, NULL, _IOLBF, 0);

	/* Initialize the list */
	filed_init();

	/* Get hostname and set local domain */
	if (gethostname(line, sizeof(line)) < 0) {
		strcpy(line, "localhost");
		LocalDomain = "";
		logerror(errno, "unable to get local hostname");
	} else if ((p = strchr(line, '.')) != NULL) {
		*p++ = 0;
		LocalDomain = strdup(p);
	} else
		LocalDomain = "";
	LocalHostName = strdup(line);

	/* Init wildcard code */
	wildcode.c_name = "*";
	wildcode.c_val = INTERNAL_NOPRI;

	/* Init signal block set and block signals */
	(void)sigemptyset(&block_set);
	(void)sigaddset(&block_set, SIGHUP);
	(void)sigaddset(&block_set, SIGALRM);
	if (sigprocmask(SIG_BLOCK, &block_set, &oset) < 0) {
		logerror(errno, "unable to set signal mask");
		exit(1);
	}

	/* Setup signals */
	sigact.sa_mask = block_set;	/* struct copy */
	sigact.sa_flags = SA_NOCLDSTOP;
	sigact.sa_handler = reapchild;
	if (sigaction(SIGCHLD, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGCHLD");
		exit(1);
	}

	sigact.sa_flags = 0;
	sigact.sa_handler = die;
	if (sigaction(SIGTERM, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGTERM");
		exit(1);
	}
	if (!Debug)
		sigact.sa_handler = SIG_IGN;
	if (sigaction(SIGINT, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGINT");
		exit(1);
	}
	if (sigaction(SIGQUIT, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGQUIT");
		exit(1);
	}
	sigact.sa_handler = read_config;
	if (sigaction(SIGHUP, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGHUP");
		exit(1);
	}
	sigact.sa_handler = domark;
	if (sigaction(SIGALRM, &sigact, NULL) < 0) {
		logerror(errno, "unable to set handler for SIGALRM");
		exit(1);
	}

	if (!Debug && daemon(0, 0) < 0) {
		logerror(errno, "unable to become a daemon");
		exit(1);
	}

	/* Initialize timer */
	itv.it_interval.tv_sec = TIMERINTVL;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = TIMERINTVL;
	itv.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &itv, NULL) < 0) {
		logerror(errno, "unable to set interval timer");
		exit(1);
	}

	/* Open and lock the PID file */
	pid_fd = open(path_logpid, O_RDWR | O_CREAT, 0644);
	if (pid_fd < 0) {
		logerror(errno, "unable to open/create %s",
			 path_logpid);
		exit(1);
	}
	/* If we can lock it we are the only one running */
	if (flock(pid_fd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)
			logerror(0, "unable to lock %s, is %s already running?",
			     path_logpid,
			     progname);
		else
			logerror(errno, "unable to lock %s",
				 path_logpid);
		exit(1);
	}
	/* Write my PID and leave file open to maintain the lock */
	len = snprintf(line, sizeof(line), "%ld", (long)getpid());
	if (len > sizeof(line))
		len = sizeof(line);
	if (ftruncate(pid_fd, (off_t) 0) < 0 ||
	    (size_t)write(pid_fd, line, len) != len) {
		logerror(errno, "unable to write %s",
			 path_logpid);
		exit(1);
	}

	/* Remove old socket file */
	if (unlink(path_log) < 0 && errno != ENOENT) {
		logerror(errno, "unable to remove %s", path_log);
		die(0);
	}

	/* Open connections */
	max_fd = -1;
	FD_ZERO(&modelfds);
	
	/* Open local domain socket */
	memset(&slocal, 0, sizeof(slocal));
	slocal.sun_family = AF_LOCAL;
	(void)strncpy(slocal.sun_path, path_log, sizeof(slocal.sun_path));
	slocal.sun_len = SUN_LEN(&slocal);
	local_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (local_fd < 0 ||
	    bind(local_fd, (struct sockaddr *)&slocal, slocal.sun_len) < 0 ||
	    chmod(path_log, 0666) < 0) {
		logerror(errno, "unable to create %s",
			 path_log);
		die(0);
	}
	len = BUFFER_SIZE;
	i = setsockopt(local_fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
	if (i < 0) {
		logerror(errno, "unable to set receive buffer size on %s",
			 path_log);
	}
	if (!iflag &&
	    setsockopt(local_fd, 0, LOCAL_CREDS, &len, sizeof(len)) != 0) {
		logerror(errno, "unable to require credentials on %s",
		    path_log);
		die(0);
	}
	max_fd = MAX(max_fd, local_fd);
	FD_SET(local_fd, &modelfds);

	inet_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (inet_fd >= 0) {
		sp = getservbyname("syslog", "udp");
		if (sp == NULL) {
			logerror(0, "syslog/udp: unknown service");
			die(0);
		}
		/* If local only, disable packet reception */
		if (lflag && shutdown(inet_fd, 0) < 0) {
			logerror(errno, "shutdown for reading");
			die(0);
		}
		memset(&sinet, 0, sizeof(sinet));
		sinet.sin_len = sizeof(sinet);
		sinet.sin_family = AF_INET;
		sinet.sin_port = LogPort = sp->s_port;
		if (bind(inet_fd, (struct sockaddr *)&sinet, sinet.sin_len) < 0) {
			logerror(errno, "bind to %s port",
				 sp->s_name);
			if (Debug) {
				(void)close(inet_fd);
				inet_fd = -1;
			} else
				die(0);
		} 
		if (!lflag && inet_fd != -1) {
			len = BUFFER_SIZE;
			i = setsockopt(inet_fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
			if (i < 0) {
				logerror(errno, "unable to set receive buffer size on %s",
					 path_log);
			}
			max_fd = MAX(max_fd, inet_fd);
			FD_SET(inet_fd, &modelfds);
		}
	} else
		sp = NULL;

	/* Open kernel log device */
	if ((klog_fd = open(path_klog, O_RDONLY, 0)) < 0) {
		logerror(errno, "can not open %s", path_klog);
		if (!Debug)
			die(0);
	} else {
		FD_SET(klog_fd, &modelfds);
		max_fd = MAX(max_fd, klog_fd);
	}

	dprintf(("off & running....\n"));

	/* Read the configuration file */
	read_config(0);

	/* Unblock signals */
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);

	/* Setup the I/O structures */
	iov.iov_base = line;
	iov.iov_len = sizeof(line);

	for (;;) {
		int nfds;
		fd_set readfds;

		readfds = modelfds;	/* struct copy */
		nfds = select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if (nfds == 0)
			continue;
		if (nfds < 0) {
			if (errno != EINTR)
				logerror(errno, "select");
			continue;
		}

		dprintf(("got a message (%d)\n", nfds));
		if (klog_fd != -1 && FD_ISSET(klog_fd, &readfds)) {
			/* Receive message from the kernel logging facility */

			i = read(klog_fd, line, sizeof(line));
			if (i > 0) {
				printsys(line, i);
			} else if (i < 0 && errno != EINTR) {
				logerror(errno, "unable to read from %s",
					 path_klog);
				(void)close(klog_fd);
				FD_CLR(klog_fd, &modelfds);
				klog_fd = -1;
			}
		}
		
		if (local_fd != -1 && FD_ISSET(local_fd, &readfds)) {
			/* Receive message from /var/run/log */

			memset(cmsg, 0, sizeof(cmsg));
			memset(&msghdr, 0, sizeof(msghdr));
			msghdr.msg_iov = &iov;
			msghdr.msg_iovlen = 1;
			msghdr.msg_flags = 0;
			if (!iflag) {
				msghdr.msg_control = (caddr_t)cmsg;
				msghdr.msg_controllen = sizeof(cmsg);
			}
			i = recvmsg(local_fd, &msghdr, 0);
			if (i > 0)
				printline(&msghdr, i);
			else if (i < 0 && errno != EINTR)
				logerror(errno, "unable to read from %s",
					 path_log);
		}
		
		if (inet_fd != -1 && FD_ISSET(inet_fd, &readfds)) {
			/* Receive a message from the network */

			memset(&msghdr, 0, sizeof(msghdr));
			msghdr.msg_iov = &iov;
			msghdr.msg_iovlen = 1;
			msghdr.msg_name = (caddr_t)&sinet;
			msghdr.msg_namelen = sizeof(sinet);
			i = recvmsg(inet_fd, &msghdr, 0);
			if (i > 0)
				printline(&msghdr, i);
			else if (i < 0 && errno != EINTR)
				logerror(errno, "unable to read from %s port",
					 sp->s_name);
		} 
	}
}

static void
usage(void)
{
	(void)fprintf(stderr,
		      "usage: %s "
		      "[-iln] [-f conf_file] [-m mark_interval] [-p log_socket]\n",
		      progname);
	exit(1);
}

/*
 * Parse beginning of message looking for <pri>.  Return 1 and set
 * *ppri and *msg if valid, return 0 if not.
 */
static int
parse_pri(char **msg, int *ppri)
{
	long pri;
	char *p;

	p = *msg;
	if (*p != '<')
		return (0);
	errno = 0;
	pri = strtol(++p, &p, 10);
	if ((errno == ERANGE && (pri == LONG_MIN || pri == LONG_MAX)) ||
	    *p != '>')
		return (0);
	*msg = p + 1;
	*ppri = pri;
	return 1;
}


/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */
static void
printline(struct msghdr *mp, size_t msglen)
{
	struct sockaddr_in *sinet;
	struct cmsghdr *cmsg;
	struct fcred *fc;
	int len;
	int pri;
	char *p, *pp, *msg;
	const char *hname;
	char line[(MAXMSG * 4) + 1];
	char prefix[1024];

	msg = mp->msg_iov[0].iov_base;

	/* Get the host name to use */
	if ((sinet = (struct sockaddr_in *)mp->msg_name) != NULL && 
	    sinet->sin_family == AF_INET)
		hname = cvthname(sinet);
	else
		hname = LocalHostName;

	/* Remove a trailing null */
	if (msglen > 0 && msg[msglen - 1] == 0)
		msglen--;

	/* Remove a trailing newline */
	if (msglen > 1 && msg[msglen - 1] == '\n')
		msglen--;

	/* Parse facility if present, otherwise set default */
	p = msg;
	if (!parse_pri(&p, &pri) || (pri & ~(LOG_FACMASK|LOG_PRIMASK)))
		pri = DEFUPRI;
	else {
		switch (pri & LOG_FACMASK) {
		case LOG_KERN:
		case LOG_SYSLOG:
			/* user-mode processes can not do this */
			pri = DEFUPRI;
			break;

		default:
			break;
		}
	}

	*(pp = prefix) = 0;
	if (mp->msg_flags & MSG_TRUNC)
		pp += snprintf(pp, sizeof(prefix) - (pp - prefix),
		    "[truncated]");

	if ((cmsg = (struct cmsghdr *)mp->msg_control) != NULL) {
		if (mp->msg_flags & MSG_CTRUNC || 
		    cmsg->cmsg_type != SCM_CREDS) {
			if (pp != prefix)
				*pp++ = ' ';
			pp += snprintf(pp, sizeof(prefix) - (pp - prefix),
			    "[credentials unavailable] ");
		} else {
			fc = (struct fcred *)CMSG_DATA(cmsg);

			if (fc->fc_ruid != 0 && fc->fc_uid != 0 &&
			    strcmp(fc->fc_login, "root") != 0) {
				if (pp != prefix)
					*pp++ = ' ';
				if (fc->fc_ruid != fc->fc_uid)
					pp += snprintf(pp, 
					    sizeof(prefix) - (pp - prefix),
					    "%s[%u,%u]: ", fc->fc_login,
					    fc->fc_ruid, fc->fc_uid);
				else
					pp += snprintf(pp,
					    sizeof(prefix) - (pp - prefix),
					    "%s[%u]: ", fc->fc_login,
					    fc->fc_ruid);
			}
		}
	}

	len = strvisx(line, p, msg + msglen - p, VIS_NL|VIS_CSTYLE|VIS_OCTAL);

	logmsg(pri, prefix, line, len, hname, 0);
}

/*
 * Take a raw input line from /dev/klog, split and format similar to
 * syslog().  Kernel provides lines optionally prefixed with priority
 * in angle brakcets (<pri>).  A read may return multiple lines,
 * seperated by a newline.
 */
static void
printsys(char *msg, size_t msglen)
{
	int pri;
	int flags;
	char *p, *q;
	char *lp;

	lp = msg + msglen;
	for (p = msg; p < lp; p = q + 1) {
		flags = SYNC_FILE | ADDDATE;	/* fsync file after write */
		if (parse_pri(&p, &pri)) {
			if (pri & ~(LOG_FACMASK|LOG_PRIMASK))
				pri = DEFSPRI;
		} else {
			/*
			 * No priority specification means it is a
			 * kernel printf and it has already been sent
			 * to the console.
			 */
			flags |= IGN_CONS;
			pri = DEFSPRI;
		}
		q = memchr(p, '\n', lp - p);
		if (q == NULL)
			q = lp;
		logmsg(pri, "kernel: ", p, q - p, LocalHostName, flags);
	}
}

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */
static void
logmsg(int pri, const char *prefix, const char *msg, size_t msglen,
       const char *from, int flags)
{
	struct filed *f;
	sigset_t oset;
	MD5_CTX md5;
	int fac, host_diff, prilev;
	char digest[MD5_SIZE];
	char timestamp[TSTMP_SIZE];

	if (Debug) {
		CODE *fn, *pn;

		for (fn = facilitynames; fn->c_name != NULL; fn++)
			if ((pri & LOG_FACMASK) == fn->c_val)
				break;

		for (pn = prioritynames; pn->c_name != NULL; pn++)
			if (LOG_PRI(pri) == pn->c_val)
				break;

		printf("logmsg: %s.%s, flags %x, from %s, prefix %s msg(%d) %.*s\n",
		    fn->c_name, pn->c_name, flags, from, prefix, msglen,
		    (int)msglen, msg);
	}

	(void)sigprocmask(SIG_BLOCK, &block_set, &oset);

	/*
	 * Check to see if msg looks non-standard.
	 */
	if (msglen < TSTMP_SIZE || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	TimeNow = time(NULL);
	if (flags & ADDDATE)
		strftime(timestamp, sizeof(timestamp), "%h %e %T",
			 localtime(&TimeNow));
	else {
		strncpy(timestamp, msg, TSTMP_SIZE);
		msg += TSTMP_SIZE;
		msglen -= TSTMP_SIZE;
	}

	/* extract facility and priority level */
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	else
		fac = LOG_FAC(pri);
	prilev = LOG_PRI(pri);

	/* Calculate MD5 digest for duplicate detection */
	MD5Init(&md5);
	if (prefix != NULL)
		MD5Update(&md5, prefix, strlen(prefix)); 
	MD5Update(&md5, msg, msglen); 
	MD5Final(digest, &md5);

	/* Search for actions based on facility and priority */
	FILED_LIST(&Files, f) {
		/* Skip messages that are incorrect priority */
		if (f->f_pmask[fac] < prilev ||
		    f->f_pmask[fac] == INTERNAL_NOPRI)
			continue;

		/* Kernel printfs have already been sent to the console */
		if (f->f_type == F_CONSOLE && (flags & IGN_CONS))
			continue;

		/* Don't output marks to recently written files */
		if ((flags & MARK) && (TimeNow - f->f_time) < MarkInterval / 2)
			continue;

		/*
		 * suppress duplicate lines to this file
		 */
		host_diff = from != f->f_prevhost &&
		    (f->f_prevhost == NULL ||
		     strcasecmp(from, f->f_prevhost) != 0);
		(void)memcpy(f->f_lasttime, timestamp, sizeof(f->f_lasttime) - 1);
		if ((flags & MARK) == 0 &&
		    msglen == f->f_prevlen &&
		    memcmp(digest, f->f_prevdigest, sizeof(digest)) == 0 &&
		    !host_diff) {
			f->f_prevcount++;
			dprintf(("msg repeated %u times, %ld sec of %d\n",
				 f->f_prevcount,
				 (long)(TimeNow - f->f_time),
				 repeatinterval[f->f_repeatcount]));
			/*
			 * If domark would have logged this by now,
			 * flush it now (so we don't hold isolated messages),
			 * but back off so we'll flush less often
			 * in the future.
			 */
			if (TimeNow > REPEATTIME(f)) {
				fprintlog(f, flags, NULL, NULL, 0);
				BACKOFF(f);
			}
		} else {
			/* new line, save it */
			if (f->f_prevcount != 0)
				fprintlog(f, 0, NULL, NULL, 0);
			f->f_repeatcount = 0;
			if (host_diff) {
				if (f->f_prevhost != NULL &&
				    f->f_prevhost != LocalHostName)
					free(f->f_prevhost);
				if (from == LocalHostName)
					f->f_prevhost = LocalHostName;
				else
					f->f_prevhost = strdup(from);
			}
			f->f_prevpri = pri;
			f->f_prevlen = msglen;
			(void)memcpy(f->f_prevdigest, digest, sizeof(f->f_prevdigest));
			fprintlog(f, flags, prefix, msg, msglen);
		}
	}
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}

static void
fprintlog(struct filed *f, int flags, const char *prefix, const char *msg, 
	  size_t msglen)
{
	struct iovec *v, *v1;
	struct iovec *iov_msg;
	struct iovec iov[7];
	struct msghdr msghdr;
	int e;
	int maxline;
	int size;
	char repbuf[80];
	char greetings[200];

	e = 0;
	v = iov;
	switch (f->f_type) {
	case F_WALL:
		v->iov_len = snprintf(greetings, sizeof(greetings),
				      "\r\n\7"
				      "Message from %s@%s at %.24s ...\r\n",
				      progname,
				      f->f_prevhost,
				      ctime(&TimeNow));
		v->iov_base = greetings;
		v++;
		break;

	case F_FORW:
		v->iov_len = snprintf(greetings, sizeof(greetings),
				      "<%d>", f->f_prevpri);
		v->iov_base = greetings;
		v++;
		/* Fall through */
		
	default:
		v->iov_base = f->f_lasttime;
		v->iov_len = strlen(v->iov_base);
		v++;
		v->iov_base = (char *)" ";
		v->iov_len = 1;
		v++;
		break;
	}

	if (f->f_type != F_FORW) {
		v->iov_base = f->f_prevhost;
		v->iov_len = strlen(v->iov_base);
		v++;
		v->iov_base = (char *)" ";
		v->iov_len = 1;
		v++;
	}

	if (msg != NULL) {
		if (prefix != NULL) {
			v->iov_base = (char *)prefix;
			v->iov_len = strlen(prefix);
			v++;
		}
		v->iov_base = (char *)msg;
		v->iov_len = msglen;
	} else {
		v->iov_len = snprintf(repbuf, sizeof(repbuf),
				      "last message repeated %d times",
				      f->f_prevcount);
		v->iov_base = repbuf;
	}
	iov_msg = v;
	v++;

	dprintf(("Logging to %s", TypeNames[f->f_type]));
	f->f_time = TimeNow;

	switch (f->f_type) {
	case F_FORW:
		dprintf((" %s", f->f_name));
		if (inet_fd == -1)
			break;
		/* Resolve hostname if necessary */
		if (f->f_addr.sin_family == AF_UNSPEC) {
			struct hostent *hp;
			
			hp = gethostbyname(f->f_name);
			if (hp == NULL) {
				if (f->f_warning == 0) {
					f->f_warning++;
					logerror(0,
						 "unable to resolve %s: %s",
						 f->f_name,
						 hstrerror(h_errno));
				}
				break;
			}
			f->f_addr.sin_family = hp->h_addrtype;
			memmove(&f->f_addr.sin_addr, hp->h_addr, hp->h_length);
			if (f->f_warning != 0) {
				f->f_warning = 0;
				logerror(0,
					 "%s resolved to %s",
					 f->f_name,
					 inet_ntoa(f->f_addr.sin_addr));
			}
		}

		memset(&msghdr, 0, sizeof(msghdr));
		msghdr.msg_iov = iov;
		msghdr.msg_iovlen = v - iov;
		msghdr.msg_name = (void *)&f->f_addr;
		msghdr.msg_namelen = f->f_addr.sin_len;

		/* Calculate maximum data size (excluding prefix) */
		maxline = MAXLINE;
		for (v1 = iov; v1 < v; v1++)
			if (v1 != iov_msg)
				maxline -= v1->iov_len;

		/* Send message, but don't exceed MAXSIZE */
		for (size = iov_msg->iov_len; size > 0; size -= maxline) {
			if (size > maxline)
				iov_msg->iov_len = maxline;
			else
				iov_msg->iov_len = size;
			dprintf((" %d", iov_msg->iov_len));
			if (sendmsg(inet_fd, &msghdr, 0) < 0) {
				if (f->f_warning++ == 0)
					logerror(errno, "sendmsg %s", f->f_name);
				else if (f->f_warning == 10) {
					f->f_warning = 0;
					f->f_addr.sin_family = AF_UNSPEC;
				}
			} else
				f->f_warning = 0;
			iov_msg->iov_base += maxline;
		}
		dprintf(("\n"));
		break;

	case F_CONSOLE:
		if (flags & IGN_CONS) {
			dprintf((" (ignored)\n"));
			break;
		}
		/* FALLTHROUGH */

	case F_TTY:
	case F_FILE:
		dprintf((" %s\n", f->f_name));
		if (f->f_type != F_FILE) {
			v->iov_base = (char *)"\r\n";
			v->iov_len = 2;
		} else {
			v->iov_base = (char *)"\n";
			v->iov_len = 1;
		}
		v++;
	again:
		if (writev(f->f_file, iov, v - iov) < 0) {
			/* Check for errors on TTY's due to loss of tty */
			if (e == 0 &&
			    (errno == EIO || errno == EBADF) &&
			    f->f_type != F_FILE) {
				e = errno;
				(void)close(f->f_file);
				f->f_file = open(f->f_name,
						 O_WRONLY|O_APPEND, 0);
				if (f->f_file < 0) {
					if (f->f_warning == 0) {
						f->f_warning++;
						logerror(errno,
							 "unable to re-open %s",
							 f->f_name);
					}
				} else
					goto again;
			} else if (errno != EAGAIN) {
				e = errno;
				(void)close(f->f_file);
				if (f->f_warning == 0) {
					f->f_warning++;
					logerror(e, "writing %s", f->f_name);
				}
			} else {
				dprintf(("Write to %s returned EAGAIN\n",
				    f->f_name));
			}
		} else {
			f->f_warning = 0;
			if (flags & SYNC_FILE)
				(void)fsync(f->f_file);
		}
		break;

	case F_USER:
		dprintf((" %s", f->f_name));
		/* Fall through */
		
	case F_WALL:
		dprintf(("\n"));
		v->iov_base = (char *)"\r\n";
		v->iov_len = 2;
		v++;
		wallmsg(f, iov, v - iov);
		break;

	case F_UNUSED:
		/* Should not happen */ 
		break;
	}
	f->f_prevcount = 0;
}

/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */
static void
wallmsg(struct filed *f, struct iovec *iov, int n)
{
	static int reenter;			/* avoid calling ourselves */
	struct utmp ut;
	FILE *uf;
	char *p;
	char line[sizeof(ut.ut_line) + 1];

	if (reenter++)
		return;
	if ((uf = fopen(path_utmp, "r")) == NULL) {
		logerror(errno, "unable to open %s",
			 path_utmp);
		reenter = 0;
		return;
	}
	/* NOSTRICT */
	while (fread(&ut, sizeof(ut), 1, uf) == 1) {
		if (ut.ut_name[0] == '\0')
			continue;
		(void)snprintf(line, sizeof(line), "%.*s",
			       sizeof(ut.ut_line),
			       ut.ut_line);
		if (f->f_type == F_WALL) {
			dprintf(("Wallmsg to line %s\n", line));
			if ((p = ttymsg(iov, n, line, TTY_TIMEOUT)) != NULL) {
				logerror(0, p);
			}
			continue;
		}
		/* should we send the message to this user? */
		if (strncmp(f->f_name, ut.ut_name, sizeof(ut.ut_name)) == 0) {
			dprintf(("Wallmsg to user %s on line %s\n",
				 f->f_name, line));
			if ((p = ttymsg(iov, n, line, TTY_TIMEOUT)) != NULL) {
				logerror(0, p);
			}
		}
	}
	(void)fclose(uf);
	reenter = 0;
}

static void
reapchild(int signo)
{

	for (;;)
		if (wait4(WAIT_ANY, NULL, WNOHANG, NULL) < 0 && errno != EINTR)
			break;
}

/*
 * Return a printable representation of a host address.
 */
static const char *
cvthname(struct sockaddr_in *f)
{
	struct hostent *hp;
	char *p;

	dprintf(("cvthname(%s)\n", inet_ntoa(f->sin_addr)));

	if (f->sin_family != AF_INET) {
		dprintf(("Malformed from address\n"));
		return ("???");
	}
	if (nflag)
		return (inet_ntoa(f->sin_addr));
	if ((hp = gethostbyaddr((char *)&f->sin_addr, sizeof(struct in_addr),
	    f->sin_family)) == NULL) {
		dprintf(("unable to resolve hostname for %s: %s\n",
		    inet_ntoa(f->sin_addr),
		    hstrerror(h_errno)));
		return (inet_ntoa(f->sin_addr));
	}
	if ((p = strchr(hp->h_name, '.')) && strcasecmp(p + 1, LocalDomain) == 0)
		*p = '\0';
	/* Check for spoofing of our hostname */
	if (strcasecmp(hp->h_name, LocalHostName) == 0)
		return (inet_ntoa(f->sin_addr));
	return (hp->h_name);
}

static void
domark(int signo)
{
	struct filed *f;

	TimeNow = time(NULL);
	MarkSeq += TIMERINTVL;
	if (MarkSeq >= MarkInterval) {
		logmsg(LOG_INFO, NULL, MARK_MSG, sizeof(MARK_MSG) - 1,
		       LocalHostName, ADDDATE|MARK);
		MarkSeq = 0;
	}

	FILED_LIST(&Files, f) {
		if (f->f_prevcount != 0 && TimeNow >= REPEATTIME(f)) {
			dprintf(("flush %s: repeated %d times, %d sec.\n",
				 TypeNames[f->f_type],
				 f->f_prevcount,
				 repeatinterval[f->f_repeatcount]));
			fprintlog(f, 0, NULL, NULL, 0);
			BACKOFF(f);
		}
	}
}

/*
 * Print syslogd errors some place.
 */
static void
#if __STDC__
logerror(int error, const char *fmt, ...)
#else
logerror(error, fmt, va_alist)
	int error;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char *p;
	char buf[LINE_MAX];

	p = buf + snprintf(buf, sizeof(buf), "%s: ", progname);
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	p += vsnprintf(p, BUFSIZ + buf - p, fmt, ap);
	va_end(ap);
	
	if (error)
		p += snprintf(p, sizeof(buf) + buf - p, ": %s",
			      strerror(errno));
	dprintf(("%s\n", buf));
	logmsg(LOG_SYSLOG|LOG_ERR, NULL, buf, p - buf, LocalHostName,
	    ADDDATE|SYNC_FILE);
}

static void
die(int signo)
{
	struct filed *f;

	FILED_LIST(&Files, f) {
		/* flush any pending output */
		if (f->f_prevcount != 0)
			fprintlog(f, 0, NULL, NULL, 0);
	}
	if (signo)
		logerror(0, "exiting on signal sig%s",
			 sys_signame[signo]);

	(void)unlink(path_log);
	(void)unlink(path_logpid);

	exit(signo == SIGTERM ? 0 : 1);
}


/* Free a filed structure and memory allocated to it */
static void
filed_free(struct filed *f)
{

	if (f->f_prevhost != NULL && f->f_prevhost != LocalHostName)
		free(f->f_prevhost);
		
	switch (f->f_type) {
	case F_FILE:
	case F_TTY:
	case F_CONSOLE:
		(void)close(f->f_file);
		/* Fall through */

	case F_FORW:
	case F_USER:
		free((char *)f->f_name);
		break;
			
	default:
		break;
	}

	free(f);
}

/* Search for an existing entry with this target and merge if found */
static int
filed_merge(struct filed_head *files, struct filed *f)
{
	struct filed *pf;
	int i;

	/* Scan the list for a duplicate file */
	FILED_LIST(files, pf) {
		if (pf->f_type != f->f_type)
			continue;

		switch (f->f_type) {
		case F_WALL:
			break;

		case F_FILE:
		case F_TTY:
		case F_CONSOLE:
		case F_USER:
			if (strcmp(pf->f_name, f->f_name) != 0)
				continue;
			break;

	        case F_FORW:
			if (strcasecmp(pf->f_name, f->f_name) != 0)
				continue;
			break;

		default:
			continue;
		}

		if (Debug) {
			printf("Old:\t");
			debug_filed(f);
			printf("New:\t");
			debug_filed(pf);
		}

		/* Merge the facilities */
		for (i = 0; i <= LOG_NFACILITIES; i++) {
			if (f->f_pmask[i] != INTERNAL_NOPRI &&
			    (pf->f_pmask[i] == INTERNAL_NOPRI ||
			    f->f_pmask[i] > pf->f_pmask[i]))
				pf->f_pmask[i] = f->f_pmask[i];
		}

		if (Debug) {
			printf("Merged:\t");
			debug_filed(pf);
		}

		return (1);
	}

	return (0);
}

/* Insert entry into list in order by type */
static void
filed_insert(struct filed_head *files, struct filed *f)
{
	struct filed *sf;

	if (TAILQ_FIRST(files) == TAILQ_END(files))
		TAILQ_INSERT_HEAD(files, f, f_link);
	else {
		FILED_LIST(files, sf) {
			if (f->f_type < sf->f_type) {
				TAILQ_INSERT_BEFORE(sf, f, f_link);
				return;
			}
		}
		TAILQ_INSERT_TAIL(files, f, f_link);
	}
}

/*
 *  Read the configuration file and update the configuration.
 */
static void
read_config(int signo)
{
	struct filed_head files;
	struct filed *f;
	FILE *cf;
	size_t len;
	int linenum;
	char *p, *q, *r;

	dprintf(("read_config\n"));

	/* Make a temporary list to hold the old configuration */
	TAILQ_INIT(&files);

	/* open the configuration file */
	if ((cf = fopen(path_logconf, "r")) == NULL) {
		char line[LINE_MAX];

		logerror(0, "unable to open %s: %s; using default configuration.",
			 path_logconf,
			 strerror(errno));
		dprintf(("unable to open %s\n", path_logconf));

		(void)snprintf(line, sizeof(line), "*.err\t%s",
			       path_console);
		cfline(&files, line, -1);
		(void)snprintf(line, sizeof(line), "*.panic\t*");
		cfline(&files, line, -2);
	} else {
		/*
		 *  Foreach line in the conf table, open that file.
		 */

		linenum = 0;
		while ((p = fgetln(cf, &len)) != NULL) {
			linenum++;

			q = p + len - 1;
			/* Convert trailing newline to terminating nul */
			if (*q == '\n')
				q--;
			q[1] = 0;

			/* Trim off leading whitespace */
			p += strspn(p, WHITESPACE);

			/* Remove comments */
			if ((r = strchr(p, '#')) != NULL) {
				*r = 0;
				q = r - 1;
			}

			/* Trim off trailing whitespace */
			while (q >= p && isblank(*q))
				q--;
			q[1] = 0;

			/* Ignore empty lines */
			if (*p == 0)
				continue;

			cfline(&files, p, linenum);
		}

		/* close the configuration file */
		(void)fclose(cf);
	}

	/* Free the old list */
	while ((f = TAILQ_FIRST(&Files)) != NULL) {
		/* flush any pending output */
		if (f->f_prevcount != 0)
			fprintlog(f, 0, NULL, NULL, 0);

		/* Remove from the list */
		TAILQ_REMOVE(&Files, f, f_link);

		/* Free it */
		filed_free(f);
	}

	/* Replace with the new list */
	Files = files;	/* struct copy */
	/* Fixup head pointer */
	if (TAILQ_FIRST(&Files) != TAILQ_END(&Files))
		TAILQ_FIRST(&Files)->f_link.tqe_prev = &TAILQ_FIRST(&Files);

	if (Debug)
		FILED_LIST(&Files, f) {
			debug_filed(f);
		}

	logmsg(LOG_SYSLOG|LOG_INFO,
	    progname, RESTART_MSG, sizeof(RESTART_MSG) - 1,
	    LocalHostName, ADDDATE);
	dprintf(("%s%s\n", progname, RESTART_MSG));
}

/* Parse one line from the configuration file */
static void
cfline(struct filed_head *files, char *line, int lno)
{
	struct filed *f, *pf;
	CODE *c;
	int i, pri;
	char *p, *q, *r;
	char *sels, *facs, *acts;

	dprintf(("cfline(%d: %s)\n", lno, line));

	/* Isolate seperators */
	p = line;
	if ((sels = strsep(&p, WHITESPACE)) == NULL || p == NULL) {
 noaction:
		logerror(0, "%s:%d: missing action",
			 path_logconf, lno);
		return;
	}

	/* Skip whitespace */
	acts = p + strspn(p, WHITESPACE);
	if (*acts == 0)
		goto noaction;

	dprintf(("cfline(%d, \"%s\", \"%s\")\n", lno, sels, acts));

	/* Allocate and initialize a filed structure */
	f = (struct filed *)calloc(1, sizeof(*f));
	if (sizeof(f->f_pmask[0]) == sizeof(u_char))
		(void)memset(f->f_pmask, INTERNAL_NOPRI, LOG_NFACILITIES);
	else
		for (i = 0; i <= LOG_NFACILITIES; i++)
			f->f_pmask[i] = INTERNAL_NOPRI;

	/* scan through the list of selectors */
	while ((p = strsep(&sels, ";")) != NULL && p != NULL) {
       		/* Catch Null selectors */
		if (*p == 0) {
			logerror(0, "%s:%d: null selector",
				 path_logconf, lno);
			continue;
		}

		/* Seperate Priority */
		q = p;
		if ((r = strsep(&q, ".")) == NULL || q == NULL) {
			logerror(0, "%s:%d: "
				 "malformed selector \"%s\" "
				 "(need facility.level)",
				 path_logconf, lno,
				 p);
			free(f);
			return;
		}

		/* Decode Priority */
		c = decode(q, prioritynames);
		if (c == NULL) {
			logerror(0, "%s:%d: "
				 "unknown level name \"%s\"",
				 path_logconf, lno,
				 q);
			free(f);
			return;
		}
		if (c == &wildcode) {
			pri = LOG_PRIMASK + 1;
		} else {
			pri = c->c_val;
		}

		/* Parse facilities */
		for (facs = p; (p = strsep(&facs, ",")) != NULL;) {

			/* Decode facility */
			c = decode(p, facilitynames);
			if (c == NULL) {
				logerror(0, "%s:%d: "
					 "unknown facility name \"%s\"",
					 path_logconf, lno,
					 p);
				free(f);
				return;
			}
			if (c == &wildcode) {
				for (i = 0; i <= LOG_NFACILITIES; i++)
					f->f_pmask[i] = pri;
			} else {
				f->f_pmask[LOG_FAC(c->c_val)] = pri;
			}
		}
	}

	/* interpret action */
	switch (*(p = acts)) {
	case '@':
		/* Save the hostname but defer the resolution until we */
		/* use it */
		f->f_name = strdup(++p);
		memset(&f->f_addr, 0, sizeof(f->f_addr));
		f->f_addr.sin_len = sizeof(f->f_addr);
		f->f_addr.sin_port = LogPort;
		f->f_type = F_FORW;
		break;

	case '/':
		if ((f->f_file = open(p, O_WRONLY|O_APPEND, 0)) < 0) {
			logerror(errno, "unable to open %s",
				 p);
			break;
		}
		f->f_name = strdup(p);
		if (strcmp(p, path_console) == 0) {
			f->f_type = F_CONSOLE;
			fcntl(f->f_file, F_SETFL, O_APPEND|O_NONBLOCK);
		} else if (isatty(f->f_file)) {
			f->f_type = F_TTY;
			fcntl(f->f_file, F_SETFL, O_APPEND|O_NONBLOCK);
		} else
			f->f_type = F_FILE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		pf = NULL;
		f->f_type = F_USER;
		while ((q = strsep(&p, "," WHITESPACE)) != NULL) {
			if (*q == 0)
				continue;
			/* Just use a pointer to the name for now */
			f->f_name = q;
			/* Merge into an existing entry if possible */
			if (filed_merge(files, f) == 0) {
				/* Put on the list, duplicate if necessary */
				pf = (struct filed *)malloc(sizeof(*pf));
				*pf = *f;	/* struct copy */
				pf->f_name = strdup(f->f_name);
				/* Insert in list */
				filed_insert(files, pf);
			}
		}
		free(f);
		return;
	}

	/* Merge an existing entry, or insert in list */
	if (filed_merge(files, f))
		filed_free(f);
	else
		filed_insert(files, f);
}

/* Decode a symbolic name to a numeric value */
static CODE *
decode(const char *name, CODE *c)
{
	long l;
	char *p;

	if (isdigit(*name)) {
		/* Numeric argument */

       		/* Convert and verify conversion */
		l = strtol(name, &p, 10);
		if ((errno == ERANGE &&
		     (l == LONG_MIN || l == LONG_MAX)) || *p != 0)
		    l = -1;

		/* Verify that this is a valid value */
		for (; c->c_name != NULL; c++)
			if (c->c_val == l)
				return (c);
	} else {
		/* Alpha argument */

		for (; c->c_name != NULL; c++)
			if (strcasecmp(name, c->c_name) == 0)
				return (c);

		if (strcasecmp(name, wildcode.c_name) == 0)
			return (&wildcode);
	}

	return ((CODE *) 0);
}

/*
 * Initialize the list with a dummy entries to facilitate the logging
 * of error messages during startup.
 */
static void
filed_init(void)
{
	struct filed *f;
	int i;
	const char *fname;

	TAILQ_INIT(&Files);
		
	f = (struct filed *)calloc(1, sizeof(*f));
	if (!Debug) {
		fname = path_console;
		f->f_type = F_CONSOLE;
		f->f_file = open(fname, O_WRONLY, 0);
	} else
		f->f_file = -1;

	if (f->f_file < 0) {
		fname = "stderr";
		f->f_type = F_FILE;
		f->f_file = dup(fileno(stderr));
	}

	if (f->f_file < 0) {
		free(f);
	} else {
		f->f_name = strdup(fname);

		for (i = 0; i <= LOG_NFACILITIES; i++)
			f->f_pmask[i] = LOG_ERR;
	
		TAILQ_INSERT_TAIL(&Files, f, f_link);
	}


	if (!Debug) {
		f = (struct filed *)calloc(1, sizeof(*f));
		f->f_type = F_FILE;
		f->f_file = open(path_messages, O_WRONLY|O_APPEND, 0);

		if (f->f_file < 0) {
			free(f);
		} else {
			f->f_name = strdup(path_messages);

			for (i = 0; i <= LOG_NFACILITIES; i++)
				f->f_pmask[i] = LOG_ERR;
	
			TAILQ_INSERT_TAIL(&Files, f, f_link);
		}
	}
}

/* Display filed for debugging */
static void
debug_filed(struct filed *f)
{
	int i;

	for (i = 0; i <= LOG_NFACILITIES; i++)
		if (f->f_pmask[i] == INTERNAL_NOPRI)
			printf("X ");
		else
			printf("%d ", f->f_pmask[i]);
	printf("%s", TypeNames[f->f_type]);
	switch (f->f_type) {
	case F_FILE:
	case F_TTY:
	case F_CONSOLE:
	case F_FORW:
	case F_USER:
		printf(": %s\n", f->f_name);
		break;

	default:
		printf("\n");
		break;
	}
}
