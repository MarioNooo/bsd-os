/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI asm.h,v 1.5 2000/01/24 21:56:26 prb Exp
 */
struct statement {
	u_short		icnt;		/* initial instruction # */
	u_short 	code;
	u_long  	jt;
	u_long  	jf;
	int32_t 	k;
#ifdef	BPF_128
	q128_t		q;
#endif
	char		*src;
	char		*iface;
	int		line;
	int		pc:16;		/* Up to 64K instructions! */
	int		needk:1;	/* needs to be defined */
	int		needt:1;
	int		needf:1;
	int		relocatek:1;	/* label found, but needs relocating */
	int		relocatet:1;
	int		relocatef:1;
	int		reachable:1;	/* set if we can ever reach here */
	int		target:1;	/* set if we are jumped to */
	unsigned	mask:5;		/* mask for jnet32 instruction */
	int		visited;	/* have we been here before */
	int32_t		value;		/* possible ending value */
	struct statement *next;
	struct statement *prev;
	struct statement *same;
	struct statement **callers;	/* Everyone who can jump to us */
	int		ncallers;	/* How many there are */
	int		maxcallers;	/* How many we have room for */
};
