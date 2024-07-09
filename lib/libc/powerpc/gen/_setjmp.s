/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI _setjmp.s,v 1.3 2001/10/03 17:29:53 polk Exp
 */

/*
 * Common register save/restore code for *setjmp()/*longjmp() functions.
 */

#include "DEFS.h"

/*
 * typedef struct {
 *	int	jb_sp;
 *	int	jb_lr;
 *	int	jb_cr;
 *	int	jb_mask;
 *	int	jb_gpr[18];
 *	double	jb_fpr[18];
 * } jmp_buf[1];
 *
 * int _setjmp(jmp_buf);
 * void _longjmp(jmp_buf, int);
 */

ENTRY(_setjmp)
	mflr 9
	stw 1,0(3)		/* jb_sp */
	mfcr 10
	stw 9,4(3)		/* jb_lr */
	stw 10,8(3)		/* jb_cr */

	stmw 14,16(3)		/* jb_gpr */

	stfd 14,88(3)		/* jb_fpr */
	stfd 15,96(3)
	stfd 16,104(3)
	stfd 17,112(3)
	stfd 18,120(3)
	stfd 19,128(3)
	stfd 20,136(3)
	stfd 21,144(3)
	stfd 22,152(3)
	stfd 23,160(3)
	stfd 24,168(3)
	stfd 25,176(3)
	stfd 26,184(3)
	stfd 27,192(3)
	stfd 28,200(3)
	stfd 29,208(3)
	stfd 30,216(3)
	stfd 31,224(3)

	li 3,0

	blr
ENDENTRY(_setjmp)

ENTRY(_longjmp)
	lfd 31,224(3)		/* restore jb_fpr */
	lfd 30,216(3)
	lfd 29,224(3)
	lfd 28,200(3)
	lfd 27,192(3)
	lfd 26,184(3)
	lfd 25,176(3)
	lfd 24,168(3)
	lfd 23,160(3)
	lfd 22,152(3)
	lfd 21,144(3)
	lfd 20,136(3)
	lfd 19,128(3)
	lfd 18,120(3)
	lfd 17,112(3)
	lfd 16,104(3)
	lfd 15,96(3)
	lfd 14,88(3)

	lmw 14,16(3)		/* jb_gpr */

	mtcrf 0x20,11
	mtcrf 0x10,11
	lwz 11,8(3)		/* jb_cr */
	mtcrf 0x08,11
	lwz 10,4(3)		/* jb_lr */
	mtlr 10
	lwz 1,0(3)		/* jb_sp */

	mr 3,4
	cmpwi 3,0
	bnelr
	li 3,1			/* guarantee nonzero return value */
	blr
ENDENTRY(_longjmp)
