/*
 * Copyright (c) 1993, 1994, 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI emulate_glue.s,v 2.4 1997/10/31 03:13:56 donn Exp
 */

#include "SYS.h"
#include "sco_errno.h"
#include "sco_sig_state.h"

	.global sig_state

	/*
	 * 'start' is the entry point for the emulator library.
	 */
ENTRY(start)
	xorl $1,%edi		/* EDI == 1 if COFF, 0 otherwise */
	movl %edi,bsd		/* invert and store in global bsd flag */
	movl %ebx,__ps_strings	/* grab the ps_strings pointer */
	pushl %esi		/* saved entry point, passed from exec */
	call emulate_start
	popl %esi
	jmp *%esi
ENDENTRY(start)

	/*
	 * Special, more efficient glue code for BSD 'emulation'.
	 * If we don't modify the semantics of this system call,
	 * we simply dispatch it to the kernel via the appropriate gate.
	 * Otherwise, we follow the usual procedure.
	 */
ENTRY(bsd_emulate_glue)
	pushfl
	cmpl %eax,bsd_syscall_max
	jb .Lbsd_direct
	cmpl $0,bsd_syscall(,%eax,4)
	jne .Lbsd_emulated

.Lbsd_direct:
	popfl		/* restore flags */
	popl (%esp)	/* adjust the stack ... */
	popl %ecx	/* ... and save the return value in ECX */
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	jmp *%ecx	/* sure hope the kernel preserves ECX */

.Lbsd_emulated:
	popfl
	jmp emulate_glue
ENDENTRY(bsd_emulate_glue)
 
	/*
	 * Enter the emulator after an LCALL from the program.
	 * We swap stacks and call an appropriate service routine.
	 */
ENTRY(emulate_glue)
	/*
	 * Here we take on the very sensitive situation of stack switching.
	 * The problem is due to the asynchronous arrival of signals.
	 * If a signal arrives while we're on the program stack,
	 * everything is fine -- we just take the signal and keep going.
	 * If a signal arrives after we've saved the program stack pointer
	 * on the emulator stack, again everything's fine --
	 * the emulator's handler glue code can find the program stack
	 * and save the signal frame there.
	 * (Note that we can't modify the program stack pointer
	 * after we switch stacks, since that will cause the saved
	 * signal frame to lie in a random location on the program stack.)
	 * If a signal arrives after we've installed a new stack but
	 * before we've saved the program stack pointer, there's trouble.
	 *
	 * The solution chosen here is a variation on Bershad's locking hack.
	 * When a signal arrives, the handler checks the saved EIP.
	 * If the EIP points at the instruction which saves the program ESP,
	 * the handler recovers the saved ESP from register state
	 * instead of from the emulator stack.
	 */

	/*
	 * Save the syscall number on the stack temporarily,
	 * to get some wiggle room.
	 */
	movl %eax,4(%esp)	/* stomp on saved CS */

	movl %esp,%eax
	movl emulate_stack,%esp

	.global saving_program_stack
saving_program_stack:
	/* Save all registers, so we can unwind on certain signals. */
	pushal
	pushfl

	/*
	 * We push a copy of the program stack pointer, so that
	 * the syscall handler can't modify the original one accidentally.
	 */
	pushl %eax

	/* Restore the syscall number to EAX. */
	movl 4(%eax),%eax

	/*
	 * Find the syscall service routine for this syscall.
	 */
	cmpl $0,bsd
	je .Libcs2_syscall

	movl bsd_syscall(,%eax,4),%eax
	jmp .Lemulate_syscall

.Libcs2_syscall:
	cmpl %eax,low_syscall_max
	jb .Lhigh_syscall

	movl low_syscall(,%eax,4),%eax
	jmp .Lemulate_syscall

	/*
	 * For some reason, the iBCS2 defines a number of syscalls
	 * with a low byte of 40 and a high byte in the range from 36 to 48.
	 */
.Lhigh_syscall:
	cmpb $40,%al
	jne .Lnosys
	cmpl %eax,high_syscall_min
	ja .Lnosys
	cmpl %eax,high_syscall_max
	jb .Lnosys

	subl high_syscall_min,%eax
	movzbl %ah,%eax
	movl high_syscall(,%eax,4),%eax

.Lemulate_syscall:
	call *%eax
	addl $(7*4),%esp	/* pop syscall number, EFLAGS and EDI-EBX */

	/*
	 * Translate back from the C syscall convention to assembly.
	 * No adds or subtracts after this point (must save the CF bit).
	 */
	cmpl $-1,%eax
	clc
	jne .Lreturn_to_program
	movl errno,%eax

	/* Translate errno into program format. */
	cmpl $0,bsd
	jne 2f
	testl $SCO_ERRNO,%eax
	jne 1f
	movl errno_out_map(,%eax,4),%eax
	jmp 2f
1:
	andl $~SCO_ERRNO,%eax
2:
	stc

.Lreturn_to_program:
	popl %edx		/* these may have been modified indirectly */
	popl %ecx
	movl (%esp),%esp

	movl $SIG_UNWIND,sig_state

	/*
	 * This hack should be 10 clocks faster than
	 * fixing the saved CS and doing an LRET (yawn).
	 * Note that ESP's value is used AFTER incrementing.
	 */
	popl (%esp)
	ret

.Lnosys:
	pushl %eax		/* the syscall number */
	call nosys		/* probably won't return */
	addl $8,%esp
	movl $SCO_ENOSYS,%eax
	stc
	jmp .Lreturn_to_program
ENDENTRY(emulate_glue)
