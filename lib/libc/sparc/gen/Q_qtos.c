/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_qtos.c,v 2.1 1998/09/21 13:34:47 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_qtos.c,v 2.1 1998/09/21 13:34:47 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_qtos: convert long double to float.
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
float
_Q_qtos(long double x) {
	float z;

	asm("fqtos %1,%0" : "=f"(z) : "e"(x));
	return z;
}
