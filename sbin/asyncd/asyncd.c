/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI asyncd.c,v 2.1 1995/10/09 09:38:08 torek Exp
 */

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

int
main(argc, argv)
	int argc;
	char **argv;
{
	pid_t pid;
	int i;
	int count = 1;

	if (argc > 2 || argc == 2 && !isdigit(argv[1][0]))
		errx(1, "usage: asyncd [count]");

	if (geteuid())
		errx(1, "you must be the superuser to run this program");

	if (argc == 2)
		count = atoi(argv[1]);

	for (i = 0; i < count; ++i) {
		if ((pid = fork()) == -1)
			err(1, "");
		if (pid == 0) {
			if (daemon(0, 0) == -1)
				err(1, "child");
			syscall(SYS_asyncdaemon);
			syslog(LOG_DAEMON|LOG_ERR|LOG_PID, "%m");
			exit(1);
		}
		wait(0);
	}

	exit(0);
}
