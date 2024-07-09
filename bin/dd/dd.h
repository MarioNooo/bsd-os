/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego and Lance
 * Visser of Convex Computer Corporation.
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
 *
 *	@(#)dd.h	8.3 (Berkeley) 4/2/94
 */

#ifndef OFF_T_MAX			/* Not currently defined by limits.h. */
#define	OFF_T_MAX	UQUAD_MAX
#endif

/* Input/output stream state. */
typedef struct {
	u_char	*db;			/* buffer address */
	u_char	*dbp;			/* current buffer I/O address */
	size_t   dbcnt;			/* current buffer byte count */
	size_t	 dbrcnt;		/* last read byte count */
	size_t	 dbsz;			/* buffer size */

#define	ISCHR		0x01		/* character device (warn on short) */
#define	ISPIPE		0x02		/* pipe (not truncatable) */
#define	ISTAPE		0x04		/* tape (not seekable) */
#define	NOREAD		0x08		/* not readable */
	u_int	 flags;

	char 	*name;			/* name */
	int	 fd;			/* file descriptor */
	u_quad_t offset;		/* # of blocks to skip */

	u_quad_t f_stats;		/* # of full blocks processed */
	u_quad_t p_stats;		/* # of partial blocks processed */
	u_quad_t s_stats;		/* # of odd swab blocks */
	u_quad_t t_stats;		/* # of truncations */
} IO;

typedef struct {
	u_quad_t in_full;		/* # of full input blocks */
	u_quad_t in_part;		/* # of partial input blocks */
	u_quad_t out_full;		/* # of full output blocks */
	u_quad_t out_part;		/* # of partial output blocks */
	u_quad_t trunc;			/* # of truncated records */
	u_quad_t swab;			/* # of odd-length swab blocks */
	u_quad_t bytes;			/* # of bytes written */
	time_t	 start;			/* start time of dd */
} STAT;

/* Flags (in ddflags). */
#define	C_ASCII		0x000001
#define	C_BLOCK		0x000002
#define	C_BS		0x000004
#define	C_CBS		0x000008
#define	C_COUNT		0x000010
#define	C_EBCDIC	0x000020
#define	C_FILES		0x000040
#define	C_IBS		0x000080
#define	C_IF		0x000100
#define	C_LCASE		0x000200
#define	C_NOERROR	0x000400
#define	C_NOTRUNC	0x000800
#define	C_OBS		0x001000
#define	C_OF		0x002000
#define	C_OSYNC		0x004000
#define	C_SEEK		0x008000
#define	C_SKIP		0x010000
#define	C_SWAB		0x020000
#define	C_SYNC		0x040000
#define	C_UCASE		0x080000
#define	C_UNBLOCK	0x100000