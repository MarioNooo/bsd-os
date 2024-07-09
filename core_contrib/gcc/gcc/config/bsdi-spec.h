/*	BSDI bsdi-spec.h,v 1.3 2003/06/26 02:28:15 torek Exp	*/

/* Base configuration file for all BSD/OS targets.
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

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
   Adapted from gcc/config/freebsd-spec.h by
   Kurt J. Lidl <lidl@pix.net> and Chris P. Ross <cross+gcc@distal.com>
   and Wind River Systems Inc.  */

/* In case we need to know.  */
#define USING_CONFIG_BSDOS_SPEC 1

/* This defines which switch letters take arguments.  On BSD/OS, just like
   FreeBSD, most of the normal cases (defined in gcc.c) apply, and we also
   have -h* and -z* options (for the linker) (coming from SVR4).
   We also have -R (alias --rpath), no -z, --soname (-h), --assert etc.  */

#define BSDOS_SWITCH_TAKES_ARG(CHAR)					\
  (DEFAULT_SWITCH_TAKES_ARG (CHAR)					\
    || (CHAR) == 'h'							\
    || (CHAR) == 'z' /* ignored by ld */				\
    || (CHAR) == 'R')

/* This defines which multi-letter switches take arguments.  */

#define BSDOS_WORD_SWITCH_TAKES_ARG(STR)				\
  (DEFAULT_WORD_SWITCH_TAKES_ARG (STR)					\
   || !strcmp ((STR), "rpath") || !strcmp ((STR), "rpath-link")		\
   || !strcmp ((STR), "soname") || !strcmp ((STR), "defsym") 		\
   || !strcmp ((STR), "assert") || !strcmp ((STR), "dynamic-linker"))

/* For earlier systems, we must check whether they used ELF, etc.;
   but Wind River only supports versions dating back to 4.x.

   N.B.: the version number comes from the gcc configuration, not
   the <sys/param.h> _BSD_OS_MAJOR macro, because we want to allow
   gcc to be built *on* BSD/OS 5.x *for* BSD/OS 4.x and vice versa,
   for instance. */
#if BSDOS_MAJOR == 5
#define BSDOS_MAJOR_DEFINES "-D__bsdos__=5 -D__ELF__"
#endif

#if BSDOS_MAJOR == 4
#define BSDOS_MAJOR_DEFINES "-D__bsdos__=4 -D__ELF__"
#endif

/* Provide defines that the preprocessor should always set.  Note that the
   architecture-specific flags (e.g., -D__i386__) are handled elsehwere. */

#if 0 /* no longer providing GAS and GAS_MINOR macros */
#define	BSDOS_CPP_PREDEFINES BSDOS_MAJOR_DEFINES \
  " -D__GAS__=" GAS_MAJOR " -D__GAS_MINOR__=" GAS_MINOR			\
  " -D__GAS__=" GAS_MAJOR " -D__GAS_MINOR__=" GAS_MINOR			\
  " -D__bsdi__ -D__unix__"						\
  " -Asystem=unix -Asystem=bsd -Asystem=bsdos -Asystem=bsdi"
#else
#define	BSDOS_CPP_PREDEFINES BSDOS_MAJOR_DEFINES \
  " -D__bsdi__ -D__unix__"						\
  " -Asystem=unix -Asystem=bsd -Asystem=bsdos -Asystem=bsdi"
#endif

/* Provide a CPP_SPEC appropriate for BSD/OS.  We deal with the GCC
   optiond -posix and -pthread, and PIC issues.  We also arrange for
   the "cpp" binary front end to use -traditional and -$ by default,
   and to deal with multiple files.  */

#ifdef CPP_FRONT_END
#define CPP_TAKES_MULTIPLE_ARGUMENTS	1
#define	CPP_NEED_GCC			1
#else
#define BSDOS_CPP_SPEC \
  "%(cpp_cpu) %{fPIC:-D__PIC__ -D__pic__} %{fpic:-D__PIC__ -D__pic__} " \
  "%{posix:-D_POSIX_SOURCE} %{pthread:-D_REENTRANT}"
#endif

/* The actual STARTFILE and ENDFILE specs are in the machine-dependent
   BSD/OS files, to allow for mixed 32-and-64-bit systems in the future.
   The same goes for the library specification.  Here we define default
   values here that most systems can use.  */

/* Provide a STARTFILE_SPEC appropriate for BSD/OS.  Here we add
   the magical crtbegin.o file (see crtstuff.c) which provides part
   of the support for getting C++ file-scope static objects constructed
   before entering `main'. */
#define BSDOS_DEFAULT_STARTFILE_SPEC \
  "%{!shared: \
     %{pg:gcrt1.o%s} %{!pg:%{p:gcrt1.o%s} \
		       %{!p:%{profile:gcrt1.o%s} \
			 %{!profile:crt1.o%s}}}} \
   crti.o%s %{!shared:crtbegin.o%s} %{shared:crtbeginS.o%s}"

/* Provide a ENDFILE_SPEC appropriate for BSD/OS.  Here we tack on
   the magical crtend.o file (see crtstuff.c) which provides part of
   the support for getting C++ file-scope static object constructed
   before entering `main', followed by a normal "finalizer" file,
   `crtn.o'.  */

#define BSDOS_DEFAULT_ENDFILE_SPEC \
  "%{!shared:crtend.o%s} %{shared:crtendS.o%s} crtn.o%s"

/* Provide a LIB_SPEC appropriate for BSD/OS.  For now we just
   need to check on profiling.  In the future, we may have a
   "no thread support" version of libc.  */

#define BSDOS_DEFAULT_LIB_SPEC \
  "%{!shared:%{pg:-lc_p}%{!pg:%{p:-lc_p}%{!p:%{profile:-lc_p}%{!profile:-lc}}}}"

/* Keep gcc.c (driver) from supplying various sundry inappropriate -L
   flags to the linker.  We need this to avoid having /usr/lib searched
   before /shlib; the latter causes dynamic linking not to happen.  */
#define LINK_LIBGCC_SPECIAL
