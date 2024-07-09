/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "strcpy.s"

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI strcpy.s,v 2.2 1997/07/17 05:27:06 donn Exp"
#endif

/*
 * Copy a string into a buffer.
 * Return the address of the buffer.
 *
 * char *strcpy(char *dst, const char *src);
 */
ENTRY(strcpy)
	STR_PROLOGUE()

	cmpl %esi,%edi
	jbe .Lforward	/* if destination precedes source, copy forward */

	COPY_REVERSE()
	jmp .Lexit

.Lforward:
	COPY()

.Lexit:
	movl 12(%esp),%eax
	COPY_EPILOGUE()
