/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI thread_attr_default.c,v 1.1 1996/12/11 19:34:49 donn Exp
 */

#include <pthread.h>
#include <stddef.h>

#include "thread_private.h"

#define DEF_FLAGS (				\
		   PTHREAD_ATTR_INITED |	\
		   PTHREAD_CREATE_JOINABLE |	\
		   PTHREAD_EXPLICIT_SCHED |	\
		   PTHREAD_SCOPE_PROCESS	\
		  )

/* Default thread attributes: */
pthread_attr_t pthread_attr_default = {
        DEF_FLAGS,
	NULL, 
	PTHREAD_STACK_DEFAULT,
	SCHED_RR,
	{ 
		PTHREAD_DEFAULT_PRIORITY 
	}
};

int _thread_attr_default_pad[16] = { 0 };
