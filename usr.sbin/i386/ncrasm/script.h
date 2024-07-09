/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI script.h,v 1.1 1995/06/21 01:49:56 cp Exp
 */

typedef struct ncr_blkmv {
	u_int	bm_byte_count		:24;
	u_int	bm_phase		:3;
	u_int	bm_opcode		:1;
	u_int	bm_table_indirect	:1;
	u_int	bm_indirect		:1;
	u_int	bm_itype		:2;
	u_int	bm_addr;	
} ncr_blkmv_t;

typedef struct ncr_memmv {
	u_int	mm_byte_count		:24;
	u_int	mm_res0			:6;
	u_int	mm_itype		:2;
	u_int	mm_source_addr;	
	u_int	mm_dest_addr;	
} ncr_memmv_t;

typedef struct ncr_ioi {
	u_int	io_res3			:3;
	u_int	io_sc_attn		:1;
	u_int	io_res2			:2;
	u_int	io_sc_ack		:1;
	u_int	io_res1			:2;
	u_int	io_sc_targetmode	:1;
	u_int	io_sc_carry		:1;
	u_int	io_res0			:5;
	u_int	io_scsi_id		:8;
	u_int	io_select_attn		:1;
	u_int	io_table_indirect	:1;
	u_int	io_relative		:1;
	u_int	io_opcode		:3;
	u_int	io_itype		:2;
	u_int	io_addr;	
} ncr_ioi_t;

typedef struct ncr_IOI {
	u_int	io_table_offset		:24;
	u_int	io_select_attn		:1;
	u_int	io_table_indirect	:1;
	u_int	io_relative		:1;
	u_int	io_opcode		:3;
	u_int	io_itype		:2;
	u_int	io_addr;	
} ncr_IOI_t;

typedef struct ncr_rwi {
	u_int	rw_res1			:8;
	u_int	rw_data			:8;
	u_int	rw_register		:7;
	u_int	rw_res0			:1;
	u_int	rw_carry_enable		:1;
	u_int	rw_operator		:2;
	u_int	rw_opcode		:3;
	u_int	rw_itype		:2;
	u_int	rw_addr;	
} ncr_rwi_t;


typedef struct ncr_tci {
	u_int	tc_data			:8;
	u_int	tc_mask			:8;
	u_int	tc_wait_phase		:1;
	u_int	tc_compare_phase	:1;
	u_int	tc_compare_data		:1;
	u_int	tc_branch_true		:1;
	u_int	tc_on_the_fly		:1;
	u_int	tc_carry_test		:1;
	u_int	tc_res0			:1;
	u_int	tc_relative		:1;
	u_int	tc_phase		:3;
	u_int	tc_opcode		:3;
	u_int	tc_itype		:2;
	u_int	tc_addr;	
} ncr_tci_t;

typedef struct ncr_bits {
	u_int	bi_data[3];
} ncr_bits_t;



#define NCR_BLKMV	0
#define NCR_IOI		1
#define NCR_RWI		1
#define NCR_TCI		2
#define NCR_MEMMV	3

typedef struct script {
	struct script	*s_next;
	u_int  	 	 s_triple;	
	union {
	ncr_blkmv_t	 s_ublkmv;
	ncr_memmv_t	 s_umemmv;
	ncr_ioi_t	 s_uioi;
	ncr_IOI_t	 s_uIOI;
	ncr_rwi_t	 s_urwi;
	ncr_tci_t	 s_utci;
	ncr_bits_t	 s_ubits;
	} s_u;
	u_int		 s_lc;			/* location counter */
	u_int		 s_w1_tablerelative;
	u_int		 s_w1_iorelative;
	u_int		 s_w2_tablerelative;
	u_int		 s_w2_iorelative;
	u_int		 s_srcline;
} script_t;

#define s_blkmv s_u.s_ublkmv
#define s_memmv s_u.s_umemmv
#define s_ioi	s_u.s_uioi
#define s_IOI	s_u.s_uIOI
#define s_rwi	s_u.s_urwi
#define s_tci	s_u.s_utci
#define s_bits	s_u.s_ubits

extern script_t *script_alloc();
extern script_t *script_head;
extern void script_passtwo();
extern location_counter;
