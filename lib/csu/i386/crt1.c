/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI crt1.c,v 2.8 2001/10/03 17:29:52 polk Exp
 */

/*
 * Rescue some values that were passed in registers.
 */
asm (
	".section \".text\";"
	".global _start;"
"_start:"
	"movl %ebx,__ps_strings;"
	"movl %edx,_term;"
	"xorl %ebp,%ebp;"		/* do a favor for gdb */
	"jmp __start"
);

/*
 * C start-up code for the i386 under ELF.
 */

#include <sys/param.h>
#include <sys/elf.h>	/* XXX */
#include <sys/exec.h>
#include <errno.h>
#include <stdlib.h>

void _init(void);
void _fini(void);

static void (*_term)(void);

extern void __monstartup(unsigned long, unsigned long);
extern void __moncleanup(void);
extern void _start(void);
extern int main(int, char **, char **);

extern char _etext;

struct ps_strings *__ps_strings;
char *__progname;
char **environ;

void
__start(int argc)
{
	struct ps_strings *psp = __ps_strings;
	char *cp, *slash;

	/*
	 * Initialize the program name for err().
	 */
	for (cp = psp->ps_argv[0], slash = cp - 1; *cp; ++cp)
		if (*cp == '/')
			slash = cp;
	__progname = slash + 1;

	/*
	 * If the kernel or a shared library wants us to call
	 * a termination function, arrange to do so.
	 */
	if (_term)
		atexit(_term);

#ifdef GCRT1
	/*
	 * Arrange to start profiling,
	 * and write out profiling data on exit.
	 */
	__monstartup((unsigned long)&_start, (unsigned long)&_etext);
	atexit(__moncleanup);
#endif

	/* Set globals now, so that they will be available to constructors.  */
	errno = 0;
	environ = psp->ps_envp;

	/*
	 * Arrange to execute the global destructors,
	 * and execute the global constructors.
	 */
	atexit(_fini);
	_init();

	/*
	 * Finally, run the program.
	 */
	exit(main(psp->ps_argc, psp->ps_argv, environ));
}

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
