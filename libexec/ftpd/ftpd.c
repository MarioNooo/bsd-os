/*
 * Copyright (c) 1985, 1988, 1990, 1992, 1993, 1994
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
 *
 *	BSDI ftpd.c,v 1.20 2002/05/30 19:31:20 dab Exp
 */

#if 0
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1985, 1988, 1990, 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */
#endif

#ifndef lint
#if 0
static char sccsid[] = "@(#)ftpd.c	8.4 (Berkeley) 4/16/94";
#endif
static const char rcsid[] =
	"ftpd.c,v 1.20 2002/05/30 19:31:20 dab Exp";
#endif /* not lint */

/*
 * FTP server.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define	FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>


#ifdef	FREEBSD
#include <libutil.h>
#endif

#include <login_cap.h>

#include "pathnames.h"
#include "extern.h"
#include "ftpd.h"

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef  AUTH_PWEXPIRED
#include <bsd_auth.h>
#endif

#ifdef	INTERNAL_LS
static char version[] = "BSDI Version 7.00LS";
#undef main
#else
static char version[] = "BSDI Version 7.00";
#endif

extern	off_t restart_point;
extern	char cbuf[];
extern	int xferbufsize;
extern	char *xferbuf;

struct	sockaddr_generic server_addr;
struct	sockaddr_generic ctrl_addr;
struct	sockaddr_generic data_source;
struct	sockaddr_generic data_dest;
struct	sockaddr_generic his_addr;
struct	sockaddr_generic pasv_addr;

#if IPSEC
void *request = NULL;
int requestlen = 0;
#endif /* IPSEC */

sigset_t allsigs;

int	daemon_mode;
int	data;
jmp_buf	errcatch, urgcatch;
int	logged_in;
struct	passwd *pw;
int	guest;
int	dochroot;
int	statfd = -1;
int	type;
int	form;
int	stru;			/* avoid C keyword */
int	mode;
int	usedefault = 1;		/* for data transfers */
int	pdata = -1;		/* for passive mode */
sig_atomic_t transflag;
off_t	file_size;
off_t	byte_count;
int	defumask = CMASK;	/* default umask value */
char	tmpline[7];
char	remotehost[MAXHOSTNAMELEN];
char	*ident = NULL;
int	show_error = 0;
int	nolreply;
char	*rfc931user;
int	reply_code;		/* what reply code we are processing */

static char ttyline[20];

char	*bind_address;
char	*pid_file = NULL;
struct	pidlist *pidlist;

char	*circuit_cache = NULL;
int	circuit_serial;
int	circuit_size = 997;
int	circuit_prio = 1024;
u_long	circuit_port;

int insert_entry(int, struct sockaddr_generic *, struct sockaddr_generic *, u_long);
int delete_entry(int, struct sockaddr_generic *, struct sockaddr_generic *, u_long);
int install_circuit_cache(int, u_long, char *, int);


/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
#define	SWAITMAX	90	/* wait at most 90 seconds */
#define	SWAITINT	5	/* interval between retries */

int	swaitmax = SWAITMAX;
int	swaitint = SWAITINT;
int	needreap;
int	needupdate;
int	needstat;

#ifdef SETPROCTITLE
#ifdef OLD_SETPROCTITLE
char	**Argv = NULL;		/* pointer to argument vector */
char	*LastArgv = NULL;	/* end of argv */
#endif /* OLD_SETPROCTITLE */
char	proctitle[LINE_MAX];	/* initial part of title */
#endif /* SETPROCTITLE */

#ifndef	MAP_FAILED
#define	MAP_FAILED	((void *)-1)
#endif

#define LOGCMD(cmd, file) \
	if (FL(ELOGGING)) \
	    syslog(LOG_INFO,"%s %s%s", cmd, \
		*(file) == '/' ? "" : curdir(), file);
#define LOGCMD2(cmd, file1, file2) \
	 if (FL(ELOGGING)) \
	    syslog(LOG_INFO,"%s %s%s %s%s", cmd, \
		*(file1) == '/' ? "" : curdir(), file1, \
		*(file2) == '/' ? "" : curdir(), file2);
#define LOGBYTES(cmd, file, cnt) \
	if (FL(ELOGGING)) { \
		if (cnt == (off_t)-1) \
		    syslog(LOG_INFO,"%s %s%s", cmd, \
			*(file) == '/' ? "" : curdir(), file); \
		else \
		    syslog(LOG_INFO, "%s %s%s = %qd bytes", \
			cmd, (*(file) == '/') ? "" : curdir(), file, cnt); \
	}

static void	 ack __P((char *));
static void	 myoob __P((int));
static int	 checkuser __P((char *, char *, int));
static FILE	*dataconn __P((char *, off_t, char *));
static void	 dolog __P((struct sockaddr *));
static char	*curdir __P((void));
static void	 end_login __P((void));
static FILE	*getdatasock __P((char *));
static char	*gunique __P((char *));
static void	 lostconn __P((int));
static int	 receive_data __P((FILE *, FILE *));
static void	 send_data __P((FILE *, FILE *, off_t, int));
static struct passwd *
		 sgetpwnam __P((char *));
static void	 statsig __P((int));
static void	 updatesig __P((int));
static void	 reapchild __P((int));
static void	 reapchildren __P((void));
static void	 fatalsig __P((int));
static void      logxfer __P((char *, long, time_t, int));
static int	 storeok __P((char *, struct ftpdir **, int));
static char	*rfc931 __P((struct sockaddr *, struct sockaddr *));

char *start_auth __P((char *, char *, struct passwd *));
char *check_auth __P((char *, char *));

#if IPSEC
int nrl_strtoreq __P((char *, void **, int *));
#endif /* IPSEC */
int satoport __P((char *port, struct sockaddr *sa));
int satosport __P((char *, struct sockaddr *));
int satolport __P((char *, struct sockaddr *));
int satoeport __P((char *, int, struct sockaddr *, int));

static char *
curdir()
{
	static char path[MAXPATHLEN+1+1];	/* path + '/' + '\0' */

	if (getcwd(path, sizeof(path)-2) == NULL)
		return ("");
	if (path[1] != '\0')		/* special case for root dir. */
		strcat(path, "/");
	/* For guest account, skip / since it's chrooted */
	return (guest ? path+1 : path);
}

int
main(argc, argv, envp)
	int argc;
	char *argv[];
	char **envp;
{
	int addrlen, ch, on = 1, tos, verify = 0;
	char *configfile;
	FILE *fd;
	struct pidlist *pl;
	pid_t pid;
#ifndef	AUTH_PWEXPIRED
	char number[16];
#endif
	char *portname = "ftp";
	char *e;

	tzset();		/* in case no timezone database in ~ftp */
        sigfillset(&allsigs);	/* used to block signals while root */

#ifdef OLD_SETPROCTITLE
	/*
	 *  Save start and extent of argv for setproctitle.
	 */
	Argv = argv;
	while (*envp)
		envp++;
	LastArgv = envp[-1] + strlen(envp[-1]);
#endif /* OLD_SETPROCTITLE */

	/*
	 * Set up initial defaults so we can override them
	 * on the command line
	 */
	config_defhost();
	configfile = _PATH_FTPCONFIG;

	while ((ch = getopt(argc, argv, "a:c:C:Ddf:n:p:P:Vv"
#if IPSEC
	    "S:"
#endif /* IPSEC */
	    )) != -1) {
		switch (ch) {
		case 'a':
			bind_address = optarg;
			break;

		case 'c':
			circuit_cache = optarg;
			break;

		case 'C':
			circuit_prio = strtol(optarg, &e, 0);
			if (e == optarg || *e)
				errx(1, "%s: invalid circuit priority",
				    optarg);
			break;

		case 'D':
			daemon_mode++;
			break;

		case 'd':
			defhost->flags |= DEBUG;
			break;

		case 'f':
			configfile = optarg;
			break;

		case 'n':
			circuit_size = strtol(optarg, &e, 0);
			if (circuit_size < 1 || circuit_size > 0xffffff || *e)
				errx(1, "%s: invalid circuit cache size",
				    optarg);
			break;

		case 'p':
			pid_file = optarg;
			break;

		case 'P':
			portname = optarg;
			break;

#if IPSEC
		case 'S':
			if (nrl_strtoreq(optarg, &request, &requestlen)) {
				fprintf(stderr,
				    "ftpd: nrl_strtoreq(%s) failed\n", optarg);
				exit(1);
			}
			break;
#endif /* IPSEC */

		case 'V':
			show_error = 1;
			verify = 1;
			break;

		case 'v':
			defhost->flags |= DEBUG;
			break;

		default:
			warnx("unknown flag -%c ignored", optopt);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	while (argc > 0) {
		warnx("unknown argument \"%s\" ignored", *argv);
		--argc;
		++argv;
	}

	if (config_parse(configfile) > 0) {
		if (!verify)
			syslog(LOG_ERR, "ftpd: unable to parse %s",
			    _PATH_FTPCONFIG);
		exit(1);
	}
	if (verify)
		exit(0);

	(void) freopen(_PATH_DEVNULL, "w", stderr);

	/*
	 * LOG_NDELAY sets up the logging connection immediately,
	 * necessary for anonymous ftp's that chroot and can't do it later.
	 */
	openlog("ftpd", LOG_PID | LOG_NDELAY, LOG_FTP);

	for (ch = 1; ch < NSIG; ++ch)
		if (ch != SIGWINCH && ch != SIGINFO && ch != SIGUSR1 &&
		    ch != SIGUSR2 && ch != SIGIO)
			signal(ch, fatalsig);

	if (circuit_cache != NULL) {
		circuit_serial = install_circuit_cache(circuit_size,
		    htonl(0x0000ffff), circuit_cache, circuit_prio);
		if (circuit_serial == 0)
			syslog(LOG_ERR, "could not install circuit cache");
	}

	if (daemon_mode) {
		int ctl_sock, fd, debug, port;
		struct servent *sv;
		struct sockaddr *sa = SA(&server_addr);

		debug = FL(DEBUG);
		/*
		 * Detach from parent.
		 */
		if (debug == 0 && daemon(1, 1) < 0) {
			syslog(LOG_ERR, "failed to become a daemon");
			exit(1);
		}
		(void) signal(SIGCHLD, reapchild);
		(void) signal(SIGHUP, updatesig);
		(void) signal(SIGTERM, statsig);
		/*
		 * Get port number for ftp/tcp.
		 */
		sv = getservbyname(portname, "tcp");
		if (sv == NULL) {
			char *end;
			port = strtol(portname, &end, 0);
			if (*portname == '\0' || *end != '\0') {
				syslog(LOG_ERR,
				    "getservbyname for \"%s\" failed",
				    portname);
				exit(1);
			}
			if (port < 1 || port > 65535) {
				syslog(LOG_ERR, "invalid port number %s", port);
				exit(1);
			}
			HTONS(port);
		} else
			port = sv->s_port;
		/*
		 * Open a socket, bind it to the FTP port, and start
		 * listening.
		 */
		memset(&server_addr, 0, sizeof(server_addr));
		if (bind_address == NULL) {
#if INET6
			goto try_inet6;
#else /* INET6 */
			goto try_inet;
#endif /* INET6 */
		} else {
			struct in_addr bsin;
#if INET6
			struct in6_addr bsin6;
#endif /* INET6 */
			if (inet_aton(optarg, &bsin)) {
				SIN(sa)->sin_addr = bsin;
			try_inet:
				SIN(sa)->sin_family = AF_INET;
				SIN(sa)->sin_len = sizeof(*SIN(sa));
				SIN(sa)->sin_port = port;
#if INET6
			} else if (inet_pton(AF_INET6, bind_address, &bsin6)) {
				SIN6(sa)->sin6_addr = bsin6;
			try_inet6:
				SIN6(sa)->sin6_family = AF_INET6;
				SIN6(sa)->sin6_len = sizeof(*SIN6(sa));
				SIN6(sa)->sin6_port = port;
#endif /* INET6 */
			} else
				errx(1, "invalid address for -a");
		}
		ctl_sock = socket(sa->sa_family, SOCK_STREAM, 0);
		if (ctl_sock < 0) {
#if INET6
			if (bind_address == NULL && sa->sa_family == AF_INET6)
				goto try_inet;
#endif /* INET6 */
			syslog(LOG_ERR, "control socket: %m");
			exit(1);
		}
		if (setsockopt(ctl_sock, SOL_SOCKET, SO_REUSEADDR,
		    (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "control setsockopt: %m");;
		if (bind(ctl_sock, sa, sa->sa_len)) {
#if INET6
			if (bind_address == NULL && sa->sa_family == AF_INET6) {
				close(ctl_sock);
				goto try_inet;
			}
#endif /* INET6 */
			syslog(LOG_ERR, "control bind: %m");
			exit(1);
		}
		if (listen(ctl_sock, 32) < 0) {
			syslog(LOG_ERR, "control listen: %m");
			exit(1);
		}
		/*
		 * Atomically write process ID
		 */
		if (pid_file) {   
			int fd;
			char buf[20];

			fd = open(pid_file, O_CREAT | O_WRONLY | O_TRUNC
				| O_NONBLOCK | O_EXLOCK, 0644);
			if (fd < 0) {
				if (errno == EAGAIN)
					errx(1, "%s: file locked", pid_file);
				else
					err(1, "%s", pid_file);
			}
			snprintf(buf, sizeof(buf),
				"%lu\n", (unsigned long) getpid());
			if (write(fd, buf, strlen(buf)) < 0)
				err(1, "%s: write", pid_file);
			/* Leave the pid file open and locked */
		}
		/*
		 * Loop forever accepting connection requests and forking off
		 * children to handle them.
		 */
		while (1) {
			fd_set rs;
			FD_ZERO(&rs);
			FD_SET(ctl_sock, &rs);

			fd = -1;
			if (needupdate == 0 && needreap == 0 && needstat == 0)
			switch (select(ctl_sock + 1, &rs, NULL, NULL, NULL)) {
			case 1:
				if (FD_ISSET(ctl_sock, &rs)) {
					addrlen = sizeof(his_addr);
					fd = accept(ctl_sock, SA(&his_addr),
					    &addrlen);
					if (fd < 0)
						syslog(LOG_ERR, "accept: %m");
				}
				break;
			}

			/*
			 * Check for SIGHUP, SIGCHLD, and SIGTERM serialy
			 * This prevents tricky race conditions of trying
			 * to reap a child while trying to update the
			 * configuration file.
			 */
			if (needreap)
				reapchildren();
			if (needupdate) {
				sigprocmask(SIG_BLOCK, &allsigs, NULL);
				needupdate = 0;
				sigprocmask(SIG_UNBLOCK,&allsigs,NULL);
				if (config_parse(configfile) > 0)
					syslog(LOG_ERR, "failed to re-read %s",
					    configfile);
			}
			if (needstat) {
				sigprocmask(SIG_BLOCK, &allsigs, NULL);
				needstat = 0;
				sigprocmask(SIG_UNBLOCK,&allsigs,NULL);
			}

			if (fd < 0)
				continue;
			addrlen = sizeof(ctrl_addr);
			if (getsockname(fd, SA(&ctrl_addr), &addrlen) < 0) {
				syslog(LOG_ERR, "getsockname (%s): %m",argv[0]);
				close(fd);
				continue;
			}
#if INET6
			fix_v4mapped(SA(&his_addr), NULL);
			fix_v4mapped(SA(&ctrl_addr), NULL);
#endif /* INET6 */
			select_host(SA(&ctrl_addr));
			if (debug || (pid = fork()) == 0) {
				/* child */
				(void) signal(SIGHUP, fatalsig);
				(void) signal(SIGTERM, fatalsig);
				(void) dup2(fd, 0);
				(void) dup2(fd, 1);
				close(ctl_sock);
				OP(sessions)++;
#ifndef	AUTH_PWEXPIRED
				snprintf(number, sizeof(number), "%d",
				    OP(sessions));
				auth_setopt("FTPD_HOST", OP(hostname));
				auth_setopt("FTPD_SESSIONS", number);
#endif
				break;
			}
			if (pid < 0) {
				syslog(LOG_ERR, "fork (%s): %m", argv[0]);
				close(fd);
				continue;
			}
			if ((pl = malloc(sizeof(struct pidlist))) == NULL)
				syslog(LOG_ERR,"memory allocation failure: %m");
			else {
				pl->pid = pid;
				pl->host = thishost;
				pl->host->sessions++;
				sigprocmask(SIG_BLOCK, &allsigs, NULL);
				pl->next = pidlist;
				pidlist = pl;
				sigprocmask(SIG_UNBLOCK,&allsigs,NULL);
			}
				
			close(fd);
		}
	} else {
		addrlen = sizeof(his_addr);
		if (getpeername(0, SA(&his_addr), &addrlen) < 0) {
			syslog(LOG_ERR, "getpeername (%s): %m",argv[0]);
			exit(1);
		}
#if INET6
		fix_v4mapped(SA(&his_addr), NULL);
#endif /* INET6 */
	}

	(void) signal(SIGCHLD, SIG_IGN);
	(void) signal(SIGPIPE, lostconn);
	if ((int)signal(SIGURG, myoob) < 0)
		syslog(LOG_ERR, "signal: %m");

	if (!daemon_mode) {
		addrlen = sizeof(ctrl_addr);
		if (getsockname(0, SA(&ctrl_addr), &addrlen) < 0) {
			syslog(LOG_ERR, "getsockname (%s): %m",argv[0]);
			exit(1);
		}
#if INET6
		fix_v4mapped(SA(&ctrl_addr), NULL);
#endif /* INET6 */
		select_host(SA(&ctrl_addr));
	}
	if (FL(VIRTUALONLY) && !FL(VIRTUAL))
		exit(1);
	if (FL(USERFC931))
		rfc931user = rfc931(SA(&ctrl_addr), SA(&his_addr));

	/*
	 * From this point on we can't change logfmt
	 * The extra copy is one we can write null's to.
	 */
	OP(tmplogfmt) = sgetsave(OP(logfmt));
	init_groups(OP(groupfile));
#ifdef IP_TOS
	tos = IPTOS_LOWDELAY;
	if (SA(&ctrl_addr)->sa_family == AF_INET &&
	    setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	memcpy(&data_source, &ctrl_addr, SA(&ctrl_addr)->sa_len);

	switch(SA(&ctrl_addr)->sa_family) {
	case AF_INET:
		SIN(&data_source)->sin_port =
		    htons(ntohs(SIN(&ctrl_addr)->sin_port) - 1);
		break;
#if INET6
	case AF_INET6:
		SIN6(&data_source)->sin6_port =
		    htons(ntohs(SIN6(&ctrl_addr)->sin6_port) - 1);
		break;
#endif /* INET6 */
	default:
		syslog(LOG_ERR, "don't know how to deal with af#%d connections!",
		    SA(&ctrl_addr)->sa_family);
		exit(1);
	}

	switch(SA(&his_addr)->sa_family) {
	case AF_INET:
#if INET6
	case AF_INET6:
#endif /* INET6 */
		break;
	default:
		syslog(LOG_ERR,
		    "don't know how to deal with af#%d connections!",
		    SA(&ctrl_addr)->sa_family);
		exit(1);
	}

	/* set this here so klogin can use it... */
	(void)snprintf(ttyline, sizeof(ttyline), "ftp%d", getpid());

	/* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
	if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on)) < 0)
		syslog(LOG_ERR, "setsockopt: %m");
#endif

#ifdef	F_SETOWN
	if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
		syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#endif
	dolog(SA(&his_addr));
#if IPSEC
	if (request == NULL) {
		requestlen = 2048; /* XXX */
		if (!(request = malloc(requestlen))) {
			syslog(LOG_ERR, "malloc(%d): %s(%d)", requestlen,
			    strerror(errno), errno);
			exit(1);
		}
		if (getsockopt(0, SOL_SOCKET, SO_SECURITY_REQUEST,
			request, (size_t *)&requestlen) < 0) {
			if (errno != ENOENT && errno != ENOPROTOOPT) {
				syslog(LOG_ERR,
				    "getsockopt(SO_SECURITY_REQUEST): %m");
				exit(1);
			}
			free(request);
			request = NULL;
			requestlen = 0;
		}
	}
#endif /* IPSEC */
	/*
	 * Set up default state
	 */
	data = -1;
	type = TYPE_A;
	form = FORM_N;
	stru = STRU_F;
	mode = MODE_S;
	tmpline[0] = '\0';

	/* If logins are disabled, print out the message. */
	if ((fd = fopen(_PATH_NOLOGIN,"r")) != NULL) {
		file_reply(-530, NULL, fd);
		reply(530, "System not available.");
		exit(0);
	}
	if ((fd = fopen(OP(welcome), "r")) != NULL) {
		file_reply(-220, NULL, fd);
		(void) fflush(stdout);
		/* reply(220,) must follow */
	}
	reply(220, "%s FTP server (%s) ready.", OP(hostname), version);
	(void) setjmp(errcatch);
	for (;;)
		(void) yyparse();
	/* NOTREACHED */
}

static void
lostconn(signo)
	int signo;
{

	if (FL(DEBUG))
		syslog(LOG_DEBUG, "lost connection");
	dologout(1);
}

/*
 * Helper function for sgetpwnam().
 */
char *
sgetsave(s)
	char *s;
{
	char *new = malloc((unsigned) strlen(s) + 1);

	if (new == NULL) {
		perror_reply(421, "Local resource failure: malloc");
		dologout(1);
		/* NOTREACHED */
	}
	(void) strcpy(new, s);
	return (new);
}

/*
 * Save the result of a getpwnam.  Used for USER command, since
 * the data returned must not be clobbered by any other command
 * (e.g., globbing).
 */
static struct passwd *
sgetpwnam(name)
	char *name;
{
	static struct passwd save;
	struct passwd *p;

	if ((p = getpwnam(name)) == NULL)
		return (p);
	if (save.pw_name) {
		free(save.pw_name);
		free(save.pw_passwd);
		free(save.pw_gecos);
		free(save.pw_dir);
		free(save.pw_shell);
	}
	save = *p;
	save.pw_name = sgetsave(p->pw_name);
	save.pw_passwd = sgetsave(p->pw_passwd);
	save.pw_gecos = sgetsave(p->pw_gecos);
	save.pw_dir = sgetsave(p->pw_dir);
	save.pw_shell = sgetsave(p->pw_shell);
	return (&save);
}

static int login_attempts;	/* number of failed login attempts */
static int askpasswd;		/* had user command, ask for passwd */
static char curname[32];	/* current USER name */

/*
 * USER command.
 * Sets global passwd pointer pw if named account exists and is acceptable;
 * sets askpasswd if a PASS command is expected.  If logged in previously,
 * need to reset state.  If name is "ftp" or "anonymous", the name is not in
 * _PATH_FTPUSERS, and ftp account exists, set guest and pw, then just return.
 * If account doesn't exist, ask for passwd anyway.  Otherwise, check user
 * requesting login privileges.  Disallow anyone who does not have a standard
 * shell as returned by getusershell().  Disallow anyone mentioned in the file
 * _PATH_FTPUSERS to allow people such as root and uucp to be avoided.
 */
void
user(name)
	char *name;
{
	char *cp, *shell, *auth;

	if (logged_in) {
		if (guest) {
			reply(530, "Can't change user from guest login.");
			return;
		} else if (dochroot) {
			reply(530, "Can't change user from chroot user.");
			return;
		}
		end_login();
	}

	if ((auth = strchr(name, ':')) != NULL)
		*auth++ = 0;

	guest = 0;
	if (strcmp(name, "ftp") == 0 || strcmp(name, "anonymous") == 0) {
		if (!FL(ALLOW_ANON) ||
		    checkuser(OP(banlist), "ftp", 0) ||
		    checkuser(OP(banlist), "anonymous", 0) ||
		    (checkuser(OP(permitlist), "ftp", 1) == 0 &&
		     checkuser(OP(permitlist), "anonymous", 1) == 0))
			reply(530, "User %s access denied.", name);
		else if (OP(maxusers) && OP(maxusers) < OP(sessions))
			reply(530, "Guest session limit exceeded.");
		else if ((pw = sgetpwnam(OP(anonuser))) != NULL) {
			if (OP(anondir) == NULL)
				OP(anondir) = pw->pw_dir;
			guest = 1;
			askpasswd = 1;
			reply(331,
			"Guest login ok, send your email address as password.");
		} else
			reply(530, "User %s unknown.", name);
		if (!askpasswd && FL(LOGGING))
			syslog(LOG_NOTICE,
			    "ANONYMOUS FTP LOGIN REFUSED FROM %s", remotehost);
		return;
	}
	if (FL(ANON_ONLY) != 0) {
		reply(530, "Sorry, only anonymous ftp allowed.");
		return;
	}
		
	if ((pw = sgetpwnam(name))) {
		if ((shell = pw->pw_shell) == NULL || *shell == 0)
			shell = _PATH_BSHELL;
		while ((cp = getusershell()) != NULL)
			if (strcmp(cp, shell) == 0)
				break;
		endusershell();

		if (cp == NULL || checkuser(OP(banlist), name, 0) ||
		    checkuser(OP(permitlist), name, 1) == 0) {
			reply(530, "User %s access denied.", name);
			if (FL(LOGGING))
				syslog(LOG_NOTICE,
				    "FTP LOGIN REFUSED FROM %s, %s",
				    remotehost, name);
			pw = (struct passwd *) NULL;
			return;
		}
	}
	strncpy(curname, name, sizeof(curname)-1);
	if ((cp = start_auth(auth, name, pw)) != NULL)
		reply(331, cp);
	else
		reply(331, "Password required for %s.", name);

	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}

/*
 * Check if a user is in the file "fname"
 * If fname is NULL or the file does not exist, return req
 */
static int
checkuser(fname, name, req)
	char *fname;
	char *name;
	int req;
{
	FILE *fd;
	int found = 0;
	char *p, line[BUFSIZ];
	struct passwd *pwd;

	if (fname == NULL)
		return (req);

	if ((fd = fopen(fname, "r")) != NULL) {
		if (pw && strcmp(pw->pw_name, name) == 0)
			pwd = pw;
		else
			pwd = getpwnam(name);
		while (!found && fgets(line, sizeof(line), fd) != NULL)
			if ((p = strchr(line, '\n')) != NULL) {
				*p = '\0';
				if (line[0] == '#')
					continue;
				/*
				 * if first chr is '@', check group membership
				 */
				if (line[0] == '@') {
					int i = 0;
					struct group *grp;

					if ((grp = getgrnam(line+1)) == NULL)
						continue;
					if (pwd && grp->gr_gid == pwd->pw_gid)
						found = 1;
					while (!found && grp->gr_mem[i])
						found = strcmp(name,
							grp->gr_mem[i++])
							== 0;
				}
				/*
				 * Otherwise, just check for username match
				 */
				else
					found = strcmp(line, name) == 0;
			}
		(void) fclose(fd);
	} else
		return (req);
	return (found);
}

/*
 * Terminate login as previous user, if any, resetting state;
 * used when USER command is given or login fails.
 */
static void
end_login()
{

	sigprocmask(SIG_BLOCK, &allsigs, NULL);
	(void) seteuid((uid_t)0);
	if (logged_in)
		ftpd_logwtmp(ttyline, "", "");
	pw = NULL;
	setusercontext(NULL, getpwuid(0), (uid_t)0,
		       LOGIN_SETPRIORITY|LOGIN_SETRESOURCES|LOGIN_SETUMASK);
	logged_in = 0;
	guest = 0;
	dochroot = 0;
}

void
pass(passwd)
	char *passwd;
{
	FILE *fd;
	char *msg, *dir;
	extern login_cap_t *class;
	static char homedir[MAXPATHLEN];

	if (logged_in || askpasswd == 0) {
		reply(503, "Login with USER first.");
		return;
	}
	if (*passwd == '-') {
		nolreply = 1;
		++passwd;
	}
	askpasswd = 0;
	if (!guest) {		/* "ftp" is only account allowed no password */
		if (pw == NULL)
			msg = "permission denied.";
		else
			msg = check_auth(curname, passwd);
		if (msg != NULL) {
			reply(530, msg);
			if (FL(LOGGING))
				syslog(LOG_NOTICE,
				    "FTP LOGIN FAILED FROM %s, %s",
				    remotehost, curname);
			pw = NULL;
			if (login_attempts++ >= 5) {
				syslog(LOG_NOTICE,
				    "repeated login failures from %s",
				    remotehost);
				exit(0);
			}
			return;
		}
	} else if ((class = login_getclass(pw->pw_class)) != NULL) {
		int r;
#ifdef  AUTH_PWEXPIRED
		auth_session_t *as;
		char number[16];

		if ((as = auth_open()) == NULL)
			fatal("internal memory allocation failure");
		snprintf(number, sizeof(number), "%d", OP(sessions));
		auth_setoption(as, "FTPD_HOST", OP(hostname));
		auth_setoption(as, "FTPD_SESSIONS", number);
		r = auth_approval(as, class, pw->pw_name, "ftp");
		auth_close(as);
#else
		r = auth_approve(class, pw->pw_name, "ftp");
#endif
		if (r == 0) {
			syslog(LOG_INFO|LOG_AUTH,
			    "FTP LOGIN FAILED (HOST) as %s: approval failure.",
			    pw->pw_name);
			reply(530, "Approval failure.\n");
			exit(0);
		}
	} else {
		syslog(LOG_INFO|LOG_AUTH,
		    "FTP LOGIN CLASS %s MISSING for %s: approval failure.",
		    pw->pw_class, pw->pw_name);
		reply(530, "Permission denied.\n");
		exit(0);
	}
	login_attempts = 0;		/* this time successful */
	if (setegid((gid_t)pw->pw_gid) < 0) {
		reply(550, "Can't set gid.");
		return;
	}
	/* May be overridden by login.conf */
	(void) umask(defumask);
	setusercontext(class, pw, (uid_t)0,
	LOGIN_SETGROUP|LOGIN_SETPRIORITY|LOGIN_SETRESOURCES|LOGIN_SETUMASK);

	/* open wtmp before chroot */
	ftpd_logwtmp(ttyline, pw->pw_name, remotehost);
	logged_in = 1;

	if (FL(STATS) && statfd < 0)
		if ((statfd = open(OP(statfile), O_WRONLY|O_APPEND)) < 0)
			OP(flags) &= ~(STATS | (STATS << GUESTSHIFT));

	dochroot =
		login_getcapbool(class, "ftp-chroot", 0) ||
		checkuser(OP(chrootlist), pw->pw_name, 0);
	dir = login_getcapstr(class, "ftp-dir", NULL, NULL);
	if (dir) {
		if ((dir = strdup(dir)) == NULL)
			fatal("Ran out of memory.");
		free(pw->pw_dir);
		pw->pw_dir = dir;
	}
	if (guest) {
		/*
		 * We MUST do a chdir() after the chroot. Otherwise
		 * the old current directory will be accessible as "."
		 * outside the new root!
		 */
		if (chroot(OP(anondir)) < 0 || chdir("/") < 0) {
			reply(550, "Can't set guest privileges.");
			goto bad;
		}
	} else if (dochroot) {
		if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
			reply(550, "Can't change root.");
			goto bad;
		}
	} else if (chdir(pw->pw_dir) < 0) {
		if (chdir("/") < 0) {
			reply(530, "User %s: can't change directory to %s.",
			    pw->pw_name, pw->pw_dir);
			goto bad;
		} else
			reply(-230, "No directory! Logging in with home=/");
	}
	/*
	 * Fixup all the incoming directories, now that we are
	 * chrooted.  Only done for guests.
	 */
	if (guest) {
		struct ftpdir *fd;
		struct stat st;

		for (fd = OP(ftpdirs); fd; fd = fd->next)
			if (stat(fd->path, &st) == 0 &&
			    S_ISDIR(st.st_mode) == 1) {
syslog(LOG_INFO, "Add %s to the path of incoming", fd->path);
				fd->dev = st.st_dev;
				fd->ino = st.st_ino;
			}
else syslog(LOG_INFO, "Failed to add %s to the path of incoming", fd->path);
	}
	if (seteuid((uid_t)pw->pw_uid) < 0) {
		reply(550, "Can't set uid.");
		goto bad;
	}
        sigprocmask(SIG_UNBLOCK,&allsigs,NULL);

	/*
	 * Set home directory so that use of ~ (tilde) works correctly.
	 */
	if (getcwd(homedir, MAXPATHLEN) != NULL)
		setenv("HOME", homedir, 1);

	/*
	 * Display a login message, if it exists.
	 * N.B. reply(230,) must follow the message.
	 */
	if ((fd = fopen(OP(loginmsg), "r")) != NULL) {
		file_reply(-230, NULL, fd);
		(void) fflush(stdout);
	}
	if (ident != NULL)
		free(ident);
	if (guest) {
		ident = strdup(passwd);
		if (ident == NULL)
			fatal("Ran out of memory.");

		reply(230, "Guest login ok, access restrictions apply.");
#ifdef SETPROCTITLE
		if (FL(VIRTUAL))
			snprintf(proctitle, sizeof(proctitle),
				 "%s: anonymous(%s)/%.*s", remotehost, OP(hostname),
				 sizeof(proctitle) - sizeof(remotehost) -
				 sizeof(": anonymous/"), passwd);
		else
			snprintf(proctitle, sizeof(proctitle),
				 "%s: anonymous/%.*s", remotehost,
				 sizeof(proctitle) - sizeof(remotehost) -
				 sizeof(": anonymous/"), passwd);
		setproctitle("%s", proctitle);
#endif /* SETPROCTITLE */
		if (FL(LOGGING))
			syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s, %s",
			    remotehost, passwd);
	} else {
	    if (dochroot)
		reply(230, "User %s logged in, access restrictions apply.", 
			pw->pw_name);
	    else
		reply(230, "User %s logged in.", pw->pw_name);

		ident = strdup(pw->pw_name);
		if (ident == NULL)
			fatal("Ran out of memory.");

#ifdef SETPROCTITLE
		snprintf(proctitle, sizeof(proctitle),
			 "%s: %s", remotehost, pw->pw_name);
		setproctitle("%s", proctitle);
#endif /* SETPROCTITLE */
		if (FL(LOGGING))
			syslog(LOG_INFO, "FTP LOGIN FROM %s as %s",
			    remotehost, pw->pw_name);
	}
	login_close(class);
	class = NULL;
	return;
bad:
	/* Forget all about it... */
	login_close(class);
	class = NULL;
	end_login();
}

void
retrieve(cmd, name)
	char *cmd, *name;
{
	FILE *fin, *dout;
	struct stat st;
	int (*closefunc) __P((FILE *));
	time_t start;

	if (cmd == 0) {
		fin = fopen(name, "r"), closefunc = fclose;
		st.st_size = 0;
	} else {
		char line[BUFSIZ];

		(void) snprintf(line, sizeof(line), cmd, name), name = line;
		fin = ftpd_popen(line, "r"), closefunc = ftpd_pclose;
		st.st_size = -1;
		st.st_blksize = BUFSIZ;
	}
	if (fin == NULL) {
		if (errno != 0) {
			perror_reply(550, name);
			if (cmd == 0) {
				LOGCMD("get", name);
			}
		}
		return;
	}
	byte_count = -1;
	if (cmd == 0 && (fstat(fileno(fin), &st) < 0 || !S_ISREG(st.st_mode))) {
		reply(550, "%s: not a plain file.", name);
		goto done;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			off_t i, n;
			int c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fin)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}
		} else if (lseek(fileno(fin), restart_point, L_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	dout = dataconn(name, st.st_size, "w");
	if (dout == NULL)
		goto done;
	time(&start);
	send_data(fin, dout, st.st_size,
		  restart_point == 0 && cmd == 0 && S_ISREG(st.st_mode));
	if (circuit_port) {
		delete_entry(circuit_serial, &his_addr,
		    &pasv_addr, circuit_port);
		circuit_port = 0;
	}
	(void) fclose(dout);
	if (cmd == 0 && FL(STATS))
		logxfer(name, st.st_size, start, 'o');
	data = -1;
	pdata = -1;
done:
	if (cmd == 0)
		LOGBYTES("get", name, byte_count);
	(*closefunc)(fin);
}

void
store(name, mode, unique)
	char *name, *mode;
	int unique;
{
	FILE *fout, *din;
	struct stat st;
	int (*closefunc) __P((FILE *));
	time_t start;
	struct ftpdir *fd;

	if ((unique || guest) && stat(name, &st) == 0 &&
	    (name = gunique(name)) == NULL) {
		LOGCMD(*mode == 'w' ? "put" : "append", name);
		return;
	}

	if (storeok(name, &fd, -553) == 0) {
		reply(553, "%s: access denied", name);
		LOGCMD(*mode == 'w' ? "put" : "append", name);
		return;
	}

	if (restart_point)
		mode = "r+";
	if (fd) {
		sigprocmask(SIG_BLOCK, &allsigs, NULL);
		(void) seteuid((uid_t)0);
		/*
		 * fchown and fchmod should not be able to fail
		 */
		fout = fopen(name, mode);
		if (fout != NULL) {
			fchown(fileno(fout), fd->uid, fd->gid);
			fchmod(fileno(fout), fd->mode);
		}
		(void) seteuid((uid_t)pw->pw_uid);
		sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
	} else
		fout = fopen(name, mode);
	closefunc = fclose;
	if (fout == NULL) {
		perror_reply(553, name);
		LOGCMD(*mode == 'w' ? "put" : "append", name);
		return;
	}
	byte_count = -1;
	if (restart_point) {
		if (type == TYPE_A) {
			off_t i, n;
			int c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}
			/*
			 * We must do this seek to "current" position
			 * because we are changing from reading to
			 * writing.
			 */
			if (fseek(fout, 0L, L_INCR) < 0) {
				perror_reply(550, name);
				goto done;
			}
		} else if (lseek(fileno(fout), restart_point, L_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	din = dataconn(name, (off_t)-1, "r");
	if (din == NULL)
		goto done;
	time(&start);
	if (receive_data(din, fout) == 0) {
		if (unique)
			reply(226, "Transfer complete (unique file name:%s).",
			    name);
		else
			reply(226, "Transfer complete.");
	}
	if (circuit_port) {
		delete_entry(circuit_serial, &his_addr,
		    &pasv_addr, circuit_port);
		circuit_port = 0;
	}
	(void) fclose(din);
	data = -1;
	pdata = -1;
	if (FL(STATS))
		logxfer(name, byte_count, start, 'i');
done:
	LOGBYTES(*mode == 'w' ? "put" : "append", name, byte_count);
	(*closefunc)(fout);
}

void
set_dataopts(s, mode, gsa)
	int s;
	char *mode;
	struct  sockaddr_generic *gsa;
{
	int on;

#ifdef IP_TOS
	on = IPTOS_THROUGHPUT;
	if (SA(gsa)->sa_family == AF_INET &&
	    setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
#ifdef TCP_NOPUSH
	/*
	 * Turn off push flag to keep sender TCP from sending short packets
	 * at the boundaries of each write().  Should probably do a SO_SNDBUF
	 * to set the send buffer size as well, but that may not be desirable
	 * in heavy-load situations.
	 */
	on = 1;
	if (setsockopt(s, IPPROTO_TCP, TCP_NOPUSH, (char *)&on, sizeof on) < 0)
		syslog(LOG_WARNING, "setsockopt (TCP_NOPUSH): %m");
#endif
#ifdef SO_SNDBUF
	on = 65536;
	if (setsockopt(s, SOL_SOCKET, (*mode == 'w') ? SO_SNDBUF : SO_RCVBUF,
	    (char *)&on, sizeof on) < 0)
		syslog(LOG_WARNING, "setsockopt (%s): %m",
		    (*mode == 'w') ? "SO_SNDBUF" : "SO_RCVBUF");
#endif
#ifdef	SO_KEEPALIVE
	if (FL(KEEPALIVE)) {
		on = 1;
		if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &on,sizeof(on)) < 0)
			syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	}
#endif
}

static FILE *
getdatasock(mode)
	char *mode;
{
	int on = 1, s, t, tries;

	if (data >= 0)
		return (fdopen(data, mode));
        sigprocmask(SIG_BLOCK, &allsigs, NULL);
	(void) seteuid((uid_t)0);
	s = socket(SA(&ctrl_addr)->sa_family, SOCK_STREAM, 0);
	if (s < 0)
		goto bad;
#if IPSEC
	if (request && setsockopt(s, SOL_SOCKET, SO_SECURITY_REQUEST, request,
	    requestlen) < 0) {
		syslog(LOG_ERR,
		    "setsockopt(..., SO_SECURITY_REQUEST, ...): %s(%d)\n",
		    strerror(errno), errno);
		goto bad;
	}
#endif /* IPSEC */
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	    (char *) &on, sizeof(on)) < 0)
		goto bad;
	/* anchor socket to avoid multi-homing problems */
	for (tries = 1; ; tries++) {
		if (bind(s, SA(&data_source), SA(&data_source)->sa_len) >= 0)
			break;
		if (errno != EADDRINUSE || tries > 10)
			goto bad;
		sleep(tries);
	}
	(void) seteuid((uid_t)pw->pw_uid);
        sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
	set_dataopts(s, mode, &data_source);

	return (fdopen(s, mode));
bad:
	/* Return the real value of errno (close may change it) */
	t = errno;
	(void) seteuid((uid_t)pw->pw_uid);
        sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
	(void) close(s);
	errno = t;
	return (NULL);
}

static FILE *
dataconn(name, size, mode)
	char *name;
	off_t size;
	char *mode;
{
	char sizebuf[32];
	FILE *file;
	int retry = 0;

	file_size = size;
	byte_count = 0;
	if (size != (off_t) -1)
		(void) snprintf(sizebuf, sizeof(sizebuf), " (%qd bytes)", size);
	else
		*sizebuf = '\0';
	if (pdata >= 0) {
		struct sockaddr_generic from;
		int s, fromlen = sizeof(from);
		struct timeval timeout;
		fd_set set;

		FD_ZERO(&set);
		FD_SET(pdata, &set);

		timeout.tv_usec = 0;
		timeout.tv_sec = 120;

		if (select(pdata+1, &set, (fd_set *) 0, (fd_set *) 0, &timeout) == 0 ||
		    (s = accept(pdata, (struct sockaddr *) &from, &fromlen)) < 0) {
			reply(425, "Can't open data connection.");
			if (circuit_port) {
				delete_entry(circuit_serial,
				    &his_addr, &pasv_addr,
				    circuit_port);
				circuit_port = 0;
			}
			(void) close(pdata);
			pdata = -1;
			return (NULL);
		}
		(void) close(pdata);
		pdata = s;
		set_dataopts(s, mode, &from);
		reply(150, "Opening %s mode data connection for '%s'%s.",
		     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
		return (fdopen(pdata, mode));
	}
	if (data >= 0) {
		reply(125, "Using existing data connection for '%s'%s.",
		    name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file = getdatasock(mode);
	if (file == NULL) {
		char addr[46], port[6];

		if (getnameinfo(SA(&data_source), SA(&data_source)->sa_len,
		    addr, sizeof(addr), port, sizeof(port),
		    NI_NUMERICHOST|NI_NUMERICSERV))
			reply(425, "Can't create data socket: %s.",
			    strerror(errno));
		else
			reply(425, "Can't create data socket (%s,%s): %s.",
			    addr, port, strerror(errno));

		return (NULL);
	}
	data = fileno(file);
	while (connect(data, SA(&data_dest), SA(&data_dest)->sa_len) < 0) {
		if (errno == EADDRINUSE && retry < swaitmax) {
			sleep((unsigned) swaitint);
			retry += swaitint;
			continue;
		}
		perror_reply(425, "Can't build data connection");
		(void) fclose(file);
		data = -1;
		return (NULL);
	}
	reply(150, "Opening %s mode data connection for '%s'%s.",
	     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
	return (file);
}

/*
 * Tranfer the contents of "instr" to "outstr" peer using the appropriate
 * encapsulation of the data subject to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
static void
send_data(instr, outstr, filesize, isreg)
	FILE *instr, *outstr;
	off_t filesize;
	int isreg;
{
	int c, cnt, filefd, netfd;
	char *buf, *bp;
	size_t len;

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	switch (type) {

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n') {
				if (ferror(outstr))
					goto data_err;
				(void) putc('\r', outstr);
			}
			(void) putc(c, outstr);
		}
		fflush(outstr);
		transflag = 0;
		if (ferror(instr))
			goto file_err;
		if (ferror(outstr))
			goto data_err;
		reply(226, "Transfer complete.");
		return;

	case TYPE_I:
	case TYPE_L:
		/*
		 * isreg is only set if we are not doing restart and we
		 * are sending a regular file
		 */
		netfd = fileno(outstr);
		filefd = fileno(instr);

		if (isreg && filesize < (off_t)16 * 1024 * 1024) {
			buf = mmap(0, filesize, PROT_READ, MAP_SHARED, filefd,
				   (off_t)0);
			if (buf == (char *)MAP_FAILED) {
				syslog(LOG_WARNING, "mmap(%lu): %m",
				       (unsigned long)filesize);
				goto oldway;
			}
			bp = buf;
			len = filesize;
			do {
				cnt = write(netfd, bp, len);
				len -= cnt;
				bp += cnt;
				if (cnt > 0) byte_count += cnt;
			} while(cnt > 0 && len > 0);

			transflag = 0;
			munmap(buf, (size_t)filesize);
			if (cnt < 0)
				goto data_err;
			reply(226, "Transfer complete.");
			return;
		}

oldway:
		setup_xferbuf(filefd, netfd, SO_SNDBUF);
		while ((cnt = read(filefd, xferbuf, (u_int)xferbufsize)) > 0 &&
		    write(netfd, xferbuf, cnt) == cnt)
			byte_count += cnt;
		transflag = 0;
		if (cnt != 0) {
			if (cnt < 0)
				goto file_err;
			goto data_err;
		}
		reply(226, "Transfer complete.");
		return;
	default:
		transflag = 0;
		reply(550, "Unimplemented TYPE %d in send_data", type);
		return;
	}

data_err:
	transflag = 0;
	perror_reply(426, "Data connection");
	return;

file_err:
	transflag = 0;
	perror_reply(551, "Error on input file");
}

/*
 * Transfer data from peer to "outstr" using the appropriate encapulation of
 * the data subject to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
static int
receive_data(instr, outstr)
	FILE *instr, *outstr;
{
	int c;
	int cnt, bare_lfs;

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return (-1);
	}

	bare_lfs = 0;

	switch (type) {

	case TYPE_I:
	case TYPE_L:
		setup_xferbuf(fileno(outstr), fileno(instr), SO_RCVBUF);
		while ((cnt = read(fileno(instr), xferbuf, xferbufsize)) > 0) {
			if (write(fileno(outstr), xferbuf, cnt) != cnt)
				goto file_err;
			byte_count += cnt;
		}
		if (cnt < 0)
			goto data_err;
		transflag = 0;
		return (0);

	case TYPE_E:
		reply(553, "TYPE E not implemented.");
		transflag = 0;
		return (-1);

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				if (ferror(outstr))
					goto data_err;
				if ((c = getc(instr)) != '\n') {
					(void) putc ('\r', outstr);
					if (c == '\0' || c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, outstr);
	contin2:	;
		}
		fflush(outstr);
		if (ferror(instr))
			goto data_err;
		if (ferror(outstr))
			goto file_err;
		transflag = 0;
		if (bare_lfs) {
			reply(-226,
		"WARNING! %d bare linefeeds received in ASCII mode",
			    bare_lfs);
		(void)printf("   File may not have transferred correctly.\r\n");
		}
		return (0);
	default:
		reply(550, "Unimplemented TYPE %d in receive_data", type);
		transflag = 0;
		return (-1);
	}

data_err:
	transflag = 0;
	perror_reply(426, "Data Connection");
	return (-1);

file_err:
	transflag = 0;
	perror_reply(452, "Error writing file");
	return (-1);
}

void
statfilecmd(filename)
	char *filename;
{
	FILE *fin;
	int c;
	char line[LINE_MAX];

	(void)snprintf(line, sizeof(line), _PATH_LS " -lgA %s", filename);
	fin = ftpd_popen(line, "r");
	reply(-211, "status of %s:", filename);
	while ((c = getc(fin)) != EOF) {
		if (c == '\n') {
			if (ferror(stdout)){
				perror_reply(421, "control connection");
				(void) ftpd_pclose(fin);
				dologout(1);
				/* NOTREACHED */
			}
			if (ferror(fin)) {
				perror_reply(551, filename);
				(void) ftpd_pclose(fin);
				return;
			}
			(void) putc('\r', stdout);
		}
		(void) putc(c, stdout);
	}
	(void) ftpd_pclose(fin);
	reply(211, "End of Status");
}

void
statcmd()
{
	struct sockaddr *sa;
	char buf[128];

	reply(-211, "%s FTP server status:", OP(hostname), version);
	printf("     %s\r\n", version);
	printf("     Connected to %s", remotehost);
	if (!getnameinfo(SA(&his_addr), SA(&his_addr)->sa_len, buf, sizeof(buf),
	    NULL, 0, NI_NUMERICHOST))
		printf(" (%s)", buf);
	printf("\r\n");
	if (logged_in) {
		if (guest)
			printf("     Logged in anonymously\r\n");
		else
			printf("     Logged in as %s\r\n", pw->pw_name);
	} else if (askpasswd)
		printf("     Waiting for password\r\n");
	else
		printf("     Waiting for user name\r\n");
	printf("     TYPE: %s", typenames[type]);
	if (type == TYPE_A || type == TYPE_E)
		printf(", FORM: %s", formnames[form]);
	if (type == TYPE_L)
#if NBBY == 8
		printf(" %d", NBBY);
#else
		printf(" %d", bytesize);	/* need definition! */
#endif
	printf("; STRUcture: %s; transfer MODE: %s\r\n",
	    strunames[stru], modenames[mode]);
	if (data != -1)
		printf("     Data connection open\r\n");
	else if (pdata != -1) {
		printf("     in Passive mode");
		sa = SA(&pasv_addr);
		goto printaddr;
	} else if (usedefault == 0) {
		printf("     PORT");
		sa = SA(&data_dest);
printaddr:
		if (!satosport(buf, sa))
			printf(" (%s)", buf);
		printf("\r\n");
	} else
		printf("     No data connection\r\n");
	reply(211, "End of status");
}

void
fatal(s)
	char *s;
{

	reply(451, "Error in server: %s\n", s);
	reply(221, "Closing connection due to server error.");
	dologout(0);
	/* NOTREACHED */
}


void
reply(int n, const char *fmt, ...)
{
	static char *buf = NULL;
	static int len;
	int r;
	char *bp, *ep, *lc;

	va_list ap;

	/*
	 * Only allow one (multi-line) reply per command (with
	 * an exception for 1yz preliminary replies).
	 * 
	 */
	if (reply_code > 0) {
		/*
		 * We've already sent one reply for this command.
		 * If the previous reply was not a 1xy, or the new
		 * one is a 1xy, ignore the new one.  Otherwise,
		 * pretend we didn't send the preliminary reply.
		 */
		if ((reply_code < 100 || reply_code >= 200) ||
		    (n < 200 && n > -200))
			return;
		reply_code = 0;
	}

	if (buf == NULL && (buf = malloc(len = 256)) == NULL)
		goto failed;

	va_start(ap, fmt);
	r = vsnprintf(buf, len, fmt, ap);
	if (r >= len) {
		free(buf);
		if ((buf = malloc(len = r + 32)) == NULL)
			goto failed;
	}
	vsnprintf(buf, len, fmt, ap);
	va_end(ap);

	if (n < 0) {
		n = -n;
		lc = "-";
	} else
		lc = "";

	for (bp = buf; (ep = strchr(bp, '\n')) != NULL; bp = ep + 1) {
		if (ep[1] == '\0') {
			ep[0] = '\0';
			break;
		}
		if (!nolreply) {
			if (reply_code == 0)
				reply_code = -n;
			printf("%d- %.*s\r\n", n, ep - bp, bp);
			if (FL(DEBUG))
				syslog(LOG_DEBUG, "<--- %d- %.*s", n, ep - bp, bp);
		}
	}
	if (!nolreply || *lc == '\0') {
		/*
		 * FTP says that if we see a code like "123-" then we
		 * must keep sending lines up until a "123 ".  Intermediate
		 * codes do not matter.
		 * Also, as soon as we see a "123-"/"123 " pair, or just
		 * a "123 " that is not bracketed as above, we are done
		 * with our replies for this line.
		 */
		if (reply_code == 0)
			reply_code = *lc ? -n : n;
		else if (*lc == '\0' && reply_code == -n)
			reply_code = n;
		printf("%d%s %s\r\n", n, lc, bp);
		if (FL(DEBUG))
			syslog(LOG_DEBUG, "<--- %d%s %s", n, lc, bp);
	}

	fflush(stdout);
	return;
failed:
	if (reply_code < 0)
		reply_code = -reply_code;
	else
		reply_code = 412;
	printf("%d Local resource failure: malloc\r\n", -reply_code);
	fflush(stdout);
	dologout(1);
}

void
file_reply(int n, const char *file, FILE *fp)
{
	int c;
	char line[1024], *p;

	if (fp == NULL && (file == NULL || (fp = fopen(file, "r")) == NULL))
		return;

	c = getc(fp);
	if (c == EOF) {
		fclose(fp);
		return;
	}
	line[0] = c;
	line[1] = '\0';
	if (c != '\n' && fgets(line + 1, sizeof(line) - 1, fp) == NULL) {
		reply(n, "%c", c);
		return;
	}

	while ((c = getc(fp)) != EOF) {
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';
		if (!nolreply)
			printf("%d- %s\r\n", n < 0 ? -n : n, line);
		line[0] = c;
		line[1] = '\0';
		if (c != '\n' && fgets(line + 1, sizeof(line) - 1, fp) == NULL)
			break;
	}
	reply(n, "%s", line);
}

static void
ack(s)
	char *s;
{

	reply(250, "%s command successful.", s);
}

void
nack(s)
	char *s;
{

	reply(502, "%s command not implemented.", s);
}

/* ARGSUSED */
void
yyerror(s)
	char *s;
{
	char *cp;

	if ((cp = strchr(cbuf,'\n')))
		*cp = '\0';
	reply(500, "'%s': command not understood.", cbuf);
}

void
delete(name)
	char *name;
{
	struct stat st;

	LOGCMD("delete", name);
	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		if (rmdir(name) < 0) {
			perror_reply(550, name);
			return;
		}
		goto done;
	}
	if (unlink(name) < 0) {
		perror_reply(550, name);
		return;
	}
done:
	ack("DELE");
}

void
cwd(path)
	char *path;
{

	if (chdir(path) < 0)
		perror_reply(550, path);
	else {
		file_reply(-250, OP(msgfile), NULL);
		ack("CWD");
	}
}

void
makedir(name)
	char *name;
{
	struct ftpdir *fd;

	LOGCMD("mkdir", name);
	if (storeok(name, &fd, -550) == 0 || (fd && fd->dmode == 0)) {
		reply(550, "%s: permission denied", name);
		return;
	}
	if (mkdir(name, 0777) < 0)
		perror_reply(550, name);
	else {
		if (fd) {
			sigprocmask(SIG_BLOCK, &allsigs, NULL);
			(void) seteuid((uid_t)0);
			(void) chown(name, fd->uid, fd->gid);
			(void) chmod(name, fd->dmode);
			(void) seteuid((uid_t)pw->pw_uid);
			sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
		}
		reply(257, "MKD command successful.");
	}
}

void
removedir(name)
	char *name;
{

	LOGCMD("rmdir", name);
	if (rmdir(name) < 0)
		perror_reply(550, name);
	else
		ack("RMD");
}

void
pwd()
{
	char path[MAXPATHLEN + 1];

	if (getwd(path) == (char *)NULL)
		reply(550, "%s.", path);
	else
		reply(257, "\"%s\" is current directory.", path);
}

char *
renamefrom(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return ((char *)0);
	}
	reply(350, "File exists, ready for destination name");
	return (name);
}

void
renamecmd(from, to)
	char *from, *to;
{
	struct stat st;

	LOGCMD2("rename", from, to);

	if (guest && (stat(to, &st) == 0)) {
		reply(550, "%s: permission denied", to);
		return;
	}

	if (rename(from, to) < 0)
		perror_reply(550, "rename");
	else
		ack("RNTO");
}

static void
dolog(sa)
	struct sockaddr *sa;
{
	if (getnameinfo(sa, sa->sa_len, remotehost, sizeof(remotehost), NULL,
	    0, 0)) {
		syslog(LOG_ERR, "getnameinfo failed, bailing");
		exit(1);
	}
#ifdef SETPROCTITLE
	if (FL(VIRTUAL))
		snprintf(proctitle, sizeof(proctitle), "%s: connected (to %s)",
			 remotehost, OP(hostname));
	else
		snprintf(proctitle, sizeof(proctitle), "%s: connected",
			 remotehost);
	setproctitle("%s", proctitle);
#endif /* SETPROCTITLE */

	if (FL(LOGGING)) {
		if (FL(VIRTUAL))
			syslog(LOG_INFO, "connection from %s (to %s)",
			       remotehost, OP(hostname));
		else

			syslog(LOG_INFO, "connection from %s (to %s)",
			       remotehost, OP(hostname));
	}
}

/*
 * Record logout in wtmp file
 * and exit with supplied status.
 */
void
dologout(status)
	int status;
{
	/*
	 * Prevent reception of SIGURG from resulting in a resumption
	 * back to the main program loop.
	 */
	transflag = 0;

	if (logged_in) {
		sigprocmask(SIG_BLOCK, &allsigs, NULL);
		(void) seteuid((uid_t)0);
		ftpd_logwtmp(ttyline, "", "");
	}
	/* beware of flushing buffers after a SIGPIPE */
	_exit(status);
}

static void
myoob(signo)
	int signo;
{
	char *cp;

	/* only process if transfer occurring */
	if (!transflag)
		return;
	cp = tmpline;
	if (getline(cp, 7, stdin) == NULL) {
		reply(221, "You could at least say goodbye.");
		dologout(0);
	}
	upper(cp);
	if (strcmp(cp, "ABOR\r\n") == 0) {
		tmpline[0] = '\0';
		reply(426, "Transfer aborted. Data connection closed.");
		/*
		 * reply() ignores multiple replies to the same command.
		 * But ABORT is special, it replies with both a 426
		 * and a 226.  Zeroing reply_code defeats the tests in
		 * reply, and gets the second reply out.
		 */
		reply_code = 0;
		reply(226, "Abort successful");
		longjmp(urgcatch, 1);
	}
	if (strcmp(cp, "STAT\r\n") == 0) {
		if (file_size != (off_t) -1)
			reply(213, "Status: %qd of %qd bytes transferred",
			    byte_count, file_size);
		else
			reply(213, "Status: %qd bytes transferred", byte_count);
	}
}

/*
 * Note: a response of 425 is not mentioned as a possible response to
 *	the PASV command in RFC959. However, it has been blessed as
 *	a legitimate response by Jon Postel in a telephone conversation
 *	with Rick Adams on 25 Jan 89.
 */
void
passive(l, af)
	int l, af;
{
	int len;
	char buffer[80];

	if (pdata >= 0)		/* close old port if one set */
		close(pdata);

	pdata = socket(af, SOCK_STREAM, 0);
	if (pdata < 0) {
		perror_reply(425, "Can't open passive connection");
		return;
	}

#ifdef IP_PORTRANGE
	{
	    int on = FL(HIGHPORTS) ? IP_PORTRANGE_HIGH
				   : IP_PORTRANGE_DEFAULT;

	    if (setsockopt(pdata, IPPROTO_IP, IP_PORTRANGE,
			    (char *)&on, sizeof(on)) < 0)
		    goto pasv_error;
	}
#endif

#if IPSEC
	if (request && setsockopt(pdata, SOL_SOCKET, SO_SECURITY_REQUEST,
	    request, requestlen) < 0) {
		syslog(LOG_ERR,
		    "setsockopt(..., SO_SECURITY_REQUEST, ...): %s(%d)\n",
		    strerror(errno), errno);
		goto pasv_error;
	}
#endif /* IPSEC */
	pasv_addr = ctrl_addr;
	switch (SA(&ctrl_addr)->sa_family) {
	case AF_INET:
		SIN(&pasv_addr)->sin_port = 0;
		break;
#if INET6
	case AF_INET6:
		SIN6(&pasv_addr)->sin6_port = 0;
		break;
#endif /* INET6 */
	default:
		goto pasv_error;
	}

	if (bind(pdata, SA(&pasv_addr), SA(&pasv_addr)->sa_len) < 0)
		goto pasv_error;

	len = sizeof(pasv_addr);
	if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
		goto pasv_error;
	if (listen(pdata, 1) < 0)
		goto pasv_error;

	switch (l) {
	case 1:	/* EPSV */
		if (satoeport(buffer, sizeof(buffer), SA(&pasv_addr), 1))
			goto pasv_error;
		reply(229, "Entering Extended Passive Mode (%s)", buffer);
		break;
	case 2: /* PASV */
		if (satoport(buffer, SA(&pasv_addr)))
			goto pasv_error;
		reply(227, "Entering Passive Mode (%s)", buffer);
		break;
	case 3: /* SPSV */
		if (satosport(buffer, SA(&pasv_addr)))
			goto pasv_error;
		reply(227, "Entering Passive Mode (%s)", buffer);	/*XXX*/
		break;
	case 4: /* LPSV */
		if (satolport(buffer, SA(&pasv_addr)))
			goto pasv_error;
		reply(228, "Entering Passive Mode (%s)", buffer);
		break;
	default:
		goto pasv_error;
	}
	if (circuit_serial) {
		circuit_port = ntohs(SIN(&pasv_addr)->sin_port);
		circuit_port = htonl(circuit_port);

	    	if (insert_entry(circuit_serial, &his_addr, &pasv_addr,
		    circuit_port) != 0) {
			circuit_port = 0;
		}
	}
	return;

pasv_error:
#if 0
	(void) seteuid((uid_t)pw->pw_uid);
#endif
	(void) close(pdata);
	pdata = -1;
	perror_reply(425, "Can't open passive connection");
	return;
}

/*
 * Generate unique name for file with basename "local".
 * The file named "local" is already known to exist.
 * Generates failure reply on error.
 */
static char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	struct stat st;
	int count;
	char *cp;

	cp = strrchr(local, '/');
	if (cp)
		*cp = '\0';
	if (stat(cp ? local : ".", &st) < 0) {
		perror_reply(553, cp ? local : ".");
		return ((char *) 0);
	}
	if (cp)
		*cp = '/';
	/* -4 is for the .nn<null> we put on the end below */
	(void) snprintf(new, sizeof(new) - 4, "%s", local);
	cp = new + strlen(new);
	*cp++ = '.';
	for (count = 1; count < 100; count++) {
		(void)snprintf(cp, 4, "%d", count);
		if (stat(new, &st) < 0)
			return (new);
	}
	reply(452, "Unique file name cannot be created.");
	return (NULL);
}

/*
 * Format and send reply containing system error number.
 */
void
perror_reply(code, string)
	int code;
	char *string;
{

	reply(code, "%s: %s.", string, strerror(errno));
}

static char *onefile[] = {
	"",
	0
};

void
send_file_list(whichf)
	char *whichf;
{
	struct stat st;
	DIR *dirp = NULL;
	struct dirent *dir;
	FILE *dout;
	char **dirlist, *dirname;
	glob_t gl;

#define	simple	(gl.gl_pathv == onefile)

	if (strpbrk(whichf, "~{[*?") != NULL) {
		int flags = GLOB_BRACE|GLOB_NOCHECK|GLOB_QUOTE|GLOB_TILDE;

		memset(&gl, 0, sizeof(gl));
		if (glob(whichf, flags, 0, &gl)) {
			reply(550, "not found");
			goto out;
		} else if (gl.gl_pathc == 0) {
			errno = ENOENT;
			perror_reply(550, whichf);
			goto out;
		}
	} else {
		onefile[0] = whichf;
		gl.gl_pathv = onefile;
	}

	if (setjmp(urgcatch)) {
		transflag = 0;
		goto out;
	}

	dirlist = gl.gl_pathv;
	dout = NULL;

	while ((dirname = *dirlist++)) {
		if (stat(dirname, &st) < 0) {
			/*
			 * If user typed "ls -l", etc, and the client
			 * used NLST, do what the user meant.
			 */
			if (dirname[0] == '-' && *dirlist == NULL &&
			    transflag == 0) {
				retrieve(_PATH_LS " %s", dirname);
				goto out;
			}
			perror_reply(550, whichf);
			if (dout != NULL) {
				if (circuit_port) {
					delete_entry(circuit_serial,
					    &his_addr,
					    &pasv_addr, circuit_port);
					circuit_port = 0;
				}
				(void) fclose(dout);
				transflag = 0;
				data = -1;
				pdata = -1;
			}
			goto out;
		}

		if (S_ISREG(st.st_mode)) {
			if (dout == NULL) {
				dout = dataconn("file list", (off_t)-1, "w");
				if (dout == NULL)
					goto out;
				transflag++;
			}
			fprintf(dout, "%s%s\n", dirname,
				type == TYPE_A ? "\r" : "");
			byte_count += strlen(dirname) + 1;
			continue;
		} else if (!S_ISDIR(st.st_mode))
			continue;

		if ((dirp = opendir(dirname)) == NULL)
			continue;

		while ((dir = readdir(dirp)) != NULL) {
			char nbuf[MAXPATHLEN];

			if (dir->d_name[0] == '.' && dir->d_namlen == 1)
				continue;
			if (dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
			    dir->d_namlen == 2)
				continue;

			snprintf(nbuf, sizeof(nbuf), 
				"%s/%s", dirname, dir->d_name);

			/*
			 * We have to do a stat to insure it's
			 * not a directory or special file.
			 */
			if (simple || (stat(nbuf, &st) == 0 &&
			    S_ISREG(st.st_mode))) {
				if (dout == NULL) {
					dout = dataconn("file list", (off_t)-1,
						"w");
					if (dout == NULL)
						goto out;
					transflag++;
				}
				if (nbuf[0] == '.' && nbuf[1] == '/')
					fprintf(dout, "%s%s\n", &nbuf[2],
						type == TYPE_A ? "\r" : "");
				else
					fprintf(dout, "%s%s\n", nbuf,
						type == TYPE_A ? "\r" : "");
				byte_count += strlen(nbuf) + 1;
			}
		}
		(void) closedir(dirp);
	}

	if (dout == NULL)
		reply(550, "No files found.");
	else if (ferror(dout) != 0)
		perror_reply(550, "Data connection");
	else
		reply(226, "Transfer complete.");

	transflag = 0;
	if (dout != NULL)
		(void) fclose(dout);
	if (circuit_port) {
		delete_entry(circuit_serial, &his_addr,
		    &pasv_addr, circuit_port);
		circuit_port = 0;
	}
	data = -1;
	pdata = -1;
out:
	if (!simple)
		globfree(&gl);
}

void
fatalsig(signo)
	int signo;
{
	syslog(LOG_ERR, "fatal signal %d", signo);
#ifdef	ALLOW_CORE_DROPS
	if (FL(DEBUG)) {
		OP(flags) = 0;
		signal(SIGABRT, SIG_DFL);
		setgid(0);
		seteuid(0);
		setuid(0);
		chdir("/tmp");
		abort();
	}
#endif
	_exit(1);
}

void
reapchild(signo)
	int signo;
{
	needreap = 1;
}

void
updatesig(signo)
	int signo;
{
	needupdate = 1;
}

void
statsig(signo)
	int signo;
{
	needstat = 1;
}

void
reapchildren()
{
	struct pidlist *pl, *lpl;
	pid_t pid;

	sigprocmask(SIG_BLOCK, &allsigs, NULL);
	needreap = 0;
	sigprocmask(SIG_UNBLOCK,&allsigs,NULL);

	while ((pid = wait3(NULL, WNOHANG, NULL)) > 0) {
		for (lpl = NULL, pl = pidlist; pl; pl = (lpl = pl)->next)
			if (pl->pid == pid) {
				if (lpl)
					lpl->next = pl->next;
				else
					pidlist = pl->next;
				/*
				 * It is possible that a sessoin will go
				 * away after the virtual host has been
				 * deleted because of an updated config file
				 */
				if (pl->host)
					pl->host->sessions--;
				free(pl);
				break;
			}
	}
}

#ifdef OLD_SETPROCTITLE
/*
 * Clobber argv so ps will show what we're doing.  (Stolen from sendmail.)
 * Warning, since this is usually started from inetd.conf, it often doesn't
 * have much of an environment or arglist to overwrite.
 */
void
#if __STDC__
setproctitle(const char *fmt, ...)
#else
setproctitle(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	int i;
	va_list ap;
	char *p, *bp, ch;
	char buf[LINE_MAX];

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);

	/* make ps print our process name */
	p = Argv[0];
	*p++ = '-';

	i = strlen(buf);
	if (i > LastArgv - p - 2) {
		i = LastArgv - p - 2;
		buf[i] = '\0';
	}
	bp = buf;
	while (ch = *bp++)
		if (ch != '\n' && ch != '\r')
			*p++ = ch;
	while (p < LastArgv)
		*p++ = ' ';
}
#endif /* OLD_SETPROCTITLE */

static void
logxfer(name, size, start, direction)
	char *name;
	long size;
	time_t start;
	int direction;
{
	char buf[1024];
	char path[MAXPATHLEN];
	time_t now;
	int i;
	char *fmt, *p, *q, *n, *bp, *ep;

	if (statfd <= 0)
		return;
	if (name == NULL)
		name = "<NULL>";

	time(&now);

	if (*name == '/')
		*path = 0;
	else if (getcwd(path, sizeof(path)) == NULL)
		strcpy(path, ".");

	i = strlen(path);
	snprintf(path + i, sizeof(path) - i, "/%s", name);
	/*
	 * Deal with pathnames that have multiple /'s
	 */
	name = path;
	while (name[0] == '/' && name[1] == '/')
		++name;

	fmt = OP(tmplogfmt);
	strcpy(fmt, OP(logfmt));

	ep = buf + sizeof(buf);
	bp = buf;

	while (bp < ep && *fmt != '\0') {
		p = fmt - 2;
		do
			p = strchr(p+2, '%');
		while (p != NULL && p[1] == '%');

		if (p == NULL) {
			bp += snprintf(bp, ep - bp, fmt);
			fmt = NULL;
			break;
		}

		if (p == NULL ||
		    (n = strchr(p, '{')) == NULL ||
		    (q = strchr(n++, '}')) == NULL)
			break;
		*q++ = '\0';
		if (strcmp(n, "time") == 0) {
			n[-1] = 's';
			n[0] = '\0';
			p = ctime(&now) + 4;
			if ((n = strchr(p, '\n')) != NULL)
				*n = '\0';
			bp += snprintf(bp, ep - bp, fmt, p);
		} else if (strcmp(n, "duration") == 0) {
			n[-1] = 'l';
			n[0] = 'd';
			n[1] = '\0';
			bp += snprintf(bp, ep - bp, fmt, now - start + (now == start));
		} else if (strcmp(n, "remote") == 0) {
			n[-1] = 's';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, remotehost);
		} else if (strcmp(n, "size") == 0) {
			n[-1] = 'l';
			n[0] = 'd';
			n[1] = '\0';
			bp += snprintf(bp, ep - bp, fmt, size);
		} else if (strcmp(n, "path") == 0) {
			n[-1] = 's';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, name);
		} else if (strcmp(n, "type") == 0) {
			n[-1] = 'c';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, (type == TYPE_A) ? 'a' : 'b');
		} else if (strcmp(n, "action") == 0) {
			n[-1] = 'c';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, '_');
		} else if (strcmp(n, "direction") == 0) {
			n[-1] = 'c';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, direction);
		} else if (strcmp(n, "session") == 0) {
			n[-1] = 'c';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, guest ? 'a' : 'r');
		} else if (strcmp(n, "user") == 0) {
			n[-1] = 's';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, ident);
		} else if (strcmp(n, "authtype") == 0) {
			n[-1] = 'd';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, rfc931user ? 1 : 0);
		} else if (strcmp(n, "authuser") == 0) {
			n[-1] = 's';
			n[0] = '\0';
			bp += snprintf(bp, ep - bp, fmt, rfc931user ? rfc931user : "*");
		} else {
			n[-1] = 's';
			bp += snprintf(bp, ep - bp, fmt, n);
		}
		fmt = q;
	}
	if (bp < ep && fmt != NULL && *fmt != '\0')
		bp += snprintf(bp, ep - bp, "%s", fmt);
	if (bp >= ep)
		bp = ep - 1;
	*bp++ = '\n';
	write(statfd, buf, bp - buf);
}

static int
storeok(char *name, struct ftpdir **pfd, int code)
{
	struct ftpdir *fd;
	struct regexps *re;
	char *slash, *parent;
	struct stat st[8];
	int s, depth;

	*pfd = NULL;
	/*
	 * Guest users are only allowed to store files in directories
	 * marked for incoming.
	 */
	if (guest) {
		slash = strrchr(name, '/');
		depth = 1;
		if (slash) {
			*slash = '\0';
			s = stat(name, st);
			if ((parent = malloc(slash - name + 3*7 + 1)) == NULL) {
                                syslog(LOG_ERR,"memory allocation failure: %m");
				return (0);
			}

			strcpy(parent, name);
			*slash++ = '/';
		} else {
			if ((parent = malloc(1 + 3 * 7 + 1)) == NULL) {
                                syslog(LOG_ERR,"memory allocation failure: %m");
				return (0);
			}
			strcpy(parent, ".");
			slash = name;
			s = stat(".", st);
		}
		/*
		 * Run the pathfilter only on the last component
		 */
		if (s == 0 && (re = OP(pathfilter)) != NULL) {
			if (regexec(&re->preg, slash, 0, NULL, 0) != 0) {
				reply(code, "%s: invalid character in name",
				    slash);
				if (parent)
					free(parent);
				return (0);
			}
			while ((re = re->next) != NULL) {
				if (regexec(&re->preg, slash, 0, NULL, 0) == 0){
					reply(code,
					    "%s: invalid character in name",
					    slash);
					if (parent)
						free(parent);
					return (0);
				}
			}
		}
		if (s == 0) {
			while (depth < 8) {
				strcat(parent, "/..");
				if (stat(parent, &st[depth]) != 0)
					break;
				for (s = 0; s < depth; ++s)
					if (st[depth].st_dev == st[s].st_dev &&
					    st[depth].st_ino == st[s].st_ino)
						break;
				if (s < depth)
					break;
				++depth;
			}
			for (fd = OP(ftpdirs); fd; fd = fd->next) {
				for (s = 0; s < depth; ++s) {
					if (fd->dev == st[s].st_dev &&
					    fd->ino == st[s].st_ino) {
						*pfd = fd;
						if (parent)
							free(parent);
						return (1);
					}
				}
			}
		}
		reply(code, "%s: writes not allowed to this directory", name);
		if (parent)
			free(parent);
		return (0);
	}
	return (1);
}

static char *
rfc931(struct sockaddr *me, struct sockaddr *him)
{
	int s, r, n;
	struct sockaddr_generic sa;
	char *b, *p, buf[256];
	struct servent *se;

	if ((s = socket(him->sa_family, SOCK_STREAM, 0)) < 0)
		return (NULL);
	sa = *(struct sockaddr_generic *)him;
	if ((se = getservbyname("ident", "tcp")) == NULL)
		SIN(&sa)->sin_port = htons(113);
	else
		SIN(&sa)->sin_port = se->s_port;

	if (connect(s, SA(&sa), SA(&sa)->sa_len) < 0) {
		close(s);
		return (NULL);
	}

	sprintf(buf, "%u, %u\r\n", ntohs(SIN(him)->sin_port),
	    ntohs(SIN(me)->sin_port));
	for (b = buf, n = strlen(buf); n > 0; n -= r, b += r) {
		if ((r = write(s, b, n)) <= 0) {
			close(s);
			return (NULL);
		}
	}
	shutdown(s, 1);			/* We are done sending now */
	b = buf;
	n = sizeof(buf) - 1;
	while (n > 0 && (r = read(s, b, n)) > 0) {
		n -= r;
		while (r-- > 0) {
			if (*b == '\r' || *b == '\n')
				break;
			++b;
		}
	}
	close(s);
	while (b > buf && isspace(b[-1]))
		--b;
	*b = '\0';

	if ((p = strchr(b = buf, ',')) == NULL)
		return (NULL);
	*p++ = '\0';
	if (atoi(b) != ntohs(SIN(him)->sin_port))
		return (NULL);

	if ((p = strchr(b = p, ':')) == NULL)
		return (NULL);
	*p++ = '\0';
	if (atoi(b) != ntohs(SIN(me)->sin_port))
		return (NULL);
	while (isspace(*p))
		++p;
	if (strncasecmp(p, "USERID", 6) != 0)
		return (NULL);
	p += 6;
	while (isspace(*p))
		++p;
	if (*p != ':')
		return (NULL);
	if ((p = strchr(p + 1, ':')) == NULL)
		return (NULL);
	while (isspace(*++p))
		;
	return (*p ? strdup(p) : NULL);
}
