/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "bzero.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI bzero.s,v 2.2 1997/07/17 05:26:36 donn Exp"
#endif

/*
 * Zero out a block of data dst of len bytes.
 *
 * void bzero(void *dst, size_t len);
 */
ENTRY(bzero)
	pushl %edi

	movl 8(%esp),%edi
	movl 12(%esp),%ecx

	cld
	movl %ecx,%edx
	andl $3,%edx
	shrl $2,%ecx
	xorl %eax,%eax
	rep; stosl
	movl %edx,%ecx
	rep; stosb

	popl %edi
	ret
