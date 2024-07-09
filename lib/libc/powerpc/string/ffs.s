/*	BSDI ffs.s,v 1.1 2002/01/09 16:07:59 donn Exp	*/

	.file "ffs.s"

#include "DEFS.h"

ENTRY(ffs)
	neg 4,3		/* arithmetically invert the operand */
	and 3,3,4	/* generate the low order bit of the operand */
	cntlzw 3,3	/* count the high order zeroes */
	subfic 3,3,32	/* subtract the result from 32 (1-based bit numbers) */
	blr
ENDENTRY(ffs)
