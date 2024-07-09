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
 *	BSDI int16.c,v 2.3 1998/01/29 16:51:53 donn Exp
 */

#include "doscmd.h"

#define	HWM	16
volatile int poll_cnt = HWM;

void
wakeup_poll(void)
{
    if (poll_cnt <= 0)
	poll_cnt = HWM;
}

void
reset_poll(void)
{
    poll_cnt = HWM;
}

void
sleep_poll(void)
{
    if (--poll_cnt <= 0) {
	poll_cnt = 0;
	while (KbdEmpty() && poll_cnt <= 0) {
#if 0
	    if (saved_valid && (IntState & PSL_I)) {
		    if ((ivec[0x28] >> 16) != 0xF000 && ivec[0x28]) {
			intjmp(0x28);
		    }
	    }
#endif
	    if (KbdEmpty() && poll_cnt <= 0)
		tty_pause();
	}
    }
}

void
int16 (tf)
struct trapframe *tf;
{               
	int c;
        struct byteregs *b = (struct byteregs *)&tf->tf_bx;

    	if (!xmode && !raw_kbd) {
		if (vflag) dump_regs(tf);
		fatal ("int16 func 0x%x only supported in X mode\n", b->tf_ah);
    	}
	switch( b->tf_ah) {
	case 0x00:
	case 0x10: /* Get enhanced keystroke */
		poll_cnt = 16;
		while (KbdEmpty())
		    tty_pause();
		c = KbdRead();
		b->tf_al = c & 0xff;
		b->tf_ah = (c >> 8) & 0xff;
		break;
	case 0x01: /* Get keystroke */
	case 0x11: /* Get enhanced keystroke */
    	    	if (!raw_kbd)
		    sleep_poll();

		if (KbdEmpty()) {
		    tf->tf_eflags |= PSL_Z;
		    break;
		}
		tf->tf_eflags &= ~PSL_Z;
		c = KbdPeek();
		b->tf_al = c & 0xff;
		b->tf_ah = (c >> 8) & 0xff;
		break;
	case 0x02:
		b->tf_al = tty_state();
		break;
	case 0x05:
		KbdWrite(tf->tf_cx);
		break;
    	case 0x12:
    	    	b->tf_al = tty_state();
    	    	b->tf_ah = tty_estate();
		break;
    	case 0x03:	/* Set typematic and delay rate */
		break;
	case 0x55:
		tf->tf_ax = 0x43af;	/* Empirical value ... */
		break;
	default:
		unknown_int2(0x16, b->tf_ah, tf);
		break;
	}
}
