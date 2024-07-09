/*	BSDI disks.h,v 2.1 1995/02/03 13:06:40 polk Exp	*/

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI disks.h,v 2.1 1995/02/03 13:06:40 polk Exp
 */

char	**dr_name;
int	dk_ndrive;
int	*dk_select;
struct	diskstats *dk_stats;
struct	diskstats *dk_ostats;

int	dkinit __P((void));
int	dkcmd __P((char *, char *));
