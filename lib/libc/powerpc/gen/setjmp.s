/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI setjmp.s,v 1.4 2001/11/13 23:17:57 donn Exp
 */

#include "SYS.h"

/*
 * int setjmp(jmp_buf env);
 *
 * Save register context and signal mask.
 */

#define	SIG_BLOCK		1	/* XXX */
#define	SIG_SETMASK		3	/* XXX */

ENTRY(setjmp)
	/*
	 * We make the sigprocmask() system call in-line so that
	 * we don't change any more register state than we have to.
	 */
	mr 10,3
	li 4,0
	li 3,SIG_BLOCK
	li 0,SYS_sigprocmask
	sc
	stw 3,12(10)		/* save jb_mask */

	mr 3,10
	b PLTCALL(_setjmp)
ENDENTRY(setjmp)

/*
 * void longjmp(jmp_buf env, int result);
 *
 * Restore register context and signal mask.
 */

ENTRY(longjmp)
	mr 10,3
	mr 9,4
	lwz 4,12(10)		/* retrieve jb_mask */
	li 3,SIG_SETMASK
	li 0,SYS_sigprocmask
	sc
	/* take any newly unblocked signals here... */

	mr 3,10
	mr 4,9
	b PLTCALL(_longjmp)
ENDENTRY(longjmp)
