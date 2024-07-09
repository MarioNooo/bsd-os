/* Header for BSD's KGDB protocol, a SLIP-like protocol for
   remote kernel debugging.
   Copyright 2003
   Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* This file exports the same values as the BSD kernel kgdb_proto.h file,
   so that KGDB can be supported on non-BSD hosts such as (gulp) Windows.  */

/* Command codes.  */

#define	KGDB_MEM_R	1
#define	KGDB_MEM_W	2
#define	KGDB_REG_R	3
#define	KGDB_REG_W	4
#define	KGDB_CONT	5
#define	KGDB_STEP	6
#define	KGDB_KILL	7
#define	KGDB_SIGNAL	8
#define	KGDB_EXEC	9

#define	KGDB_CMD(c)	((c) & 0xf)

/* Protocol control bits.  */

#define	KGDB_SEQ	0x10
#define	KGDB_MORE	0x20
#define	KGDB_DELTA	0x40
#define	KGDB_ACK	0x80
