/*-
 * Copyright (c) 1997, 1999 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI elf_nlist_offset.h,v 2.3 1999/08/11 16:05:54 donn Exp
 */

/*
 * This generic ELF nlist() code is compiled twice,
 * once for 32-bit ELF and once for 64-bit ELF.
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <a.out.h>
#include <errno.h>

#include "nlist_var.h"

static off_t _ELF_(nlist_offset_by_pht)(unsigned long, const void *, size_t);
static off_t _ELF_(nlist_offset_by_sht)(unsigned long, const void *, size_t);

/*
 * Find the offset in the mapped binary file that corresponds to 
 * the given address in the memory image, if there is one.
 * We require that the file have a program header table.
 */
off_t
_ELF_(nlist_offset)(unsigned long address, const void *v, size_t size)
{
	const Elf_Ehdr *e = v;

	if (size < sizeof (*e)) {
		errno = EFTYPE;
		return (-1);
	}

	if (e->e_phnum > 0)
		return (_ELF_(nlist_offset_by_pht)(address, v, size));

	return (_ELF_(nlist_offset_by_sht)(address, v, size));
}

/*
 * Convert using program headers -- the preferred method.
 */
static off_t
_ELF_(nlist_offset_by_pht)(unsigned long address, const void *v, size_t size)
{
	const Elf_Ehdr *e = v;
	const char *base = v;
	const Elf_Phdr *ph;

	if (e->e_phoff > size ||
	    e->e_phentsize < sizeof (Elf_Phdr) ||
	    e->e_phoff + e->e_phentsize * e->e_phnum > size ||
	    e->e_phoff + e->e_phentsize * e->e_phnum < e->e_phoff) {
		errno = EFTYPE;
		return (-1);
	}

	for (ph = (const Elf_Phdr *)(base + e->e_phoff);
	    ph < (const Elf_Phdr *)
		(base + e->e_phoff + e->e_phentsize * e->e_phnum);
	    ph = (const Elf_Phdr *)((char *)ph + e->e_phentsize)) {
		if (ph->p_type != PT_LOAD)
			continue;
		if (ph->p_offset + ph->p_filesz > size) {
			errno = EFTYPE;
			return (-1);
		}
		if (address >= ph->p_vaddr &&
		    address < ph->p_vaddr + ph->p_filesz)
			return (ph->p_offset + (address - ph->p_vaddr));
	}

	errno = EINVAL;
	return (-1);
}

/*
 * Convert using section headers -- the backup method.
 */
static off_t
_ELF_(nlist_offset_by_sht)(unsigned long address, const void *v, size_t size)
{
	const Elf_Ehdr *e = v;
	const char *base = v;
	const Elf_Shdr *sh;
	const char *shstrtab;

	if (e->e_shnum == 0 ||
	    e->e_shoff > size ||
	    e->e_shentsize < sizeof (Elf_Shdr) ||
	    e->e_shoff + e->e_shentsize * e->e_shnum > size ||
	    e->e_shoff + e->e_shentsize * e->e_shnum < e->e_shoff) {
		errno = EFTYPE;
		return (-1);
	}

	if (e->e_type == ET_REL) {
		sh = (const Elf_Shdr *)(base + e->e_shoff +
		    e->e_shstrndx * e->e_shentsize);
		shstrtab = base + sh->sh_offset;
	}

	for (sh = (const Elf_Shdr *)(base + e->e_shoff + e->e_shentsize);
	    sh < (const Elf_Shdr *)
		(base + e->e_shoff + e->e_shentsize * e->e_shnum);
	    sh = (const Elf_Shdr *)((char *)sh + e->e_shentsize)) {
		if (sh->sh_type != SHT_PROGBITS)
			continue;
		if (sh->sh_offset + sh->sh_size > size) {
			errno = EFTYPE;
			return (-1);
		}
		/*
		 * There is a hack in _elfNN_nlist_scan() that sets the
		 * nlist address of a symbol from a relocatable file to its
		 * offset in the file minus the offset of the first loadable
		 * section; here we reverse that hack.
		 */
		if (e->e_type == ET_REL) {
			if (sh->sh_flags & SHF_ALLOC)
				return (address + sh->sh_offset);
			continue;
		}

		if (address >= sh->sh_addr &&
		    address < sh->sh_addr + sh->sh_size)
			return (sh->sh_offset + (address-sh->sh_addr));
	}

	errno = EINVAL;
	return (-1);
}
