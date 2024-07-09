/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI size.h,v 2.1 1997/11/04 01:14:16 donn Exp
 */

int _ar_size(const char *, const void *, size_t);
int _aout_size(const char *, const void *, size_t);
int _elf32_size(const char *, const void *, size_t);
int _elf64_size(const char *, const void *, size_t);

void display(const char *, u_long, u_long, u_long);
