/*	BSDI SYS.h,v 2.3 1998/08/17 02:11:27 torek Exp	*/

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
 *	@(#)SYS.h	8.1 (Berkeley) 6/4/93
 *
 * from: Header: SYS.h,v 1.2 92/07/03 18:57:00 torek Exp (LBL)
 */

#include <sys/syscall.h>
#include <machine/trap.h>

#include "DEFS.h"

/*
 * SET_ERRNO_PIC() sets errno via the global offset table method for PIC.
 * SET_ERRNO_NOPIC() sets errno for non-PIC code.  Only one of the two
 * expands to anything.
 */
#ifdef __PIC__
#define	SET_ERRNO_PIC(x, tmp, pic) \
	set errno,tmp; ld [pic+tmp], tmp; st x,[tmp]
#define	SET_ERRNO_NOPIC(x, tmp)
#else
#define	SET_ERRNO_PIC(x, tmp, pic)
#define	SET_ERRNO_NOPIC(x, tmp) \
	sethi %hi(errno),tmp; st x,[tmp+%lo(errno)]
#endif

/*
 * Most libc code turns out not to need PIC until an error occurs.
 * In general, we set errno from %o0 and then return -1, doing it
 * with the PIC two-step if necessary.  This ERROR() macro assumes
 * it is safe to clobber %g1 and/or %g2 and that PIC has not been
 * set up yet.
 *
 * Note that we do not need #ifdefs here since only one of the two
 * SET_ERRNOs expands to anything, and SET_PIC_NOFRAME vanishes for
 * non-PIC.
 */
#define	ERROR()	\
	SET_PIC_NOFRAME(%g1, %g2); SET_ERRNO_PIC(%o0, %g1, %g2); \
	SET_ERRNO_NOPIC(%o0, %g1); \
	mov -1,%o0; retl; mov -1,%o1

#define SYS(x)		CAT(SYS_,x)

/*
 * SYSCALL is used when further action must be taken before returning.
 * Note that it adds one instruction over what we could do, if we only
 * knew what came at label 2....
 */
#define	SYSCALL(x) \
	ENTRY(x); mov SYS(x),%g1; t ST_SYSCALL; bcc 2f; nop; ERROR(); 2:

/*
 * RSYSCALL is used when the system call should just return.  Here
 * we use the SYSCALL_G2RFLAG to put the `success' return address in %g2
 * and avoid a branch.
 */
#define	RSYSCALL(x) \
	ENTRY(x); \
	mov SYS(x)|SYSCALL_G2RFLAG,%g1; add %o7,8,%g2; t ST_SYSCALL; \
	ERROR()

/*
 * Support for user-space threads which require that some syscalls be
 * private to the threaded library.  We detect the initialization state
 * of the library here and call either the alternate threaded function
 * or the original one depending on the state.  In each case there are
 * three total entry points:
 *
 *	foo			(switches on _threads_initialized)
 *	_syscall_sys_foo	(just does the syscall)
 *	_thread_sys_foo		(in the pthreads library; implements threads)
 */
#define SYSCALL_SYS(x)	CAT(_syscall_sys_,x)
#define THREAD_SYS(x)	CAT(_thread_sys_,x)

#ifdef __PIC__
/*
 * Even though this defines, e.g., both sigpending() and
 * _syscall_sys_sigpending() in the same bit of code, the
 * sigpending() entry point must look up _syscall_sys_sigpending(),
 * so that user code can interpose in front of the latter.  On the
 * bright side, this means that profiling would look normal, if we
 * profiled PIC code, which we don't ...
 */
#define	PENTRY(x) \
	ENTRY(x); \
	SET_PIC_NOFRAME(%g1,%g2); set _threads_initialized,%g1; \
	ld [%g2+%g1],%g1; ld [%g1],%g1; tst %g1; \
	sethi %hi(SYSCALL_SYS(x)),%g1; be 0f; or %g1,%lo(SYSCALL_SYS(x)),%g1; \
	set THREAD_SYS(x),%g1; 0: \
	ld [%g2+%g1],%g1; jmpl %g1,%g0; nop; \
	ENTRY(SYSCALL_SYS(x))
#else
/*
 * This used to do some fancy two-stepping to fake out profiling, but
 * now we just show that whatever calls foo also calls _syscall_sys_foo.
 */
#define PENTRY(x) \
	ENTRY(x); \
	sethi %hi(_threads_initialized), %g1; \
	ld [%g1 + %lo(_threads_initialized)], %g1; \
	tst %g1; be SYSCALL_SYS(x); /*nop;*/ \
	sethi %hi(THREAD_SYS(x)),%g1; jmpl %g1+%lo(THREAD_SYS(x)),%g0; nop; \
	ENTRY(SYSCALL_SYS(x))
#endif

#define	PSYSCALL(x) \
	PENTRY(x); mov SYS(x),%g1; t ST_SYSCALL; bcc 2f; nop; ERROR(); 2:

#define	PRSYSCALL(x) \
	PENTRY(x); \
	mov SYS(x)|SYSCALL_G2RFLAG,%g1; add %o7,8,%g2; t ST_SYSCALL; \
	ERROR()

/*
 * PSEUDO(x,y) is like RSYSCALL(y) except that the name is x.
 */
#define	PSEUDO(x,y) \
	ENTRY(x); \
	mov SYS(y)|SYSCALL_G2RFLAG,%g1; add %o7,8,%g2; t ST_SYSCALL; \
	ERROR()

#define	ASMSTR		.asciz
