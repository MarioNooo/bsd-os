/*	BSDI linux.c,v 1.2 2001/06/24 16:52:43 donn Exp	*/

#include <stdarg.h>
#include <stdio.h>
#include "transform.h"

static void aprintf(const char *, ...);
static void emit_kernel_function_epilogue(size_t, int);
static void emit_kernel_function_error_conversion(size_t);
static void emit_kernel_function_prologue(size_t, int);

/*
 * Generate an OS-specific prologue for a translated file.
 * Here we set up macros to deal with error handling,
 * which is somewhat unique and awkward on Linux.
 */
void
os_prologue()
{

	printf("#include <sys/types.h>\n");
	printf("#include <stddef.h>\n");
	printf("#include <sys/syscall.h>\n");

	/* Force inclusion of <errno.h> here to avoid #define conflicts.  */
	printf("#include <errno.h>\n\n");

	/* XXX we need a flag other than kflag to signify glibc emulation! */
	if (kflag) {
		printf("int *__errno_location(void);\n");
		printf("#undef errno\n");
		printf("#define errno (*__errno_location())\n\n");
	} else {
		printf("int *__normal_errno_location(void);\n");
		printf("#undef errno\n");
		printf("#define errno (*__normal_errno_location())\n\n");
	}

	printf("int __errno_out(int) __attribute__((__regparm__(1)));\n");
	printf("int %ssyscall() __attribute__((__regparm__(1)));\n",
	    native);
	printf("long long %sqsyscall() __attribute__((__regparm__(1)));\n\n",
	    native);

	printf("#define _EVALSTRING(s) _EVALSTRING1(s)\n");
	printf("#define _EVALSTRING1(s) #s\n\n");

	/* Test a syscall value to see whether the syscall flagged an error.  */
	printf("#define %serror(v) ((unsigned)(v) > 0xfffff000)\n", native);
	printf("#define %serror_ll(v) ((unsigned long long)(v) > "
	    "0xfffffffffffff000LL)\n", native);
	/* Given a return value from a syscall, produce the errno value.  */
	printf("#define %serror_val(v) (-(int)(v))\n", native);
	/* Given an errno constant, produce the syscall return value. */
	printf("#define %serror_retval(e) (-(e))\n", native);
}

/*
 * For the inline syscall code, print the standard goo that
 * begins and ends each output line, along with the variable contents.
 * The goal is to make both the code below and the output code readable.
 */
static void
aprintf(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);

	printf("\"");
	if (s[0] != '\0' && s[strlen(s) - 1] != ':')
		printf("\t");
	vprintf(s, ap);
	va_end(ap);

	printf("\\n\"\n");
}

void
os_emit_library_function(struct entry *e)
{
	struct func *f;
	struct overload *go;

	if (e->library)
		f = e->library;
	else if (e->generic)
		f = e->generic;
	else
		return;

	go = TAILQ_LAST(&f->o_head, overload_head);
	emit_aliases(e->name, go);

	if (f->body_type == BODY_FULL) {
		emit_full_function(e->name, f, 0);
		return;
	}

	/* All of the other body types are assembly language functions.  */
	printf("asm(\n");

	aprintf(".section \\\".text\\\"");
	aprintf(".align 4");
	aprintf(".type %s%s,@function", foreign, e->name);
	aprintf("%s%s:", foreign, e->name);

	switch (f->body_type) {
	case BODY_SYSCALL:
		aprintf("movl $\" _EVALSTRING(SYS_%s) \",%%eax", f->body_value);
		aprintf("lcall $7,$0");
		aprintf("jc 0f");
		aprintf("ret");
		aprintf("0:");

	error_common:
		aprintf("pushl %%ebx");
		aprintf("call 0f");
		aprintf("0:");
		aprintf("popl %%ebx");
		aprintf("addl $_GLOBAL_OFFSET_TABLE_+[.-0b],%%ebx");
		aprintf("call __errno_out@PLT");
		if (kflag) {
			aprintf("pushl %%eax");
			aprintf("call __errno_location@PLT");
			aprintf("popl %%ecx");
			aprintf("movl %%ecx,(%%eax)");
		} else {
			aprintf("movl errno@GOT(%%ebx),%%ecx");
			aprintf("movl %%eax,(%%ecx)");
		}
		aprintf("popl %%ebx");
		aprintf("movl $-1,%%eax");
		break;

	case BODY_VALUE:
		aprintf("movl $\" _EVALSTRING(%s) \",%%eax", f->body_value);
		break;

	case BODY_ERROR:
		aprintf("movl $\" _EVALSTRING(%s) \",%%eax", f->body_value);
		goto error_common;

	case BODY_DEFAULT:
	case BODY_FULL:
	case BODY_UNSET:
		fatal("internal error in os_emit_library_function");
		break;
	}

	aprintf("ret");
	aprintf("0:");
	aprintf(".size %s%s,0b-%s%s", foreign, e->name, foreign, e->name);

	printf(");\n\n");
}

static void
emit_kernel_function_prologue(size_t nparams, int syscall)
{
	size_t i;

	/*
	 * Since ECX and EDX are caller-saved registers,
	 * we have to save them if we call C code.
	 * Since we also must push those registers on the stack
	 * to pass them as parameters in some situations,
	 * this code takes care to save those registers only once.
	 * We assume that the C code does not modify stack parameters.
	 * Note that a syscall saves EDX 'for free', so don't save it twice.
	 */
	switch (nparams) {
	case 5:
		aprintf("pushl %%edi");
		/* fall through */
	case 4:
		aprintf("pushl %%esi");
		/* fall through */
	case 3:
		aprintf("pushl %%edx");
		aprintf("pushl %%ecx");
		aprintf("pushl %%ebx");
		break;
	case 2:
	case 1:
		if (!syscall)
			aprintf("pushl %%edx");
		aprintf("pushl %%ecx");
		aprintf("pushl %%ebx");
		break;

	default:
		if (!syscall)
			aprintf("pushl %%edx");
		aprintf("pushl %%ecx");
		for (i = nparams * sizeof (int); i > 0; )
			aprintf("pushl %u(%%ebx); ", i -= sizeof (int));
		break;
	}
}

static void
emit_kernel_function_epilogue(size_t nparams, int syscall)
{

	/*
	 * Restore the stack correctly, and restore ECX and EDX.
	 * We assume that the saved registers are unmodified.
	 */
	switch (nparams) {
	default:
		aprintf("addl $%u,%%esp", nparams * sizeof (int));
		/* fall through */
	case 0:
		aprintf("popl %%ecx");
		if (!syscall)
			aprintf("popl %%edx");
		break;

	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		aprintf("popl %%ebx");
		aprintf("popl %%ecx");
		if (nparams >= 3 || !syscall)
			aprintf("popl %%edx");
		if (nparams <= 3)
			break;
		aprintf("popl %%esi");
		if (nparams <= 4)
			break;
		aprintf("popl %%edi");
		break;
	}

	aprintf("iret");
}

static void
emit_kernel_function_error_conversion(size_t nparams)
{
	int save_ebx = nparams == 0 || nparams > 5;

	/*
	 * If the parameter count is 1-5, EBX is on the stack
	 * and it will be restored by the epilogue code.
	 * We assume that the native errno value is in EAX.
	 */
	if (save_ebx)
		aprintf("pushl %%ebx");
	aprintf("call 0f");
	aprintf("0:");
	aprintf("popl %%ebx");
	aprintf("addl $_GLOBAL_OFFSET_TABLE_+[.-0b],%%ebx");
	aprintf("call __errno_out@PLT");
	if (save_ebx)
		aprintf("popl %%ebx");
	aprintf("negl %%eax");
}

void
os_emit_kernel_function(struct entry *e)
{
	struct func *f;
	struct overload *go;
	struct parameter *p;
	size_t nparams = 0;

	if (e->kernel)
		f = e->kernel;
	else if (e->generic)
		f = e->generic;
	else
		return;

	if ((go = TAILQ_LAST(&f->o_head, overload_head)) == NULL)
		return;
	for (p = TAILQ_FIRST(&go->p_head); p != NULL; p = TAILQ_NEXT(p, list))
		++nparams;

	if (f->body_type == BODY_FULL)
		emit_full_function(e->name, f, 1);

	printf("asm (\n");

	aprintf(".section \\\".text\\\"");
	aprintf(".global %sglue_%s", native, e->name);
	aprintf(".type %sglue_%s,@function", native, e->name);
	aprintf(".align 4");
	aprintf("%sglue_%s:", native, e->name);

	switch (f->body_type) {

	case BODY_FULL:
		emit_kernel_function_prologue(nparams, 0);
		aprintf("call %s%s%s", native, kernel, e->name);
		emit_kernel_function_epilogue(nparams, 0);
		break;

	case BODY_SYSCALL:
		emit_kernel_function_prologue(nparams, 1);
		aprintf("movl $\" _EVALSTRING(SYS_%s) \",%%eax", f->body_value);
		aprintf("pushl %%edx");	/* save EDX in return EIP slot */
		aprintf("lcall $7,$0");
		aprintf("jc 9f");
		aprintf("8:");
		aprintf("popl %%edx");
		emit_kernel_function_epilogue(nparams, 1);
		aprintf("9:");
		emit_kernel_function_error_conversion(nparams);
		aprintf("jmp 8b");
		break;

	case BODY_ERROR:
		/* It's probably not worth optimizing this case further...  */
		emit_kernel_function_prologue(0, 0);
		aprintf("movl $\" _EVALSTRING(%s) \",%%eax", f->body_value);
		emit_kernel_function_error_conversion(0);
		emit_kernel_function_epilogue(0, 0);
		break;

	case BODY_VALUE:
		aprintf("movl $\" _EVALSTRING(%s) \",%%eax", f->body_value);
		aprintf("iret");
		break;

	case BODY_DEFAULT:
	case BODY_UNSET:
		fatal("internal error in os_emit_kernel_function");
		break;
	}

	aprintf("0:");
	aprintf(".size %sglue_%s,0b-%sglue_%s", native, e->name, native,
	    e->name);

	printf(");\n\n");
}
