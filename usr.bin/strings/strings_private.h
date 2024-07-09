/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strings_private.h,v 2.1 1997/09/06 19:46:32 donn Exp
 */

#define	DEFAULT_THRESHOLD	4
#define	DEFAULT_BUFSIZE		8192

typedef off_t (*sec_t)(off_t *);

sec_t check_aout(FILE *, off_t *);
sec_t check_elf(FILE *, off_t *);
sec_t check_whole_file(FILE *, off_t *);

void unget(void *, size_t, FILE *);
