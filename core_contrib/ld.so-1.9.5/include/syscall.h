/*	BSDI syscall.h,v 1.2 1998/09/09 05:31:59 torek Exp	*/

#ifndef _LINUX_SYSCALL_H_
#define	_LINUX_SYSCALL_H_ 1

/*
 * Some minimal glue to get the effect of Linux's syscall.h.
 */

#include <sys/syscall.h>

#define	_syscall1(rtype, name, atype1, arg1) \
	inline rtype \
	name(atype1 arg1) \
	{ \
		return (rtype)syscall(SYS_ ## name, arg1); \
	}

#ifdef __i386__

#define	__IBCS_exit	SYS_exit
#define	__IBCS_close	SYS_close
#define	__IBCS_mmap	SYS_mmap
#define	__IBCS_open	SYS_open
#define	__IBCS_write	SYS_write
#define	__IBCS_read	SYS_read
#define	__IBCS_mprotect	SYS_mprotect
#define	__IBCS_stat	SYS_stat
#define	__IBCS_munmap	SYS_munmap
#define	__IBCS_getuid	SYS_getuid
#define	__IBCS_getgid	SYS_getgid

#define _dl_mmap	_dl_linux_mmap
#include <i386/syscall.h>
#undef _dl_mmap

/* Linux inline mmap() modified for 64-bit off_t.  */
extern inline int
_dl_mmap(void *addr, size_t size, u_int prot, u_int flags, int fd,
    u_int f_offset)
{
	int malloc_buffer;

	asm volatile ("pushl $0\n\t"
	    "pushl %7\n\t"
	    "pushl $0\n\t"
	    "pushl %6\n\t"
	    "pushl %5\n\t"
	    "pushl %4\n\t"
	    "pushl %3\n\t"
	    "pushl %2\n\t"
	    "pushl $0\n\t"
	    "movl %1,%%eax\n\t"
	    "lcall $7,$0\n\t"
	    "jnb .+4\n\t"
	    "negl %0\n\t"
	    "addl $36,%%esp\n\t" :
	    "=a" (malloc_buffer) :
	    "a" (__IBCS_mmap), "rm" (addr), "rm" (size), "rm" (prot),
	    "rm" (flags), "rm" (fd), "rm" (f_offset));

	return (malloc_buffer);
}

#elif __sparc__

#define	__NR_exit	SYS_exit
#define	__NR_close	SYS_close
#define	__NR_mmap	SYS_mmap
#define	__NR_open	SYS_open
#define	__NR_write	SYS_write
#define	__NR_read	SYS_read
#define	__NR_mprotect	SYS_mprotect
#define	__NR_stat	SYS_stat
#define	__NR_munmap	SYS_munmap
#define	__NR_getuid	SYS_getuid
#define	__NR_geteuid	SYS_geteuid
#define	__NR_getgid	SYS_getgid
#define	__NR_getegid	SYS_getegid

/*
 * ANSI C preprocesing requires double expansion to expand X in:
 *	#define ANSWER 42
 * into "42".  STR0(ANSWER) gives "ANSWER" instead.
 */
#define STR0(x)	#x
#define	STR(x)	STR0(x)

#include <machine/trap.h>

#define trap	"t " STR(ST_SYSCALL) "\n\t"

#define _dl_mmap	_dl_linux_mmap
#include <sparc/syscall.h>
#undef _dl_mmap

/*
 * Our sparc mmap syscall needs the offset argument on the stack, so we want
 * to have the compiler call a function with properly stack-ized arguments,
 * rather than trying to do the entire thing inline.  This also lets us
 * use the G2RFLAG trick to return, instead of testing the carry.
 */
extern caddr_t __sys_mmap(caddr_t, size_t, int, int, int, int, off_t);

asm(".text; .align 4; .type __sys_mmap,#function; __sys_mmap:");
asm("	mov " STR(SYS_mmap) "|" STR(SYSCALL_G2RFLAG) ",%g1");
asm("   add %o7,8,%g2; t " STR(ST_SYSCALL) "; retl; mov -1,%o0");

extern inline int
_dl_mmap(void *addr, size_t size, u_int prot, u_int flags, int fd,
    u_int f_offset)
{

	return (int)__sys_mmap(addr, size, prot, flags, fd, 0, f_offset);
}

/* Our mmap just returns -1 on error. */
#undef _dl_mmap_check_error
#define	_dl_mmap_check_error(retval) ((int)(retval) == -1)

#else

whoops! unsupported architecture

#endif /* machine architecture */

#endif /* _LINUX_SYSCALL_H_ */
