/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "rindex.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI rindex.s,v 2.2 1997/07/17 05:26:56 donn Exp"
#endif

/*
 * Look for the last byte c with a particular value in a string s.
 * We have to scan the entire string, so we use REP SCAS to do it.
 *
 * char *rindex(const char *s, int c);
 */
ENTRY(rindex)
	pushl %edi

	movl 8(%esp),%edi

	/* find the end of the string */
	xorl %eax,%eax
	movl $-1,%ecx
	cld
	repne; scasb
	negl %ecx
	decl %edi
	decl %ecx

	/* search backwards from the end of the string for the byte */
	movl 12(%esp),%eax
	std
	repne; scasb
	jnz .Lnotfound

	leal 1(%edi),%eax

.Lexit:
	cld
	popl %edi
	ret

.Lnotfound:
	xorl %eax,%eax
	jmp .Lexit
