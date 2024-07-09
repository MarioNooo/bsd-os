/*	BSDI libc_data.s,v 2.2 1997/10/31 03:14:02 donn Exp	*/

/*
 * Offsets to public shared library data.
 * From iBCS2 p 6-3...
 */

	.section ".data"

	.org 0x00
	.global __Fp_Used
__Fp_Used:

	.org 0x04
	.global _libc__ctype
_libc__ctype:

	.org 0x08
	.global _libc_malloc
_libc_malloc:

	.org 0x0c
	.global _libc_realloc
_libc_realloc:

	.org 0x10
	.global _libc_free
_libc_free:

	.org 0x14
	.global _libc__allocs
_libc__allocs:

	.org 0x18
	.global _libc__sibuf
_libc__sibuf:

	.org 0x1c
	.global _libc__sobuf
_libc__sobuf:

	.org 0x20
	.global _libc__smbuf
_libc__smbuf:

	.org 0x24
	.global _libc__iob
_libc__iob:

	.org 0x28
	.global _libc__lastbuf
_libc__lastbuf:

	.org 0x2c
	.global _libc__bufendtab
_libc__bufendtab:

	.org 0x30
	.global _libc_end
_libc_end:

	.org 0x34
	.global _libc__cleanup
_libc__cleanup:

	.org 0x38
	.global _libc_environ
_libc_environ:

	.org 0x3c
	.global opterr
opterr:
	.long 1

	.org 0x40
	.global optind
optind:
	.long 1

	.org 0x44
	.global optopt
optopt:

	.org 0x48
	.global optarg
optarg:

	.org 0x4c
	.global errno
errno:

	.org 0x94
	.global __fltused
__fltused:
	.space 4
