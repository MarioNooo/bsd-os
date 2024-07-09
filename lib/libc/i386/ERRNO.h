/*-
 * Copyright (c) 1996,1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI ERRNO.h,v 2.3 2003/06/03 23:02:18 polk Exp
 */

#ifndef _I386_ERRNO_H_
#define	_I386_ERRNO_H_	1

#ifndef __PIC__

#ifdef notyet
#define	SET_ERRNO(x)		\
	pushl	x;		\
	call	__errnop;	\
	popl	%ecx;		\
	movl	%ecx,(%eax)
#else
#define	SET_ERRNO(x)		\
	.global errno;		\
	movl	x,errno
#endif

#else

#define	SET_ERRNO(x)			\
	.global errno;			\
	call 1f;			\
1:					\
	popl %ecx;			\
	addl $_GLOBAL_OFFSET_TABLE_+[.-1b],%ecx; \
	movl errno@GOT(%ecx),%ecx;	\
	movl x,(%ecx)

#endif

#endif
