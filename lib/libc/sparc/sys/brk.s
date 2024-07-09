/*	BSDI brk.s,v 2.3 2000/09/06 21:31:45 torek Exp	*/

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
 * from: Header: brk.s,v 1.3 92/06/25 12:56:05 mccanne Exp (LBL)
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)brk.s	8.1 (Berkeley) 6/4/93"
#endif /* LIBC_SCCS and not lint */

#include "SYS.h"

	.global	_curbrk
	.common	_curbrk,4,4

	.global	_minbrk

	.section ".data"
	.align	4
	.type	_minbrk, #object
	.size	_minbrk, 4
_minbrk: .word	_end			! lower brk limit; also for gmon code

	.section ".text"

ENTRY(brk)
#ifdef __PIC__
	SET_PIC_NOFRAME(%o2, %o3)
	set	_minbrk, %o1
	ld	[%o3 + %o1], %o1
	ld	[%o1], %o1
#else
	sethi	%hi(_minbrk), %o1	! %o1 = _minbrk
	ld	[%o1 + %lo(_minbrk)], %o1
#endif
	cmp	%o1, %o0		! if (_minbrk > %o0)
	bg,a	0f
	 mov	%o1, %o0		!	%o0 = _minbrk
0:
	mov	%o0, %o2		! save argument to syscall
	mov	SYS_break, %g1
	t	ST_SYSCALL
	bcc,a	1f
	 sethi	%hi(_curbrk), %g1

	/* set errno with PIC or non-PIC as appropriate */
	SET_ERRNO_PIC(%o0, %g1, %o3)
	SET_ERRNO_NOPIC(%o0, %g1)
	/* and return failure -- note, %o1 is not used here */
	retl
	 mov	-1, %o0

1:
	/* Success: record new break and return 0. */
	clr	%o0
#ifdef __PIC__
	or	%g1, %lo(_curbrk), %g1
	ld	[%o3 + %g1], %g1
	retl	
	 st	%o2, [%g1]
#else
	retl
	 st	%o2, [%g1 + %lo(_curbrk)]
#endif
