/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI elf_nlist_classify.h,v 2.1 1997/10/25 15:33:35 donn Exp
 */

/*
 * This generic ELF nlist() code is compiled twice,
 * once for 32-bit ELF and once for 64-bit ELF.
 */

#include <sys/types.h>
#include <sys/elf.h>

#include "nlist_var.h"

enum _nlist_filetype
_ELF_(nlist_classify)(const void *v, size_t size)
{
	const Elf_Ehdr *e = v;

	if (size < sizeof (Elf_Ehdr))
		return (_unknown_nlist_filetype);

	/* Correct magic number?  */
	if ((e)->e_ident[EI_MAG0] != ELFMAG0 ||
	    (e)->e_ident[EI_MAG1] != ELFMAG1 ||
	    (e)->e_ident[EI_MAG2] != ELFMAG2 ||
	    (e)->e_ident[EI_MAG3] != ELFMAG3)
		return (_unknown_nlist_filetype);

	/* Correct word size?  */
	if ((e)->e_ident[EI_CLASS] != ELFCLASS)
		return (_unknown_nlist_filetype);

	/* Correct byte order?  For now, no bisexual architectures.  */
	if (((e)->e_ident[EI_DATA] != ELFDATA2LSB && \
	    BYTE_ORDER == LITTLE_ENDIAN) || \
	    ((e)->e_ident[EI_DATA] != ELFDATA2MSB && \
	    BYTE_ORDER == BIG_ENDIAN))
		return (_unknown_nlist_filetype);

	return (_ELF_(nlist_filetype));
}
