/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_stoq.c,v 2.1 1998/08/17 05:24:09 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_stoq.c,v 2.1 1998/08/17 05:24:09 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_stoq: convert float to quad.
 *
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
long double
_Q_stoq(float x) {
	long double z;

	asm("fstoq %1,%0" : "=e"(z) : "f"(x));
	return z;
}
