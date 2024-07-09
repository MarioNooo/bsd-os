#	BSDI crtn.s,v 2.1 1997/07/17 05:35:15 donn Exp

#
# Terminal constructor/destructor glue code for the i386 ELF C start-up.
#

	.section ".init"
	leave
	ret

	.section ".fini"
	leave
	ret
