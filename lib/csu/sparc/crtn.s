!	BSDI crtn.s,v 2.1 1998/08/17 05:59:59 torek Exp

!
! Terminal constructor/destructor glue code for the i386 ELF C start-up.
!

	.section ".init"
	ret
	 restore

	.section ".fini"
	ret
	 restore
