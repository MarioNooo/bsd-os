/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pipe.s,v 1.5 2001/11/12 18:41:18 donn Exp
 */

#include "SYS.h"

PENTRY(pipe)
	mr 5,3
	li 0,SYS_pipe
	sc
	bso- 1f
	stw 3,0(5)
	stw 4,4(5)
	li 3,0
	blr
1:
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(pipe)
