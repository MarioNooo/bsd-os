/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ldexp.s,v 2.3 1997/07/17 05:26:03 donn Exp
 */

	.file "ldexp.s"

#include "DEFS.h"
#include "ERRNO.h"

/*
 * ANSI conformance requires that we notify on overflow.
 *
 * should get ERANGE from <errno.h>	XXX
 */
#define	ERANGE			34

ENTRY(ldexp)
	fildl 12(%esp)
	fldl 4(%esp)
	fscale
	fstl -8(%esp)		/* force double precision overflow detection */
	fwait			/* paranoia */
	fnstsw %ax
	testb $8,%al		/* check for overflow */
	jz 0f
	SET_ERRNO($ERANGE)	/* %st should have +/- Inf */
0:
	fstp %st(1)		/* reset the stack */
	ret
