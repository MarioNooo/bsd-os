/*
 * Copyright (c) 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI fixtfdi.c,v 2.1 2000/06/04 22:03:38 torek Exp
 */

#include "quad.h"

/*
 * Convert long double to (signed) quad.
 * We clamp anything that is out of range.
 */
quad_t
__fixtfdi(long double x)
{
	if (x < 0)
		if (x <= QUAD_MIN)
			return (QUAD_MIN);
		else
			return ((quad_t)-(u_quad_t)-x);
	else
		if (x >= QUAD_MAX)
			return (QUAD_MAX);
		else
			return ((quad_t)(u_quad_t)x);
}
