/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_list.h,v 1.1 1996/06/03 15:24:49 mdickson Exp
 */

/*
 * Macros to deal with the management of the "allthreads" and
 * "deadthreads" linked lists.
 */

#ifndef	_THREAD_LIST_H_
#define	_THREAD_LIST_H_

extern struct pthread *	_thread_allthreads;
extern struct pthread *	_thread_deadthreads;

#define	_thread_list_init(listp)				\
	*(listp) = NULL;

#define	_thread_list_add(listp,threadp) { 			\
	(threadp)->nxt = *(listp);				\
	*(listp) = (threadp);					\
	}

#define	_thread_list_delete(listp, threadp) {			\
	if (*(listp) == (threadp)) 				\
	    *(listp) = (threadp)->nxt;				\
	else {							\
	    struct pthread * indexp;				\
	    indexp = (*listp);					\
	    while (indexp != NULL && indexp->nxt != threadp)	\
		indexp = indexp->nxt;				\
	    if (indexp != NULL) 				\
		indexp->nxt = (threadp)->nxt;			\
	    }							\
	}

#endif
