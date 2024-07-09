/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_dtoq.c,v 2.1 1998/08/17 05:22:22 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_dtoq.c,v 2.1 1998/08/17 05:22:22 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_dtoq: convert double to quad.
 *
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
long double
_Q_dtoq(double x) {
	long double z;

	asm("fdtoq %1,%0" : "=e"(z) : "f"(x));
	return z;
}
