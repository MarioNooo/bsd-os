/*	BSDI libnsl_jump.s,v 2.2 1997/10/31 03:14:04 donn Exp	*/

/*
 * iBCS2 shared network services library jump table.
 * From iBCS2 p 6-7 through p 6-8...
 */

	.section ".text"

	.org 0x0000
	jmp t_accept

	.org 0x0005
	jmp t_alloc

	.org 0x000a
	jmp t_bind

	.org 0x000f
	jmp t_close

	.org 0x0014
	jmp t_connect

	.org 0x0019
	jmp t_error

	.org 0x001e
	jmp t_free

	.org 0x0023
	jmp t_getinfo

	.org 0x0028
	jmp t_getstate

	.org 0x002d
	jmp t_listen

	.org 0x0032
	jmp t_look

	.org 0x0037
	jmp t_open

	.org 0x003c
	jmp t_optmgmt

	.org 0x0041
	jmp t_rcv

	.org 0x0046
	jmp t_rcvconnect

	.org 0x004b
	jmp t_rcvdis

	.org 0x0050
	jmp t_rcvrel

	.org 0x0055
	jmp t_rcvudata

	.org 0x005a
	jmp t_rcvuderr

	.org 0x005f
	jmp t_snd

	.org 0x0064
	jmp t_snddis

	.org 0x0069
	jmp t_sndrel

	.org 0x006e
	jmp t_sndudata

	.org 0x0073
	jmp t_sync

	.org 0x0078
	jmp t_unbind

	.org 0x0fa8
	jmp _t_checkfd

	.org 0x0ff4
	jmp _t_aligned_copy

	.org 0x102c
	jmp _t_max

	.org 0x1044
	jmp _t_putback

	.org 0x1084
	jmp _t_is_event

	.org 0x10dc
	jmp _t_is_ok

	.org 0x1284
	jmp _t_do_ioctl

	.org 0x1314
	jmp _t_alloc_bufs

	.org 0x1510
	jmp _t_setsize

	.org 0x1538
	jmp _null_tiptr

	.org 0x1ddc
	jmp _snd_conn_req

	.org 0x1f44
	jmp _rcv_conn_con

	.org 0x2a0c
	jmp _alloc_buf
