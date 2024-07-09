/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI yacc.h,v 1.2 1999/09/08 06:18:46 prb Exp
 */
typedef struct e e_t;

struct e {
	e_t	*next;
	struct	in_addr addr;
	struct	in_addr mask;
	u_short		port;
	u_short		endport;
	int		protocol;
};
