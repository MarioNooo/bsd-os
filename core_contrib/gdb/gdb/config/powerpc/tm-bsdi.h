/*	BSDI tm-bsdi.h,v 1.1 2003/02/02 02:00:47 donn Exp	*/

/* Macro definitions for Power PC running BSD/OS.
   Copyright 1995 Free Software Foundation, Inc.

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

/* Cheerfully stolen from tm-ppc-eabi.h */

#ifndef TM_PPC_BSDI_H
#define TM_PPC_BSDI_H

/* Use generic RS6000 definitions. */
#include "rs6000/tm-rs6000.h"

#include "config/tm-bsdi.h"

#undef	DEFAULT_LR_SAVE
#define	DEFAULT_LR_SAVE 4	/* eabi saves LR at 4 off of SP */

/* Support for longjmp() breakpoints.  */
#define	JB_ELEMENT_SIZE	sizeof(int)
#define	JB_LR	1
#define	JB_PC	JB_LR

extern int
get_longjmp_target PARAMS ((CORE_ADDR *));

#define	GET_LONGJMP_TARGET(ADDR)	get_longjmp_target(ADDR)

#define	nounderscore			1

#define GDB_TARGET_POWERPC

#undef PC_LOAD_SEGMENT
#undef PROCESS_LINENUMBER_HOOK
#undef CHILD_SPECIAL_WAITSTATUS
#undef CONVERT_FROM_FUNC_PTR_ADDR
#undef NO_SINGLE_STEP

/* XXX ??? */
#undef TEXT_SEGMENT_BASE
#define TEXT_SEGMENT_BASE	0x00010000

#undef STACK_END_ADDR
#define	STACK_END_ADDR		0xe0000000

#ifdef KERNELDEBUG

#define	MEM_DEVICE			2
#define	KERNEL_XFER_MEMORY

extern int kernel_debugging;
extern int inside_kernel();

int kernel_xfer_memory
    PARAMS ((CORE_ADDR, char *, int, int, struct target_ops *));
void ppcbsdi_init_frame_pc_first PARAMS((int, struct frame_info *));
void kernel_init_frame_pc_first PARAMS((int, struct frame_info *));
CORE_ADDR ppcbsdi_frame_chain PARAMS((struct frame_info *));
CORE_ADDR kernel_frame_chain PARAMS((struct frame_info *));
int kernel_frame_chain_valid PARAMS((CORE_ADDR, struct frame_info *));
unsigned long ppcbsdi_frame_saved_pc PARAMS((struct frame_info *));

void frame_get_cache_fsr
    PARAMS((struct frame_info *, struct rs6000_framedata *));

#undef INIT_FRAME_PC_FIRST
#define	INIT_FRAME_PC_FIRST(leaf, fr) \
	(kernel_debugging ? kernel_init_frame_pc_first(leaf, fr) : \
	ppcbsdi_init_frame_pc_first(leaf, fr))

#undef FRAME_CHAIN
#define	FRAME_CHAIN(next) \
	(kernel_debugging ? kernel_frame_chain(next) : \
	ppcbsdi_frame_chain(next))

#undef FRAME_CHAIN_VALID
#define	FRAME_CHAIN_VALID(chain, next) \
	(kernel_debugging ? inside_kernstack(chain, next) : \
	    (chain) && !inside_main_func(next->pc))

#undef FRAME_SAVED_PC
#define	FRAME_SAVED_PC(fi)	ppcbsdi_frame_saved_pc(fi)

#endif

#undef IN_SIGTRAMP
#define	IN_SIGTRAMP(pc, name) \
	((name) != NULL && STREQ(name, "_sigtramp"))

#undef SIG_FRAME_PC_OFFSET
#define	SIG_FRAME_PC_OFFSET	36
#undef SIG_FRAME_FP_OFFSET
#define	SIG_FRAME_FP_OFFSET	24

#define	SIGCONTEXT_PC_OFFSET	20

/* This feature seems to have been added by someone who didn't realize
   that stabs in ELF shared objects need to be relocated using the
   ".stab.rel" section -- unless you perform the relocation, function
   addresses are zero.  The hack seems to work, however.  */
#define	SOFUN_ADDRESS_MAYBE_MISSING	1

#endif /* TM_PPC_BSDI_H */
