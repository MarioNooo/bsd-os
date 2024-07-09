/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sequencer.c,v 1.1 1995/06/21 01:41:56 cp Exp
 */

#include <sys/types.h>
#include <stdlib.h>
#include "sequencer.h"

sequencer_t *sequencer_head = NULL;
sequencer_t *sequencer_tail = NULL;

int location_counter;
int section = AIC_S_CODE;
extern int yyline;

sequencer_t *
sequencer_alloc()
{
	sequencer_t *s;
	s = malloc(sizeof (*s));
	if (s == NULL) {
		printf("malloc failed\n");
		exit (1);
	}
	bzero(s, sizeof (*s));
	s->s_lc = location_counter;
	s->s_srcline = yyline;
	if (sequencer_head == NULL) {
		sequencer_head = s;
		sequencer_tail = s;
		return (s);
	}
	sequencer_tail->s_next = s;
	sequencer_tail = s;
	return (s);
}

void
sequencer_passtwo()
{
	sequencer_t *s, *s1;
	for (s = sequencer_head; s != NULL; s = s1)
	{
		s1 = s->s_next;
		free(s);
	}
	sequencer_head = NULL;
	sequencer_tail = NULL;
	location_counter = 0;
	section = AIC_S_CODE;
}
