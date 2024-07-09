/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ptrace.s,v 1.2 2001/10/03 17:29:54 polk Exp
 */

#include "SYS.h"

ENTRY(ptrace)
	li 7,0
	SET_ERRNO(7)		/* guarantee that errno == 0 on success */
	li 0,SYS_ptrace
	sc
	bso- 1f
	blr
1:
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(ptrace)
