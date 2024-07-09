/*
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI Q_sqrt.c,v 2.1 1998/08/17 05:24:02 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Q_sqrt.c,v 2.1 1998/08/17 05:24:02 torek Exp"
#endif /* LIBC_SCCS and not lint */

#include <limits.h>

/*
 * _Q_sqrt: square root of long double.
 */
long double
_Q_sqrt(long double x) {
	long double z;

	asm("fsqrtq %1,%0" : "=e"(z) : "e"(x));
	return z;
}
