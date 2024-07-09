/*	BSDI s_scalbnf.c,v 1.1 1998/03/05 00:22:47 donn Exp	*/

/*
 * Float wrapper for a double function on the i386.
 * All floating point arithmetic is performed in double precision,
 * so we just need to adjust precision and call the double function.
 */

#include "math.h"
#include "math_private.h"

float
scalbnf(float x, int n)
{

	return (scalbn(x, n));
}