/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_qtoi.c,v 2.1 1998/08/17 05:23:47 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_qtoi.c,v 2.1 1998/08/17 05:23:47 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_qtoi: convert long double to int.
 * We just rely on the instruction set to do the trick.
 * The inline asm is what gcc emits when using -mhard-quad-float.
 */
int
_Q_qtoi(long double x) {
	int i;

	asm("fqtoi %1,%0" : "=f"(i) : "e"(x));
	return i;
}
