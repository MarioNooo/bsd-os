/*	BSDI bsdi.h,v 1.1 2003/01/31 07:52:31 torek Exp	*/

/* Base configuration file for all BSD/OS targets.
   Copyright (C) 2003 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Common BSD/OS configuration. 
   All BSD/OS architectures should include this file, which will specify
   their commonalities.
   Adapted from gcc/config/freebsd.h by 
   Kurt J. Lidl <lidl@pix.net> and Chris P. Ross <cross+gcc@distal.com>
   and Wind River Systems Inc.  */


/* See bsdi-spec.h.  */

#undef  SWITCH_TAKES_ARG
#define SWITCH_TAKES_ARG(CHAR) (BSDOS_SWITCH_TAKES_ARG(CHAR))

#undef  WORD_SWITCH_TAKES_ARG
#define WORD_SWITCH_TAKES_ARG(STR) (BSDOS_WORD_SWITCH_TAKES_ARG(STR))

#undef  CPP_PREDEFINES
#define CPP_PREDEFINES BSDOS_CPP_PREDEFINES

#undef  CPP_SPEC
#define CPP_SPEC BSDOS_CPP_SPEC

/* We use GNU ld.  Saying so in the config.gcc does not work right as
   the configuration still wants to use "collect2".  There is no need
   for that, so force LINKER_NAME here.  */
#define	LINKER_NAME	"ld"

/* startfile, endfile, and lib spec come from $arch/bsdi.h */

/************************[  Target stuff  ]***********************************/

/* ??? maybe this should be in $arch/bsdi.h? */
/* All modern BSD/OS Architectures support the ELF object file format.  */
#undef  OBJECT_FORMAT_ELF
#define OBJECT_FORMAT_ELF

/* Don't assume anything about the header files.  */
#undef  NO_IMPLICIT_EXTERN_C
#define NO_IMPLICIT_EXTERN_C	1

/* Allow #sccs in preprocessor.  */
#undef  SCCS_DIRECTIVE
#define SCCS_DIRECTIVE	1

#define MATH_LIBRARY_PROFILE    "-lm_p"

/* Code generation parameters.  */

/* ??? move to $arch/bsdi.h? */
/* Use periods rather than dollar signs in special g++ assembler names.
   This ensures the configuration knows our system correctly so we can link
   with libraries compiled with the native cc.  */
#undef NO_DOLLAR_IN_LABEL

/* Used by libgcc2.c.  We support file locking with fcntl / F_SETLKW.
   This enables the test coverage code to use file locking when exiting a
   program, which avoids race conditions if the program has forked.  */
#define TARGET_HAS_F_SETLKW	1
