/*
 * Copyright (c) 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI floatunsditf.c,v 2.1 2000/06/04 22:03:39 torek Exp
 */

#include "quad.h"

/*
 * Convert (unsigned) quad to long double.
 * This is exactly like floatditf.c except that negatives never occur.
 */
long double
__floatunsditf(x)
	u_quad_t x;
{
	long double d;
	union uu u;

	u.uq = x;
	d = (long double)u.ul[H] * ((1 << (LONG_BITS - 2)) * 4.0L);
	d += u.ul[L];
	return (d);
}
