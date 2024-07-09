/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strcat.s"

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strcat.s,v 2.2 1997/07/17 05:26:59 donn Exp"
#endif

/*
 * Concatenate one string with another.
 * Return the address of the destination string.
 *
 * char *strcat(char *dst, const char *src);
 */
ENTRY(strcat)
	STR_PROLOGUE()

	movl %ecx,%edx
	/* xorl %eax,%eax -- done in prologue */
	movl $-1,%ecx
	repne; scasb

	decl %edi
	movl %edx,%ecx

	cmpl %esi,%edi
	jbe .Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp .Lexit

.Lforward:
	COPY()

.Lexit:
	movl 12(%esp),%eax
	COPY_EPILOGUE()
