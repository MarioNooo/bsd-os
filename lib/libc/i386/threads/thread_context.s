/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_context.s,v 1.7 2000/01/20 18:57:13 donn Exp
 */

	.file "thread_context.s"

#include "DEFS.h"

#ifdef KTR
	.section ".rodata"
.Lstring1:   .string  "_thread_machdep_savectx"
#endif

/*
 * entry parm 1 (trace parm 1) - return address
 * entry parm 2 - context save area
 * trace parm 1 - context save area
 * trace parm 2 - saved ESP
 */
ENTRY(_thread_machdep_savectx)
#ifdef KTR
#ifndef __PIC__
	movl	pttr_idx,%eax
	movl	%eax,%ecx
	incl	%ecx
	andl	pttr_size_mask,%ecx
	movl	%ecx,pttr_idx
	leal	(%eax,%eax,4),%eax		/* EAX *= 5 */
	leal	pttr_buf(,%eax,4),%ecx		/* ECX = &pttr_buf + EAX * 4 */
	movl	$.Lstring1,8(%ecx)
#else /* PIC */
	pushl	%ebx
	pushl	%esi

	call	1f
1:
	popl	%ebx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b],%ebx

	movl	pttr_idx@GOT(%ebx),%eax
	movl	(%eax),%esi
	movl	%esi,%ecx			/* save old value */
	incl	%esi
	movl	pttr_size_mask@GOT(%ebx),%edx
	andl	(%edx),%esi
	movl	%esi,(%eax)			/* update pttr_idx */

	movl	pttr_buf@GOT(%ebx),%edx
	leal	(%ecx,%ecx,4),%ecx		/* EBX *= 5 */
	leal	(%edx,%ecx,4),%ecx		/* EBX = &pttr_buf + EBX * 4 */

	leal	.Lstring1@GOTOFF(%ebx),%edx
	movl	%edx,8(%ecx)

	popl	%esi
	popl	%ebx
#endif	/* PIC */
	rdtsc
	movl	%edx,(%ecx)
	movl	%eax,4(%ecx)
	movl	4(%esp),%eax
	movl	%eax,12(%ecx)
	movl	%esp,16(%ecx)
#endif	/* KTR */
	movl	4(%esp),%ecx 
	movl	(%esp),%edx
	movl	%edx,(%ecx)
	movl	%ebx,4(%ecx)
	movl	%esp,8(%ecx)
	movl	%ebp,12(%ecx)
	movl	%esi,16(%ecx)
	movl	%edi,20(%ecx)
	fnsave	24(%ecx)		/* floating point state */
	fldcw	24(%ecx)		/* restore FP control word */
	movl	$0,%eax
	ret

#ifdef DEBUG
ENTRY(_thread_machdep_bug)
	ret
#endif

/*
 * entry parm 1 - ptr to save area
 * entry parm 2 - value to return to new context
 *
 * trace parm 1 - save area
 * trace parm 2 - saved ESP
 */

#ifdef KTR
	.section ".rodata"
.Lstring2:   .string  "_thread_machdep_restctx"
#endif

ENTRY(_thread_machdep_restctx)
#ifdef KTR
#ifndef __PIC__
	movl	pttr_idx,%eax
	movl	%eax,%ecx
	incl	%ecx
	andl	pttr_size_mask,%ecx
	movl	%ecx,pttr_idx
	leal	(%eax,%eax,4),%eax		/* EAX *= 5 */
	leal	pttr_buf(,%eax,4),%ecx		/* ECX = &pttr_buf + EAX * 4 */
	movl	$.Lstring2,8(%ecx)
#else /* PIC */
	pushl	%ebx
	pushl	%esi

	call	1f
1:
	popl	%ebx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b],%ebx

	movl	pttr_idx@GOT(%ebx),%eax
	movl	(%eax),%esi
	movl	%esi,%ecx			/* save old value */
	incl	%esi
	movl	pttr_size_mask@GOT(%ebx),%edx
	andl	(%edx),%esi
	movl	%esi,(%eax)			/* update pttr_idx */

	movl	pttr_buf@GOT(%ebx),%edx
	leal	(%ecx,%ecx,4),%ecx		/* EBX *= 5 */
	leal	(%edx,%ecx,4),%ecx		/* EBX = &pttr_buf + EBX * 4 */

	leal	.Lstring2@GOTOFF(%ebx),%edx
	movl	%edx,8(%ecx)

	popl	%esi
	popl	%ebx
#endif	/* PIC */
	rdtsc
	movl	%edx,(%ecx)
	movl	%eax,4(%ecx)
	movl	4(%esp),%edx
	movl	%edx,12(%ecx)
	movl	8(%edx),%ebx
	movl	%ebx,16(%ecx)
#endif
	movl	4(%esp),%edx
	movl	8(%esp),%eax
	movl	(%edx),%ecx
	movl	4(%edx),%ebx
	movl	8(%edx),%esp
	movl	12(%edx),%ebp
	movl	16(%edx),%esi
	movl	20(%edx),%edi
	frstor	24(%edx)		/* floating point state */
	testl	%eax,%eax
	jne	1f
	movl	$1,%eax
1:	movl	%ecx,(%esp)
	ret
