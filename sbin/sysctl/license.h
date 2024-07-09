/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI license.h,v 2.1 1996/09/12 17:06:45 prb Exp
 */

extern int license_decodes __P((char *src, license_t *lp));
extern char *license_encodes __P((license_t *lp));
