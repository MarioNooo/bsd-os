#	BSDI crti.s,v 2.2 1998/07/29 16:14:51 donn Exp

#
#  Initial constructor/destructor glue code for the i386 ELF C start-up.
#

	.section ".init"
	.globl _init
	.type _init,@function
_init:
	pushl %ebp
	movl %esp,%ebp

	.section ".fini"
	.globl _fini
	.type _fini,@function
_fini:
	pushl %ebp
	movl %esp,%ebp
