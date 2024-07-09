/*	BSDI bsdi.h,v 1.3 2003/08/13 16:11:42 polk Exp	*/

/* Definitions for Intel 386 running BSD/OS with ELF format
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.
   Adapted from {Free,Net,Open}BSD/ELF by Kurt J. Lidl <lidl@pix.net>
   and Chris P. Ross <cross+gcc@distal.com>
   and Wind River Systems Inc.

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


/* We do not include i386/i386elf.h, just i386/i386.h, but we want
   to duplicate selected sections of it.  Note that we get i386/i386.h
   then i386/att.h (which in turn includes i386/unix.h) before this
   file gets included.  */

#undef  TARGET_VERSION
#define TARGET_VERSION fprintf (stderr, " (i386 ELF BSD/OS)");

/* Override the default comment-starter of "/".  */
#undef  ASM_COMMENT_START
#define ASM_COMMENT_START "#"

#undef  ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef  ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

#undef  SET_ASM_OP
#define SET_ASM_OP	"\t.set\t"

#undef  DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(n) \
  (TARGET_64BIT ? dbx64_register_map[n] : svr4_dbx_register_map[n])

/* Return small struct/union values in %eax.  Violates SVR4 ABI, alas.  */
#undef DEFAULT_PCC_STRUCT_RETURN
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Make gcc agree with <machine/ansi.h>.  */

#undef  SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef  PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef  WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef  WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE BITS_PER_WORD

/* We use the defaults from ../bsdi-spec.h.  */
#define	STARTFILE_SPEC	BSDOS_DEFAULT_STARTFILE_SPEC
#define	ENDFILE_SPEC	BSDOS_DEFAULT_ENDFILE_SPEC
#define	LIB_SPEC	BSDOS_DEFAULT_LIB_SPEC

/* I am not sure we really need "-m elf_i386" here any more... */
#define LINK_SPEC							\
  "-m elf_i386 %{shared:-shared} "					\
  "%{!shared:"								\
    "%{!static:"							\
      "%{rdynamic:-export-dynamic }"					\
      "%{!dynamic-linker:-dynamic-linker /shlib/ld-bsdi.so } }"		\
    "%{static:-static }}"

/* We never use this counter, but traditionally we left it in: */
#undef  NO_PROFILE_COUNTERS

#undef  FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)  \
{									\
  if (flag_pic)								\
    {									\
      fprintf (FILE, "\tleal %sP%d@GOTOFF(%%ebx),%%edx\n",		\
	       LPREFIX, (LABELNO));					\
      fprintf (FILE, "\tcall *_mcount@GOT(%%ebx)\n");			\
    }									\
  else									\
    {									\
      fprintf (FILE, "\tmovl $%sP%d,%%edx\n", LPREFIX, (LABELNO));	\
      fprintf (FILE, "\tcall _mcount\n");				\
    }									\
}

/* A C statement to output to the stdio stream FILE an assembler
   command to advance the location counter to a multiple of 1<<LOG
   bytes if it is within MAX_SKIP bytes.

   This is used to align code labels according to Intel recommendations.  */

#ifdef HAVE_GAS_MAX_SKIP_P2ALIGN
#define ASM_OUTPUT_MAX_SKIP_ALIGN(FILE, LOG, MAX_SKIP)			\
  if ((LOG) != 0) {							\
    if ((MAX_SKIP) == 0) fprintf ((FILE), "\t.p2align %d\n", (LOG));	\
    else fprintf ((FILE), "\t.p2align %d,,%d\n", (LOG), (MAX_SKIP));	\
  }
#endif

/* Unlike linux.h, we do not define these, so that "global bss"
   (which never occurs in C code) is just emitted as common symbol.
   Global bss does occur in libgcc and causes trouble with static
   shared libraries.  */
/* #define ASM_OUTPUT_BSS ... */
/* #define ASM_OUTPUT_ALIGNED_BSS ... */

/* These two features cause trouble with the static shared libraries, and
   eat boatloads of space.  Probably a better thing to do is check for -Os
   in the i386.c code that implements them, but for the moment...  */
#undef CONSTANT_ALIGNMENT
#undef DATA_ALIGNMENT

/* Don't default to pcc-struct-return, we want to retain compatibility with
   older gcc versions AND pcc-struct-return is nonreentrant.
   (even though the SVR4 ABI for the i386 says that records and unions are
   returned in memory).  */

#undef  DEFAULT_PCC_STRUCT_RETURN
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Copied from sysv4.h, but seems not to have much effect.  */
/* The routine used to output sequences of byte values.  We use a special
   version of this for most svr4 targets because doing so makes the
   generated assembly code more compact (and thus faster to assemble)
   as well as more readable.  Note that if we find subparts of the
   character sequence which end with NUL (and which are shorter than
   STRING_LIMIT) we output those using ASM_OUTPUT_LIMITED_STRING.  */

#undef ASM_OUTPUT_ASCII
#define ASM_OUTPUT_ASCII(FILE, STR, LENGTH)				\
  do									\
    {									\
      register const unsigned char *_ascii_bytes =			\
        (const unsigned char *) (STR);					\
      register const unsigned char *limit = _ascii_bytes + (LENGTH);	\
      register unsigned bytes_in_chunk = 0;				\
      for (; _ascii_bytes < limit; _ascii_bytes++)			\
        {								\
	  register const unsigned char *p;				\
	  if (bytes_in_chunk >= 64)					\
	    {								\
	      fputc ('\n', (FILE));					\
	      bytes_in_chunk = 0;					\
	    }								\
	  for (p = _ascii_bytes; p < limit && *p != '\0'; p++)		\
	    continue;							\
	  if (p < limit && (p - _ascii_bytes) <= (long) STRING_LIMIT)	\
	    {								\
	      if (bytes_in_chunk > 0)					\
		{							\
		  fputc ('\n', (FILE));					\
		  bytes_in_chunk = 0;					\
		}							\
	      ASM_OUTPUT_LIMITED_STRING ((FILE), _ascii_bytes);		\
	      _ascii_bytes = p;						\
	    }								\
	  else								\
	    {								\
	      if (bytes_in_chunk == 0)					\
		fprintf ((FILE), "\t.byte\t");				\
	      else							\
		fputc (',', (FILE));					\
	      fprintf ((FILE), "0x%02x", *_ascii_bytes);		\
	      bytes_in_chunk += 5;					\
	    }								\
	}								\
      if (bytes_in_chunk > 0)						\
        fprintf ((FILE), "\n");						\
    }									\
  while (0)

/* The Tornado debugger supports only DWARF, so let's make DWARF
   the default.  */
#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DWARF2_DEBUG

/* Dwarf2 turns on special C++ error-handler unwind goop.  This is from
   i386/linux.h.  I do not understand it at all, and have not delved into
   the unwind code.  I did not copy the signal-frame-catching code since
   the BSD/OS signal frames are different... */
/* Handle special EH pointer encodings.  Absolute, pc-relative, and
   indirect are handled automatically.  */
#define ASM_MAYBE_OUTPUT_ENCODED_ADDR_RTX(FILE, ENCODING, SIZE, ADDR, DONE) \
  do {									\
    if ((SIZE) == 4 && ((ENCODING) & 0x70) == DW_EH_PE_datarel)		\
      {									\
	fputs (ASM_LONG, FILE);						\
	assemble_name (FILE, XSTR (ADDR, 0));				\
	fputs (((ENCODING) & DW_EH_PE_indirect ? "@GOT" : "@GOTOFF"), FILE); \
	goto DONE;							\
      }									\
  } while (0)

/* Used by crtstuff.c to initialize the base of data-relative relocations.
   These are GOT relative on x86, so return the pic register.  */
#ifdef __PIC__
#define	CRT_GET_RFIB_DATA(BASE)			\
  {						\
    register void *ebx_ __asm__("ebx");		\
    BASE = ebx_;				\
  }
#else
#define CRT_GET_RFIB_DATA(BASE)						\
  __asm__ ("call\t.LPR%=\n"						\
	   ".LPR%=:\n\t"						\
	   /* Due to a GAS bug, this cannot use EAX.  That encodes	\
	      smaller than the traditional EBX, which results in the	\
	      offset being off by one.  */				\
	   "addl\t$_GLOBAL_OFFSET_TABLE_+[.-.LPR%=],%0"			\
	   : "=d"(BASE))
#endif

/* Do code reading to identify a signal frame, and set the frame
   state data appropriately.  See unwind-dw2.c for the structs.  */
/* XXX -- not done yet -- signal code is changing anyway */
