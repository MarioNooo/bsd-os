/*	BSDI elf32.h,v 2.1 1997/10/25 15:33:32 donn Exp	*/

/*
 * This header file converts generic ELF type and structure names
 * into 32-bit-specific names, so that the same sources may be
 * built as either 32-bit or 64-bit ELF code.
 */

/*
 * Generic types.
 *
 * Note that 64-bit ELF structures use some 32-bit fields,
 * so be careful about coding the non-generic data type when necessary.
 */
#define	Elf_Addr		Elf32_Addr
#define	Elf_Off			Elf32_Off
#define	Elf_Sword		Elf32_Sword
#define	Elf_Word		Elf32_Word

/* Structured types.  */
#define	Elf_Ehdr		Elf32_Ehdr
#define	Elf_Shdr		Elf32_Shdr
#define	Elf_Sym			Elf32_Sym
#define	Elf_Rel			Elf32_Rel
#define	Elf_Rela		Elf32_Rela
#define	Elf_Phdr		Elf32_Phdr
#define	Elf_Dyn			Elf32_Dyn

/* A generic handle for testing binary file compatibility.  */
#define	ELFCLASS		ELFCLASS32

/* A macro that prepends _elf32_ to names, for generic definitions.  */
#define	_ELF_(s)		_elf32_ ## s
