/*	BSDI unix.c,v 2.8 2003/08/20 23:57:38 polk Exp	*/

/*-
 * Copyright (c) 1983, 1988, 1993
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

#ifndef lint
static char sccsid[] = "@(#)unix.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * Display protocol blocks in the unix domain.
 */

/*
 * We need the normally kernel-only struct selinfo definition
 */
#define _NEED_STRUCT_SELINFO

#include <sys/types.h>
#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/un.h>
#include <sys/unpcb.h>
#define KERNEL
struct uio;
struct proc;
#include <sys/file.h>

#include <netinet/in.h>

#include <err.h>
#include <kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include "netstat.h"

static	void unixdomainpr __P((struct socket *, caddr_t));

extern kvm_t	*kvmd;

static struct data_info unixsw_info = {
    "_unixsw",
    NULL, 0,
    0
};

void
unixpr(void)
{
	struct file *fbuf, *fp, *fplim;
	struct socket sock, *so = &sock;
	struct protosw *unixsw;
	int n_files;

	resolve("unixpr", &unixsw_info, NULL);
	unixsw = (struct protosw *)unixsw_info.di_off;

	if ((fbuf = kvm_files(kvmd, &n_files)) == NULL) {
		warnx("kvm_files: %s", kvm_geterr(kvmd));
		return;
	}
	fplim = fbuf + n_files;
	for (fp = fbuf; fp < fplim; fp++) {
		if (fp->f_count == 0 || fp->f_type != DTYPE_SOCKET)
			continue;
		if (kread((u_long)fp->f_data, so, sizeof (*so)))
			continue;
		/* kludge */
		if (so->so_proto >= unixsw && so->so_proto <= unixsw + 2)
			if (so->so_pcb)
				unixdomainpr(so, fp->f_data);
	}
	free(fbuf);
}

static	char *socktype[] =
    { "#0", "stream", "dgram", "raw", "rdm", "seqpacket" };

static void
unixdomainpr(struct socket *so, caddr_t soaddr)
{
	struct unpcb unpcb, *unp = &unpcb;
	struct mbuf mbuf, *m;
	struct sockaddr_un *sa;
	static int first = 1;

	sa = NULL;

	if (kread((u_long)so->so_pcb, unp, sizeof (*unp)))
		return;
	if (unp->unp_addr) {
		m = &mbuf;
		if (kread((u_long)unp->unp_addr, m, sizeof (*m)))
			m = NULL;
		sa = (struct sockaddr_un *)(m->m_dat);
	} else
		m = NULL;
	if (first) {
		printf("Active Local domain sockets\n");
		printf("%-8.8s %-6.6s %-6.6s %-6.6s "
		    "%8.8s %8.8s %8.8s %8.8s Addr\n",
		    "Address", "Type", "Recv-Q", "Send-Q",
		    "Inode", "Conn", "Refs", "Nextref");
		first = 0;
	}
	printf("%8lx %-6.6s %6lu %6lu %8lx %8lx %8lx %8lx",
	    (u_long)soaddr, socktype[so->so_type],
	    so->so_rcv.sb_cc, so->so_snd.sb_cc,
	    (u_long)unp->unp_vnode, (u_long)unp->unp_conn,
	    (u_long)unp->unp_refs, (u_long)unp->unp_nextref);
	if (m)
		printf(" %.*s",
		    m->m_len - (int)(sizeof(*sa) - sizeof(sa->sun_path)),
		    sa->sun_path);
	putchar('\n');
}