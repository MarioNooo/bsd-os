/*
 * Copyright (c) 1995 Berkeley Software Design Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz "BSDI strcmp.s,v 2.2 1998/08/17 05:35:21 torek Exp"
#endif

#include "DEFS.h"

/*
 * int strcmp(const char *s1, const char *s2) { ... }
 */
ENTRY(strcmp)
0:
	ldub	[%o0], %o2		! *(u_char *)s1
	ldub	[%o1], %o3		! *(u_char *)s2
	subcc	%o2, %o3, %o3		! compare via subtraction
	bne	1f			! done if different
	 inc	%o0
	cmp	%o2, 0			! or if one is 0
	bne	0b			! else loop with s1++,s2++
	 inc	%o1
1:
	retl				! return the difference
	 mov	%o3, %o0		! (which is 0 if *s1==*s2==0)
