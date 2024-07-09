/*	BSDI config.h,v 1.2 2001/01/23 03:59:14 donn Exp	*/

#ifndef _LAP_CONFIG_H_
#define _LAP_CONFIG_H_ 1

/* Rename some symbols: */
#define	__brk		brk
#define	__getegid	getegid
#define	__geteuid 	geteuid 
#define	__getgid	getgid
#define	__getuid	getuid
#define	__mmap		mmap
#define	__mprotect	mprotect
#define	__munmap	munmap

#ifndef ASSEMBLER
/* Force the creation of a .init section.  */
#define	__libc_global_ctors	_init
void __libc_global_ctors(void) __attribute__((__section__(".init")));
#endif

/* From config.h.in: */

/* Define if using GNU ld, with support for weak symbols in a.out,
   and for symbol set and warning messages extensions in a.out and ELF.
   This implies HAVE_WEAK_SYMBOLS; set by --with-gnu-ld.  */
#define	HAVE_GNU_LD 1

/* Define if using ELF, which supports weak symbols.
   This implies HAVE_ASM_WEAK_DIRECTIVE and NO_UNDERSCORES; set by
   --with-elf.  */
#define	HAVE_ELF 1

/* Define if C symbols are asm symbols.  Don't define if C symbols
   have a `_' prepended to make the asm symbol.  */
#define	NO_UNDERSCORES 1

/* Define if weak symbols are available via the `.weak' directive.  */
#define	HAVE_ASM_WEAK_DIRECTIVE 1

/* Define if weak symbols are available via the `.weakext' directive.  */
#undef	HAVE_ASM_WEAKEXT_DIRECTIVE

/* Define if not using ELF, but `.init' and `.fini' sections are available.  */
#undef	HAVE_INITFINI

/* Define if using the GNU assembler, gas.  */
#define	HAVE_GNU_AS 1

/* Define if the assembler supports the `.set' directive.  */
#define	HAVE_ASM_SET_DIRECTIVE 1

/* Define to the name of the assembler's directive for
   declaring a symbol global (default `.globl').  */
#define	ASM_GLOBAL_DIRECTIVE .global

/* Define to use GNU libio instead of GNU stdio.
   This is defined by configure under --enable-libio.  */
#undef	USE_IN_LIBIO

/* Define if using ELF and the assembler supports the `.previous'
   directive.  */
#define	HAVE_ASM_PREVIOUS_DIRECTIVE 1

/* Define if using ELF and the assembler supports the `.popsection'
   directive.  */
#define	HAVE_ASM_POPSECTION_DIRECTIVE 1

/* Define to the prefix Alpha/ELF GCC emits before ..ng symbols.  */
#undef  ASM_ALPHA_NG_SYMBOL_PREFIX

/* Define if versioning of the library is wanted.  */
#undef	DO_VERSIONING

/* Define if static NSS modules are wanted.  */
#undef	DO_STATIC_NSS

/* Define if gcc uses DWARF2 unwind information for exception support.  */
#undef	HAVE_DWARF2_UNWIND_INFO

/* Define if gcc uses DWARF2 unwind information for exception support
   with static variable. */
#undef	HAVE_DWARF2_UNWIND_INFO_STATIC

/* Define if the compiler supports __builtin_expect.  */
#undef	HAVE_BUILTIN_EXPECT

/* Define if the regparm attribute shall be used for local functions
   (gcc on ix86 only).  */
#define	USE_REGPARMS	1

/* Defined on PowerPC if the GCC being used has a problem with clobbering
   certain registers (CR0, MQ, CTR, LR) in asm statements.  */
#undef	BROKEN_PPC_ASM_CR0


/* Defined to some form of __attribute__ ((...)) if the compiler supports
   a different, more efficient calling convention.  */
#if defined USE_REGPARMS && !defined PROF
# define internal_function __attribute__ ((regparm (3), stdcall))
#endif

/*
 */

#ifndef	_LIBC

/* These symbols might be defined by some sysdeps configures.
   They are used only in miscellaneous generator programs, not
   in compiling libc itself.   */

/* sysdeps/generic/configure.in */
#undef	HAVE_PSIGNAL

/* sysdeps/unix/configure.in */
#undef	HAVE_STRERROR

/* sysdeps/unix/common/configure.in */
#undef	HAVE_SYS_SIGLIST
#undef	HAVE__SYS_SIGLIST
#undef	HAVE__CTYPE_
#undef	HAVE___CTYPE_
#undef	HAVE___CTYPE
#undef	HAVE__CTYPE__
#undef	HAVE__CTYPE
#undef	HAVE__LOCP

#endif

/*
 */

#ifdef	_LIBC

/* The zic and zdump programs need these definitions.  */

#define	HAVE_STRERROR	1

/* The locale code needs these definitions.  */

#define HAVE_REGEX 1

#endif

#include "../D/linux/config.h"

#endif /* _LAP_CONFIG_H_ */
