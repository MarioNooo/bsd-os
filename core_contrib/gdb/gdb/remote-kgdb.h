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
 * Copyright (c) 1991, 1992 Regents of the University of California.
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
 *
 * @(#) /master/usr.bin/gdb/gdb/remote.h,v 2.1 1995/02/03 11:56:27 polk Exp (LBL)
 */

struct remote_kgdb_fn
{
  int (*send) (unsigned char, unsigned char *, int); /* send an rpc message */
  int (*recv) (int *, unsigned char *, int *, int); /* receive an rpc message */
  void (*close) (void);			/* shutdown the link layer */
  int maxdata;				/* maximum number of read/write bytes */
  int rpcsize;				/* size of rpc msg buffers */
};

/* Error codes.  */
#define EKGDB_CSUM	1	/* failed checksum */
#define EKGDB_2BIG	2	/* "giant" packet */
#define EKGDB_RUNT	3	/* short packet */
#define EKGDB_BADOP	4	/* bad op code in packet */
#define EKGDB_TIMEOUT	5	/* request timed out */
#define EKGDB_IO	6	/* generic I/O error */
