/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bcopy.s,v 1.1 1995/12/18 21:50:16 donn Exp
 */

#include "DEFS.h"

/*
 * void bcopy(const void *src, void *dst, size_t len);
 */
ENTRY(bcopy)
	/*
	 * If src or dst is not word aligned, copy bytes.
	 * If src and dst are compatibly aligned, we could align them
	 * to word boundaries, but that's overhead that should rarely pay off.
	 */
	or 0,3,4
	andi. 0,0,3
	bne .L.bcopy.bytes

	/* Copy backward if src precedes dst.  */
	cmplw 3,4
	blt .L.bcopy.reverse

	subi 3,3,4
	subi 4,4,4

	/* Copy in chunks of 32 bytes.  */
	rlwinm. 0,5,27,5,31
	beq .L.bcopy.forw.4bytes

	mtctr 0
1:
	lwz 6,4(3)
	lwz 7,8(3)
	lwz 8,12(3)
	lwz 9,16(3)
	stw 6,4(4)
	stw 7,8(4)
	stw 8,12(4)
	stw 9,16(4)
	lwz 6,20(3)
	lwz 7,24(3)
	lwz 8,28(3)
	lwzu 9,32(3)
	stw 6,20(4)
	stw 7,24(4)
	stw 8,28(4)
	stwu 9,32(4)
	bdnz 1b

.L.bcopy.forw.4bytes:
	rlwinm. 0,5,30,29,31
	beq .L.bcopy.forw.trailing

	mtctr 0
1:
	lwzu 6,4(3)
	stwu 6,4(4)
	bdnz 1b

.L.bcopy.forw.trailing:
	andi. 0,5,3
	beqlr				/* no bytes left */
	cmpwi 0,2			/* -1, 0, or 1 */
	bgt .L.bcopy.forw.3bytes
	beq .L.bcopy.forw.2bytes

	/* Store one byte.  */
	lbz 0,4(3)
	stb 0,4(4)
	blr

.L.bcopy.forw.2bytes:
	lhz 0,4(3)
	sth 0,4(4)
	blr

.L.bcopy.forw.3bytes:
	lwz 6,4(3)
	lwz 0,4(4)
	rlwimi 0,6,0,0,23
	stw 0,4(4)
	blr

.L.bcopy.reverse:
	add 3,3,5
	add 4,4,5

	/* Become word aligned.  */
	andi. 0,5,3
	beq .L.bcopy.rev.4bytes
	cmpwi 0,2
	bgt .L.bcopy.rev.3bytes
	beq .L.bcopy.rev.2bytes

	/* Store one byte.  */
	lbzu 0,-1(3)
	stbu 0,-1(4)
	b .L.bcopy.rev.4bytes

.L.bcopy.rev.2bytes:
	lhzu 0,-2(3)
	sthu 0,-2(4)
	b .L.bcopy.rev.4bytes

.L.bcopy.rev.3bytes:
	lwzu 6,-3(3)
	lwz 0,-3(4)
	rlwimi 0,6,0,0,23
	stwu 0,-3(4)

.L.bcopy.rev.4bytes:
	rlwinm. 0,5,30,29,31
	beq .L.bcopy.rev.32bytes

	mtctr 0
1:
	lwzu 6,-4(3)
	stwu 6,-4(4)
	bdnz 1b

.L.bcopy.rev.32bytes:
	rlwinm. 0,5,27,5,31
	beqlr

	mtctr 0
1:
	lwz 9,-4(3)
	lwz 8,-8(3)
	lwz 7,-12(3)
	lwz 6,-16(3)
	stw 9,-4(4)
	stw 8,-8(4)
	stw 7,-12(4)
	stw 6,-16(4)
	lwz 9,-20(3)
	lwz 8,-24(3)
	lwz 7,-28(3)
	lwzu 6,-32(3)
	stw 9,-20(4)
	stw 8,-24(4)
	stw 7,-28(4)
	stwu 6,-32(4)
	bdnz 1b

	blr

.L.bcopy.bytes:
	/* Check for copy of 0 bytes.  */
	cmpwi 5,0
	beqlr

	cmplw 3,4
	blt .L.bcopy.rev.bytes

	subi 3,3,1
	subi 4,4,1
	mtctr 5

1:
	lbzu 0,1(3)
	stbu 0,1(4)
	bdnz 1b

	blr

.L.bcopy.rev.bytes:
	add 3,3,5
	add 4,4,5
	mtctr 5

1:
	lbzu 0,-1(3)
	stbu 0,-1(4)
	bdnz 1b

	blr
ENDENTRY(bcopy)
