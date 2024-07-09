/*-
 * Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
 * The Wind River Systems Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI wint.h,v 2.1 2001/11/09 15:18:05 erwan Exp
 */

/*
 * wint.h --
 *
 *	Redefinition of wait macros.
 */

#ifndef	_WINT
#define	_WINT

#ifndef	_W_INT
#define	_W_INT(w)	(*(int *) &(w))

#ifdef	WIFEXITED
#undef	WIFEXITED
#endif	/* WIFEXITED */
#define	WIFEXITED(w)	(((_W_INT (w)) & 0xff) == 0)

#ifdef	WIFSIGNALED
#undef	WIFSIGNALED
#endif	/* WIFSIGNALED */
#define	WIFSIGNALED(w)	(((_W_INT (w)) & 0x7f) > 0 && 	\
			(((_W_INT (w)) & 0x7f) < 0x7f))

#ifdef	WIFSTOPPED
#undef	WIFSTOPPED
#endif	/* WIFSTOPPED */
#define	WIFSTOPPED(w)	(((_W_INT (w)) & 0xff) == 0xff)
#endif	/* _W_INT */

#endif	/* _WINT */
