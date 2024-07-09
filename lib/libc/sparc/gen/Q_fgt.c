/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_fgt.c,v 2.1 1998/08/17 05:22:42 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_fgt.c,v 2.1 1998/08/17 05:22:42 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_fgt: compare two long doubles for greater.
 *
 * Here we rely on being compiled with -mhard-quad-float.
 * It gets the exceptions right and is just a whole heck of a lot easier.
 */
int
_Q_fgt(long double x, long double y) {

	return x > y;
}
