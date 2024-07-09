/*	$NetBSD: ppc_reloc.c,v 1.31 2002/09/26 20:42:11 mycroft Exp $	*/

/*-
 * Copyright (C) 1998	Tsubai Masanari
 * Portions copyright 2002 Charles M. Hannum.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/cpu.h>

#include "debug.h"
#include "rtld.h"

void _rtld_powerpc_pltcall __P((Elf_Word));
void _rtld_powerpc_pltresolve __P((Elf_Word, Elf_Word));

#define ha(x) ((((u_int32_t)(x) & 0x8000) ? \
			((u_int32_t)(x) + 0x10000) : (u_int32_t)(x)) >> 16)
#define l(x) ((u_int32_t)(x) & 0xffff)

void _rtld_bind_start(void);
void _rtld_relocate_nonplt_self(Elf_Dyn *, Elf_Addr);
caddr_t _rtld_bind __P((const Obj_Entry *, Elf_Word));

/*
 * Setup the plt glue routines.
 */
#define PLTCALL_SIZE	20
#define PLTRESOLVE_SIZE	24

void
_rtld_setup_pltgot(obj)
	const Obj_Entry *obj;
{
	Elf_Word *pltcall, *pltresolve;
	Elf_Word *jmptab;
	int N = obj->pltrelalim - obj->pltrela;

	/* Entries beyond 8192 take twice as much space. */
	if (N > 8192)
		N += N-8192;

	pltcall = obj->pltgot;
	jmptab = pltcall + 18 + N * 2;

	memcpy(pltcall, _rtld_powerpc_pltcall, PLTCALL_SIZE);
	pltcall[1] |= ha(jmptab);
	pltcall[2] |= l(jmptab);

	pltresolve = obj->pltgot + 8;

	memcpy(pltresolve, _rtld_powerpc_pltresolve, PLTRESOLVE_SIZE);
	pltresolve[0] |= ha(_rtld_bind_start);
	pltresolve[1] |= l(_rtld_bind_start);
	pltresolve[3] |= ha(obj);
	pltresolve[4] |= l(obj);

	__syncicache(pltcall, 72 + N * 8);
}

void
_rtld_relocate_nonplt_self(dynp, relocbase)
	Elf_Dyn *dynp;
	Elf_Addr relocbase;
{
	const Elf_Rela *rela = 0, *relalim;
	Elf_Addr relasz = 0;
	Elf_Addr *where;

	for (; dynp->d_tag != DT_NULL; dynp++) {
		switch (dynp->d_tag) {
		case DT_RELA:
			rela = (const Elf_Rela *)(relocbase + dynp->d_un.d_ptr);
			break;
		case DT_RELASZ:
			relasz = dynp->d_un.d_val;
			break;
		}
	}
	relalim = (const Elf_Rela *)((caddr_t)rela + relasz);
	for (; rela < relalim; rela++) {
		where = (Elf_Addr *)(relocbase + rela->r_offset);
		*where = (Elf_Addr)(relocbase + rela->r_addend);
	}
}

int
_rtld_relocate_nonplt_objects(obj)
	const Obj_Entry *obj;
{
	const Elf_Rela *rela;

	for (rela = obj->rela; rela < obj->relalim; rela++) {
		Elf_Addr        *where;
		const Elf_Sym   *def;
		const Obj_Entry *defobj;
		Elf_Addr         tmp;
		unsigned long	 symnum;

		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		symnum = ELF_R_SYM(rela->r_info);

		switch (ELF_R_TYPE(rela->r_info)) {
#if 1 /* XXX Should not be necessary. */
		case R_TYPE(JMP_SLOT):
#endif
		case R_TYPE(NONE):
			break;

		case R_TYPE(32):	/* word32 S + A */
		case R_TYPE(GLOB_DAT):	/* word32 S + A */
			def = _rtld_find_symdef(symnum, obj, &defobj, false);
			if (def == NULL)
				return -1;

			tmp = (Elf_Addr)(defobj->relocbase + def->st_value +
			    rela->r_addend);
			if (*where != tmp)
				*where = tmp;
			rdbg(("32/GLOB_DAT %s in %s --> %p in %s",
			    obj->strtab + obj->symtab[symnum].st_name,
			    obj->path, (void *)*where, defobj->path));
			break;

		case R_TYPE(RELATIVE):	/* word32 B + A */
			*where = (Elf_Addr)(obj->relocbase + rela->r_addend);
			rdbg(("RELATIVE in %s --> %p", obj->path,
			    (void *)*where));
			break;

		case R_TYPE(COPY):
			/*
			 * These are deferred until all other relocations have
			 * been done.  All we do here is make sure that the
			 * COPY relocation is not in a shared library.  They
			 * are allowed only in executable files.
			 */
			if (obj->isdynamic) {
				_rtld_error(
			"%s: Unexpected R_COPY relocation in shared library",
				    obj->path);
				return -1;
			}
			rdbg(("COPY (avoid in main)"));
			break;

		default:
			rdbg(("sym = %lu, type = %lu, offset = %p, "
			    "addend = %p, contents = %p, symbol = %s",
			    symnum, (u_long)ELF_R_TYPE(rela->r_info),
			    (void *)rela->r_offset, (void *)rela->r_addend,
			    (void *)*where,
			    obj->strtab + obj->symtab[symnum].st_name));
			_rtld_error("%s: Unsupported relocation type %ld "
			    "in non-PLT relocations\n",
			    obj->path, (u_long) ELF_R_TYPE(rela->r_info));
			return -1;
		}
	}
	return 0;
}

int
_rtld_relocate_plt_lazy(obj)
	const Obj_Entry *obj;
{
	const Elf_Rela *rela;
	int reloff;

	for (rela = obj->pltrela, reloff = 0; rela < obj->pltrelalim; rela++, reloff++) {
		Elf_Word *where = (Elf_Word *)(obj->relocbase + rela->r_offset);
		int distance;
		Elf_Addr *pltresolve;

		assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

		pltresolve = obj->pltgot + 8;

		if (reloff < 32768) {
	       		/* li	r11,reloff */
			*where++ = 0x39600000 | reloff;
		} else {
			/* lis  r11,ha(reloff) */
			/* addi	r11,l(reloff) */
			*where++ = 0x3d600000 | ha(reloff);
			*where++ = 0x396b0000 | l(reloff);
		}
		/* b	pltresolve */
		distance = (Elf_Addr)pltresolve - (Elf_Addr)where;
		*where++ = 0x48000000 | (distance & 0x03fffffc);
		/* __syncicache(where - 12, 12); */
	}

	return 0;
}

caddr_t
_rtld_bind(obj, reloff)
	const Obj_Entry *obj;
	Elf_Word reloff;
{
	const Elf_Rela *rela = obj->pltrela + reloff;
	Elf_Word *where = (Elf_Word *)(obj->relocbase + rela->r_offset);
	Elf_Addr value;
	const Elf_Sym *def;
	const Obj_Entry *defobj;
	int distance;

	assert(ELF_R_TYPE(rela->r_info) == R_TYPE(JMP_SLOT));

	def = _rtld_find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj, true);
	if (def == NULL)
		_rtld_die();

	value = (Elf_Addr)(defobj->relocbase + def->st_value);
	distance = value - (Elf_Addr)where;
	rdbg(("bind now/fixup in %s --> new=%p", 
	    defobj->strtab + def->st_name, (void *)value));

	if (abs(distance) < 32*1024*1024) {	/* inside 32MB? */
		/* b	value	# branch directly */
		*where = 0x48000000 | (distance & 0x03fffffc);
		__syncicache(where, 4);
	} else {
		Elf_Addr *pltcall, *jmptab;
		int N = obj->pltrelalim - obj->pltrela;
	
		/* Entries beyond 8192 take twice as much space. */
		if (N > 8192)
			N += N-8192;

		pltcall = obj->pltgot;
		jmptab = pltcall + 18 + N * 2;
	
		jmptab[reloff] = value;

		if (reloff < 32768) {
			/* li	r11,reloff */
			*where++ = 0x39600000 | reloff;
		} else {
			/* lis  r11,ha(reloff) */
			/* addi	r11,l(reloff) */
			*where++ = 0x3d600000 | ha(reloff);
			*where++ = 0x396b0000 | l(reloff);
		}
		/* b	pltcall	*/
		distance = (Elf_Addr)pltcall - (Elf_Addr)where;
		*where++ = 0x48000000 | (distance & 0x03fffffc);
		__syncicache(where - 12, 12);
	}

	return (caddr_t)value;
}
