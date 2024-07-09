/*
 * Copyright (c) 1993, 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_glue.s,v 2.3 1997/11/01 04:50:15 donn Exp
 */

/*
 * Miscellaneous assembly code for various SCO-dependent activities.
 */

#include "SYS.h"
#include <sys/syscall.h>
#include <machine/segments.h>

#include "sco_sig_state.h"

#define	DEFAULT_STACKSIZE	8192

/* Offsets to program registers saved on the emulator stack. */
#define	PROGRAM_EAX	-4
#define	PROGRAM_ECX	-8
#define	PROGRAM_EDX	-12
#define	PROGRAM_EBX	-16
#define	PROGRAM_EBP	-24
#define	PROGRAM_ESI	-28
#define	PROGRAM_EDI	-32
#define	PROGRAM_PS	-36
#define	PROGRAM_FP	-(36+SIZEOF_SAVE87)

/* Offsets in sigframe; they include the return EIP installed by sigcode. */
#define	SF_SIGNUM	4
#define	SF_CODE		8
#define	SF_SCP		12
#define	SF_HANDLER	16
#define	SF_EAX		20
#define	SF_EDX		24
#define	SF_ECX		28
#define	SF_FPU		32
#define	SC_ONSTACK	156
#define	SC_MASK		160
#define	SC_SP		164
#define	SC_FP		168
#define	SC_AP		172
#define	SC_PC		176
#define	SC_PS		180

#define	SIZEOF_SIGFRAME	184

#define	SIGILL		4		/* XXX */
#define	SIGTRAP		5		/* XXX */

#define	SIG_SETMASK	3		/* XXX */

#define	LCALL_OP	0x9a		/* LCALL opcode */
#define	LCALL_LEN	7		/* 0x9a + 6 byte far pointer */

#define	SIZEOF_SAVE87	124

	.comm sig_state,4,4
	.comm sig_pending_mask,4,4
	.comm sig_saved_mask,4,4
	.comm sig_prev_mask,4,4
	.comm sig_saved_eip,4,4
	.comm sig_saved_esp,4,4
	.comm sig_last_pass,4,4

	.section ".text"

/*
 * Signal support.
 */

	/*
	 * Handle incoming signals.
	 * The emulator takes signals rather than the program,
	 * so that it can translate the signal number argument and
	 * so that we can guarantee that we won't re-enter the emulator.
	 * XXX This will screw up with alternate signal stacks.
	 * XXX Fortunately iBCS2 doesn't define these.
	 */
ENTRY(sco_sig_handler_glue)

	/* are we on the program stack? */
	movl emulate_stack,%eax
	cmpl %esp,%eax
	jbe .Lin_program
	subl $DEFAULT_STACKSIZE,%eax
	cmpl %esp,%eax
	jbe .Lin_emulator

.Lin_program:
	/*
	 * We execute the following on the program stack.
	 * We have come here from the 'sigcode' glue;
	 * our stack consists of a return pointer and a struct sigframe.
	 * All signals are blocked.
	 * We must translate the signal number on the stack,
	 * then unblock exactly the right signals depending
	 * on whether this is a POSIX signal handler, or an old-style one.
	 * When we're done, we can jump to the program's handler.
	 */

	/* map the signal number */
	movl SF_SIGNUM(%esp),%ecx
	movl sig_out_map(,%ecx,4),%eax
	movl %eax,SF_SIGNUM(%esp)

	/* get the handler address and save it on the stack */
	movl %ecx,%eax				/* multiply index by 12 */
	shll $2,%eax
	movl sco_action(%eax,%ecx,8),%eax
	pushl %eax

	/* prepare to fix the signal mask with sigprocmask() */
	movl SC_MASK(%esp),%edx		/* grab the original mask */
	movl %ecx,%eax
	decl %eax
	btl %eax,sco_reset_on_sig	/* sigaction() or signal()? */
	jc .Lold_style_signal

	/* sigaction(): mask in sigmask(sig) and sa_mask */
	btsl %eax,%edx
	orl sco_bsd_sa_mask(,%ecx,4),%edx
	jmp .Lsetprocmask

.Lold_style_signal:
	/* signal(): if not SIGILL or SIGTRAP, reset action to SIG_DFL */
	/* this craziness is documented in signal(5) in SVr4.2 SFDR p 357 */
	cmpl $SIGILL,%ecx
	je .Lsetprocmask
	cmpl $SIGTRAP,%ecx
	je .Lsetprocmask

	/* movl $0,sco_bsd_sa_mask(,%ecx,4) -- clear BSD sa_mask, already 0 */
	movl %ecx,%eax
	shll $2,%eax
	leal sco_action(%eax,%ecx,8),%eax
	movl $0,(%eax)			/* clear SCO sa_handler */
	/* movl $0,4(%eax) -- SCO sa_mask should already be 0 */
	movl $0,8(%eax)			/* clear SCO sa_flags */

	/* call sigaction() to fix kernel's version too */
	pushl $0
	pushl %eax			/* all zeroes, so it's BSD format too */
	pushl %ecx
	pushl $0
	movl $SYS_sigaction,%eax
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	addl $16,%esp

.Lsetprocmask:
	pushl %edx
	pushl $SIG_SETMASK
	pushl $0
	movl $SYS_sigprocmask,%eax
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	addl $12,%esp

	/* jump to the user's handler */
	ret

.Lin_emulator:
	/*
	 * We've caught a signal on the emulator stack.
	 * All signals are blocked.
	 * To deal with the signal, we must get out of the emulator and
	 * execute the user's handler on the user stack.
	 * Depending on sig_state, we will either leave the emulator now,
	 * or return and finish execution.
	 * In either case, on exiting the emulator we will end up
	 * on the user stack executing signal handler support code.
	 */

	/*
	 * If this is our first signal, save some information
	 * about our 'ground' state.
	 */
	cmpl $0,sig_pending_mask
	jne .Lsave_signal

	/*
	 * Recover the user ESP and EIP and save them away.
	 */

	/* did we take a signal while switching stacks? */
	cmpl $saving_program_stack,SC_PC(%esp)
	jne 0f

	/* we did; the program stack pointer is in the saved EAX */
	movl SF_EAX(%esp),%edx
	jmp 1f

0:
	/* pick up the program stack pointer and return EIP */
	movl emulate_stack,%eax
	movl -4(%eax),%edx
1:
	movl (%edx),%eax		/* recover the return EIP */
	movl %eax,sig_saved_eip

	/*
	 * Patch the program frame so that
	 * the emulator returns to glue code.
	 */
	movl $sco_emulator_return_sig_glue,(%edx)

	/* 
	 * Save the original signal mask.
	 */
	movl SC_MASK(%esp),%eax
	movl %eax,sig_saved_mask

.Lsave_signal:
	/*
	 * Record the pending signal.
	 */
	movl SF_SIGNUM(%esp),%ecx
	decl %ecx
	movl $1,%eax
	shll %cl,%eax
	orl %eax,sig_pending_mask

	/*
	 * Figure out our signal state.
	 */
	cmpl $SIG_COMMIT,sig_state
	jne 1f

	/* check the opcode */
	movl SC_PC(%esp),%eax
	movb (%eax),%al
	cmpb $LCALL_OP,%al
	jne 0f

	/* pointing at LCALL => signal arrived before we entered syscall */
	movl $SIG_UNWIND,sig_state
	jmp 1f
0:
	/* pointing after LCALL => signal arrived during or after syscall */
	movl $SIG_POSTPONE,sig_state
1:

	/*
	 * If we're in the POSTPONE state,
	 * we block the signal and resume execution in the emulator.
	 * If we were executing a system call,
	 * this signal may have interrupted it, which is fine.
	 */
	cmpl $SIG_POSTPONE,sig_state
	jne .Lunwind

	/* block pending signals */
	movl sig_pending_mask,%eax
	orl %eax,SC_MASK(%esp)

	/* return to trampoline and execute a sigreturn() */
	ret

#ifdef DEBUG
	/* XXX print something as evidence that we exercised this code */
	.section ".rodata"
.Lwarn:
	.string "unwinding state after signal"
	.section ".text"
#endif
.Lunwind:
#ifdef DEBUG
	cmpl $0,debug
	je 0f
	pushl $.Lwarn
	call warnx
	popl %eax
0:
#endif

	/*
	 * Adjust the current signal frame on the emulator stack
	 * so that when we return, we execute the standard signal glue
	 * and run the user's handler, then restart the system call.
	 */
	subl $LCALL_LEN,sig_saved_eip
	movl $sco_emulator_return_sig_glue,SC_PC(%esp)

	movl emulate_stack,%edx
	movl PROGRAM_PS(%edx),%eax
	movl %eax,SC_PS(%esp)
	movl PROGRAM_EBP(%edx),%eax
	movl %eax,SC_FP(%esp)
	movl PROGRAM_EAX(%edx),%eax	/* work around emulate_glue hackery */
	addl $8,%eax
	movl %eax,SC_SP(%esp)
#ifdef DEBUG
	movl $~(1<<(SIGTRAP-1)),SC_MASK(%esp)
#else
	movl $-1,SC_MASK(%esp)
#endif
	movl -4(%eax),%eax
	movl %eax,SF_EAX(%esp)		/* syscall number */
	movl PROGRAM_ECX(%edx),%eax
	movl %eax,SF_ECX(%esp)
	movl PROGRAM_EDX(%edx),%eax
	movl %eax,SF_EDX(%esp)

	movl PROGRAM_EBX(%edx),%ebx
	movl PROGRAM_ESI(%edx),%esi
	movl PROGRAM_EDI(%edx),%edi

	ret
ENDENTRY(sco_sig_handler_glue)

	/*
	 * We reach here when leaving the emulator
	 * after a signal arrived during emulation.
	 * We create the necessary signal frame(s) and
	 * then pretend we were called from sigcode.
	 */
ENTRY(sco_emulator_return_sig_glue)

	/*
	 * Save registers and flags before we change anything.
	 * If a signal somehow sneaks in here before we can block it,
	 * we're still guaranteed that the registers won't change
	 * from our point of view.
	 */
	pushal
	pushfl

	/* block all signals */
#ifdef DEBUG
	pushl $~(1<<(SIGTRAP-1))
#else
	pushl $-1
#endif
	pushl $SIG_SETMASK
	pushl $0
	movl $SYS_sigprocmask,%eax
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	addl $12,%esp

	/* extract floating point state */
	subl $SIZEOF_SAVE87,%esp
	fnsave (%esp)

	/* initialize some loop variables */
	leal -(PROGRAM_FP)(%esp),%edx
	movl %edx,sig_saved_esp
	movl sig_pending_mask,%ebx
	movl $0,sig_pending_mask
	movl $0,sig_prev_mask
	movl $0,sig_last_pass

	/*
	 * Loop, creating a signal frame for each pending signal,
	 * plus an extra one for us to return from.
	 * Note that we create BSD-style frames and
	 * let sco_sig_handler_glue() patch them up.
	 */
.Lsig_loop:
	bsfl %ebx,%ecx
	jnz 0f
	btsl $0,sig_last_pass
	jc .Lsig_start_handler

0:
	btrl %ecx,%ebx
	subl $SIZEOF_SIGFRAME,%esp

	/*
	 * Set the mask up so that when we sigreturn(),
	 * we are blocking the appropriate signal(s) in the frame
	 * that we are returning to.
	 */
	movl sig_saved_mask,%eax
	orl sig_prev_mask,%eax
	movl %eax,SC_MASK(%esp)
	xorl %eax,%eax
	btsl %ecx,%eax
	movl %eax,sig_prev_mask

	incl %ecx
	movl %ecx,SF_SIGNUM(%esp)
	movl $0,SF_CODE(%esp)
	leal SC_ONSTACK(%esp),%eax
	movl %eax,SF_SCP(%esp)
	movl $sco_sig_handler_glue,SF_HANDLER(%esp)
	movl PROGRAM_EAX(%edx),%eax
	movl %eax,SF_EAX(%esp)
	movl PROGRAM_EDX(%edx),%eax
	movl %eax,SF_EDX(%esp)
	movl PROGRAM_ECX(%edx),%eax
	movl %eax,SF_ECX(%esp)

	movl $SIZEOF_SAVE87/4,%ecx
	leal PROGRAM_FP(%edx),%esi
	leal SF_FPU(%esp),%edi
	cld
	rep; movsl

	movl $0,SC_ONSTACK(%esp)

	/* took care of SC_MASK above, while ECX was still useful */

	movl sig_saved_esp,%eax
	movl %eax,SC_SP(%esp)
	leal 4(%esp),%eax
	movl %eax,sig_saved_esp

	movl PROGRAM_EBP(%edx),%eax
	movl %eax,SC_FP(%esp)
	movl $0,SC_AP(%esp)

	movl sig_saved_eip,%eax
	movl %eax,SC_PC(%esp)
	movl $.Lsig_trampoline,sig_saved_eip

	movl PROGRAM_PS(%edx),%eax
	movl %eax,SC_PS(%esp)

	jmp .Lsig_loop

.Lsig_start_handler:
	movl PROGRAM_EBX(%edx),%ebx
	movl PROGRAM_ESI(%edx),%esi
	movl PROGRAM_EDI(%edx),%edi

	/* fake a return to sigcode */
	movl $.Lsig_trampoline_epilogue,(%esp)
	ret
ENDENTRY(sco_emulator_return_sig_glue)

	/*
	 * A copy of the kernel's trampoline code, in a predictable place.
	 */
	.section ".text"
	.align 4
.Lsig_trampoline:
	movl (SF_HANDLER)-4(%esp),%eax
	call *%eax
.Lsig_trampoline_epilogue:
	movl $SYS_sigreturn,%eax
	lcall $LSEL(L43BSDCALLS_SEL,SEL_UPL),$0
	hlt			/* never returns */
