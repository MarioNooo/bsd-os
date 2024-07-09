/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "fixunsdfsi.s"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz "BSDI fixunsdfsi.s,v 2.4 2002/01/14 18:42:09 donn Exp"
#endif

#include "DEFS.h"

#define	FCW_CHOP	0x0c00

/*
 * Truncate a double to an unsigned int.
 * See fixdfsi.s for comments about the difficulties with truncation.
 */
ENTRY(__fixunsdfsi)
	/*
	 * Prepare to change the rounding mode.
	 */
	subl $4,%esp
	fstcw (%esp)
	movw (%esp),%ax
	orw $FCW_CHOP,%ax
	movw %ax,2(%esp)

	/*
	 * Is it too big for an int?
	 */
	fldl 8(%esp)
#ifndef __PIC__
	fcoml .L.intovfl
#else
	pushl $0
	pushl $0x41e00000	/* 2147483648.0 as a hex number */
	fcoml (%esp)
#endif
	fstsw %ax
	sahf
	jb 0f

	/*
	 * It doesn't fit.
	 * Since we don't have an unsigned conversion instruction,
	 * we must adjust into int range, convert and adjust back.
	 */
#ifndef __PIC__
	fsubl .L.intovfl
#else
	fsubl (%esp)
#endif
	movl $0x80000000,%eax
	jmp 1f

0:
	xorl %eax,%eax		/* null adjustment */

	/*
	 * Peform the conversion from double to int.
	 * Note the little dance with the rounding mode.
	 */
1:
#ifdef __PIC__
	addl $8,%esp
#endif
	fldcw 2(%esp)
	fistpl 8(%esp)
	fldcw (%esp)

	/*
	 * Make any final adjustments.
	 */
	addl 8(%esp),%eax
	addl $4,%esp
	ret

#ifndef __PIC__
	.section ".rodata"
	.align 4
.L.intovfl:
	.double 2147483648.
#endif

