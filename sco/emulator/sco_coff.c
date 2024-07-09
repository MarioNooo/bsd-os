/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_coff.c,v 2.1 1995/02/03 15:13:59 polk Exp
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <string.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

struct filehdr {
	unsigned short	f_magic;
	unsigned short	f_nscns;
	long		_f_pad[3];
	unsigned short	f_opthdr;
	unsigned short	f_flags;
};

#define	F_EXEC		0x0002

struct aouthdr {
	short		magic;
	short		_pad1;
	long		_pad2[3];
	long		entry;
	long		_pad3[2];
};

struct scnhdr {
	char		_s_pad1[8];
	long		_s_pad2;
	long		s_vaddr;
	long		s_size;
	long		s_scnptr;
	long		_s_pad3[2];
	unsigned short	_s_pad4[2];
	long		s_flags;
};

#define	STYP_DSECT	0x0001
#define	STYP_NOLOAD	0x0002
#define	STYP_PAD	0x0008
#define	STYP_TEXT	0x0020
#define	STYP_DATA	0x0040
#define	STYP_BSS	0x0080
#define	STYP_INFO	0x0200
#define	STYP_OVER	0x0400
#define	STYP_LIB	0x0800

#define	COFF_START	0

#define	COFF_MAGIC	0514
#define	COFF_ZMAGIC	0413

#ifndef RUNSCO

/*
 * Initialize global variables for sco_break().
 * We call this 'program_*' because the name of the function
 * is not SCO-specific, although the implementation is.
 */
void
program_init_break()
{
	struct filehdr *fp = (struct filehdr *)(vm_offset_t)0;
	struct scnhdr *sp =
	    (struct scnhdr *)((vm_offset_t)&fp[1] + fp->f_opthdr);
	struct scnhdr *last_sp = &sp[fp->f_nscns];

	for (; sp < last_sp; ++sp)
		if (sp->s_flags == STYP_BSS) {
			sco_break_low = sp->s_vaddr + sp->s_size;
			sco_break_high = sco_break_low;
			return;
		}

	errx(1, "can't determine location of break");
}

#if 0
/*
 * The kernel has already loaded the COFF file and libraries;
 * we merely recover the COFF entry point from the COFF header,
 * which we assume is mapped at address 0.
 */
vm_offset_t
get_coff_entry()
{
	struct filehdr *fp = (struct filehdr *)(vm_offset_t)0;
	struct aouthdr *ap;

	if (fp->f_magic != COFF_MAGIC)
		errx(1, "COFF file has invalid magic number (%#x)", fp->f_magic);
	if (fp->f_opthdr < sizeof *ap)
		errx(1, "COFF opthdr field is bogus (%#x)", fp->f_opthdr);
	ap = (struct aouthdr *)&fp[1];
	return (ap->entry);
}
#endif

#else /* RUNSCO */

/*
 * Initialize global variables for sco_break().
 */
void
program_init_break(initial_break)
	vm_offset_t initial_break;
{

	sco_break_low = initial_break;
	sco_break_high = initial_break;
}

/*
 * We must load the COFF file ourselves.
 */

static void load_slib __P((vm_offset_t, struct scnhdr *));

vm_offset_t
load_coff(f)
	int f;
{
	struct stat s;
	struct filehdr *fp;
	struct aouthdr *ap;
	struct scnhdr *sp, *last_sp;
	vm_offset_t base = COFF_START;	/* XXX should pass as a parameter */
	vm_offset_t entry;
	vm_offset_t edata = -1;
	vm_offset_t vstart, vend, vtext = -1;
	size_t off;
	int prot;

	if (fstat(f, &s) == -1)
		err(1, "can't stat program");

	if (mmap((caddr_t)base, s.st_size, PROT_EXEC|PROT_READ,
	    MAP_FILE|MAP_FIXED|MAP_PRIVATE, f, 0) == (caddr_t)-1)
	    	err(1, "can't map program");
	fp = (struct filehdr *)base;
	off = sizeof (*fp);
	if (off > s.st_size)
		errx(1, "program is too small to be a COFF binary");

	if (fp->f_magic != COFF_MAGIC)
		errx(1, "program has bad magic number (%#o)", fp->f_magic);
	if ((fp->f_flags & F_EXEC) == 0)
		errx(1, "program header is not marked executable");
	if (fp->f_opthdr < sizeof (struct aouthdr))
		errx(1, "program is missing executable header");

	ap = (struct aouthdr *)(base + off);
	off += fp->f_opthdr;
	if (off > s.st_size)
		errx(1, "program is truncated (header size > file size)");

	if (ap->magic != COFF_ZMAGIC)
		errx(1, "unsupported program format (%#o)", ap->magic);
	entry = (vm_offset_t)ap->entry;

	sp = (struct scnhdr *)(base + off);
	last_sp = &sp[fp->f_nscns];
	for (; sp < last_sp; ++sp) {
	    	if ((off += sizeof (*sp)) > s.st_size)
			errx(1, "program is truncated (header size > file size)");

		if (sp->s_flags & STYP_NOLOAD)
			continue;
		switch (sp->s_flags) {

		case STYP_DSECT:
		case STYP_INFO:
		case STYP_OVER:
			/* not loaded */
			break;

		case STYP_LIB:
			load_slib(base, sp);
			break;

		case STYP_TEXT:
			/*
			 * A quick hack: if the file window is mapped
			 * at the text address, don't bother to map the text.
			 */
			if (i386_trunc_page(sp->s_vaddr) == base) {
				vtext =
				    i386_round_page(sp->s_vaddr + sp->s_size);
#if 0	/* unmap excess text */
				vend = i386_round_page(base + s.st_size);
				if (vend > vtext)
					munmap((caddr_t)vtext, vend - vtext);
#endif
				break;
			}
			prot = PROT_EXEC|PROT_READ;
			goto textdata;

		case STYP_DATA:
			prot = PROT_EXEC|PROT_READ|PROT_WRITE;
			edata = sp->s_vaddr + sp->s_size;
		textdata:
			vstart = i386_trunc_page(sp->s_vaddr);
			vend = i386_round_page(sp->s_vaddr + sp->s_size);
#if 0
			/* XXX this better not unmap our file window */
			if (munmap((caddr_t)vstart, vend - vstart) == -1)
				err(1, "can't clear region for text/data");
#endif
			if (mmap((caddr_t)vstart, vend - vstart, prot,
			    MAP_FILE|MAP_FIXED|MAP_PRIVATE,
			    f, i386_trunc_page(sp->s_scnptr)) == (caddr_t)-1)
				err(1, "can't map text/data");
			break;

		case STYP_BSS:
			if (edata == -1)
				errx(1, "no base address for bss");
			vend = i386_round_page(edata);
			if (edata != vend)
				bzero((caddr_t)edata,
				    MIN(sp->s_size, vend - edata));
			if (edata + sp->s_size > vend) {
				vstart = vend;
				vend = i386_round_page(edata + sp->s_size);
				if (mmap((caddr_t)vstart, vend - vstart,
				    PROT_EXEC|PROT_READ|PROT_WRITE,
				    MAP_ANON|MAP_FIXED|MAP_PRIVATE,
				    -1, 0) == (caddr_t)-1)
				    	err(1, "can't create bss");
			}
			program_init_break(edata + sp->s_size);
			break;

		default:
			errx(1, "unsupported section type (%#x)", sp->s_flags);
		}
	}

	if (vtext == -1)
		/* if we didn't re-use the file window, remove it */
		if (munmap((caddr_t)base, s.st_size) == -1)
			err(1, "can't remove file window");

	close(f);
	return (entry);
}

/*
 * This is currently a placeholder.
 */
static void
load_slib(base, sp)
	vm_offset_t base;
	struct scnhdr *sp;
{
	struct slib_entry {
		long	entsz;
		long	pathndx;
	};
	struct slib_entry *sep = (struct slib_entry *)(base + sp->s_scnptr);
	struct slib_entry *last_sep = (struct slib_entry *)
	    (base + sp->s_scnptr + sp->s_size);

	while (sep < last_sep) {
		warnx("requires shared library `%s'",
		    (char *)((long *)sep + sep->pathndx));
		sep = (struct slib_entry *)((long *)sep + sep->entsz);
	}
	errx(1, "shared libraries not yet supported");
}

#endif /* RUNSCO */
