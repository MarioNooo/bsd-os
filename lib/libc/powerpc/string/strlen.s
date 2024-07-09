/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strlen.s,v 1.1 1995/12/18 21:50:27 donn Exp
 */

#include "DEFS.h"

/*
 * size_t strlen(const char *s);
 */
ENTRY(strlen)
	subi 4,3,4		/* compensate for pre-increment */

	andi. 5,3,3
	beq .L.strlen.aligned

	subfic 7,5,4		/* how many bytes do we need to test? */
	lwzux 6,4,7		/* extract aligned word containing bytes */
	cmpwi 7,2		/* where do we start? */
	blt 2f			/* one byte to test => go to last byte */
	beq 1f			/* two bytes to test => go to middle byte */
	rlwinm. 7,6,0,8,15
	beq .L.strlen.byte1
1:
	rlwinm. 8,6,0,16,23
	beq .L.strlen.byte2
2:
	rlwinm. 9,6,0,24,31
	beq .L.strlen.byte3

.L.strlen.aligned:
	lwzu 5,4(4)
	rlwinm. 6,5,0,0,7
	beq .L.strlen.byte0
	rlwinm. 7,5,0,8,15
	beq .L.strlen.byte1
	rlwinm. 8,5,0,16,23
	beq .L.strlen.byte2
	rlwinm. 9,5,0,24,31
	bne .L.strlen.aligned

.L.strlen.byte3:
	addi 4,4,3
	sub 3,4,3
	blr

.L.strlen.byte2:
	addi 4,4,2
	sub 3,4,3
	blr

.L.strlen.byte1:
	addi 4,4,1

.L.strlen.byte0:
	sub 3,4,3
	blr
ENDENTRY(strlen)
