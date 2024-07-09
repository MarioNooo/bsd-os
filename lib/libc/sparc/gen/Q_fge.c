/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_fge.c,v 2.1 1998/08/17 05:22:35 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_fge.c,v 2.1 1998/08/17 05:22:35 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_fge: compare two long doubles for greater or equal.
 *
 * Here we rely on being compiled with -mhard-quad-float.
 * It gets the exceptions right and is just a whole heck of a lot easier.
 */
int
_Q_fge(long double x, long double y) {

	return x >= y;
}
