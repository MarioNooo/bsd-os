/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI ERRNO.h,v 1.2 2001/10/03 17:29:53 polk Exp
 */

#ifndef _POWERPC_ERRNO_H_
#define	_POWERPC_ERRNO_H_	1

#ifndef __PIC__

/* Clobbers R12. */
#define	SET_ERRNO(x)			\
	.global errno;			\
	lis 12,errno@ha;		\
	stw x,errno@l(12)

#else

/* Clobbers R0 and R12.  */
#define	SET_ERRNO(x)			\
	.section ".got2","aw";		\
0:					\
	.global errno;			\
	.long errno;			\
	.section ".text";		\
	mflr 0;				\
	bl 2f;				\
1:					\
	.long 0b-1b;			\
2:					\
	mflr 12;			\
	mtlr 0;				\
	lwz 0,0(12);			\
	lwzx 12,12,0;			\
	stwx x,0,12

#endif

#endif
