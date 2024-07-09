/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigprocmask.s,v 1.4 2001/10/03 17:29:54 polk Exp
 */

#include "SYS.h"

/* XXX from <signal.h>: */
#define	SIG_BLOCK	1

PENTRY(sigprocmask)
	cmpwi 4,0
	bne+ 1f			/* if new mask pointer is 0... */
	li 3,SIG_BLOCK		/* ... switch to a SIG_BLOCK */
	b 2f
1:
	lwz 4,0(4)		/* ... otherwise load the mask */

2:
	li 0,SYS_sigprocmask
	sc
	bso- 4f

	cmpwi 5,0
	beq 3f			/* if the old mask pointer is nonzero... */
	stw 3,0(5)		/* ... store the old mask */
3:
	li 3,0			/* return success */
	blr

4:
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(sigprocmask)
