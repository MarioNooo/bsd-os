!	BSDI crti.s,v 2.1 1998/08/17 05:59:58 torek Exp

!
!  Initial constructor/destructor glue code for the sparc ELF C start-up.
!

	.section ".init"
	.globl _init
	.type _init,#function
_init:
	save %sp,-96,%sp

	.section ".fini"
	.globl _fini
	.type _fini,#function
_fini:
	save %sp,-96,%sp
