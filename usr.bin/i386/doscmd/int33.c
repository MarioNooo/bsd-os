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
 *	BSDI int33.c,v 2.3 1998/01/29 16:51:55 donn Exp
 */

#include "doscmd.h"
#include "mouse.h"

mouse_t mouse_status;
u_char *mouse_area = 0;
int nmice = 0;

void mouse_probe() { }

void
mouse_init()
{
	mouse_area[1] = 24;
}

void
int33 (tf)
struct trapframe *tf;
{
    	u_long vec;
	u_short mask;
	int i;

	if (!nmice) {
		tf->tf_eflags |= PSL_C;	/* We don't support a mouse */
		return;
	}

printf("Mouse: %02x\n", tf->tf_ax);
	switch (tf->tf_ax) {
	case 0x00:	/* Reset Mouse */
printf("Installing mouse driver\n");
		tf->tf_ax = 0xffff;	/* Mouse installed */
		tf->tf_bx = 2;		/* Number of mouse buttons */
    	    	memset(&mouse_status, 0, sizeof(mouse_status));
		mouse_status.installed = 1;
		mouse_status.hardcursor = 1;
		mouse_status.end = 16;
		mouse_status.hmickey = 8;
		mouse_status.vmickey = 16;
		mouse_status.doubling = 100;
		mouse_status.init = -1;
		mouse_status.range.w = 8 * 80;
		mouse_status.range.h = 16 * 25;
		break;
	case 0x01:	/* Display Mouse Cursor */
		if ((mouse_status.init += 1) == 0) {
		    mouse_status.show = 1;
		}
		break;
	case 0x02:	/* Hide Mouse Cursor */
		if (mouse_status.init == 0)
		    mouse_status.show = 0;
		mouse_status.init -= 1;
		break;
	case 0x03:	/* Get cursor position/button status */
		mouse_probe();
		tf->tf_cx = mouse_status.x;
		tf->tf_dx = mouse_status.y;
		tf->tf_bx = mouse_status.buttons;
		break;
	case 0x04:	/* Move mouse cursor */
		/* mouse_move(tf->tf_cx, tf->tf_dx); */
		break;
	case 0x05:	/* Determine number of times mouse button was active */
		if ((tf->tf_bx &= 0x3) == 3)
		    tf->tf_bx = 1;

		tf->tf_bx = mouse_status.downs[tf->tf_bx];
		mouse_status.downs[tf->tf_bx] = 0;
		tf->tf_ax = mouse_status.buttons;
		tf->tf_cx = mouse_status.x;	/* Not quite right */
		tf->tf_dx = mouse_status.y;	/* Not quite right */
		break;
	case 0x06:	/* Determine number of times mouse button was relsd */
		if ((tf->tf_bx &= 0x3) == 3)
		    tf->tf_bx = 1;

		tf->tf_bx = mouse_status.ups[tf->tf_bx];
		mouse_status.ups[tf->tf_bx] = 0;
		tf->tf_ax = mouse_status.buttons;
		tf->tf_cx = mouse_status.x;	/* Not quite right */
		tf->tf_dx = mouse_status.y;	/* Not quite right */
		break;
	case 0x07:	/* Set min/max horizontal cursor position */
		mouse_status.range.x = tf->tf_cx;
		mouse_status.range.w = tf->tf_dx - tf->tf_cx;
		break;
	case 0x08:	/* Set min/max vertical cursor position */
		mouse_status.range.y = tf->tf_cx;
		mouse_status.range.h = tf->tf_dx - tf->tf_cx;
	case 0x09:	/* Set graphics cursor block */
		/* BX,CX is hot spot, ES:DX is data. */
		break;
	case 0x0a:	/* Set Text Cursor */
		mouse_status.hardcursor = tf->tf_bx ? 1 : 0;
		mouse_status.start = tf->tf_cx;
		mouse_status.end = tf->tf_cx;
		break;
	case 0x0b:	/* Read Mouse Motion Counters */
		mouse_probe();
		tf->tf_cx = mouse_status.x - mouse_status.lastx;
		tf->tf_dx = mouse_status.y - mouse_status.lasty;
		mouse_status.lastx = mouse_status.x;
		mouse_status.lasty = mouse_status.y;
		break;
	case 0x0c:	/* Set event handler */
		mouse_status.mask = tf->tf_cx;
		mouse_status.handler = (((u_long)tf->tf_es)<<16) | tf->tf_dx;
		break;
	case 0x0d:	/* Enable light pen */
	case 0x0e:	/* Disable light pen */
		break;
	case 0x0f:	/* Set cursor speed */
		mouse_status.hmickey = tf->tf_cx;
		mouse_status.vmickey = tf->tf_dx;
		break;
	case 0x10:	/* Exclusive area */
		mouse_status.exclude.x = tf->tf_cx;
		mouse_status.exclude.y = tf->tf_dx;
		mouse_status.exclude.w = tf->tf_si - tf->tf_cx;
		mouse_status.exclude.h = tf->tf_di - tf->tf_dx;
		break;
	case 0x13:	/* Set maximum for mouse speed doubling */
		break;
	case 0x14:	/* Exchange event handlers */
		vec = mouse_status.handler;
		mask = mouse_status.mask;

		mouse_status.handler = (((u_long)tf->tf_es)<<16) | tf->tf_dx;
		mouse_status.mask = tf->tf_cx;
    	    	tf->tf_cx = mask;
    	    	tf->tf_es = vec >> 16;
    	    	tf->tf_dx = vec & 0xffff;
		break;
    	case 0x15:	/* Determine mouse status buffer size */
		tf->tf_bx = sizeof(mouse_status);
		break;
	case 0x16:	/* Store mouse buffer */
		memcpy(MAKE_ADDR(tf->tf_es, tf->tf_dx), &mouse_status,
		       sizeof(mouse_status));
		break;
	case 0x17:	/* Restore mouse buffer */
		memcpy(&mouse_status, MAKE_ADDR(tf->tf_es, tf->tf_dx),
		       sizeof(mouse_status));
		break;
	case 0x18:	/* Install alternate handler */
		mask = tf->tf_cx & 0xff;
		if ((tf->tf_cx & 0xe0) == 0x00 ||
		    mask == mouse_status.altmask[0] ||
		    mask == mouse_status.altmask[1] ||
		    mask == mouse_status.altmask[2] ||
		    (mouse_status.altmask[i = 0] &&
		     mouse_status.altmask[i = 1] &&
		     mouse_status.altmask[i = 2])) {
			tf->tf_ax = 0xffff;
			break;
		}
    	    	mouse_status.altmask[i] = tf->tf_cx;
    	    	mouse_status.althandler[i] = (((u_long)tf->tf_es)<<16) |
						tf->tf_dx;
		break;
	case 0x19:	/* Determine address of alternate event handler */
		mask = tf->tf_cx & 0xff;
    	    	if (mask == mouse_status.altmask[0])
			vec = mouse_status.althandler[0];
    	    	else if (mask == mouse_status.altmask[1])
			vec = mouse_status.althandler[1];
    	    	else if (mask == mouse_status.altmask[2])
			vec = mouse_status.althandler[2];
		else {
			vec = 0;
			tf->tf_cx = 0;
		}
		tf->tf_es = vec >> 16;
		tf->tf_dx = vec & 0xffff;
		break;
	case 0x1a:	/* set mouse sensitivity */
		mouse_status.hmickey = tf->tf_bx;
		mouse_status.vmickey = tf->tf_cx;
		mouse_status.doubling = tf->tf_dx;
		break;
	case 0x1b:	/* set mouse sensitivity */
		tf->tf_bx = mouse_status.hmickey;
		tf->tf_cx = mouse_status.vmickey;
		tf->tf_dx = mouse_status.doubling;
		break;
    	case 0x1c:	/* set mouse hardware rate */
		break;
	case 0x1d:	/* set display page */
		break;
	case 0x1e:	/* get display page */
		tf->tf_bx = 0;	/* Always on display page 0 */
		break;
	case 0x1f:	/* Disable mouse driver */
		if (mouse_status.installed) {
		    tf->tf_es = mouse_status.handler >> 16;
		    tf->tf_dx = mouse_status.handler & 0xffff;
		    mouse_status.installed = 0;
    	    	} else {
		    	tf->tf_ax = 0xffff;
		}
		break;
	case 0x20:	/* Enable mouse driver */
		mouse_status.installed = 1;
		break;
	case 0x21:	/* Reset mouse driver */
		if (mouse_status.installed) {
			mouse_status.show = 0;
			mouse_status.handler = 0;
			mouse_status.mask = 0;
			mouse_status.cursor = 0;
		} else
		    	tf->tf_ax = 0xffff;
		break;
	case 0x22:	/* Specified language for mouse messages */
		break;
	case 0x23:	/* Get language number */
		tf->tf_bx = 0;	/* Always return english */
		break;
	case 0x24:	/* Get mouse type */
		tf->tf_cx = 0x0400;		/* PS/2 style mouse */
		tf->tf_bx = 0x0600 + 24;	/* Version 6.24 */
		break;
	default:
		tf->tf_eflags |= PSL_C;
		break;
	}
}
