/*	BSDI libnsl_data.s,v 2.2 1997/10/31 03:14:04 donn Exp	*/

/*
 * Offsets to public NSL shared library data.
 * from iBCS2 p 6-7 and p 6-8.
 */

	.section ".data"

	.org 0x0004
	.global _ti_user
_ti_user:

	.org 0x0038
	.global t_errno
t_errno:

	.org 0x003c
	.global t_nerr
t_nerr:

	.org 0x0070
	.global t_errlist
t_errlist:

	.org 0x0404
	.global sco__calloc
sco__calloc:

	.org 0x0408
	.global sco__errno
sco__errno:

	.org 0x0410
	.global sco__free
sco__free:

	.org 0x04d4
	.global ibc__calloc
ibc__calloc:


	.org 0x04d8
	.global ibc__errno
ibc__errno:


	.org 0x04e0
	.global ibc__free
ibc__free:


	.org 0x0514
	.global tiusr_statetbl
tiusr_statetbl:
	.space 9 * 25

	.align 8
