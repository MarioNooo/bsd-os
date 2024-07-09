/* @(#)shs.h	12.2 G%G 01:25:09 */
/*
 * shs - old Secure Hash Standard
 *
 **************************************************************************
 * This version implements the old Secure Hash Algorithm specified by     *
 * (FIPS Pub 180).  This version is kept for backward compatibility with  *
 * shs version 2.10.1.  See the shs utility for the new standard.         *
 **************************************************************************
 *
 * Written 2 September 1992, Peter C. Gutmann.
 *
 * This file was Modified by:
 *
 *	 Landon Curt Noll  (chongo@toad.com)	chongo <was here> /\../\
 *
 * This code has been placed in the public domain.  Please do not
 * copyright this code.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH  REGARD  TO
 * THIS  SOFTWARE,  INCLUDING  ALL IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS.  IN NO EVENT SHALL  LANDON  CURT
 * NOLL  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM  LOSS  OF
 * USE,  DATA  OR  PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * See shsdrvr.c for version and modification history.
 */

#if !defined(SHS_H)
#define SHS_H

#include <sys/types.h>
#include <sys/stat.h>

/*
 * determine if we are checked in
 */
#define SHS_H_WHAT "@(#)"	/* @(#) if checked in */

/*
 * These macros are in common with shs.h, shs.h and md5.h.  We use
 * HASH_MACROS to gaurd against multiple inclusion by external progs
 * that may want to use multiple hash codes in one module.
 */
#if !defined(HASH_MACROS)
#define HASH_MACROS

/*
 * Useful defines/typedefs
 */
typedef unsigned char BYTE;	/* must be a 1 byte unsigned value */
typedef unsigned long UINT;	/* must be a 2 byte unsigned value */
typedef unsigned long ULONG;	/* must be a 4 byte unsigned value */

#endif /* HASH_MACROS */

/* SHS_CHUNKSIZE must be a power of 2 - fixed value defined by the algorithm */
#define SHS_CHUNKSIZE (1<<6)
#define SHS_CHUNKWORDS (SHS_CHUNKSIZE/sizeof(ULONG))

/* SHS_DIGESTSIZE is a the length of the digest as defined by the algorithm */
#define SHS_DIGESTSIZE (20)
#define SHS_DIGESTWORDS (SHS_DIGESTSIZE/sizeof(ULONG))

/* SHS_LOW - where low 32 bits of 64 bit count is stored during final */
#define SHS_LOW 15

/* SHS_HIGH - where high 32 bits of 64 bit count is stored during final */
#define SHS_HIGH 14

/* SHS_BLOCKSIZE is how large a chunk multiStream processes at a time */
#define SHS_BLOCKSIZE (SHS_CHUNKSIZE<<8)

/* SHS_READSIZE must be a multiple of SHS_BLOCKSIZE */
#define SHS_READSIZE (SHS_BLOCKSIZE<<2)
#define SHS_READWORDS (SHS_READSIZE/sizeof(ULONG))

/* maximum part of pre_file used */
#define SHS_MAX_PRE_FILE 32768

/*
 * SHS_SWAP_BYTE_SEX(ULONG *dest, ULONG *src)
 *
 *	dest	- array of SHS_CHUNKWORDS ULONG of fixed data (may be src)
 *	src	- array of SHS_CHUNKWORDS ULONG of what to fix
 *
 * This macro will either switch to the opposite byte sex (Big Endian vs.
 *  Little Endian).
 */
#define SHS_SWAP_BYTE_SEX(dest, src) {	/* swap byte sex if needed */	\
    int tmp_i;	/* index */						\
    for (tmp_i=0; tmp_i < SHS_CHUNKWORDS; ++tmp_i) {			\
	((ULONG*)(dest))[tmp_i] =					\
	  (((ULONG*)(src))[tmp_i] << 16) |				\
	  (((ULONG*)(src))[tmp_i] >> 16);				\
	((ULONG*)(dest))[tmp_i] =					\
	  ((((ULONG*)(dest))[tmp_i] & 0xff00ff00UL) >> 8) |		\
	  ((((ULONG*)(dest))[tmp_i] & 0x00ff00ffUL) << 8);		\
    }									\
}

/*
 * SHS_ROUNDUP(x,y) - round x up to the next multiple of y
 */
#define SHS_ROUNDUP(x,y) ((((x)+(y)-1)/(y))*(y))

/* 
 * SHS_TRANSFORM(SHS_INFO *a, ULONG *b, ULONG *c)
 *
 * 	a	pointer to our current digest state
 *	b	pointer to SHS_CHUNKSIZE words of byte sex fixed data
 *	c	pointer to SHS_CHUNKSIZE words that do not need to be fixed
 */
#if BYTE_ORDER == BIG_ENDIAN
# define SHS_TRANSFORM(a,b,c)						\
    shsTransform(((SHS_INFO *)(a))->digest, (ULONG *)(c))
#else
# define SHS_TRANSFORM(a,b,c) { 					\
    SHS_SWAP_BYTE_SEX((b), (c));					\
    shsTransform(((SHS_INFO *)(a))->digest, (ULONG *)(b));		\
}
#endif

/*
 * SHSCOUNT(SHS_INFO*, ULONG) - update the 64 bit count in an SHS_INFO
 *
 * We will count bytes and convert to bit count during the final
 * transform.
 */
#define SHSCOUNT(shsinfo, count) {					\
    ULONG tmp_countLo;						\
    tmp_countLo = (shsinfo)->countLo;				\
    if (((shsinfo)->countLo += (count)) < tmp_countLo) {	\
	(shsinfo)->countHi++;					\
    }								\
}

/*
 * The structure for storing SHS info
 *
 * We will assume that bit count is a multiple of 8.
 */
typedef struct {
    ULONG digest[SHS_DIGESTWORDS];	/* message digest */
    ULONG countLo;			/* 64 bit count: bits 3-34 */
    ULONG countHi;			/* 64 bit count: bits 35-63 */
    ULONG datalen;			/* length of data in data */
    ULONG data[SHS_CHUNKWORDS];		/* SHS chunk buffer */
} SHS_INFO;

/*
 * elements of the stat structure that we will process
 */
struct shs_stat {
    dev_t stat_dev;
    ino_t stat_ino;
    mode_t stat_mode;
    nlink_t stat_nlink;
    uid_t stat_uid;
    gid_t stat_gid;
    off_t stat_size;
    time_t stat_mtime;
    time_t stat_ctime;
};

/*
 * Used to remove arguments in function prototypes for non-ANSI C
 */
#if defined(__STDC__) && __STDC__ == 1
# define PROTO
#endif
#ifdef PROTO
# define P(a) a
#else	/* !PROTO */
# define P(a) ()
#endif	/* ?PROTO */

/* shs.c */
void shsInit P((SHS_INFO*));
void shsUpdate P((SHS_INFO*, BYTE*, ULONG));
void shsfullUpdate P((SHS_INFO*, BYTE*, ULONG));
void shsFinal P((SHS_INFO*));
extern char *shs_what;

/* shsio.c */
void shsStream P((BYTE*, UINT, FILE*, SHS_INFO*));
void shsFile P((BYTE*, UINT, char*, int, SHS_INFO*));
extern ULONG shs_zero[];

/*
 * Some external programs use the functions found in shs.c and shsio.c.
 * These routines define SHS_IO so that such external programs will not
 * pick up the following declarations.
 */

#if !defined(SHS_IO)

/* shsdual.c */
void multiMain P((int, char**, BYTE*, UINT, char*, int, UINT));
void multiTest P((void));
extern char *shsdual_what;

/* shsdrvr.c */
void shsPrint P((int, int, SHS_INFO*));
extern int c_flag;
extern int i_flag;
extern int q_flag;
extern int dot_zero;
#endif /* SHS_IO */
extern int debug;
extern char *program;

#endif /* SHS_H */
