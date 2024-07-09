/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI kvm_ttys.c,v 2.4 2003/08/20 23:25:25 polk Exp
 */

/*-
 * Copyright (c) 1980, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * We need the normally kernel-only struct tty definition (which requires
 * the normally kernel-only struct selinfo definition).
 */
#define _NEED_STRUCT_SELINFO
#define _NEED_STRUCT_TTY

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/tty.h>
#include <sys/ttystats.h>

#include <db.h>
#include <kvm.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "kvm_private.h"
#include "kvm_stat.h"

/*
 * Return the (externalized) ttys.
 * Allocates memory.
 */
int
kvm_ttys(kvm_t *kd, struct ttydevice_tmp **tdpp, int *ntdp, struct ttystat **ttpp,
    int *nttp)
{
	struct ttydevice_tmp *td, *ttyhead, attydev;
	struct ttystat *ts;
	struct tty *tg, *tp;
	struct pgrp *pgrp;
	char *tdbuf, *etdbuf, *tdbp;
	char *ttbuf, *ettbuf, *ttbp;
	size_t ntd, ntt, tdspace, ttspace, nttgrp;
	int i, pgid;
	u_long addr;
	size_t size;
	int mib[2];

#define	KGET(addr, var) (kvm_read(kd, addr, &var, sizeof var) != sizeof var)

	if (ISALIVE(kd)) {
		mib[0] = CTL_HW;
		mib[1] = HW_TTYNAMES_TMP;
		td = _kvm_s2alloc(kd, mib, "HW_TTYNAMES_TMP", &ntd, sizeof *td);
		if (td == NULL)
			return (-1);
		mib[1] = HW_TTYSTATS_TMP;
		ts = _kvm_s2alloc(kd, mib, "HW_TTYSTATS_TMP", &ntt, sizeof *ts);
		if (ts == NULL) {
			free(td);
			return (-1);
		}
		*tdpp = td;
		*ntdp = ntd;
		*ttpp = ts;
		*nttp = ntt;
		return (0);
	}

	/*
	 * Simulate the sysctl.
	 */
	if ((addr = _kvm_symbol(kd, "_ttyhead")) == 0 || KGET(addr, ttyhead))
		return (-1);

	/* First pass: count ttydevice_tmp's and tty's. */
	nttgrp = ntd = ntt = 0;
	for (td = ttyhead; td != NULL; td = attydev.tty_next) {
		if (KGET((u_long)td, attydev))
			return (-1);
		ntd++;
		ntt += attydev.tty_count;
		if (attydev.tty_count > nttgrp)
			nttgrp = attydev.tty_count;
	}

	/* Allocate space for ttydevice_tmp's and tty's. */
	tdspace = ntd * sizeof *td;
	if ((tdbuf = malloc(tdspace)) == NULL) {
		_kvm_syserr(kd, kd->program, "malloc(%lu)", (u_long)tdspace);
		return (-1);
	}
	etdbuf = tdbuf + tdspace;

	/*
	 * Allocate enough room for both arrays and allow for weird
	 * allignment problems on tg
	 */
	ttspace = ntt * sizeof *ts + (nttgrp + 1) * sizeof *tg;
	if ((ttbuf = malloc(ttspace)) == NULL) {
		_kvm_syserr(kd, kd->program, "malloc(%lu)", (u_long)ttspace);
		goto bad;
	}
	ettbuf = ttbuf + (ntt * sizeof *ts);

	/*
	 * We count on malloc returning ttbuf correctly alligned for tg
	 * we also allocated an entire extra element for tg
	 * so, we can simply go forward and align tg on a *tg boundry
	 * past ttbuf.
	 */
	tg = (struct tty *)ttbuf;
	tg += (ntt * sizeof *ts) / sizeof *tg + 1;

	/* Second pass: fill in allocated space */
	tdbp = tdbuf;
	ttbp = ttbuf;
	for (td = ttyhead; td != NULL; td = attydev.tty_next) {
		if (tdbp >= etdbuf) {
			_kvm_err(kd, kd->program, "sync error 1");
			goto bad;
		}
		if (KGET((u_long)td, attydev))
			goto bad;
		memmove(tdbp, &attydev, sizeof attydev);
		tdbp += sizeof attydev;

		i = attydev.tty_count;
		size = i * sizeof *tg;
		if (kvm_read(kd, (u_long)attydev.tty_ttys, tg, size) != (ssize_t)size)
			goto bad;

		ts = (struct ttystat *)ttbp;
		ttbp += i * sizeof *ts;
		if (ttbp > ettbuf) {
			_kvm_err(kd, kd->program, "sync error 2");
			goto bad;
		}

		for (tp = tg; i-- >= 0; ++tp, ++ts) {
                        tp->t_stat.ts_rawqcc = tp->t_rawq.c_cc;
                        tp->t_stat.ts_canqcc = tp->t_canq.c_cc;
                        tp->t_stat.ts_outqcc = tp->t_outq.c_cc;
			pgrp = tp->t_pgrp;
			if (pgrp != NULL) {
				if (KGET((u_long)&pgrp->pg_id, pgid))
					goto bad;
				tp->t_stat.ts_pgid = pgid;
			}
			memmove(ts, &tp->t_stat, sizeof *ts);
		}
	}
	if (tdbp != etdbuf || ttbp != ettbuf)
		_kvm_err(kd, kd->program, "sync error 3");
	*tdpp = (struct ttydevice_tmp *)tdbuf;
	*ntdp = ntd;
	*ttpp = (struct ttystat *)ttbuf;
	*nttp = ntt;
	return (0);
bad:
	free(tdbuf);
	free(ttbuf);
	return (-1);
}
