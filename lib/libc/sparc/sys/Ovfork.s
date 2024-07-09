/*	BSDI Ovfork.s,v 2.3 2000/09/15 23:39:06 donn Exp	*/

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
 * from: /master/lib/libc/sparc/sys/Ovfork.s,v 2.3 2000/09/15 23:39:06 donn Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)Ovfork.s	8.1 (Berkeley) 6/4/93"
#endif /* LIBC_SCCS and not lint */

/*
 * pid = vfork();
 *
 * %o1 == 0 in parent process, 1 in child process.
 * %o0 == pid of child in parent, pid of parent in child.
 *
 * We simulate vfork() with sfork().
 */

#include "SYS.h"

#define LOCORE 1
#include <sys/sfork.h>

#define	SIGCHLD		20	/* XXX */

PENTRY(vfork)
	set	SF_WAITCHILD|SF_MEM, %o0	! sfork flags
	mov	0, %o1				! sfork stack
	mov	SIGCHLD, %o2			! sfork exit signal
	mov	SYS_sfork, %g1
	t	ST_SYSCALL
	bcc,a	2f
	 dec	%o1		! from 1 to 0 in child, 0 to -1 in parent
	ERROR()
2:
	retl
	 and	%o0, %o1, %o0	! return 0 in child, pid in parent