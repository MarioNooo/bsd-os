/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI crt1.c,v 1.6 2001/10/03 17:29:52 polk Exp
 */

/*
 * C start-up code for the PowerPC under ELF.
 */

#include <sys/param.h>
#include <sys/elf.h>	/* XXX */
#include <sys/exec.h>
#include <errno.h>
#include <stdlib.h>

void _init(void);
void _fini(void);

extern void _sigtramp();
extern void __monstartup(unsigned long, unsigned long);
extern void __moncleanup(void);
extern int main(int, char **, char **);

extern char _etext;

struct ps_strings *__ps_strings;
char *__progname;
char **environ;

void
_start(int argc, char **argv, char **envp, auxv_t *auxv, void (*term)(void),
    struct ps_strings *psp)
{
	char *cp, *slash;

	/*
	 * Store the address of the signal trampoline in a known location.
	 */
	((unsigned *)psp)[-1] = (unsigned)_sigtramp;

	/*
	 * Initialize argument strings for setproctitle().
	 */
	__ps_strings = psp;

	/*
	 * Initialize the program name for err().
	 */
	for (cp = argv[0], slash = cp - 1; *cp; ++cp)
		if (*cp == '/')
			slash = cp;
	__progname = slash + 1;

	/*
	 * If the kernel or a shared library wants us to call
	 * a termination function, arrange to do so.
	 */
	if (term)
		atexit(term);

#ifdef GCRT1
	/*
	 * Arrange to start profiling,
	 * and write out profiling data on exit.
	 */
	__monstartup((unsigned long)&_start, (unsigned long)&_etext);
	atexit(__moncleanup);
#endif

	/*
	 * Arrange to execute the global destructors,
	 * and execute the global constructors.
	 */
	atexit(_fini);
	_init();

	/*
	 * Finally, run the program.
	 */
	errno = 0;
	environ = envp;
	exit(main(argc, argv, envp));
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
