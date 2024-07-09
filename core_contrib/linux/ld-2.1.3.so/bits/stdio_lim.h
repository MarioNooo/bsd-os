/*	BSDI stdio_lim.h,v 1.1 2000/02/12 17:16:05 donn Exp	*/

/* Copyright (C) 1994, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if !defined _STDIO_H && !defined __need_FOPEN_MAX
# error "Never include <bits/stdio_lim.h> directly; use <stdio.h> instead."
#endif

#ifdef _STDIO_H
# define L_tmpnam 1
# define TMP_MAX 0
# define FILENAME_MAX 14

# ifdef __USE_POSIX
#  define L_ctermid 1024
#  define L_cuserid 16
# endif
#endif

#if defined __need_FOPEN_MAX || defined _STDIO_H
# undef  FOPEN_MAX
# define FOPEN_MAX 16
#endif

/* XXX !!! */
extern int _sys_nerr;
extern char *_sys_errlist[];
