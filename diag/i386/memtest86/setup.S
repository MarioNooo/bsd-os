!
! setup.s is responsible for getting the system data from the BIOS,
! and putting them into the appropriate places in system memory.
! both setup.s and system has been loaded by the bootblock.
!
! 1-Jan-96 Modified by Chris Brady for use as a boot/loader for memtest-86.

#include "mtest.h"

.globl begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:
.text

entry start
start:

! ok, the read went well so we get current cursor position and save it for
! posterity.

	mov	ax,#INITSEG	! this is done in bootsect already, but...
	mov	ds,ax

! Get memory size (extended mem, kB)

        mov     ah,#0x88
        int     0x15
        mov     [0],ax

! now we want to move to protected mode ...

	cli			! no interrupts allowed !
	mov	al,#0x80	! disable NMI for the bootup sequence
	out	#0x70,al

! first we move the system to its rightful place

	mov	ax,#TESTSEG	! start of destination segment
	mov	bx,#TSTLOAD	! start of source segment
	cld			! 'direction'=0, movs moves forward
do_move:
	mov	es,ax		! destination segment
	add	ax,#0x100
	cmp	ax,#TESTSEG+TSTSIZE
	jz	end_move
	mov	ds,bx		! source segment
	add	bx,#0x100
	sub	di,di
	sub	si,si
	mov 	cx,#0x800
	rep
	movsw
	jmp	do_move

! then we load the segment descriptors

end_move:
	push	cs
	pop	ds
	lidt	idt_48		! load idt with 0,0
	lgdt	gdt_48		! load gdt with whatever appropriate

! that was painless, now we enable A20

	call    empty_8042
	mov	al,#0xD1		! command write
	out	#0x64,al
	call    empty_8042
	mov	al,#0xDF		! A20 on
	out	#0x60,al
	call    empty_8042

! Note that the short jump isn't strictly needed, althought there are
! reasons why it might be a good idea. It won't hurt in any case.
!
	mov	ax,#0x0001	! protected mode (PE) bit
	lmsw	ax		! This is it!
	jmp	flush_instr
flush_instr:
	mov	ax,#KERNEL_DS
	mov	ds,ax
	mov	ss,ax
	mov	ax,#0x0720
	mov	ebx,#0xb8000
	mov	ecx,#0xc0000
clrlp:
	mov	(ebx),ax
	inc	ebx
	inc	ebx
	cmp	ebx,ecx
	jnz	clrlp

	jmpi	TESTADR,KERNEL_CS	! jmp offset 2000 of segment 0x10 (cs)

! This routine checks that the keyboard command queue is empty
! (after emptying the output buffers)
!
! No timeout is used - if this hangs there is something wrong with
! the machine, and we probably couldn't proceed anyway.
empty_8042:
	call	delay
	in	al,#0x64	! 8042 status port
	test	al,#1		! output buffer?
	jz	no_output
	call	delay
	in	al,#0x60	! read it
	jmp	empty_8042
no_output:
	test	al,#2		! is input buffer full?
	jnz	empty_8042	! yes - loop
	ret
!
! Delay is needed after doing i/o
!
delay:
	.word	0x00eb			! jmp $+2
	ret

gdt:
	.word	0,0,0,0		! dummy

	.word	0,0,0,0		! unused

	.word	0x7FFF		! limit 128mb
	.word	0x0000		! base address=0
	.word	0x9A00		! code read/exec
	.word	0x00C0		! granularity=4096, 386

	.word	0x7FFF		! limit 128mb
	.word	0x0000		! base address=0
	.word	0x9200		! data read/write
	.word	0x00C0		! granularity=4096, 386

idt_48:
	.word	0			! idt limit=0
	.word	0,0x3			! idt base=0L

gdt_48:
	.word	0x800		! gdt limit=2048, 256 GDT entries
	.word	512+gdt,0x9	! gdt base = 0X9xxxx

.text
endtext:
.data
enddata:
.bss
endbss:
