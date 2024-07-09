/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strlen.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strlen.s,v 2.2 1997/07/17 05:27:09 donn Exp"
#endif

/*
 * Count the bytes in a string.
 *
 * size_t strlen(const char *s);
 */
ENTRY(strlen)
	pushl %edi

	movl 8(%esp),%edi
	xorl %eax,%eax
	movl $-1,%ecx

	cld
	repne; scasb

	/* we either succeeded, or dumped core; adjust the result */
	movl %ecx,%eax
	negl %eax
	subl $2,%eax

	popl %edi
	ret
