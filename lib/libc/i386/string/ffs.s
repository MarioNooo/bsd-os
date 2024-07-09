/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

	.file "ffs.s"

#include "DEFS.h"

#ifdef LIBC_SCCS
	.section ".rodata"
	.asciz	"BSDI ffs.s,v 2.2 1997/07/17 05:26:39 donn Exp"
#endif

/*
 * Look for the first set bit, starting at the low order end.
 * Return bit address + 1, or 0 if no bits found.
 *
 * int ffs(int bits);
 */
ENTRY(ffs)
	bsfl 4(%esp),%eax
	jz 0f
	incl %eax
	ret

0:
	xorl %eax,%eax
	ret
