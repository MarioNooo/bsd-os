/* Target-dependent definitions for Intel 386 running BSD/OS, for GDB.
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

/* This appears to be required to prevent ugly problems.  */
#define	HAVE_I387_REGS			1

/* Dependencies on other configurations.  */

#include "config/i386/tm-i386v.h"
#include "config/tm-sysv4.h"
#include "config/tm-bsdi.h"

/* The i386 port is still only partially multi-arched.  For BSD/OS,
   let's remove almost all of the non-multi-arched defines.  */

#undef FRAME_CHAIN
#undef FRAME_CHAIN_VALID
#undef FRAME_NUM_ARGS
#undef FRAME_SAVED_PC
#undef FRAME_INIT_SAVED_REGS
#undef GET_LONGJMP_TARGET
#undef IN_SOLIB_CALL_TRAMPOLINE
#undef IS_CALL_INSN
#undef PREPARE_TO_PROCEED
#undef SAVED_PC_AFTER_CALL
#undef SKIP_TRAMPOLINE_CODE

#undef NUM_REGS

/* Handle the current hacked-up situation with GET_LONGJMP_TARGET().  */

#define JB_ELEMENT_SIZE	sizeof (int)
#define JB_PC		0

extern int get_longjmp_target (CORE_ADDR *);

/* Adjustments to the generic configurations.  */

#undef START_INFERIOR_TRAPS_EXPECTED
#define	START_INFERIOR_TRAPS_EXPECTED	2


#ifdef KERNELDEBUG

/* Used to optimize remote text reads with remote-kgdb.  */
#define	NEED_TEXT_START_END		1

extern int bsdi_kernel_debugging;

int i386bsdi_inside_kernstack (CORE_ADDR);
CORE_ADDR i386bsdi_kernel_frame_saved_pc (struct frame_info *);
void i386bsdi_kernel_frame_init_saved_regs (struct frame_info *);

#endif


/* IN_SIGTRAMP() needs to be gdbarchified...  */
extern int i386bsdi_in_sigtramp PARAMS ((CORE_ADDR, char *));

#undef IN_SIGTRAMP
#define	IN_SIGTRAMP(pc, name)		i386bsdi_in_sigtramp (pc, NULL)

#define	SIGCONTEXT_PC_OFFSET		20


/* This feature seems to have been added by someone who didn't realize
   that stabs in ELF shared objects need to be relocated using the
   ".stab.rel" section -- unless you perform the relocation, function
   addresses are zero.  The hack seems to work, however.  (In later
   versions of binutils, the ".stab.rel" section is no longer emitted.) */
#define	SOFUN_ADDRESS_MAYBE_MISSING	1


extern CORE_ADDR i386bsdi_saved_pc_after_call PARAMS ((struct frame_info *));
extern int i386bsdi_call_insn_p PARAMS((CORE_ADDR));
