/*-
 * Copyright (c) 1992, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sched.c,v 2.7 1999/01/04 03:13:09 donn Exp
 */

/*-
 * sched.c
 *
 * Function:	Arrange to run processes who's time has arrived
 *
 * Author:	Tony Sanders
 * Date:	08/17/92
 *
 * Remarks:
 * History:	08/17/92 Tony Sanders -- creation
 */
 
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <login_cap.h>

#include "pathnames.h"
#include "at.h"
#include "when.h"
#include "errlib.h"

static	int	atrun_clean = 0;
static	char	atrun_name[MAXPATHLEN];
static  void	(*int_func)();
static  void	(*quit_func)();
static  void	(*term_func)();

static  void
atrun_cleanup()
{
	if (atrun_clean == 0)
		return;                 /* don't call exit from an atexit() */
	atrun_clean = 0;

	Pmsg(("atrun cleaning up!!\n"));
	if (*atrun_name) {
		seteuid(0);
		unlink(atrun_name);
	}
	exit(1);
}

static void
Flock()
{
#define	LOCKMSG_LEN 30		/* big enough for pid and short msg */
	char lockmsg[LOCKMSG_LEN];
	int atrun_fd;

	if (atexit(atrun_cleanup) < 0)
		Perror("unable to register atrun cleanup function");
	int_func = signal(SIGINT, atrun_cleanup);
	quit_func = signal(SIGQUIT, atrun_cleanup);
	term_func = signal(SIGTERM, atrun_cleanup);

	snprintf(atrun_name, MAXPATHLEN, "%s%s", AT_DIR, AT_RUNLOCK);
	seteuid(0);
	if ((atrun_fd = open(atrun_name, O_WRONLY|O_CREAT|O_EXCL, 0600)) < 0)
		Perror("could not lock %s", atrun_name);
	seteuid(getuid());
	/*
	 * This isn't atomic but close enough for gvmt work.
	 * We don't want to enable atrun_cleanup until we have the
	 * lock.  So we set this flag AFTER we open the file.
	 */
	atrun_clean = 1;

	snprintf(lockmsg, LOCKMSG_LEN, "locked %d\n", getpid());
	write(atrun_fd, lockmsg, strlen(lockmsg)+1);
	close(atrun_fd);
#undef LOCKMSG_LEN
}

static void
Funlock()
{
	seteuid(0);
	if (unlink(atrun_name) < 0)
		Pwarn("could not unlock %s", atrun_name);
	seteuid(getuid());
	atrun_clean = 0;                      /* turn off cleanup on exit */
	signal(SIGINT, int_func);
	signal(SIGQUIT, quit_func);
	signal(SIGTERM, term_func);
}

static void
Fchild()
{
	atrun_clean = 0;	/* don't remove lock when child exits */
}

static void
runjob(JobQ p)
{
	char *envp[] = { NULL, };
	struct passwd *pwd;

	Pmsg(("running job %d: %d\n", p->id, getpid()));

	if ((pwd = getpwuid(p->owner)) == NULL)
		Perror("Can't get passwd entry for %d", p->owner);

	if (chdir(AT_TMP) < 0)
		Perror("Can't chdir to %s", AT_TMP);

	/* stdin */
	close(0);
	seteuid(p->owner);	/* paranoia */
	if (open(p->fn, O_RDONLY, 0) != 0)
		Perror("Can't open %s", p->fn);

	/* stdout */
	close(1);
	seteuid(0);
	if (open(outputpath(p), O_WRONLY|O_CREAT, 0600) != 1)
		Perror("Can't open", outputpath(p));
	if (fchown(1, p->owner, p->gid) == -1)
		Perror("Can't chown", outputpath(p));

	/* stderr */
	close(2);
	dup2(1, 2);

	/* If we aren't already a session leader, become one.  */
	setsid();

	if (setusercontext(0, pwd, p->owner,
	    LOGIN_SETRESOURCES | LOGIN_SETPRIORITY |
	    LOGIN_SETUMASK | LOGIN_SETLOGIN) < 0)
		Perror("setusercontext failed for %s", pwd->pw_name);

	setgid(p->gid);
	setgroups(p->ngroups, p->groups);
	setuid(p->owner);
	if (auth_approve(0, pwd->pw_name, "at") <= 0)
		Perror("approval failure for %s\n", pwd->pw_name);
	execle(_PATH_BSHELL, "sh", NULL, envp);
	_exit(127);
}

static int
nonzero(char *fn)
{
	struct stat st;
	if (stat(fn, &st) < 0)
		Perror("stat(%s)", fn);
	return st.st_size != 0;
}

static void
notifyuser(JobQ p)
{
	char *outf = outputpath(p);
	char cmd[ARG_MAX];
	char *uname = strdup(shellesc(user_from_uid(p->owner, 0)));

	if (uname == NULL)
		Perror("Can't allocate memory for user name");
	if (p->notify || nonzero(outf)) {
		snprintf(cmd, ARG_MAX,
			"/usr/bin/mail -s 'at job %d' '%s' < '%s' %s",
			p->id, uname, shellesc(outf), ">/dev/null 2>&1");
		system(cmd);
	}
	free(uname);
}

static void
bgjob(JobQ p)
{
	pid_t	child;
	int	status;

	signal(SIGCHLD, SIG_DFL);

	Pmsg(("bgjob(%d) %d\n", p->id, getpid()));

	switch (child = fork()) {
	    case -1:
		Perror("2nd fork");
	    case 0:
		runjob(p);
	    default:
		waitpid(child, &status, 0);
		Pmsg(("return status %d\n", status));
		notifyuser(p);
		removeitem(p);
		_exit(0);
	}
	_exit(127);
}

static void
lockjob(JobQ p)
{
	char	*to;

	Pmsg(("locking job %d\n", p->id));

	mkdir(AT_DIR AT_LOCKDIR, 0700);
	errno = 0;

	to = xfpath(p);
	if (rename(p->fn, to) < 0)
		Perror("locking %s", to);
	free(p->fn);
	p->fn = strdup(to);
	if (p->fn == NULL)
		Perror("strdup");
}

void
scheduler()
{
	JobQ p, head;
	time_t now;

	signal(SIGCHLD, SIG_IGN);

	Pmsg(("scheduler()\n"));

	if (time(&now) < 0)
		Perror("cannot get current time");

	head = readqueue();
	Flock();

	for (p = head->next; p; p = p->next) {
		if (! p->valid) {		/* bad file format */
			rename(p->fn, bfpath(p));
			continue;
		}
		if (p->when > now)		/* not time yet */
			continue;
		Pmsg(("scheduling %d : %d\n", p->id, getpid()));
		lockjob(p);		/* prevent double runs */
		switch (fork()) {
		    case -1:
			Perror("fork");
		    case 0:
			Fchild();		/* undo locking in child */
			bgjob(p);
			break;
		    default:
			sleep(1);		/* pause between jobs */
			break;
		}
	}

	Funlock();
	exit(0);
}
