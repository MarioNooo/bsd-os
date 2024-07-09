/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_itoq.c,v 2.1 1998/08/17 05:23:10 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_itoq.c,v 2.1 1998/08/17 05:23:10 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_itoq: convert int to long double.
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
long double
_Q_itoq(int i) {
	long double z;

	asm("fitoq %1,%0" : "=e"(z) : "f"(i));
	return z;
}
