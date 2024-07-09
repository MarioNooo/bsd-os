/* Work with BSD/OS kernel crash dumps.
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

/* BSD/OS kernel crash dumps are currently just a snapshot of physical
   memory on the system shortly before a crash.  We dig through kernel
   data structures to recover information about processes, using the
   native KVM library to simulate virtual mappings.

   TO DO:
	On the OS side, we should generate ELF crash dumps that
	present the normal kernel virtual mapping and .reg/N sections
	for each process's kernel register state.  This would allow us
	to omit user memory in enormous crash dumps, and it would
	relieve GDB of the duty of interpreting the virtual space in
	the dump, so that we could cross-debug crash dumps easily.
	It would also let BFD handle kernel crash dump / core file
	interpretation, which would better fit the model that GDB
	uses (sigh).

	On the GDB side, we should add (actually, finish adding)
	code to recognize kernel processes as threads.  This would
	help live kernel debugging too.  */

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "defs.h"
#include "frame.h"  /* required by inferior.h */
#include "inferior.h"
#include "symtab.h"
#include "command.h"
#include "bfd.h"
#include "target.h"
#include "gdbcore.h"
#include "bsdi-kernel.h"

#include <kvm.h>
#include <sys/stat.h>

/* Exported to architecture-dependent kgdb files.  */
kvm_t *bsdi_kd;

/* Forward declaration.  */
extern struct target_ops kernel_core_ops;


/* Discard all vestiges of any previous core file
   and mark data and stack spaces as empty.  */

static void
kernel_core_close (int quitting)
{
  if (bsdi_kd != NULL)
    {
      kvm_close (bsdi_kd);
      bsdi_kd = NULL;
    }
}

static void
kernel_core_close_cleanup (void *unused)
{
  kernel_core_close (0);
}

/* Read the panic string and print it out if set.  */

static void
panicinfo ()
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  char *panicstr_buf = alloca (core_addr_len);
  CORE_ADDR panicstr;
  char *cp;
  char buf[256];

  /* Print out the panic string if there is one.  Don't use
     read_memory_unsigned_integer() because it prints errors.  */
  if (target_read_memory (bsdi_ksym_lookup ("panicstr"), panicstr_buf,
			  core_addr_len))
    return;
  panicstr = extract_address (panicstr_buf, core_addr_len);
  if (panicstr == 0)
    return;

  if (target_read_memory (panicstr, buf, sizeof buf))
    return;

  for (cp = buf; cp < &buf[sizeof buf] && *cp; cp++)
    if (!isascii (*cp) || (!isprint (*cp) && !isspace (*cp)))
      *cp = '?';
  *cp = '\0';

  if (buf[0] != '\0')
    printf ("panic: %s\n", buf);
}

/* This routine opens and sets up the core file bfd.  */

static void
kernel_core_open (char *filename, int from_tty)
{
  char *cp;
  struct cleanup *old_chain;
  int ontop;
  char *execfile;
  struct stat stb;
  kvm_t *temp_kd;

  bsdi_kernel_debugging = 1;

  if (bsdi_kd != NULL) 
    error ("kvm already target -- must detach first");

  target_preopen (from_tty);
  if (filename == NULL)
    error ("No core file specified.");
  filename = tilde_expand (filename);
  if (filename[0] != '/')
    {
      cp = concat (current_directory, "/", filename, NULL);
      free (filename);
      filename = cp;
    }
  old_chain = make_cleanup (free, filename);
  execfile = exec_bfd ? bfd_get_filename (exec_bfd) : NULL;
  if (execfile == 0) 
    error ("No executable image specified");

  /* If we want the active memory file, just use a null arg for kvm.
     The SunOS kvm can't read from the default swap device, unless
     /dev/mem is indicated with a null pointer.  This has got to be 
     a bug.  */
  if (stat (filename, &stb) == 0 && S_ISCHR (stb.st_mode))
    filename = NULL;

  temp_kd = kvm_open (execfile, filename, NULL,
		      write_files ? O_RDWR : O_RDONLY, "gdb");
  if (temp_kd == NULL)
    error ("Cannot open kernel core image");

  unpush_target (&kernel_core_ops);
  bsdi_kd = temp_kd;
  old_chain = make_cleanup (kernel_core_close_cleanup, NULL);
#if 0
  validate_files ();
#endif
  ontop = !push_target (&kernel_core_ops);
  discard_cleanups (old_chain);
  
  if (ontop)
    {
      target_fetch_registers (-1);
      
      /* Set up the frame cache, and print the top of stack.  */
      flush_cached_frames ();
#if 1
      set_current_frame (create_new_frame (read_register (FP_REGNUM),
					   read_pc ()));
#endif
      select_frame (get_current_frame (), 0);
      print_stack_frame (selected_frame, selected_frame_level, 1);
    }
  else
    {
      printf ("Warning: you won't be able to access this core file until "
	      "you terminate\n"
	      "your %s; do ``info files''\n", current_target.to_longname);
    }

  panicinfo ();
}

static void
kernel_core_detach (char *args, int from_tty)
{
  if (args)
    error ("Too many arguments");
  unpush_target (&kernel_core_ops);
  if (from_tty)
    printf ("No kernel core file now.\n");
}

static void
kernel_core_files_info (struct target_ops *t)
{
#if 0
  struct section_table *p;
  
  printf_filtered ("\t`%s', ", bfd_get_filename (core_bfd));
  wrap_here ("        ");
  printf_filtered ("file type %s.\n", bfd_get_target (core_bfd));
 
  for (p = t->sections; p < t->sections_end; p++)
    {
      printf_filtered ("\t%s", local_hex_string_custom (p->addr, "08"));
      printf_filtered (" - %s is %s",
		       local_hex_string_custom (p->endaddr, "08"),
		       bfd_section_name (p->bfd, p->sec_ptr));
      if (p->bfd != core_bfd)
	printf_filtered (" in %s", bfd_get_filename (p->bfd));
      printf_filtered ("\n");
    }
#endif
}

/* The following target methods are architecture specific.  */

struct gdbarch_data *bsdi_kernel_core_xfer_memory_handle;

static int
kernel_core_xfer_memory (CORE_ADDR addr, char *cp, int len, int write,
     struct mem_attrib *attrib, struct target_ops *target)
{
  int (*core_xfer_memory) (CORE_ADDR, char *, int, int, struct mem_attrib *,
		           struct target_ops *);

  core_xfer_memory = gdbarch_data (bsdi_kernel_core_xfer_memory_handle);
  return core_xfer_memory (addr, cp, len, write, attrib, target);
}

struct gdbarch_data *bsdi_kernel_core_registers_handle;

static void
kernel_core_registers (int regno)
{
  void (*core_registers) (int);

  core_registers = gdbarch_data (bsdi_kernel_core_registers_handle);
  core_registers (regno);
}


static int
ignore (CORE_ADDR addr, char *contents)
{
  return 0;
}

struct target_ops kernel_core_ops;

void
init_kernel_core_ops ()
{
  kernel_core_ops.to_shortname = "kvm";
  kernel_core_ops.to_longname = "Kernel core file";
  kernel_core_ops.to_doc = "Use a kernel core file as a target.  Specify the filename of the core file.";
  kernel_core_ops.to_open = kernel_core_open;
  kernel_core_ops.to_close = kernel_core_close;
  kernel_core_ops.to_attach = find_default_attach;
  kernel_core_ops.to_detach = kernel_core_detach;
  kernel_core_ops.to_fetch_registers = kernel_core_registers;
  kernel_core_ops.to_xfer_memory = kernel_core_xfer_memory;
  kernel_core_ops.to_files_info = kernel_core_files_info;
  kernel_core_ops.to_insert_breakpoint = ignore;
  kernel_core_ops.to_remove_breakpoint = ignore;
  kernel_core_ops.to_create_inferior = find_default_create_inferior;
  kernel_core_ops.to_has_memory = 1;
  kernel_core_ops.to_has_stack = 1;
  kernel_core_ops.to_has_registers = 1;
  kernel_core_ops.to_magic = OPS_MAGIC;

  /* If we saw a -k flag, supersede the regular core ops.  */
  if (bsdi_kernel_debugging)
    {
      core_ops.to_stratum = dummy_stratum;
      kernel_core_ops.to_stratum = core_stratum;
    }
  else
    kernel_core_ops.to_stratum = dummy_stratum;
}

void
_initialize_bsdi_kernel_core ()
{
  init_kernel_core_ops ();

#if 0
  add_com ("kcore", class_files, core_file_command,
	   "Use FILE as core dump for examining memory and registers.\n"
	   "No arg means have no core file.  "
	   "This command has been superseded by the\n"
	   "`target core' and `detach' commands.");
#endif
  add_target (&kernel_core_ops);

  bsdi_kernel_core_xfer_memory_handle = register_gdbarch_data (NULL, NULL);
  bsdi_kernel_core_registers_handle = register_gdbarch_data (NULL, NULL);
}
