/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI emulate_bits.c,v 2.1 1995/02/03 15:13:25 polk Exp
 */

#include <sys/param.h>
#include "emulate.h"

/*
 * Translate bits from one format to another given a translation table.
 * Perhaps we should make this an inline function?
 * Note that more efficient translation occurs if
 * frequently used bits occur earlier in the table.
 */
unsigned long
transform_bits(x, in)
	const struct xbits *x;
	unsigned long in;
{
	const unsigned long *map = x->x_map;
	unsigned long out = in & x->x_common;

	for (in &= ~out; in && map[0]; in &= ~map[0], map += 2)
		if (in & map[0])
			out |= map[1];
	return (out);
}
