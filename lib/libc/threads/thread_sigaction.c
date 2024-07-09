/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_sigaction.c,v 1.4 2003/07/08 21:52:13 polk Exp
 */

#include <sys/types.h>

#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_private.h"
#include "thread_trace.h"

#define	IDX(x)	((x) - 1)
/*
 * _thread_sys_sigaction:
 *
 * Description:
 * 	Thread aware sigaction call. This is user callable and so will
 * 	set the kernel pre-emption monitor lock.
 */
int
_thread_sys_sigaction(sig, nact, oact)
	int 	sig;
	const 	struct sigaction * nact;
	struct 	sigaction * oact;
{
	struct 	sigaction ours;

	TR("_thread_sys_sigaction", sig, nact);

	/* Check if the signal number is out of range: */
	if ((sig < 1) || (sig > NSIG) ||
	    ((nact != NULL) && ((sig == SIGKILL) || (sig == SIGSTOP)))) {
		errno = EINVAL;
		return (-1);
	}

	_thread_kern_enter();

	/* Initialise the global signal action structure */
	SIGFILLSET(ours.sa_mask);
	SIGDELSET(ours.sa_mask, SIGTRAP);      /* For debuggers */
	ours.sa_flags = SA_RESTART;
	ours.sa_handler = (void (*)()) _thread_sig_handler;

	/* Return the current signal vector settings */
	if (oact != NULL)
		*oact = sigvector[IDX(sig)];

	if (nact != NULL) {
                if ((sig != SIGALRM) && (sig != SIGCHLD) &&
		   (sig != SIGKILL) && (sig != SIGSTOP)) {
			/* reset to default/ignore */
			if ((nact->sa_handler == SIG_IGN) || 
			   (nact->sa_handler == SIG_DFL))
				(void) _syscall_sys_sigaction(sig, nact, NULL);
			else /* set to ours */
				(void) _syscall_sys_sigaction(sig, &ours, NULL);
		}
		sigvector[IDX(sig)] = *nact;
	}

	_thread_kern_exit();

	return (0);
}
