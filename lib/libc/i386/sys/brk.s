/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI brk.s,v 2.3 1997/10/30 22:14:04 donn Exp
 */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 */

	.file "brk.s"

#if defined(SYSLIBC_SCCS) && !defined(lint)
	.section ".rodata"
	.asciz "@(#)brk.s	8.1 (Berkeley) 6/4/93"
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"

	.globl	_curbrk
	.globl	_minbrk

#ifndef __PIC__

	.section ".text"
	.align 4

.Lerr:
	SET_ERRNO(%eax)
	movl	$-1,%eax
	ret

ENTRY(_brk)
	jmp	.Lok

ENTRY(brk)
	movl	4(%esp),%eax
	cmpl	%eax,_minbrk
	jl	.Lok
	movl	_minbrk,%eax
	movl	%eax,4(%esp)
.Lok:
	lea	SYS_break,%eax
	LCALL(7,0)
	jb	.Lerr
	movl	4(%esp),%eax
	movl	%eax,_curbrk
	movl	$0,%eax
	ret

#else

	.global	errno

ENTRY(_brk)
	call	.Lpic2
.Lpic2:
	popl	%ecx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.Lpic2],%ecx
	lea	SYS_break,%eax
	LCALL(7,0)
	jb	.Lerr
	jmp	.Lcommon

ENTRY(brk)
	call	.Lpic
.Lpic:
	popl	%ecx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.Lpic],%ecx
	movl	_minbrk@GOT(%ecx),%edx
	movl	4(%esp),%eax
	cmpl	%eax,(%edx)
	jl	.Lok
	movl	(%edx),%eax
	movl	%eax,4(%esp)
.Lok:
	lea	SYS_break,%eax
	LCALL(7,0)
	jb	.Lerr
.Lcommon:
	movl	4(%esp),%eax
	movl	_curbrk@GOT(%ecx),%edx
	movl	%eax,(%edx)
	movl	$0,%eax
	ret
.Lerr:
	movl	errno@GOT(%ecx),%edx
	movl	%eax,(%edx)
	movl	$-1,%eax
	ret

#endif