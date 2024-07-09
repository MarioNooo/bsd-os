/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strchr.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strchr.s,v 2.2 1997/07/17 05:27:01 donn Exp"
#endif

/*
 * Look for a byte c with a particular value in a string s.
 * Since we don't have to check the entire string before finding the byte,
 * we don't want to use the REP SCAS technique to locate it.
 *
 * char *strchr(const char *s, int c);
 */
ENTRY(strchr)
	pushl %esi

	movl 8(%esp),%esi
	movl 12(%esp),%edx

	cld

.Lagain:
	lodsb
	cmpb %al,%dl
	je .Lfound
	testb %al,%al
	jnz .Lagain

	xorl %eax,%eax

.Lexit:
	popl %esi
	ret

.Lfound:
	leal -1(%esi),%eax
	jmp .Lexit
