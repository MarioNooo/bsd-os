/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI memset.s,v 1.4 2002/12/06 21:30:21 db Exp
 */

#include "DEFS.h"

/*
 * void *memset(void *dst, int byte, size_t len);
 */
ENTRY(memset)
	/*
	 * Call speedier bzero() for the most frequent case.
	 * XXX Worry about name pollution?  Fix it with weak symbols?
	 */
	cmpwi 4,0
	bne 1f
	mr 4,5
	b PLTCALL(_bzero)
1:
	cmpwi 5,0	/* Nothing to do, return */
	beqlr

	/* Propagate fill byte into the rest of the word.  */
	mr 0,4
	rlwimi 0,0,8,16,23
	rlwimi 0,0,16,0,15

	/*
	 * If the pointer isn't yet word-aligned, align it.
	 * Take care not to fill more than the requested amount.
	 */
	andi. 7,3,3
	beq .L.memset.blocks

	subfic 7,7,4		/* how many bytes needed for word alignment? */
	mr 8,5			/* keep track of potentially small counts */
	add 3,3,7		/* bump pointer to word-aligned boundary */
	lwz 10,-4(3)		/* aligned word containing unaligned bytes */
	cmpwi 7,2
	blt 2f			/* just one byte? */
	beq 1f			/* just two bytes? */

	rlwimi 10,0,0,8,15	/* fill byte[1] */
	subic. 5,5,1
	beq 3f			/* no more bytes? */

1:
	rlwimi 10,0,0,16,23	/* fill byte[2] */
	subic. 5,5,1
	beq 3f			/* no more bytes? */

2:
	rlwimi 10,0,0,24,31	/* fill byte[3] */
	subic. 5,5,1
3:
	stw 10,-4(3)		/* store the modified word */

	beqlr

	/*
	 * Fill in chunks of 32 bytes.
	 */
.L.memset.blocks:
	subi 3,3,4			/* compensate for pre-increment */
	rlwinm. 6,5,27,5,31
	beq .L.memset.4bytes

	mtctr 6
1:
	stw 0,4(3)
	stw 0,8(3)
	stw 0,12(3)
	stw 0,16(3)
	stw 0,20(3)
	stw 0,24(3)
	stw 0,28(3)
	stwu 0,32(3)
	bdnz 1b

.L.memset.4bytes:
	rlwinm. 6,5,30,29,31
	beq .L.memset.trailing

	mtctr 6
1:
	stwu 0,4(3)
	bdnz 1b

.L.memset.trailing:
	andi. 6,5,3
	beqlr				/* no bytes left */
	cmpwi 6,2
	bgt .L.memset.3bytes
	beq .L.memset.2bytes

	/* Store one byte.  */
	stb 0,4(3)
	blr

.L.memset.2bytes:
	sth 0,4(3)
	blr

.L.memset.3bytes:
	lwz 7,4(3)
	rlwimi 7,0,0,0,23
	stw 7,4(3)
	blr
ENDENTRY(memset)
