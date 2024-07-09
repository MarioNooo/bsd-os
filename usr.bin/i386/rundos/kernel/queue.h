/*
 * Copyright (c) 1992, 1993, 1996
 *	Berkeley Software Design, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI queue.h,v 2.2 1996/04/08 19:31:41 bostic Exp
 */

#define QSIZE 1024

struct queue {
  unsigned char buffer[QSIZE];
  int head;
  int tail;
  int intno;
  int locked;
  struct queue *next;
};

static inline int check_queue_overflow (struct queue *q)
{
  if ( q->head == q->tail ) {
    if ( q->tail-- == 0 )
      q->tail = QSIZE-1;
    return 1;
  } else
    return 0;
}

static inline void put_char_q (struct queue *q, unsigned char ch)
{
/* check of buffer overflow !!! (it is but not here for kbd_queue) */
  q->buffer[q->tail++] = ch;
  if (q->tail == QSIZE)
    q->tail = 0;
}

static inline void put_char_to_head (struct queue *q, unsigned char ch)
{
/* check of buffer overflow !!! (it is but not here for kbd_queue) */
  if ( q->head-- == 0 )
    q->head = QSIZE-1;
  q->buffer[q->head] = ch;
}

static inline int queue_not_empty (struct queue *q)
{
  return (q->head != q->tail);
}

static inline int queue_empty (struct queue *q)
{
  return (q->head == q->tail);
}

static inline unsigned char get_char_q (struct queue *q)
{
  unsigned char ch = q->buffer[q->head++];

  if (q->head == QSIZE)
    q->head = 0;
  return (ch);
}

static inline unsigned char head_char (struct queue *q)
{
  return (q->buffer[q->head]);
}

static inline int queue_is_not_locked (struct queue *q)
{
  return (q->locked == 0);
}

static inline int queue_is_locked (struct queue *q)
{
  return (q->locked != 0);
}

static inline void lock_queue (struct queue *q)
{
  q->locked = 1;
}

static inline void unlock_queue (struct queue *q)
{
  q->locked = 0;
}

static inline void clear_queue (struct queue *q)
{
  q->head = q->tail = 0;
}

static inline int buffer_contents (struct queue *q)
{
  int num;
  
  num = q->head - q->tail;
  return (num < 0) ? QSIZE - num : num;
}

struct queue *create_queue (int intno);
