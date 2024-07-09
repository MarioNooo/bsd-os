/*	BSDI DEFS.h,v 2.4 1999/02/14 14:41:18 torek Exp	*/

/*-
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
 *	@(#)DEFS.h	8.1 (Berkeley) 6/4/93
 */

#if __STDC__
#define CAT(a,b) a##b
#define	CAT3(a,b,c) a##b##c
#else
#define	CAT(a,b) a/**/b
#define	CAT3(a,b,c) a/**/b/**/c
#endif

/*
 * We have:
 *	non-PIC vs PIC
 *	non-profiled vs profiled
 * On sparc, -fPIC/-fpic disables -p, so PROF implies !PIC.  This
 * would matter if the .data word we pass to mcount were used, but
 * since it is not, we could profile PIC.  (Must fix gcc for this.)
 */

/*
 * SET_PIC() sets the PIC reg for indexing into GOT.  Note that GOT
 * indices must be loaded separately (ugh!) and that SET_PIC clobbers %o7.
 * Normally we would need a frame for this, but instead of save+restore
 * we can copy %o7 to a temporary register , in SET_PIC_NOFRAME.
 *
 * (By the way, did you know that the assembler recognizes the string
 * _GLOBAL_OFFSET_TABLE_ and uses special relocations for it?  Ugh!)
 *
 * To avoid some #ifdefs elsewhere, non-__PIC__ makes SET_PIC* vanish.
 */
#ifdef __PIC__
#define	SET_PIC(pic) \
	0: call 1f; sethi %hi(_GLOBAL_OFFSET_TABLE_-(0b-.)),pic; \
	1: or pic,%lo(_GLOBAL_OFFSET_TABLE_-(0b-.)),pic; \
	add pic,%o7,pic
#define	SET_PIC_NOFRAME(tmp, pic) \
	mov %o7,tmp; SET_PIC(pic); mov tmp,%o7
#else
#define	SET_PIC(pic)
#define	SET_PIC_NOFRAME(tmp, pic)
#endif

#ifdef PROF
# if 0
#   define ENTRY(x) \
	.align 4; .global x; .type x,@function; CAT(x,:); \
	.data; .align 4; 1: .word 0; .text; save %sp,-96,%sp; \
	sethi %hi(1b),%o0; call _mcount; add %lo(1b),%o0,%o0; restore
# else
#  define ENTRY(x) \
	.align 4; .global x; .type x,@function; CAT(x,:); \
	save %sp,-96,%sp; call _mcount; nop; restore
# endif
#else
# define ENTRY(x) \
	.align 4; .global x; .type x,@function; CAT(x,:)
#endif

#define	ASENTRY(x)	ENTRY(x)	/* compat */

#define	ENDENTRY(x) \
	CAT3(.LLend_,x,:) .size x,CAT2(.LLend_,x)-x
