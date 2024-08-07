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
 *	BSDI rundos.c,v 2.2 1996/04/08 19:32:15 bostic Exp
 */

#include <stdio.h>
#include <a.out.h>
#include "paths.h"

int load_kernel(void)
{
  FILE *fp;
  struct exec exec;
  int start_address;

  if ((fp = fopen(KERNEL_FILE_NAME, "r")) == NULL) {
    perror("load_kernel");
    exit(1);
  }
  if (fread(&exec, sizeof(exec), 1, fp) != 1 || exec.a_magic != OMAGIC) {
    fprintf(stderr, "bad kernel file format\n");
    exit(1);
  }
  start_address = exec.a_entry & (~(getpagesize() - 1));
  if (brk(start_address + exec.a_text + exec.a_data + exec.a_bss) < 0) {
    perror("load_kernel");
    exit(1);
  }
  fread((char *)start_address, exec.a_text + exec.a_data, 1, fp);
  bzero((char *)(start_address + exec.a_text + exec.a_data), exec.a_bss);
  fclose(fp);
  return(exec.a_entry);
}

void main(int argc, char **argv, char **environ)
{
  void (*entry_point)() = (void (*)()) load_kernel();

  (*entry_point)(argc, argv, environ);
  fprintf(stderr, "return from kernel???\n");
  exit(1);
}
