/*
 * Copyright (c) 2003 Wind River Systems, Inc.  All rights reserved.
 *
 *	BSDI stdbool.h,v 2.2 2003/06/17 00:12:56 torek Exp
 */

#ifndef _STDBOOL_H_
#define	_STDBOOL_H_

#ifdef __cplusplus /* ??? should we even bother allowing __cplusplus here? */

#define	_bool	bool
#define	bool	bool
#define	true	true
#define	false	false

#else /* not C++ */

#define	bool	_Bool
#define	true	1
#define	false	0

#endif /* C++ */

#define	__bool_true_false_are_defined	1

#endif	/* _STDBOOL_H_ */
