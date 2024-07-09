/* @(#)shsio.c	12.1 17 Nov 1995 04:22:33 */
/*
 * shsfile - old Secure Hash Standard
 *
 **************************************************************************
 * This version implements the old Secure Hash Algorithm specified by     *
 * (FIPS Pub 180).  This version is kept for backward compatibility with  *
 * shs version 2.10.1.  See the shs utility for the new standard.	  *
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
 * See shsdrvr.c for version and modification history.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#define SHS_IO
#include "shs.h"

/* global variables */
ULONG shs_zero[SHS_CHUNKWORDS];	/* block of zeros */


/*
 * shsStream - digest a open file stream
 */
void
shsStream(pre_str, pre_len, stream, dig)
    BYTE *pre_str;		/* data prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    FILE *stream;		/* the stream to process */
    SHS_INFO *dig;		/* current digest */
{
    ULONG data[SHS_READWORDS];	/* our read buffer */
    int bytes;			/* bytes last read */
    int ret;			/* partial fread return value */

    /*
     * pre-process prefix if needed
     */
    if (pre_str != NULL) {
	shsUpdate(dig, pre_str, pre_len);
	SHSCOUNT(dig, pre_len);
    }

    /*
     * if we have a partial chunk, try to read until we have a full chunk
     */
    clearerr(stream);
    if (dig->datalen > 0) {

        /* determine what we have so far */
        bytes = dig->datalen;

	/* try to read what we need to fill the chunk */
	while (bytes < SHS_CHUNKSIZE) {

	    /* try to read what we need */
	    ret = fread((char*)data+bytes, 1, SHS_CHUNKSIZE-bytes, stream);

	    /* carefully examine the result */
	    if (ret < 0 || ferror(stream)) {
		/* error processing */
		fprintf(stderr, "%s: ", program);
		perror("read #3 error");
		exit(1);
	    } else if (ret == 0 || feof(stream)) {
		/* EOF processing */
		SHSCOUNT(dig, SHS_CHUNKSIZE-dig->datalen);
		shsUpdate(dig, (BYTE *)data+dig->datalen, 
		  SHS_CHUNKSIZE-dig->datalen);
		return;
	    }

	    /* note that we have more bytes */
	    bytes += ret;
        }
        SHSCOUNT(dig, SHS_CHUNKSIZE-dig->datalen);
        shsUpdate(dig, (BYTE *)data+dig->datalen, SHS_CHUNKSIZE-dig->datalen);
    }

    /*
     * process the contents of the file
     */
    while ((bytes = fread((char *)data, 1, SHS_READSIZE, stream)) > 0) {

	/*
	 * if we got a partial read, try to read up to a full chunk
	 */
	while (bytes < SHS_READSIZE) {

	    /* try to read more */
	    ret = fread((char *)data+bytes, 1, SHS_READSIZE-bytes, stream);

	    /* carefully examine the result */
	    if (ret < 0 || ferror(stream)) {
	    	/* error processing */
	    	fprintf(stderr, "%s: ", program);
	    	perror("read #1 error");
	    	exit(2);
	    } else if (ret == 0 || feof(stream)) {
	    	/* EOF processing */
	    	shsUpdate(dig, (BYTE *)data, bytes);
	    	SHSCOUNT(dig, bytes);
	    	return;
	    }

	    /* note that we have more bytes */
	    bytes += ret;
	}

	/*
	 * digest the read
	 */
	shsfullUpdate(dig, (BYTE *)data, bytes);
	SHSCOUNT(dig, bytes);
    }

    /*
     * watch for errors
     */
    if (bytes < 0 || ferror(stream)) {
	/* error processing */
	fprintf(stderr, "%s: ", program);
	perror("read #2 error");
	exit(3);
    }
    return;
}


/*
 * shsFile - digest a file
 */
void
shsFile(pre_str, pre_len, filename, inode, dig)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    char *filename;		/* the filename to process */
    int inode;			/*  1 => process inode & filename */
    SHS_INFO *dig;		/* current digest */
{
    FILE *inFile;		/* the open file stream */
    struct stat buf;		/* stat or lstat of file */
    struct shs_stat hashbuf;	/* stat data to digest */
    struct shs_stat hashlbuf;	/* lstat data to digest */
    ULONG filename_len;		/* length of the filename */

    /*
     * open the file
     */
    inFile = fopen(filename, "rb");
    if (inFile == NULL) {
	fprintf(stderr, "%s: cannot open %s: ", program, filename);
	perror("");
	return;
    }

    /*
     * pre-process prefix if needed
     */
    if (pre_str == NULL) {
	if (inode) {
	    filename_len = strlen(filename);
	    shsUpdate(dig, (BYTE *)filename, filename_len);
	    SHSCOUNT(dig, filename_len);
#if defined(DEBUG)
	    if (debug) {
		fprintf(stderr, "DEBUG: filename_len:%d count:%d\n",
		    filename_len, dig->countLo);
	    }
#endif
	}
    } else {
	if (inode) {
	    shsUpdate(dig, pre_str, pre_len);
	    filename_len = strlen(filename);
	    shsUpdate(dig, (BYTE *)filename, filename_len);
	    SHSCOUNT(dig, filename_len+pre_len);
#if defined(DEBUG)
	    if (debug) {
		fprintf(stderr, "DEBUG: pre_len:%d filename_len:%d count:%d\n",
		    pre_len, filename_len, dig->countLo);
	    }
#endif
	} else {
	    shsUpdate(dig, pre_str, pre_len);
	    SHSCOUNT(dig, pre_len);
	}
    }

    /*
     * digest file stat and lstat
     */
    if (inode) {
	if (fstat(fileno(inFile), &buf) < 0) {
	    printf("%s can't be stated.\n", filename);
	    return;
	}
	hashbuf.stat_dev = buf.st_dev;
	hashbuf.stat_ino = buf.st_ino;
	hashbuf.stat_mode = buf.st_mode;
	hashbuf.stat_nlink = buf.st_nlink;
	hashbuf.stat_uid = buf.st_uid;
	hashbuf.stat_gid = buf.st_gid;
	hashbuf.stat_size = buf.st_size;
	hashbuf.stat_mtime = buf.st_mtime;
	hashbuf.stat_ctime = buf.st_ctime;
#if defined(DEBUG)
	if (debug) {
	    fprintf(stderr, 
	      "DEBUG: dev:%d ino:%d mode:%o nlink:%d uid:%d gid:%d\n",
	      hashbuf.stat_dev, hashbuf.stat_ino, hashbuf.stat_mode,
	      hashbuf.stat_nlink, hashbuf.stat_uid, hashbuf.stat_gid);
	    fprintf(stderr, 
	      "DEBUG: size:%d mtime:%d ctime:%d\n",
	      hashbuf.stat_size, hashbuf.stat_mtime, hashbuf.stat_ctime);
	}
#endif
	shsUpdate(dig, (BYTE *)&hashbuf, sizeof(hashbuf));
	if (lstat(filename, &buf) < 0) {
	    printf("%s can't be lstated.\n", filename);
	    return;
	}
	hashlbuf.stat_dev = buf.st_dev;
	hashlbuf.stat_ino = buf.st_ino;
	hashlbuf.stat_mode = buf.st_mode;
	hashlbuf.stat_nlink = buf.st_nlink;
	hashlbuf.stat_uid = buf.st_uid;
	hashlbuf.stat_gid = buf.st_gid;
	hashlbuf.stat_size = buf.st_size;
	hashlbuf.stat_mtime = buf.st_mtime;
	hashlbuf.stat_ctime = buf.st_ctime;
#if defined(DEBUG)
	if (debug) {
	    fprintf(stderr, 
	      "DEBUG: ldev:%d lino:%d mode:%o lnlink:%d luid:%d lgid:%d\n",
	      hashlbuf.stat_dev, hashlbuf.stat_ino, hashlbuf.stat_mode,
	      hashlbuf.stat_nlink, hashlbuf.stat_uid, hashlbuf.stat_gid);
	    fprintf(stderr, 
	      "DEBUG: lsize:%d lmtime:%d lctime:%d\n",
	      hashlbuf.stat_size, hashlbuf.stat_mtime, hashlbuf.stat_ctime);
	}
#endif
	shsUpdate(dig, (BYTE *)&hashlbuf, sizeof(hashlbuf));

	/*
	 * pad with zeros to process file data faster
	 */
	if (dig->datalen > 0) {
#if defined(DEBUG)
	    if (debug) {
		fprintf(stderr, 
		  "DEBUG: pad_len:%d\n", SHS_CHUNKSIZE - dig->datalen);
	    }
#endif
	    SHSCOUNT(dig, sizeof(hashbuf) + sizeof(hashlbuf) + 
	          SHS_CHUNKSIZE - dig->datalen);
	    shsUpdate(dig, (BYTE *)shs_zero, SHS_CHUNKSIZE - dig->datalen);
	} else {
	    SHSCOUNT(dig, sizeof(hashbuf) + sizeof(hashlbuf));
	}
#if defined(DEBUG)
	if (debug) {
	    fprintf(stderr, "DEBUG: datalen:%d count:%d\n", 
	      dig->datalen, dig->countLo);
	}
#endif
    }

    /*
     * process the data stream
     */
    shsStream(NULL, 0, inFile, dig);
    fclose(inFile);
}
