/*
 * Copyright (c) 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fixunstfdi.c,v 2.1 2000/06/04 22:03:38 torek Exp
 */

#include "quad.h"

#define	ONE_FOURTH	(1 << (LONG_BITS - 2))
#define	ONE_HALF	(ONE_FOURTH * 2.0L)
#define	ONE		(ONE_FOURTH * 4.0L)

/*
 * Convert long double to (unsigned) quad.
 * Not sure what to do with negative numbers---for now, anything out
 * of range becomes UQUAD_MAX.
 */
u_quad_t
__fixunstfdi(long double x)
{
	double toppart;
	union uu t;

	if (x < 0)
		return (UQUAD_MAX);	/* ??? should be 0?  ERANGE??? */
	if (x >= UQUAD_MAX)
		return (UQUAD_MAX);
	/*
	 * Get the upper part of the result.  Note that the divide
	 * may round up; we want to avoid this if possible, so we
	 * subtract `1/2' first.  Also, can use regular "double"
	 * here since the range is bounded and we do not care too much
	 * about precision.
	 */
	toppart = (x - ONE_HALF) / ONE;
	/*
	 * Now build a u_quad_t out of the top part.  The difference
	 * between x and this is the bottom part (this may introduce
	 * a few fuzzy bits, but what the heck).  With any luck this
	 * difference will be nonnegative: x should wind up in the
	 * range [0..ULONG_MAX].  For paranoia, we assume [LONG_MIN..
	 * 2*ULONG_MAX] instead.
	 */
	t.ul[H] = (unsigned long)toppart;
	t.ul[L] = 0;
	x -= (long double)t.uq;
	if (x < 0) {
		t.ul[H]--;
		x += ULONG_MAX + 1.0;
	} else if (x > ULONG_MAX) {
		t.ul[H]++;
		x -= ULONG_MAX + 1.0;
	}
	t.ul[L] = (u_long)x;
	return (t.uq);
}
