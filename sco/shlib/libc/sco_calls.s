/*	BSDI sco_calls.s,v 2.3 1997/10/31 03:14:02 donn Exp	*/

#include "SYS.h"

#undef LCALL
#define	LCALL(x,y)	lcall $LSEL(LDEFCALLS_SEL,SEL_UPL),$0

/* from iBCS2 p 3-31 to 3-34 */

#define	SYS_sco__exit		 1
#define	SYS_sco_read		 3
#define	SYS_sco_write		 4
#define	SYS_sco_open		 5
#define	SYS_sco_close		 6
#define	SYS_sco_creat		 8
#define	SYS_sco_unlink		10
#define	SYS_sco_chdir		12
#define	SYS_sco_time		13
#define	SYS_sco_chmod		15
#define	SYS_sco_brk		17
#define	SYS_sco_stat		18
#define	SYS_sco_lseek		19
#define	SYS_sco_getpid		20
#define	SYS_sco_fstat		28
#define	SYS_sco_utime		30
#define	SYS_sco_access		33
#define	SYS_sco_kill		37
#define	SYS_sco_signal		48
#define	SYS_sco_ioctl		54
#define	SYS_sco_fcntl		62
#define	SYS_sco_select	      9256
#define	SYS_sco_sigprocmask  10280

RSYSCALL(sco__exit)
RSYSCALL(sco_access)
RSYSCALL(sco_brk)
RSYSCALL(sco_chdir)
RSYSCALL(sco_chmod)
RSYSCALL(sco_close)
RSYSCALL(sco_creat)
RSYSCALL(sco_fcntl)
RSYSCALL(sco_fstat)
RSYSCALL(sco_getpid)
RSYSCALL(sco_ioctl)
RSYSCALL(sco_kill)
RSYSCALL(sco_lseek)
RSYSCALL(sco_open)
RSYSCALL(sco_read)
RSYSCALL(sco_select)
RSYSCALL(sco_signal)
RSYSCALL(sco_sigprocmask)
RSYSCALL(sco_stat)
RSYSCALL(sco_time)
RSYSCALL(sco_unlink)
RSYSCALL(sco_utime)
RSYSCALL(sco_write)

#define	SCO_SET(name) \
	.global name,_syscall_sys_/**/name; \
	.set name,sco_/**/name; \
	.set _syscall_sys_/**/name,sco_/**/name

SCO_SET(_exit)
SCO_SET(close)
SCO_SET(fcntl)
SCO_SET(getpid)
SCO_SET(kill)
SCO_SET(lseek)
SCO_SET(read)
SCO_SET(select)
SCO_SET(write)

	.section ".text"
	.global sco_cerror
	.align 4
sco_cerror:
	SET_ERRNO(%eax)
	movl $-1,%eax
	ret
