/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "memcmp.s"

#include "DEFS.h"
#include "COPY.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI memcmp.s,v 2.2 1997/07/17 05:26:46 donn Exp"
#endif

/*
 * Compare two blocks of data b1 and b2 of length len.
 * Returns the difference between the differing bytes, or 0.
 *
 * int memcmp(const void *b1, const void *b2, size_t len);
 */
ENTRY(memcmp)
	COPY_PROLOGUE()

	cld
	movl %ecx,%eax
	andl $3,%eax
	shrl $2,%ecx
	repe; cmpsl
	jne 1f
	movl %eax,%ecx
	jmp 2f

1:	/* back up to start of failing word and scan by bytes */
	movl $4,%ecx
	subl %ecx,%edi
	subl %ecx,%esi

2:
	repe; cmpsb
	jne 3f

	xorl %eax,%eax
	jmp 4f

3:	/* compute difference between bytes */
	movzbl -1(%esi),%eax
	movzbl -1(%edi),%ecx
	subl %ecx,%eax

4:
	COPY_EPILOGUE()
