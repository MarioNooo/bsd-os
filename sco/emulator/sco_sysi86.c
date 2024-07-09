/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_sysi86.c,v 2.1 1995/02/03 15:15:44 polk Exp
 */

#include <sys/param.h>
#include <errno.h>
#include "emulate.h"
#include "sco.h"

/*
 * Support for sysi86 emulation.
 */

/* from iBCS2 p 6-59 */
#define	SCO_SI86FPHW	40

/* from SVr4 ABI i386 processor supplement, p 6-76 */
#define	SCO_FP_NO	0
#define	SCO_FP_SW	1
#define	SCO_FP_HW	2
#define	SCO_FP_287	2
#define	SCO_FP_387	3

int
sco_sysi86(op, arg1, arg2, arg3)
	int op, arg1, arg2, arg3;
{

	switch (op) {

	case SCO_SI86FPHW:
		*(int *)arg1 = SCO_FP_HW;
		return (0);
	}

	errno = EINVAL;
	return (-1);
}
