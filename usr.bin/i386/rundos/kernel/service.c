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
 *	BSDI service.c,v 2.2 1996/04/08 19:31:42 bostic Exp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <i386/isa/pcconsioctl.h>
#include "kernel.h"

/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
extern int v86_proc;
int monitor_initialized = 0;
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/

void init_services(void)
{
}

void system_service(struct trapframe *tf)
{
  switch (tf->tf_ax & 0xff) {
  case 0x00:       /* then goes to case 0x13 !!!!!!! */
    init_disk_service();
  case 0x13:
    (void)v86_do_int13();
    break;
  case 0x10:       /* temp.: remote terminal service in future */
#ifdef DEBUG
    ErrorTo86 ("Obsolete CGA video call");
#endif
    break;
  case 0x17:
    PrinterService ((u_char)(tf->tf_ax >> 8), tf->tf_bx);
    break;
  case 0x18:
    ErrorTo86 ("ERROR: ROM BASIC not present and boot fails");
    break;
  case 0x40:
    ioctl (0, PCCONIOCBEEP);
    break;
  case 0x64:
    xms_service (tf);
    break;
/*********************************************************/
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
  case 0x6D:
  case 0xF0:
    v86_proc = 3;
    break;
  case 0xF1:
    if (! monitor_initialized) {
      InitVideo(tf);
      monitor_initialized = 1;
    }
    break;
/* !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! !!!!! */
/*********************************************************/
  default:
    ErrExit (0x09, "Exit caused by DOS' program\n");
  }
}
