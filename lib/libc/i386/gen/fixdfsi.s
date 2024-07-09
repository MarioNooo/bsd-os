/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "fixdfsi.s"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz "BSDI fixdfsi.s,v 2.2 1997/07/17 05:25:58 donn Exp"
#endif

#include "DEFS.h"

#define	FCW_CHOP	0x0c00

/*
 * Truncate a double to an int.
 * Unfortunately there is no 'convert with truncation' instruction
 * on the 80387 like there is on the 68881.
 * We're forced to change the rounding mode temporarily;
 * this can cause funny side effects if a signal arrives
 * while the mode is altered.
 */
ENTRY(__fixdfsi)
	/*
	 * Convert the floating point rounding mode to truncate.
	 * Save the old control word.
	 */
	subl $4,%esp
	fstcw (%esp)
	movw (%esp),%ax
	orw $FCW_CHOP,%ax
	movw %ax,2(%esp)

	/*
	 * Switch rounding mode, convert double-to-int and switch back.
	 * This makes the window small (but not nonexistent).
	 */
	fldl 8(%esp)
	fldcw 2(%esp)
	fistpl 8(%esp)
	fldcw (%esp)

	movl 8(%esp),%eax
	addl $4,%esp
	ret
