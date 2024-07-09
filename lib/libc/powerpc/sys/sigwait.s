/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1998, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigwait.s,v 1.3 2001/10/03 17:29:54 polk Exp
 */

#include "SYS.h"

ENTRY(sigwait)
	lwz 3,0(3)
	li 0,SYS_sigwait
	sc
	bso- 1f
	li 3,0
	blr
1:
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(sigwait)