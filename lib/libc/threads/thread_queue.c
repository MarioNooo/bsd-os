/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI thread_queue.c,v 1.9 2000/08/24 15:54:30 jch Exp
 */

#include <pthread.h>

#include "thread_private.h"
#include "thread_trace.h"

int
_thread_queue_init(queue)
	struct pthread_queue *queue;
{

	TR("_thread_queue_init", queue, 0);

	/* Initialise the pointers in the queue structure: */
	queue->q_head = NULL;

	return (0);
}

/*
 * Priority Queue ENQ, DEQ, REMOVE operations. 
 */

void
_thread_queue_enq(queue, thread)
	struct pthread_queue * queue;
	struct pthread * thread;
{
	struct pthread *cur, *prev;

	TR("_thread_queue_enq", queue, thread);

#ifdef DEBUG
	if (thread->queue || thread->qnxt) {
		TR("_thread_queue_enq_error1", queue, thread);
		abort();
	}
#endif

	thread->queue = queue;
	thread->qnxt = NULL;

	if (queue->q_head == NULL) {
		queue->q_head = thread;
		return;
	}

#ifdef DEBUG
	TR("_thread_queue_enq2", queue->q_head, queue->q_head->qnxt);
#endif

	for (prev = cur = queue->q_head; 
	     cur != NULL && cur->priority >= thread->priority;) {
#ifdef DEBUG
		if (cur == thread || cur->qnxt == thread) {
			TR("_thread_queue_enq_error", queue, thread);
			abort();
			return;
		}
#endif 
		prev = cur;
		cur = cur->qnxt;
	}

	thread->qnxt = cur;

	/* check for priority greater than the first element on queue */
	if (prev == cur)
		queue->q_head = thread;
	else
		prev->qnxt = thread;

	return;
}

struct pthread *
_thread_queue_get(queue)
	struct pthread_queue * queue;
{

	TR("_thread_queue_get", queue, queue->q_head);

	/* Return the pointer to the next thread in the queue: */
	return (queue->q_head);
}

struct pthread *
_thread_queue_deq(queue)
	struct pthread_queue * queue;
{
	struct pthread *thread = NULL;

	TR("_thread_queue_deq", queue, queue->q_head);

	if (queue->q_head) {
		thread = queue->q_head;
		queue->q_head = thread->qnxt;
#ifdef DEBUG
		if (thread && thread == queue->q_head) {
			TR("_thread_queue_deq_error", queue, queue->q_head);
			printf("bad qnxt pointer (%x) on dequeue\n", thread);
		}
#endif
		thread->queue = NULL;
		thread->qnxt = NULL;
	}

	return (thread);
}

int
_thread_queue_remove(queue, thread)
	struct pthread_queue * queue;
	struct pthread * thread;
{
	struct pthread *current, *prev;
	int ret = -1;

	TR("_thread_queue_remove", queue, thread);

	prev = current = queue->q_head;

	while (current) {
		if (current == thread) {
			prev->qnxt = current->qnxt;
			if (prev == current)
				queue->q_head = current->qnxt;
			TR("_thread_queue_remove_success", current, prev);
			ret = 0;
			break;
		}
		prev = current;
		current = current->qnxt;
	}
	thread->queue = NULL;
	thread->qnxt = NULL;

	return (ret);
}

/*
 * thread_queue_member() - returns 1 if thread is on the indicated
 * queue, 0 if not.
 */
int
_thread_queue_member(queue, thread)
	struct pthread_queue * queue;
	struct pthread * thread;
{
	struct pthread *current;
	int ret = 0;

	TR("_thread_queue_member", queue, thread);

	current = queue->q_head;

	while (current) {
		TR("_thread_queue_member_check", current, 0);
		if (current == thread) {
			TR("_thread_queue_member_success", current, 0);
			ret = 1;
			break;
		}
		current = current->qnxt;
	}
	return (ret);
}
