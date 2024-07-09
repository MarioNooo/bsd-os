/*	BSDI elf64.h,v 2.2 1998/09/10 20:50:58 torek Exp	*/

/*
 * This header file converts generic ELF type and structure names
 * into 64-bit-specific names, so that the same sources may be
 * built as either 32-bit or 64-bit ELF code.
 */

/*
 * Generic types.
 */
#define	Elf_Addr		Elf64_Addr
#define	Elf_Off			Elf64_Off
#define	Elf_Sword		Elf64_Sxword
#define	Elf_Word		Elf64_Xword

/* Structured types.  */
#define	Elf_Ehdr		Elf64_Ehdr
#define	Elf_Shdr		Elf64_Shdr
#define	Elf_Sym			Elf64_Sym
#define	Elf_Rel			Elf64_Rel
#define	Elf_Rela		Elf64_Rela
#define	Elf_Phdr		Elf64_Phdr
#define	Elf_Dyn			Elf64_Dyn

/* A generic handle for testing binary file compatibility.  */
#define	ELFCLASS		ELFCLASS64

/* A macro that prepends _elf64_ to names, for generic definitions.  */
#define	_ELF_(s)		_elf64_ ## s
