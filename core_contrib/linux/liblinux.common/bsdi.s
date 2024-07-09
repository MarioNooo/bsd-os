/*	BSDI bsdi.s,v 1.5 2003/09/23 17:46:34 donn Exp	*/

/*
 * BSD/OS support routines.
 */

#define	ASSEMBLY
#include <sys/syscall.h>
#include "liblinux.h"

	.file "bsdi.s"

	.section ".text"

	/*
	 * Make a syscall according to the syscall number at 4(%esp).
	 * We have to fiddle with the stack so that it looks one way
	 * to the kernel and a different way to our caller.
	 * This function is declared as a regparm(1) function --
	 * it gets the syscall number in EAX, so no stack games are needed.
	 */
	.global __bsdi_syscall
	.type __bsdi_syscall,@function
	.align 4
__bsdi_syscall:
	lcall $7,$0
	jc .Lerrno
	ret

	/*
	 * Common code to return an error value.
	 * We use the Linux convention of returning -errno so that
	 * concurrency is simple and emulating Linux kernel calls is cheap.
	 */
.Lerrno:
	negl %eax
	ret

	/*
	 * Like __bsdi_syscall, but for syscalls that require
	 * quad alignment for the argument list.
	 * Not a problem on i386.
	 */
	.global __bsdi_qsyscall
	.type __bsdi_qsyscall,@function
	.align 4
__bsdi_qsyscall:
	lcall $7,$0
	jc .Lerrno_ll
	ret
.Lerrno_ll:
	negl %eax
	movl $-1,%edx		/* make sure that the high word is negative */
	ret

	/*
	 * Fork() returns zero in EDX in the parent, nonzero in the child.
	 */
	.global __bsdi_fork
	.type __bsdi_fork,@function
	.align 4
__bsdi_fork:
	movl $SYS_fork,%eax
	lcall $7,$0
	jc .Lerrno
	cmpl $0,%edx
	je 1f
	xorl %eax,%eax
1:
	ret

	/*
	 * Unfortunately, real vfork() may only be called from
	 * a function that does not return before calling exec.
	 * Therefore we must disable vfork()'s memory sharing
	 * behavior; we can select this explicitly using sfork().
	 */
	.global __bsdi_vfork
	.type __bsdi_vfork,@function
	.align 4
__bsdi_vfork:
	pushl %ecx
	pushl $SIGCHLD		/* Send SIGCHLD on exit.  */
	pushl $0		/* NULL stack means use the current stack.  */
	pushl $SF_WAITCHILD
	pushl %ecx
	movl $SYS_sfork,%eax
	lcall $7,$0
	popl %ecx
	popl %ecx
	popl %ecx
	popl %ecx
	popl %ecx
	jc .Lerrno
	cmpl $0,%edx
	je 1f
	xorl %eax,%eax
1:
	ret

	/*
	 * This works like fork(), but the child starts on a new stack.
	 * The start address of the child is at the bottom of that stack.
	 */
	.global __bsdi_sfork
	.type __bsdi_sfork,@function
	.align 4
__bsdi_sfork:
	movl $SYS_sfork,%eax
	lcall $7,$0
	jc .Lerrno
	cmpl $0,%edx
	jne 0f
	ret			/* parent returns here */
0:
	xorl %ebp,%ebp		/* be nice to GDB */
	ret			/* child returns here */

	/*
	 * Pipe() returns file descriptors in EAX and EDX.
	 */
	.global __bsdi_pipe
	.type __bsdi_pipe,@function
	.align 4
__bsdi_pipe:
	movl $SYS_pipe,%eax
	lcall $7,$0
	jc .Lerrno
	movl 4(%esp),%ecx	/* get the pointer */
	movl %eax,(%ecx)	/* store fd[0] */
	movl %edx,4(%ecx)	/* store fd[1] */
	xorl %eax,%eax
	ret

	/*
	 * The BSD getpriority() may legitimately return negative values,
	 * which is impossible with the Linux errno encoding.
	 * On Linux, the value is 'normalized' to a positive number,
	 * so we do that here too.
	 */
	.global __bsdi_getpriority
	.type __bsdi_getpriority,@function
	.align 4
__bsdi_getpriority:
	movl $SYS_getpriority,%eax
	lcall $7,$0
	jc .Lerrno
	addl $LINUX_PZERO,%eax
	ret
