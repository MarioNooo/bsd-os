/*
 * Copyright (c) 1993,1994,1995 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "emulate_start.c,v 2.7 2003/06/26 20:48:16 polk Exp";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <machine/segments.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"		/* for load_coff() declaration */
#include "sco_ops.h"
#include "sco_sig_state.h"

vm_offset_t emulate_stack;
char *__progname;
char **environ;
struct ps_strings *__ps_strings;
int bsd;
int *program_edx;
int *program_eflags;
#ifdef DEBUG
int debug;
#endif

static void emulate_ldt __P((vm_offset_t));

/*
 * This is the first routine called in the emulator library.
 * It loads the program and jumps to it.
 * Also, all global initialization is performed here.
 */
int
emulate_start()
{
	extern struct genericops bsd_generictab;
	extern char start;
	char *s;

	emulate_stack = ((vm_offset_t)&start) & ~PDROFSET;

	/* This is an input AND output parameter... */
	program_edx = (int *)(emulate_stack - 12);

	/* This is only an input parameter; we stomp on output EFLAGS. */
	program_eflags = (int *)(emulate_stack - 36);

	/* Since we didn't load crt0.o ... */
	if (__ps_strings->ps_argc > 0)
		__progname = __ps_strings->ps_argv[0];
	else
		__progname = "emulator";

	emulate_ldt((vm_offset_t)(bsd ? bsd_emulate_glue : emulate_glue));

	emulate_init_break();

	environ = __ps_strings->ps_envp;

#ifdef DEBUG
	if ((s = getenv("EMULATE_DEBUG")) && getuid() == geteuid())
		debug = atoi(s);
	if (s = getenv("EMULATE_DEBUG_FD"))
		stderr->_file = atoi(s);		/* XXX */

	if (debug & DEBUG_BREAKPOINT)
		asm volatile ("int3");
#endif

	if (!bsd)
		program_init_break();

	if (bsd)
		generic = &bsd_generictab;

	if (curdir = getenv("EMULATE_PWD"))
		curdir = strdup(curdir);

	sig_state = SIG_UNWIND;

	return (0);
}

/*
 * Switch the system call gate to point at the emulator instead.
 * 'glue' is the address of an assembly routine which dispatches calls.
 */
static void
emulate_ldt(glue)
	vm_offset_t glue;
{
	union descriptor cgt;
	int sel = LSEL(LDEFCALLS_SEL, SEL_UPL);

	cgt.gd.gd_type = SDT_SYS386CGT;
	cgt.gd.gd_p = 1;
	cgt.gd.gd_selector = LSEL(LUCODE_SEL, SEL_UPL);
	cgt.gd.gd_stkcpy = 0;
	cgt.gd.gd_xx = 0;
	cgt.gd.gd_dpl = SEL_UPL;
	cgt.gd.gd_looffset = glue;
	cgt.gd.gd_hioffset = glue >> 16;

	if (setdescriptor(sel, &cgt) == -1)
		err(1, "can't set system call gate");
}

int
nosys(n)
	int n;
{

#ifdef DEBUG
	if (debug & DEBUG_SYSCALLS)
		warnx("unsupported system call %d", n);
#endif
	kill(getpid(), SIGSYS);		/* most likely, instant death */

	/* Actually, emulate_glue() ignores the return value.  */
	errno = ENOSYS;
	return (-1);
}
