/*
 *  linux/boot/head.S
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 *  head.S contains the 32-bit startup code.
 *
 *  1-Jan-96 Modified by Chris Brady for use as a boot/loader for MemTest-86.
 *  Setup the memory management for flat non-paged linear addressing.
 */

.text

#include <linux/linkage.h>
#include "mtest.h"

#ifndef SYMBOL_NAME
#define SYMBOL_NAME(X) _/**/X
#endif

.globl SYMBOL_NAME(memsz)

startup_32:
	cld
	cli
	movl $(KERNEL_DS),%eax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs
	mov %ax,%ss
	mov $(TESTADR),%esp
/*
 * start system 32-bit setup. We need to re-do some of the things done
 * in 16-bit mode for the "real" operations.
 */
	xorl %eax,%eax
a20:	incl %eax		# check that A20 really IS enabled
	movl %eax,0x000000	# loop forever if it isn't
	cmpl %eax,0x100000
	je a20
/*
 * copy the memory size value 
 */
	movw (0x90000),%ax
	movw %ax,SYMBOL_NAME(memsz)

        lgdt gdt_descr
        ljmp $(KERNEL_CS),$flush
flush:  movl $(KERNEL_DS),%eax  # reload all the segment registers
        mov %ax,%ds             # after changing gdt.
        mov %ax,%es
        mov %ax,%fs
        mov %ax,%gs
	mov %ax,%ss

        call SYMBOL_NAME(do_test)

gdt_descr:
        .word (16)*8-1
        .long _gdt

.align 4
_gdt:
        .quad 0x0000000000000000        /* NULL descriptor */
        .quad 0x0000000000000000        /* not used */
        .quad 0x00c09a000000ffff        /* 0x10 main 256mb code at 0x000000 */
        .quad 0x00c092000000ffff        /* 0x18 main 256mb data at 0x000000 */
        .quad 0x00c09a100000ffff        /* 0x20 relo 256mb code at 0x100000 */
        .quad 0x00c092100000ffff        /* 0x28 relo 256mb data at 0x100000 */
