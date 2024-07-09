/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_qtod.c,v 2.1 1998/08/17 05:23:41 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_qtod.c,v 2.1 1998/08/17 05:23:41 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_qtod: convert long double to int.
 *
 * Here we rely on being compiled with -mhard-quad-float; it makes for
 * the best code.
 */
double
_Q_qtod(long double x) {

	return x;
}
