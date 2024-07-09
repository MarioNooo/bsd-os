/* Support for GDB targets running Wind River Systems' BSD/OS on IA-32.
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

#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <machine/frame.h>
#include <machine/reg.h>

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"
#include "i386-tdep.h"
#include "elf-bfd.h"
#include "bsdi-kernel.h"
#include "solist.h"
#include "solib-svr4.h"
#include "arch-utils.h"

static int i386bsdi_attach_thread_event (CORE_ADDR *);
static int i386bsdi_core_thread_event (CORE_ADDR *);
static CORE_ADDR i386bsdi_frame_chain (struct frame_info *);
static int i386bsdi_frame_chain_valid (CORE_ADDR, struct frame_info *);
static void i386bsdi_frame_init_saved_regs (struct frame_info *);
static CORE_ADDR i386bsdi_frame_saved_pc (struct frame_info *);
static int i386bsdi_thread_event (CORE_ADDR *);
static void i386bsdi_thread_fetch_registers (struct pthread_cache *);
static void i386bsdi_thread_resume (void);
static void i386bsdi_thread_store_registers (struct pthread_cache *);


#if !defined (offsetof)
#define offsetof (TYPE, MEMBER) ((unsigned long) &((TYPE *) 0)->MEMBER)
#endif

/* Thread support.
   XXX We need to handle floating point state too.
   XXX We need to remove dependencies on pthread_var.h.
   XXX We need to remove assumptions that host = target.  */

/* For each GDB register, we list the equivalent thread register offset.
   We only save callee-saved registers, so some GDB registers are omitted.  */

static int thread_reg_map[] =
{
  -1,
  -1,
  -1,
  1 * 4,	/* _THREAD_I386_EBX */
  2 * 4,	/* _THREAD_I386_ESP */
  3 * 4,	/* _THREAD_I386_EBP */
  4 * 4,	/* _THREAD_I386_ESI */
  5 * 4,	/* _THREAD_I386_EDI */
  0 * 4		/* _THREAD_I386_PC */
};

struct i386bsdi_thread_machdep_state
{
  char regs[6 * 4];
  char saved_fp[108];
};

/* Retrieve thread register state and convert it into GDB register state.
   XXX This code should NOT use native pthread headers.
   XXX This code should NOT assume that host == target.  */

static void
i386bsdi_thread_fetch_registers (struct pthread_cache *ptc)
{
  struct i386bsdi_thread_machdep_state *t = (void *)&ptc->pthread.saved_context;
  size_t i;

  for (i = 0; i < sizeof thread_reg_map / sizeof (int); ++i)
    if (thread_reg_map[i] >= 0)
      supply_register (i, &t->regs[thread_reg_map[i]]);
#if 0
  i387_supply_fsave (&t->saved_fp);
#endif
}

/* Convert GDB's register state back into thread register state.
   XXX The same issues mentioned for fetching registers apply here.  */

static void
i386bsdi_thread_store_registers (struct pthread_cache *ptc)
{
  struct i386bsdi_thread_machdep_state *t = (void *)&ptc->pthread.saved_context;
  struct pthread *external_pthread = (void *)ptid_get_tid (ptc->id);
  struct i386bsdi_thread_machdep_state *xt
    = (void *)&external_pthread->saved_context;
  int ret;
  size_t i;

  for (i = 0; i < sizeof thread_reg_map / sizeof (int); ++i)
    if (thread_reg_map[i] >= 0)
      memcpy (&t->regs[thread_reg_map[i]], &registers[REGISTER_BYTE (i)],
	      REGISTER_RAW_SIZE (i));

  ret = child_ops.to_xfer_memory ((CORE_ADDR) &xt->regs[0],
				  (char *) &t->regs[0], sizeof t->regs,
				  1, NULL, NULL);
  if (ret != sizeof t->regs)
    error ("i386bsdi_thread_store_registers: store failed");
}

/* Check for a thread event during native debugging.
   A thread event occurs when the child calls ptrace(PT_EVENT, ...,
   PT_EVT_THREAD_HOOK) to deliver the address of a data
   structure that we use to recover user thread data.
   The system automatically sends a SIGTRAP if a process
   makes this system call and it is being debugged.  */

static int
i386bsdi_thread_event (CORE_ADDR *v)
{
  const caddr_t pcb_flags_offset
    = (caddr_t) offsetof (struct user, u_pcb.pcb_flags);
  const caddr_t lastsyscall_offset
    = (caddr_t) offsetof (struct user, u_lastsyscall);
  pid_t pid = PIDGET (inferior_ptid);
  int r[NUM_REGS];
  int syscall_number;
  int esp;
  int req;
  int event;

  /* We assume that we're looking at a stopped child.
     We also assume that inferior_pid is the child's real PID.
     Note that we don't work through the GDB register cache,
     since we don't know which thread owns these registers yet.  */

  /* Are we in a syscall?  */
  if ((ptrace (PT_READ_U, pid, pcb_flags_offset, 0) & FM_TYPE_MASK)
      != FM_TYPE_SYSC)
    return 0;

  /* Is it a ptrace() syscall?  */
  syscall_number = ptrace (PT_READ_U, pid, lastsyscall_offset, 0);
  if (syscall_number == -1)
    error ("Can't get syscall number for thread event");
  if (syscall_number != SYS_ptrace)
    return 0;

  /* Is it a PT_EVENT request?  */
  if (ptrace (PT_GETREGS, pid, (caddr_t) r, 0) == -1)
    error ("Can't get ESP register for thread event");
  esp = r[SP_REGNUM];
  req = ptrace (PT_READ_D, pid, (caddr_t) esp + 4, 0);
  if (req == -1)
    error ("Can't get ptrace request for thread event");
  if (req != PT_EVENT)
    return 0;

  /* XXX We should check for PT_EVT_THREAD_HOOK.  */

  /* It's a thread event.  If needed, fetch the event value.  */
  if (v != NULL)
    {
      event = ptrace (PT_READ_D, pid, (caddr_t) esp + 12, 0);
      if (event == -1)
	error ("Can't get ptrace event information");
      *v = (CORE_ADDR) event;
    }

  return 1;
}

/* Dig out the ptrace event after attaching to a process.
   Here we assume native debugging.  */

static int
i386bsdi_attach_thread_event (CORE_ADDR *v)
{
  int addr_ptrace_event;
  const caddr_t ptrace_event_offset
    = (caddr_t) offsetof (struct user, u_kproc.kp_eproc.e_ptrace_event);

  /* We assume that we're looking at a stopped child.
     This read forces the system to fill eproc, so it's expensive...  */
  addr_ptrace_event
    = ptrace (PT_READ_U, PIDGET (inferior_ptid), ptrace_event_offset, 0);
  if (addr_ptrace_event == -1)
    return 0;
  *v = (CORE_ADDR) addr_ptrace_event;
  return 1;
}

/* Report the ptrace_event value for a core dump.
   The ptrace_event value is saved in the kinfo_proc structure
   in the third core note in the ELF core dump.
   Here we assume that it is the last element in the core note section.  */

static int
i386bsdi_core_thread_event (CORE_ADDR *v)
{
  const bfd_size_type ptr_bytes = TARGET_PTR_BIT / TARGET_CHAR_BIT;
  char buf[ptr_bytes];
  asection *sect;

  if (core_bfd == NULL)
    return 0;

  sect = bfd_get_section_by_name (core_bfd, "note0");
  if (sect == NULL)
    return 0;

  if (bfd_seek (core_bfd, sect->filepos + sect->_raw_size - 4, SEEK_SET) != 0)
    return 0;
  if (bfd_bread (buf, ptr_bytes, core_bfd) != ptr_bytes)
    return 0;
  *v = extract_address(buf, ptr_bytes);
  return 1;
}

/* Take action when resuming a thread.  A no-op on IA-32.  */

static void
i386bsdi_thread_resume ()
{
}

/* Architecture-dependent operations on BSD/OS threads.  We copy
   these to 'bsdi_arch_thread_ops', and the architecture-independent
   code in bsdi-thread.c uses the ops to recover machine state.  */

struct bsdi_arch_thread_ops i386bsdi_arch_thread_ops =
{
  i386bsdi_thread_fetch_registers,
  i386bsdi_thread_store_registers,
  i386bsdi_thread_event,
  i386bsdi_attach_thread_event,
  i386bsdi_core_thread_event,
  i386bsdi_thread_resume
};


/* Core file ops.  */

#define	PCB_SAVEFPU_OFFSET	0x68

static void
fetch_core_registers (char *data, unsigned size, int which, CORE_ADDR reg_addr)
{
  int i;

  switch (which)
    {
    case 0:
      /* General registers are in GDB order.  */
      for (i = 0; i < FP0_REGNUM; ++i)
	supply_register (i, &data[REGISTER_BYTE (i)]);
      break;

    case 2:
      /* The floating point registers follow a TSS in the core note.  */
#if 0
      i387_supply_fsave (data + PCB_SAVEFPU_OFFSET);
#endif
      break;

    case 3:
      /* FALL THROUGH -- we don't currently support core MMX registers.  */
    default:
      break;
  }
}

static struct core_fns i386bsdi_core_fns =
{
  bfd_target_elf_flavour,
  default_check_format,
  default_core_sniffer,
  fetch_core_registers,
  NULL
};


/* Handle debugging of dynamically linked executables.  BSD/OS currently
   uses an ld.so derived from the NetBSD ELF ld.so whose link_map and
   r_debug structures appear to be compatible with Linux.  The code for
   the following function comes from the IA-32 Linux support code.  */

struct link_map_offsets *
i386bsdi_svr4_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_debug_size = 8;	/* The actual size is 20 bytes, but
				   this is all we need.  */
      lmo.r_map_offset = 4;
      lmo.r_map_size   = 4;

      lmo.link_map_size = 20;	/* The actual size is 552 bytes, but
				   this is all we need.  */
      lmo.l_addr_offset = 0;
      lmo.l_addr_size   = 4;

      lmo.l_name_offset = 4;
      lmo.l_name_size   = 4;

      lmo.l_next_offset = 12;
      lmo.l_next_size   = 4;

      lmo.l_prev_offset = 16;
      lmo.l_prev_size   = 4;
    }

  return lmp;
}


/* The i386_frame_chain() code stops when it reaches the entry file.
   This can break if symbols are stripped.  We rely on null chain pointers.  */

static CORE_ADDR
i386bsdi_frame_chain (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return frame->frame;

  return read_memory_unsigned_integer (frame->frame, 4);
}

/* As with frame_chain(), don't use inside_entry_func().
   We also deal with kernel debugging.  */

static int
i386bsdi_frame_chain_valid (CORE_ADDR chain, struct frame_info *frame)
{
  if (chain != 0 && bsdi_kernel_debugging)
    return i386bsdi_inside_kernstack (chain);
  return !inside_main_func (frame->pc);
}

/* Handle kernel debugging for frame_saved_pc().  Kernels stacks have
   trap, system call and interrupt frames rather than signal frames.  */

static CORE_ADDR
i386bsdi_frame_saved_pc (struct frame_info *frame)
{
  if (bsdi_kernel_debugging)
    return i386bsdi_kernel_frame_saved_pc (frame);

  if (frame->signal_handler_caller)
    return sigtramp_saved_pc (frame);

  return read_memory_unsigned_integer (frame->frame + 4, 4);
}

/* Handle kernel debugging for frame_init_saved_regs.  */

static void
i386bsdi_frame_init_saved_regs (struct frame_info *frame)
{
  if (bsdi_kernel_debugging)
    i386bsdi_kernel_frame_init_saved_regs (frame);
  else
    i386_frame_init_saved_regs (frame);
}

/* We ignore the name and look at the stack trampoline instead.
   XXX The IN_SIGTRAMP() interface ought to be multi-arch, no?  */

int
i386bsdi_in_sigtramp (CORE_ADDR pc, char *unused)
{
  static CORE_ADDR psp;
  struct minimal_symbol *m;

  if (psp == 0)
    {
      if ((m = lookup_minimal_symbol ("__ps_strings", 0, 0)) == 0)
	/* help! */
	return 0;
      psp = read_memory_unsigned_integer (SYMBOL_VALUE_ADDRESS (m), 4);
    }

  return pc >= psp - 32 && pc < psp;
}

/* See whether the given ELF note section is a BSD/OS ABI tag.
   We're given a reference to an i386_abi_handler pointer;
   we clear the pointer on success.  */

static void
process_i386bsdi_abi_tag_sections (bfd *abfd, asection *sect, void *obj)
{
  struct i386_abi_handler **handlerp = obj;
  const char *name;
  unsigned int sect_size;

  /* When we find a matching section, we set *handlerp = NULL.  */
  if (*handlerp == NULL)
    return;

  name = bfd_get_section_name (abfd, sect);
  sect_size = bfd_section_size (abfd, sect);
  if ((strcmp (name, ".note.ABI-tag") == 0 || strcmp (name, "note0") == 0)
      && sect_size > 0)
    {
      unsigned int name_length, note_type;
      char *note = alloca (sect_size);

      bfd_get_section_contents (abfd, sect, note,
                                (file_ptr) 0, (bfd_size_type) sect_size);

      name_length = bfd_h_get_32 (abfd, note);
      note_type = bfd_h_get_32 (abfd, note + 8);

      if (name_length == sizeof "BSD/OS" && note_type == 1
          && strcmp (note + 12, "BSD/OS") == 0)
	*handlerp = NULL;
    }
}

/* Given an ELF IA-32 executable, return true if it's a BSD/OS executable.  */

static boolean
i386_check_bsdi_abi (struct i386_abi_handler *handler, bfd *abfd)
{
  if (elf_elfheader (abfd)->e_ident[EI_OSABI] != ELFOSABI_NONE)
    return false;

  bfd_map_over_sections (abfd, process_i386bsdi_abi_tag_sections, &handler);

  /* A null handler indicates success.  */
  return handler == NULL;
}

/* Set up the i386bsdi gdbarch methods.  We register ourselves as
   an OS ABI for the i386 architecture, and the i386_gdbarch_init()
   function calls us after performing the normal initialization.  */

static void
i386_init_abi_bsdi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  extern struct target_ops kernel_core_ops;
  extern void i386bsdi_set_curproc (void);
  extern void i386bsdi_set_procaddr_com (char *, int);
  extern int i386bsdi_kernel_core_xfer_memory (CORE_ADDR, char *, int, int,
					       struct mem_attrib *,
					       struct target_ops *);
  extern void i386bsdi_kernel_core_registers (int);
  extern int i386_is_call_insn (CORE_ADDR);
  extern CORE_ADDR i386_saved_pc_after_call (struct frame_info *);

  /* BSD/OS specific ops.  */
  set_gdbarch_frame_chain (gdbarch, i386bsdi_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, i386bsdi_frame_chain_valid);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);
  set_gdbarch_frame_saved_pc (gdbarch, i386bsdi_frame_saved_pc);
  set_gdbarch_frame_init_saved_regs (gdbarch, i386bsdi_frame_init_saved_regs);
  set_gdbarch_get_longjmp_target (gdbarch, get_longjmp_target);
  set_gdbarch_in_solib_call_trampoline (gdbarch, bsdi_in_solib_call_trampoline);
  set_gdbarch_is_call_insn (gdbarch, i386_is_call_insn);
  set_gdbarch_prepare_to_proceed (gdbarch, bsdi_thread_prepare_to_proceed);
  set_gdbarch_saved_pc_after_call (gdbarch, i386_saved_pc_after_call);
  set_gdbarch_skip_trampoline_code (gdbarch, find_bsdi_solib_trampoline_target);

#if 0
  set_gdbarch_num_regs (gdbarch, NUM_GREGS + NUM_FREGS + NUM_SSE_REGS);
#else
  set_gdbarch_num_regs (gdbarch, NUM_GREGS);
#endif

  if (bsdi_arch_thread_ops_handle != NULL)
    /* Specify the IA-32 flavor of BSD/OS thread ops.  */
    set_gdbarch_data (gdbarch, bsdi_arch_thread_ops_handle,
		      &i386bsdi_arch_thread_ops);

  /* Set up the shared library hack.  We do this here so that we can
     override any value set in an _initialize*() function in an
     solib-*.c file.  There surely is a better way to do this...  */
  current_target_so_ops = &generic_bsdi_so_ops;

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 i386bsdi_svr4_fetch_link_map_offsets);

  /* Set up some kernel debugging features.  */
  set_gdbarch_data (gdbarch, bsdi_kernel_core_xfer_memory_handle,
		    i386bsdi_kernel_core_xfer_memory);
  set_gdbarch_data (gdbarch, bsdi_kernel_core_registers_handle,
		    i386bsdi_kernel_core_registers);
  set_gdbarch_data (gdbarch, bsdi_set_curproc_handle, i386bsdi_set_curproc);
  set_gdbarch_data (gdbarch, bsdi_set_procaddr_com_handle,
		    i386bsdi_set_procaddr_com);
}

void
_initialize_i386bsdi_tdep ()
{
  i386_gdbarch_register_os_abi (ELFOSABI_BSDI, 0, i386_check_bsdi_abi,
				i386_init_abi_bsdi);
  add_core_fns (&i386bsdi_core_fns);
}
