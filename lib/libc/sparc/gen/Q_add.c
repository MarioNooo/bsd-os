/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_add.c,v 2.1 1998/08/17 05:22:08 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_add.c,v 2.1 1998/08/17 05:22:08 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_add: sum two long doubles.
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
long double
_Q_add(long double x, long double y) {
	long double z;

	asm("faddq %1,%2,%0" : "=e"(z) : "e"(x), "e"(y));
	return z;
}
