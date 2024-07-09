/*
 * Copyright (c) 1993,1994,1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_fork.c,v 2.3 1996/08/20 17:39:24 donn Exp
 */

#include <sys/param.h>
#include <err.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

extern char **environ;

/*
 * SCO emulator support for fork/exec.
 */

pid_t
sco_fork()
{
	pid_t pid;

	pid = fork();

	/* iBCS2 p 3-34 gives EAX/EDX values for parent/child */
	if (pid > 0)
		/* parent: EAX = child pid, EDX = 0 */
		*program_edx = 0;
	else if (pid == 0) {
		/* child: EAX = garbage, EDX = nonzero */
		*program_edx = 1;
	}

	return (pid);
}

int
sco_execve(path, argv, envp)
	const char *path;
	char * const *argv;
	char * const *envp;
{
	int argc;
	char *argv0;
	char **ap;

	environ = (char **)envp;
	if (curdir)
		setenv("EMULATE_PWD", curdir, 1);
	if (argv[0] && argv[0][0] == '.' && argv[0][1] == '/' &&
	    (argv0 = smalloc(strlen(curdir) + strlen(argv[0]) + 1))) {
		/*
		 * WordPerfect 6.0 'xwpinstall' seems to require that
		 * we put a full pathname into argv[0].
		 * How bizarre.
		 */
		for (ap = (char **)argv; *ap; ++ap)
			;
		argc = ap - argv;
		if (ap = smalloc((argc + 1) * sizeof (char *))) {
			bcopy(argv, ap, (argc + 1) * sizeof (char *));
			strcpy(argv0, curdir);
			strcat(argv0, ap[0]);
			ap[0] = argv0;
		}
		execve(path, ap, environ);
		sfree(argv0);
		sfree(ap);
		return (-1);
	}
	return (execve(path, argv, environ));
}

int
sco_execv(path, argv)
	const char *path;
	char * const *argv;
{

	sco_execve(path, argv, environ);
	return (-1);
}
