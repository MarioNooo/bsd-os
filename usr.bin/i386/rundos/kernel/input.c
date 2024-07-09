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
 *	BSDI input.c,v 2.2 1996/04/08 19:31:27 bostic Exp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "kernel.h"
#include "queue.h"

struct ihandler {
  int fd;
  void (*input_function)(void);
  struct ihandler *next;
};

static struct ihandler *ihandlers = NULL;
static fd_set select_mask;
static int select_count;
static int input_busy = 0;

static void compute_select_count(void)
{
  struct ihandler *ih;

  FD_ZERO(&select_mask);
  select_count = 0;

  for (ih = ihandlers; ih != NULL; ih = ih->next) {
    FD_SET(ih->fd, &select_mask);
    if (select_count <= ih->fd)
      select_count = ih->fd + 1;
  }
}

void add_input_handler(int fd, void (*input_function)(void))
{
  struct ihandler *ih;
  void *malloc();

  input_busy++;

  if ((ih = (struct ihandler *)malloc(sizeof(struct ihandler))) == NULL)
    ErrExit (0x07, "Out of memory\n");

  ih->fd = fd;
  ih->input_function = input_function;
  ih->next = ihandlers;
  ihandlers = ih;
  
  compute_select_count();
  input_busy--;
}

void delete_input_handler(int fd)
{
  struct ihandler *ih, *temp_ih;

  input_busy++;
  if (select_count == 0)
    ErrExit (100, "No handlers to delete\n");
  if (ihandlers->fd == fd) {
    temp_ih = ihandlers;
    ihandlers = ihandlers->next;
    free(temp_ih);
  } else {
    for (ih = ihandlers; ih->next != NULL; ih = ih->next) {
      if (ih->next->fd == fd) {
	temp_ih = ih->next;
	ih->next = ih->next->next;
	free(temp_ih);
	break;
      }
    }
  }
  compute_select_count();
  input_busy--;
}

void do_inputs(void)
{
  struct ihandler *ih;
  struct timeval tv;
  fd_set files;

  if (select_count == 0 || input_busy)
    return;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  files = select_mask;
  select(select_count, &files, NULL, NULL, &tv);

  for (ih = ihandlers; ih != NULL; ih = ih->next)
    if (FD_ISSET(ih->fd, &files))
      ih->input_function();
}
