/*	BSDI sysident.h,v 1.3 2001/11/27 23:42:38 donn Exp	*/
/* $NetBSD: sysident.h,v 1.9 2001/06/19 12:07:21 fvdl Exp $ */

/*
 * Copyright (c) 1997 Christopher G. Demetriou
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#ifndef __bsdi__

/*
 * Here we define the NetBSD OS Version in an ELF .note section, structured
 * like:
 *
 * [NOTE HEADER]
 *	long		name size
 *	long		description size
 *	long		note type
 *
 * [NOTE DATUM]
 *	string		OS name
 *
 * OSVERSION notes also have:
 *	long		OS version (NetBSD constant from param.h)
 *
 * The DATUM fields should be padded out such that their actual (not
 * declared) sizes % 4 == 0.
 *
 * These are used by the kernel to determine if this binary is really a
 * NetBSD binary, or some other OS's.
 */

/* XXX: NetBSD 1.5 compatibility only! */
#if __NetBSD_Version__ < 105010000
#define	ELF_NOTE_TYPE_NETBSD_TAG	1
#endif

#define	__S(x)	__STRING(x)
__asm(
	".section \".note.netbsd.ident\", \"a\"\n"
	"\t.p2align 2\n\n"

	"\t.long   7\n"
	"\t.long   4\n"
	"\t.long   " __S(ELF_NOTE_TYPE_NETBSD_TAG) "\n"
	"\t.ascii \"NetBSD\\0\\0\"\n"
	"\t.long   " __S(NetBSD) "\n\n"

	"\t.p2align 2\n"
);

#else

/*
 * Implement Linux/FreeBSD/NetBSD ELF style binary branding.
 * See http://www.netbsd.org/Documentation/kernel/elf-notes.html
 * for more information.
 */
#define ABI_OSNAME	"BSD/OS"
#define ABI_NOTETYPE	1

static const struct {
	Elf32_Nhdr hdr;
	char	name[roundup(sizeof (ABI_OSNAME), sizeof (int32_t))];
	char	desc[roundup(sizeof (RELEASE), sizeof (int32_t))];
} ident __attribute__ ((__section__ (".note.ABI-tag"))) = {
	{ sizeof (ABI_OSNAME), sizeof (RELEASE), ABI_NOTETYPE },
	ABI_OSNAME,
	RELEASE
};

#endif