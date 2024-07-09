/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI thread_context.s,v 1.2 2001/10/03 17:29:54 polk Exp
 */

	.file "thread_context.s"

#include "DEFS.h"
#define	ASSEMBLY	1
#include "threads/thread_machdep.h"

#define	REG(off)	(off*4)
#define	FPREG(off)	((_THREAD_POWERPC_NREGS*4)+(off*8))

/*
 * int _thread_machdep_savectx(thread_machdep_state *state);
 */
ENTRY(_thread_machdep_savectx)
	mflr 0
	stw 0,REG(_THREAD_POWERPC_LR)(3)
	stw 1,REG(_THREAD_POWERPC_SP)(3)
	stw 2,REG(_THREAD_POWERPC_TOC)(3)
	stw 13,REG(_THREAD_POWERPC_G_13)(3)
	stw 14,REG(_THREAD_POWERPC_G_14)(3)
	stw 15,REG(_THREAD_POWERPC_G_15)(3)
	stw 16,REG(_THREAD_POWERPC_G_16)(3)
	stw 17,REG(_THREAD_POWERPC_G_17)(3)
	stw 18,REG(_THREAD_POWERPC_G_18)(3)
	stw 19,REG(_THREAD_POWERPC_G_19)(3)
	stw 20,REG(_THREAD_POWERPC_G_20)(3)
	stw 21,REG(_THREAD_POWERPC_G_21)(3)
	stw 22,REG(_THREAD_POWERPC_G_22)(3)
	stw 23,REG(_THREAD_POWERPC_G_23)(3)
	stw 24,REG(_THREAD_POWERPC_G_24)(3)
	stw 25,REG(_THREAD_POWERPC_G_25)(3)
	stw 26,REG(_THREAD_POWERPC_G_26)(3)
	stw 27,REG(_THREAD_POWERPC_G_27)(3)
	stw 28,REG(_THREAD_POWERPC_G_28)(3)
	stw 29,REG(_THREAD_POWERPC_G_29)(3)
	stw 30,REG(_THREAD_POWERPC_G_30)(3)
	stw 31,REG(_THREAD_POWERPC_G_31)(3)
	mfcr 0
	stw 0,REG(_THREAD_POWERPC_CR)(3)

	stfd 14,FPREG(_THREAD_POWERPC_FP_14)(3)
	stfd 15,FPREG(_THREAD_POWERPC_FP_15)(3)
	stfd 16,FPREG(_THREAD_POWERPC_FP_16)(3)
	stfd 17,FPREG(_THREAD_POWERPC_FP_17)(3)
	stfd 18,FPREG(_THREAD_POWERPC_FP_18)(3)
	stfd 19,FPREG(_THREAD_POWERPC_FP_19)(3)
	stfd 20,FPREG(_THREAD_POWERPC_FP_20)(3)
	stfd 21,FPREG(_THREAD_POWERPC_FP_21)(3)
	stfd 22,FPREG(_THREAD_POWERPC_FP_22)(3)
	stfd 23,FPREG(_THREAD_POWERPC_FP_23)(3)
	stfd 24,FPREG(_THREAD_POWERPC_FP_24)(3)
	stfd 25,FPREG(_THREAD_POWERPC_FP_25)(3)
	stfd 26,FPREG(_THREAD_POWERPC_FP_26)(3)
	stfd 27,FPREG(_THREAD_POWERPC_FP_27)(3)
	stfd 28,FPREG(_THREAD_POWERPC_FP_28)(3)
	stfd 29,FPREG(_THREAD_POWERPC_FP_29)(3)
	stfd 30,FPREG(_THREAD_POWERPC_FP_30)(3)
	stfd 31,FPREG(_THREAD_POWERPC_FP_31)(3)
	mffs 0
	stfd 0,FPREG(_THREAD_POWERPC_FPSCR)(3)

	li 3,0
	blr
ENDENTRY(_thread_machdep_savectx)

/*
 * void _thread_machdep_restctx(thread_machdep_state *state, int rval);
 */
ENTRY(_thread_machdep_restctx)
	cmpwi 4,0
	bne+ 0f
	li 4,1				/* always return nonzero */
0:

	lwz 0,REG(_THREAD_POWERPC_LR)(3)
	mtlr 0
	lwz 1,REG(_THREAD_POWERPC_SP)(3)
	lwz 2,REG(_THREAD_POWERPC_TOC)(3)
	lwz 13,REG(_THREAD_POWERPC_G_13)(3)
	lwz 14,REG(_THREAD_POWERPC_G_14)(3)
	lwz 15,REG(_THREAD_POWERPC_G_15)(3)
	lwz 16,REG(_THREAD_POWERPC_G_16)(3)
	lwz 17,REG(_THREAD_POWERPC_G_17)(3)
	lwz 18,REG(_THREAD_POWERPC_G_18)(3)
	lwz 19,REG(_THREAD_POWERPC_G_19)(3)
	lwz 20,REG(_THREAD_POWERPC_G_20)(3)
	lwz 21,REG(_THREAD_POWERPC_G_21)(3)
	lwz 22,REG(_THREAD_POWERPC_G_22)(3)
	lwz 23,REG(_THREAD_POWERPC_G_23)(3)
	lwz 24,REG(_THREAD_POWERPC_G_24)(3)
	lwz 25,REG(_THREAD_POWERPC_G_25)(3)
	lwz 26,REG(_THREAD_POWERPC_G_26)(3)
	lwz 27,REG(_THREAD_POWERPC_G_27)(3)
	lwz 28,REG(_THREAD_POWERPC_G_28)(3)
	lwz 29,REG(_THREAD_POWERPC_G_29)(3)
	lwz 30,REG(_THREAD_POWERPC_G_30)(3)
	lwz 31,REG(_THREAD_POWERPC_G_31)(3)
	lwz 0,REG(_THREAD_POWERPC_CR)(3)
	mtcr 0

	lfd 14,FPREG(_THREAD_POWERPC_FP_14)(3)
	lfd 15,FPREG(_THREAD_POWERPC_FP_15)(3)
	lfd 16,FPREG(_THREAD_POWERPC_FP_16)(3)
	lfd 17,FPREG(_THREAD_POWERPC_FP_17)(3)
	lfd 18,FPREG(_THREAD_POWERPC_FP_18)(3)
	lfd 19,FPREG(_THREAD_POWERPC_FP_19)(3)
	lfd 20,FPREG(_THREAD_POWERPC_FP_20)(3)
	lfd 21,FPREG(_THREAD_POWERPC_FP_21)(3)
	lfd 22,FPREG(_THREAD_POWERPC_FP_22)(3)
	lfd 23,FPREG(_THREAD_POWERPC_FP_23)(3)
	lfd 24,FPREG(_THREAD_POWERPC_FP_24)(3)
	lfd 25,FPREG(_THREAD_POWERPC_FP_25)(3)
	lfd 26,FPREG(_THREAD_POWERPC_FP_26)(3)
	lfd 27,FPREG(_THREAD_POWERPC_FP_27)(3)
	lfd 28,FPREG(_THREAD_POWERPC_FP_28)(3)
	lfd 29,FPREG(_THREAD_POWERPC_FP_29)(3)
	lfd 30,FPREG(_THREAD_POWERPC_FP_30)(3)
	lfd 31,FPREG(_THREAD_POWERPC_FP_31)(3)
	lfd 0,FPREG(_THREAD_POWERPC_FPSCR)(3)
	mtfsf 0xff,0

	mr 3,4
	blr
ENDENTRY(_thread_machdep_restctx)
