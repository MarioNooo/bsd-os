/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * BSDI $Id: 
 */

#ifndef _THREAD_EVTLOG_H
#define _THREAD_EVTLOG_H

#include <sys/types.h>
#include <sys/ptrace.h>
#include <pthread.h>

#include <sys/evtlog.h>

typedef struct {
	unsigned long 		traceflag;
	struct pthread *	curthd;
} thread_evtlog_t;

extern  thread_evtlog_t _thread_evtlog;

#ifdef WV_INST

#define _thread_run (_thread_evtlog.curthd)
#define _thread_evtlogInst (_thread_evtlog.traceflag)

#define THREAD_OBJ_USER_INIT()					\
	_thread_evtlogInst = 					\
	ptrace(PT_EVENT, 0, (caddr_t)&_thread_evtlogInst, PT_EVT_EVTLOG_INIT)

/* bufSize + wv_event_t + timestamp = 4+2+4 */
#define THREAD_EVT_OBJ_0_SIZE		10
#define THREAD_EVT_OBJ_SIZE(n)		\
	(THREAD_EVT_OBJ_0_SIZE + n * sizeof(int))

#define THREAD_EVT_OBJ_MAX_PARAM	4
#define THREAD_EVT_OBJ_MAX_SIZE		\
	(THREAD_EVT_OBJ_SIZE(THREAD_EVT_OBJ_MAX_PARAM))

#define THREAD_EVT_OBJ_USER_0(mask, evtId)				\
	if (_thread_evtlogInst & mask) {				\
		char		evtBuffer[THREAD_EVT_OBJ_MAX_SIZE];	\
		int *		intBase = (int *)evtBuffer;		\
		wv_event_t * 	evtBase = (wv_event_t *)(evtBuffer+4);	\
									\
		EVT_STORE_UINT32(intBase, THREAD_EVT_OBJ_SIZE(0));	\
		EVT_STORE_UINT16(evtBase, evtId);			\
		intBase = (int *)evtBase;				\
		EVT_STORE_UINT32(intBase, 0); /* timestamp */		\
		if (ptrace(PT_EVENT, 0, (caddr_t)evtBuffer, PT_EVT_EVTLOG)) \
			_thread_evtlogInst = 0;				\
	}

#define THREAD_EVT_OBJ_USER_0_IF(cond, mask, evtId)			\
	if (cond) {							\
		THREAD_EVT_OBJ_USER_0(mask, evtId);			\
	}

#define THREAD_EVT_OBJ_USER_1(mask, evtId, arg1)			\
	if (_thread_evtlogInst & mask) {				\
		char		evtBuffer[THREAD_EVT_OBJ_MAX_SIZE];	\
		int *		intBase = (int *)evtBuffer;		\
		wv_event_t * 	evtBase = (wv_event_t *)(evtBuffer+4);	\
		int		evtArg1 = (int)arg1;			\
									\
		EVT_STORE_UINT32(intBase, THREAD_EVT_OBJ_SIZE(1));	\
		EVT_STORE_UINT16(evtBase, evtId);			\
		intBase = (int *)evtBase;				\
		EVT_STORE_UINT32(intBase, 0); /* timestamp */		\
		EVT_STORE_UINT32(intBase, evtArg1);			\
		if (ptrace(PT_EVENT, 0, (caddr_t)evtBuffer, PT_EVT_EVTLOG)) \
			_thread_evtlogInst = 0;				\
	}

#define THREAD_EVT_OBJ_USER_1_IF(cond, mask, evtId, arg1)		\
	if (cond) {							\
	    THREAD_EVT_OBJ_USER_1(mask, evtId, arg1);			\
	}

#define THREAD_EVT_OBJ_USER_2(mask, evtId, arg1, arg2)			\
	if (_thread_evtlogInst & mask) {				\
		char		evtBuffer[THREAD_EVT_OBJ_MAX_SIZE];	\
		int	*	intBase = (int *)evtBuffer;		\
		wv_event_t * 	evtBase = (wv_event_t *)(evtBuffer+4);	\
		int		evtArg1 = (int)arg1;			\
		int		evtArg2 = (int)arg2;			\
									\
		EVT_STORE_UINT32(intBase, THREAD_EVT_OBJ_SIZE(2));	\
		EVT_STORE_UINT16(evtBase, evtId);			\
		intBase = (int *)evtBase;				\
		EVT_STORE_UINT32(intBase, 0); /* timestamp */		\
		EVT_STORE_UINT32(intBase, evtArg1);			\
		EVT_STORE_UINT32(intBase, evtArg2);			\
		if (ptrace(PT_EVENT, 0, (caddr_t)evtBuffer, PT_EVT_EVTLOG)) \
			_thread_evtlogInst = 0;				\
	}

#define THREAD_EVT_OBJ_USER_2_IF(cond, mask, evtId, arg1, arg2)		\
	if (cond) {							\
	    THREAD_EVT_OBJ_USER_2(mask, evtId, arg1, arg2);		\
	}


#define THREAD_EVT_OBJ_USER_3(mask, evtId, arg1, arg2, arg3)		\
	if (_thread_evtlogInst & mask) {				\
		char		evtBuffer[THREAD_EVT_OBJ_MAX_SIZE];	\
		int *		intBase = (int *)evtBuffer;		\
		wv_event_t * 	evtBase = (wv_event_t *)(evtBuffer+4);	\
		int		evtArg1 = (int)arg1;			\
		int		evtArg2 = (int)arg2;			\
		int		evtArg3 = (int)arg3;			\
									\
		EVT_STORE_UINT32(intBase, THREAD_EVT_OBJ_SIZE(3));	\
		EVT_STORE_UINT16(evtBase, evtId);			\
		intBase = (int *)evtBase;				\
		EVT_STORE_UINT32(intBase, 0); /* timestamp */		\
		EVT_STORE_UINT32(intBase, evtArg1);			\
		EVT_STORE_UINT32(intBase, evtArg2);			\
		EVT_STORE_UINT32(intBase, evtArg3);			\
		if (ptrace(PT_EVENT, 0, (caddr_t)evtBuffer, PT_EVT_EVTLOG)) \
			_thread_evtlogInst = 0;				\
	}

#define THREAD_EVT_OBJ_USER_4(mask, evtId, arg1, arg2, arg3, arg4)	\
	if (_thread_evtlogInst & mask) {				\
		char		evtBuffer[THREAD_EVT_OBJ_MAX_SIZE];	\
		int *		intBase = (int *)evtBuffer;		\
		wv_event_t * 	evtBase = (wv_event_t *)(evtBuffer+4);	\
		int		evtArg1 = (int)arg1;			\
		int		evtArg2 = (int)arg2;			\
		int		evtArg3 = (int)arg3;			\
		int		evtArg4 = (int)arg4;			\
									\
		EVT_STORE_UINT32(intBase, THREAD_EVT_OBJ_SIZE(4));	\
		EVT_STORE_UINT16(evtBase, evtId);			\
		intBase = (int *)evtBase;				\
		EVT_STORE_UINT32(intBase, 0); /* timestamp */		\
		EVT_STORE_UINT32(intBase, evtArg1);			\
		EVT_STORE_UINT32(intBase, evtArg2);			\
		EVT_STORE_UINT32(intBase, evtArg3);			\
		EVT_STORE_UINT32(intBase, evtArg4);			\
		if (ptrace(PT_EVENT, 0, (caddr_t)evtBuffer, PT_EVT_EVTLOG)) \
			_thread_evtlogInst = 0;				\
	}

#else /* WV_INST */

#define THREAD_OBJ_USER_INIT()
#define THREAD_EVT_OBJ_USER_0(mask, evtId)
#define THREAD_EVT_OBJ_USER_0_IF(cond, mask, evtId)
#define THREAD_EVT_OBJ_USER_1(mask, evtId, arg1)
#define THREAD_EVT_OBJ_USER_1_IF(cond, mask, evtId, arg1)
#define THREAD_EVT_OBJ_USER_2(mask, evtId, arg1, arg2)
#define THREAD_EVT_OBJ_USER_2_IF(cond, mask, evtId, arg1, arg2)
#define THREAD_EVT_OBJ_USER_3(mask, evtId, arg1, arg2, arg3)
#define THREAD_EVT_OBJ_USER_4(mask, evtId, arg1, arg2, arg3, arg4)

#endif /* WV_INST */

#endif  
