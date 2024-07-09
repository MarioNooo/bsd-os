/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI memmove.s,v 1.1 1995/12/18 21:50:23 donn Exp
 */

#include "DEFS.h"

/*
 * void *memmove(void *dst, const void *src, size_t len);
 */
ENTRY(memmove)
	/*
	 * If src or dst is not word aligned, copy bytes.
	 * If src and dst are compatibly aligned, we could align them
	 * to word boundaries, but that's overhead that should rarely pay off.
	 */
	or 0,4,3
	andi. 0,0,3
	bne .L.memmove.bytes

	/* Copy backward if src precedes dst.  */
	cmplw 4,3
	blt .L.memmove.reverse

	subi 4,4,4
	subi 10,3,4

	/* Copy in chunks of 32 bytes.  */
	rlwinm. 0,5,27,5,31
	beq .L.memmove.forw.4bytes

	mtctr 0
1:
	lwz 6,4(4)
	lwz 7,8(4)
	lwz 8,12(4)
	lwz 9,16(4)
	stw 6,4(10)
	stw 7,8(10)
	stw 8,12(10)
	stw 9,16(10)
	lwz 6,20(4)
	lwz 7,24(4)
	lwz 8,28(4)
	lwzu 9,32(4)
	stw 6,20(10)
	stw 7,24(10)
	stw 8,28(10)
	stwu 9,32(10)
	bdnz 1b

.L.memmove.forw.4bytes:
	rlwinm. 0,5,30,29,31
	beq .L.memmove.forw.trailing

	mtctr 0
1:
	lwzu 6,4(4)
	stwu 6,4(10)
	bdnz 1b

.L.memmove.forw.trailing:
	andi. 0,5,3
	beqlr				/* no bytes left */
	cmpwi 0,2
	bgt .L.memmove.forw.3bytes
	beq .L.memmove.forw.2bytes

	/* Store one byte.  */
	lbz 0,4(4)
	stb 0,4(10)
	blr

.L.memmove.forw.2bytes:
	lhz 0,4(4)
	sth 0,4(10)
	blr

.L.memmove.forw.3bytes:
	lwz 6,4(4)
	lwz 0,4(10)
	rlwimi 0,6,0,0,23
	stw 0,4(10)
	blr

.L.memmove.reverse:
	add 4,4,5
	add 10,3,5

	/* Become word aligned.  */
	andi. 0,5,3
	beq .L.memmove.rev.4bytes
	cmpwi 0,2
	bgt .L.memmove.rev.3bytes
	beq .L.memmove.rev.2bytes

	/* Store one byte.  */
	lbzu 0,-1(4)
	stbu 0,-1(10)
	b .L.memmove.rev.4bytes

.L.memmove.rev.2bytes:
	lhzu 0,-2(4)
	sthu 0,-2(10)
	b .L.memmove.rev.4bytes

.L.memmove.rev.3bytes:
	lwzu 6,-3(4)
	lwz 0,-3(10)
	rlwimi 0,6,0,0,23
	stwu 0,-3(10)

.L.memmove.rev.4bytes:
	rlwinm. 0,5,30,29,31
	beq .L.memmove.rev.32bytes

	mtctr 0
1:
	lwzu 6,-4(4)
	stwu 6,-4(10)
	bdnz 1b

.L.memmove.rev.32bytes:
	rlwinm. 0,5,27,5,31
	beqlr

	mtctr 0
1:
	lwz 9,-4(4)
	lwz 8,-8(4)
	lwz 7,-12(4)
	lwz 6,-16(4)
	stw 9,-4(10)
	stw 8,-8(10)
	stw 7,-12(10)
	stw 6,-16(10)
	lwz 9,-20(4)
	lwz 8,-24(4)
	lwz 7,-28(4)
	lwzu 6,-32(4)
	stw 9,-20(10)
	stw 8,-24(10)
	stw 7,-28(10)
	stwu 6,-32(10)
	bdnz 1b

	blr

.L.memmove.bytes:
	/* Check for copy of 0 bytes.  */
	cmpwi 5,0
	beqlr

	cmplw 4,3
	blt .L.memmove.rev.bytes

	subi 4,4,1
	subi 10,3,1
	mtctr 5

1:
	lbzu 0,1(4)
	stbu 0,1(10)
	bdnz 1b

	blr

.L.memmove.rev.bytes:
	add 4,4,5
	add 10,3,5
	mtctr 5

1:
	lbzu 0,-1(4)
	stbu 0,-1(10)
	bdnz 1b

	blr
ENDENTRY(memmove)
