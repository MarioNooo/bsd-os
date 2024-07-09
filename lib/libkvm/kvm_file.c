/*	BSDI kvm_file.c,v 2.5 1997/05/25 15:09:00 jch Exp	*/

/*-
 * Copyright (c) 1989, 1992, 1993
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)kvm_file.c	8.2 (Berkeley) 8/20/94";
#endif /* LIBC_SCCS and not lint */

/*
 * File list interface for kvm.  pstat, fstat and netstat are 
 * users of this code, so we've factored it out into a separate module.
 * Thus, we keep this grunge out of the other kvm applications (i.e.,
 * most other applications are interested only in open/close/read/nlist).
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/exec.h>
#define KERNEL
#include <sys/file.h>
#undef KERNEL
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/vm.h>

#include <sys/sysctl.h>

#include <limits.h>
#include <ndbm.h>
#include <paths.h>
#include <stdlib.h>

#include "kvm_private.h"

/*
 * Return the (externalized) file table.
 */
struct file *
kvm_files(kvm_t *kd, int *cnt)
{
	struct file *fbuf, *filehead;
	struct file *kfp, *next, *ufp, *ufplim;
	int n, nfiles;
	u_long addr;
	size_t size;
	int mib[2];

#define	KGET(addr, var) (kvm_read(kd, addr, &var, sizeof var) != sizeof var)

	if (ISALIVE(kd)) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_FILES;
		fbuf = (struct file *)_kvm_s2alloc(kd, mib, "KERN_FILES",
		    &size, sizeof(struct file));
		if (fbuf == NULL)
			return (NULL);
		*cnt = size;
		return (fbuf);
	}

	/*
	 * Simulate the sysctl.
	 */
	if ((addr = _kvm_symbol(kd, "_nfiles")) == 0 || KGET(addr, nfiles))
		return (NULL);
	if ((addr = _kvm_symbol(kd, "_filehead")) == 0 || KGET(addr, filehead))
		return (NULL);

	/*
	 * Copy out the array of file structures, modifying the "next" pointers
	 * (f_list.le_next) to be "self" pointers into the kernel.  Count them
	 * as we go, in case nfiles is a little off.
	 */
#define SLOP 10
	size = (nfiles + SLOP) * sizeof(struct file);
	fbuf = malloc(size);
	if (fbuf == NULL) {
		_kvm_syserr(kd, kd->program, "malloc(%lu)", (u_long)size);
		return (NULL);
	}

	ufp = fbuf;
	ufplim = fbuf + nfiles + SLOP;
	n = 0;
	for (kfp = filehead; kfp != NULL; kfp = next) {
		if (ufp >= ufplim) {
			/* XXX - should realloc */
			_kvm_err(kd, kd->program, "no more room for files");
			free(fbuf);
			return (NULL);
		}
		if (KGET((long)kfp, *ufp)) {
			_kvm_syserr(kd, kd->program,
			    "can't read file at %lx", (long)kfp);
			return (NULL);
		}
		next = ufp->f_list.le_next;
		ufp->f_list.le_next = kfp;
		ufp++;
		n++;
	}
	*cnt = n;
	return ((struct file *)fbuf);
}
