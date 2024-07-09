/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Ovfork.s,v 1.5 2001/11/13 23:16:09 donn Exp
 */

#include "SYS.h"

/*
 * Unlike the i386, the Power PC puts its return value in a register,
 * so we don't have to worry about the child trashing the parent's stack.
 */
ENTRY(vfork)
	li 0,SYS_vfork
	sc
	bso 2f
	cmpwi 4,0
	beq 1f
	li 3,0
1:
	blr
2:
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(vfork)
