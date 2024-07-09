/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 2001 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI shlicrt0.c,v 2.2 2001/10/03 17:30:00 polk Exp
 */

/*
 * This file contains code that needs to appear at the start of programs
 * that use statically linked shared libraries.
 *
 * Currently the only information that we need to add is the brand.
 * Note that shlicc(1) generates information into the data section
 * that must appear at the start of data, so don't create data here.
 */

#include <sys/param.h>
#include <sys/elf.h>

/*
 * Implement Linux/FreeBSD/NetBSD ELF style binary branding.
 * See http://www.netbsd.org/Documentation/kernel/elf-notes.html
 * for more information.
 */
#define	ABI_OSNAME	"BSD/OS"
#define	ABI_NOTETYPE	1

static const struct {
	Elf32_Nhdr hdr;
	char	name[roundup(sizeof (ABI_OSNAME), sizeof (int32_t))];
	char	desc[roundup(sizeof (RELEASE), sizeof (int32_t))];
} ident __attribute__ ((__section__ (".note.ABI-tag"))) = {
	{ sizeof (ABI_OSNAME), sizeof (RELEASE), ABI_NOTETYPE },
	ABI_OSNAME,
	RELEASE
};
