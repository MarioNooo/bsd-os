/* Macro definitions for GDB on all BSDI target systems.
   This first section is stolen from config/tm-sysv4.h:
   Copyright 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
   Written by Fred Fish at Cygnus Support (fnf@cygnus.com).

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


/* Support for BSDI kernel debugging.  */

extern int bsdi_kernel_debugging;
extern struct target_ops child_ops;
extern struct target_ops core_ops;
extern struct target_ops bsdi_kernel_core_ops;
extern void bsdi_kernel_set_sections (void);

#define	ADDITIONAL_OPTIONS \
     {"k", no_argument, 0, 'k'},

#define	ADDITIONAL_OPTION_CASES \
  case 'k': \
    bsdi_kernel_debugging = 1; \
    break;

#define	ADDITIONAL_OPTION_HELP \
     "  -k                 Treat EXECFILE as a BSD kernel.\n"


/* Machine-independent thread-related stuff.  */

int bsdi_in_solib_call_trampoline (CORE_ADDR, char *);
CORE_ADDR find_bsdi_solib_trampoline_target (CORE_ADDR);
int bsdi_thread_prepare_to_proceed (int);

#include <pthread.h>
#include <pthread_var.h>

/* An extension for caching BSD/OS user thread information.
   XXX For cross-debugging, we need to move the pthread data
   out into a separate memory, and then access it only using
   architecture-specific ops.  We should NOT include pthread.h
   or pthread_var.h, which are host files.  */

struct pthread_cache
{
  struct pthread pthread;
  struct pthread_cache *next;
  ptid_t id;
  long stamp;
};

/* Architecture dependent operations for BSD/OS user threads.  */
struct bsdi_arch_thread_ops
{
  void (*fetch_registers) (struct pthread_cache *);
  void (*store_registers) (struct pthread_cache *);
  int (*event) (CORE_ADDR *);
  int (*attach_event) (CORE_ADDR *);
  int (*core_event) (CORE_ADDR *);
  void (*resume) (void);
};

extern struct gdbarch_data *bsdi_arch_thread_ops_handle;

/* Export generic_bsdi_so_ops, so that we can override the default value
   of current_target_so_ops.  */

extern struct target_so_ops generic_bsdi_so_ops;
