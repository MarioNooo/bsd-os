/*
 * Copyright (c) 2000 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI floatdixf.c,v 2.1 2000/06/04 22:03:39 torek Exp
 */

/*
 * This is ugly, but gcc insists on calling "long double" "xf" on i386,
 * and "tf" on sparc, even though the algorithm works the same in both
 * cases.  ("XF" denotes a 96-bit float, "TF" denotes a 128-bit float,
 * but the code to convert to a 64-bit integer is the same.)
 */
#define __floatditf __floatdixf
#include "floatditf.c"
