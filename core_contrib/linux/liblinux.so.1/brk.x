/*	BSDI brk.x,v 1.2 2001/06/24 16:51:25 donn Exp	*/

/*
 * Handle the brk() syscalls in libc5.
 */

include "types.xh"

%{
extern void *___brk_addr;
extern char _end;
int __init_brk();
%}

int brk(void *v)
{
	int e;

	if (__init_brk() != 0)
		return (__bsdi_error_retval(ENOMEM));
	if (v == NULL)
		return (0);
	if (__bsdi_error(e = __bsdi_syscall(SYS_break, v)))
		return (e);
	___brk_addr = v;
	return (0);
}

/* We declare sbrk() as int because it returns -1 for errors...  */
int sbrk(int incr)
{
	void *old_break;
	void *new_break;
	int e;

	if (__init_brk() != 0)
		return (__bsdi_error_retval(ENOMEM));
	old_break = ___brk_addr;
	new_break = (char *)old_break + incr;
	if (__bsdi_error(e = __bsdi_syscall(SYS_break, new_break)))
		return (e);
	___brk_addr = new_break;
	return ((int)old_break);
}

/*
 * We make the brk/sbrk emulations call this function
 * in case it gets interposed.
 */
int __init_brk()
{
	if (___brk_addr == NULL)
		___brk_addr = &_end;
	return (0);
}
