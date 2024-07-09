/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */


/* Changes:
 *
 *   02/08/01  [rcg]
 *            - Now using p->nMaxBulls instead of NEWBULLCNT.
 *
 *   01/31/01  [rcg]
 *            - Used saved-off original group ID instead of modified
 *              group, which is normally set to group MAIL.
 *
 *   01/30/01  [rcg]
 *            - Now using p->bGroup_bulls instead of GROUP_BULLS.
 *            - Now using p->bUpdate_status_hdrs instead of NO_STATUS.
 *
 *   12/27/00  [rcg]
 *            - Bulletins whose group is 'ALL' are sent to all users.
 *            - Bulletins with invalid groups are sent to no users.
 *
 *   09/22/00  [rcg]
 *            - Handle static mp->uidl_str.
 *
 *   07/07/00  [rcg]
 *            - Fixed incorrect calculation of visible_msg_num.
 *
 *   06/30/00  [rcg]
 *            - Replaced strncpy/strncat with strlcpy/strlcat, which
 *             guarantee to null-terminate the buffer, and are easier
 *             to use safely.
 *
 *   06/16/00  [rcg]
 *            - Properly clean up during errors.
 *            - Using passed-in passwd structure instead of calling
 *              getpwnam yet again.
 *
 *   03/08/00  [rcg]
 *            - Use pop_log instead of pop_msg, since our caller does not
 *              exit if we return POP_FAILURE.
 *
 *   03/03/00  [rcg]
 *            - Added bulletin group feature, based on patch by Mikolaj 
 *              Rydzewski.
 *
 *   02/14/00  [rcg]
 *            - Added umask() calls around fopen calls which could
 *              create .popbull file, to avoid creating it 
 *              group-writable.
 *
 *   01/18/00  [rcg]
 *            - Added --enable-popbulldir to specify alternate location for
 *              user's popbull files.
 *
 */

/*
 * bullcopy
 *
 */


#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <flock.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#ifdef BULLDB
#  include <sys/file.h>

#  ifdef GDBM
#    include <gdbm.h>
#  else
#    if HAVE_NDBM_H
#      include <ndbm.h>
#    endif /* HAVE_NDBM_H */
#  endif /* GDBM */

#  ifndef dbm_dirfno
     extern int dbm_dirfno();
#  endif
#endif /* BULLDB */

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* ifdef HAVE_DIRENT_H */
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen

#  ifdef HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif /* HAVE_SYS_NDIR_H */

#  ifdef HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif /* HAVE_SYS_DIR_H */

#  if HAVE_NDIR_H
#    include <ndir.h>
#  endif /* HAVE_NDIR_H */
#endif /* else of ifdef HAVE_DIRENT_H */

#ifdef HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef HAVE_BCOPY
#  define bcopy(src,dest,len)   (void) (memcpy(dest,src,len))
#endif /* ndef HAVE_BCOPY */

#include "popper.h"
#include "string_util.h"


static int sequence = 0;
static long timestamp;
static char *ERRMESG = "[SYS/TEMP] Unable to copy bulletin %s to pop dropbox %s: %s (%d)";


typedef struct _file_list file_list;

struct _file_list {
    char *bull_name;
    long bull_value;
    file_list *next;
};


int CopyOneBull ( POP *p, long bnum, char *name );


/*
 * Based on patch by Mikolaj Rydzewski.
 *
 * Depending on bulletin's filename, decide if user gets to receive it.
 *
 * A group name is contained in the bulletin's filename.  If the user is 
 * a member of this group then he/she gets it.  The group name is the second
 * token in the bulletin's filename, for example, 
 * "001.staff.new_program_installed".  (Tokens are separated by periods.)
 
 * If there is no second token, the bulletins is delivered to all users.
 *
 * If the second token doesn't match any group in the system (returned by 
 * getgrnam()) then the bulletin is not delivered to any user.  However, a
 * group name of 'ALL' is sent to all users. 
 *
 * If the user is not a member of any group, no bulletins are shown.
 */
BOOL 
IsBulletinForMe ( POP *p, struct passwd *pwd, const char *bull_name )

{
    char           *token;
    char            group_name [ MAXUSERNAMELEN ];
    char           *ptr;
    char          **u_list;
    size_t          group_len = 0;
    struct group   *grp;
    struct group   *pwgrp;
    gid_t           b_group;


    token = strchr ( bull_name, '.' );

    /* 
     * there's no token -- bulletin is for everybody 
     */
    if ( token == NULL ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Can't find '.' in bulletin name; delivering to all users: %.128s",
                  bull_name );
        return TRUE;
    }

    ptr = group_name;
    token++;
    while ( *token != '\0' && 
            *token != '.'  && 
            group_len < sizeof(group_name) -1 ) {
        *(ptr++) = *(token++);
        group_len++;
    }
    *ptr = 0;

    pwgrp = getgrgid ( p->orig_group );
    /*
     * If user is not a member of a group, no bulletins are shown.
     */
    if ( pwgrp == NULL ) {
        DEBUG_LOG3 ( p, "getgrgid failed for gid %u: %s (%d)",
                    (unsigned) p->orig_group, STRERROR(errno), errno );
        return FALSE;
    }

    DEBUG_LOG2 ( p, "Primary gid for user %s is %u", 
                 p->user, (unsigned) p->orig_group );

    /*
     * if group is 'ALL', deliver to all users.
     */
    if ( strcmp ( group_name, "ALL" ) == 0 ) {
        DEBUG_LOG1 ( p, "Bulletin is for 'ALL'; delivering to all users: %.128s",
                     bull_name );
        return TRUE;
    }
    /* 
     * now, let's find specified group 
     */
    grp = getgrnam ( group_name );

    /* 
     * if there's no such group bulletin is not delivered to anyone 
     */
    if ( grp == NULL ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Non-existent group \"%.128s\" in bulletin; "
                  "bulletin omitted: %.128s",
                  group_name, bull_name );
        return FALSE;
    }
    
    /*
     * Save bulletin's group ID
     */
    b_group = grp->gr_gid;

    /*
     * If user's primary group matches the bulletin's group, we matched
     * it the easy way.
     */
    if ( p->orig_group == b_group ) {
        DEBUG_LOG1 ( p, "User's gid matches bulletin's: %s", bull_name );
        return TRUE;
    }
    
    /* 
     * look for this user in this group's member list, if there isn't,
     * bulletin is obviously not for him/her.
     *
     * Note that for every new bulletin we have to check (almost) all
     * groups in the system to find whether user belongs to it or not.
     * If you have a lot of users and a lot of new bulletins it may have
     * an impact on performance. 
     */
    u_list = grp->gr_mem; 
    while ( *u_list != NULL ) {
        if ( !strcmp ( *u_list, p->user ) ) {
            DEBUG_LOG2 ( p, "Found user %s in group list for %s",
                         p->user, bull_name );
            return TRUE; /* user found! */
            }
        u_list++;
    }

    DEBUG_LOG2 ( p, "User %s not found in group list for %s",
                 p->user, bull_name );
    return FALSE;
}


file_list *
insert_list ( POP *p, file_list *head, char *name )
{
    long       new_bull;
    file_list *new_rec, *current;

    new_bull = atol ( name );

    new_rec = (file_list *)malloc(sizeof(file_list));
    new_rec->next = NULL;
    new_rec->bull_value = new_bull;
    new_rec->bull_name = (char *)malloc(strlen(name) + 1);
    strcpy ( new_rec->bull_name, name );

    current = head;
        
    if ( head == NULL ) {
        DEBUG_LOG1 ( p, "Bull %ld is head of new list", new_bull );
        return ( new_rec );
    } else {
        if ( head->bull_value > new_bull ) {
            DEBUG_LOG2 ( p, "Bull %ld being inserted at head (before %ld)",
                         new_bull, head->bull_value );
            new_rec->next = head;
            head = new_rec;
            return ( head );
        }
    }

    while ( current->next ) {
        if ( current->next->bull_value > new_bull )
            break;
        current = current->next;
    }

    /*
    DEBUG_LOG3 ( p, "Bull %ld being inserted between %ld and %ld",
                 new_bull, current->bull_value, 
                 (current->next ? current->next->bull_value : -1) );
    */
    new_rec->next = current->next;
    current->next = new_rec;
    return( head );
}


/*
 *  pop_bull: Append any new bulletins to the end of the user's
 *  temporary maildrop. This function is called only from pop_dropcopy.
 *  There is no need to open or close the dbms since they are done by 
 *  the calling function.
 */

int
pop_bull ( POP *p, struct passwd *pwp )
{
   char             popBullName    [ MAXFILENAME ];
#ifdef POPBULLDIR
   char             oldPopBullName [ MAXFILENAME ];
#endif
   FILE            *popBull                     = NULL;
   DIR             *dirp;
   file_list       *list                        = NULL;
   struct dirent   *dp;
   long             maxBullNumber               = 0;
   long             bullNumber;
   long             lastBullSent;
   char             buffer [ MAXMSGLINELEN ];
   int              res                         = POP_SUCCESS;
   int              err                         = 0;
   int              save_count;
   int              save_visible_count;
   long             save_drop_size;

#ifdef BULLDB
   datum            bull_count;
   datum            name;
#else
   mode_t           my_umask;
#endif /* BULLDB */

   /* 
    * Construct full path name of .popbull file.
    */
#ifdef POPBULLDIR
   strlcpy  ( popBullName, POPBULLDIR, sizeof(popBullName) );
   if ( popBullName [ strlen(popBullName) -1 ] != '/' )
       strlcat ( popBullName, "/", sizeof(popBullName) );
   strlcat ( popBullName, p->user,    sizeof(popBullName) );
   strlcat ( popBullName, ".popbull", sizeof(popBullName) );
   
   strlcpy ( oldPopBullName, pwp->pw_dir, sizeof(oldPopBullName) );
   strlcat ( oldPopBullName, "/.popbull", sizeof(oldPopBullName) );
#else /* not using POPBULLDIR */
   strlcpy ( popBullName, pwp->pw_dir, sizeof(popBullName) );
   strlcat ( popBullName, "/.popbull", sizeof(popBullName) );
#endif /* POPBULLDIR */

   /* 
    * Scan bulletin directory and compute the maximum current
    * bulletin number. 
    */
   dirp = opendir ( p->bulldir );
   if ( dirp == NULL ) {
      pop_log ( p, POP_PRIORITY, HERE,
                "Unable to open bulletin directory '%.128s': %s (%d)", 
                p->bulldir, strerror(errno), errno );
      return POP_FAILURE;
   }

    while ( ( dp = readdir ( dirp ) ) != NULL ) {
      if ( isdigit ( (int) *dp->d_name ) == FALSE ) 
         continue;
      bullNumber = atol ( dp->d_name );
      DEBUG_LOG2 ( p, "Found bulletin #%ld: %s",
                   bullNumber, dp->d_name );
      /*
       * We found a bulletin.  Insert it if it is for the user.
       */
      if ( p->bGroup_bulls == FALSE ||
           IsBulletinForMe ( p, pwp, dp->d_name ) 
         ) {
        if ( bullNumber > maxBullNumber ) 
            maxBullNumber = bullNumber;
        list = insert_list ( p, list, dp->d_name );
      }
   } /* while loop */

   closedir ( dirp );
   DEBUG_LOG1 ( p, "Maximum bulletin: %ld", maxBullNumber );

   timestamp = time(0);

   /* 
    * Open the user's .popbull file and read the number of the last
    * bulletin sent to this user. If the file doesn't exist, create
    * it and seed it with the current max bulletin number.
    *
    * p->nMaxBulls specifies the maximum number of old bulletins
    * seen by new users. 
    */

#ifdef BULLDB
   name.dptr  = p->user;
   name.dsize = strlen ( p->user ) + 1;

#  ifdef GDBM
   bull_count = gdbm_fetch ( p->bull_db, name );
#  else
   bull_count = dbm_fetch ( p->bull_db, name );
#  endif /* GDBM */

   if ( bull_count.dptr == NULL ) {
       /* 
        * If the database does not have a value, check the users .popbull
        * file.  If it's not empty, then use the value there, otherwise,
        * create a new value.
        */
       DEBUG_LOG1 ( p, "No bulletin db record for %s", p->user );
       
       popBull = fopen ( popBullName, "r" );

#  ifdef POPBULLDIR
       if ( popBull == NULL ) {
           popBull = fopen ( oldPopBullName, "r" );
           DEBUG_LOG2 ( p, "User's %s file doesn't exist -- trying %s",
                        popBullName, oldPopBullName );
       }
#  endif /* POPBULLDIR */
       
       if ( popBull == NULL ) {
           DEBUG_LOG1 ( p, "No .popbull file for %s", p->user );
           lastBullSent = maxBullNumber - p->nMaxBulls;
       }
       else
       if ( fgets ( buffer, MAXMSGLINELEN, popBull ) == NULL ) {
           DEBUG_LOG1 ( p, "Popbull file %s empty", popBullName );
           lastBullSent = maxBullNumber - p->nMaxBulls; 
           fclose ( popBull );
           popBull = NULL;
       }
       else
       if ( isdigit ( (int) *buffer ) == FALSE ) {
           DEBUG_LOG2 ( p, "Invalid data in file %s: \"%.128s\"",
                        popBullName, buffer );
           lastBullSent = maxBullNumber - p->nMaxBulls;
           fclose ( popBull );
           popBull = NULL;
       }
       else {
           lastBullSent = atol( buffer );
           DEBUG_LOG2 ( p, "Last bulletin sent to %s was %ld",
                        p->user, lastBullSent );
           fclose ( popBull );
           popBull = NULL;
       }
       if ( lastBullSent < 0 )
           lastBullSent = 0;

       bull_count.dptr  = (char *)&lastBullSent;
       bull_count.dsize = sizeof(lastBullSent);

       /* 
        * Block while waiting for a lock to create the entry 
        */
       if ( flock ( dbm_dirfno(p->bull_db), LOCK_EX ) == -1 ) {
           pop_log ( p, POP_PRIORITY, HERE, 
                     "[SYS/TEMP] Bulletin database lock failed" );
           return POP_FAILURE;
       }
           
#  ifdef GDBM
       gdbm_store ( p->bull_db, name, bull_count, GDBM_REPLACE );
#  else
       dbm_store  ( p->bull_db, name, bull_count, DBM_REPLACE  );
#  endif /* GDBM */
       
       flock ( dbm_dirfno(p->bull_db), LOCK_UN );

   } /* bull_count.dptr == NULL */
   else {
      bcopy ( bull_count.dptr, &lastBullSent, bull_count.dsize );
      DEBUG_LOG2 ( p, "Last bulletin sent to %s was %ld",
                        p->user, lastBullSent );
   }
      
#else /* not BULLDB */

   popBull = fopen ( popBullName, "r" );

#  ifdef POPBULLDIR
   if ( popBull == NULL ) {
       popBull = fopen ( oldPopBullName, "r" );
       DEBUG_LOG2 ( p, "User's %s file doesn't exist -- trying %s",
                    popBullName, oldPopBullName );
   }
#  endif /* POPBULLDIR */

   if ( popBull == NULL ) {
       DEBUG_LOG1 ( p, "No .popbull file for %s", p->user );
       lastBullSent = maxBullNumber - p->nMaxBulls;
   }
   else
   if ( fgets ( buffer, MAXMSGLINELEN, popBull ) == NULL ) {
       DEBUG_LOG1 ( p, "Popbull file %s empty; assuming "
                       "all bulletins sent", popBullName );
       lastBullSent = maxBullNumber;
       fclose ( popBull );
       /*
        * Try to store the current max bull number in the user's
        * .popbull file so the user can start getting new
        * bulletins.
        */
       my_umask = umask ( 0077 ); /* .popbull shouldn't be group-writable */
       popBull  = fopen ( popBullName, "w" );
       umask ( my_umask );

       if ( popBull == NULL 
            || fprintf ( popBull, "%ld\n", maxBullNumber ) < 0
            || fclose ( popBull ) == EOF ) {
                   pop_log ( p, POP_DEBUG, HERE,
                             "Unable to create/write/close "
                             "file %s: %s (%d)",
                             popBullName, strerror(errno), errno );
       }
       popBull = NULL;
   }
   else
   if ( !isdigit ( (int) *buffer ) ) {
       DEBUG_LOG2 ( p, "Invalid data in file %s: \"%.128s\"",
                    popBullName, buffer );
       lastBullSent = maxBullNumber - p->nMaxBulls;
       fclose ( popBull );
       unlink ( popBullName );
       /*
        * Try to store the current max bull number in the user's
        * .popbull file so the user can start getting new
        * bulletins.
        */
       my_umask = umask ( 0077 ); /* .popbull shouldn't be group-writable */
       popBull  = fopen ( popBullName, "w" );
       umask ( my_umask );

       if ( popBull == NULL 
            || fprintf ( popBull, "%ld\n", maxBullNumber ) < 0
            || fclose ( popBull ) == EOF ) {
                   pop_log ( p, POP_DEBUG, HERE,
                             "Unable to create/write/close "
                             "file %s: %s (%d)",
                             popBullName, strerror(errno), errno );
       }
       popBull = NULL;
   }
   else {
       lastBullSent = atol ( buffer );
       DEBUG_LOG2 ( p, "Last bulletin sent to %s was %ld",
                    p->user, lastBullSent );
       fclose ( popBull );
       popBull = NULL;
   }
   if ( lastBullSent < 0 )
       lastBullSent = 0;

#endif /* BULLDB */

   /* 
    * If there aren't any new bulletins for this user, return. 
    */
   if ( lastBullSent >= maxBullNumber ) 
       return POP_SUCCESS;

   /* 
    * Append the new bulletins to the end of the user's temporary 
    * mail drop. 
    */

   res                = POP_SUCCESS;
   save_count         = p->msg_count;
   save_visible_count = p->visible_msg_count;
   save_drop_size     = p->drop_size;

   while ( list != NULL ) {
       if ( list->bull_value > lastBullSent ) {
            res = CopyOneBull ( p, list->bull_value, list->bull_name );
            if ( res != POP_SUCCESS ) {
                break;
            }
       }
       list = list->next;
   }

   if ( res == POP_SUCCESS ) {

#ifdef BULLDB
       bull_count.dptr  = (char *) &maxBullNumber;
       bull_count.dsize = sizeof(maxBullNumber);
       err = flock ( dbm_dirfno ( p->bull_db ), LOCK_EX );
       if ( err == -1 ) {
           pop_log ( p, POP_PRIORITY, HERE, 
                     "[SYS/TEMP] Bulletin database lock failed: %s (%d)",
                     strerror(errno), errno );
           res = POP_FAILURE;
           goto Exit;
       }
#  ifdef GDBM
       gdbm_store ( p->bull_db, name, bull_count, GDBM_REPLACE );
#  else
       dbm_store  ( p->bull_db, name, bull_count, DBM_REPLACE );
#  endif /* GDBM */
       flock ( dbm_dirfno ( p->bull_db ), LOCK_UN );
#else /* not BULLDB */

       /* 
        * Update the user's .popbull file.  If the write or
        * close fails we end up with an empty file hanging
        * around, which will prevent the user from seeing
        * bulletins.  This can happen if the user is over
        * quota or the disk is full, for example.
        */
       my_umask = umask ( 0077 ); /* .popbull shouldn't be group-writable */
       popBull = fopen ( popBullName, "w" );
       umask ( my_umask );

       if ( popBull == NULL ) {
            
           pop_log ( p, POP_PRIORITY, HERE, 
                     "Unable to open %s for writing: %s (%d)", 
                     popBullName, strerror(errno), errno );
           res = POP_FAILURE;
           goto Exit;
       }
       if ( fprintf ( popBull, "%ld\n", maxBullNumber ) < 0 ) {
           pop_log ( p, POP_PRIORITY, HERE,
                     "Unable to write to %s: %s (%d)",
                     popBullName, strerror(errno), errno );
           res = POP_FAILURE;
           goto Exit;
       }
       if ( fclose ( popBull ) == EOF ) {
           pop_log ( p, POP_PRIORITY, HERE,
                     "Unable to close %s after writing: %s (%d)",
                     popBullName, strerror(errno), errno );
           res = POP_FAILURE;
           goto Exit;
       }
#endif /* BULLDB */
   } /* res == POP_SUCCESS */

Exit:
   if ( res != POP_SUCCESS ) {
       DEBUG_LOG6 ( p, "pop_bull returning failure; restoring msg_count "
                       "from %d to %d; visible from %d to %d; "
                       "drop size from %ld to %ld",
                    p->msg_count,         save_count,
                    p->visible_msg_count, save_visible_count,
                    p->drop_size,         save_drop_size );
       p->msg_count         = save_count;
       p->visible_msg_count = save_visible_count;
       p->drop_size         = save_drop_size;
   }
   
   return res;
}

extern int newline; /* lives in pop_dropcopy.c */

/*
 *  CopyOneBull: Append a single bulletin file to the end of the
 *  temporary maildrop file.
 */

int
CopyOneBull ( POP *p, long bnum, char *name )
{
    FILE          *bull;
    char           buffer [ MAXMSGLINELEN ];
    BOOL           in_header            = TRUE;
    BOOL           first_line           = TRUE;
    int            nchar;
    int            msg_num;
    int            msg_vis_num          = 0;
    int            msg_ends_in_nl       = 0;
    char           bullName [ 256 ];
    MsgInfoList   *mp;               /* Pointer to message info list */


    msg_num = p->msg_count;
    while ( msg_vis_num == 0 && msg_num > 0 ) {
        mp = &p->mlp [ --msg_num ];
        if ( mp->visible_num > 0 )
            msg_vis_num = mp->visible_num;
    }

    msg_num = p->msg_count;
    p->msg_count = ( ( ( p->msg_count - 1 ) / ALLOC_MSGS ) + 1 ) * ALLOC_MSGS;

    sprintf ( bullName, "%s/%s", p->bulldir, name );
    bull = fopen ( bullName, "r" );
    if ( bull == NULL ) {
       pop_log ( p, POP_PRIORITY, HERE,
                 "Unable to open bulletin file %s: %s (%d)", 
                 name, strerror(errno), errno );
       return POP_FAILURE;
    }

    newline = 1;

    if ( ( fgets ( buffer, MAXMSGLINELEN, bull ) != NULL ) 
         && ! ( isfromline(buffer) ) ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Bulletin %s does not start with a valid \"From \" separator",
                  name );
        fclose ( bull );
        return POP_FAILURE;
    }

    /* 
     * Just append the message, no Status or UIDL updates here 
     */
    if ( p->bUpdate_status_hdrs )
        p->dirty = 1;

    mp = p->mlp + msg_num - 1;

    if ( ++msg_num > p->msg_count ) {
        p->msg_count += ALLOC_MSGS;
        p->mlp = (MsgInfoList *) realloc ( p->mlp,
                                           p->msg_count * sizeof(MsgInfoList) );
        if ( p->mlp == NULL)  {
            p->msg_count = 0;
            pop_log ( p, POP_PRIORITY, HERE,
                      "[SYS/TEMP] Bull: Can't build message list for"
                      " '%s': Out of memory",
                      p->user );
            return POP_FAILURE;
        }

        mp = p->mlp + msg_num - 2;
    }

    ++mp;
    mp->number      = msg_num;
    mp->visible_num = ++msg_vis_num;
    mp->length      = 0;
    mp->lines       = 0;
    mp->body_lines  = 0;
    mp->offset      = ftell(p->drop);
    mp->del_flag    = FALSE;
    mp->hide_flag   = FALSE;
    mp->retr_flag   = FALSE;

    if ( fputs ( buffer, p->drop ) == EOF ) {
        pop_log ( p, POP_PRIORITY, HERE, ERRMESG, 
                  name, p->temp_drop, 
                  strerror(errno), errno );
        return POP_FAILURE;
    }

    p->drop_size += strlen(buffer);
    mp->lines++;

    sprintf ( mp->uidl_str, "%ld.%03d\n", timestamp, sequence++ );
    sprintf ( buffer, "X-UIDL: %s", mp->uidl_str );
    if ( fputs ( buffer, p->drop ) == EOF ) {
        pop_log ( p, POP_PRIORITY, HERE, ERRMESG, 
                  name, p->temp_drop, 
                  strerror(errno), errno );
        return POP_FAILURE;
    }

    mp->length   += strlen ( buffer );
    p->drop_size += mp->length;
    mp->lines    ++;

    DEBUG_LOG5 ( p, "Bull msg %lu (as %d [vis: %d]) being added to "
                    "list, offset %lu: %s",
                 bnum, mp->number, mp->visible_num, mp->offset, name );
    

    first_line = FALSE;

    while ( fgets ( buffer, MAXMSGLINELEN, bull ) != NULL ) {
        nchar = strlen(buffer);

        if ( in_header ) { /* Header */
            if ( !strncasecmp(buffer, "X-UIDL:", 7) ) {
                continue;       /* Skip any UIDLs */

            } else if ( strncasecmp(buffer, "To:", 3) == 0 ) {
                sprintf(buffer,"To: %s@%s\n", p->user, p->myhost);
                nchar = strlen(buffer);

            } else if ( strncasecmp(buffer, "Status:", 7) == 0 ) {
                continue;

            } else if ( *buffer == '\n' ) {
                in_header = FALSE;
                mp->body_lines = 1; /* Count new line as first line of body */
            }
        }
        else {
            mp->body_lines++;
            msg_ends_in_nl = ( *buffer == '\n' );
        }

        mp->length   += nchar + 1; /* +1 is for CRLF */
        p->drop_size += nchar + 1;
        mp->lines    ++;

        if ( fputs ( buffer, p->drop ) == EOF ) {
            pop_log ( p, POP_PRIORITY, HERE, ERRMESG, 
                      name, p->temp_drop, 
                      strerror(errno), errno );
            return POP_FAILURE;
        }
    } /* while loop */

    /* 
     * Make sure msg ends with two newlines 
     */
    if ( !CONTENT_LENGTH && !msg_ends_in_nl ) {
        if ( fputc ( '\n', p->drop ) == EOF ) {
            pop_log ( p, POP_PRIORITY, HERE, ERRMESG, 
                      name, p->temp_drop, 
                      strerror(errno), errno );
            return POP_FAILURE;
        }
        p->drop_size   ++;
        mp->length     ++;
        mp->lines      ++;
        mp->body_lines ++;
        DEBUG_LOG1 ( p, "Bulletin does not end in two new-lines: %s",
                     name );
    }
    
    fflush ( p->drop );

    p->msg_count         = msg_num;
    p->visible_msg_count = msg_vis_num;
    fclose ( bull );

    return POP_SUCCESS;
}









