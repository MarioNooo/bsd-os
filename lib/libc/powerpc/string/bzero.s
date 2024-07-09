/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bzero.s,v 1.3 2002/12/06 21:30:21 db Exp
 */

#include "DEFS.h"

/*
 * void bzero(void *dst, size_t len);
 * int _bzero(void *dst, size_t len);
 *
 * The second entry point explicitly returns its first argument;
 * thus it can be accessed by memset().
 */
ENTRY(bzero)
	.globl _bzero
_bzero:
	li 0,0
	cmplw 0,4
	beqlr			/* if count is 0, then done */
	mr 11,3			/* leave r3 untouched as a return value */

	/* Check for block-aligned pointer.  */
	andi. 5,11,31
	beq .L.bzero.512bytes

	/*
	 * If the pointer isn't yet word-aligned, align it.
	 * Take care not to clear more than the requested amount.
	 */
	subfic 5,5,32		/* how many bytes to reach 32-byte alignment? */
	cmplw 4,5
	bgt 1f			/* is count greater than alignment bytes? */
	mr 5,4			/* no, use count */
1:
	mr 8,5
	andi. 6,11,3
	beq .L.bzero.align_words

	subfic 7,6,4		/* how many bytes needed for word alignment? */
	add 11,11,7		/* bump pointer to word-aligned boundary */
	lwz 10,-4(11)		/* aligned word containing unaligned bytes */
	cmpwi 7,2
	blt 2f			/* just one byte? */
	beq 1f			/* just two bytes? */

	rlwimi 10,0,0,8,15	/* clear byte[1] */
	subic. 5,5,1
	beq 3f			/* no more bytes? */

1:
	rlwimi 10,0,0,16,23	/* clear byte[2] */
	subic. 5,5,1
	beq 3f			/* no more bytes? */

2:
	rlwimi 10,0,0,24,31	/* clear byte[3] */
	subic. 5,5,1
3:
	stw 10,-4(11)		/* store the modified word */

	/*
	 * If the pointer isn't block-aligned, align it.
	 */
.L.bzero.align_words:
	srwi. 6,5,2
	beq 2f
	mtctr 6
1:
	stw 0,0(11)
	addi 11,11,4
	bdnz 1b
2:

	/* Subtract alignment bytes from total bytes.  */
	andi. 5,5,3		/* how many bytes did we fail to clear? */
	sub 8,8,5		/* => how many bytes did we actually clear? */
	sub. 4,4,8		/* adjust the count */
	beqlr			/* if none remain, we're done */

	/*
	 * The pointer is now block-aligned.
	 * We use the 'data cache block set to zero' to zero entire blocks.
	 */
.L.bzero.512bytes:
	/* Take advantage of loop unrolling.  */
	rlwinm. 5,4,23,9,31
	beq .L.bzero.blocks

	mtctr 5
1:
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	dcbz 0,11
	addi 11,11,32
	bdnz 1b

	/* Zero in chunks of 32 bytes.  */
.L.bzero.blocks:
	rlwinm. 5,4,27,28,31
	beq .L.bzero.4bytes

	mtctr 5
1:
	dcbz 0,11
	addi 11,11,32
	bdnz 1b

.L.bzero.4bytes:
	subi 11,11,4			/* compensate for pre-increment */
	rlwinm. 5,4,30,29,31
	beq .L.bzero.trailing

	mtctr 5
1:
	stwu 0,4(11)
	bdnz 1b

.L.bzero.trailing:
	andi. 5,4,3
	beqlr				/* no bytes left */
	cmpwi 5,2
	bgt .L.bzero.3bytes
	beq .L.bzero.2bytes

	/* Store one byte.  */
	stb 0,4(11)
	blr

.L.bzero.2bytes:
	sth 0,4(11)
	blr

.L.bzero.3bytes:
	lbz 6,7(11)
	stw 6,4(11)
	blr
ENDENTRY(bzero)
