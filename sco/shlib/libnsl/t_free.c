/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_free.c,v 2.1 1995/02/03 15:19:49 polk Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

int
t_free(ptr, type)
	void *ptr;
	int type;
{
	t_unitdata_t	*u = ptr;

	DBG_ENTER(("t_free", phex, &ptr, phex, &type, 0));

	switch (type) {
	case T_UNITDATA:
		if (u->udata.maxlen)
			memfree(u->udata.buf);
	case T_BIND:
	case T_CALL:
	case T_UDERROR:
		if (u->addr.maxlen)
			memfree(u->addr.buf);
	case T_OPTMGMT:
	case T_DIS:
	case T_INFO:
		memfree(u);
	}
	DBG_EXIT(("t_free", 0, 0));
	return (0);
}
