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
 *	BSDI debug.c,v 2.2 1996/04/08 19:31:12 bostic Exp
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "config.rundos"
#endif

static struct termios tios;
static int fd;

void init_debug(char num)
{
  char tty[64];

  strcpy(tty, "/dev/ptypx");

  tty[strlen(tty) - 1] = num;

  if ((fd = open(tty, O_RDWR)) < 0) {
    fprintf(stderr, "Cannot open debug terminal %s\n", tty);
    exit(1);
  }
  ioctl(fd, TIOCGETA, &tios);
  cfmakeraw(&tios);
  ioctl(fd, TIOCSETA, &tios);
}

void syntax_error(int line)
{
  fprintf(stderr, "Syntax error in config file (line %d)\n", line);
  exit(1);
}

main()
{
  static fd_set selected_files;
  fd_set files;
  char buffer[BUFSIZ];
  int k, line = 0;
  FILE *fp;
  int num = 0;

  if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
    fprintf(stderr, "can't open configuration file\n");
    exit(1);
  }
  while (fgets(buffer, BUFSIZ, fp)) {
    line++;
    if (strncmp(buffer, "trace", 5) == 0) {
      if (sscanf(buffer, "%*s %c", &num) != 1)
	syntax_error(line);
      fprintf(stderr, "debug terminal = /dev/ptyp%c\n", num);
      init_debug(num);
      break;
    }
  }

  if (!num) {
    fprintf(stderr, "Trace terminal not found in config\n");
    exit(1);
  }

  FD_ZERO(&selected_files);
  FD_SET(0, &selected_files);
  FD_SET(fd, &selected_files);

  while (1) {

    files = selected_files;
    select(fd + 1, &files, NULL, NULL, NULL);
    
    if (FD_ISSET(fd, &files))
      do {
	k = read(fd, buffer, BUFSIZ);
	write(1, buffer, k);
      } while (k == BUFSIZ);
    if (FD_ISSET(0, &files)) 
      do {
	k = read(0, buffer, BUFSIZ);
	write(fd, buffer, k);
      } while (k == BUFSIZ);
  }
}

