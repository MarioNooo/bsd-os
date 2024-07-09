/*
 * Copyright (c) 1992, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI diskdefect.h,v 2.4 1997/10/31 22:57:01 torek Exp
 */

/*-
 * Copyright (c) 1980,1986,1988 Regents of the University of California.
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
 * Encapsulation of disk defect state.  Bad block information is
 * stored as opaque data, so that this program can be used (eventually)
 * with other formats than DEC STD 144.
 */
struct ddstate {
	int	d_fd;		/* open file descriptor */
	char	*d_name;	/* pathname corresponding to d_fd */
	short	d_part;		/* which partition was opened */
	short	d_rawpart;	/* which partition is considered `raw' */
	int	d_bflags;	/* flags (below) */

	char	*d_secbuf;	/* 1-sector-sized buffer, for misc use */

	void	*d_badinfo;	/* bad-sector information */
	struct	bbops *d_bb;	/* operations (in bbops.h) */

#ifdef notdef
	/*
	 * Disk label d_nsectors, d_ntracks do not include alterates/spares;
	 * we add a few variables that do include them.  See sys/disklabel.h.
	 */
	int	d_phys_spt;	/* physical sectors per track */
	int	d_phys_tpc;	/* physical tracks per cylinder */
	int	d_phys_spc;	/* physical sectors per cylinder */
#endif

	struct	disklabel d_dl;	/* geometry (for cyl/trk/sec conversion) */
};

/* d_bflags */
#define	DB_COPY		0x0001	/* copy contents of sectors when replacing */
#define	DB_LOADED	0x0002	/* state loaded via load() */
#define	DB_MODIFIED	0x0004	/* state has been modified */

/*
 * Macros for conversion between `sector number' and `cyl/trk/sec' triples.
 * Note that these use LOGICAL sector numbers (ie, not counting spares on
 * a track or cylinder).
 */
#define	CYL(ds, sn) ((sn) / (ds)->d_dl.d_secpercyl)
#define	TRK(ds, sn) (((sn) / (ds)->d_dl.d_nsectors) % (ds)->d_dl.d_ntracks)
#define	SEC(ds, sn) ((sn) % (ds)->d_dl.d_nsectors)

#define	SN(ds, c, t, s) \
	((daddr_t)(c) * (daddr_t)(ds)->d_dl.d_secpercyl + \
	 (daddr_t)(t) * (daddr_t)(ds)->d_dl.d_nsectors + (daddr_t)(s))

/* global flags  XXX */
extern int	copytries;	/* number of times to try to copy orig. data */
extern int	debug;		/* for debugging bad-block methods */
extern int	verbose;	/* chatty */
extern int	nflag;		/* no write of updated bad block list */
extern int	no_io;		/* fake the I/O (for testing) */
extern int	yflag;		/* assume yes to `overwrite?' type questions */

/* scan.c */
int	scandisk __P((struct ddstate *, int, int));

/* util.c */
__dead void panic __P((const char *)) __attribute__((volatile));

int	yesno __P((void));

int	rsec __P((struct ddstate *, void *secbuf, daddr_t sn));
int	wsec __P((struct ddstate *, void *secbuf, daddr_t sn));
int	ratoff __P((struct ddstate *, void *buf, off_t where, int size));
int	watoff __P((struct ddstate *, void *buf, off_t where, int size));

/*
 * N.B.: copysec will write the target sector even if the read fails,
 * using whatever data might have been read, if any (some drives will
 * return the best approximation even on error).  It therefore has
 * return flags, as defined here.
 */
int	copysec __P((struct ddstate *, daddr_t src, daddr_t dst));
int	zerosec __P((struct ddstate *, daddr_t));

#define	COPY_OK		0	/* successful */
#define	COPY_RDFAILED	1	/* read failed */
#define	COPY_WRFAILED	2	/* write failed */

void	*emalloc __P((size_t));
void	*erealloc __P((void *, size_t));
