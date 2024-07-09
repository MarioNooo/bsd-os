
/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 02/23/01  [rcg]
 *         - Non-existent cache file isn't an error.
 *
 * 01/02/01  [rcg]
 *         - Don't set first_msg_hidden when no messages.
 *
 * 09/20/00  [rcg]
 *         - File added.
 *
 */




#include <sys/types.h>
#include <errno.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "config.h"

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/file.h>

#include "genpath.h"
#include "popper.h"
#include "misc.h"



typedef struct {
    char    magic_num [ 6 ];        /* Magic number for file */
#define         CACHE_MAGIC_NUM "+Qpop+"
    int     version;                /* Version of cache file */
#define         CACHE_VERSION   1
    long    toc_str_sz;             /* bytes in MsgInfoList structure */
    long    toc_size;               /* bytes in allocated MsgInfoList */
    time_t  modtime;                /* modtime of spool file */
    int     msg_count;              /* Number of msgs in list */
    long    drop_size;              /* total bytes in spool */
    off_t   spool_end;              /* offset of end of spool */
    char    eol;                    /* set to '\n' */
} cache_hdr;


/*---------------------------------------------------------------------
 * We save the MsgInfoList structure in a cache file for faster
 * startup next time.
 *
 * The cache file starts with the contents of the cache_header structure,
 * then has the MsgInfoList structure.  Everything is written in binary,
 * in native endian format.
 *
 * The cache file is only written or read in server mode.  The assumption
 * of server mode is that the spool is only modified by Qpopper and the
 * local delivery agent, which only appends messages.
 *---------------------------------------------------------------------
 */



/*
 * Write the current MsgInfoList to the cache file.
 *
 * Parameters:
 *     p:           points to POP structure.
 *     spool_fd:    file descriptor for spool.
 *     mp:          points to the MsgInfoList structure.
 *
 * Result:
 *     POP_SUCCESS or POP_FAILURE.
 */
int
cache_write ( POP *p, int spool_fd, MsgInfoList *mp )
{
    long         toc_size        = p->msg_count * sizeof ( MsgInfoList );
    int          msg_count       = p->msg_count;
    char         cache_name [ MAXFILENAME ];
    cache_hdr    header;
    long         rslt            = 0;
    int          cache_fd        = -1;
    FILE        *cache_file      = NULL;
    struct stat  stbuf; 


    if ( p->mmdf_separator ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Can't write cache file for %s; spool is in mmdf format",
                  p->user );
        return POP_FAILURE;
    }

    if ( genpath ( p, cache_name, sizeof(cache_name), GNPH_CACHE ) < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to get .cache path for %.100s",
                  p->user );
        return POP_FAILURE;
    }

    cache_fd = open ( cache_name, O_RDWR | O_CREAT | O_TRUNC, 0600 );
    if ( cache_fd == -1 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to open cache file '%s': %s (%d)",
                  cache_name, STRERROR(errno), errno );
        return POP_FAILURE;
    }

    cache_file = fdopen ( cache_fd, "wb" );
    if ( cache_file == NULL ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to open cache fd as stream '%s': %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close ( cache_fd );
        return POP_FAILURE;
    }

    rslt = fstat ( spool_fd, &stbuf );
    if ( rslt < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "fstat() failed for spool (%d): %s (%d)",
                  spool_fd, STRERROR(errno), errno );
        close  ( cache_fd );
        fclose ( cache_file );
        return POP_FAILURE;
    }

    /*
     * Get rid of any entries for deleted messages in MsgInfoList
     */
    if ( p->msgs_deleted > 0 ) {
        MsgInfoList    *new_mp     = NULL;
        int             msgx;
        int             num  = 1;
        int             vnum = 1;
        int             keep_count = p->msg_count - p->msgs_deleted;
        long            new_size   = 0;
        MsgInfoList    *newp = NULL;


        new_size = keep_count * sizeof ( MsgInfoList );
        new_mp   = (MsgInfoList *) malloc ( new_size );
        if ( new_mp == NULL ) {
            pop_log ( p, POP_PRIORITY, HERE,
                      "Unable to allocate %ld for new MsgInfoList",
                      new_size );
            close  ( cache_fd );
            fclose ( cache_file );
            return POP_FAILURE;
        }

        DEBUG_LOG3 ( p, "Keeping %d of %d msgs; allocated %ld for new toc",
                     keep_count, p->msg_count, new_size );

        keep_count = 0;
        for ( msgx = 0; msgx < p->msg_count; ++msgx ) {
            newp = &new_mp [ keep_count ];
            if ( mp[ msgx ].del_flag == FALSE ) {
                *newp = mp [ msgx ];
                newp->number = num++;
                newp->visible_num =  mp[ msgx ].hide_flag ? 0 : vnum++;
                newp->orig_retr_state = newp->retr_flag;
                DEBUG_LOG5 ( p, "Copied msg %d as %d (%d) from toc %d to %d",
                             mp[ msgx ].number, newp->number,
                             newp->visible_num, msgx, keep_count );
                keep_count++;
            }
        }
    
    mp        = new_mp;
    toc_size  = new_size;
    msg_count = keep_count;
    }
    else {
        int             msgx = 0;
        MsgInfoList    *ptr  = NULL;
        
        while ( msgx < msg_count ) {
            ptr = &mp [ msgx++ ];
            ptr->orig_retr_state = ptr->retr_flag;
        }
    }


    memcpy ( header.magic_num, CACHE_MAGIC_NUM, sizeof(header.magic_num) );
    header.version      = CACHE_VERSION;
    header.toc_str_sz   = sizeof ( MsgInfoList );
    header.modtime      = stbuf.st_mtime;
    header.msg_count    = msg_count;
    header.toc_size     = toc_size;
    header.drop_size    = p->drop_size;
    header.spool_end    = p->spool_end;
    header.eol          = '\n';

    rslt = write ( cache_fd, &header, sizeof (header) );
    rslt = write ( cache_fd, mp, header.toc_size );
    if ( rslt < header.toc_size ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "I/O error writing .cache file %s: %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close  ( cache_fd );
        fclose ( cache_file );
        return POP_FAILURE;
    }

    if ( p->msgs_deleted > 0 )
        free ( mp ); /* be nice */

    rslt = close ( cache_fd );
    if ( rslt < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "I/O error closing .cache file %s: %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close  ( cache_fd );
        fclose ( cache_file );
        return POP_FAILURE;
    }

    DEBUG_LOG5 ( p, "Wrote cache file \"%s\"; msg_count=%d; toc_size=%ld; "
                    "drop_size=%ld; spool_end=%ld",
                 cache_name, header.msg_count, header.toc_size,
                 header.drop_size, (long) header.spool_end );

    return POP_SUCCESS;
}


/*
 * Read the cache file into a newly-allocated MsgInfoList structure.
 *
 * Parameters:
 *     p:           points to POP structure.
 *     spool_fd:    file descriptor for spool.
 *     mp:          points MsgInfoList structure pointer which we allocate.
 *
 * Result:
 *     POP_SUCCESS or POP_FAILURE.
 */
int
cache_read ( POP *p, int spool_fd, MsgInfoList **mp )
{
    char         cache_name [ MAXFILENAME ];
    cache_hdr    header;
    MsgInfoList *mp_buf          = NULL;
    long         rslt            = 0;
    int          cache_fd        = -1;
    long         str_size        = 0;
    struct stat  stbuf; 


    if ( genpath ( p, cache_name, sizeof(cache_name), GNPH_CACHE ) < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to get .cache path for %.100s",
                  p->user );
        return POP_FAILURE;
    }

    cache_fd = open ( cache_name, O_RDONLY );
    if ( cache_fd == -1 ) {
        if ( errno == 2 ) {
            DEBUG_LOG1 ( p, "(no cache file %s)", cache_name );
        } else
            pop_log ( p, POP_PRIORITY, HERE,
                      "Unable to open cache file '%s': %s (%d)",
                      cache_name, STRERROR(errno), errno );
        return POP_FAILURE;
    }

    rslt = read ( cache_fd, (char *) &header, sizeof(header) );
    if ( rslt < (long) sizeof(header) ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "I/O error reading .cache file %s: %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close  ( cache_fd );
        return POP_FAILURE;
    }

    if ( memcmp ( header.magic_num, 
                  CACHE_MAGIC_NUM, 
                  sizeof(header.magic_num) ) != 0 ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Invalid .cache file %s",
                  cache_name );
        close  ( cache_fd );
        return POP_FAILURE;
    }

    if ( header.version != CACHE_VERSION ) {
        pop_log ( p, POP_DEBUG, HERE,
                  "Version mismatch on .cache file %s: %u vs %u",
                  cache_name, header.version, CACHE_VERSION );
        close  ( cache_fd );
        return POP_FAILURE;
    }
    
    if ( header.toc_str_sz != sizeof ( MsgInfoList ) ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "toc size mismatch in .cache file %s: %lu vs %u",
                  cache_name, header.toc_str_sz, sizeof ( MsgInfoList ) );
        close  ( cache_fd );
        return POP_FAILURE;
    }

    rslt = fstat ( spool_fd, &stbuf );
    if ( rslt < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "fstat() failed for spool (%d) %s (%d)",
                  spool_fd, STRERROR(errno), errno );
        close  ( cache_fd );
        return POP_FAILURE;
    }

    if ( header.modtime > stbuf.st_mtime ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "spool older than cache file %s",
                  cache_name );
        close ( cache_fd );
        return POP_FAILURE;
    }

    if ( header.spool_end > stbuf.st_size ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "spool smaller than indicated in cache file %s "
                  "(expected %ld; found %ld)",
                  cache_name, (long) header.spool_end, (long) stbuf.st_size );
        close ( cache_fd );
        return POP_FAILURE;
    }

    /*
     * For now, exit if the spool has been modified or is larger than
     * indicated in the cache.  This is temporary.
     */
    if ( header.spool_end != stbuf.st_size ||
         header.modtime   != stbuf.st_mtime ) {
        DEBUG_LOG5 ( p, "spool modified or larger than indicated in "
                     "cache: %s (%ld %ld vs %ld %ld)",
                     cache_name, (long) header.spool_end, header.modtime,
                     (long) stbuf.st_size, stbuf.st_mtime );
        close ( cache_fd );
        return POP_FAILURE;
    }


    /*
     * Now we know that the cache information is probably compatible
     * with the spool.  Note that the local delivery agent may have
     * added messages to the spool since the cache was written,
     * in which case the new messages need to be processed normally.
     */

    /*
     * Allocate the MsgInfoList structure.  Add room for one more msg,
     * to make sure no one walks off the end.
     */
    str_size = header.toc_size + sizeof ( MsgInfoList );
    mp_buf = (MsgInfoList *) malloc ( str_size );
    if ( mp_buf == NULL ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Can't allocate memory (%ld) for message list",
                  str_size );
        close ( cache_fd );
        return POP_FAILURE;
    }


    rslt = read ( cache_fd, mp_buf, header.toc_size );
    if ( rslt < header.toc_size ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "I/O error reading .cache file %s: %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close  ( cache_fd );
        free   ( mp_buf );
        return POP_FAILURE;
    }

    rslt = close ( cache_fd );
    if ( rslt < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "I/O error closing .cache file %s: %s (%d)",
                  cache_name, STRERROR(errno), errno );
        close  ( cache_fd );
        free   ( mp_buf   );
        return POP_FAILURE;
    }

    if ( *mp != NULL )
        free ( *mp );
    *mp = mp_buf;

    p->drop_size         = header.drop_size;
    p->msg_count         = header.msg_count;
    if ( p->msg_count > 0 ) {
        p->first_msg_hidden  = (*mp) [ 0 ].hide_flag;
        p->visible_msg_count = (*mp) [ header.msg_count - 1 ].visible_num;
    }

    DEBUG_LOG7 ( p, "Read cache file \"%s\"; msg_count=%d; toc_size=%ld; "
                    "drop_size=%ld; spool_end=%ld; first_msg_hidden=%d; "
                    "visible_msg_count=%d",
                 cache_name, header.msg_count, header.toc_size,
                 header.drop_size, (long) header.spool_end,
                 p->first_msg_hidden, p->visible_msg_count );

    return POP_SUCCESS;
}


/*
 * Unlink the cache file if it exists.
 */
void
cache_unlink ( POP *p )
{
    char         cache_name [ MAXFILENAME ];


    if ( genpath ( p, cache_name, sizeof(cache_name), GNPH_CACHE ) < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to get .cache path for %.100s",
                  p->user );
        return;
    }

    unlink ( cache_name );

    DEBUG_LOG1 ( p, "Unlinked cache file %s", cache_name );
}

