/*	BSDI cpu.h,v 1.1 2001/11/27 23:15:10 donn Exp	*/
/*	$NetBSD: syncicache.c,v 1.2 1999/05/05 12:36:40 tsubai Exp $	*/

/*
 * Copyright (C) 1995-1997, 1999 Wolfgang Solfrank.
 * Copyright (C) 1995-1997, 1999 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MACHINE_CPU_H__
#define	__MACHINE_CPU_H__	1

#include <sys/param.h>

#define	__CACHELINESIZE		32

#define	__syncicache(from, len) ({					\
	void *_from = (from);						\
	int _len = (len);						\
	int _l, _off;							\
	char *_p;							\
									\
	_off = (u_int)_from & (__CACHELINESIZE - 1);			\
	_l = _len += _off;						\
	_p = (char *)_from - _off;					\
	do {								\
		__asm__ __volatile ("dcbst 0,%0" :: "r"(_p));		\
		_p += __CACHELINESIZE;					\
	} while ((_l -= __CACHELINESIZE) > 0);				\
	__asm__ __volatile ("sync");					\
	_l = _len += _off;						\
	_p = (char *)_from - _off;					\
	do {								\
		__asm__ __volatile ("icbi 0,%0" :: "r"(_p));		\
		_p += __CACHELINESIZE;					\
	} while ((_l -= __CACHELINESIZE) > 0);				\
	__asm__ __volatile ("isync");					\
})

#endif
