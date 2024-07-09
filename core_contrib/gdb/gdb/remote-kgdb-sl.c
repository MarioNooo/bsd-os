/* SLIP-style 'kgdb' protocol for remote BSD/OS kernels.
   Copyright (C) 1988, 1989, 1991 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * Copyright (c) 1990, 1991, 1992 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char rcsid[] =
    "@(#) /master/usr.bin/gdb/gdb/config/remote-sl.c,v 2.1 1995/02/03 12:01:16 polk Exp (LBL)";
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "defs.h"
#include "command.h"
#include "bsdi-kernel.h"
#include "remote-kgdb-sl.h"
#include "remote-kgdb.h"

static int rib_filbuf (int);
static void set_sl_baudrate_command (char *, int);
static void sl_close (void);
static void sl_conf (int);
static void sl_info (char *, int);
static int sl_recv (int *, unsigned char *, int *, int);
static int sl_send (unsigned char, unsigned char *, int);

/* Descriptor for I/O to remote machine.  */
static int sl_fd = -1;

/* User-configurable baud rate of serial line.  */
static int speed = 9600;

/* Command line argument.  */
extern int baud_rate;

/* Statistics.  */
static int sl_errs;
static int sl_inbytes;
static int sl_outbytes;;

static void
sl_close ()
{
  if (sl_fd >= 0)
    {
      close (sl_fd);
      sl_fd = -1;
    }
}

/* Configure the serial link.  */
static void
sl_conf (int fd)
{
  struct termios tios;

  tcgetattr (fd, &tios);
  cfmakeraw (&tios);
  cfsetspeed (&tios, speed);
  tios.c_cflag |= CLOCAL;
  tcsetattr (fd, TCSANOW, &tios);
}

/* Open a serial line for remote debugging.  */
void
kgdb_sl_open (char *name, struct remote_kgdb_fn *remote_fnp)
{
  char device[80];

  sl_close ();

  if (name[0] != '/')
    {
      snprintf (device, sizeof device, "/dev/%s", name);
      name = device;
    }

  /* Use non-blocking mode so we don't wait for a carrier.
     This allows the open to complete, then we set CLOCAL
     mode since we don't need the modem control lines.  */
  sl_fd = open (name, O_RDWR|O_NONBLOCK);
  if (sl_fd < 0)
    {
      perror_with_name (name);
      /* NOTREACHED */
    }

  if (baud_rate != -1)
    {
      speed = baud_rate;
      baud_rate = -1;
    }
  sl_conf (sl_fd);

  remote_fnp->send = sl_send;
  remote_fnp->recv = sl_recv;
  remote_fnp->close = sl_close;
  remote_fnp->maxdata = SL_MAXDATA;
  remote_fnp->rpcsize = SL_RPCSIZE;
  
  sl_errs = 0;
  sl_inbytes = 0;
  sl_outbytes = 0;
}

/* Remote input buffering.  */
static unsigned char rib[2 * SL_MTU];
static unsigned char *rib_cp, *rib_ep;

#define GETC(to)	((rib_cp < rib_ep) ? *rib_cp++ : rib_filbuf (to))

/* Fill up the input buffer (with a non-blocking read).
   On error, return the negation of the error code, otherwise
   return the first character read and set things up so GETC will
   read the remaining chars.  */
static int
rib_filbuf (int to)
{
  int cc, fd = sl_fd;
  fd_set fds;
  struct timeval timeout;

  if (to >= 0)
    {
      timeout.tv_sec = to / 1000;
      timeout.tv_usec = to % 1000;
    }

  FD_ZERO (&fds);
  FD_SET (fd, &fds);

  cc = select (fd + 1, &fds, NULL, NULL, to >= 0 ? &timeout : NULL);

  if (cc == 0)
    return -EKGDB_TIMEOUT;

  if (cc < 0)
    return -EKGDB_IO;

  cc = read (fd, rib, sizeof rib);
  if (cc < 0)
    return -EKGDB_IO;

  rib_cp = &rib[1];
  rib_ep = &rib[cc];

  sl_inbytes += cc;

  return rib[0];
}

#define PUTESC(p, c) \
do \
  { \
    if (c == FRAME_END) \
      { \
	*p++ = FRAME_ESCAPE; \
	c = TRANS_FRAME_END; \
      } \
    else if (c == FRAME_ESCAPE) \
      { \
	*p++ = FRAME_ESCAPE; \
	c = TRANS_FRAME_ESCAPE; \
      } \
    else if (c == FRAME_START) \
      { \
	*p++ = FRAME_ESCAPE; \
	c = TRANS_FRAME_START; \
      } \
    *p++ = c; \
  } \
while (0)

/* Send a message to the remote host.  An error code is returned.  */
static int
sl_send (unsigned char type, unsigned char *bp, int len)
{
  unsigned char *p, *ep;
  unsigned char csum, c;
  unsigned char buf[SL_MTU];

  /* Build a packet.  The framing byte comes first, then the command
     byte, the message, the checksum, and another framing character.
     We must escape any bytes that match the framing or escape chars.  */
  p = buf;
  *p++ = FRAME_START;
  csum = type;
  PUTESC (p, type);

  for (ep = bp + len; bp < ep; )
    {
      c = *bp++;
      csum += c;
      PUTESC (p, c);
    }
  csum = -csum;
  PUTESC (p, csum);
  *p++ = FRAME_END;

  len = p - buf;
  sl_outbytes += len;
  if (write (sl_fd, buf, len) != len)
    return EKGDB_IO;
  return 0;
}

/* Read a packet from the remote machine.  An error code is returned.  */
static int
sl_recv (int *tp, unsigned char *ip, int *lenp, int to)
{
  unsigned char csum, *bp;
  int c;
  int escape, len;
  int type;
  unsigned char buf[SL_RPCSIZE + 1];	/* room for checksum at end of buffer */

  /* Allow immediate quit while reading from device, it could be hung.  */
  ++immediate_quit;

  /* Throw away garbage characters until we see the start
     of a frame (i.e., don't let framing errors propagate up).
     If we don't do this, we can get pretty confused.  */
  while ((c = GETC (to)) != FRAME_START)
    if (c < 0)
      return -c;

restart:
  csum = len = escape = 0;
  type = -1;
  bp = buf;
  for (;;)
    {
      c = GETC (to);
      if (c < 0)
	return -c;

      switch (c)
	{
	case FRAME_ESCAPE:
	  escape = 1;
	  continue;
	      
	case TRANS_FRAME_ESCAPE:
	  if (escape)
	    c = FRAME_ESCAPE;
	  break;
	      
	case TRANS_FRAME_END:
	  if (escape)
	    c = FRAME_END;
	  break;

	case TRANS_FRAME_START:
	  if (escape)
	    c = FRAME_START;
	  break;

	case FRAME_START:
	  goto restart;
	      
	case FRAME_END:
	  if (type < 0 || --len < 0)
	    {
	      csum = len = escape = 0;
	      continue;
	    }
	  if (csum != 0)
	    {
	      ++sl_errs;
	      return EKGDB_CSUM;
	    }
	  --immediate_quit;

	  /* Store saved rpc reply type */
	  *tp = type;

	  /* Store length of rpc reply packet */
	  if (lenp)
	    *lenp = len;

	  if (ip)
	    bcopy (buf, ip, len);
	  return 0;
	}
      csum += c;
      if (type < 0)
	{
	  type = c;
	  escape = 0;
	  continue;
	}
      if (++len > (int) sizeof buf)
	{
	  do
	    {
	      if ((c = GETC (to)) < 0)
		return -c;
	    }
	  while (c != FRAME_END);
	  return EKGDB_2BIG;
      }
      *bp++ = c;

      escape = 0;
    }
}

static void
set_sl_baudrate_command (char *arg, int from_tty)
{
  int baudrate;

  if (arg == 0)
    error_no_arg ("set remote-baudrate");
  while (*arg == ' ' || *arg == '\t')
    ++arg;
  if (*arg == 0)
    error_no_arg ("set remote-baudrate");
  if (*arg < '0' || *arg > '9')
    error ("non-numeric arg to \"set remote-baudrate\".");

  baudrate = atoi (arg);
  if (baudrate < 0)
    error ("unknown baudrate for \"set remote-baudrate\".");
  speed = baudrate;

  /* Don't use command line option anymore.  */
  baud_rate = -1;
}

static void
sl_info (char *arg, int from_tty)
{
  int linespeed = speed;

  if (speed <= EXTB)
    switch (speed)
      {
      default:		linespeed = 0; break;
      case B50:		linespeed = 50; break;
      case B75:		linespeed = 75; break;
      case B110:	linespeed = 110; break;
      case B134:	linespeed = 134; break;
      case B150:	linespeed = 150; break;
      case B200:	linespeed = 200; break;
      case B300:	linespeed = 300; break;
      case B600:	linespeed = 600; break;
      case B1200:	linespeed = 1200; break;
      case B1800:	linespeed = 1800; break;
      case B2400:	linespeed = 2400; break;
      case B4800:	linespeed = 4800; break;
      case B9600:	linespeed = 9600; break;
      case EXTA:	linespeed = 19200; break;
      case EXTB:	linespeed = 38400; break;
      }
  printf ("sl-baudrate     %6d\n", linespeed);
  printf ("bytes received  %6d\n", sl_inbytes);
  printf ("bytes sent      %6d\n", sl_outbytes);
  printf ("checksum errors %6d\n", sl_errs);
}

extern struct cmd_list_element *setlist;

void
_initialize_sl ()
{
  add_info ("sl", sl_info,
	    "Show current settings of serial line debugging options.");
  add_cmd ("sl-baudrate", class_support, set_sl_baudrate_command,
	   "Set remote debug serial line baudrate.", &setlist);
}
