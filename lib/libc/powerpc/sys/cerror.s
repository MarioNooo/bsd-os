/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995, 2001 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cerror.s,v 1.3 2001/10/03 17:29:53 polk Exp
 */

/*
 * We no longer branch to __cerror() on syscall errors,
 * but we retain this entry point for compatibility.
 */

#undef PROF

#include "SYS.h"
#include "ERRNO.h"

ENTRY(__cerror)
	SET_ERRNO(3)
	li 3,-1
	blr
ENDENTRY(__cerror)
