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
 *	BSDI int1a.c,v 2.3 2002/03/15 21:12:19 dab Exp
 */

#include "doscmd.h"

void
int1a(tf)
struct trapframe *tf;
{
	struct	timeval	tod;
	struct	timezone zone;
	struct	tm *tm;
	long	value, nvalue;
	static long midnight = 0;
	static long clockcount_delta = 0;
	static long tod_delta = 0;

	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
        tf->tf_eflags &= ~PSL_C;

	switch (b->tf_ah) {
	case 0x00:
	case 0x01:
		gettimeofday(&tod, &zone);

		if (midnight == 0) {
		    tm = localtime(&boot_time.tv_sec);
		    midnight = boot_time.tv_sec - (((tm->tm_hour * 60)
					           + tm->tm_min) * 60
						   + tm->tm_sec);
		}

		b->tf_al = (tod.tv_sec - midnight) / (24 * 60 * 60);

		if (b->tf_al) {
		    tm = localtime(&boot_time.tv_sec);
		    midnight = boot_time.tv_sec - (((tm->tm_hour * 60)
					           + tm->tm_min) * 60
						   + tm->tm_sec);
		}

		tod.tv_sec -= midnight;
		tod.tv_usec -= boot_time.tv_usec;

		value = (tod.tv_sec * 182 + tod.tv_usec / (1000000L/182)) / 10;
		value += clockcount_delta;
		if (b->tf_ah == 0x00) {
			tf->tf_cx = value >> 16;
			tf->tf_dx = value;
		} else {
			nvalue = (tf->tf_cx << 16) + tf->tf_dx;
			clockcount_delta += nvalue - value;
		}
		break;

	case 0x02:
		gettimeofday(&tod, &zone);
		tod.tv_sec += tod_delta;
		tm = localtime(&tod.tv_sec);
		b->tf_ch = tm->tm_hour;
		b->tf_cl = tm->tm_min;
		b->tf_dh = tm->tm_sec;
		break;

	case 0x03:
		gettimeofday(&tod, &zone);
		tod.tv_sec += tod_delta;
		tm = localtime(&tod.tv_sec);
		value = (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec;
		nvalue = (b->tf_ch * 60 + b->tf_cl) * 60 + b->tf_dh;
		tod_delta += nvalue - value;
		break;

	case 0x04:
		gettimeofday(&tod, &zone);
		tm = localtime(&tod.tv_sec);
		b->tf_ch = (1900 + tm->tm_year) / 100;
		b->tf_cl = tm->tm_year % 100;
		b->tf_dh = tm->tm_mon;
		b->tf_dl = tm->tm_mday;
		break;

	default:
		unknown_int2(0x1a, b->tf_ah, tf);
		break;
	}
}
