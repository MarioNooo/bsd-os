/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigtramp.s,v 1.3 2001/10/03 17:29:54 polk Exp
 */

#include "SYS.h"

/*
 * void _sigtramp(int signal, int code);
 *
 * The sigcontext structure is located 16 bytes above the stack frame.
 * The handler address is in LR.
 */

	.globl _sigtramp
	.type _sigtramp,@function
	.balign 4
_sigtramp:
	addi 5,1,16
	blrl
	addi 3,1,16
	li 0,SYS_sigreturn
	sc
	.long 0			/* guaranteed illegal instruction */
.L.end._sigtramp:
	.size _sigtramp,.L.end._sigtramp-_sigtramp
