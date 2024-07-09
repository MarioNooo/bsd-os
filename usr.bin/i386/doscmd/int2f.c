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
 *	BSDI int2f.c,v 2.3 1998/01/29 16:51:55 donn Exp
 */

#include "doscmd.h"

int int2f_11(struct trapframe *);

void
int2f(struct trapframe *tf)
{               
        struct byteregs *b = (struct byteregs *)&tf->tf_bx;

	switch(b->tf_ah) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
		debug (D_FILE_OPS, "Called printer function 0x%02x", b->tf_ah);
		tf->tf_eflags |= PSL_C;
		b->tf_al = FUNC_NUM_IVALID;
		break;
	case 0x12:
		switch (b->tf_al) {
		case 0x2e:
		    /* XXX - GET/SET ERROR TABLE ADDRESSES */
		    debug (D_HALF, "GET/SET ERROR TABLE %d\n", b->tf_dl);
		    tf->tf_eflags |= PSL_C;
		    break;
		default:
		    unknown_int4(0x2f, 0x12, b->tf_al, b->tf_dl, tf);
		    break;
    	    	}
		break;
	case 0x43:
		switch (b->tf_al) {
		case 0x00:
			debug (D_HALF, "Get XMS status\n");
			b->tf_al = 0;
			break;
		default:
			unknown_int3(0x2f, 0x43, b->tf_al, tf);
			break;
		}
		break;
	case 0xb7:
		switch (b->tf_al) {
		case 0x00:
			debug (D_HALF, "Get APPEND status\n");
			b->tf_al = 0;
			break;
		case 0x02:
			debug (D_HALF, "Verify DOS 5.0 APPEND compatibility\n");
			tf->tf_ax = 0;
			break;
		case 0x04:
			debug (D_HALF, "Get APPEND path\n");
			tf->tf_es = 0;
			tf->tf_di = 0;
			break;
		case 0x06:
			debug (D_HALF, "Deterimine APPEND mode\n");
			tf->tf_bx = 0;
			break;
		case 0x07:
			debug (D_HALF, "Set APPEND mode to %04x\n", tf->tf_bx);
			break;
		default:
			if (vflag) dump_regs(tf);
			fatal ("unknown int2f:b7 func 0x%x\n", b->tf_al);
			break;
		}
		break;
    	case 0xae:
		b->tf_al = 0;
		tf->tf_eflags &= ~PSL_C;
		break;
    	case 0x48:	/* Read command line */
#if 0
		tf->tf_ax = 0x7b5;	/* I don't know */
#endif
		tf->tf_eflags &= ~PSL_C;
		break;
    	case 0x55:
		printf("Interrupt 2f:%02x\n", b->tf_ah);
		printf("AX: %04x BX: %04x CX: %04x DX: %04x\n",
			tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx);
		printf("SI: %04x DI: %04x BP: %04x SP: %04x\n",
			tf->tf_si, tf->tf_di, tf->tf_bp, tf->tf_sp);
		printf("CS: %04x DS: %04x SS: %04x ES: %04x\n",
			tf->tf_cs, tf->tf_ds, tf->tf_ss, tf->tf_es);
		break;
	case 0x44:
		tf->tf_eflags |= PSL_C;
		break;
    	case 0x11:
    	case 0x16:
	default:
		unknown_int2(0x2f, b->tf_ah, tf);
	}
}
