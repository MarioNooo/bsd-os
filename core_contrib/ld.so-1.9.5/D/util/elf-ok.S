	.text
	.globl _start
_start:
#if defined(__i386__)
	movl $1,%eax
	movl $0,%ebx
	int  $0x80
#elif defined(__mc68000__)
	movel #1,%d0
	clrl %d1
	trap #0
#elif defined(__sparc__)
	mov  1,%g1
	mov  0,%o0
	t    0x10
#else
#error Only know how to support i386, m68k and sparc architectures
#endif
