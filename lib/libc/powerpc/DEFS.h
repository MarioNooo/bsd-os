/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1997, 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI DEFS.h,v 1.4 2001/11/13 23:17:57 donn Exp
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
 *
 *	@(#)DEFS.h	8.1 (Berkeley) 6/4/93
 */

#ifndef __PIC__
#define	PLTCALL(x)		x
#else
#define	PLTCALL(x)		x/**/@plt
#endif

#ifndef PROF

#define	ENTRY(x)			\
	.section ".text";		\
	.global x; 			\
	.type x,@function;		\
	.balign 4;			\
x: 

#else

#ifndef __PIC__

/*
 * The code below assumes that _mcount() is a leaf function that
 * takes its sole parameter in register 0.
 * _mcount() restores LR from the caller's frame
 * (hence it can't return using BLR).
 */
#define	ENTRY(x) 			\
	.section ".data";		\
	.balign 4;			\
.Lprofdata_/**/x:			\
	.long 0; 			\
	.section ".text"; 		\
	.global x;			\
	.type x,@function;		\
	.balign 4;			\
x:  					\
	mflr 0;				\
	lis 12,.Lprofdata_/**/x@ha;	\
	stw 0,4(1);			\
	la 0,.Lprofdata_/**/x@l(12);	\
	bl _mcount

#else	/* PIC and PROF */

#error "fatal error: we don't currently support PIC and PROF together"

#endif

#endif

#define	ENDENTRY(x)			\
.Lend_/**/x:				\
	.size x,.Lend_/**/x-x

#define	ASENTRY(x)	ENTRY(x)			/* compatibility */

/*
 * Support for user-space threads which require that some syscalls be
 * private to the threaded library.  We detect the initialization state
 * of the library here and call either the alternate threaded function
 * or the original one depending on the state.
 *
 * XXX This code makes unthreaded syscalls look normal in profiling output,
 * but threaded syscalls will appear to be called from the generic routine's
 * caller rather than generic routine itself.
 */

#ifndef __PIC__

#define PENTRY(x)			\
	ENTRY(x);			\
	.global _threads_initialized;	\
	lis 12,_threads_initialized@ha;	\
	lwz 12,_threads_initialized@l(12); \
	cmpwi 12,0;			\
	bne- .Lthread_/**/x;		\
	b _syscall_sys_/**/x;		\
	.global _thread_sys_/**/x;	\
.Lthread_/**/x:				\
	b _thread_sys_/**/x;		\
	ENTRY(_syscall_sys_/**/x); 

#else	/* PIC */

/*
 * The assembler makes us do most of the PIC work by hand
 * rather than processing it automatically like on Intel.
 * Note that R0, R11 and R12 are all scratch registers.
 */
#define PENTRY(x)			\
	.section ".text";		\
	.balign 4;			\
.Ltioff_/**/x:				\
	.long .Ltigot_/**/x-.Ltipic_/**/x; \
	.section ".got2","aw";		\
.Ltigot_/**/x:				\
	.global _threads_initialized;	\
	.long _threads_initialized;	\
	ENTRY(x);			\
	mflr 11;			\
	bl .Ltipic_/**/x;		\
.Ltipic_/**/x:				\
	mflr 12;			\
	lwz 0,(.Ltioff_/**/x-.Ltipic_/**/x)(12); \
	lwzx 12,12,0;			\
	mtlr 11;			\
	lwz 12,0(12);			\
	cmpwi 12,0;			\
	bne- .Lthread_/**/x;		\
	b _syscall_sys_/**/x@plt;	\
	.global _thread_sys_/**/x;	\
.Lthread_/**/x:				\
	b _thread_sys_/**/x@plt;	\
	ENTRY(_syscall_sys_/**/x); 

#endif
