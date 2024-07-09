/* External declarations common to kernel debugging in GDB.
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

extern int bsdi_kernel_debugging;

CORE_ADDR bsdi_ksym_lookup (char *);
CORE_ADDR bsdi_ksym_lookup_default (char *, CORE_ADDR);
void bsdi_set_curproc (void);

extern struct gdbarch_data *bsdi_set_curproc_handle;
extern struct gdbarch_data *bsdi_set_procaddr_com_handle;

extern struct gdbarch_data *bsdi_kernel_core_xfer_memory_handle;
extern struct gdbarch_data *bsdi_kernel_core_registers_handle;

/* This variable allows random kgdb code to reset state after changes.  */
extern int remote_kgdb_generation;
