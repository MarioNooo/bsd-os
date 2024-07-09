/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 * BSDI thread_spec.c,v 1.14 2000/08/18 18:03:36 jch Exp
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

struct pthread_key {
	void	(*destructor) ();
	int     flags;
#define PTHREAD_KEY_INITED      0x0001
};

/* Lets us use these routines in non-threaded applications */
void **_thread_initial_specific_data;

/* Static variables: */
static struct pthread_key * _thread_key_table;
static pthread_once_t _thread_key_table_initialized = PTHREAD_ONCE_INIT;

static 
void _thread_key_init()
{

	TR("_thread_key_init", 0, 0);

	_thread_key_table = calloc(PTHREAD_DATAKEYS_MAX,
	   sizeof(struct pthread_key));
	return;
}

int
pthread_key_create(key, destructor)
	pthread_key_t *key;
	void (*destructor) __P((void *));
{
	int i;

	TR("pthread_key_create", key, destructor);

	_thread_kern_enter();
	_thread_once(&_thread_key_table_initialized, _thread_key_init);

	if (_thread_key_table == NULL) {
		_thread_kern_exit();
		return (ENOMEM);
	}

	for (i = 0; i < PTHREAD_DATAKEYS_MAX; i++) {
		if ((_thread_key_table[i].flags & PTHREAD_KEY_INITED) == 0) {
			_thread_key_table[i].flags |= PTHREAD_KEY_INITED;
			_thread_key_table[i].destructor = destructor;
			if (key)
				*key = i;
			_thread_kern_exit();
			return (0);
		}
	}

	_thread_kern_exit();
	return (EAGAIN);
}

int
pthread_key_delete(key)
	pthread_key_t key;
{
	int ret;

	TR("pthread_key_delete", key, 0);

	_thread_kern_enter();
	_thread_once(&_thread_key_table_initialized, _thread_key_init);

	if (_thread_key_table == NULL) {
		_thread_kern_exit();
		return (EINVAL);
	}

	if ((key >= 0) && (key < PTHREAD_DATAKEYS_MAX)) {
		_thread_key_table[key].destructor = NULL;
		_thread_key_table[key].flags &= ~PTHREAD_KEY_INITED;
		ret = 0;
	} else 
		ret = EINVAL;

	_thread_kern_exit();
	return (ret);
}

int 
pthread_setspecific(key, value)
	pthread_key_t key;
	const void * value;
{
	struct pthread * selfp;
	void **specific_data;

	TR("pthread_setspecific", key, value);

	_thread_kern_enter();
	_thread_once(&_thread_key_table_initialized, _thread_key_init);

	if (_thread_key_table == NULL) {
		_thread_kern_exit();
		return (ENOMEM);
	}

	if ((_thread_key_table[key].flags & PTHREAD_KEY_INITED) == 0) {
		_thread_kern_exit();
		return (EINVAL);
	}

	if ((selfp = _thread_selfp()) == NULL)
		specific_data = _thread_initial_specific_data;
	else
		specific_data = selfp->specific_data;

	if (specific_data == NULL &&
	    (specific_data = calloc(PTHREAD_DATAKEYS_MAX, sizeof(void *)))
	    == NULL) {
		_thread_kern_exit();
		return (ENOMEM);
	}

	if (selfp == NULL)
		_thread_initial_specific_data = specific_data;
	else
		selfp->specific_data = specific_data;

	specific_data[key] = (void *)value;

	_thread_kern_exit();
	return (0);
}

void * 
pthread_getspecific(key)
	pthread_key_t key;
{
	struct pthread *selfp;
	void **specific_data;

	TR("pthread_getspecific", key, _thread_key_table);

	_thread_kern_enter();
	_thread_once(&_thread_key_table_initialized, _thread_key_init);

	if ((_thread_key_table == NULL) ||
	    ((_thread_key_table[key].flags & PTHREAD_KEY_INITED) == 0)) {
		_thread_kern_exit();
		return (NULL);
	}

	if ((selfp = _thread_selfp()) == NULL)
		specific_data = _thread_initial_specific_data;
	else
		specific_data = selfp->specific_data;

	TR("pthread_getspecific1", specific_data,
	    specific_data ? specific_data[key] : NULL);

	_thread_kern_exit();
	return ((specific_data != NULL) ? specific_data[key] : NULL);
}

/* 
 * Called from pthread_exit to call any destructors for
 * thread specific data elements. This routine is called
 * with the thread monitor locked and leaves it that way.
 */
void 
_thread_key_cleanupspecific()
{
	struct pthread * selfp;
	void * data;
	int i, key, did_stuff;

	selfp = _thread_selfp();

	TR("_thread_key_cleanupspecific", selfp, selfp->specific_data);

	if (selfp->specific_data == NULL ||
	    selfp->specific_data == _thread_initial_specific_data)
		return;

	for (i = 0; i < PTHREAD_DESTRUCTOR_ITERATIONS; i++) {

		did_stuff = 0;

		for (key = 0; key < PTHREAD_DATAKEYS_MAX; key++) {
			if (_thread_key_table == 0)
				break;
			if ((_thread_key_table[key].flags &
			   PTHREAD_KEY_INITED) == 0)
				continue;

			if (_thread_key_table[key].destructor == NULL) 
				continue;

			if ((data = selfp->specific_data[key]) != NULL) {
				/* 
				 * kernel is locked here. If user calls a
				 * routine which tries to lock again
				 * then kernel aborts
				 */
				_thread_kern_exit();
				TR("_thread_key_cleanupspecific_destructor",
				   key, data);
				_thread_key_table[key].destructor(data);
				_thread_kern_enter();
				selfp->specific_data[key] = NULL;
				did_stuff++;
			}
		}

		if (did_stuff == 0)
			break;
	}

	free(selfp->specific_data);
	selfp->specific_data = NULL;
	return;
}
