/*
 * Copyright (c) 1992, 1993, 1996
 *	Berkeley Software Design, Inc.  All rights reserved.
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
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI signal.c,v 2.4 2003/07/08 21:57:40 polk Exp
 */

#include "doscmd.h"
#include <setjmp.h>
#include <signal.h>

int jmp_okay = 0;
static void (*handler[NSIG])(struct sigframe *, struct trapframe *);
static char signal_stack[16 * 1024];

#define PSS(w) { char s; printf(w " @ %08x\n", (signal_stack + sizeof signal_stack) - &s); }

struct sigframe saved_sigframe;
struct trapframe saved_trapframe;
int saved_valid = 0; 

static void
generichandler(struct sigframe rsf, struct trapframe rtf)
{
    if ((rsf.sf_sc.sc_ps & PSL_VM) && (rtf.tf_eflags & PSL_VM)) { 
	saved_sigframe = rsf;
	saved_trapframe = rtf;
	saved_valid = 1;
	if (handler[rsf.sf_signum])
	    (*handler[rsf.sf_signum])(&saved_sigframe, &saved_trapframe);
	saved_valid = 0;
	switch_vm86(&saved_sigframe, &saved_trapframe);
    } else if (handler[rsf.sf_signum]) {
	(*handler[rsf.sf_signum])(&rsf, &rtf);
    }
}

void
setsignal(int s, void (*h)(struct sigframe *, struct trapframe *))
{
    static int first = 1;

    if (first) {
#if defined(SA_DISABLE)
        struct sigaltstack sstack;

	sstack.ss_base = signal_stack;
	sstack.ss_size = sizeof signal_stack;
	sstack.ss_flags = 0;
        sigaltstack (&sstack, NULL);
#else
        struct sigstack sstack;

        sstack.ss_sp = signal_stack + sizeof signal_stack;
        sstack.ss_onstack = 0;
        sigstack (&sstack, NULL);
#endif
        first = 0;
    }

    if (s >= 0 && s < NSIG) {
	struct sigaction sa;
	handler[s] = h;

	sa.sa_handler = generichandler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGIO);
	sigaddset(&sa.sa_mask, SIGALRM);
	sa.sa_flags = SA_ONSTACK;
	sigaction(s, &sa, NULL);
    }
}

char *signame[] = {
    "SIGNONE",
    "SIGHUP",
    "SIGINT",
    "SIGQUIT",
    "SIGILL",
    "SIGTRAP",
    "SIGABRT",
    "SIGEMT",
    "SIGFPE",
    "SIGKILL",
    "SIGBUS",
    "SIGSEGV",
    "SIGSYS",
    "SIGPIPE",
    "SIGALRM",
    "SIGTERM",
    "SIGURG",
    "SIGSTOP",
    "SIGTSTP",
    "SIGCONT",
    "SIGCHLD",
    "SIGTTIN",
    "SIGTTOU",
    "SIGIO",
    "SIGXCPU",
    "SIGXFSZ",
    "SIGVTALRM",
    "SIGPROF",
    "SIGWINCH",
    "SIGINFO",
    "SIGUSR1",
    "SIGUSR2",
};

void
switch_vm86(struct sigframe *sf, struct trapframe *tf)
{
#if 0
    	sigset_t set;
	static sigset_t lset;
	sigset_t nset;
	int i;
        saved_valid = 0;

	sigprocmask(0, 0, &nset);

	for (i = 1; i < 32; ++i) {
	    if (sigismember(&nset, i) != sigismember(&lset, i))
		fprintf(debugf, "Signal %s %s being caught\n",
		    signame[i], sigismember(&nset, i) ? "now" : "no longer");
	}
	lset = nset;
#endif

	_switch_vm86(*sf, *tf);
}

void
_switch_vm86(struct sigframe sf, struct trapframe tf)
{
	while (dead)
		tty_pause();

        if ((sf.sf_sc.sc_ps & PSL_VM) == 0
            || (tf.tf_eflags & PSL_VM) == 0) {
                fatal ("illegal switch_vm86");
        }

        __asm__ volatile("addl $8, %esp");      /* drop garbage from stack */
        __asm__ volatile("movl $103,%eax");     /* sigreturn syscall number */
        /* enter kernel with args on stack */
        __asm__ volatile(".byte 0x9a; .long 0; .word 0x7");

        fatal ("sigreturn for v86 mode failed\n");
}

typedef struct {
    u_short	cs;
    u_short	ip;
} istack_t;

istack_t istack[16];
int ihead = 0;

int jumping = 0;

int
preserve_int(struct trapframe *tf)
{
    ihead = (ihead + 1) & 0xf;
    istack[ihead].cs = tf->tf_cs;
    istack[ihead].ip = tf->tf_ip;
    return(1);
}

void
spoil_int()
{
    if (istack[ihead].cs == 0 && istack[ihead].ip == 0) {
	printf("Spoiling with nothing to spoil\n");
	return;
    }

    istack[ihead].cs = 0;
    istack[ihead].ip = 0;

    if (--ihead < 0)
	ihead = 15;

    jmp_okay = -2;
}

void
intret()
{
    if (istack[ihead].cs == 0 && istack[ihead].ip == 0) {
	printf("Can't return back!\n");
	return;
    }

    saved_trapframe.tf_cs = istack[ihead].cs;
    saved_trapframe.tf_ip = istack[ihead].ip;

    istack[ihead].cs = 0;
    istack[ihead].ip = 0;

    if (--ihead < 0)
	ihead = 15;

    switch_vm86(&saved_sigframe, &saved_trapframe);
}

void
intjmp(int i)
{
    u_long vec = ivec[i];

    if (dead)
	return;

    if ((vec >> 16) == 0xf000 ||
	*(u_char *)((vec >> 12) + (vec & 0xffff)) == 0xcf) {
	return;
    }

    debug (D_TRAPS|i, "Int%x [%04x:%04x]\n",
		    i, ivec[i] >> 16, ivec[i] & 0xffff);

    PUSH(saved_trapframe.tf_eflags, &saved_trapframe);
    PUSH(0xf100, &saved_trapframe);
    PUSH(0x02, &saved_trapframe);
    saved_trapframe.tf_cs = vec >> 16;
    saved_trapframe.tf_ip = vec & 0xffff;

    jmp_okay = -3;
    jumping = 1;
    switch_vm86(&saved_sigframe, &saved_trapframe);
}

void
callint(int i)
{
    u_long vec = ivec[i];

    if (dead || !saved_valid || vec == 0)
	return;

    if ((vec >> 16) == 0xf000 ||
	*(u_char *)((vec >> 12) + (vec & 0xffff)) == 0xcf) {
	return;
    }

    debug (D_TRAPS|i, "Int%x [%04x:%04x]\n",
		    i, ivec[i] >> 16, ivec[i] & 0xffff);

    PUSH(saved_trapframe.tf_eflags, &saved_trapframe);
    PUSH(saved_trapframe.tf_cs, &saved_trapframe);
    PUSH(saved_trapframe.tf_ip, &saved_trapframe);
    saved_trapframe.tf_cs = vec >> 16;
    saved_trapframe.tf_ip = vec & 0xffff;

    switch_vm86(&saved_sigframe, &saved_trapframe);
}
