/* Target dependent code for Wind River Systems' BSD/OS on PowerPC.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1997, 2003
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

#include "defs.h"
#include "frame.h"
#include "gdbcore.h"
#include "target.h"
#include "value.h"

static void fetch_core_registers (char *, unsigned, int, CORE_ADDR);
static int ppcbsdi_attach_thread_event (CORE_ADDR *);
static int ppcbsdi_core_thread_event (CORE_ADDR *);
static int ppcbsdi_frame_chain_valid (CORE_ADDR, struct frame_info *);
static void ppcbsdi_frame_init_saved_regs (struct frame_info *);
static CORE_ADDR ppcbsdi_frame_saved_pc (struct frame_info *);
static void ppcbsdi_init_frame_pc_first (int, struct frame_info *);
static int ppcbsdi_is_call_insn (CORE_ADDR);
static int ppcbsdi_thread_event (CORE_ADDR *);
static void ppcbsdi_thread_fetch_registers (struct pthread_cache *);
static void ppcbsdi_thread_resume (void);
static void ppcbsdi_thread_store_registers (struct pthread_cache *);
static void process_ppcbsdi_abi_tag_sections (bfd *, asection *, void *);
static boolean rs6000_check_bsdi_abi (struct rs6000_abi_handler *, bfd *);
static struct void rs6000_init_abi_bsdi (struct gdbarch_info, struct gdbarch *);

/* Thread support.
   XXX We need to remove dependencies on pthread_var.h.
   XXX We need to remove assumptions that host = target.  */

/* For each GDB register, we list the equivalent thread register offset.
   We only save callee-saved registers, so some GDB registers are omitted.  */

static int thread_reg_map[] =
{
  -1,
  1 * 4,	/* _THREAD_POWERPC_SP */
  2 * 4,	/* _THREAD_POWERPC_TOC */
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  3 * 4,	/* _THREAD_POWERPC_G_13 */
  4 * 4,	/* _THREAD_POWERPC_G_14 */
  5 * 4,	/* _THREAD_POWERPC_G_15 */
  6 * 4,	/* _THREAD_POWERPC_G_16 */
  7 * 4,	/* _THREAD_POWERPC_G_17 */
  8 * 4,	/* _THREAD_POWERPC_G_18 */
  9 * 4,	/* _THREAD_POWERPC_G_19 */
  10 * 4,	/* _THREAD_POWERPC_G_20 */
  11 * 4,	/* _THREAD_POWERPC_G_21 */
  12 * 4,	/* _THREAD_POWERPC_G_22 */
  13 * 4,	/* _THREAD_POWERPC_G_23 */
  14 * 4,	/* _THREAD_POWERPC_G_24 */
  15 * 4,	/* _THREAD_POWERPC_G_25 */
  16 * 4,	/* _THREAD_POWERPC_G_26 */
  17 * 4,	/* _THREAD_POWERPC_G_27 */
  18 * 4,	/* _THREAD_POWERPC_G_28 */
  19 * 4,	/* _THREAD_POWERPC_G_29 */
  20 * 4,	/* _THREAD_POWERPC_G_30 */
  21 * 4,	/* _THREAD_POWERPC_G_31 */
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  0 * 8,	/* _THREAD_POWERPC_FP_14 */
  1 * 8,	/* _THREAD_POWERPC_FP_15 */
  2 * 8,	/* _THREAD_POWERPC_FP_16 */
  3 * 8,	/* _THREAD_POWERPC_FP_17 */
  4 * 8,	/* _THREAD_POWERPC_FP_18 */
  5 * 8,	/* _THREAD_POWERPC_FP_19 */
  6 * 8,	/* _THREAD_POWERPC_FP_20 */
  7 * 8,	/* _THREAD_POWERPC_FP_21 */
  8 * 8,	/* _THREAD_POWERPC_FP_22 */
  9 * 8,	/* _THREAD_POWERPC_FP_23 */
  10 * 8,	/* _THREAD_POWERPC_FP_24 */
  11 * 8,	/* _THREAD_POWERPC_FP_25 */
  12 * 8,	/* _THREAD_POWERPC_FP_26 */
  13 * 8,	/* _THREAD_POWERPC_FP_27 */
  14 * 8,	/* _THREAD_POWERPC_FP_28 */
  15 * 8,	/* _THREAD_POWERPC_FP_29 */
  16 * 8,	/* _THREAD_POWERPC_FP_30 */
  17 * 8,	/* _THREAD_POWERPC_FP_31 */
  -1,
  -1,
  22 * 4	/* _THREAD_POWERPC_CR */
  0 * 4,	/* _THREAD_POWERPC_LR */
  -1,
  -1,
  -1
};

struct ppcbsdi_thread_machdep_state
{
  char regs[24 * 4];
  char saved_fp[19 * 8];
};

/* Retrieve thread register state and convert it into GDB register state.
   XXX This code should NOT use native pthread headers.
   XXX This code should NOT assume that host == target.  */

static void
ppcbsdi_thread_fetch_registers (struct pthread_cache *ptc)
{
  struct ppcbsdi_thread_machdep_state *t = (void *) &ptc->pthread.saved_context;
  size_t i;

  for (i = 0; i < sizeof thread_reg_map / sizeof (int); ++i)
    {
      if (thread_reg_map[i] < 0)
	continue;
      if (i >= FP0_REGNUM && i <= FPLAST_REGNUM)
	supply_register (i, &t->saved_fp[thread_reg_map[i]]);
      else
	supply_register (i, &t->regs[thread_reg_map[i]]);
    }
}

/* Convert GDB's register state back into thread register state.
   XXX The same issues mentioned for fetching registers apply here.  */

static void
ppcbsdi_thread_store_registers (struct pthread_cache *ptc)
{
  struct ppcbsdi_thread_machdep_state *t = (void *) &ptc->pthread.saved_context;
  struct ppcbsdi_thread_machdep_state *xt = (void *) &ptc->id->saved_context;
  int ret;
  size_t i;

  for (i = 0; i < sizeof thread_reg_map / sizeof (int); ++i)
    {
      if (thread_reg_map[i] < 0)
	continue;
      if (i >= FP0_REGNUM && i <= FPLAST_REGNUM)
	memcpy (&t->saved_fp[thread_reg_map[i]], &registers[REGISTER_BYTE (i)],
		REGISTER_RAW_SIZE (i));
      else
	memcpy (&t->regs[thread_reg_map[i]], &registers[REGISTER_BYTE (i)],
		REGISTER_RAW_SIZE (i));
    }

  ret = child_ops.to_xfer_memory ((CORE_ADDR) &xt->regs[0], &t->regs[0],
				  sizeof t->regs, 1, NULL);
  if (ret != sizeof t->regs)
    error ("ppcbsdi_thread_store_registers: store failed");

  ret = child_ops.to_xfer_memory ((CORE_ADDR) &xt->saved_fp[0], &t->saved_fp[0],
				  sizeof t->saved_fp, 1, NULL);
  if (ret != sizeof t->saved_fp)
    error ("ppcbsdi_thread_store_registers: store failed");
}

/* Check for a thread event during native debugging.
   A thread event occurs when the child calls ptrace(PT_EVENT, ...,
   PT_EVT_THREAD_HOOK) to deliver the address of a data
   structure that we use to recover user thread data.
   The system automatically sends a SIGTRAP if a process
   makes this system call and it is being debugged.  */

#define	SYSC_INSN		0x44000002

static enum event_state { INVALID, EVENT, NON_EVENT } event_state = INVALID;

static int
ppcbsdi_thread_event (CORE_ADDR *v)
{
  static CORE_ADDR ptrace_event;
  int r[NUM_REGS];
  int ip;
  int insn;

  switch (event_state)
    {
    case INVALID:
      break;
    case EVENT:
      if (v != NULL)
	*v = ptrace_event;
      return 1;
    case NON_EVENT:
      return 0;
    }

  event_state = NON_EVENT;

  /* Does r0 contain the right syscall number?  */
  if (ptrace (PT_GETREGS, inferior_pid, (caddr_t) r, 0) == -1)
    error ("Can't get registers for thread event");
  if (r[0] != SYS_ptrace)
    return 0;

#if 0 /* XXX We currently stomp on r3 before gdb can look at it.  */
  /* Was this a PT_EVENT request?  */
  if (r[3] != PT_EVENT)
	  return 0;
#endif

  /* Was this a syscall trap?  */
  ip = r[REGISTER_BYTE (PC_REGNUM) / sizeof r[0]];
  insn = ptrace (PT_READ_D, inferior_pid, (caddr_t) (ip - 4), 0);
  if (insn == -1)
    error ("Can't read trapping instruction for thread creation check");
  if (insn != SYSC_INSN)
    return 0;

  /* The ptrace event is in r5.  */
  ptrace_event = (CORE_ADDR) r[5];
  if (v != NULL)
    *v = ptrace_event;

  event_state = EVENT;
  return 1;
}

/* Dig out the ptrace event after attaching to a process.
   Here we assume native debugging.  */

static int
ppcbsdi_attach_thread_event (CORE_ADDR *v)
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
ppcbsdi_core_thread_event (CORE_ADDR *v)
{
  asection *sect;

  if (core_bfd == NULL)
    return 0;

  sect = bfd_get_section_by_name (core_bfd, "note0");
  if (sect == NULL)
    return 0;

  *v = (CORE_ADDR) bfd_get_32 (abfd, sect->filepos + sect->_raw_size - 4);
  return 1;
}

/* Take action when resuming a thread.  A no-op on IA-32.  */

static void
ppcbsdi_thread_resume ()
{
  event_state = INVALID;
}

/* Architecture-dependent operations on BSD/OS threads.  We copy
   these to 'bsdi_arch_thread_ops', and the architecture-independent
   code in bsdi-thread.c uses the ops to recover machine state.  */

struct bsdi_arch_thread_ops
{
  ppcbsdi_thread_fetch_registers,
  ppcbsdi_thread_store_registers,
  ppcbsdi_thread_event,
  ppcbsdi_attach_thread_event,
  ppcbsdi_core_thread_event,
  ppcbsdi_thread_resume
} ppcbsdi_arch_thread_ops;


/* Core file ops.  */

static void
fetch_core_registers (char *data, unsigned size, int which, CORE_ADDR reg_addr)
{
  int i;

  switch (which)
    {
    case 0:
      for (i = 0; i < FP0_REGNUM; ++i)
	supply_register (i, &data[REGISTER_BYTE (i)]);
      for (i = FIRST_UISA_SP_REGNUM; i <= LAST_UISA_SP_REGNUM; ++i)
	supply_register (i, &data[REGISTER_BYTE (FP0_REGNUM)
				  + REGISTER_BYTE (i)
				  - REGISTER_BYTE (FIRST_UISA_SP_REGNUM)]);
      break;

    case 2:
      for (i = 0; i < FPLAST_REGNUM + 1 - FP0_REGNUM; ++i)
	supply_register (FP0_REGNUM + i,
		         &data[REGISTER_BYTE (FP0_REGNUM + i)
			       - REGISTER_BYTE (FP0_REGNUM)]);
      break;

    default:
      break;
  }
}

static struct core_fns ppcbsdi_core_fns =
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
ppcbsdi_svr4_fetch_link_map_offsets (void)
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


/* The rs6000_frame_chain() code stops when it reaches the entry file.
   This can break if symbols are stripped.  We rely on null chain pointers.  */

CORE_ADDR
ppcbsdi_frame_chain (struct frame_info *frame)
{
  CORE_ADDR fp, fpp, lr;
  int wordsize = gdbarch_tdep (current_gdbarch)->wordsize;

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    return frame->frame;	/* dummy frame same as caller's frame */

  if (frame->signal_handler_caller)
    fp = read_memory_unsigned_integer (frame->frame + SIG_FRAME_FP_OFFSET,
			               wordsize);
  else if (frame->next != NULL
	   && frame->next->signal_handler_caller
	   && FRAMELESS_FUNCTION_INVOCATION (frame))
    /* A frameless function interrupted by a signal did not change the
       frame pointer.  */
    fp = FRAME_FP (frame);
  else
    fp = read_memory_unsigned_integer (frame->frame, wordsize);

  lr = read_register (gdbarch_tdep (current_gdbarch)->ppc_lr_regnum);
  if (lr == entry_point_address ())
    if (fp != 0 && (fpp = read_memory_unsigned_integer (fp, wordsize)) != 0)
      if (PC_IN_CALL_DUMMY (lr, fpp, fpp))
	return fpp;

  return fp;
}

static int
ppcbsdi_frame_chain_valid (CORE_ADDR chain, struct frame_info *frame)
{
  if (bsdi_kernel_debugging)
    return ppcbsdi_inside_kernstack (chain);
  return chain != 0;
}

static CORE_ADDR
ppcbsdi_frame_saved_pc (struct frame_info *frame)
{
  if (bsdi_kernel_debugging)
    return ppcbsdi_kernel_frame_saved_pc (frame);
  else
    return rs6000_frame_saved_pc (frame);
}

static void
ppcbsdi_init_frame_pc_first (int leaf, struct frame_info *fr)
{
  if (bsdi_kernel_debugging)
    {
      ppcbsdi_kernel_init_frame_pc_first (leaf, fr);
      return;
    }

  if (fr->next == 0)
    {
      fr->pc = read_pc ();
      return;
    }

  if (leaf)
    fr->pc = SAVED_PC_AFTER_CALL (fr->next);
  else
    fr->pc = FRAME_SAVED_PC (fr->next);
}

static void
ppcbsdi_frame_init_saved_regs (struct frame_info *frame)
{
  if (bsdi_kernel_debugging)
    ppcbsdi_kernel_frame_init_saved_regs (frame);
  else
    rs6000_frame_init_saved_regs (frame);
}

int
ppcbsdi_in_sigtramp (CORE_ADDR pc, char *name)
{
  if (name != NULL)
    return STREQ (name, "_sigtramp");
  return false;
}

#define	BL_MASK		0xfc000001
#define	BL_INSN		0x48000001
#define	BL_ADDR		0x07fffffc

static int
ppcbsdi_is_call_insn (CORE_ADDR pc)
{
  unsigned int insn;

  insn = read_memory_unsigned_integer (pc, 4);
  if ((insn & BL_MASK) != BL_INSN)
    return false;

  if ((insn & BL_ADDR) == 4)
    /* Part of loading the GOT pointer.  */
    return false;

  return true;
}

/* See whether the given ELF note section is a BSD/OS ABI tag.
   We're given a reference to an rs6000_abi_handler pointer;
   we clear the pointer on success.  */

static void
process_ppcbsdi_abi_tag_sections (bfd *abfd, asection *sect, void *obj)
{
  struct rs6000_abi_handler **handlerp = obj;
  const char *name;
  unsigned int sect_size;

  /* When we find a matching section, we set *handlerp = NULL.  */
  if (*handlerp == NULL)
    return;

  name = bfd_get_section_name (abfd, sect);
  sect_size = bfd_section_size (abfd, sect);
  if (strcmp (name, ".note.ABI-tag") == 0 && sect_size > 0)
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

/* Given an ELF PowerPC executable, return true if it's a BSD/OS executable.  */

static boolean
rs6000_check_bsdi_abi (struct rs6000_abi_handler *handler, bfd *abfd)
{
  if (elf_elfheader (abfd)->e_ident[EI_OSABI] != ELFOSABI_NONE)
    return false;

  bfd_map_over_sections (abfd, process_ppcbsdi_abi_tag_sections, &handler);

  /* A null handler indicates success.  */
  return handler == NULL;
}

/* Set up the ppcbsdi gdbarch methods.  We register ourselves as
   an OS ABI for the rs6000 architecture, and the rs6000_gdbarch_init()
   function calls us after performing the normal initialization.  */

static struct void
rs6000_init_abi_bsdi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_convert_from_func_ptr_addr (gdbarch, core_addr_identity);
  set_gdbarch_frame_chain (gdbarch, ppcbsdi_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, ppcbsdi_frame_chain_valid);
  set_gdbarch_frame_init_saved_regs (gdbarch, ppcbsdi_frame_init_saved_regs);
  set_gdbarch_frame_saved_pc (gdbarch, ppcbsdi_frame_saved_pc);
  set_gdbarch_init_frame_pc_first (gdbarch, ppcbsdi_init_frame_pc_first);
  set_gdbarch_is_call_insn (gdbarch, ppcbsdi_is_call_insn);

  set_gdbarch_skip_trampoline_code (gdbarch, find_bsdi_solib_trampoline_target);

  set_gdbarch_data (gdbarch, bsdi_arch_thread_ops_handle,
		    &ppcbsdi_arch_thread_ops);

  /* Set up the shared library hack.  We do this here so that we can
     override any value set in an _initialize*() function in an
     solib-*.c file.  There surely is a better way to do this...  */
  current_target_so_ops = &generic_bsdi_so_ops;

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 ppcbsdi_svr4_fetch_link_map_offsets);

  set_gdbarch_data (gdbarch, bsdi_kernel_core_xfer_memory_handle,
		    ppcbsdi_kernel_core_xfer_memory);
  set_gdbarch_data (gdbarch, bsdi_kernel_core_registers_handle,
		    ppcbsdi_kernel_core_registers);
  set_gdbarch_data (gdbarch, bsdi_set_curproc_handle, ppcbsdi_set_curproc);
  set_gdbarch_data (gdbarch, bsdi_set_procaddr_com_handle,
		    ppcbsdi_set_procaddr_com);
}

void
_initialize_ppcbsdi_tdep ()
{
  rs6000_gdbarch_register_os_abi (ELFOSABI_BSDI, 0, rs6000_check_bsdi_abi,
				  rs6000_init_abi_bsdi);
  add_core_fns (&ppcbsdi_core_fns);
}
