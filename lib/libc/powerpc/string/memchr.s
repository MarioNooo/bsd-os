/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI memchr.s,v 1.1 1995/12/18 21:50:20 donn Exp
 */

#include "DEFS.h"

/*
 * void *memchr(const void *s, int c, size_t len);
 */
ENTRY(memchr)
	cmpwi 5,0
	beq .L.memchr.fail

	cmpwi 4,0
	bne .L.memchr.nonzero

	mtctr 5

	subi 3,3,4		/* compensate for pre-increment */

	andi. 5,3,3
	beq .L.memchr.aligned

	subfic 7,5,4		/* how many bytes do we need to test? */
	lwzux 6,3,7		/* extract aligned word containing bytes */
	cmpwi 7,2		/* where do we start? */
	blt 2f			/* one byte to test => go to last byte */
	beq 1f			/* two bytes to test => go to middle byte */
	rlwinm. 7,6,0,8,15
	beq .L.memchr.byte1
	bdz .L.memchr.fail
1:
	rlwinm. 8,6,0,16,23
	beq .L.memchr.byte2
	bdz .L.memchr.fail
2:
	rlwinm. 9,6,0,24,31
	beq .L.memchr.byte3
	bdz .L.memchr.fail

.L.memchr.aligned:
	lwzu 5,4(3)
	rlwinm. 6,5,0,0,7
	beq .L.memchr.byte0
	bdz .L.memchr.fail
	rlwinm. 7,5,0,8,15
	beq .L.memchr.byte1
	bdz .L.memchr.fail
	rlwinm. 8,5,0,16,23
	beq .L.memchr.byte2
	bdz .L.memchr.fail
	rlwinm. 9,5,0,24,31
	beq .L.memchr.byte3
	bdz .L.memchr.fail
	b .L.memchr.aligned

.L.memchr.byte3:
	addi 3,3,3
	blr

.L.memchr.byte2:
	addi 3,3,2
	blr

.L.memchr.byte1:
	addi 3,3,1

.L.memchr.byte0:
	blr

.L.memchr.nonzero:
	mtctr 5

	subi 3,3,4		/* compensate for pre-increment */

	andi. 5,3,3
	beq .L.memchr.nz_aligned

	subfic 7,5,4		/* how many bytes do we need to test? */
	lwzux 6,3,7		/* extract aligned word containing bytes */
	cmpwi 7,2		/* where do we start? */
	blt 2f			/* one byte to test => go to last byte */
	beq 1f			/* two bytes to test => go to middle byte */
	rlwinm 7,6,16,24,31
	cmpw 7,4
	beq .L.memchr.byte1
	bdz .L.memchr.fail
1:
	rlwinm 8,6,24,24,31
	cmpw 8,4
	beq .L.memchr.byte2
	bdz .L.memchr.fail
2:
	rlwinm 9,6,0,24,31
	cmpw 9,4
	beq .L.memchr.byte3
	bdz .L.memchr.fail

.L.memchr.nz_aligned:
	lwzu 5,4(3)
	rlwinm 6,5,8,24,31
	cmpw 6,4
	beq .L.memchr.byte0
	bdz .L.memchr.fail
	rlwinm 7,5,16,24,31
	cmpw 7,4
	beq .L.memchr.byte1
	bdz .L.memchr.fail
	rlwinm 8,5,24,24,31
	cmpw 8,4
	beq .L.memchr.byte2
	bdz .L.memchr.fail
	rlwinm 9,5,0,24,31
	cmpw 9,4
	beq .L.memchr.byte3
	bdz .L.memchr.fail
	b .L.memchr.nz_aligned

.L.memchr.fail:
	li 3,0
	blr
ENDENTRY(memchr)
