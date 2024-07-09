/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI t_alloc.c,v 2.1 1995/02/03 15:19:24 polk Exp
 */

/*
 * NOT YET	Do something with allocating options.
 * CHECK ON	If T_UDATA is set should we return a length of zero instead
 *		of an error.
 */

#include "nsl_compat.h"
#include "nsl_debug.h"

static char *
alloc_noaddr(size, fields)
	size_t size;
	int fields;
{
	if (fields & T_ALL) {
		errno = EINVAL;
		t_errno = TSYSERR;
		tli_maperrno();
		return (NULL);
	}
	return ((char *)memalloc(size));
}

static char *
alloc_addr(size, fields)
	size_t size;
	int fields;
{
	bind_t *bp;	/* all structs have addr field first */
#if 0
	if (fields & (T_OPT | T_UDATA)) {
		errno = EINVAL;
		tli_maperrno();
		t_errno = TSYSERR;
		return (NULL);
	}
#endif
	if ((bp = memalloc(size)) == NULL)
		return (NULL);
	if (fields & (T_ADDR)) {
		bp->addr.buf = memalloc(sizeof (struct sockaddr));
		if (bp->addr.buf == NULL) {
			memfree(bp);
			return (NULL);
		}
		bp->addr.maxlen = sizeof (struct sockaddr);
	}
	return ((char *)bp);
}

char *
t_alloc(fd, type, fields)
	int fd, type, fields;
{
	ts_t *tsp;
	char *res;

	DBG_ENTER(("t_alloc", phex, &fd, phex, &type, phex, &fields, 0));


	if ((tsp = ts_get(fd)) == NULL) {
		DBG_EXIT(("t_accept TBADF", NULL, 0));
		return NULL;
	}

	/* have to do something different here for unix domain sockets */

	switch (type) {
	case T_BIND:
		 res = alloc_addr(sizeof (bind_t), fields);
		 break;

	case T_OPTMGMT:
		 res = alloc_noaddr(sizeof (optmgmt_t), fields);
		 break;

	case T_CALL:
		 res = alloc_addr(sizeof (call_t), fields);
		 break;

	case T_DIS:
		 res = alloc_noaddr(sizeof (discon_t), fields);
		 break;

	case T_UNITDATA:
	{
		unitdata_t	*u;
		u = (unitdata_t*)alloc_addr(sizeof (*u), fields & ~T_UDATA);
		if (u == NULL)
			return (NULL);
		if (!(fields & T_UDATA))
			return ((char *)u);
		u->udata.buf = memalloc(2048);
		if (u->udata.buf == NULL) {
			t_free(u, T_UNITDATA);
			return (NULL);
		}
		u->udata.maxlen = 2048;
		res = (char *)u;
		break;
	}

	case T_INFO:
		res = alloc_noaddr(sizeof (info_t), fields);
		break;

	case T_UDERROR:
		res = alloc_addr(sizeof (uderr_t), fields);
		break;
	}
	DBG_EXIT(("t_alloc", 0, phex, &res, 0));
	return (res);
}
