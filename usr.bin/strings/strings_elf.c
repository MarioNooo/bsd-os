/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strings_elf.c,v 2.1 1997/09/06 19:46:32 donn Exp
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "strings_private.h"

#ifdef ELF64
#define	Elf_Ehdr	Elf64_Ehdr
#define	Elf_Shdr	Elf64_Shdr
#define	Elf_Phdr	Elf64_Phdr
#define	Elf_Off		Elf64_Off
#else
#define	Elf_Ehdr	Elf32_Ehdr
#define	Elf_Shdr	Elf32_Shdr
#define	Elf_Phdr	Elf32_Phdr
#define	Elf_Off		Elf32_Off
#endif

static Elf_Ehdr e;

static Elf_Phdr *pht;

/*
 * Arrange to scan through text or data segments in an ELF file.
 */
static off_t
elf_pht_section(off_t *op)
{
	Elf_Phdr *best = NULL;
	Elf_Phdr *p;
	Elf_Off phtend = e.e_phoff + e.e_phnum * e.e_phentsize;
	Elf_Off base;
	int i;

	for (i = 0; i < e.e_phnum; ++i) {
		p = (Elf_Phdr *)((char *)pht + i * e.e_phentsize);
		/* Special hack for text that includes headers.  */
		if (p->p_offset == 0)
			base = phtend;
		else
			base = p->p_offset;
		if (p->p_type == PT_LOAD && base >= *op && p->p_filesz > 0 &&
		    (best == NULL || best->p_offset > p->p_offset))
			best = p;
	}

	if (best == NULL) {
		*op = 0;
		free(pht);
		return (0);
	}

	if (best->p_offset == 0) {
		*op = phtend;
		return (best->p_filesz - phtend);
	}
	*op = best->p_offset;
	return (best->p_filesz);
}

static Elf_Shdr *sht;

/*
 * Arrange to scan through text or data sections in an ELF file.
 */
static off_t
elf_sht_section(off_t *op)
{
	Elf_Shdr *best = NULL;
	Elf_Shdr *s;
	int i;

	for (i = 0; i < e.e_shnum; ++i) {
		s = (Elf_Shdr *)((char *)sht + i * e.e_shentsize);
		if (s->sh_type == SHT_PROGBITS &&
		    s->sh_flags & SHF_ALLOC &&
		    s->sh_offset >= *op &&
		    s->sh_size > 0 &&
		    (best == NULL || best->sh_offset > s->sh_offset))
			best = s;
	}

	if (best == NULL) {
		*op = 0;
		free(sht);
		return (0);
	}

	*op = best->sh_offset;
	return (best->sh_size);
}

/*
 * See whether this is an ELF file.
 * If so, return elf_pht_section() if it has a program header table,
 * or elf_sht_section() if it has a section header table.
 */
sec_t
check_elf(FILE *f, off_t *op)
{
	size_t len;
	size_t n;

	/*
	 * If we can reject this file based on just the magic number,
	 * do so and save the effort of rejecting the entire header.
	 */
	if ((n = fread((char *)&e.e_ident, 1, EI_MAG3 + 1, f)) == 0)
		return (NULL);
	if (n != EI_MAG3 + 1 || e.e_ident[EI_MAG0] != ELFMAG0 ||
	    e.e_ident[EI_MAG1] != ELFMAG1 || e.e_ident[EI_MAG2] != ELFMAG2 ||
	    e.e_ident[EI_MAG3] != ELFMAG3) {
		unget(&e.e_ident, n, f);
		return (NULL);
	}

	/* Read the rest of the ELF header.  */
	n += fread((char *)(&e.e_ident[EI_MAG3 + 1]), 1, sizeof (e) - n, f);
	if (ferror(f))
		return (NULL);
	if (n != sizeof (e) ||
	    (e.e_ident[EI_CLASS] == ELFCLASS32 &&
		sizeof (Elf_Off) * NBBY != 32) ||
	    (e.e_ident[EI_CLASS] == ELFCLASS64 &&
		sizeof (Elf_Off) * NBBY != 64) ||
	    (e.e_ident[EI_DATA] == ELFDATA2LSB &&
		BYTE_ORDER != LITTLE_ENDIAN) ||
	    (e.e_ident[EI_DATA] == ELFDATA2MSB && BYTE_ORDER != BIG_ENDIAN)) {
		unget(&e, n, f);
		return (NULL);
	}

	/* We assume that the PHT must precede text and data...  */
	if (e.e_phnum > 0) {
		len = e.e_phnum * e.e_phentsize;
		if ((pht = malloc(len)) == NULL)
			err(1, NULL);
		if (fseek(f, e.e_phoff, SEEK_SET) == -1)
			for (; n < e.e_phoff; ++n)
				if (getc_unlocked(f) == EOF)
					break;
		if (!ferror(f) && !feof(f) && fread(pht, 1, len, f) == len) {
			*op = e.e_phoff + len;
			return (elf_pht_section);
		}
		free(pht);
		/* XXX if the seek fails, we're hosed; too bad */
		fseek(f, 0, SEEK_SET);
	}

	/* ... but we can't and don't assume that about the SHT.  */
	if (e.e_shnum > 0) {
		len = e.e_shnum * e.e_shentsize;
		if ((sht = malloc(len)) == NULL)
			err(1, NULL);
		if (fseek(f, e.e_shoff, SEEK_SET) == 0 &&
		    fread(sht, 1, len, f) == len) {
			fseek(f, sizeof (e), SEEK_SET);
			*op = sizeof (e);
			return (elf_sht_section);
		}
		fseek(f, sizeof (e), SEEK_SET);
		free(sht);
	}

	unget(&e, n, f);
	return (NULL);
}
