/*	BSDI elf64.h,v 2.4 2002/04/19 22:08:39 donn Exp	*/

/*
 * This header file converts generic ELF type and structure names
 * into 64-bit-specific names, so that the same sources may be
 * built as either 32-bit or 64-bit ELF code.
 */

/*
 * Generic types.
 *
 * Note that 64-bit ELF structures use some 32-bit fields,
 * so be careful about coding the non-generic data type when necessary.
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

/* A macro that inserts elf64 in names, for generic definitions.  */
#define	ELFNN(s,t)		s ## elf64 ## t

#define SWAP_WORD(val) \
(!cross_endian ? val : \
    (((u_int64_t)SWAP_32((u_int32_t)((u_int64_t)(val) & 0xffffffff)) << 32) | \
    (u_int64_t)SWAP_32((u_int32_t)(((u_int64_t)(val) >> 32) & 0xffffffff))))

#define SWAP_SHORT(val) \
(!cross_endian ? val : \
    (((u_int16_t)((val) & 0x00ff) << 8) | \
    ((u_int16_t)((val) & 0xff00) >> 8)))
