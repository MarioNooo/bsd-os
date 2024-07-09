/*
 * Copyright (c) 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI floatditf.c,v 2.1 2000/06/04 22:03:39 torek Exp
 */

#include "quad.h"

/*
 * Convert (signed) quad to long double.
 */
long double
__floatditf(x)
	quad_t x;
{
	long double d;
	union uu u;
	int neg;

	/*
	 * Get an unsigned number first, by negating if necessary.
	 */
	if (x < 0)
		u.q = -x, neg = 1;
	else
		u.q = x, neg = 0;

	/*
	 * Now u.ul[H] has the factor of 2^32 (or whatever) and u.ul[L]
	 * has the units.  Ideally we could just set d, add LONG_BITS to
	 * its exponent, and then add the units, but this is portable
	 * code and does not know how to get at an exponent.  Machine-
	 * specific code may be able to do this more efficiently.
	 */
	d = (long double)u.ul[H] * ((1 << (LONG_BITS - 2)) * 4.0L);
	d += u.ul[L];

	return (neg ? -d : d);
}
