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
 *	BSDI int11.c,v 2.2 1996/04/08 19:32:41 bostic Exp
 */

#include "doscmd.h"

void
int11 (tf)
struct trapframe *tf;
{
	/*
	 * If we are an XT
	 */
	tf->tf_ax = 0x0001		/* has a disk */
		  | 0x000c		/* 64K on main board */
		  | 0x0020		/* 80x25 color card */
		  | 0x0080		/* 3 disks */
		  | (0x0 << 9)		/* number rs232 ports */
		  | (0x0 << 14);	/* number of printers */
	/*
	 * If we are an AT
	 */
	tf->tf_ax = nfloppies ? 0x0001 : 0x0000		/* has a disk */
		  | (nmice << 1)	/* No mouse */
		  | (0x3 << 2)		/* at least 64 K on board */
		  | (0x2 << 4)		/* 80x25 color card */
		  | ((nfloppies - 1) <<	6)	/* has 1 disk */
		  | (0x0 << 8)		/* No DMA */
		  | (nserial << 9)	/* number rs232 ports */
		  | (0x0 << 12)		/* No game card */
		  | (0x0 << 13)		/* No serial printer/internal modem */
		  | (nparallel << 14);	/* number of printers */
}
