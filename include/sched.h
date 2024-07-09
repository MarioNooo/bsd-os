/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI sched.h,v 1.3 2000/08/16 17:19:47 donn Exp 
 */

#ifndef	_SCHED_H_
#define	_SCHED_H_

#include <time.h>

#define	SCHED_FIFO	1	/* Preemptive priority scheduler */
#define	SCHED_RR	2	/* Preemptive priority scheduler w/quantum */
#define	SCHED_OTHER	3	/* Implementation defined scheduler */

struct sched_param {
	int	sched_priority;	/* priority for priority based schedulers */
};

/*
 * Scheduling function prototypes
 */
__BEGIN_DECLS
int	sched_yield __P((void));
__END_DECLS

#endif
