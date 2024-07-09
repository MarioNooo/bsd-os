/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI symbol.c,v 1.1 1995/06/21 01:42:39 cp Exp
 */

#include <sys/types.h>
#include "symbol.h"
#include <stdio.h>
#include <stdlib.h>

symbol_t *symlist;
extern int pass;
extern int yyline;

symbol_t *
getsym(char *name)
{
	symbol_t *sp;
	int len = strlen(name);

	for (sp = symlist; sp != NULL; sp = sp->s_next) {
		if (len != sp->s_len)
			continue;
		if (strncmp(name, sp->s_name, len) == 0)
			return (sp);
	}
	sp = malloc(sizeof *sp);
	bzero(sp, sizeof *sp);
	sp->s_name = malloc(len + 1);
	sp->s_len = len;
	strcpy(sp->s_name, name);
	sp->s_next = symlist;
	symlist = sp;
	return (sp);
}

getsymvalue(symbol_t  *s)
{
	if ( pass == 2 && s->s_set == 0) {
		fprintf(stderr, "symbol %s undefined line %d\n",
			s->s_name, yyline);
	}
	return s->s_value;
}
