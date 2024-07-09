/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI script.c,v 1.2 1995/07/18 22:57:55 cp Exp
 */

#include <sys/types.h>
#include <stdlib.h>
#include "script.h"

script_t *script_head = NULL;
script_t *script_tail = NULL;

int location_counter;
extern int yyline;

script_t *
script_alloc()
{
	script_t *s;
	s = malloc(sizeof (*s));
	if (s == NULL) {
		printf("malloc failed\n");
		exit (1);
	}
	bzero(s, sizeof (*s));
	s->s_lc = location_counter;
	s->s_srcline = yyline;
	if (script_head == NULL) {
		script_head = s;
		script_tail = s;
		return (s);
	}
	script_tail->s_next = s;
	script_tail = s;
	return (s);
}

void
script_passtwo()
{
	script_t *s, *s1;
	for (s = script_head; s != NULL; s = s1)
	{
		s1 = s->s_next;
		free(s);
	}
	script_head = NULL;
	script_tail = NULL;
	location_counter = 0;
}
