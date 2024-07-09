/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995, 1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI crt1.c,v 2.6 2001/10/03 17:29:52 polk Exp
 */

/*
 * C start-up code for the SPARC under ELF.
 */

#include <sys/param.h>
#include <sys/elf.h>	/* XXX */
#include <sys/exec.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void _init(void);
void _fini(void);

char **environ;
char *__progname;
struct ps_strings *__ps_strings;

#ifdef GCRT1
extern char _etext[];
extern void __monstartup(char *, char *);
extern void __moncleanup(void);
#endif

extern int main(int, char **, char **);

/*
 * %o0 holds ps_strings pointer.  For Solaris compat and/or shared
 * libraries, if %g1 is not 0, it is a routine to pass to atexit().
 * (By passing the pointer in the usual argument register, we avoid
 * having to do any inline assembly, except to recover %g1.)
 *
 * Note: kernel may (is not set in stone yet) pass ELF aux vector in %o1,
 * but for now we do not use it here.
 */
void
_start(struct ps_strings *psp)
{
	char *p, *s, **argv, **envp;
	void (*term)(void);

	/* Grab %g1 before it gets used for anything by the compiler. */
	asm volatile("mov %%g1,%0" : "=r"(term));

	__ps_strings = psp;
	argv = psp->ps_argv;
	environ = envp = psp->ps_envp;

	/* Set up program name for err(3). */
	if ((p = *argv) != NULL) {
		s = strrchr(p, '/');
		__progname = s ? s + 1 : p;
	} else
		__progname = "";

	/*
	 * If the kernel or a shared library wants us to call
	 * a termination function, arrange to do so.
	 */
	if (term)
		atexit(term);

#ifdef paranoid
	/*
	 * The standard I/O library assumes that file descriptors 0, 1, and 2
	 * are open. If one of these descriptors is closed prior to the start 
	 * of the process, I/O gets very confused. To avoid this problem, we
	 * insure that the first three file descriptors are open before calling
	 * main(). Normally this is undefined, as it adds two unnecessary
	 * system calls.
	 */
    {
	register int fd;
	do {
		fd = open("/dev/null", 2);
	} while (fd >= 0 && fd < 3);
	(void)close(fd);
    }
#endif

#ifdef GCRT1
	/* Begin profiling, and arrange to write out the data on exit. */
	__monstartup((char *)_start, _etext);
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
	exit(main(psp->ps_argc, argv, envp));
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
