/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI symbol.h,v 1.1 1995/06/21 01:50:21 cp Exp
 */

typedef struct symbol {
	struct	symbol *s_next;
	char	*s_name;
	u_int	 s_value;
	int	 s_len;
	int	 s_set;
	int	 s_setpass2;
	int	 s_tag;
} symbol_t;

extern symbol_t *getsym(char*);
extern symbol_t *symlist;

