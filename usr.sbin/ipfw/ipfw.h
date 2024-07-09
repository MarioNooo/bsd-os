/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI ipfw.h,v 1.1 1997/06/16 15:12:53 prb Exp
 */

struct ipfw_header {
	u_long	magic;	/* magic number */
	u_long	icnt;	/* number of instructions in filter */
	u_long	rcnt;	/* number of relocations in filter */
	u_char	type;	/* type of filter */
	u_char	reserved[3];
};

#define	IPFW_MAGIC	0x88955DD
