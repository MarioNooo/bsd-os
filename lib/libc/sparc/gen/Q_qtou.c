/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_qtou.c,v 2.1 1998/08/17 05:23:55 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_qtou.c,v 2.1 1998/08/17 05:23:55 torek Exp"
#endif /* LIBC_SCCS and not lint */

/*
 * _Q_qtou: convert long double to unsigned int.
 *
 * Here we rely on being compiled with -mhard-quad-float; it is too
 * annoying to code this with inline asm.
 */
unsigned int
_Q_qtou(long double x) {

	return x;
}
