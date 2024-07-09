/* Machine-dependent kernel debugging support for BSD/OS on IA-32.
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


/* The code below implements kernel debugging of crashdumps (or /dev/kmem)
   or remote systems (via a serial link).  For remote kernels, the remote
   context does most the work, so there is very little to do -- we just
   manage the kernel stack boundaries so we know where to stop a backtrace.

   The crashdump/kmem (kvm) support is a bit more grungy, but thanks to
   libkvm (see bsdi-kcore.c) not too bad.  The main work is kvm_fetch_registers
   which sucks the register state out of the current process's pcb.
   There is a command that lets you set the current process -- hopefully,
   to something that's blocked (in the live kernel case).

   This code currently requires BSD/OS 5.0 or higher.
   Originally modelled on sparcbsd-tdep.c from LBL and
   now (reluctantly) converted to GNU Normal Form.  */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/vmparam.h>
#include <machine/pcpu.h>

#include <kvm.h>
#include <unistd.h>

#include "defs.h"
#include "frame.h"
#include "value.h"
#include "target.h"
#include "gdbcore.h"
#include "inferior.h"

#include "bsdi-kernel.h"

static CORE_ADDR fetch_curproc (void);
static void init_translate_limits (void);
static void kvm_supply_register (int, CORE_ADDR);
static void kvm_fetch_registers (CORE_ADDR);
static CORE_ADDR read_memory_core_addr (CORE_ADDR);

extern kvm_t *bsdi_kd;

static CORE_ADDR masterprocp;
static CORE_ADDR kernbase;
static CORE_ADDR tstart, tend;

/* Pull a pointer out of the image and byte-swap it into host order.  */

static CORE_ADDR
read_memory_core_addr (CORE_ADDR v)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  char *result = alloca (core_addr_len);

  if (target_read_memory (v, result, core_addr_len) != 0)
    error ("cannot read pointer at %#x", v);
  return extract_address (result, (int) core_addr_len);
}

static void
init_translate_limits ()
{
  tstart = bsdi_ksym_lookup ("START_TRAPS");
  tend = bsdi_ksym_lookup ("END_TRAPS");
}

int
i386bsdi_inside_kernel (CORE_ADDR pc)
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
i386bsdi_inside_kernstack (CORE_ADDR addr)
{
  /* Currently, any kernel address works as a kernel stack.  */
  return i386bsdi_inside_kernel (addr);
}

/* Look for the current process.  We look first for the owner of the
   kgdb_lock mutex.  Failing that, we fall back on the current process
   in first_pcpu.  */

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
  static size_t p_addr;
  static size_t pcb_eip;
  static size_t pcb_ebp;
  static size_t pcb_esp0;
  static size_t pcb_eflags;
  static size_t pcb_eax;
  static size_t pcb_ecx;
  static size_t pcb_edx;
  static size_t pcb_ebx;
  static size_t pcb_esi;
  static size_t pcb_edi;
  static size_t pcb_cs;
  static size_t pcb_ss;
  static size_t pcb_ds;
  static size_t pcb_es;
  static size_t pcb_fs;
  static size_t pcb_gs;
  CORE_ADDR pcb;

  if (p_addr == 0)
    {
      p_addr = (size_t) bsdi_ksym_lookup ("P_ADDR");
      pcb_eip = (size_t) bsdi_ksym_lookup ("PCB_EIP");
      pcb_ebp = (size_t) bsdi_ksym_lookup ("PCB_EBP");
      pcb_esp0 = (size_t) bsdi_ksym_lookup ("PCB_ESP0");
      pcb_eflags = (size_t) bsdi_ksym_lookup ("PCB_EFLAGS");
      pcb_eax = (size_t) bsdi_ksym_lookup ("PCB_EAX");
      pcb_ecx = (size_t) bsdi_ksym_lookup ("PCB_ECX");
      pcb_edx = (size_t) bsdi_ksym_lookup ("PCB_EDX");
      pcb_ebx = (size_t) bsdi_ksym_lookup ("PCB_EBX");
      pcb_esi = (size_t) bsdi_ksym_lookup ("PCB_ESI");
      pcb_edi = (size_t) bsdi_ksym_lookup ("PCB_EDI");
      pcb_cs = (size_t) bsdi_ksym_lookup ("PCB_CS");
      pcb_ss = (size_t) bsdi_ksym_lookup ("PCB_SS");
      pcb_ds = (size_t) bsdi_ksym_lookup ("PCB_DS");
      pcb_es = (size_t) bsdi_ksym_lookup ("PCB_ES");
      pcb_fs = (size_t) bsdi_ksym_lookup ("PCB_FS");
      pcb_gs = (size_t) bsdi_ksym_lookup ("PCB_GS");
    }

  if (p_addr == 0)
    error ("Can't retrieve P_ADDR symbol from kernel");

  pcb = read_memory_core_addr (p + p_addr);

  /* Invalidate all the registers, then fill in the ones we know about.  */
  registers_changed ();

  kvm_supply_register (PC_REGNUM, pcb + pcb_eip);
  kvm_supply_register (FP_REGNUM, pcb + pcb_ebp);
  kvm_supply_register (SP_REGNUM, pcb + pcb_esp0);
  kvm_supply_register (PS_REGNUM, pcb + pcb_eflags);

  kvm_supply_register (0, pcb + pcb_eax);
  kvm_supply_register (1, pcb + pcb_ecx);
  kvm_supply_register (2, pcb + pcb_edx);
  kvm_supply_register (3, pcb + pcb_ebx);
  kvm_supply_register (6, pcb + pcb_esi);
  kvm_supply_register (7, pcb + pcb_edi);
  kvm_supply_register (10, pcb + pcb_cs);
  kvm_supply_register (11, pcb + pcb_ss);
  kvm_supply_register (12, pcb + pcb_ds);
  kvm_supply_register (13, pcb + pcb_es);
  kvm_supply_register (14, pcb + pcb_fs);
  kvm_supply_register (15, pcb + pcb_gs);
}

/* Called after the remote kernel has stopped.  Look up the current
   proc, and set up boundaries.  For active kernels only.  */

void
i386bsdi_set_curproc ()
{
  masterprocp = fetch_curproc ();
}

/* Set the process context to that of the proc structure at system
   address paddr.  Read in the register state.  */

static int
i386bsdi_set_procaddr (CORE_ADDR paddr)
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
i386bsdi_set_procaddr_com (char *arg, int from_tty)
{
  CORE_ADDR paddr;

  if (!bsdi_kernel_debugging)
    error ("Not kernel debugging.");

  if (arg == 0)
    i386bsdi_set_procaddr (0);
  else
    {
      paddr = parse_and_eval_address (arg);
      if (i386bsdi_set_procaddr (paddr))
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
i386bsdi_kernel_core_registers (int regno)
{
  /* Need to find current u area to get kernel stack and pcb where
     "panic" saved registers.  (Libkvm also needs to know current u
     area to get user address space mapping).  */
  i386bsdi_set_procaddr (masterprocp);
}

/* Copy memory in and out of a kernel.  We don't currently support
   the mem_attrib feature, although it looks interesting.  */

int
i386bsdi_kernel_core_xfer_memory (CORE_ADDR addr, char *cp, int len, int write,
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
	i386bsdi_set_procaddr (0);

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
i386bsdi_kernel_frame_saved_pc (struct frame_info *fr)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  static size_t tf_eip;
  CORE_ADDR pc;

  if (tstart == 0)
    init_translate_limits ();
  if (tf_eip == 0)
    tf_eip = (size_t) bsdi_ksym_lookup ("TF_EIP");
  pc = read_memory_core_addr (fr->frame + core_addr_len);
  if (tstart <= pc && pc < tend)
    {
      CORE_ADDR tfr = fr->frame + 2 * core_addr_len;
      return read_memory_core_addr (tfr + tf_eip);
    }
  return pc;
}

void
i386bsdi_kernel_frame_init_saved_regs (struct frame_info *fr)
{
  const size_t core_addr_len = TARGET_ADDR_BIT / TARGET_CHAR_BIT;
  static size_t tf_eflags;
  static size_t tf_eax;
  static size_t tf_ecx;
  static size_t tf_edx;
  static size_t tf_ebx;
  static size_t tf_esi;
  static size_t tf_edi;
  static size_t tf_esp;
  CORE_ADDR pc;

  if (tstart == 0)
    init_translate_limits ();

  if (tf_eflags == 0)
    {
      tf_eflags = (size_t) bsdi_ksym_lookup ("TF_EFLAGS");
      tf_eax = (size_t) bsdi_ksym_lookup ("TF_EAX");
      tf_ecx = (size_t) bsdi_ksym_lookup ("TF_ECX");
      tf_edx = (size_t) bsdi_ksym_lookup ("TF_EDX");
      tf_ebx = (size_t) bsdi_ksym_lookup ("TF_EBX");
      tf_esi = (size_t) bsdi_ksym_lookup ("TF_ESI");
      tf_edi = (size_t) bsdi_ksym_lookup ("TF_EDI");
      tf_esp = (size_t) bsdi_ksym_lookup ("TF_ESP");
    }

  pc = read_memory_core_addr (fr->frame + core_addr_len);
  if (tstart <= pc && pc < tend)
    {
      CORE_ADDR tfr = fr->frame + 2 * core_addr_len;

      if (fr->saved_regs == NULL)
	frame_saved_regs_zalloc (fr);

      fr->saved_regs[PC_REGNUM] = fr->frame + core_addr_len;
      fr->saved_regs[FP_REGNUM] = fr->frame;

      /* Hack:  We assume that we trapped from kernel mode and
	 that the hardware did not push ESP and SS. 
	 Hence the previous stack frame starts where
	 we would otherwise expect to see the saved ESP and SS.  */
      fr->saved_regs[SP_REGNUM] = tfr + tf_esp;

      fr->saved_regs[PS_REGNUM] = tfr + tf_eflags;
      fr->saved_regs[0] = tfr + tf_eax;
      fr->saved_regs[1] = tfr + tf_ecx;
      fr->saved_regs[2] = tfr + tf_edx;
      fr->saved_regs[3] = tfr + tf_ebx;
      fr->saved_regs[6] = tfr + tf_esi;
      fr->saved_regs[7] = tfr + tf_edi;
      return;
    }

  i386_frame_init_saved_regs (fr);
}
