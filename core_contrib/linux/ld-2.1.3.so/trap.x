/*	BSDI trap.x,v 1.1 2001/01/23 03:56:42 donn Exp	*/

%{

/*
 * Install the user mode trap handler for raw/inline Linux system calls.
 * The handler itself is generated automatically by a shell script.
 */

#include <machine/segments.h>
#include <sys/elf.h>		/* XXX native header! */
#include <signal.h>
#include <unistd.h>

/* XXX Hacked in to avoid including ldsodefs.h...  */
int _dl_catch_error(char **, void (*)(void *), void *)
    __attribute__((__regparm__(3), __stdcall__));
Elf32_Addr _dl_lookup_symbol(const char *, const Elf32_Sym **, void **,
    const char *, int) __attribute__((__regparm__(3), __stdcall__));
extern void *_dl_global_scope[];

static void __bsdi_init_inline_linux_syscall(void)
    __attribute__((__constructor__));

static const char trapmsg[] = "can't enable Linux syscalls\n";

struct lookup_args {
	Elf32_Addr base;
	const Elf32_Sym *sym;
};

static void
lookup(void *arg)
{
	struct lookup_args *l = arg;

	l->sym = NULL;
	l->base = _dl_lookup_symbol("__bsdi_inline_syscall_handler", &l->sym,
	    _dl_global_scope, NULL, 0);
}

/*
 * This code assumes that we've loaded liblinux.so.2.
 * We need to initialize syscall traps in the dynamic linker,
 * since we may have to service inline syscalls before
 * we execute a constructor for the C or liblinux libraries.
 */
static void
__bsdi_init_inline_linux_syscall()
{
	union descriptor d;
	struct lookup_args l;
	u_int32_t v;
	int sel;
	char *s;

	if (_dl_catch_error(&s, lookup, &l) != 0) {
		free(s);
		return;
	}
	v = (u_int32_t)l.base + l.sym->st_value;

	sel = GSEL(IDTVEC2IDX(LINUX_VEC), SEL_UPL);
	d.gd.gd_looffset = v;
	d.gd.gd_selector = LSEL(LUCODE_SEL, SEL_UPL);
	d.gd.gd_stkcpy = 0;
	d.gd.gd_xx = 0;
	d.gd.gd_type = SDT_SYS386TGT;
	d.gd.gd_dpl = SEL_UPL;
	d.gd.gd_p = 1;
	d.gd.gd_hioffset = v >> 16;

	if (__bsdi_syscall(SYS_setdescriptor, sel, &d) == -1) {
		__bsdi_syscall(SYS_write, STDERR_FILENO, trapmsg,
		    sizeof (trapmsg) - 1);
		__bsdi_syscall(SYS_kill, __bsdi_syscall(SYS_getpid), SIGABRT);
		__bsdi_syscall(SYS_exit, 255);
	}
}

%}
