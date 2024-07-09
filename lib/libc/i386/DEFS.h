/*
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI DEFS.h,v 2.5 2003/06/05 16:34:36 polk Exp
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

#if __STDC__
#define CAT(a,b) a##b
#else
#define CAT(a,b) a/**/b
#endif

#ifndef PROF

#define	ENTRY(x)			\
	.section ".text";		\
	.global x; 			\
	.type x,@function;		\
	.align 4;			\
x: 

#else

#ifndef REGSAVE
#define	REGSAVE
#define	REGRESTORE
#endif

#ifndef __PIC__

#define	ENTRY(x) 			\
	.section ".data";		\
CAT(.Lprofdata_,x):			\
	.long 0; 			\
	.section ".text"; 		\
	.global x;			\
	.type x,@function;		\
	.align 4;			\
x:  					\
	REGSAVE;			\
	pushl %ebp; 			\
	movl %esp,%ebp; 		\
	leal CAT(.Lprofdata_,x),%eax; 	\
	call _mcount; 			\
	leave;				\
	REGRESTORE

#else	/* PIC and PROF */

#define	ENTRY(x) 			\
	.section ".data";		\
CAT(.Lprofdata_,x):			\
	.long 0; 			\
	.section ".text"; 		\
	.global x;			\
	.type x,@function;		\
	.align 4;			\
x:  					\
	REGSAVE;			\
	pushl %ebp; 			\
	movl %esp,%ebp; 		\
	call CAT(.Lprof_,x);		\
CAT(.Lprof_,x):				\
	popl %ecx;			\
	addl $_GLOBAL_OFFSET_TABLE_+[.-CAT(.Lprof_,x)],%ecx; \
	leal CAT(.Lprofdata_,x)@GOTOFF(%ecx),%eax; \
	call _mcount@PLT; 		\
	leave;				\
	REGRESTORE

#endif

#endif

#define	ENDENTRY(x)			\
CAT(.Lend_,x):				\
	.size x,CAT(.Lend_,x)-x

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
	cmpl $0,_threads_initialized;	\
	je CAT(_syscall_sys_,x);	\
	.global CAT(_thread_sys_,x);	\
	jmp CAT(_thread_sys_,x);	\
	ENTRY(CAT(_syscall_sys_,x)); 

#else	/* PIC */

/*
 * XXX This code is pretty nasty.
 * XXX We can't jump indirect through the PLT here,
 * XXX because we can't set EBX to point at the GOT.
 * XXX Instead, we relocate a pair of function pointers.
 */
#define PENTRY(x)			\
	.section ".data";		\
	.align 4;			\
	.global CAT(_thread_sys_,x);	\
CAT(.Lthread_sys_ptr_,x):		\
	.long CAT(_thread_sys_,x);	\
CAT(.Lsyscall_sys_ptr_,x):		\
	.long CAT(_syscall_sys_,x);	\
	ENTRY(x);			\
	.global _threads_initialized;	\
	call CAT(.Lthreadinit_,x);	\
CAT(.Lthreadinit_,x):			\
	popl %ecx;			\
	addl $_GLOBAL_OFFSET_TABLE_+[.-CAT(.Lthreadinit_,x)],%ecx; \
	movl _threads_initialized@GOT(%ecx),%eax; \
	cmpl $0,(%eax);			\
	jne CAT(.Lthread_sys_,x);	\
	movl CAT(.Lsyscall_sys_ptr_,x)@GOTOFF(%ecx),%eax; \
	jmp *%eax;			\
CAT(.Lthread_sys_,x):			\
	movl CAT(.Lthread_sys_ptr_,x)@GOTOFF(%ecx),%eax; \
	jmp *%eax;			\
	ENTRY(CAT(_syscall_sys_,x)); 

#endif
