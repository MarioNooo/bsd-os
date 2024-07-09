/* BFD back-end for i386 a.out binaries under BSD.
   Copyright 1990, 1991, 1992, 1993, 1994, 2001
   Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This data should be correct for the format used under all the various
   BSD ports for 386 machines.  */

#define	BYTES_IN_WORD	4

/* ZMAGIC files never have the header in the text.  */
#define	N_HEADER_IN_TEXT(x)	0

/* ZMAGIC files start at address 0.  This does not apply to QMAGIC.  */
#define TEXT_START_ADDR 0
#define N_SHARED_LIB(x) 0

#define	TARGET_PAGE_SIZE	4096
#define	SEGMENT_SIZE	TARGET_PAGE_SIZE

/*	BSDI i386bsd.c,v 1.2 2003/01/14 16:28:42 torek Exp	*/

#define	DEFAULT_ARCH	bfd_arch_i386
#define MACHTYPE_OK(mtype) ((mtype) == M_386 || (mtype) == M_UNKNOWN)

/* Do not "beautify" the CONCAT* macro args.  Traditional C will not
   remove whitespace added here, and thus will fail to concatenate
   the tokens.  */
#define MY(OP) CONCAT2 (i386bsd_,OP)
#define TARGETNAME "a.out-i386-bsd"

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "libaout.h"

#ifdef __bsdi__

#include "aout/aout64.h"

#define	MY_callback	MY(callback)

static const bfd_target *MY(callback) PARAMS ((bfd *));

static const bfd_target *
MY(callback) (abfd)
     bfd *abfd;
{
  struct internal_exec *execp = exec_hdr (abfd);
  unsigned int arch_align_power;
  unsigned long arch_align;

  /* Calculate the file positions of the parts of a newly read aout header */
  obj_textsec (abfd)->_raw_size = N_TXTSIZE(*execp);

  /* The virtual memory addresses of the sections */
  obj_textsec (abfd)->vma = N_TXTADDR(*execp);
  obj_datasec (abfd)->vma = N_DATADDR(*execp);
  obj_bsssec  (abfd)->vma = N_BSSADDR(*execp);

  /* Check for kernel binaries.  */
  if ((execp->a_entry & 0xff000000) != 0)
    {
      bfd_vma adjust;

      adjust = execp->a_entry - obj_textsec (abfd)->vma;
      /* Adjust only by whole pages. */
      adjust &= ~(TARGET_PAGE_SIZE - 1);
      obj_textsec (abfd)->vma += adjust;
      obj_datasec (abfd)->vma += adjust;
      obj_bsssec (abfd)->vma += adjust;
    }

  /* Set the load addresses to be the same as the virtual addresses.  */
  obj_textsec (abfd)->lma = obj_textsec (abfd)->vma;
  obj_datasec (abfd)->lma = obj_datasec (abfd)->vma;
  obj_bsssec (abfd)->lma = obj_bsssec (abfd)->vma;

  /* The file offsets of the sections */
  obj_textsec (abfd)->filepos = N_TXTOFF (*execp);
  obj_datasec (abfd)->filepos = N_DATOFF (*execp);

  /* The file offsets of the relocation info */
  obj_textsec (abfd)->rel_filepos = N_TRELOFF(*execp);
  obj_datasec (abfd)->rel_filepos = N_DRELOFF(*execp);

  /* The file offsets of the string table and symbol table.  */
  obj_sym_filepos (abfd) = N_SYMOFF (*execp);
  obj_str_filepos (abfd) = N_STROFF (*execp);

  /* Determine the architecture and machine type of the object file.  */
#ifdef SET_ARCH_MACH
  SET_ARCH_MACH(abfd, *execp);
#else
  bfd_default_set_arch_mach(abfd, DEFAULT_ARCH, 0);
#endif

  /* The number of relocation records.  This must be called after
     SET_ARCH_MACH.  It assumes that SET_ARCH_MACH will set
     obj_reloc_entry_size correctly, if the reloc size is not
     RELOC_STD_SIZE.  */
  obj_textsec (abfd)->reloc_count =
    execp->a_trsize / obj_reloc_entry_size (abfd);
  obj_datasec (abfd)->reloc_count =
    execp->a_drsize / obj_reloc_entry_size (abfd);

  /* Now that we know the architecture, set the alignments of the
     sections.  This is normally done by NAME(aout,new_section_hook),
     but when the initial sections were created the architecture had
     not yet been set.  However, for backward compatibility, we don't
     set the alignment power any higher than as required by the size
     of the section.  */
  arch_align_power = bfd_get_arch_info (abfd)->section_align_power;
  arch_align = 1 << arch_align_power;
  if ((BFD_ALIGN (obj_textsec (abfd)->_raw_size, arch_align)
       == obj_textsec (abfd)->_raw_size)
      && (BFD_ALIGN (obj_datasec (abfd)->_raw_size, arch_align)
        == obj_datasec (abfd)->_raw_size)
      && (BFD_ALIGN (obj_bsssec (abfd)->_raw_size, arch_align)
        == obj_bsssec (abfd)->_raw_size))
    {
      obj_textsec (abfd)->alignment_power = arch_align_power;
      obj_datasec (abfd)->alignment_power = arch_align_power;
      obj_bsssec (abfd)->alignment_power = arch_align_power;
    }

  /* Don't set sizes now -- can't be sure until we know arch & mach.
     Sizes get set in set_sizes callback, later.  */
#if 0
  adata(abfd).page_size = TARGET_PAGE_SIZE;
  adata(abfd).segment_size = SEGMENT_SIZE;
  adata(abfd).exec_bytes_size = EXEC_BYTES_SIZE;
#endif

  return abfd->xvec;
}

static void MY_final_link_callback
  PARAMS ((bfd *, file_ptr *, file_ptr *, file_ptr *));
static boolean i386bsd_bfd_final_link
  PARAMS ((bfd *, struct bfd_link_info *));

/*
 * Make QMAGIC be the default output format.
 */
static boolean
i386bsd_bfd_final_link (abfd, info)
     bfd *abfd;
     struct bfd_link_info *info;
{
  obj_aout_subformat (abfd) = q_magic_format;
  return NAME(aout,final_link) (abfd, info, MY_final_link_callback);
}

#define	MY_bfd_final_link	i386bsd_bfd_final_link
#endif

#include "aout-target.h"
