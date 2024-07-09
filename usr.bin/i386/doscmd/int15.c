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
 *	BSDI int15.c,v 2.3 1998/01/29 16:51:53 donn Exp
 */

#include "doscmd.h"

void
int15(tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	int cond;
	int count;

        tf->tf_eflags &= ~PSL_C;

	switch (b->tf_ah) {
	case 0x00:	/* Get Cassette Status */
		b->tf_ah = 0x86;
		tf->tf_eflags |= PSL_C;	/* We don't support a cassette */
		break;
	case 0x04:	/* Set ABIOS table */
		tf->tf_eflags |= PSL_C;	/* We don't support it */
		break;
	case 0x41:
		if (b->tf_al & 0x10) {
			debug (D_HALF, "WAIT on port %d", tf->tf_dx);
		} else {
			debug (D_HALF, "WAIT");
		}
		cond = b->tf_al;
		count = b->tf_bl;
		for (;;) {
		    struct timeval tv;
		    if (cond & 0x10) {
			inb(tf, tf->tf_dx);	/* XXX ??? XXX */
		    } else {
			b->tf_al = *(u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
		    }
		    switch (cond & 7) {
/* next line as per prb recommendation mcl 01/01/94 */
/*                  case 0: goto out; break; */
                    case 0:
                        if (tf->tf_dx == 0x82) {
                                goto out; break;
                        }
                        fprintf(stderr, "waiting for external event\n");
                        fprintf(stderr,
                                "ax %04x, bx %04x, dx %04x, es:di %04x:%04x\n",
                                tf->tf_ax, tf->tf_bx, tf->tf_dx, tf->tf_es,
                                tf->tf_di);
                        break;
		    case 1: if (b->tf_al == b->tf_bh) goto out; break;
		    case 2: if (b->tf_al != b->tf_bh) goto out; break;
		    case 3: if (b->tf_al != 0) goto out; break;
		    case 4: if (b->tf_al == 0) goto out; break;
		    }
		    
		    tv.tv_sec = 0;
		    tv.tv_usec = 10000000 / 182;
		    select(0, 0, 0, 0, &tv);
		    if (count && --count == 0)
			break;
		}
	    out:
		b->tf_al = cond;
		break;
    	case 0x4f:
		/*
		 * XXX - Check scan code in b->tf_al.
		 */
		break;
	case 0x88:
		tf->tf_ax = 0;	/* memory past 1M */
		break;
	case 0xc0:	/* get configuration */
		debug (D_TRAPS|0x15, "Get configuration", tf->tf_dx);
    	    	tf->tf_es = rom_config >> 16;
    	    	tf->tf_bx = rom_config & 0xffff;
		break;
    	case 0xc1:	/* get extended BIOS data area */
                tf->tf_eflags |= PSL_C;
		break;
	case 0xc2:	/* Pointing device */
                tf->tf_eflags |= PSL_C;
		b->tf_ah = 5;	/* No pointer */
		break;
	default:
		unknown_int2(0x15, b->tf_ah, tf);
		break;
	}
}
