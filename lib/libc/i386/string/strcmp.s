/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strcmp.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strcmp.s,v 2.2 1997/07/17 05:27:04 donn Exp"
#endif

/*
 * Compare two strings and return >0/0/<0 depending on string order.
 *
 * int strcmp(const char *s1, const char *s2);
 */
ENTRY(strcmp)
	pushl %esi
	pushl %edi

	movl 12(%esp),%esi
	movl 16(%esp),%edi

	cld

.Lagain:
	cmpsb
	jne .Lfound
	cmpb $0,-1(%edi)
	jnz .Lagain

	xorl %eax,%eax

.Lexit:
	popl %edi
	popl %esi
	ret

.Lfound:
	movzbl -1(%esi),%eax
	movzbl -1(%edi),%ecx
	subl %ecx,%eax
	jmp .Lexit
