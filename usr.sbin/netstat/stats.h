/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI stats.h,v 2.2 2000/04/29 04:37:09 dab Exp
 */

/* Structures and defines used by the stats_xxx.h files. */

struct	stats {
	u_long	s_offset[2];	/* Offset to counter, or address if static */
	u_int	s_flags;	/* Type, flags */
	const char *s_fmt;	/* Format string */
	u_int	s_aux;		/* Aux info */
};

#define	S_FMT_5		0x01	/* One variable, not plural */
#define	S_FMT1		0x02	/* One variable, one plural */
#define	S_FMT1ES	0x04	/* One variable, "es" plural */
#define	S_FMT1IES	0x08	/* One variable, "ies" plural */
#define	S_FMT1_5	0x10	/* One variable w/plural, one w/o */
#define	S_FMT2		0x20	/* Two variables w/plural */
#define	S_FMT_DIRECT	0x40	/* Use direct offset to variable */
#define	S_FMT_SKIPZERO	0x80	/* Never display when zero */
#define	S_FMT_FLAG1	0x0100	/* Protocol specific flag */
#define	S_FMT_FLAG2	0x0200	/* Protocol specific flag */
#define	S_FMT_TYPE(f)	(f & 0x3F)	/* To remove the extra flags */

/* 
 * Programs including the stats_xxx.h files must define the following:
 * 
 * FMT(s)	Macro for appending and prepending strings to the
 *		printf format specifiers (i.e. "\t" s "\n")
 * FMT_I(s)	Same as above but for indented lines (i.e. "\t\t" s "\n")
 * FMT_Q	Format to use for printing a (u_quad_t) value
 * 		(i.e. "%qu")
 */
