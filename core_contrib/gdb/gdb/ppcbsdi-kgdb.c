/* PowerPC target-specific KGDB protocol support.
   Copyright (C) 1988, 1989, 1991, 2003 Free Software Foundation, Inc.

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
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <vm/vm.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/vmparam.h>

#include <kvm.h>

#include "defs.h"
#include "frame.h"
#include "value.h"
#include "target.h"
#include "gdbcore.h"

#include "bsdi-kernel.h"

static CORE_ADDR fetch_curproc (void);
static void kvm_supply_register (int, CORE_ADDR);
static void kvm_fetch_registers (CORE_ADDR);
static CORE_ADDR read_memory_core_addr (CORE_ADDR);

extern kvm_t *bsdi_kd;

static CORE_ADDR leave_kernel;
static CORE_ADDR kernbase;
static CORE_ADDR masterprocp;
static size_t ef_srr0;
static size_t ef_lr;

/* Pull a pointer out of the image and byte-swap it into host order.  */

static CORE_ADDR
read_memory_core_addr(CORE_ADDR v)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  char *result = alloca (core_addr_len);

  if (target_read_memory(v, result, core_addr_len) != 0)
	  error("cannot read pointer at %#x", v);
  return extract_address (result, (int) core_addr_len);
}

/* The code below implements kernel debugging of crashdumps (or /dev/kmem)
   or remote systems (via a serial link).  For remote kernels, the remote
   context does most the work, so there is very little to do -- we just
   manage the kernel stack boundaries so we know where to stop a backtrace.
  
   The crashdump/kmem (kvm) support is a bit more grungy, but thanks to
   libkvm (see bsdi-kcore.c) not too bad.  The main work is kvm_fetch_registers
   which sucks the register state out of the current process's pcb.
   There is a command that lets you set the current process -- hopefully,
   to something that's blocked (in the live kernel case).  */

int
ppcbsdi_inside_kernel (CORE_ADDR pc)
{
  static CORE_ADDR vm_maxuser_address;

  if (vm_maxuser_address == 0)
    vm_maxuser_address
      = bsdi_ksym_lookup_default (NL_VM_MAXUSER_ADDRESS, __VM_MAXUSER_ADDRESS);

  return pc >= vm_maxuser_address;
}

/* Return true if ADDR is a valid stack address according to the
   current boundaries (which are determined by the currently running 
   user process).  */

int
ppcbsdi_inside_kernstack(CORE_ADDR addr)
{
  /* Currently, any kernel address works as a kernel stack.  */
  return inside_kernel (addr);
}

/* Look for the current process.  We look first for the owner of
   the kgdb_lock mutex.  Failing that, we fall back on the current
   process in first_pcpu.  */

static CORE_ADDR
fetch_curproc ()
{
  static CORE_ADDR kgdb_lock;
  static CORE_ADDR dumpproc;
  static CORE_ADDR proc0;
  static size_t mtx_lock;
  static size_t e_paddr;
  CORE_ADDR p;
  void *kipp;
  int count;

  if (proc0 == NULL)
    {
      kgdb_lock = bsdi_ksym_lookup_default ("kgdb_lock", 0);
      dumpproc = bsdi_ksym_lookup ("dumpproc");
      proc0 = bsdi_ksym_lookup ("proc0");
      mtx_lock = (size_t) bsdi_ksym_lookup ("MTX_LOCK");
      e_paddr = (size_t) bsdi_ksym_lookup ("KP_EPROC_E_PADDR");
    }

  if (bsdi_kd == NULL && kgdb_lock != 0)
    {
      /* A remote kernel.  By default, use the owner of the kgdb_lock.  */
      p = read_memory_core_addr (kgdb_lock + mtx_lock);
      p &= ~0x3;	/* XXX clear MTX_FLAGMASK */
      if (p < proc0)	/* XXX hack for non-zero MTX_UNOWNED */
	p = 0;
    }
  else
    {
      /* The address of the proc that generated a crash dump is
	 recorded in 'dumpproc'.  */
      p = read_memory_core_addr (dumpproc);
      if (p == 0)
	{
	  /* Live debugging.  Use kvm_getprocs() to retrieve our
	     own proc address.  */
	  kipp = kvm_getprocs (bsdi_kd, KERN_PROC_PID, getpid (), &count);
	  if (kipp != NULL && count == 1)
	    p = *(CORE_ADDR *) ((char *) kipp + e_paddr);
	}
    }

  /* If no one owns the kgdb_lock, use process 0.  */
  if (p == 0)
    p = proc0;

  return p;
}

static void
kvm_supply_register (int reg, CORE_ADDR v)
{
  char result[MAX_REGISTER_RAW_SIZE];

  target_read_memory (v, result, REGISTER_RAW_SIZE (reg));
  supply_register (reg, result);
}

/* Fetch registers from a crashdump or /dev/kmem.  */

static void
kvm_fetch_registers (CORE_ADDR p)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  static size_t p_addr;
  static size_t kpcb_sp;
  static size_t kpcb_lr;
  static size_t kpcb_cr;
  static size_t kpcb_gpr;
  CORE_ADDR pcb;
  CORE_ADDR sp;
  CORE_ADDR saved_lr;
  int i;

  if (p_addr == 0)
    {
      p_addr = (size_t) bsdi_ksym_lookup ("P_ADDR");
      kpcb_sp = (size_t) bsdi_ksym_lookup ("PCB_SP");
      kpcb_lr = (size_t) bsdi_ksym_lookup ("PCB_LR");
      kpcb_cr = (size_t) bsdi_ksym_lookup ("PCB_CR");
      kpcb_gpr = (size_t) bsdi_ksym_lookup ("PCB_GPR");
    }

  if (p_addr == 0)
    error ("Can't retrieve P_ADDR symbol from kernel");

  pcb = read_memory_core_addr (p + p_addr);

  /* Invalidate all the registers then fill in the ones we know about.  */
  registers_changed ();

  sp = read_memory_core_addr (pcb + kpcb_sp);
  saved_lr = read_memory_core_addr (sp + core_addr_len);

  kvm_supply_register (PC_REGNUM, pcb + kpcb_lr);
  kvm_supply_register (CR_REGNUM, pcb + kpcb_cr);
  kvm_supply_register (LR_REGNUM, saved_lr);
  kvm_supply_register (SP_REGNUM, sp);

  for (i = 0; i < 18; ++i)
    kvm_supply_register (GP0_REGNUM + i + 14,
			 pcb + kpcb_gpr + i * core_addr_len);
}

/* Called after the remote kernel has stopped.  Look up the current
   proc, and set up boundaries.  For active kernels only.  */

void
ppcbsdi_set_curproc ()
{
  masterprocp = fetch_curproc ();
}

/* Set the process context to that of the proc structure at system
   address paddr.  Read in the register state.  */

static int
ppcbsdi_set_procaddr (CORE_ADDR paddr)
{
  if (kernbase == 0)
    kernbase = bsdi_ksym_lookup_default (NL_KERNBASE, __KERNBASE);

  if (paddr == 0)
    masterprocp = fetch_curproc ();
  else if (paddr != masterprocp)
    {
      /* Eliminate the common mistake of pid instead of proc ptr.  */
      if (paddr < kernbase)
	return 1;
      masterprocp = paddr;
    }
  kvm_fetch_registers (masterprocp);
  return 0;
}

/* XXX This code doesn't work for remote kernels.
   XXX The 'process-address' command is a botch;
   we should replace it with a kernel thread interface.  */

void
ppcbsdi_set_procaddr_com (char *arg, int from_tty)
{
  CORE_ADDR paddr;

  if (!bsdi_kernel_debugging)
    error ("Not kernel debugging.");

  if (arg == 0)
    ppcbsdi_set_procaddr (0);
  else
    {
      paddr = parse_and_eval_address (arg);
      if (ppcbsdi_set_procaddr (paddr))
	error ("Invalid proc address");
      flush_cached_frames ();
      set_current_frame (create_new_frame (read_register (FP_REGNUM),
					   read_pc ()));
      select_frame (get_current_frame (), 0);
    }
}

/* Get the registers out of a crashdump or /dev/kmem.
   We just get all the registers, so we don't use regno.  */

void
ppcbsdi_kernel_core_registers (int regno)
{
  /* Need to find current u area to get kernel stack and pcb where
     "panic" saved registers.  (Libkvm also needs to know current u
     area to get user address space mapping).  */
  ppcbsdi_set_procaddr (masterprocp);
}

/* Copy memory in and out of a kernel.  We don't currently support
   the mem_attrib feature, although it looks interesting.  */

int
ppcbsdi_kernel_xfer_memory (CORE_ADDR addr, char *cp, int len, int write,
     struct mem_attrib *ignored, struct target_ops *target)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  static size_t p_vmspace;
  char fakeproc[1024];		/* XXX a guess at the size */

  if (write)
    return kvm_write (bsdi_kd, addr, cp, len);

  if (kernbase == 0)
    kernbase = bsdi_ksym_lookup_default (NL_KERNBASE, __KERNBASE);
  if (p_vmspace == 0)
    p_vmspace = (size_t) bsdi_ksym_lookup ("P_VMSPACE");

  if (addr < kernbase)
    {
      if (masterprocp == 0)
	ppcbsdi_set_procaddr (0);

      /* Hack: the only part of the proc structure that we currently
	 use in kvm_uread() is the p_vmspace field, so that's all
	 that we actually fill in.  Here, we use the kernel's field
	 offset with the installed libkvm and hope that it does the
	 right thing!   Note that we use target byte order.  */
      if (target_read_memory (masterprocp + p_vmspace, &fakeproc[p_vmspace],
			      core_addr_len)
	  != 0)
	error ("cannot read pointer at %#x", masterprocp + p_vmspace);
      if (extract_address (&fakeproc[p_vmspace], (int) core_addr_len) == 0)
	return 0;
      return kvm_uread (bsdi_kd, (struct proc *) &fakeproc[0], addr, cp, len);
    }
  return kvm_read (bsdi_kd, addr, cp, len);
}

CORE_ADDR
ppcbsdi_kernel_frame_saved_pc (struct frame_info *fr)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  struct rs6000_framedata rf;
  CORE_ADDR v;
  CORE_ADDR saved_pc;

  if (ef_srr0 == 0)
    {
      ef_srr0 = bsdi_ksym_lookup ("EF_IP");
      ef_lr = bsdi_ksym_lookup ("EF_LR");
    }
  if (leave_kernel == 0)
    leave_kernel = bsdi_ksym_lookup ("leave_kernel");

  /* All exception vectors call their exception handlers
     with LR set to leave_kernel.  If we see leave_kernel
     as a saved LR, it means that a function took an exception.
     We recover the PC from SRR0 in the exception_frame.  */
  if (rs6000_frame_saved_pc (fr) == leave_kernel)
    {
      v = ppcbsdi_frame_chain (fr) + 4 * core_addr_len;
      return read_memory_core_addr (v + ef_srr0);
    }

  /* If the next frame down represents a function that
     took an exception, and that function didn't save LR,
     look for the LR value in the exception_frame instead.  */
  if (fr->next != NULL && ppcbsdi_frame_saved_pc (fr->next) == leave_kernel)
    {
      v = get_pc_function_start (fr->pc) + FUNCTION_START_OFFSET;
      skip_prologue (v, &rf);
      if (rf.nosavedpc)
	{
	  v = ppcbsdi_frame_chain (fr->next) + 4 * core_addr_len;
	  return read_memory_core_addr (v + ef_lr);
	}
    }

  return rs6000_frame_saved_pc (fr);
}

void
ppcbsdi_kernel_init_frame_pc_first (int leaf, struct frame_info *fr)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  struct rs6000_framedata rf;
  CORE_ADDR v;

  if (fr->next == 0)
    {
      fr->pc = read_pc ();
      return;
    }

  if (ef_srr0 == 0)
    {
      ef_srr0 = bsdi_ksym_lookup ("EF_IP");
      ef_lr = bsdi_ksym_lookup ("EF_LR");
    }
  if (leave_kernel == 0)
    leave_kernel = bsdi_ksym_lookup ("leave_kernel");

  /* All exception vectors call their exception handlers
     with LR set to leave_kernel.  If we see leave_kernel
     as a saved LR, it means that a function took an exception.
     We recover the PC from SRR0 in the exception_frame.  */
  if (ppcbsdi_frame_saved_pc (fr->next) == leave_kernel)
    {
      v = ppcbsdi_frame_chain (fr->next) + 4 * core_addr_len;
      fr->pc = read_memory_core_addr (v + ef_srr0);
      return;
    }

  /* If the next frame down represents a function that
     took an exception, and that function didn't save LR,
     look for the LR value in the exception_frame instead.  */
  if (fr->next->next != NULL
      && ppcbsdi_frame_saved_pc (fr->next->next) == leave_kernel)
    {
      v = get_pc_function_start (fr->next->pc) + FUNCTION_START_OFFSET;
      skip_prologue (v, &rf);
      if (rf.nosavedpc)
	{
	  v = ppcbsdi_frame_chain (fr->next->next) + 4 * core_addr_len;
	  fr->pc = read_memory_core_addr (v + ef_lr);
	  return;
	}
    }

  if (leaf)
    fr->pc = SAVED_PC_AFTER_CALL (fr->next);
  else
    fr->pc = FRAME_SAVED_PC (fr->next);
}

void
ppcbsdi_kernel_frame_init_saved_regs (struct frame_info *frame)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  static size_t ef_cr;
  static size_t ef_ctr;
  static size_t ef_xer;
  static size_t ef_gpr0;

  if (ef_srr0 == 0)
    {
      ef_srr0 = bsdi_ksym_lookup ("EF_IP");
      ef_lr = bsdi_ksym_lookup ("EF_LR");
    }
  if (leave_kernel == 0)
    leave_kernel = bsdi_ksym_lookup ("leave_kernel");
  if (ef_cr == 0)
    {
      ef_cr = bsdi_ksym_lookup ("EF_CR");
      ef_ctr = bsdi_ksym_lookup ("EF_CTR");
      ef_xer = bsdi_ksym_lookup ("EF_XER");
      ef_gpr0 = bsdi_ksym_lookup ("EF_GPR0");
    }

  if (rs6000_frame_saved_pc (fr) == leave_kernel)
    {
      CORE_ADDR tfr = fr->frame + 4 * core_addr_len;
      int i;

      for (i = 0; i < 32; ++i)
	fr->saved_regs[tdep->ppc_gp0_regnum + i]
	  = tfr + ef_gpr0 + tdep->wordsize * i;

      /* FIXME?  We don't get a ppc_pc_regnum offset...  */
      fr->saved_regs[FIRST_UISA_SP_REGNUM] = tfr + ef_srr0;
      fr->saved_regs[tdep->ppc_ps_regnum] = tfr + ef_srr1;
      fr->saved_regs[tdep->ppc_cr_regnum] = tfr + ef_cr;
      fr->saved_regs[tdep->ppc_lr_regnum] = tfr + ef_lr;
      fr->saved_regs[tdep->ppc_ctr_regnum] = tfr + ef_ctr;
      fr->saved_regs[tdep->ppc_xer_regnum] = tfr + ef_xer;
    }
  else
    rs6000_frame_init_saved_regs (frame);
}
