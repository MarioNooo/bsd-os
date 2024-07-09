/*	BSDI elf.h,v 1.2 1998/09/09 05:59:05 torek Exp	*/

#ifndef _LINUX_ELF_H_
#define	_LINUX_ELF_H_	1

#include <sys/elf.h>

/*
 * For some reason, the Linux code wants to refer to these structures
 * using structure tags instead of ELF typedefs.
 * We keep them out of the normal namespace with #defines.
 */

#define	elfhdr		__elfhdr
#define	elf_phdr	__elf_phdr
#define	elf32_sym	__elf32_sym
#define	elf32_rel	__elf32_rel
#define	elf32_rela	__elf32_rela
#define	dynamic		__dynamic

/* XXX Until we attempt Linux binary emulation...   */
#undef EM_486
#define	EM_486		EM_386

#endif
