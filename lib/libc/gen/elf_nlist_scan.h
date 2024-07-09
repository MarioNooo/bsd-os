/*-
 * Copyright (c) 1997, 1999 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI elf_nlist_scan.h,v 2.5 1999/10/08 07:06:53 donn Exp
 */

/*
 * This generic ELF nlist() code is compiled twice,
 * once for 32-bit ELF and once for 64-bit ELF.
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <errno.h>
#include <a.out.h>	/* for struct nlist */
#include <string.h>

#include "nlist_var.h"

static void _ELF_(to_nlist)(const void *, const Elf_Sym *, struct nlist *,
    Elf_Addr);

/*
 * Read an ELF file's symbol table, convert it to nlist-style symbols,
 * and feed the symbols and names to the given callback function.
 */
int
_ELF_(nlist_scan)(nlist_callback_fn callback, void *arg, const void *v,
    size_t size)
{
	const Elf_Ehdr *e = v;
	const Elf_Shdr *sh, *strsh;
	const Elf_Sym *s;
	struct nlist nl;
	const char *base = v;
	const char *strtab;
	const char *name;
	size_t strtab_size;
	Elf_Addr rel = 0;
	int r;

	/*
	 * Perform some sanity checking on the file header.
	 */
	if (size < sizeof (*e) ||
	    e->e_shoff > size ||
	    e->e_shentsize < sizeof (Elf_Shdr) ||
	    e->e_shoff + e->e_shentsize * e->e_shnum > size ||
	    e->e_shoff + e->e_shentsize * e->e_shnum < e->e_shoff)
		return (EFTYPE);

	if (e->e_shnum == 0)
		return (0);

	/*
	 * Loop over symbol table sections.
	 * We ignore dynamic symbol sections.
	 */
	for (sh = (const Elf_Shdr *)(base + e->e_shoff);
	    sh < (const Elf_Shdr *)
		(base + e->e_shoff + e->e_shentsize * e->e_shnum);
	    sh = (const Elf_Shdr *)((char *)sh + e->e_shentsize)) {

		/*
		 * ELF relocatable files use section-relative symbol addresses,
		 * unlike a.out relocatable files, so we compensate by
		 * returning the offset of the object in the file
		 * relative to the start of the first loadable section.
		 */
		if (sh->sh_flags & SHF_ALLOC && e->e_type == ET_REL && rel == 0)
			rel = sh->sh_offset;

		if (sh->sh_type != SHT_SYMTAB)
			continue;

		/*
		 * More sanity checking.
		 * We're mainly looking for truncated files,
		 * but trashed files are a bit of a concern too.
		 */
		if (sh->sh_offset > size ||
		    sh->sh_entsize < sizeof (Elf_Sym) ||
		    sh->sh_link >= e->e_shnum ||
		    sh->sh_offset + sh->sh_size > size ||
		    sh->sh_offset + sh->sh_size < sh->sh_offset)
			return (EFTYPE);

		strsh = (const Elf_Shdr *)
		    (base + e->e_shoff + sh->sh_link * e->e_shentsize);
		if (strsh->sh_offset > size ||
		    strsh->sh_offset + strsh->sh_size > size ||
		    strsh->sh_offset + strsh->sh_size < strsh->sh_offset)
			return (EFTYPE);

		if (strsh->sh_size <= 1)
			continue;
		strtab = base + strsh->sh_offset;
		strtab_size = strsh->sh_size;

		/*
		 * Loop over symbols.
		 */
		for (s = (const Elf_Sym *)(base + sh->sh_offset);
		    s < (const Elf_Sym *)(base + sh->sh_offset + sh->sh_size);
		    s = (const Elf_Sym *)((const char *)s + sh->sh_entsize)) {
			if (s->st_name >= strtab_size)
				return (EFTYPE);
			if (s->st_name > 0)
				name = &strtab[s->st_name];
			else
				name = NULL;
			_ELF_(to_nlist)(v, s, &nl, rel);
			if ((r = (*callback)(&nl, name, arg)) != 0)
				return (r < 0 ? errno : 0);
		}
	}

	return (0);
}

/* Convert ELF symbols into a.out-style nlist symbols.  */
static void
_ELF_(to_nlist)(const void *v, const Elf_Sym *s, struct nlist *n, Elf_Addr rel)
{
	const char *base = v;
	Elf_Shdr *sh;

	memset(n, '\0', sizeof (*n));

	n->n_value = s->st_value;

	switch (s->st_shndx) {
	case SHN_UNDEF:
		/* Tricky -- handle dynamically linked symbols.  */
		switch (ELF32_ST_TYPE(s->st_info)) {
		case STT_OBJECT:
			n->n_type = N_DATA;
			break;
		case STT_FUNC:
			n->n_type = N_TEXT;
			break;
		default:
			n->n_type = N_UNDF;
			break;
		}
		break;
	case SHN_ABS:
		if (ELF32_ST_TYPE(s->st_info) == STT_FILE)
			n->n_type = N_FN;
		else
			n->n_type = N_ABS;
		break;
	case SHN_COMMON:
		n->n_type = N_UNDF;
		n->n_value = s->st_size;
		break;
	default:
		sh = (Elf_Shdr *)(base + ((Elf_Ehdr *)base)->e_shoff +
		    s->st_shndx * ((Elf_Ehdr *)base)->e_shentsize);

		/* Handle section-relative symbols in relocatable files.  */
		if (rel != 0)
			n->n_value += sh->sh_offset - rel;

		/* Try to convert ELF section types to a.out segment types.  */
		if (sh->sh_type == SHT_NOBITS && (sh->sh_flags & SHF_ALLOC)) {
			n->n_type = N_BSS;
			break;
		}
		if (sh->sh_type == SHT_PROGBITS && sh->sh_flags & SHF_ALLOC)
			switch (sh->sh_flags & (SHF_WRITE|SHF_EXECINSTR)) {
			case 0:
				n->n_type = N_RODATA;
				break;
			case SHF_EXECINSTR:
				n->n_type = N_TEXT;
				break;
			default:
				n->n_type = N_DATA;
				break;
			}
		if (n->n_type == 0)
			switch (ELF32_ST_TYPE(s->st_info)) {
			default:
				n->n_type = N_DATA;
				break;
			case STT_FUNC:
				n->n_type = N_TEXT;
				break;
			case STT_FILE:
				n->n_type = N_FN;
				break;
			}
	}
	if (ELF32_ST_BIND(s->st_info) == STB_GLOBAL ||
	    ELF32_ST_BIND(s->st_info) == STB_WEAK)
		n->n_type |= N_EXT;
}
