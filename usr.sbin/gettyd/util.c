/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI util.c,v 1.6 1999/01/04 22:45:00 chrisk Exp
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ucred.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "daemon.h"

#define	STALL_TIMEOUT	30

extern int numberfds;
extern sigset_t blockedsigs;
void setsig(int sig, void (*func)());

pid_t
callscript(int pfd, fd_set *pfds, int ctty, argv_t *avt)
{
	char *prog;
	pid_t pid;
	int i;

	prog = avt->argv[0];

	if ((avt->argv[0] = strrchr(prog, '/')) != NULL)
		avt->argv[0]++;
	else
		avt->argv[0] = prog;

	switch (pid = fork()) {
	case -1:
		pid = 0;
	default:
		break;
	case 0:
		for (i = 0; i < numberfds; ++i)
			if (!FD_ISSET(i, pfds) && i != pfd)
				close(i);
		if (pfd >= 0) {
			dup2(pfd, 0);
			dup2(pfd, 1);
			dup2(pfd, 2);
			if (!FD_ISSET(pfd, pfds))
				close(pfd);
		}
		(void) sigemptyset(&blockedsigs);
		sigprocmask(SIG_SETMASK, &blockedsigs, 0);
		for (i = 0; i < NSIG; ++i)
			setsig(i, SIG_DFL);
		setsid();
		fcntl(0, F_SETFL, 0);
		if (ctty)
			ioctl(ctty, TIOCSCTTY, 0);
		execv(prog, avt->argv);
		err(1, prog);
	}
	return (pid);
}

void
execscript(argv_t *avt)
{
	char *prog;
	int i;

	prog = avt->argv[0];

	if ((avt->argv[0] = strrchr(prog, '/')) != NULL)
		avt->argv[0]++;
	else
		avt->argv[0] = prog;

	(void) sigemptyset(&blockedsigs);
	sigprocmask(SIG_SETMASK, &blockedsigs, 0);
	for (i = 0; i < NSIG; ++i)
		setsig(i, SIG_DFL);
	execv(prog, avt->argv);
	err(1, prog);
}

void
setsig(int sig, void (*func)())
{
	struct sigaction sa;

	sa.sa_handler = func;
	sa.sa_mask = blockedsigs;
	sa.sa_flags = (sig == SIGCHLD) ? SA_NOCLDSTOP : 0;

	sigaction(sig, &sa, 0);
}

/*
 * Construct an argument vector from a command line.
 * Up to 8 additional arguments can be specified.
 */
char **
construct_argv(char *command, ...)
{
	static const char separators[] = " \t";
	int argc;
	char *copy, **argv;
        va_list ap;
	int ac;

	argc = (strlen(command) + 1) / 2 + 1 + 8;
	copy = malloc(strlen(command) + 1 + argc * sizeof (char *));

	if (copy == NULL)
		return (NULL);

	argv = (char **)copy;

	copy += argc * sizeof(char *);
    	strcpy(copy, command);
	argc = 0;

	while ((argv[argc] = strsep(&copy, separators)) != NULL)
		if (argv[argc][0])
			++argc;
	if (argc == 0) {
		free(argv);
		return (NULL);
	}

        va_start(ap, command);
	ac = 0; 
        while (ac < 8 && (argv[argc + ac] = va_arg(ap, char *)))
                ++ac;
	va_end(ap);

	argv[argc + ac] = 0;
	return (argv);
}

void
sendmessage(struct linent *le, char *fmt, ...)
{
	char buf[1024];
	int r;
        va_list ap;
        va_start(ap, fmt);

	strcpy(buf, "message:");
        r = vsnprintf(buf + 8, sizeof(buf) - 9, fmt, ap);
	r += 8;
	buf[r] = '\n';
        va_end(ap);
	write(le->le_rfd, buf, r + 1);
}


void
senderror(struct linent *le, char *fmt, ...)
{
	char buf[1024];
	int r;
        va_list ap;
        va_start(ap, fmt);

	strcpy(buf, "error:");
        r = vsnprintf(buf + 6, sizeof(buf) - 7, fmt, ap);
	r += 6;
	buf[r] = '\n';
        va_end(ap);
	write(le->le_rfd, buf, r + 1);
}

/*
 * Log a message and sleep for a while (to give someone an opportunity
 * to read it and to save log or hardcopy output if the problem is chronic).
 * NB: should send a message to the session logger to avoid blocking.
 */
void
stall(char *fmt, ...)
{
        va_list ap;
	time_t now;
	char buf[64];

        va_start(ap, fmt);

	if (!daemon_mode) {
		time(&now);
		strftime(buf, sizeof(buf), "%b %d %T ", localtime(&now));
		printf("%s", buf);
		vprintf(fmt, ap);
		printf("\n");
	} else
		vsyslog(LOG_NOTICE, fmt, ap);

        va_end(ap);
        sleep(STALL_TIMEOUT);
}

void
warning(char *fmt, ...)
{
	va_list ap;
	time_t now;
	char buf[64];

	va_start(ap, fmt);
	if (!daemon_mode) {
		time(&now);
		strftime(buf, sizeof(buf), "%b %d %T ", localtime(&now));
		printf("%s", buf);
		vprintf(fmt, ap);
		printf("\n");
	} else
		vsyslog(LOG_WARNING, fmt, ap);
	va_end(ap);
}

void
info(char *fmt, ...)
{
	va_list ap;
	time_t now;
	char buf[64];
	FILE *fp;

	va_start(ap, fmt);
	if (!daemon_mode) {
		time(&now);
		strftime(buf, sizeof(buf), "%b %d %T ", localtime(&now));
		printf("%s", buf);
		vprintf(fmt, ap);
		printf("\n");
	} else if (actfile && (fp = fopen(actfile, "a"))) {
		vfprintf(fp, fmt, ap);
		fprintf(fp, "\n");
		fclose(fp);
	} else
		vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
}

void
dprintf(char *fmt, ...)
{
	va_list ap;
	time_t now;
	char buf[64];
	FILE *fp;

	if (!debug)
		return;

	va_start(ap, fmt);
	if (!daemon_mode) {
		time(&now);
		strftime(buf, sizeof(buf), "%b %d %T ", localtime(&now));
		printf("%s", buf);
		vprintf(fmt, ap);
		printf("\n");
	} else if (logfile && (fp = fopen(logfile, "a"))) {
		time(&now);
		strftime(buf, sizeof(buf), "%b %d %T ",
		    localtime(&now));
		fprintf(fp, "%s", buf);
		vfprintf(fp, fmt, ap);
		fprintf(fp, "\n");
		fclose(fp);
	} else
		vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
}

void
infocall(char *name, argv_t *avt)
{
	char buf[1024];
	char *b = buf;
	int ac;
	int n;

	if (!verbose && !debug)
		return;

	for (ac = 0; ac < avt->argc; ++ac) {
		n = strlen(avt->argv[ac]);
		if (b + n + 2 < buf + sizeof(buf)) {
			if (ac > 0)
				*b++ = ' ';
			strcpy(b, avt->argv[ac]);
			while (*b)
				++b;
		} else {
			if (b + 1 < buf + sizeof(buf))
				*b++ = ' ';
			if (b + 1 < buf + sizeof(buf))
				*b++ = '.';
			if (b + 1 < buf + sizeof(buf))
				*b++ = '.';
			if (b + 1 < buf + sizeof(buf))
				*b++ = '.';
			break;
		}
	}
	*b = 0;

	/*
	 * Since dprintf only prints if debug is turned on, make sure
	 * debug is turned on.  Reset it back once we are done.
	 */
	n = debug;
	debug = 1;
	dprintf("%s:time=%d:command=%s:", name, time(0), buf);
	debug = n;
}

int
devopen(char *path, int mode)
{
	char name[MAXPATHLEN];
	int fd;

	snprintf(name, sizeof(name), "/dev/%s", path);
	if ((fd = open(name, mode)) >= 0)
		fcntl(fd, F_SETFL, 0);

	return(fd);
}

int
devrevoke(char *path, uid_t owner, gid_t group, mode_t mode)
{
	char name[MAXPATHLEN];

	snprintf(name, sizeof(name), "/dev/%s", path);
	chown(name, owner, group);
	chmod(name, mode);
	return(revoke(name));
}

void
devchall(char *path, uid_t owner, gid_t group, mode_t mode)
{
	char name[MAXPATHLEN];

	snprintf(name, sizeof(name), "/dev/%s", path);
	chown(name, owner, group);
	chmod(name, mode);
}

char *
nstrdup(char *s)
{
	if (s)
		return(strdup(s));
	if ((s = malloc(1)) != NULL)
		*s = 0;
	return(s);
}

/*
 * splice string s onto end of *sp using 0377 as a separator
 * return location where s was placed.
 */
char *
strsplice(char **sp, char *s)
{
	int olen = 0;
	int tlen = 0;
	char *n;
	if (*sp)
		tlen = (olen = strlen(*sp)) + 1;
	tlen += strlen(s) + 1;
	if ((n = malloc(tlen)) != NULL) {
		if (*sp) {
			memcpy(n, *sp, olen);
			free(*sp);
			*sp = n;
			n += olen;
			*n++ = '\377';
		} else
			*sp = n;
		strcpy(n, s);
	}
	return(n);
}
