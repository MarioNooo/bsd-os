/*	BSDI crti.s,v 1.2 2001/02/23 02:22:15 donn Exp	*/

/*
 * Initial constructor/destructor glue code for the Power PC C start-up.
 * XXX The ABI (chapter 4, section 'Tags') wants us to initialize tags here...
 */

	.section ".init"
	.globl _init
	.type _init,@function
	.balign 4
_init:
	mflr 0
	stwu 1,-16(1)
	stw 0,20(1)

	.section ".fini"
	.globl _fini
	.type _fini,@function
	.balign 4
_fini:
	mflr 0
	stwu 1,-16(1)
	stw 0,20(1)
