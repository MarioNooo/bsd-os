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
 * from: /master/lib/libc/sparc/gen/_setjmp.s,v 2.5 1998/09/24 00:34:14 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)_setjmp.s	8.1 (Berkeley) 6/4/93"
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v?v:1)" from
 * the last call to
 *	_setjmp(a)
 * by unwinding the call stack.
 * The previous signal state is NOT restored.
 */

#include "DEFS.h"

#define	EMPTY		/* branch delay slot marker */

ENTRY(_setjmp)
	! beware, jmp_buf need not be doubleword aligned
	st	%sp, [%o0+0]	/* caller's stack pointer */
	st	%o7, [%o0+4]	/* caller's pc */
	st	%fp, [%o0+8]	/* store caller's frame pointer */
	retl
	 clr	%o0		! return 0

ENTRY(_longjmp)
	addcc	%o1, %g0, %g2	! compute v ? v : 1 in a global register
	be,a	0f
	 mov	1, %g2
0:
	mov	%o0, %g1	! save a in another global register
	ld	[%g1+8], %o2	/* get caller's frame */
1:
	cmp	%fp, %o2	! compare against desired frame
	bl,a	1b		! if below,
	 restore %o2, 0, %o2	!    pop frame and loop, holding on to %o2
	be,a	.Lfound		! if there,
	 ld	[%g1+0], %o2	!    fetch return %sp, and get out

.Lbotch:
	! went too far; bomb out
	mov	2, %o0		! STDERR_FILENO
	SET_PIC_NOFRAME(%o1,%o2)

	set	.Lmsg, %o1	! "_longjmp environment no longer available\n"
#ifdef __PIC__
	ld	[%o2 + %o1], %o1
#endif
	call	write
	 mov	41, %o2		! strlen(%o1)
	call	abort
	 nop
	unimp	0

.Lfound:
	cmp	%o2, %sp	! %sp must not decrease
	bl	.Lbotch
	 EMPTY
	ld	[%g1+4], %o7	! also need return pc
	mov	%o2, %sp	! it is OK, put it in place
	retl			! success, return %g2
	 mov	%g2, %o0

	.section ".rodata"
.Lmsg:	.ascii	"_longjmp environment no longer available\n"
	.align	4
