/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sequencer.h,v 1.3 1999/12/09 19:09:48 cp Exp
 */

typedef struct aic_format1 {
	u_int	f1_immediate		:8;
	u_int	f1_source		:9;
	u_int	f1_destination		:9;
	u_int	f1_return		:1;
	u_int	f1_opcode		:4;
	u_int	f1_format3		:1;
} aic_format1_t;

typedef struct aic_format2 {
	u_int	f2_shift_control	:8;
	u_int	f2_source		:9;
	u_int	f2_destination		:9;
	u_int	f2_return		:1;
	u_int	f2_opcode		:4;
	u_int	f2_format3		:1;
} aic_format2_t;

typedef struct aic_format3 {
	u_int	f3_immediate		:8;
	u_int	f3_source		:9;
	u_int	f3_address		:10;
	u_int	f3_opcode		:4;
	u_int	f3_format3		:1;
} aic_format3_t;

typedef struct sequencer {
	struct sequencer	*s_next;
	union {
	aic_format1_t	s_format1;
	aic_format2_t	s_format2;
	aic_format3_t	s_format3;
	u_int		s_bits;			/* perhaps should be bytes */
	} s_u;
	u_int		 s_lc;			/* location counter */
	u_int		 s_srcline;
} sequencer_t;

#define s_format1 s_u.s_format1
#define s_format2 s_u.s_format2
#define s_format3 s_u.s_format3
#define s_bits s_u.s_bits

/*
 * The following are not hardware real op codes. They
 * are used just between the parse code and the grammer code.
 * They are also psuedo op codes define by the assembler.
 */
#define AIC_S_SHIFTLEFT		0
#define AIC_S_SHIFTRIGHT	1
#define AIC_S_ROTATELEFT	2
#define AIC_S_ROTATERIGHT	3

extern sequencer_t *sequencer_alloc();
extern sequencer_t *sequencer_head;
extern void sequencer_passtwo();
extern location_counter;

extern section;
#define	AIC_S_CODE	0
#define AIC_S_SRAM	1
#define AIC_S_SCB	2
#define AIC_S_REG_RW	3
#define AIC_S_REG_RO	4
#define AIC_S_REG_WO	5
#define AIC_S_REG_AC	6	/* special semantics for this one */
#define AIC_S_REG_HO	7	/* only access from host */


/*
 * Some definitions to avoid chicken and egg problem
 */

#define	AIC_ALLZEROS	0x6a
#define	AIC_FLAGS	0x6b
#define	AIC_NONE	0x6a
#define	AIC_SCB		0xa0
#define	AIC_SINDEX	0x65
#define	AIC_SRAM	0x20
