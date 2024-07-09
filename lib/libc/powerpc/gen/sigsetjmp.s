/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigsetjmp.s,v 1.2 2001/11/13 23:17:57 donn Exp
 */

#include "DEFS.h"

/*
 * int sigsetjmp(sigjmp_buf env, int savemask);
 *
 * Save register context without saving the signal mask.
 * We don't use ENTRY() because profiling would alter our register state.
 */

	.globl sigsetjmp
	.long 0
	.type sigsetjmp,@function
sigsetjmp:
	stw 4,232(3)		/* save sjb_restore_mask */
	cmpwi 4,0
	bne 1f
	b PLTCALL(_setjmp)
1:
	b PLTCALL(setjmp)
.L.end.sigsetjmp:
	.size sigsetjmp,.L.end.sigsetjmp-sigsetjmp

/*
 * void siglongjmp(sigjmp_buf env, int result);
 *
 * Restore register context without saving the signal mask.
 */

	.globl siglongjmp
	.long 0
	.type siglongjmp,@function
siglongjmp:
	lwz 0,232(3)		/* retrieve sjb_restore_mask */
	cmpwi 0,0
	bne 1f
	b PLTCALL(_longjmp)
1:
	b PLTCALL(longjmp)
.L.end.siglongjmp:
	.size siglongjmp,.L.end.siglongjmp-siglongjmp
