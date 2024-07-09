/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI fs.h,v 1.2 1996/07/29 22:42:02 polk Exp	*/

#include <sys/param.h>
#if	_BSDI_VERSION > 199312
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#else
#include	<ufs/fs.h>
#endif
