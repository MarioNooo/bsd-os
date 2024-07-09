/*	BSDI clockgen.c,v 1.1 1999/10/13 20:09:45 prb Exp
/* Id: clockgen.c,v 1.4 1999/02/18 08:24:27 explorer Exp */

/*
 *   (C) Copyright LAN Media Corporation 1998, 1999.
 *       All Rights Reserved.
 *
 *      This source file is the property of LAN Media Corporation
 *      and may not be copied or distributed in any isomorphic form
 *      without an appropriate prior licensing arrangement with
 *      LAN Media Corporation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netinet/in.h>

#include "if_lmcioctl.h"

#define T1_FREF  20000000
#define T1_FMIN  30000000
#define T1_FMAX 330000000

void
lmc_av9110_freq(u_int32_t target, lmc_av9110_t *av)
{
	unsigned int n, m, v, x, r;
	unsigned long f;
	unsigned long iFvco;

	av->n = 0;
	av->m = 0;
	av->v = 0;
	av->x = 0;
	av->r = 0;
	av->f = 0;
	av->exact = 0;

	target *= 16;

	for (n = 3 ; n <= 127 ; n++)
		for (m = 3 ; m <= 127 ; m++)
			for (v = 1 ; v <= 8 ; v *= 8)
				for (x = 1 ; x <= 8 ; x <<= 1)
					for (r = 1 ; r <= 8 ; r <<= 1) {
						iFvco = (T1_FREF / m) * n * v;
						if (iFvco < T1_FMIN || iFvco > T1_FMAX)
							continue;
						f = iFvco / (x * r);
						if (f >= target)
							if ((av->f == 0) || (f - target < av->f - target)) {
						
							av->n = n;
							av->m = m;
							if (v == 1)
								av->v = 0;
							else
								av->v = 1;
							if (x == 1)
								av->x = 0;
							else if (x == 2)
								av->x = 1;
							else if (x == 4)
								av->x = 2;
							else if (x == 8)
								av->x = 3;
							if (r == 1)
								av->r = 0;
							else if (r == 2)
								av->r = 1;
							else if (r == 4)
								av->r = 2;
							else if (r == 8)
								av->r = 3;
							av->f = f;
							if (f == target) {
								av->exact = 1;
								av->f /= 16;
								return;
							}
						}
					}
	av->f /= 16;
}

