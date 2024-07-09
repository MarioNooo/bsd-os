/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI poppassd.c,v 1.3 1996/07/31 23:10:29 prb Exp
 */

/*
 * A Eudora and NUPOP change password server.
 *
 * Totally rewritten from an earlier version by Roy Smith
 * <roy@nyu.edu> and Daniel L. Leavitt <dll.mitre.org> as modified by
 * John Norstad <j-norstad@nwu.edu>.
 * 
 */
 
/* Steve Dorner's description of the simple protocol:
 *
 * The server's responses should be like an FTP server's responses; 
 * 1xx for in progress, 2xx for success, 3xx for more information
 * needed, 4xx for temporary failure, and 5xx for permanent failure.  
 * Putting it all together, here's a sample conversation:
 *
 *   S: 200 hello\r\n
 *   E: user yourloginname\r\n
 *   S: 300 please send your password now\r\n
 *   E: pass yourcurrentpassword\r\n
 *   S: 200 My, that was tasty\r\n
 *   E: newpass yournewpassword\r\n
 *   S: 200 Happy to oblige\r\n
 *   E: quit\r\n
 *   S: 200 Bye-bye\r\n
 *   S: <closes connection>
 *   E: <closes connection>
 */
 
#define VERSION "BSDI v1.0"

#define	PASSWD_PROG		"passwd"
#define	PASSWD_PATH		"/usr/bin/"PASSWD_PROG
#define	PASSWD_ARGS(user)	PASSWD_PATH, PASSWD_PROG, "-l", user

#define	TIMEOUT_PASSWD	60
#define	TIMEOUT_NETWORK	60

#define	CODE_INPROGRESS	100
#define	CODE_OK		200
#define	CODE_MOREINFO	300
#define	CODE_TMPFAIL	400
#define	CODE_ERR	500

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <libpasswd.h>
#include <login_cap.h>

#if	__STDC__
#include <stdarg.h>
#else
#include <varags.h>
#endif

#include <paths.h>


/* States for the passwd response engine */
#define	STATE_NC	0	/* No change in state */
#define	STATE_OLD	1	/* Waiting for prompt of old password */
#define	STATE_NEW	2	/* Waiting for prompt of new password */
#define	STATE_RENEW	3	/* Waiting for reprompt of new password */
#define	STATE_WAIT	4	/* Waiting for indication it's done */
#define	STATE_DONE	5	/* Successful completion */
#define	STATE_FAIL	6	/* Unknown Failure */
#define	STATE_TMPFAIL	7	/* Temporary failure (password file is locked)  */
#define	STATE_REJECT	8	/* Too short, all lower case */
#define	STATE_ERROR	9	/* System error */

#define	STATE(s)	(1 << s)
#define	STATE_ALL	0xffffffff

struct response {
	u_int	r_oldstates;		/* Expected states */
	int 	r_newstate;		/* New state */
	int	(*r_trans)(int);	/* Transition routine */
	const	char *r_string;		/* Expected string */
	regex_t	r_preg;			/* Precompiled regexp */
} ;


static int dflag;
static int state = STATE_OLD;
static int child_done;
static int net_timeout;
static pid_t child_pid;
static char *user;
static char *oldpass;
static char *newpass;
static const char *peername = "";
static char *progname;
static char response_buf[BUFSIZ];
static char tty_name[PATH_MAX];
static struct timeval net_wait = { TIMEOUT_NETWORK, 0 };
static struct timeval pass_wait = { TIMEOUT_PASSWD, 0 };


static void	check_child __P((pid_t));
static char	*clean_buf __P((char *));
static int	client_read __P((const char *, const char *, char **));
static void	client_write __P((int, const char *, ...));
static void	client_write_srverr __P((const char *, ...));
#ifdef	DEBUG
static void	debug __P((char *, ...));
#endif
static void	erase_passwords __P((void));
static void	notify __P((int, const char *, ...));
static int	process_response __P((int));
static int	send_n __P((int));
static int	send_newpass __P((int));
static int	send_oldpass __P((int));
static void	sig_alarm __P((void));
static void	sig_child __P((void));
static int	writestring __P((int, const char *));


static struct response responses[] = {
	/* Errors from child - %s is filled in with progname */
	{ STATE(STATE_OLD),
		  STATE_ERROR,	NULL, "^%s: (.+)$" },

	/* An information message, ignore it */
	{ STATE(STATE_OLD),
		  STATE_NC,	NULL,
		  "^(Changing (local )?password for .*)$" },

	/* Prompt for old password */
	{ STATE(STATE_OLD),
		  STATE_NEW,	send_oldpass,
		  "^(([^ ]*'s )?Old password:)$" },

	/* Prompt for new password */	
	{ STATE(STATE_NEW),
		  STATE_RENEW,	send_newpass,
		  "^(([^ ]*'s )?New password[^:]*: ?)$" },

	/* Re-prompt for new password */	
	{ STATE(STATE_RENEW),
		  STATE_WAIT,	send_newpass,
		  "^(((Re-)?Enter)|(Retype) ([^ ]*'s)?new password: ?)$" },

	/* Did not like your password */
	{ STATE(STATE_RENEW)|STATE(STATE_WAIT),
		  STATE_REJECT,	NULL,
		  "^(Please .*)$" },

	/* A progress message */
	{ STATE(STATE_WAIT),
		  STATE_NC,	NULL,
		  "^(passwd: (rebuilding|updating) (the )?(passwd )?database.*)$" },

	/* A temporary error */	
	{ STATE(STATE_WAIT),
		  STATE_TMPFAIL,	NULL,
		  "^(The password database is currently locked\\.)$" },

	/* Prompt to wait for database to be released */
	{ STATE(STATE_WAIT),
		  STATE_DONE,	send_n,
		"^(Wait for lock to be released\\? \\[y\\] )$" },

	/* Success! */
	{ STATE(STATE_WAIT),
		  STATE_DONE,	NULL,
		  "^(passwd: done)$" },

	{ 0, 0, NULL }
};


/*
 * Implements the Eudora POP password change protocol.  Reads the user
 * identification, old and new passwords from the network and invokes the
 * passwd program to make the change.  Description of protocol is above.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct passwd *pw;
	int master;
	struct response *rp;
	struct sockaddr_in peer;
	struct hostent *hp;
	struct sigaction sigact;
	login_cap_t *lc;
	char *cap;
	int ch;

	progname = strrchr(argv[0], '/');
	if (!progname)
		progname = argv[0];

	cap = NULL;
	user = strdup("unknown");

	while ((ch = getopt(argc, argv, "dr:t:w:")) != EOF) {
		switch(ch) {
#ifdef	DEBUG
		case 'd':
			/* Debug */
			dflag = 1;
			break;
#endif	/* DEBUG */
	     
		case 'r':
			/* Login.conf atribute required */
			cap = optarg;
			break;

		case 't':
			/* Network timeout */
			net_wait.tv_sec = strtoul(optarg, (char **) 0, 10);
			if (net_wait.tv_sec < 1) {
				client_write_srverr("-%c %s: invalid",
						    ch, optarg);
			}
			break;

		case 'w':
			/* Password response timeout */
			pass_wait.tv_sec = strtoul(optarg, (char **) 0, 10);
			if (pass_wait.tv_sec < 1) {
				client_write_srverr("-%c %s: invalid",
						    ch, optarg);
			}
			break;
	     
		case '?':
		default:
			client_write_srverr("unknown switch: `%c`", ch);
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1) {
		/* We dont' take any arguents */
		client_write_srverr("unknown argument: \"%s\"",
				    argv[argc]);
		/*NOTREACHED*/
	}

	/* Compile the regular expressions */
	for (rp = responses; rp->r_string; rp++) {
		int rc;

		/* Fix strings that match on our program name */
		if (strstr(rp->r_string, "%s")) {
			(void) snprintf(response_buf, sizeof response_buf,
					rp->r_string,
					progname);
			rp->r_string = strdup(response_buf);
		}

		rc = regcomp(&rp->r_preg, rp->r_string,
			     REG_EXTENDED|REG_ICASE);
		if (rc) {
			(void) regerror(rc, &rp->r_preg,
					response_buf, sizeof response_buf);
			client_write_srverr("%s", response_buf);
			/*NOTREACHED*/
		}
	}

	/* Set the output to be unbuffered */
	if (setvbuf(stdout, NULL, _IONBF, 0) < 0) {
		client_write_srverr("unable to set buffering: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}

	/* Set syslog parameters */
	openlog(progname, LOG_PID, LOG_AUTHPRIV);

	/* Build an identification string for syslog */
	if (!dflag) {
		int len;

		/* Identify the remote end of the socket */
		len = sizeof peer;
		if (getpeername(fileno(stdout),
				(struct sockaddr *) &peer, &len) < 0) {
			client_write_srverr("Unable to get client address: %s.",
					    strerror(errno));
			/*NOTREACHED*/
		}

		/* Try to lookup the hostname associated with the remote address */
		hp = gethostbyaddr((char *) &peer.sin_addr,
				   sizeof peer.sin_addr,
				   AF_INET);
		if (hp) {
			/* Found it, use in identifier */

			(void) snprintf(response_buf, sizeof response_buf,
					"%s:%u",
					hp->h_name,
					ntohs(peer.sin_port));
		} else {
			/* Could not find hostname, use IP address */

			(void) snprintf(response_buf, sizeof response_buf,
					"%s:%u",
					inet_ntoa(peer.sin_addr),
					ntohs(peer.sin_port));
		}
	} else {
		/* Fake an identifier for test mode */

		strcat(response_buf, "test");
	}
	peername = strdup(response_buf);

	/* Set a signal handler to catch network timeout */
	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = sig_alarm;
	if (sigaction(SIGALRM, &sigact, (struct sigaction *) 0) < 0) {
		client_write_srverr("unable to set handler for SIGARLM: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}

	/* Send banner and try to read USER response */
	client_write(CODE_OK,
		     "poppassd %s hello, who are you?",
		     VERSION);
	setproctitle("%s expect USER", peername);
	if (client_read("USER", "Username", &user)) {
		return 1;
	}

	/* Try to read old password response */
	client_write(CODE_OK, "your password please.");
	setproctitle("%s expect PASS", peername);
	if (client_read("PASS", "Password", &oldpass)) {
		erase_passwords();
		return 1;
	}

	/* Try to read new password response */
	client_write(CODE_OK, "your new password please.");
	setproctitle("%s expect NEWPASS", peername);
	if (client_read("NEWPASS", "New password", &newpass)) {
		erase_passwords();
		return 1;
	}

	/* Lookup the user in the password database */
	if ((pw = getpwnam(user)) == NULL) {
		client_write(CODE_ERR,
			     "Unknown user, %s.", user);
		erase_passwords();
		return 1;
	}
	/* Close the database */
	endpwent();

	/* Get login class */
	lc = login_getclass(pw ? pw->pw_class : NULL);
	if (!lc) {
		client_write_srverr("unable to get login class");
		/*NOTREACHED*/
	}
	if (cap) {
		int ok;

		/* A login class capability is required, see if it is present */
		ok = login_getcapbool(lc, cap, FALSE);
		if (!ok) {

			/* Not present, too bad... */
			client_write(CODE_ERR,
				     "%s",
				     strerror(EACCES));
			erase_passwords();
			return 1;
		}
	}

	/* Verify old password */
	if (strcmp(crypt(oldpass, pw->pw_passwd), pw->pw_passwd) != 0) {
		client_write(CODE_ERR,
			     "Old password is incorrect.");
		erase_passwords();
		return 1;
	}

	/* Fork a process with a pty */
	setproctitle("%s forking", peername);
	if ((child_pid = forkpty(&master, tty_name, NULL, NULL)) < 0) {
		client_write_srverr("unable to fork: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}
	if (child_pid == 0) {
		struct termios termios;

		/* Child */

		setproctitle("%s child", peername);

		/* Set some terminal attributes */
		if (tcgetattr(fileno(stdin), &termios) < 0) {
			err(1, "unable to get attributes of %s",
			    tty_name);
			/*NOTREACHED*/
		}
		termios.c_lflag &= ~(ECHO|ECHONL);
		termios.c_oflag &= ~(ONLCR);
		if (tcsetattr(fileno(stdin), TCSANOW, &termios) < 0) {
			err(1, "unable to set attributes of %s",
			    tty_name);
			/*NOTREACHED*/
		}

		/*
		 * Become the user trying who's password is being changed.
		 * We're about to exec passwd which is setuid root anyway,
		 * but this way it looks to the child completely like it's
		 * being run by the normal user, which makes it do its own
		 * password verification before doing any thing.  In theory,
		 * we've already verified the password, but this extra level
		 * of checking doesn't hurt.  Besides, the way I do it here,
		 * if somebody manages to change somebody else's password,
		 * you can complain to your vendor about security holes, not
		 * to me!
		 */
		if (setusercontext(lc,
				   pw,
				   pw->pw_uid,
				   LOGIN_SETALL & ~LOGIN_SETPATH) < 0) {
			err(1, "unable to set user context");
			/*NOTREACHED*/
		}

		/* Exec the passwd program. */
		if (execl(PASSWD_ARGS(user), NULL) < 0) {
			err(1, "unable to exec %s",
			    PASSWD_PATH);
			/*NOTREACHED*/
		}

		/*NOTREACHED*/
		abort();
	}

	/* Parent */

	/* Set a signal handler to catch the child termination signal */
	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = sig_child;
	if (sigaction(SIGCHLD, &sigact, (struct sigaction *) 0) < 0) {
		client_write_srverr("unable to set handler for SIGCHLD: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}

	*response_buf = (char) 0;
	for (;;) {
		int nfds;
		fd_set read_fds;
		struct timeval tv;

		/* Wait for input from the child */
		FD_ZERO(&read_fds);
		FD_SET(master, &read_fds);
		tv = pass_wait;	/* struct copy */
		nfds = select(master + 1,
			      &read_fds, (fd_set *) 0, (fd_set *) 0,
			      &tv);
		if (nfds < 0) {
			switch (errno) {
			case EINTR:
				if (child_done) {
					check_child(child_pid);
					continue;
				}
				break;

			default:
				break;
			}
			client_write_srverr("select failed: %s",
					    strerror(errno));
			/*NOTREACHED*/
		}

		if (nfds == 0) {
			/* Timer expired */
			client_write_srverr("no response from %s in %u seconds",
					    PASSWD_PROG,
					    pass_wait.tv_sec);
			/*NOTREACHED*/
		}

		if (nfds > 0) {
			int newstate;

			if (!FD_ISSET(master, &read_fds)) {
				client_write_srverr("select unknown fd",
						    strerror(errno));
				/*NOTREACHED*/
			}

			/* Read and act on response */
			newstate = process_response(master);

			/* State has changed */
			if (state != newstate) {
				switch (newstate) {
				case STATE_NC:
					/* No change in state */
					continue;

				case STATE_OLD:
					/* Received prompt for old password */
					setproctitle("%s %s passwd -old",
						     peername);
					break;

				case STATE_NEW:
					/* Received prompt for new password */
					setproctitle("%s passwd -new",
						     peername);
					break;

				case STATE_RENEW:
					/* Received re-prompt for new password */
					setproctitle("%s passwd -repeat",
						     peername);
					break;

				case STATE_WAIT:
					/* Waiting for completion */
					setproctitle("%s passwd -done",
						     peername);
					break;
		     
				case STATE_DONE:
					/* Update done */
					setproctitle("%s expect QUIT",
						     peername);
					goto Done;

				case STATE_FAIL:
					/* Failure */
					setproctitle("%s passwd -fail",
						     peername);
					client_write(CODE_ERR,
						     "%s",
						     clean_buf(response_buf));
					erase_passwords();
					return 1;
		     
				case STATE_TMPFAIL:
					/* Failure */
					setproctitle("%s passwd -tmpfail",
						     peername);
					client_write(CODE_TMPFAIL,
						     "%s",
						     clean_buf(response_buf));
					erase_passwords();
					return 1;
		     
				case STATE_REJECT:
					/* Failure */
					setproctitle("%s passwd -reject",
						     peername);
					client_write(-CODE_ERR,
						     "%s",
						     clean_buf(response_buf));
					erase_passwords();
					return 1;
		     
				case STATE_ERROR:
					/* A fatal error occured */
					setproctitle("%s passwd -error",
						     peername);
					client_write_srverr("%s",
							    response_buf);
					/*NOTREACHED*/

				default:
					/* Should not happen! */
					abort();
					/*NOTREACHED*/
				}

				/* Enter new state */
				state = newstate;
			}
		}	    
	}

 Done:
	/* Delete the passwords */
	erase_passwords();
     
	/*
	 * Adding the close(master) will keep the program waiting for
	 * passwd to die if there is some additional unread output
	 * that we're not expect()ing.  
	 */
	if (close(master) < 0) {
		client_write_srverr("close failed: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}

	notify(LOG_INFO, "password changed");
	client_write(CODE_OK, "Password changed, thank-you.");
     
	if (client_read("QUIT", NULL, NULL)) {
		return 1;
	}

	return 0;
}


/* Called when the child terminates */
void
sig_child()
{
	child_done = 1;
}

/* Verify that the child terminated gracefully */
static void
check_child(pid)
	pid_t pid;
{
	int wstat;
	pid_t wpid;

	child_done = 0;
	     
	if ((wpid = waitpid(pid, &wstat, WNOHANG)) < 0) {
		client_write_srverr("wait failed: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}
	if (pid != wpid) {
		client_write_srverr("wrong child");
		/*NOTREACHED*/
	}
	if (WIFSIGNALED(wstat)) {
		client_write_srverr("terminated by SIG%s",
				    sys_signame[WTERMSIG(wstat)]);
		/*NOTREACHED*/
	}
	if (WIFSTOPPED(wstat)) {
		client_write_srverr("stopped by SIG%s",
				    sys_signame[WTERMSIG(wstat)]);
		/*NOTREACHED*/
	}
	if (WEXITSTATUS(wstat) != 0) {
		client_write_srverr("abnormal exit: %d",
				    WEXITSTATUS(wstat));
		/*NOTREACHED*/
	}
}


/*
 * Read output from the passwd program and try to match it against the
 * list regular expressions.  A match may result in a state
 * transition.
 */
static int
process_response(fd)
	int fd;
{
	int rc;
	static int len;
	struct response *rp;
	regmatch_t resp[2];

	/* First read the response and deal with errors */
	rc = read(fd, response_buf + len, BUFSIZ - 1 - len);
	if (rc < 0) {
		switch (errno) {
		case EINTR:
			if (child_done) {
				check_child(child_pid);
				return STATE_NC;
			}
			break;

		default:
			break;
		}
		(void) snprintf(response_buf, sizeof response_buf,
				"read error: %s",
				strerror(errno));
		return STATE_ERROR;
	} else if (rc == 0) {
		(void) snprintf(response_buf, sizeof response_buf,
				"end-of-file from passwd");
		return STATE_ERROR;
	}

	/* Add this response to the length and nul-terminate the buffer */
	len += rc;
	response_buf[len] = (char) 0;

 Repeat:
	/* Eat any leading newlines */
	rc = strspn(response_buf, "\n\r");
	if (rc == len)
		return STATE_NC;
	if (rc)
		resp[0].rm_so = rc;
	else
		resp[0].rm_so = 0;
	rc = strcspn(response_buf + resp[0].rm_so, "\n\r");
	if (rc)
		resp[0].rm_eo = resp[0].rm_so + rc;
	else
		resp[0].rm_eo = len;

	/* Scan the regular expressions for those that apply in this state */
	for (rp = responses; rp->r_string; rp++) {

		if (!(rp->r_oldstates && STATE(state)))
			continue;

		/* Found one that applies, try it */
		rc = regexec(&rp->r_preg, response_buf, 2, resp, REG_STARTEND);
		switch (rc) {
		case 0:
			/* It matches! */

			/* Convert from quad type */
			rc = resp[1].rm_eo;

			switch (rp->r_newstate) {
			case STATE_ERROR:
			case STATE_TMPFAIL:
			case STATE_REJECT:
				rc = resp[1].rm_eo - resp[1].rm_so;

				/* And extract the interesting part of the message */
				if (resp[1].rm_so > 0)
					strncpy(response_buf,
						response_buf + resp[1].rm_so,
						rc);

				/* Trim trailing whitespace */
				for (rc--; isspace(response_buf[rc]); rc--)
					;
				response_buf[++rc] = (char) 0;

				return rp->r_newstate;

#ifdef	REPORT_PROGRESS
			case STATE_NC:
				client_write(CODE_INPROGRESS,
					     "%.*s",
					     rc, response_buf);
				break;
#endif
			default:
				break;
			}

			/*
			 * Calculate matched length and copy any
			 * remaining data to the beginning of the
			 * buffer.
			 */
			len -= rc;
			if (len)
				strcpy(response_buf, response_buf + rc);
			else
				*response_buf = (char) 0;

			/* Invoke state transition routine if present */
			if (rp->r_trans) {
				rc = rp->r_trans(fd);
				if (rc < 0)
					return STATE_ERROR;
			}
			if (len && rp->r_newstate == STATE_NC)
				goto Repeat;
			
			return rp->r_newstate;

		case REG_NOMATCH:
			continue;

		default:
			(void) regerror(rc, &rp->r_preg,
					response_buf, sizeof response_buf);
			return STATE_ERROR;
		}
	}

	/* We should have matched any valid response that ends in a newline */
	if (strchr(response_buf + resp[0].rm_so, '\n')) {
		return STATE_FAIL;
	}

	return STATE_NC;
}


/* State transition routine to send a negative response */
static int
send_n(master)
	int master;
{
	return writestring(master, "n");
}


/* State transition routine to send the old password */
static int
send_oldpass(master)
	int master;
{
	return writestring(master, oldpass);
}


/* State transision routine to send the new password */
static int
send_newpass(master)
	int master;
{
	return writestring(master, newpass);
}


/*
 * Write a string in a single writev() system call.  Writev() is used
 * to append a newline w/o copying An error results in a description
 * of the error being left in response_buf.
 */
static struct iovec iov[2] = {
	{ NULL, 0 },
	{ (char *) "\n", 1 }
} ;

static int
writestring(fd, s)
	int fd;
	const char *s;
{
	iov[0].iov_base = (char *) s;
	iov[0].iov_len = strlen(s);

	if (writev(fd, iov, 2) < 0) {
		(void) snprintf(response_buf, sizeof response_buf,
				"writing to passwd program: %s",
				strerror(errno));
		return -1;
	}

	return 0;
}


/* Syslog a message with a given priority */
static void
#if	__STDC__
notify(int pri, const char *fmt, ...)
#else
notify(pri, fmt, va_alist)
	int pri;
	char *fmt;
#endif
{
	va_list ap;
	char line[BUFSIZ];

#if	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void) vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);

	if (dflag) {
		warn("%d: [%d] %s@%s: %s",
		     pri, getpid(), user, peername, line);
	} else {
		syslog(pri, "%s@%s: %s",
		       user, peername, line);
	}
}


#ifdef	DEBUG
/* Used for debugging */
static void
#if	__STDC__
debug(char *fmt, ...)
#else
debug(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char line[BUFSIZ];

#if	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
    
	(void) vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);

	if (dflag) {
		warn("DEBUG [%d] %s@%s: %s",
		     getpid(), user, peername, line);
	} else {
		syslog(LOG_DEBUG, "DEBUG %s@%s: %s",
		       user, peername, line);
	}
}
#endif	/* DEBUG */


/*
 * Write a response to the client.  If it is an error response Also
 * call notify to syslog the response.
 */
static void
#if	__STDC__
client_write(int code, const char *fmt, ...)
#else
client_write(code, fmt, va_alist)
	int code;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char line[BUFSIZ];

#if	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void) vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);

	(void) printf("%d %s\r\n", (code > 0) ? code : -code, line);

	/* Syslog the message if it is an error */
	switch (code) {
	case CODE_TMPFAIL:
	case CODE_ERR:
		notify(LOG_ERR, "%d %s", code, line);
		break;

	default:
		break;
	}
}

/* Write a ``Server error'' message and exit */
static void
#if	__STDC__
client_write_srverr(const char *fmt, ...)
#else
client_write_srverr(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	char line[BUFSIZ];

#if	__STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void) vsnprintf(line, sizeof line, fmt, ap);
	va_end(ap);

	(void) printf("%u Server error (%s), contact System Administrator!\r\n",
		      CODE_ERR, line);
	notify(LOG_ERR, "Server error: %s", line);

	erase_passwords();
	exit(1);
}


/* Called when the alarm fires */
void
sig_alarm()
{
	net_timeout = 1;
}



/*
 * Read a response from the network (with timeout) and match it
 * against what we expect to receive.  Return a copy of the provided
 * data in the specified location.  Recognizes QUIT at any point.
 */
static int
client_read(keyword, desc, data)
	const char *keyword;
	const char *desc;
	char **data;
{
	int rc = 0;
	char *sp, *lp;
	size_t len;
	size_t kwl;
	struct itimerval itv;

	kwl = strlen(keyword);

	/* Set wait timer */
	memset(&itv, 0, sizeof itv);
	itv.it_value = net_wait;	/* struct copy */
	if (setitimer(ITIMER_REAL, &itv, (struct itimerval *) 0) < 0) {
		client_write_srverr("unable to set client timer: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}

	for (;;) {
		lp = fgetln(stdin, &len);
		if (lp == NULL) {
			if (feof(stdin)) {
				notify(LOG_ERR,
				       "end-of-file reading from client");
			} else if (ferror(stdin)) {
				switch (errno) {
				case EINTR:
					if (net_timeout) {
						client_write(CODE_ERR,
					"no response from client in %u seconds",
							     net_wait.tv_sec);
						break;
					}
					if (child_done) {
						check_child(child_pid);
						continue;
					}
					/* Fall through */

				default:
					notify(LOG_ERR,
					       "error reading from client: %s",
					       strerror(errno));
					break;
				}
			} else {
				notify(LOG_ERR,
				       "unknown error reading from client");
			}
			rc = 1;
			goto Return;
		}
		break;
	}

	/* Trim trailing LF & CF characters and nul-terminate */
	if (lp[len - 1] == '\n') {
		lp[--len] = (char) 0;
		if (lp[len - 1] == '\r') {
			lp[--len] = (char) 0;
		}
	} else {
		lp[len] = (char) 0;
	}
	sp = lp + kwl;

	if (strncasecmp(lp, "QUIT", 4) == 0) {
		client_write(CODE_OK, "Bye.");
		rc = 1;
		goto Return;
	}

	if (strncasecmp(lp, keyword, kwl)) {
		/* Not the keyword we expected */

		client_write(CODE_ERR, "%s expected", keyword);

		/* Clear input in case it contains a password */
		memset(lp, 0, len);
		rc = 1;
		goto Return;
	}

	if (data) {
		if (*sp++ != ' ' || !strlen(sp)) {
			/* Not space seperated, or no data */

			client_write(CODE_ERR, "%s required", desc);
			/* Clear input in case it contains a password */
			memset(lp, 0, len);
			rc = 1;
			goto Return;
		}
		if (*data)
			free(*data);
		*data = strdup(sp);
	}

 Return:
	/* Clear input in case it contains a password */
	memset(lp, 0, len);

	/* Reset timer */
	itv.it_value.tv_sec = 0;
	if (setitimer(ITIMER_REAL, &itv, (struct itimerval *) 0) < 0) {
		client_write_srverr("unable to reset client timer: %s",
				    strerror(errno));
		/*NOTREACHED*/
	}
    
	return rc;
}


/* Zero any memory containing the passwords */
static void
erase_passwords()
{
	if (oldpass) {
		memset(oldpass, 0, strlen(oldpass));
		free(oldpass);
	}

	if (newpass) {
		memset(newpass, 0, strlen(newpass));
		free(newpass);
	}
}


/* Translate controls to spaces and trim whitespace */
static char *
clean_buf(buf)
	char *buf;
{
	char *sp;
	
	/* Translate any controls to spaces */
	for (sp = buf; *sp; sp++) {
		if (iscntrl(*sp)) {
			*sp = ' ' ;
		}
	}

	/* Trim any trailing spaces */
	for (--sp; *sp == ' '; sp--) {
		*sp = (char) 0;
	}

	/* Skip any leading spaces */
	for (sp = buf; isspace(*sp); sp++)
		;

	return sp;
}
