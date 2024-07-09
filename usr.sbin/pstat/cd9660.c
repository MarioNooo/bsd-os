/*-
 * Copyright (c) 1994 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cd9660.c,v 2.3 1996/06/07 21:29:25 bostic Exp
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/ucred.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <isofs/cd9660/cd9660_node.h>

#include <kvm.h>
#include <stdio.h>

#include "extern.h"

int	usenumflag;

void
iso_header()
{
	(void)printf(" FILEID XFLAG       MODE          UID   RDEV|SZ");
}

void
iso_print(vp)
	struct vnode *vp;
{
	struct iso_node isonode, *isop = &isonode;
	static struct flagmap fm[] = {
		{ IN_ACCESS, 'A' },
		{ 0, 0 }
	};

	KGETRETVOID(vp->v_data, &isonode, sizeof(isonode), "vnode's isonode");
	(void)printf(" %6ld %5s",
	    isop->i_number, fmtflags(isop->i_flag, fm, 1));
	v_mode(isop->inode.iso_mode, vp->v_type);
	v_uid(isop->inode.iso_uid);
	if (S_ISCHR(isop->inode.iso_mode) || S_ISBLK(isop->inode.iso_mode))
		(void)printf(" %7s",
		    fmtdev(isop->inode.iso_rdev, isop->inode.iso_mode));
	else
		(void)printf(" %7ld", isop->i_size);
}
