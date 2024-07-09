/* Native-dependent definitions for Intel 386 running BSD/OS, for GDB.
   Copyright 1986, 1987, 1989, 1992, 1994 Free Software Foundation, Inc.

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

#ifndef NM_BSDI_H
#define NM_BSDI_H

/* Cheerfully stolen from the NetBSD headers... */

#include <string.h>		/* for memcpy() prototype */

/* Get generic BSD/OS native definitions. */
#include "config/nm-bsdi.h"

#define	FETCH_INFERIOR_REGISTERS

typedef struct { int greg[17]; } bsdi_gregset_t;
typedef struct { int fpreg[65]; } bsdi_fpregset_t;

#define	GDB_GREGSET_T bsdi_gregset_t
#define	GDB_FPREGSET_T bsdi_fpregset_t

/* Native register ordering is the same as GDB's ordering.  */
extern __inline void
supply_gregset (bsdi_gregset_t *g)
{
  extern char *registers;

  memcpy (&registers[0], &g->greg[0], 16 * 4);
}

extern __inline void
supply_fpregset (bsdi_fpregset_t *unused)
{
  /* Nothing, for now.  */
}

extern __inline void
fill_gregset (bsdi_gregset_t *g, int unused)
{
  extern char *registers;

  memcpy (&g->greg[0], &registers[0], 16 * 4);
}

extern __inline void
fill_fpregset (bsdi_fpregset_t *unused1, int unused2)
{
  /* Nothing, for now.  */
}

#endif /* NM_BSDI_H */
