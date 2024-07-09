/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI dial.h,v 1.1 1996/05/02 13:24:46 prb Exp
 */
typedef struct {
	char	*S[10];	/* String to send on nth step of logging in. */
	char	*E[10];	/* Strings to wait for on nth step of logging in */
			/* (after sending s0-s9) */
	char	*F[10];	/* Strings to send if no expected str appeared */
	long	 T[10];	/* Timeout in seconds for e0-e9 */
} dial_t;
