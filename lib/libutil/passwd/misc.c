/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI misc.c,v 2.4 2003/07/08 21:52:54 polk Exp
 */
#include <sys/types.h>
#include <sys/wait.h>

#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int sigs[] = {
	SIGALRM, SIGHUP, SIGINT, SIGPIPE, SIGQUIT, SIGTERM, SIGTTOU
};
#define NS (sizeof sigs / sizeof *sigs)

/*
 * Block or catch signals.  If actp is not NULL, defaulted and caught signals
 * are delivered to its handler, otherwise the signals are simply blocked.  If
 * setp is not NULL, the set of `normally caught' signals is stored through it.
 */
void
pw_sigs(setp, actp)
	sigset_t *setp;
	struct sigaction *actp;
{
	struct sigaction oact;
	sigset_t set, oset;
	int i;

	(void)sigemptyset(&set);
	for (i = 0; i < NS; i++)
		(void)sigaddset(&set, sigs[i]);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
	if (setp != NULL)
		*setp = set;
	if (actp != NULL) {
		for (i = 0; i < NS; i++) {
			(void)sigaction(sigs[i], actp, &oact);
			if (oact.sa_handler == SIG_IGN)
				(void)sigaction(sigs[i], &oact, NULL);
		}
		(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	}
}

int
pw_prompt()
{
	int c;

	(void)printf("re-edit the password file? [y]: ");
	(void)fflush(stdout);
	c = getchar();
	if (c != EOF && c != '\n')
		while (getchar() != '\n');
	return (c == 'n');
}

int
pw_edit(file)
	char *file;
{
	pid_t pid;
	sigset_t oset, set;
	struct sigaction act, oact;
	int i, pstat;
	char *p, *editor;

	if ((editor = getenv("EDITOR")) == NULL)
		editor = _PATH_VI;
	if ((p = strrchr(editor, '/')) == NULL)
		p = editor;
	else 
		++p;

	/* Block interesting signals in both the parent and child. */
	(void)sigemptyset(&set);
	for (i = 0; i < NS; i++)
		(void)sigaddset(&set, sigs[i]);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (!(pid = vfork())) {
		/* Interrupt should be normal in the child. */
		act.sa_handler = SIG_DFL;
		(void)sigaction(SIGINT, &act, NULL);

		/* Unblock signals in the child. */
		(void)sigprocmask(SIG_SETMASK, &oset, NULL);

		(void)setgid(getgid());
		(void)setuid(getuid());

		execlp(editor, p, file, NULL);
		_exit(1);
	}

	/*
	 * The parent shouldn't exit if the child is interrupted.  Note that
	 * a blocked + ignored signal is immediately ignored, so it is safe
	 * to leave SIGINT blocked here.
	 */
	act.sa_handler = SIG_IGN;
	(void)sigaction(SIGINT, &act, &oact);

	pid = waitpid(pid, (int *)&pstat, 0);

	/* Reset the parent's interrupt handler. */
	(void)sigaction(SIGINT, &oact, NULL);

	/* Unblock signals in the parent. */
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);

	return (pid == -1 || !WIFEXITED(pstat) || WEXITSTATUS(pstat) != 0);
}
