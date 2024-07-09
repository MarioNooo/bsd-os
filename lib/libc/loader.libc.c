/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1994,1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI loader.libc.c,v 2.7 2001/10/03 17:29:52 polk Exp
 */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <machine/shlib.h>

LIBC_ENTRY("loader_libc");

/*
 * Initial internal bootstrap for fixed-address static shared libraries.
 * Called from a stub in programs that use static shared libraries.
 * The loader is always presumed to be embedded in the C library.
 */

extern unsigned long _minbrk;
extern unsigned long _curbrk;

void loader_libc(int, char **, char **, auxv_t *, void (*)(void),
    struct ps_strings *);
static void clean_env(char **);

char **environ = 0;
char *__progname = 0;
struct ps_strings *__ps_strings = 0;

static void
clean_env(char **envp)
{
	char *s;

	while ((s = *envp++) != NULL) {
		if (s[0] != 'L' || s[1] != 'D' || s[2] != '_')
			continue;
		if (strncmp(s + 3, "PRELOAD=", 8) == 0)
			s[11] = '\0';
		else if (strncmp(s + 3, "LIBRARY_PATH=", 13) == 0)
			s[16] = '\0';
	}
}

struct shlib_info {
	struct ldtab *lp;
	/* Variable-length array of library and application init()/fini().  */
	struct initfini ia[1];
};

/*
 * We assume that the kernel has already loaded
 * the C library and started us at our requested entry point.
 * That means that we have to dig stuff out of registers ourselves...
 */
void
loader_libc(int _argc, char **_argv, char **_envp, auxv_t *_ap,
    void (*_term)(void), struct ps_strings *_psp)
{
	int (*main)(int, char **, char **);
	void (*fp)(struct ldtab *, int, int, struct initfini *);
#ifdef NO_PARAMETERS
	struct ps_strings *psp = __ps_strings;
	int argc = psp->ps_argc;
	char **argv = psp->ps_argv;
	char **envp = psp->ps_envp;
	auxv_t *ap = (auxv_t *)&psp->ps_envp[psp->ps_nenv + 1];
	void (*term)(void) = NULL;
#else
	int argc = _argc;
	char **argv = _argv;
	char **envp = _envp;
	auxv_t *ap = _ap;
	void (*term)(void) = _term;
	struct ps_strings *psp = _psp;
#endif
	Elf32_Phdr *pht;
	struct ldtab *lp;
	struct shlib_info *ei;
	struct initfini *ip;
	struct initfini *ip_last;
	char *p, *s;
	uid_t euid, ruid;
	gid_t egid, rgid;
	unsigned long heap;
	int phent, phnum;
	int pagesz;
	int f;
	int initfini_count;
	int i;

	for (; ap->a_type != AT_NULL; ++ap)
		switch (ap->a_type) {
		case AT_EGID:
			egid = ap->a_un.a_val;
			break;
		case AT_ENTRY:
			main = (int (*)(int, char **, char **))ap->a_un.a_fcn;
			break;
		case AT_EUID:
			euid = ap->a_un.a_val;
			break;
		case AT_GID:
			rgid = ap->a_un.a_val;
			break;
		case AT_PAGESZ:
			pagesz = ap->a_un.a_val;
			break;
		case AT_PHDR:
			pht = ap->a_un.a_ptr;
			break;
		case AT_PHENT:
			phent = ap->a_un.a_val;
			break;
		case AT_PHNUM:
			phnum = ap->a_un.a_val;
			break;
		case AT_UID:
			ruid = ap->a_un.a_val;
			break;
		}

	/* Protect dynamic libraries from some simple attacks.  */
	if (euid != ruid || egid != rgid)
		clean_env(envp);

	/* Scan the PHT looking for the data segment.  */
	for (; phnum > 0; --phnum, pht = (Elf32_Phdr *)((caddr_t)pht + phent))
		if (pht->p_type == PT_LOAD && pht->p_flags & PF_W)
			break;

	/* Pull interesting data out of data space.  */
	ei = (struct shlib_info *)pht->p_vaddr;
	ip = ei->ia;

	/* The end of the data segment is the start of the heap.  */
	heap = pht->p_vaddr + pht->p_memsz;

	/* Set up well-known global variables.  */
	_minbrk = _curbrk = heap;
	environ = envp;
	__ps_strings = psp;
	if ((p = *argv) != NULL) {
		s = strrchr(p, '/');
		__progname = s ? s + 1 : p;
	} else
		__progname = "";

	/* Perform any machine-dependent initializations.  */
	DO_LIBC_INIT();
	DO_INIT(ip);

	/* Load all remaining shared library segments.  */
	for (lp = ei->lp + 1, ++ip; lp->name != NULL; ++lp, ++ip) {
		if ((f = open(lp->name, O_RDONLY)) == -1 ||
		    __mmap(lp->address, pagesz, PROT_READ|PROT_EXEC,
		      MAP_PRIVATE|MAP_FIXED, f, 0, 0) == (caddr_t)-1) {
			write(STDERR_FILENO, "can't open `", 12);
			write(STDERR_FILENO, lp->name, strlen(lp->name));
			write(STDERR_FILENO, "'\n", 2);
			__abort();
		}

		/* We assume that the loader function is in the first page.  */
		fp = LIB_START(lp->address);
		(*fp)(lp, f, pagesz, ip);
		close(f);
	}
	ip_last = ip;		/* includes the application's init/fini */

	/* Take care of destructors and constructors.  */
	if (term != NULL)
		atexit(term);
	for (ip = ei->ia; ip <= ip_last; ++ip) {
		if (ip->fini != NULL)
			atexit(ip->fini);
		(*ip->init)();
	}

	/* Run the program.  */
	exit((*main)(argc, argv, envp));
}

LIBRARY
