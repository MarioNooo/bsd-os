/*	BSDI sbrk.s,v 2.2 1998/08/17 05:39:28 torek Exp	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: sbrk.s,v 1.3 92/07/02 00:56:49 torek Exp (LBL)
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)sbrk.s	8.1 (Berkeley) 6/4/93"
#endif /* LIBC_SCCS and not lint */

#include "SYS.h"

	.global	_curbrk

	.section ".data"
	.align	4
	.type	_curbrk, #object
	.size	_curbrk, 4
_curbrk: .word	_end

	.section ".text"

ENTRY(sbrk)
#ifdef __PIC__
	SET_PIC_NOFRAME(%g1, %o5)
	set	_curbrk, %o2
	ld	[%o5 + %o2], %o2
	ld	[%o2], %o3			! %o3 = old break
#else
	sethi	%hi(_curbrk), %o2
	ld	[%o2 + %lo(_curbrk)], %o3	! %o3 = old break
#endif
	add	%o3, %o0, %o4			! %o4 = new break
	mov	%o4, %o0			! copy for syscall
	mov	SYS_break, %g1
	t	ST_SYSCALL			! break(new_break)
	bcc,a	1f				! if success,
	 mov	%o3, %o0			!    set return value

	/* (see also brk.s) */
	SET_ERRNO_PIC(%o0, %g1, %o5)
	SET_ERRNO_NOPIC(%o0, %g1)
	retl
	 mov	-1, %o0

1:
	/* Success: record new break and return %o0 value. */
	retl
#ifdef __PIC__
	 st	%o4, [%o2]
#else
	 st	%o4, [%o2 + %lo(_curbrk)]
#endif