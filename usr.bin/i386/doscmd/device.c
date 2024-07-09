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
 *	BSDI device.c,v 2.3 1998/01/29 16:51:48 donn Exp
 */

#include "doscmd.h"

#define	MAXDEVICE	0x3F

/*
 * configuration table for device initialization routines
 */

struct devinitsw {
	void		(*p_devinit)();
} devinitsw[MAXDEVICE + 1];

static int devinitsw_ptr;

void null_devinit_handler(void) { }

void init_devinit_handlers(void)
{
	int i;

	for (i = 0; i <= MAXDEVICE; i++)
		devinitsw[i].p_devinit = null_devinit_handler;
	devinitsw_ptr = 0;
}

void define_devinit_handler(void (*p_devinit)())
{
	if (devinitsw_ptr < MAXDEVICE)
		devinitsw[devinitsw_ptr++].p_devinit = p_devinit;
	else
		fprintf (stderr, "attempt to install more than %d devices",
			MAXDEVICE);
}

void
init_optional_devices(void)
{
	int i;

	if (devinitsw_ptr > 0)
		for (i = 0; i < devinitsw_ptr; i++)
			(*devinitsw[i].p_devinit)();
}
