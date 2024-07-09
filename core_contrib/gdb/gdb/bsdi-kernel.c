/* Generic BSD/OS kernel debugging support.
   Copyright 1986, 1987, 1989, 1991, 2003 Free Software Foundation, Inc.
   
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

#include <sys/param.h>
#include <ctype.h>
#include <stdlib.h>
#include "defs.h"
#include "value.h"
#include "symtab.h"
#include "frame.h"
#include "target.h"
#include "inferior.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "bsdi-kernel.h"

static int set_procaddr (CORE_ADDR);
static void set_procaddr_com (char *, int);

/* This variable represents the state of the -k flag to gdb.
   It enables BSD/OS kernel debugging features.  */

int bsdi_kernel_debugging;

/* Look up the address of a symbol and report a meaningful message 
   if not found.  */

CORE_ADDR
bsdi_ksym_lookup (char *name)
{
  struct minimal_symbol *sym;

  sym = lookup_minimal_symbol (name, NULL, NULL);
  if (sym == NULL)
    error ("Kernel symbol `%s' not found.", name);

  return SYMBOL_VALUE_ADDRESS (sym);
}

/* Look up the address of a symbol and return a default value if not found.  */

CORE_ADDR
bsdi_ksym_lookup_default (char *name, CORE_ADDR defvalue)
{
  struct minimal_symbol *sym;

  sym = lookup_minimal_symbol(name, NULL, NULL);
  if (sym == NULL)
	  return defvalue;

  return SYMBOL_VALUE_ADDRESS (sym);
}

/* The following functions call architecture dependent implementations.  */

struct gdbarch_data *bsdi_set_curproc_handle;

void
bsdi_set_curproc ()
{
  void (*arch_set_curproc) (void);

  arch_set_curproc = gdbarch_data (bsdi_set_curproc_handle);
  arch_set_curproc ();
}

struct gdbarch_data *bsdi_set_procaddr_com_handle;

static void
set_procaddr_com (char *arg, int from_tty)
{
  void (*arch_set_procaddr_com) (char *, int);

  arch_set_procaddr_com = gdbarch_data (bsdi_set_procaddr_com_handle);
  arch_set_procaddr_com (arg, from_tty);
}

/* Pull a pointer out of the image and byte-swap it into host order.  */

static inline CORE_ADDR
read_memory_core_addr (CORE_ADDR v)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  char *result = alloca (core_addr_len);

  if (target_read_memory (v, result, core_addr_len) != 0)
    error ("cannot read pointer at %#x", v);
  return extract_address (result, (int) core_addr_len);
}

/* Pull an int out of the image and byte-swap it into host order.  */

static inline LONGEST
read_memory_int (CORE_ADDR v)
{
  const size_t int_len = TARGET_INT_BIT / TARGET_CHAR_BIT;
  char *result = alloca (int_len);

  if (target_read_memory (v, result, int_len) != 0)
    error ("cannot read int at %#x", v);
  return extract_signed_integer (result, (int) int_len);
}

/* Pull an unsigned int out of the image and byte-swap it into host order.  */

static inline ULONGEST
read_memory_uint (CORE_ADDR v)
{
  const size_t uint_len = TARGET_INT_BIT / TARGET_CHAR_BIT;
  char *result = alloca (uint_len);

  if (target_read_memory (v, result, uint_len) != 0)
    error ("cannot read unsigned int at %#x", v);
  return extract_unsigned_integer (result, (int) uint_len);
}

/* Pull an unsigned int out of the image and byte-swap it into host order.  */

static inline ULONGEST
read_memory_ulonglong (CORE_ADDR v)
{
  const size_t ulonglong_len = TARGET_LONG_LONG_BIT / TARGET_CHAR_BIT;
  char *result = alloca (ulonglong_len);

  if (target_read_memory (v, result, ulonglong_len) != 0)
    error ("cannot read unsigned long long at %#x", v);
  return extract_unsigned_integer (result, (int) ulonglong_len);
}

static void
ktr_command (char *s, int from_tty)
{
  const char delta_format[] = "%d %5x %12x %s\n";
  const char timestamp_format[] = "%d %5x %16x %s\n";
  static enum { DELTA, TIMESTAMP } tick_mode = DELTA;
  static enum direction { BACKWARD, FORWARD } direction = BACKWARD;
  enum direction sign;
  static LONGEST ktr_size_mask;
  static CORE_ADDR cpuhead_addr;
  static int current_cpuno = -1;
  int new_cpuno = current_cpuno;
  static CORE_ADDR kgdb_lock_addr;
  static size_t p_cpuid;
  static size_t sizeof_ktr;
  static size_t pcpu_cpuno;
  static size_t pcpu_allcpu;
  static size_t pcpu_ktr_idx;
  static size_t pcpu_ktr_buf;
  static CORE_ADDR ktr_buf;
  static size_t ktr_clkv;
  static size_t ktr_desc;
  static size_t ktr_parm[5];
  static size_t mtx_lock;
  static LONGEST idx;
  static LONGEST position;
  static int generation = 0;
  int state_changed;
  int at_last_idx;
  LONGEST offset = 1;
  LONGEST explicit_offset;
  static ULONGEST clkv;
  static ULONGEST previous_clkv = 0;
  /* 0 = uninitialized, -1 = not configured.  */
  CORE_ADDR ktr_size_mask_addr;
  int need_last_idx_hack = 0;
  static long count = 1;
  long n;
  long new_count;
  char *e;

  if (! bsdi_kernel_debugging)
    error ("Kernel debugging is not enabled for KTR.");

  /* Configure our KTR parameters.  */
  if (ktr_size_mask == 0)
    {
      ktr_size_mask_addr = bsdi_ksym_lookup_default ("ktr_size_mask", 0);
      cpuhead_addr = bsdi_ksym_lookup_default ("cpuhead", 0);
      kgdb_lock_addr = bsdi_ksym_lookup_default ("kgdb_lock", 0);
      p_cpuid = (size_t) bsdi_ksym_lookup_default ("P_CPUID", -1);
      sizeof_ktr = (size_t) bsdi_ksym_lookup_default ("SIZEOF_KTR", -1);
      pcpu_cpuno = (size_t) bsdi_ksym_lookup_default ("PCPU_CPUNO", -1);
      pcpu_allcpu = (size_t) bsdi_ksym_lookup_default ("PCPU_ALLCPU", -1);
      pcpu_ktr_idx = (size_t) bsdi_ksym_lookup_default ("PCPU_KTR_IDX", -1);
      pcpu_ktr_buf = (size_t) bsdi_ksym_lookup_default ("PCPU_KTR_BUF", -1);
      ktr_clkv = (size_t) bsdi_ksym_lookup_default ("KTR_CLKV", -1);
      ktr_desc = (size_t) bsdi_ksym_lookup_default ("KTR_DESC", -1);
      ktr_parm[0] = (size_t) bsdi_ksym_lookup_default ("KTR_PARM1", -1);
      ktr_parm[1] = (size_t) bsdi_ksym_lookup_default ("KTR_PARM2", -1);
      ktr_parm[2] = (size_t) bsdi_ksym_lookup_default ("KTR_PARM3", -1);
      ktr_parm[3] = (size_t) bsdi_ksym_lookup_default ("KTR_PARM4", -1);
      ktr_parm[4] = (size_t) bsdi_ksym_lookup_default ("KTR_PARM5", -1);
      mtx_lock = (size_t) bsdi_ksym_lookup_default ("MTX_LOCK", -1);
      if (ktr_size_mask_addr == 0
	 || (ktr_size_mask = read_memory_int (ktr_size_mask_addr)) == 0
	 || cpuhead_addr == 0 || p_cpuid == -1 || sizeof_ktr == -1
	 || pcpu_cpuno == -1 || pcpu_allcpu == -1 || pcpu_ktr_idx == -1
	 || pcpu_ktr_buf == -1 || ktr_clkv == -1 || ktr_desc == -1
	 || ktr_parm[-1] == -1 || ktr_parm[1] == -1 || ktr_parm[2] == -1
	 || ktr_parm[3] == -1 || ktr_parm[4] == -1 || mtx_lock == -1)
	ktr_size_mask = -1;
    }
  if (ktr_size_mask == -1)
    error ("KTR was not configured or not initialized in this kernel.");

  state_changed = generation != remote_kgdb_generation;
  generation = remote_kgdb_generation;

  if (s == NULL)
    s = "";

  while (isblank (*s))
    ++s;

  /* Parse options.  */
  if (*s == '/')
    {
      while (*++s != '\0' && !isblank(*s))
	switch (*s)
	  {
	  case '/':
	    break;
	  case 'c':
	  case 'C':
	    new_cpuno = (int) strtol (s, &e, 10);
	    if (s == e)
	      new_cpuno = 0;
	    s = e;
	    break;
	  case 'd':
	  case 'D':
	    tick_mode = DELTA;
	    break;
	  case 't':
	  case 'T':
	    tick_mode = TIMESTAMP;
	    break;
	  default:
	    /* XXX Print a message about unknown flags?  */
	    break;
	  }
      while (isblank (*s))
	++s;
    }

  /* Set the default CPU number.  */
  if (state_changed || current_cpuno != new_cpuno)
    {
      CORE_ADDR pcpu;

      /* If the user didn't request a specific CPU number,
	 use the current one.  */
      if (new_cpuno < 0 && kgdb_lock_addr != 0)
	{
	  CORE_ADDR paddr = read_memory_core_addr (kgdb_lock_addr + mtx_lock);

	  /* The mtx_lock field contains the proc address of the lock owner.
	     The p_cpuid field in the proc structure records the CPU number.
	     If we're looking at a crash dump, the lock might be unowned.  */
	  if (paddr == 0 || paddr == 8)
	    new_cpuno = 0;
	  else
	    new_cpuno = read_memory_integer (paddr + p_cpuid, 1);
	}

      pcpu = read_memory_core_addr (cpuhead_addr);

      while (pcpu != 0)
	{
	  int this_cpuno = read_memory_int (pcpu + pcpu_cpuno);

	  if (this_cpuno == new_cpuno)
	    break;
	  pcpu = read_memory_core_addr (pcpu + pcpu_allcpu);
	}

      if (pcpu == 0)
	error ("Can't locate CPU %d.", new_cpuno);

      current_cpuno = new_cpuno;
      ktr_buf = read_memory_core_addr (pcpu + pcpu_ktr_buf);
      /* Idx points at the last entry, which is not the same as pc_ktr_idx.
	 Yes, that's confusing.  */
      position = (read_memory_int (pcpu + pcpu_ktr_idx) & ktr_size_mask);
      idx = (position - sizeof_ktr) & ktr_size_mask;
      need_last_idx_hack = 1;
    }

  /* Get the starting address.  */
  switch (*s)
    {
    case '+':
      if (!isblank (*++s) && (explicit_offset = strtol (s, &e, 0)) > 0)
	{
	  offset = explicit_offset;
	  s = e;
	}
      position = (position + (offset * sizeof_ktr)) & ktr_size_mask;
      break;
    case '-':
      if (!isblank (*++s) && (explicit_offset = strtol (s, &e, 0)) > 0)
	{
	  offset = explicit_offset;
	  s = e;
	  /* Hack: at starting position, we pretend that '-OFFSET' is an offset
	     from the last record, but straight '-' prints the last record.
	     This allows you to start with 'ktr' or 'ktr -' and repeat a
	     few times to see the last few records.  */
	  if (need_last_idx_hack)
	    ++offset;
	}
      position = (position - (offset * sizeof_ktr)) & ktr_size_mask;
      break;
    case '\0':
      /* If no starting point, step 1 in the same direction as before.  */
      if (direction == FORWARD)
	position = (position + sizeof_ktr) & ktr_size_mask;
      else
	position = (position - sizeof_ktr) & ktr_size_mask;
      break;
    case '.':
      /* Hack: An initial 'ktr .' shows the last record, not the first.  */
      if (need_last_idx_hack)
	position = (position - sizeof_ktr) & ktr_size_mask;
      ++s;
      break;
    default:
      offset = strtol (s, &e, 16);
      if (s == e)
	offset = 1;
      s = e;
      position = (offset * sizeof_ktr) & ktr_size_mask;
      break;
    }
  while (isblank (*s))
    ++s;

  /* Get the repetition count.  */
  switch (*s)
    {
    case '\0':
      /* If no count, use the same count as before.  */
      break;
    case '+':
      direction = FORWARD;
      if (!isblank (*++s))
	{
	  if ((new_count = strtol (s, &e, 0)) > 0)
	    count = new_count;
	  else if (s == e)
	    count = 1;
	}
      break;
    case '-':
      direction = BACKWARD;
      if (! isblank (*++s))
	{
	  if ((new_count = strtol (s, &e, 0)) > 0)
	    count = new_count;
	  else if (s == e)
	    count = 1;
	}
      break;
    default:
      count = strtol (s, &e, 0);
      if (s == e)
	count = 1;
      break;
    }

  /* If we're printing tick deltas, get the previous tick.
     Note the 'singularity' at the last record.  */
  if (tick_mode == DELTA)
    {
      LONGEST previous;

      if (direction == FORWARD)
	{
	  if (position == ((idx + sizeof_ktr) & ktr_size_mask))
	    previous = position;
	  else
	    previous = (position - sizeof_ktr) & ktr_size_mask;
	}
      else
	{
	  if (position == idx)
	    previous = position;
	  else
	    previous = (position + sizeof_ktr) & ktr_size_mask;
	}

      previous_clkv = read_memory_ulonglong (ktr_buf + previous + ktr_clkv);
    }

  /* Loop over KTR entries, printing each one.  */
  for (n = count;;)
    {
      CORE_ADDR format_addr;
      struct cleanup *old_chain;
      unsigned int prt_idx = position / sizeof_ktr;
      /* XXX This code assumes way too much about host vs. target sizes.  */
      unsigned int parm[5];
      char *format;
      char *desc;
      int i;

      /* Scan the format and copy string parameters into our address space.  */
      format_addr = read_memory_core_addr (ktr_buf + position + ktr_desc);
      target_read_string (format_addr, &format, 1024, NULL);
      old_chain = make_cleanup (xfree, format);
      for (i = 0, s = format; (s = (strchr (s, '%'))) != NULL; )
	{
	  char *parm_string;

	  if (*++s == '%')
	    continue;
	  s += strspn (s, "0123456789ul.-");
	  parm[i] = read_memory_uint (ktr_buf + position + ktr_parm[i]);
	  if (*s == 's')
	    {
	      target_read_string ((CORE_ADDR) parm[i], &parm_string, 1024,
				  NULL);
	      make_cleanup (xfree, parm_string);
	      parm[i] = (unsigned int)parm_string;
	    }
	  ++i;
	}
      /* XXX BIG ASSUMPTION here that the native printf() can print
	the target's operands correctly!  */
      asprintf (&desc, format, parm[0], parm[1], parm[2], parm[3], parm[4]);
      if (desc == NULL)
	error ("Can't allocate memory for KTR descriptor.");
      make_cleanup (xfree, desc);

      clkv = read_memory_ulonglong (ktr_buf + position + ktr_clkv);
      if (tick_mode == DELTA)
	{
	  ULONGEST delta;

	  if (direction == BACKWARD)
	    delta = previous_clkv - clkv;
	  else
	    delta = clkv - previous_clkv;
	  /* XXX Libiberty's printf() can't handle long longs,
	     hence the cast to 'unsigned int'.  */
	  printf_filtered (delta_format, current_cpuno,
			   prt_idx, (unsigned int) delta, desc);
	}
      else
	/* XXX Libiberty's printf() can't handle long longs.  */
	printf_filtered (timestamp_format, current_cpuno,
			 prt_idx, (unsigned int) clkv, desc);

      do_cleanups (old_chain);

      if (--n <= 0)
	break;

      if (direction == BACKWARD)
	position = (position - sizeof_ktr) & ktr_size_mask;
      else
	position = (position + sizeof_ktr) & ktr_size_mask;
      previous_clkv = clkv;
    }
}

void
_initialize_bsdi_kernel ()
{
  add_com ("process-address", class_obscure, set_procaddr_com, 
	   "Make the process with proc structure at ADDR "
	   "be the current process.\n"
	   "Used in kernel debugging to set the stack for backtracing, etc.");
  add_com_alias ("paddr", "process-address", class_obscure, 0);
  add_com ("ktr", class_obscure, ktr_command,
	   "Dump records from the kernel trace buffer.\n"
	   "Usage:\n"
	   "  ktr[/d][/t][/cCPUNO] [[[.|+]OFFSET|POSITION] "
	   "[[+|-]COUNT]]\n"
	   "where\n"
	   "  /d        print deltas rather than tick counts\n"
	   "  /t        print tick counts rather than deltas\n"
	   "  /cCPUNO   switch tracing to processor number CPUNO\n"
	   "  .         start printing from the current trace record\n"
	   "  +OFFSET   start printing at OFFSET records ahead\n"
	   "  -OFFSET   start printing at OFFSET records behind\n"
	   "  POSITION  start printing from the given record number\n"
	   "  +COUNT    print forward COUNT records\n"
	   "  -COUNT    print backward COUNT records\n"
	   "The tick mode (deltas vs. timestamps), the processor number, "
	   "the print direction\n"
	   "and the count are sticky: new values are remembered in later "
	   "commands, unless\n"
	   "the remote kernel is detached or continued.\n"
	   "If OFFSET is omitted after a sign, KTR assumes a value of 1.\n"
	   "If COUNT is omitted after a sign, KTR preserves the count but "
	   "sets the direction.\n"
	   "If no offset or count are provided, KTR starts at the next record "
	   "in the previously\n"
	   "specified direction, printing the same count in the same direction "
	   "as before.\n");
  add_show_from_set
    (add_set_cmd ("kernel-debugging", class_obscure, var_boolean,
		  (char *) &bsdi_kernel_debugging,
		  "Set BSD/OS kernel debugging features.", &setlist),
     &showlist);

  bsdi_set_curproc_handle = register_gdbarch_data (NULL, NULL);
  bsdi_set_procaddr_com_handle = register_gdbarch_data (NULL, NULL);
}
