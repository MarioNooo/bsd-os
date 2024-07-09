/* @(#)shsdrvr.c	12.4 9/1/96 11:44:16 */
/*
 * shsdrvr - old shs driver code
 *
 **************************************************************************
 * This version implements the old Secure Hash Algorithm specified by     *
 * (FIPS Pub 180).  This version is kept for backward compatibility with  *
 * shs version 2.10.1.  See the shs utility for the new standard.         *
 **************************************************************************
 *
 * This file was written by:
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
 ***
 *
 * NOTE: The version information below refers to all shs code, not
 *	 just this file.  In particular, this file was created by
 *	 Landon Curt Noll.
 *
 * Version 1.1: 02 Sep 1992?			original authors
 *     This code is based on code by Peter C. Gutmann.  Much thanks goes
 *     to Peter C. Gutman (pgut1@cs.aukuni.ac.nz) , Shawn A. Clifford
 *     (sac@eng.ufl.edu), Pat Myrto (pat@rwing.uucp) and others who wrote
 *     and/or worked on the original code.
 *
 * Version 2.1:	31 Dec 1993		Landon Curt Noll   (chongo@toad.com)
 *     Reformatted, performance improvements and bug fixes
 *
 * Version 2.2:	02 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     fixed -p usage
 *     better error messages
 *     added -c help
 *     added -c 0	(concatenation)
 *     reordered -i stat buffer pre-pending
 *
 * Version 2.3:	03 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     added -c 1	(side by side)
 *     added -c 2	(even force to be odd)
 *     added -c x	(shs dual test suite)
 *     changed -c help to be -c h
 *     changed -c operand to type[,opt[,...]]
 *     prefix & string ABI now can take arbitrary binary data
 *     fixed memory leak
 *     fixed even/odd byte split bug
 *     added -P file
 *     added -q
 *     added UNROLL_LOOPS to control shs.c loop unrolling
 *     major performance improvements
 *
 * Version 2.4: 05 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     renamed calc mode to dual mode
 *     removed all -c code
 *     added -d		(dual digests, space separated)
 *     rewrote most of the file, string and stream logic using shsdual code
 *
 * Version 2.5: 08 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     added (new) -c	(print 0x in front of digests)
 *     removed st_blksize and st_blocks from -i preprocessing data
 *     only print .0 suffix if -i and digesting a file
 *     non-zero edit codes are now unique
 *     changed the dual test suite (shorter, added non alpha numeric chars)
 *     -i requires filenames
 *     fixed @(#) what string code
 *     boolean logic simplication by Rich Schroeppel (rcs@cs.arizona.edu)
 *     on the fly in a circular buffer by Colin Plumb (colin@nyx10.cs.du.edu)
 *
 * Version 2.6: 11 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     Merged the shs and md5 Makefiles to build both in the same directory
 *     alignment and byte order performance improvements
 *     eliminate wateful memory copies
 *     shs transform contains no function calls
 *     beta release
 *
 * Version 2.7: 13 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     code cleanup
 *     chunk is now 64 bytes, block is determined by blocking factor
 *     magic 64 and 64 related values defined in terms of #defines
 *     added blocking code (-b block_len)
 *     added xor feedback code (-f)
 *     added xor feedback and block options to performance test
 *     performance improvements
 *
 * Version 2.8: 16 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     code cleanup
 *     performance improvements
 *     removed blocking and feedback code
 *     count bytes in driver, convert to 64 bit count in final transform
 *     added debug mode
 *     handle read errors and EOF better
 *     prefix strings not multiple of 64 bytes in length do not slow down hash
 *     renumbered exit codes
 *     fixed dual digest split bug
 *     byte sex swapping is now controlled thru the SHS_TRANSFORM macro
 *     shsTransform() is now called via the SHS_TRANSFORM macro
 *
 * Version 2.9: 12 Feb 1994		Landon Curt Noll   (chongo@toad.com)
 *     prep for beta release
 *     removed all feedback code
 *
 * Version 2.10: 25 Mar 1994		Landon Curt Noll   (chongo@toad.com)
 *     must_align catchs signal to detect misaligned access
 *
 * Version 3.1: 09 Mar 1995		Landon Curt Noll   (chongo@toad.com)
 *     Bumped version from 2.10 to 3.1.
 *
 *     Moved stream and file routines into shsio.c.
 *
 * Version 3.2: 17 Nov 1995		Landon Curt Noll   (chongo@toad.com)
 *     Fixed help string.
 *
 *     Added multiple digests capability instead of just dual.  Added
 *     -m num to denote 2 or more multiple digests.
 *
 *     Added -C to prevent spaces (and later 0x if -c) between multi digests.
 *
 * Version 3.3: 01 Sep 1996		Landon Curt Noll   (chongo@toad.com)
 *     Provide sha as well as shs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "shs.h"

/* size of test in megabytes */
#define TEST_MEG 16

/* number of chunks to process */
#define TEST_CHUNKS (TEST_MEG*1024*1024/SHS_READSIZE)

/* SHS1 test suite strings */
#define ENTRY(str) {(BYTE *)str, NULL, sizeof(str)-1}
struct shs_test {
    BYTE *ro_data;	/* read only string data or NULL to test */
    BYTE *data;		/* data or NULL to test */
    int len;		/* length of data */
} shs_test_data[] = {
    ENTRY(""),
    ENTRY("a"),
    ENTRY("abc"),
    ENTRY("message digest"),
    ENTRY("abcdefghijklmnopqrstuvwxyz"),
    ENTRY("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
    ENTRY("12345678901234567890123456789012345678901234567890123456789012345678901234567890"),
    ENTRY("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
};
#define MAX_SHS_TEST_DATA (sizeof(shs_test_data)/sizeof(shs_test_data[0]))

/* shs test filenames */
char *shs_test_file[] = {
    "file1",
    "file2",
};
#define MAX_SHS_TEST_FILE (sizeof(shs_test_file)/sizeof(shs_test_file[0]))

/* where the test files are located by default */
#if !defined(TLIB)
#define TLIB "."
#endif

/* version */
static char *version = "12";

/* Prototypes of the static functions */
static void shsOutput P((char*, int, int, int, SHS_INFO*));
static int shsPreFileRead P((char*, BYTE**));
static void shsTestSuite P((void));
static void shsHelp P((void));
void main P((int, char**));

/* global variables */
int c_flag = 0;			/* 1 => print C style digest with leading 0x */
int i_flag = 0;			/* 1 => process inode & filename */
int q_flag = 0;			/* 1 => print the digest only */
int debug = 0;			/* 1 => add debug */
char *program = "<program name unknown>";	/* our name */


/*
 * shsOutput - output the digest
 */
static void
shsOutput(str, quot, hexhdr, trail, dig)
    char *str;		/* print string after digest, NULL => none */
    int quot;		/* 1 => surround str with a double quotes */
    int hexhdr;		/* 1 => print 0x before the hex value */
    int trail;		/* 1 => print a trailing ".0" */
    SHS_INFO *dig;	/* current digest */
{
    /*
     * finalize the digest
     */
    shsFinal(dig);
#if defined(DEBUG)
    if (debug) {
	fprintf(stderr, 
	  "DEBUG: 64 bit count: 0x%08x%08x\n",
	  ((dig->countHi << 3) | (dig->countLo >> 29)), (dig->countLo << 3));
    }
#endif

    /*
     * print the digest
     */
    shsPrint(hexhdr, trail, dig);
    if (str && !q_flag) {
	if (quot) {
	    printf(" \"%s\"\n", str);
	} else {
	    printf(" %s\n", str);
	}
    } else {
	putchar('\n');
    }
    fflush(stdout);
}


/*
 * shsPrint - print a digest in hex
 *
 * Prints message digest buffer in shsInfo as 40 hexadecimal digits. Order is
 * from low-order byte to high-order byte of digest. Each byte is printed
 * with high-order hexadecimal digit first.
 *
 * If hexhdr, then print a leading "0x".  If trail, then print a trailing ".0".
 */
void
shsPrint(hexhdr, trail, shsInfo)
    int hexhdr;		/* 1 => print 0x before the hex value */
    int trail;		/* 1 => print a trailing ".0" */
    SHS_INFO *shsInfo;
{
    if (hexhdr) {
	fputs("0x", stdout);
    }
    printf("%08lx%08lx%08lx%08lx%08lx",
	shsInfo->digest[0], shsInfo->digest[1], shsInfo->digest[2],
	shsInfo->digest[3], shsInfo->digest[4]);
    if (trail) {
	fputs(".0", stdout);
    }
}


/*
 * shsTimeTrial - measure the speed of SHS1
 *
 * Measures user time required to digest TEST_MEG megabytes of characters.
 *
 * This function will time blocking and under xor feedback mode if they
 * are set.
 */
static void
shsTimeTrial()
{
    ULONG data[SHS_READWORDS];	/* test buffer */
    SHS_INFO shsInfo;		/* hash state */
    struct rusage start;	/* test start time */
    struct rusage stop;		/* test end time */
    double usrsec;		/* duration of test in user seconds */
    unsigned int i;

    /*
     * initialize test data
     */
    for (i = 0; i < SHS_READSIZE; i++) {
	((BYTE *)data)[i] = (BYTE)(i & 0xFF);
    }

    /*
     * announce test
     */
    if (!q_flag) {
	printf("shs time trial for %d megs of test data ...", TEST_MEG);
	fflush(stdout);
    }

    /* 
     * digest data in SHS_READSIZE byte chunks
     */
    getrusage(RUSAGE_SELF, &start);
    shsInit(&shsInfo);
    for (i=0; i < TEST_CHUNKS; ++i) {
	shsfullUpdate(&shsInfo, (BYTE *)data, SHS_READSIZE);
    }
    SHSCOUNT(&shsInfo, SHS_READSIZE*TEST_CHUNKS);
    shsFinal(&shsInfo);
    getrusage(RUSAGE_SELF, &stop);

    /*
     * announce the test results
     */
    usrsec = (stop.ru_utime.tv_sec - start.ru_utime.tv_sec) +
    	   (double)(stop.ru_utime.tv_usec - start.ru_utime.tv_usec)/1000000.0;
    if (!q_flag) {
	putchar('\n');
    }
    shsPrint(0, 0, &shsInfo);
    if (q_flag) {
	putchar('\n');
    } else {
	printf(" is digest of test data\n");
	printf("user seconds to process test data: %.2f\n", usrsec);
	printf("characters processed per user second: %d\n",
	    (int)((double)TEST_MEG*1024.0*1024.0/usrsec));
    }
}


/*
 * shsTestSuite - run a standard suite of test data
 */
static void
shsTestSuite()
{
    struct shs_test *t;		/* current shs test */
    struct stat buf;		/* stat of a test file */
    SHS_INFO digest;		/* test digest */
    char **f;			/* current file being tested */
    int i;

    /*
     * copy our test strings into writable data
     */
    for (i=0, t=shs_test_data; i < MAX_SHS_TEST_DATA; ++i, ++t) {
	if (t->ro_data != NULL) {
	    t->data = (BYTE *)malloc(t->len + 1);
	    if (t->data == NULL) {
		fprintf(stderr, "%s: malloc #4 failed\n", program);
		exit(4);
	    }
	    strcpy((char *)t->data, (char *)t->ro_data);
        }
    }

    /*
     * print test header
     */
    puts("old shs test suite results:");

    /*
     * find all of the test files
     */
    for (i=0, f=shs_test_file; i < MAX_SHS_TEST_FILE; ++i, ++f) {
	if (stat(*f, &buf) < 0) {
	    /* no file1 in this directory, cd to the test suite directory */
	    if (chdir(TLIB) < 0) {
		fflush(stdout);
		fprintf(stderr,
		    "%s: cannot find %s or %s/%s\n", program, *f, TLIB, *f);
		return;
	    }
	}
    }

    /*
     * try all combinations of test strings as prefixes and data
     */
    for (i=0, t=shs_test_data; i < MAX_SHS_TEST_DATA; ++i, ++t) {
	shsInit(&digest);
	shsUpdate(&digest, t->data, t->len);
	SHSCOUNT(&digest, t->len);
	shsOutput((char *)t->ro_data, 1, c_flag, i_flag, &digest);
    }

    /*
     * try the files with all test strings as prefixes
     */
    for (i=0, f=shs_test_file; i < MAX_SHS_TEST_FILE; ++i, ++f) {
	shsInit(&digest);
	shsFile(NULL, 0, *f, 0, &digest);
	shsOutput(*f, 0, c_flag, i_flag, &digest);
    }
    exit(0);
}


/*
 * shsPreFileRead - read and process a prepend file
 *
 * Returns the length of pre_str, and modifies pre_str to
 * point at the malloced prepend data.
 */
static int
shsPreFileRead(pre_file, buf)
    char *pre_file;		/* form pre_str from file pre_file */
    BYTE **buf;			/* pointer to pre_str pointer */
{
    struct stat statbuf;	/* stat for pre_file */
    int pre_len;		/* length of pre_file to be used */
    int bytes;			/* bytes read from pre_file */
    FILE *pre;			/* pre_file descriptor */

    /* obtain the length that we will use */
    if (stat(pre_file, &statbuf) < 0) {
	fprintf(stderr, "%s: unpable to find prepend file %s\n",
	    program, pre_file);
	exit(5);
    }
    pre_len = statbuf.st_size;
    if (pre_len > SHS_MAX_PRE_FILE) {
	/* don't use beyond SHS_MAX_PRE_FILE in size */
	pre_len = SHS_MAX_PRE_FILE;
    }

    /* malloc our pre string */
    *buf = (BYTE *)malloc(pre_len+1);
    if (*buf == NULL) {
	fprintf(stderr, "%s: malloc #3 failed\n", program);
	exit(6);
    }

    /* open our pre_file */
    pre = fopen(pre_file, "rb");
    if (pre == NULL) {
	fprintf(stderr, "%s: unable to open prepend file %s\n",
	  program, pre_file);
	exit(7);
    }

    /* read our pre_file data */
    bytes = fread((char *)(*buf), 1, pre_len, pre);
    if (bytes != pre_len) {
	fprintf(stderr,
	  "%s: unable to read %d bytes from prepend file %s\n",
	  program, pre_len, pre_file);
	exit(8);
    }

    /* return our length */
    return (pre_len);
}


/*
 * shsHelp - print shs help message and exit
 */
static void
shsHelp()
{
    fprintf(stderr,
      "%s [-cCd%shimqtx][-p prefix][-P pfile][-s str] [file ...]\n", 
      program,
#if defined(DEBUG)
      "D"
#else
      ""
#endif
      );
    fprintf(stderr,
      "    -c          print C style digests with a leading 0x\n");
    fprintf(stderr,
      "    -C          do not space seperate multi digests\n");
    fprintf(stderr,
      "    -d          same as -m 2\n");
#if defined(DEBUG)
    fprintf(stderr,
      "    -D          debug mode\n");
#endif
    fprintf(stderr,
      "    -h          prints this message\n");
    fprintf(stderr,
      "    -i          process inode and filename as well as file data\n");
    fprintf(stderr,
      "    -m num      process num digests on every num-th byte\n");
    fprintf(stderr,
      "    -p prefix   prepend str to data before digesting\n");
    fprintf(stderr,
      "    -P pfile    prepend the file 'str' to data before digesting\n");
    fprintf(stderr,
      "    -q          print only the digest\n");
    fprintf(stderr,
      "    -r          reverse feedback mode\n");
    fprintf(stderr,
      "    -s str      prints digest and contents of string\n");
    fprintf(stderr,
      "    -t          prints time statistics for %dM chars\n", TEST_MEG);
    fprintf(stderr,
      "    -v          print version\n");
    fprintf(stderr,
      "    -x          execute an extended standard suite of test data\n");
    fprintf(stderr,
      "    file        print digest and name of file\n");
    fprintf(stderr,
      "    (no args)   print digest of stdin\n");
    exit(0);
}


/*
 * main - shs main control function
 */
void
main(argc, argv)
    int argc;			/* arg count */
    char **argv;		/* the args */
{
    SHS_INFO digest;		/* our current digest */
    BYTE *pre_str = NULL;	/* pre-process this data first */
    char *pre_file = NULL;	/* pre-process this file first */
    char *data_str = NULL;	/* data is this string, not a file */
    UINT pre_str_len;		/* length of pre_str or pre_file */
    UINT data_str_len;		/* length of data_str */
    int C_flag = 0;		/* 1 => don't space seperate multi digests */
    int d_flag = 0;		/* 1 => dual digest mode */
    int m_flag = 0;		/* 1 => multi digest mode */
    int t_flag = 0;		/* 1 => -t was given */
    int x_flag = 0;		/* 1 => -x was given */
    int multi;			/* number of digests to do in parallel */
    extern char *optarg;	/* argument to option */
    extern int optind;		/* option index */
    int c;

    /*
     * parse args
     */
    program = argv[0];
    while ((c = getopt(argc, argv, "cCdDihm:p:P:qs:tvx")) != -1) {
        switch (c) {
        case 'c':
	    c_flag = 1;
	    break;
        case 'C':
	    C_flag = 1;
	    break;
        case 'd':
	    d_flag = 1;
	    multi = 2;
	    break;
        case 'D':
#if defined(DEBUG)
	    debug = 1;
#else
	    fprintf(stderr, "%s: not compiled with -DDEBUG\n", program);
	    exit(9);
	    /*NOTREACHED*/
#endif
	    break;
	case 'h':
	    shsHelp();
	    /*NOTREACHED*/
	    break;
	case 'i':
            i_flag = 1;
            break;
	case 'm':
	    m_flag = 1;
	    multi = atoi(optarg);
	    break;
	case 'p':
	    pre_str = (BYTE *)optarg;
	    break;
	case 'q':
	    q_flag = 1;
	    break;
	case 'P':
	    pre_file = optarg;
	    break;
        case 's':
            data_str = optarg;
            break;
	case 't':
	    t_flag = 1;
	    break;
	case 'v':
	    printf("%s: version 3.%d.%s%s %s\n",
	        program, atoi(version)-10, "1",
	        (strcmp(shs_what,"@(#)") == 0 &&
	         strcmp("@(#)","@(#)") == 0 &&
	         strcmp(shsdual_what,"@(#)") == 0 &&
	         strcmp(SHS_H_WHAT,"@(#)") == 0) ? "" : "+",
	        "95/11/17");
	    exit(0);
	case 'x':
	    x_flag = 1;
	    break;
	default:
	    shsHelp();
	    break;
        }
    }
    /* arg checking */
    if (data_str && optind != argc) {
	fprintf(stderr, "%s: -s is not compatible with digesting files\n",
	    program);
	exit(10);
    }
    if (i_flag && optind == argc) {
	fprintf(stderr, "%s: -i works only on filenames\n", program);
	exit(11);
    }
    if (d_flag && m_flag) {
	fprintf(stderr, "%s: -d imples -m 2, use one or the other\n", program);
	exit(12);
    }
    if ((d_flag || m_flag) && multi < 2) {
	fprintf(stderr, "%s: multi count must be > 1\n", program);
	exit(13);
    }
    if (C_flag && !d_flag && !m_flag) {
	fprintf(stderr, "%s: -C requires -d or -m num\n", program);
	exit(14);
    }

    /*
     * process -x if needed
     */
    if (x_flag) {
        if (d_flag) {
            multiTest();
	} else {
	    shsTestSuite();
	}
	exit(0);
    }

    /*
     * process -t if needed
     */
    if (t_flag) {
	shsTimeTrial();
	exit(0);
    }

    /*
     * process -P or -p if needed
     */
    if (pre_str && pre_file) {
	fprintf(stderr, "%s: -p and -P conflict\n", program);
	exit(15);
    }
    if (pre_file) {
	pre_str_len = shsPreFileRead(pre_file, &pre_str);
    } else if (pre_str) {
        pre_str_len = strlen((char *)pre_str);
    } else {
        pre_str_len = 0;
    }
    if (pre_str_len > SHS_MAX_PRE_FILE) {
	fprintf(stderr, "%s: prefix may not be longer than %d bytes\n",
	    program, SHS_MAX_PRE_FILE);
    	exit(15);
    }

    /*
     * if -d of -m num, perform multi digest processing instead
     */
    if (d_flag || m_flag) {
	multiMain(argc, argv, pre_str, pre_str_len, data_str, C_flag,
		  (UINT)multi);

    /*
     * if no -d and no -m num, process string, stdin or files
     */
    } else {

	/*
	 * case: digest a string
	 */
	if (data_str != NULL) {
	    data_str_len = strlen(data_str);
	    shsInit(&digest);
	    shsUpdate(&digest, pre_str, pre_str_len);
	    shsUpdate(&digest, (BYTE *)data_str, data_str_len);
	    SHSCOUNT(&digest, pre_str_len + data_str_len);
	    shsOutput(data_str, 1, c_flag, i_flag, &digest);

	/*
	 * case: digest stdin
	 */
	} else if (optind == argc) {
	    shsInit(&digest);
	    shsStream(pre_str, pre_str_len, stdin, &digest);
	    shsOutput(NULL, 0, c_flag, i_flag, &digest);

	/*
	 * case: digest files
	 */
	} else {
	    for (; optind < argc; optind++) {
		shsInit(&digest);
		shsFile(pre_str, pre_str_len, argv[optind], i_flag, &digest);
		shsOutput(argv[optind], 0, c_flag, i_flag, &digest);
	    }
	}
    }

    /* all done */
    exit(0);
}
