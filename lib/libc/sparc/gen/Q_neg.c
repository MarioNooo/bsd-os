/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_neg.c,v 2.1 1998/08/17 05:23:34 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_neg.c,v 2.1 1998/08/17 05:23:34 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_neg: negate long double.
 * gcc happens to compile this to what we want in the first place
 * (but we will supply the library routine for completeness).
 */
long double
_Q_neg(long double x) {

	/* asm("fnegs %1,%0" : "=f"(x) : "f"(x)); return x; */
	return -x;
}
