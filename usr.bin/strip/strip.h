/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strip.h,v 2.2 2002/02/18 11:04:57 benoitp Exp
 */

#ifdef TORNADO
#include <sys/bsdosTypes.h>

#ifdef __CYGWIN__

#ifndef SIZE_T_MAX
#define SIZE_T_MAX	UINT_MAX
#endif

#ifndef EFTYPE
#define EFTYPE		EINVAL
#endif

#endif	/* __CYGWIN__ */

#endif	/* TORNADO */

#ifndef O_BINARY
#define O_BINARY	0
#endif

/*
 * Could do these in line, but that would violate data hiding...
 */
int	is_aout __P((const void *, size_t));
int	is_elf32 __P((const void *, size_t));
int	is_elf64 __P((const void *, size_t));

/*
 * These routines return the size of the stripped file on success,
 * or -1 if there was an error.
 */
ssize_t aout_stab __P((void *, size_t));
ssize_t aout_sym __P((void *, size_t));
ssize_t elf32_stab __P((void *, size_t));
ssize_t elf32_sym __P((void *, size_t));
ssize_t elf64_stab __P((void *, size_t));
ssize_t elf64_sym __P((void *, size_t));
