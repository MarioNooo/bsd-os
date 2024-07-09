/*	BSDI brk.x,v 1.5 2000/12/08 03:45:19 donn Exp	*/

/*
 * Handle the brk() syscall in glibc.  This would normally be part of
 * unistd.x, but libc5 and glibc implement it somewhat differently.
 */

include "types.xh"

int brk(const void *v)
{
	extern void *__curbrk;
	extern char _end;
	int i;

	if (__curbrk == NULL && !__bsdi_error(__bsdi_syscall(SYS_break, 0)))
		__curbrk = (void *)i;
	if (v == NULL)
		return (0);
	if (__bsdi_error(i = __bsdi_syscall(SYS_break, v)))
		return (i);
	__curbrk = (void *)v;
	return (0);
}

int __kernel_brk(const void *v)
{
	extern void *__curbrk;
	extern char _end;
	int i;

	if (__bsdi_error(i = __bsdi_syscall(SYS_break, v)))
		return ((int)__curbrk);
	if (v == NULL)
		return (i);
	return ((int)v);
}
