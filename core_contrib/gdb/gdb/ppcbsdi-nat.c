/* Native debugging support for GDB targets
   running Wind River Systems' BSD/OS on PowerPC.

   Copyright 2003 Free Software Foundation, Inc.

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
#include <machine/reg.h>

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"

void
fetch_inferior_registers (int regno)
{
  struct reg r;
  int i;

  if (regno < FP0_REGNUM || regno > FPLAST_REGNUM)
    {
      if (ptrace (PT_GETREGS, inferior_pid, (caddr_t) &r, 0) == -1)
	error ("Can't fetch general registers from inferior");
      memcpy (&registers[REGISTER_BYTE (0)], &r.r_regs[GP0_REGNUM],
	      4 * (FP0_REGNUM - GP0_REGNUM));
      for (i = GP0_REGNUM; i < FP0_REGNUM; ++i)
	register_valid[i] = 1;
      memcpy (&registers[REGISTER_BYTE (FIRST_UISA_SP_REGNUM)],
	      &r.r_regs[FP0_REGNUM], 
	      4 * (LAST_UISA_SP_REGNUM + 1 - FIRST_UISA_SP_REGNUM));
      for (i = FIRST_UISA_SP_REGNUM; i <= LAST_UISA_SP_REGNUM; ++i)
	register_valid[i] = 1;
    }
  else
    {
      if (ptrace (PT_GETFPREGS, inferior_pid,
		 (caddr_t) &registers[REGISTER_BYTE (FP0_REGNUM)], 0)
	  == -1)
	error ("Can't fetch FP registers from inferior");
      for (i = FP0_REGNUM; i <= FPLAST_REGNUM; ++i)
	register_valid[i] = 1;
    }
}

void
store_inferior_registers (int regno)
{
  struct reg r;

  if (regno < FP0_REGNUM || regno > FPLAST_REGNUM)
    {
      memcpy (&r.r_regs[GP0_REGNUM], &registers[REGISTER_BYTE (0)],
	      4 * (FP0_REGNUM - GP0_REGNUM));
      memcpy (&r.r_regs[FP0_REGNUM],
	      &registers[REGISTER_BYTE (FIRST_UISA_SP_REGNUM)],
	      4 * (LAST_UISA_SP_REGNUM + 1 - FIRST_UISA_SP_REGNUM));
      if (ptrace (PT_SETREGS, inferior_pid, (caddr_t) &r, 0) == -1)
	error ("Can't store general registers into inferior");
    }
  else
    {
      if (ptrace (PT_SETFPREGS, inferior_pid,
		  (caddr_t) &registers[REGISTER_BYTE (FP0_REGNUM)], 0) == -1)
	error ("Can't store FP registers into inferior");
    }
}

int
kernel_u_size ()
{
  return sizeof (struct user);
}

