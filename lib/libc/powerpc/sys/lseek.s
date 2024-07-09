/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1996, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI lseek.s,v 1.3 2001/10/03 17:29:53 polk Exp
 */

#include "SYS.h"

/*
 * The PowerPC calling convention 'aligns' 64-bit values in
 * odd-even register pairs, so it's one of the architectures that
 * actually needs the annoying pad word.
 */
ENTRY(lseek)
	li 0,SYS_lseek
	sc
	bso- 2f
	blr
2:
	SET_ERRNO(3)
	li 4,-1
	li 3,-1
	blr
ENDENTRY(lseek)
