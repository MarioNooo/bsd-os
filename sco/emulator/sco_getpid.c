/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_getpid.c,v 2.1 1995/02/03 15:14:28 polk Exp
 */

/*
 * Support for process/pgrp ops
 */

#include <sys/param.h>
#include <err.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

/* iBCS2 p 3-35 */
#define	SCO_GETPGRP	0
#define	SCO_SETPGRP	1
#define	SCO_SETPGID	2
#define	SCO_SETSID	3

pid_t
sco_getpid()
{

	/* iBCS2 p 3-35 documents the EDX hack */
	*program_edx = getppid();
	return (getpid());
}

int
#ifdef __STDC__
sco_ptrace(int c, pid_t pid, caddr_t v, int data)
#else
sco_ptrace(c, pid, v, data)
	int c, data;
	pid_t pid;
	caddr_t v;
#endif
{

	errx(1, "unsupported ptrace system call");
}

int
#ifdef __STDC__
sco_pgrpctl(int c, int x, pid_t pid, pid_t pgrp)
#else
sco_pgrpctl(c, x, pid, pgrp)
	int c, x;
	pid_t pid, pgrp;
#endif
{

	switch (c) {

	case SCO_GETPGRP:
		return (getpgrp());

	case SCO_SETPGRP:
		return (setpgid(0, 0));

	case SCO_SETPGID:
		return (setpgid(pid, pgrp));

	case SCO_SETSID:
		return (setsid());

	default:
		errx(1, "unsupported pgrp system call");
	}
}

int
sco_plock(c)
	int c;
{

	errx(1, "unsupported plock system call");
}
