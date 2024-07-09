/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI isppp.c,v 1.1 1996/03/29 03:42:07 prb Exp
 */

/*
 * All valid PPP LCP frames start with one of the following 3 sequences
 * The initial ~ is never caught.  We make sure it isn't below.
 * The first character is which character to try next in this sequence.
 * It is base 1 since it itself is the 0th character of the array.
 */
static char pppnames[][9] = {
	{ 1, /* 0x7e, */ 0xff, 0x03, 0xc0, 0x21, 0x00, },
	{ 1, /* 0x7e, */ 0xff, 0x7d, 0x23, 0xc0, 0x21, 0x00, },
	{ 1, /* 0x7e, */ 0x7d, 0xdf, 0x7d, 0x23, 0xc0, 0x21, 0x00, },
};

#define	npppnames	(sizeof(pppnames)/sizeof(pppnames[0]))

int
isppp(c)
	int c;
{
	int i;

	for (i = 0; i < npppnames; ++i) {
		if (c == pppnames[i][pppnames[i][0]]) {
			if (pppnames[i][++pppnames[i][0]] == '\0')
				return(1);
		} else
			pppnames[i][0] = 1;
	}
	return(0);
}
