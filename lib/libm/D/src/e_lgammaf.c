/* e_lgammaf.c -- float version of e_lgamma.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "e_lgammaf.c,v 1.1.1.1 1998/01/20 17:41:39 polk Exp";
#endif

/* __ieee754_lgammaf(x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgammaf_r
 */

#include "math.h"
#include "math_private.h"

extern int signgam;

#ifdef __STDC__
	float __ieee754_lgammaf(float x)
#else
	float __ieee754_lgammaf(x)
	float x;
#endif
{
	return __ieee754_lgammaf_r(x,&signgam);
}