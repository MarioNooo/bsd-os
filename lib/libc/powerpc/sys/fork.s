/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fork.s,v 1.3 2001/10/03 17:29:53 polk Exp
 */

#include "SYS.h"

PSYSCALL(fork)
	cmpwi 4,0
	beq 1f
	li 3,0
1:
	blr
ENDENTRY(fork)
