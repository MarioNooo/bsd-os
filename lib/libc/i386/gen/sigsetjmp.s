/*-
 * Copyright (c) 1993,1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sigsetjmp.s,v 2.3 1997/08/02 21:50:00 donn Exp
 */

	.file "sigsetjmp.s"

#include "DEFS.h"

#define	_JBLEN	10	/* XXX from setjmp.h */

#ifndef __PIC__
#define	SIGPROCMASK	sigprocmask
#define	FIX_EBX
#define	RESTORE_EBX
#else
#define	SIGPROCMASK	sigprocmask@PLT
#define	FIX_EBX \
	pushl %ebx; \
	call 0f; \
0: \
	popl %ebx; \
	addl $_GLOBAL_OFFSET_TABLE_+[.-0b],%ebx
#define	RESTORE_EBX	popl %ebx
#endif

#define	SIG_SETMASK		3	/* XXX from signal.h */

ENTRY(sigsetjmp)
	movl	4(%esp),%ecx
	movl	8(%esp),%eax
	movl	%eax,(_JBLEN*4)(%ecx)
	testl	%eax,%eax
	jz	.Lsj_no_sig_mask

	FIX_EBX
	leal	24(%ecx),%edx
	pushl	%edx
	pushl	$0
	pushl	$SIG_SETMASK
	call	SIGPROCMASK
	addl	$12,%esp
	RESTORE_EBX
	movl	4(%esp),%ecx

.Lsj_no_sig_mask:
	movl	0(%esp),%edx
	movl	%edx, 0(%ecx)
	movl	%ebx, 4(%ecx)
	movl	%esp, 8(%ecx)
	movl	%ebp,12(%ecx)
	movl	%esi,16(%ecx)
	movl	%edi,20(%ecx)
	movl	$0,%eax
	ret

ENTRY(siglongjmp)
	movl	4(%esp),%edx
	cmpl	$0,(_JBLEN*4)(%edx)
	jz	.Llj_no_sig_mask

	FIX_EBX
	leal	24(%edx),%eax
	pushl	$0
	pushl	%eax
	pushl	$SIG_SETMASK
	call	SIGPROCMASK
	addl	$12,%esp
	RESTORE_EBX
	movl	4(%esp),%edx

.Llj_no_sig_mask:
	movl	8(%esp),%eax
	movl	0(%edx),%ecx
	movl	4(%edx),%ebx
	movl	8(%edx),%esp
	movl	12(%edx),%ebp
	movl	16(%edx),%esi
	movl	20(%edx),%edi
	cmpl	$0,%eax
	jne	1f
	movl	$1,%eax
1:	movl	%ecx,0(%esp)
	ret
