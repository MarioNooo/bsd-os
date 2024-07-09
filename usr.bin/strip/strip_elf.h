/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strip_elf.h,v 2.3 2002/04/19 22:08:14 donn Exp
 */

#include <sys/param.h>
#include <sys/elf.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "strip.h"

static int cross_endian = 0;

#define SWAP_32(val)	(!cross_endian ? \
			 val : \
			 (((unsigned int)((val) & 0x000000ff) << 24) | \
                          ((unsigned int)((val) & 0x0000ff00) << 8)  | \
                          ((unsigned int)((val) & 0x00ff0000) >> 8)  | \
                          ((unsigned int)((val) & 0xff000000) >> 24)))

int
ELFNN(is_,)(v, size)
	const void *v;
	size_t size;
{
	const Elf_Ehdr *e = v;

	/* Make sure it really is an ELF file. */
	if (size < sizeof(*e) ||
	    e->e_ident[EI_MAG0] != ELFMAG0 ||
	    e->e_ident[EI_MAG1] != ELFMAG1 ||
	    e->e_ident[EI_MAG2] != ELFMAG2 ||
	    e->e_ident[EI_MAG3] != ELFMAG3)
		return (0);

	/* We are only compiled to handle one word size at a time.  */
	if (e->e_ident[EI_CLASS] != ELFCLASS)
		return (0);

	if (e->e_ident[EI_DATA] == ELFDATA2LSB) {
		if (BYTE_ORDER != LITTLE_ENDIAN)
			cross_endian = 1;
	} else if (e->e_ident[EI_DATA] == ELFDATA2MSB) {
		if (BYTE_ORDER != BIG_ENDIAN)
			cross_endian = 1;
	} else
		return (0);

	return (1);
}

ssize_t
ELFNN(,_sym)(v, size)
	void *v;
	size_t size;
{
	Elf_Ehdr *e = v;
	char *base = v;
	Elf_Phdr *p, *p_end;
	Elf_Off hi = 0;

	/* We only strip if we can find a program header.  */
	if (e->e_phnum == 0)
		return (size);

	/* Zap the section header table.  */
	e->e_shoff = 0;
	e->e_shentsize = 0;
	e->e_shnum = 0;
	e->e_shstrndx = 0;

	/* Find the greatest allocated file offset.  */
	p = (Elf_Phdr *)(base + SWAP_WORD(e->e_phoff));
	p_end = (Elf_Phdr *)((char *)p +
			     SWAP_SHORT(e->e_phnum) *
			     SWAP_SHORT(e->e_phentsize));

	for (;
	     p < p_end;
	     p = (Elf_Phdr *)((char *)p + SWAP_SHORT(e->e_phentsize)))
		if (hi < SWAP_WORD(p->p_offset) + SWAP_WORD(p->p_filesz))
			hi = SWAP_WORD(p->p_offset) + SWAP_WORD(p->p_filesz);

	return ((ssize_t)hi);
}

struct sect_desc {
	int n;
	char *start;
	char *end;
};

static int
ELFNN(,_debugging)(s, strtab)
	Elf_Shdr *s;
	char *strtab;
{
	char *name = strtab + SWAP_32(s->sh_name);
	int type = SWAP_32(s->sh_type);

	switch (type) {

	case SHT_PROGBITS:
		if (strcmp(name, ".stab") == 0 ||
		    strncmp(name, ".debug", 6) == 0 ||
		    strcmp(name, ".line") == 0)
			return (1);
		break;

	case SHT_STRTAB:
		if (strcmp(name, ".stabstr") == 0)
			return (1);
		break;

	case SHT_REL:
		if (strcmp(name, ".rel.stab") == 0 ||
		    strncmp(name, ".rel.debug", 10) == 0 ||
		    strcmp(name, ".rel.line") == 0)
			return (1);
		break;

	case SHT_RELA:
		if (strcmp(name, ".rela.stab") == 0 ||
		    strncmp(name, ".rela.debug", 11) == 0 ||
		    strcmp(name, ".rela.line") == 0)
			return (1);
		break;

	default:
		break;
	}

	return (0);
}

static int
ELFNN(,_desc_compare)(v1, v2)
	const void *v1;
	const void *v2;
{
	const struct sect_desc *d1 = v1;
	const struct sect_desc *d2 = v2;

	return (d1->start - d2->start);
}

ssize_t
ELFNN(,_stab)(v, size)
	void *v;
	size_t size;
{
	Elf_Ehdr *e = v;
	char *base = v;
	Elf_Shdr *s, *s_start, *s_end;
	struct sect_desc *d, *d_start, *d_end;
	char *p, *np;
	char *shstrtab;
	int nsects;
	u_int align;

	nsects = SWAP_SHORT(e->e_shnum);
	if (nsects == 0)
		return (size);

	if ((d_start = malloc((nsects + 3) * sizeof (*d))) == 0)
		err(1, "malloc");
	bzero((char *)d_start, (nsects + 3) * sizeof (*d));

	s_start = (Elf_Shdr *)(base + SWAP_WORD(e->e_shoff));
	s_end = (Elf_Shdr *)((char *)s_start +
			     nsects * SWAP_SHORT(e->e_shentsize));

	shstrtab = base +
		   SWAP_WORD(s_start[SWAP_SHORT(e->e_shstrndx)].sh_offset);

	/* The headers are implied sections.  */
	d = d_start;
	d->n = -1;
	d->start = base;
	d->end = base + SWAP_SHORT(e->e_ehsize);
	++d;
	d->start = base + SWAP_WORD(e->e_shoff);
	d->end = d->start + nsects * SWAP_SHORT(e->e_shentsize);
	++d;
	if (SWAP_SHORT(e->e_phnum) > 0) {
		d->n = -1;
		d->start = base + SWAP_WORD(e->e_phoff);
		d->end = d->start +
			 SWAP_SHORT(e->e_phnum) * SWAP_SHORT(e->e_phentsize);
		++d;
	}

	for (s = s_start;
	    s < s_end;
	    s = (Elf_Shdr *)((char *)s + SWAP_SHORT(e->e_shentsize))) {
		if (s->sh_name == 0)
			continue;
		d->n = s - s_start;
		d->start = base + SWAP_WORD(s->sh_offset);
		d->end = d->start + (SWAP_32(s->sh_type) == SHT_NOBITS ?
			             0 : SWAP_WORD(s->sh_size));
		++d;
	}
	d_end = d;

	/* Sort the sections by start address.  */
	qsort(d_start, d_end - d_start, sizeof (*d), ELFNN(,_desc_compare));

	/* Loop over the sorted sections, eliminating debugging sections.  */
	p = d_start->end;
	for (d = d_start + 1; d < d_end; ++d) {

		/* Make a feeble effort to deal with overlaps.  */
		if (d->end < p)
			continue;

		/* Don't move sections that get loaded.  */
		s = d->n == -1 ? 0 : &s_start[d->n];
		if (s == 0 || SWAP_WORD(s->sh_flags) & SHF_ALLOC) {
			p = d->end;
			continue;
		}

		/* Zap debugging sections.  */
		if (ELFNN(,_debugging)(s, shstrtab)) {
			bzero((char *)s, sizeof (*s));
			continue;
		}

		/* Move movable sections.  */
		align = SWAP_WORD(s->sh_addralign);
		if (align == 0)
			align = sizeof(Elf_Sword);	/* conservative */
		np = base + roundup(p - base, align);
		if (np < d->start) {
			bcopy(d->start, np, d->end - d->start);
			p = np + (d->end - d->start);
			if (s == s_start) {
				e->e_shoff = SWAP_WORD(np - base);
				s_start = (Elf_Shdr *)np;
			} else
				s->sh_offset = SWAP_WORD(np - base);
			continue;
		}

		p = d->end;
	}

	free(d_start);

	return (p - base);
}
