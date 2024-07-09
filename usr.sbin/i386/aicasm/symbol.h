/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI symbol.h,v 1.2 1999/12/09 19:09:48 cp Exp
 */

typedef struct symbol {
	struct	symbol *s_next;
	char	*s_name;
	u_int	 s_value;
	int	 s_len;
	int	 s_set;
	int	 s_setpass2;
	int	 s_tag;
	int	 s_etag;	/* external tag */
	int	 s_section;
	int	 s_define;
} symbol_t;

extern symbol_t *getsym(char*);
extern symbol_t *symlist;
extern int getsymvalue(symbol_t*);

