#	BSDI Makefile.inc,v 1.4 1996/02/04 01:15:19 donn Exp

SRCS:=${SRCS:Nbrk.s:Nexect.s:Nsbrk.s:Nsetlogin.s}
SRCS+=	brk.c exect.c sbrk.c setlogin.c
SRCS+=	sigtramp.s lseek.s
SRCS:=${SRCS:Nftruncate.c:Nlseek.c:Nmmap.c:Nmount.s:Nptrace.s:Nreboot.s:Nsigreturn.s:Nsyscall.s:Ntruncate.c}
ASM+=   ftruncate.o mmap.o ptrace.o reboot.o sigreturn.o \
	syscall.o truncate.o
