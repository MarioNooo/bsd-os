/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI authsrv.c,v 1.1 1997/08/27 17:11:16 prb Exp
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <login_cap.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "include.h"

#define	IBS	64
#define	BS	(IBS * 3 + 16)

static void process(char *);

char	*style;
char	*service;
char	*class;
char	*user;
int	start;
int	master;
static int	done;
static pid_t	pid;
static void	*a;

char **av;
int ac;
int mac;

static int backchan[2];

static void exitpid();
static void addarg(char *, char *);

void
main(int argc, char **argv)
{
	struct timeval tv;
	int r, nfds, status;
	char line[MAXPATHLEN];
	char *myclass, *data, *ib, *p;
	char ibuf[IBS + sizeof(long)];
	char buf[BS];
	fd_set rset;
	login_cap_t *lc;

	ib = &ibuf[sizeof(long)-1];	/* magic to make ioctl pkts work */
	data = ib + 1;

	myclass = "AUTHSRV";
chdir("/krystal/prb/src/auth");

	mac = 16;
	ac = 1; /* leave room for arg0 */
	if ((av = malloc(sizeof(char *) * mac)) == NULL) {
		syslog(LOG_ERR, "%m");
		exit(1);
	}

	while ((r = getopt(argc, argv, "c:t:T:")) != EOF) {
		switch (r) {
		case 'c':
			myclass = optarg;
			break;
		case 'T':
			traceall = 1;
			/* FALL THRU to 't' */
		case 't':
			if (trace) {
				syslog(LOG_ERR, "trace file already open");
				exit(1);
			}
			if ((trace = fopen(optarg, "a")) == NULL) {
				syslog(LOG_ERR, "%s: %m", optarg);
				exit(1);
			}
			break;
		default:
			syslog(LOG_ERR, "usage: authsrv [-c class] [-[tT] tracefile]");
			exit(1);
		}
	}

	if ((lc = login_getclass(myclass)) == NULL) {
		syslog(LOG_ERR, "%s: no such class", myclass);
		exit(1);
	}

	a = start_auth(0);
	master = -1;

	for (start = 0; start == 0;) {
		do {
			buf[0] = '\0';
			if (read_auth(a, buf, sizeof(buf)) < 0) {
				syslog(LOG_ERR, "reading data from client");
				exit(1);
			}
			if (buf[0])
				process(buf);
		} while (read_auth(a, NULL, 0));
	}
	if (style == NULL || user == NULL) {
		syslog(LOG_ERR, "both STYLE and USERNAME are required");
		exit(1);
	}

	/*
	 * The "auth" style implies we should use the default style
	 */
	if (strcmp(style, "auth") == 0)
		style = NULL;

        if ((p = login_getstyle(lc, style, "auth-authsrv")) == NULL) {
		if (style == NULL)
			style = "auth";
		syslog(LOG_ERR, "%s: invalid style", style);
		if (strlen(style) > IBS - 22)
			style[IBS-22] = '\0';
		p = buf;
		p += sprintf(p, "FD1: ");
		encodestring(p, style, strlen(style));
		strcat(p, ": invalid style\n");
		if (send_auth(a, buf) < 0) {
			syslog(LOG_ERR, "sending data to client: %m");
			exit(1);
		}
		exit(1);
        }
	login_close(lc);
	style = p;

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, backchan) < 0) {
		syslog(LOG_ERR, "%m");
		err(1, NULL);
	}

	master = GetPty(line);
	r = 1;
        if (ioctl(master, TIOCPKT, (char *)&r))
		syslog(LOG_ERR, "TIOCPKT: %m");

	switch (pid = fork()) {
	case -1:
		err(1, NULL);
	case 0:
		GetPtySlave(line, backchan[1]);
		snprintf(buf, BS, _PATH_AUTHPROG "%s", style);
		style = strrchr(buf, '/') + 1;
		av[0] = style;
		if (service)
			addarg("-s", service);
		addarg(user, NULL);
		if (class)
			addarg(class, NULL);
		execv(buf, av);
		syslog(LOG_ERR, "%s: %m", buf);
		exit(1);
	default:
		atexit(exitpid);
	}
	close(backchan[1]);

	nfds = ((master > backchan[0]) ? master : backchan[0]) + 1;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	for (;;) {
		FD_ZERO(&rset);
		FD_SET(0, &rset);
		if (master >= 0)
			FD_SET(master, &rset);
		if (backchan[0] >= 0)
			FD_SET(backchan[0], &rset);

		switch (select(nfds, &rset, 0, 0, done ? &tv : 0)) {
		case -1:
			if (errno != EINTR) {
				syslog(LOG_ERR, "%m");
				exit(1);
			}
			break;
		case 0:
			if (done) {
				if (WIFEXITED(status))
					sprintf(buf, "EXIT: %d\n", WEXITSTATUS(status));
				else
					sprintf(buf, "EXIT: 1\n");
				send_auth(a, buf);
				exit(0);
			}
			syslog(LOG_ERR, "select returned 0?");
			exit(1);
		}
		if (FD_ISSET(0, &rset)) {
			do {
				buf[0] = '\0';
				if (read_auth(a, buf, sizeof(buf)) < 0) {
					syslog(LOG_ERR,
					    "reading data from client: %m");
					exit(1);
				}
				if (buf[0])
					process(buf);
			} while (read_auth(a, NULL, 0));
		}
		if (backchan[0] >= 0 && FD_ISSET(backchan[0], &rset)) {
			if ((r = read(backchan[0], data, IBS)) < 0) {
				syslog(LOG_ERR, "reading backchannel: %m");
				exit(1);
			}
			if (r > 0) {
				p = buf;
				p += sprintf(p, "FD3: ");
				encodestring(p, data, r);
				if (send_auth(a, buf) < 0) {
					syslog(LOG_ERR, "sending data to client: %m");
					exit(1);
				}
			} else
				backchan[0] = -1;
		}
		if (master >= 0 && FD_ISSET(master, &rset)) {
			if ((r = read(master, ib, IBS)) < 0) {
				syslog(LOG_ERR, "reading data from master: %m");
				exit(1);
			}
			if (r > 0 && ib[0]) {
				struct termios *t = (struct termios *)(data);
				if (ib[0] == 0x40 &&
				    r - 1 == sizeof(struct termios)) {
					if (t->c_lflag & ECHO)
						send_auth(a, "ECHO: ON\n");
					else
						send_auth(a, "ECHO: OFF\n");
				}
			} else if (--r > 0) {
				p = buf;
				p += sprintf(p, "FD1: ");
				encodestring(p, ib+1, r);
				if (send_auth(a, buf) < 0) {
					syslog(LOG_ERR, "sending data to client: %m");
					exit(1);
				}
			} else {
				master = -1;
			}
		}
		if (!done && waitpid(pid, &status, WNOHANG) == pid) {
			pid = 0;
			done = 1;
		}
	}
}

static void
addarg(char *a1, char *a2)
{
	/*
	 * always make sure we have room for two arguments
	 */
	if (ac >= mac - 2) {
		mac += 16;
		if ((av = realloc(av, sizeof(char *) * mac)) == NULL) {
			syslog(LOG_ERR, "%m");
			exit(1);
		}
	}
	if (a1)
		av[ac++] = a1;
	if (a2 && (av[ac++] = strdup(a2)) == NULL) {
		syslog(LOG_ERR, "%m");
		exit(1);
	}
	av[ac] = NULL;
}

static void
process(char *cmd)
{
	char *data;
	int r;

	if (data = strchr(cmd, ' '))
		*data++ = '\0';

	if (strcmp(cmd, "START:") == 0)
		start = 1;
	else if (strcmp(cmd, "VARG:") == 0) {
		if (start) {
			syslog(LOG_ERR, "VARG after START");
			exit(1);
		}
		if ((r = decodestring(data)) < 0) {
			syslog(LOG_ERR, "Invalid VARG data");
			exit(1);
		}
		data[r] = '\0';
		if (r == 0 || strlen(data) < r) {
			syslog(LOG_ERR, "Invalid VARG data");
			exit(1);
		}
		addarg("-v", data);
	} else if (strcmp(cmd, "CLASS:") == 0) {
		if (!cleanstring(data) || *data == '-') {
			syslog(LOG_ERR, "Invalid CLASS field");
			exit(1);
		}
		if (start || (class = strdup(data)) == NULL) {
			syslog(LOG_ERR, "%m");
			exit(1);
		}
	} else if (strcmp(cmd, "SERVICE:") == 0) {
		if (!cleanstring(data) || *data == '-') {
			syslog(LOG_ERR, "Invalid SERVICE field");
			exit(1);
		}
		if (start || (service = strdup(data)) == NULL) {
			syslog(LOG_ERR, "%m");
			exit(1);
		}
	} else if (strcmp(cmd, "STYLE:") == 0) {
		if (!cleanstring(data) || strchr(data, '/')) {
			syslog(LOG_ERR, "Invalid STYLE field");
			exit(1);
		}
		if (start || (style = strdup(data)) == NULL) {
			syslog(LOG_ERR, NULL);
			exit(1);
		}
	} else if (strcmp(cmd, "USER:") == 0) {
		if (!cleanstring(data) || *data == '-') {
			syslog(LOG_ERR, "Invalid USER field");
			exit(1);
		}
		if (start || (user = strdup(data)) == NULL) {
			syslog(LOG_ERR, "%m");
			exit(1);
		}
	} else if (strcmp(cmd, "FD0:") == 0) {
		if (data == NULL || master < 0)
			return;
		if ((r = decodestring(data)) < 0) {
			syslog(LOG_ERR, "garbage from client");
			exit(1);
		}
		if (r)
			write(master, data, r);
		else
			write(master, "\004", 1);
	} else if (strcmp(cmd, "FD3:") == 0) {
		if (data == NULL || master < 0)
			return;
		if ((r = decodestring(data)) < 0) {
			syslog(LOG_ERR, "garbage from client");
			exit(1);
		}
		if (r)
			write(backchan[0], data, r);
	} else if (cleanstring(cmd))
		syslog(LOG_ERR, "unknown command from client: %s", cmd);
	else {
		syslog(LOG_ERR, "garbage from client");
		exit(1);
	}
}

static void
exitpid()
{
	if (!done) {
		char buf[16];
		sprintf(buf, "EXIT: 1\n");
		send_auth(a, buf);
	}
	if (pid) {
		syslog(LOG_ERR, "killing %d", pid);
		kill(pid, SIGKILL);
	}
	
}
