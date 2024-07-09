/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strncat.s"

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strncat.s,v 2.2 1997/07/17 05:27:11 donn Exp"
#endif

/*
 * Concatenate one string with another.
 * Return the address of the destination string.
 *
 * char *strncat(char *dst, const char *src, size_t len);
 */
ENTRY(strncat)
	STRN_PROLOGUE()

	/* movl %ecx,%edx -- done in prologue */
	/* xorl %eax,%eax -- done in prologue */
	movl $-1,%ecx
	repne; scasb

	decl %edi
	movl %edx,%ecx
	addl %edi,%edx

	cmpl %esi,%edi
	jbe .Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp .Lterminate

.Lforward:
	COPY()

.Lterminate:		/* nul terminate the string */
	movb $0,(%edx)

	movl 12(%esp),%eax
	COPY_EPILOGUE()
