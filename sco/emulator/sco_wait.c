/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_wait.c,v 2.1 1995/02/03 15:16:10 polk Exp
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <string.h>
#include "emulate.h"
#include "sco.h"
#include "sco_ops.h"
#include "sco_sig_state.h"

/*
 * Support for waitpid() emulation.
 */

/* The EFLAGS hack is documented in iBCS2 p 3-37. */

/* XXX these flag bits should be in <machine/foo.h> for some foo... */
#define	PF	0x004
#define	ZF	0x040
#define	SF	0x080
#define	OF	0x800

#define	SCO_WAITPID_EFLAGS		(PF|ZF|SF|OF)

pid_t
sco_waitpid(pid, status, options)
	int pid;
	int *status;
	int options;
{
	pid_t r;

	if ((*program_eflags & SCO_WAITPID_EFLAGS) != SCO_WAITPID_EFLAGS) {
		status = (int *)pid;
		pid = -1;
		options = 0;
	}

	if ((r = commit_wait4(pid, status, options, 0)) != -1 && status)
		*program_edx = *status;

	return (r);
}
