/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI elf.h,v 2.1 1997/10/26 15:00:27 donn Exp
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <errno.h>
#include <stdio.h>

#include "shlist.h"

/*
 * Print the list of shared libraries used by an ELF executable.
 * If this is not an ELF executable, return 0.
 * If we encounter a corrupt ELF executable, return -1 with EFTYPE.
 * If we correctly process the executable, return 1.
 */
int
_ELF_(shlist)(const char *name, void *v, size_t len)
{
	const Elf_Ehdr *e = v;
	const char *base = v;
	const Elf_Phdr *ph;
	const Elf_Phdr *interp = NULL;
	const Elf_Phdr *text = NULL;
	const Elf_Phdr *data = NULL;
	const Elf_Phdr *dynamic = NULL;
	const Elf_Dyn *dp;
	const Elf_Dyn *d_start;
	const struct ldtab *lp;
	const char *strtab;
	unsigned long addr;

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

	if (e->e_type != ET_EXEC && e->e_type != ET_DYN)
		return (1);

	for (ph = (const Elf_Phdr *)(base + e->e_phoff);
	    ph < (const Elf_Phdr *)
		(base + e->e_phoff + e->e_phentsize * e->e_phnum);
	    ph = (const Elf_Phdr *)((char *)ph + e->e_phentsize)) {
		if (ph->p_offset + ph->p_filesz > len) {
			errno = EFTYPE;
			return (-1);
		}
		switch (ph->p_type) {
		case PT_INTERP:
			if (interp == NULL)
				interp = ph;
			break;
		case PT_LOAD:
			if (text == NULL && (ph->p_flags & PF_X) != 0)
				text = ph;
			if (data == NULL && (ph->p_flags & PF_W) != 0)
				data = ph;
			break;
		case PT_DYNAMIC:
			if (dynamic == NULL)
				dynamic = ph;
			break;
		default:
			break;
		}
	}

	if (interp == NULL)
		return (1);

	errno = EFTYPE;		/* the default */

	printf("%s:", name);

	if (dynamic != NULL) {
		printf(" %s", base + interp->p_offset);
		if (dynamic == NULL || text == NULL)
			return (-1);

		d_start = (Elf_Dyn *)(base + dynamic->p_offset);
		for (dp = d_start; dp->d_tag != DT_NULL; ++dp)
			if (dp->d_tag == DT_STRTAB) {
				addr = (unsigned long)dp->d_un.d_ptr;
				if (addr < text->p_vaddr ||
				    addr >= text->p_vaddr + text->p_filesz)
					return (-1);
				strtab = base + text->p_offset +
				    (addr - text->p_vaddr);
				break;
			}
		for (dp = d_start; dp->d_tag != DT_NULL; ++dp)
			if (dp->d_tag == DT_NEEDED)
				printf(" %s", strtab + dp->d_un.d_val);
	} else {
		/*
		 * XXX We 'assume' that interpreted binaries that
		 * XXX don't have dynamic sections must be BSD/OS
		 * XXX static shared library executables.
		 */
		if (text == NULL || data == NULL)
			return (-1);

		addr = *(unsigned long *)(base + data->p_offset);
		if (addr < text->p_vaddr ||
		    addr >= text->p_vaddr + text->p_filesz)
			return (-1);

		lp = (struct ldtab *)
		    (base + text->p_offset + (addr - text->p_vaddr));
		for (; lp->name; ++lp) {
			printf(" %s", base + text->p_offset +
			    ((unsigned long)lp->name - text->p_vaddr));
			if (aflag)
				printf("<%#10lx>", (unsigned long)lp->address);
		}
	}

	printf("\n");

	errno = 0;
	return (1);
}
