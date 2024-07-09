/*	BSDI Ovfork.s,v 2.5 2000/09/15 16:04:03 donn Exp	*/

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

	.file "Ovfork.s"

#if defined(SYSLIBC_SCCS) && !defined(lint)
	.section ".rodata"
	.asciz "@(#)Ovfork.s	8.1 (Berkeley) 6/4/93"
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"
#include "ERRNO.h"

#define	LOCORE
#include <sys/sfork.h>

/* We should include <signal.h>...  */
#define	SIGCHLD		20	/* XXX */

/*
 * pid_t vfork(void);
 *
 * %edx == 0 in parent process, %edx == 1 in child process.
 * %eax == pid of child in parent, %eax == pid of parent in child.
 *
 * We implement vfork() using the sfork() system call.
 */

PENTRY(vfork)
	popl %ecx		/* save return address */
	pushl $SIGCHLD		/* exit signal */
	pushl $0		/* stack pointer */
	pushl $SF_MEM|SF_WAITCHILD
	pushl %ecx		/* dummy return address */
	movl $SYS_sfork, %eax
	lcall $7,$0
	jb .Lverror
.Lvforkok:
	cmpl $0,%edx		/* child process? */
	jne .Lchild		/* yes */
	jmp  .Lparent 
.Lverror:
	addl $16,%esp		/* restore the stack */
	pushl %ecx
	SET_ERRNO(%eax)		/* PIC version of SET_ERRNO() destroys %ecx */
	movl $-1,%eax
	ret
.Lchild:
	movl $0,%eax
.Lparent:
	addl $16,%esp		/* pop the sfork() arguments */
	jmp *%ecx		/* jump to the original return address */
