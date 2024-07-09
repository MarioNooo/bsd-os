/*	BSDI SYS.h,v 2.3 1997/10/31 03:14:06 donn Exp	*/

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
 *	@(#)SYS.h	8.1 (Berkeley) 6/4/93
 */

#define LOCORE		1

#include <sys/syscall.h>
#include <machine/segments.h>

#ifndef COMMIT
#define	NAME(x)		x
#define	SNAME(x)	_syscall_sys_/**/x
#else
#define	NAME(x)		commit_/**/x
#define SNAME(x)	_syscall_sys_commit_/**/x
#endif

#define	ENTRY(x)			\
	.section ".text";		\
	.global NAME(x); 		\
	.global SNAME(x); 		\
	.type NAME(x),@function;	\
	.type SNAME(x),@function;	\
	.align 4;			\
NAME(x): \
SNAME(x):

#define	ENDENTRY(x)			\
.Lend_/**/x:				\
	.size x,.Lend_/**/x-x

#ifndef COMMIT
#define	LCALL(x,y) \
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
#else
#define	LCALL(x,y) \
	movl $1,sig_state; \
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0; \
	movl $0,sig_state
#endif

#define	SET_ERRNO(x)			\
	.global errno;			\
	movl	x,errno

#define	SYSCALL(x)			\
2:					\
	SET_ERRNO(%eax);		\
	movl $-1,%eax;			\
	ret;				\
	ENTRY(x); 			\
	lea SYS_/**/x,%eax; 		\
	LCALL(0,0); 			\
	jb 2b

#define	RSYSCALL(x)			\
	SYSCALL(x); 			\
	ret

/* no thread support in emulator */
#define	PENTRY(x)	ENTRY(x)

#define	PSYSCALL(x)			\
2:					\
	SET_ERRNO(%eax);		\
	movl $-1,%eax;			\
	ret;				\
	PENTRY(x); 			\
	lea SYS_/**/x,%eax; 		\
	LCALL(0,0); 			\
	jb 2b

#define	PRSYSCALL(x)			\
	PSYSCALL(x); 			\
	ret

#define	PSEUDO(x,y)			\
	ENTRY(x); 			\
	lea SYS_/**/y, %eax; 		\
	LCALL(0,0); 			\
	ret

#define	ASMSTR		.string
