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
 *	BSDI console.c,v 2.2 1996/04/08 19:31:24 bostic Exp
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <termios.h>
#include <i386/isa/pcconsioctl.h>
#include "kernel.h"

static void do_console_redirect(void);

static int tty_fd, pty_fd;
static struct termios tty_cook, tty_raw;
extern FILE *logfile;
extern struct connect_area *connect_area;

void redirect_console(void)
{
  if (!connect_area->remote_terminal) {
    int on = 1;
    static char ttydev[64], ptydev[64];
    char *ptychars = "0123456789abcdef", *ptychar;

    strcpy (ttydev, "/dev/ttypx");
    strcpy (ptydev, "/dev/ptypx");

    for (ptychar = ptychars; *ptychar; ptychar++) {
      ttydev [strlen(ttydev) - 1] = ptydev [strlen(ptydev) - 1] = *ptychar;
      if ((pty_fd = open (ptydev, 2)) >= 0 && (tty_fd = open (ttydev, 2)) >= 0)
	break;
      else if (pty_fd >= 0)
	close (pty_fd);
    }
    if (pty_fd < 0) {
      fprintf(stderr, "no pty found\n");
      exit(1);
    }
    seteuid (0);
    if (ioctl (tty_fd, TIOCCONS, (char *) &on) < 0) {
      seteuid (getuid ());
      perror("TIOCCONS");
      exit(1);
    }
    seteuid (getuid ());
    fcntl(pty_fd, F_SETFL, O_NDELAY);
    add_input_handler(pty_fd, do_console_redirect);
  }
}

void normal_console(void)
{
  if (!connect_area->remote_terminal) {
    int off = 0;

    seteuid (0);
    ioctl (tty_fd, TIOCCONS, (char *) &off);
    seteuid (getuid ());
    delete_input_handler(pty_fd);
    close(pty_fd);
    close(tty_fd);
  }
}

void select_raw_keyboard(void)
{
  if (!connect_area->remote_terminal)
    ioctl(0, PCCONIOCRAW, 0);		/* turn kbd raw mode on */
  fcntl(0, F_SETFL, O_NDELAY|O_ASYNC);
}

void select_cook_keyboard(void)
{
  int mode = 0;		/* read|write */

  fcntl(0, F_SETFL, 0);
  if (!connect_area->remote_terminal) {
    ioctl(0, PCCONIOCCOOK, 0);		/* turn kbd raw mode off */
    ioctl(0, TIOCFLUSH, &mode);
  }
}

void select_raw_console()
{
  ioctl(0, TIOCSETA, &tty_raw);
}

void select_cook_console(void)
{
  ioctl(0, TIOCSETA, &tty_cook);
}

void init_console(void)
{
  redirect_console();
  ioctl(0, TIOCGETA, &tty_cook);
  tty_raw = tty_cook;
  cfmakeraw(&tty_raw);
  select_raw_console();
  select_raw_keyboard();
/*
  if (!connect_area->remote_terminal)
    vga_save_initial_mode();
*/
}

void reset_console(void)
{
  if (!connect_area->remote_terminal)
    vga_restore_initial_mode();
  select_cook_console();
  select_cook_keyboard();
  normal_console();
}


#define BUFSIZE 1024

static void do_console_redirect(void)
{
  unsigned char buffer[BUFSIZE];
  int k;

  if (connect_area->remote_terminal)
    return;
  do {
    k = read(pty_fd, buffer, BUFSIZE);
    if (k > 0 && logfile)
      fwrite(buffer, k, 1, logfile);
  } while (k == BUFSIZE);
}
