/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI elf.h,v 2.1 1997/11/04 01:14:13 donn Exp
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <errno.h>
#include <stdio.h>

#include "size.h"

static void _ELF_(size_pht)(const char *, const Elf_Phdr *, size_t, size_t);
static void _ELF_(size_sht)(const char *, const Elf_Shdr *, size_t, size_t);

/*
 * Print the size of the text, data and bss segments in an ELF executable.
 * If this is not an ELF executable, return 0.
 * If we encounter a corrupt ELF executable, return -1 with EFTYPE.
 * If we correctly process the executable, return 1.
 */
int
_ELF_(size)(const char *name, const void *v, size_t len)
{
	const Elf_Ehdr *e = v;
	const char *base = v;

	if (len < sizeof (Elf_Ehdr))
		return (0);

	if (e->e_ident[EI_MAG0] != ELFMAG0 ||
	    e->e_ident[EI_MAG1] != ELFMAG1 ||
	    e->e_ident[EI_MAG2] != ELFMAG2 ||
	    e->e_ident[EI_MAG3] != ELFMAG3)
		return (0);

	if (e->e_ident[EI_CLASS] != ELFCLASS)
		return (0);

	if ((e->e_ident[EI_DATA] != ELFDATA2LSB && \
	    BYTE_ORDER == LITTLE_ENDIAN) || \
	    (e->e_ident[EI_DATA] != ELFDATA2MSB && \
	    BYTE_ORDER == BIG_ENDIAN))
		return (0);

	errno = EFTYPE;
	if (e->e_phnum > 0) {
		if (e->e_phoff + e->e_phnum * e->e_phentsize > len)
			return (-1);
		_ELF_(size_pht)(name, (Elf_Phdr *)(base + e->e_phoff),
		    e->e_phentsize, e->e_phnum);
	} else if (e->e_shnum > 0) {
		if (e->e_shoff + e->e_shnum * e->e_shentsize > len)
			return (-1);
		_ELF_(size_sht)(name, (Elf_Shdr *)(base + e->e_shoff),
		    e->e_shentsize, e->e_shnum);
	} else
		return (-1);

	errno = 0;
	return (1);
}

void
_ELF_(size_pht)(const char *name, const Elf_Phdr *ph, size_t eltsize, size_t n)
{
	const Elf_Phdr *ph_end =
	    (const Elf_Phdr *)((const char *)ph + n * eltsize);
	u_long text = 0;
	u_long data = 0;
	u_long bss = 0;

	for (; ph < ph_end;
	    ph = (const Elf_Phdr *)((const char *)ph + eltsize)) {
		if (ph->p_type != PT_LOAD)
			continue;
		if ((ph->p_flags & PF_W) == 0)
			text += ph->p_memsz;
		else {
			bss += ph->p_memsz - ph->p_filesz;
			data += ph->p_filesz;
		}
	}

	display(name, text, data, bss);
}

void
_ELF_(size_sht)(const char *name, const Elf_Shdr *sh, size_t eltsize, size_t n)
{
	const Elf_Shdr *sh_end =
	    (const Elf_Shdr *)((const char *)sh + n * eltsize);
	u_long text = 0;
	u_long data = 0;
	u_long bss = 0;

	for (; sh < sh_end;
	    sh = (const Elf_Shdr *)((const char *)sh + eltsize)) {
		if ((sh->sh_flags & SHF_ALLOC) == 0)
			continue;
		if (sh->sh_type == SHT_NOBITS)
			bss += sh->sh_size;
		else {
			if (sh->sh_flags & SHF_WRITE)
				data += sh->sh_size;
			else
				text += sh->sh_size;
		}
	}

	display(name, text, data, bss);
}
