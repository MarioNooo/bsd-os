/*	BSDI crtn.s,v 1.1 1995/12/18 21:36:00 donn Exp	*/

/*
 * Terminal constructor/destructor glue code for the Power PC C start-up.
 * XXX The ABI (chapter 4, section 'Tags') wants us to finish tags here...
 */

	.section ".init"
	lwz 1,0(1)
	lwz 0,4(1)
	mtlr 0
	blr

	.section ".fini"
	lwz 1,0(1)
	lwz 0,4(1)
	mtlr 0
	blr
