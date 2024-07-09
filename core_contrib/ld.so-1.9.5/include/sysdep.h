/*	BSDI sysdep.h,v 1.2 1998/09/09 05:43:10 torek Exp	*/

#ifdef __i386__

#include <i386/sysdep.h>

/*
 * Add a hack to set the ps_strings pointer correctly.
 * Note that the register constraints serve to get the given values
 * into the appropriate, specific registers before we jump.
 */
#undef START
#define	START() \
	asm volatile ("movl %3,%%ebx\n\t" \
	    "leave\n\t" \
	    "jmp *%2\n\t" : \
	    "=a" (status) : "d" (_dl_interpreter_exit), "a" (_dl_elf_main), \
	    "m" (dl_data[AT_PSSTRINGS])); \

#elif __sparc__

#include <sparc/sysdep.h>

#undef START
#define START() \
	asm volatile("mov %1,%%i0; clr %%g1; call %0; restore" :: \
	    "r"(_dl_elf_main), "r"(dl_data[AT_PSSTRINGS]) : "g1", "o0")

#else

unsupported architecture

#endif
