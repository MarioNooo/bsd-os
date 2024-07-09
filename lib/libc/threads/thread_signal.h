/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_signal.h,v 1.6 2003/07/08 21:52:13 polk Exp
 */

#ifndef	_THREAD_SIGNALS_H_
#define	_THREAD_SIGNALS_H_

#include <signal.h>

/*
 * Signal handlers are per process so we store a per process sigaction
 * set in the pthreads signal manager. The mask is maintained per thread 
 * so its stored in the pthread structure.  Signals may be posted either 
 * per-process or per-thread if specific to a thread.
 */

#define	IDX(x)	((x) - 1)

extern sigset_t _thread_sig_pending;
extern sigset_t _thread_sig_blocked;
extern struct sigaction *sigvector;		/* The signal vector */

__BEGIN_DECLS
void 	_thread_sig_block __P((void));
void 	_thread_sig_deliver_pending __P((sigset_t, int, struct sigcontext *,
	    int));
void 	_thread_sig_handler __P((int, int, struct sigcontext *));
int 	_thread_sig_init __P((void));
int	_thread_sig_post __P((struct pthread *, int));
void 	_thread_sig_unblock __P((void));
int 	_thread_sig_wakeup __P((int, struct pthread *));
void 	_thread_sig_wakeup_one __P((int));
__END_DECLS

#endif
