/* popauth.c - manipulate POP authentication DB:
 *   aka "apopauth" [same thing really] and "scramauth" [manipulate 
 *   scram entries]
 */


/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 *
 * Revisions:
 *
 *   01/02/00  [Randall Gellens]
 *           - Added checks for dbm errors.
 *
 *   12/29/00  [Randall Gellens]
 *           - Added -safe flag for -init.
 *
 *    8/20/00  [Randall Gellens]
 *           - Added logging and tracing.
 *           - Reorganized command-line syntax.
 *           - Users can now list their own entry.  '-list ALL' lists
 *             all users.
 *
 *    6/30/00  [Randall Gellens]
 *           - Ensured all uses of strncpy properly null-terminate; use
 *             strlcpy for many.
 *
 *    6/14/00  [Randall Gellens]
 *           - Fixed passed-in password so it doesn't prompt to confirm.
 *           - Added -help switch.
 *
 *    6/6/00   [Randall Gellens]
 *           - Allow password to be passed as extra parameter to popauth
 *             so it can be used in a scripted envirovment (based on patch
 *             by Dejan Ilic).
 *           - Cast value.dptr to char * when dereferencing to avoid errors
 *             on IRIX, using patch from Rick Troxel.
 *
 *    4/21/00  [Randall Gellens]
 *           - Remove __STDC__ (some compilers don't define this when extensions
 *             are enabled, leading to the erroneous conclusion that ANSI C is
 *             not supported.  These days, it is far more likely that a compiler
 *             fails to correctly handle K&R syntax than ANSI.)
 *
 *    4/29/99  [Randall Gellens]
 *           - Don't reference auth_file when GDBM is undefined (syntax error)
 *
 *    2/04/99  [Randall Gellens]
 *           - Don't flock() when using gdbm.
 *
 *    2/13/98  [Rob Duwors]
 *           - Added SCRAM support.
 */

/*
 *. All fields for a given user entry are null terminated strings.
 *. APOP field come first [may be a null string if logically absent].
 *. SCRAM field come second [may be physically or logically absent].
 *. SCRAM fields are kept in base64 representation of the original 40 byte
 *   SCRAM verifier (salt, client id, and server key).
 *. Deleting a user still deletes the entire entry [APOP and SCRAM 
 *   disappear].
 *. parameter -list also shows the presence of APOP and SCRAM entries.
 *. old authentication databases and APOP password fields can be used 
 *   as is [the missing SCRAM field will be treated as null]
 *. a specific field [APOP or SCRAM] can be cleared to null by using
 *   password "*", i.e. a single star character as the new password.
 *. scramauth by default will clear the APOP field [disabling APOP 
 *   access for that user].  This can be stopped by using the -only   
 *   switch.
 *. popauth/apopauth can ONLY change the APOP password, never distrubing 
 *   the the SCRAM field.
 *. APOP password and SCRAM pass phrases are not related to each other 
 *   in any way ([a]pop|scram)auth, i.e. the APOP password can not 
 *   compromise the SCRAM verifiers, except by user choice.
 */

#include "config.h"

#ifdef SCRAM
#  include <md5.h>
#  include <hmac-md5.h>
#  include <scram.h>
#endif /* SCRAM */

#ifdef HAVE_GDBM_H
#  include <gdbm.h>
#else /* no GDBM */
#  ifdef HAVE_NDBM_H
#    include <ndbm.h>
#  else /* no NDBM */
#    ifdef HAVE_DBM_H
#      include <dbm.h>
#    endif /* HAVE_DBM_H */
#  endif /* HAVE_NDBM_H */
#endif /* HAVE_GDBM_H */

#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef HAVE_BCOPY
#  define bcopy(src,dest,len) (void) (memcpy(dest,src,len))
#  define bzero(dest,len)     (void) (memset(dest, (char)NULL, len))
#  define bcmp(b1,b2,n)       memcmp(b1,b2,n)
#endif /* HAVE_BCOPY */

#ifndef HAVE_INDEX
#  define index(s,c)         strchr(s,c)
#  define rindex(s,c)        strrchr(s,c)
#endif /* HAVE_INDEX */

/*
 * Some old OSes don't have srandom, but do have srand
 */
#ifndef   HAVE_SRANDOM
#  include <stdlib.h>
#  define srandom srand
#  define  random  rand
#else
   extern void srandom();
#endif /* HAVE_SRANDOM */

#include <flock.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#if HAVE_SYS_FILE_H
#  include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include "string_util.h"

#include <time.h>
#define TIME_T time_t

#ifdef SCRAM
extern int encode64();
extern int decode64();
#endif /* SCRAM */

#ifndef HAVE_STRERROR
char *strerror();
#endif /* HAVE_STRERROR */

#include "popper.h"
#include "logit.h"

#define UID_T   uid_t


static struct mods {
       char   *name;
} modes[] = {
#define SCRAM_AUTH  0
    { "scramauth" },
#define APOP_AUTH   1
    { "apopauth"  },
#define POP_AUTH    2 
    { "popauth"   },
#define OTHER       3
    { NULL        }
};


typedef enum {
    INITSW  = 1,
    SAFESW,
    LISTSW,
    USERSW,
    DELESW,
    ONLYSW,
    HELPSW,
    TRACESW,
    DEBUGSW,
    VERSNSW,
    DUMPSW,

    BAD_SW
} switchType;


static struct swit {
    char      *name;
    int        param;
    switchType val;
} switches[] = {
    { "init"    , 0, INITSW  },
    { "safe"    , 0, SAFESW  },
    { "list"    , 0, LISTSW  },
    { "user"    , 1, USERSW  },
    { "delete"  , 1, DELESW  },
    { "only"    , 0, ONLYSW  },
    { "help"    , 0, HELPSW  },
    { "-help"   , 0, HELPSW  },
    { "trace"   , 1, TRACESW },
    { "debug"   , 0, DEBUGSW },
    { "version" , 0, VERSNSW },
#ifdef _DEBUG
    { "dump"    , 0, DUMPSW  },
#endif /* _DEBUG */

    { NULL        },
};


#define BLATHER0(b) do {                                                   \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b);                       \
    } while ( 0 );
#define BLATHER1(b,a1) do {                                                \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1);                   \
    } while ( 0 );
#define BLATHER2(b,a1,a2) do {                                             \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2);               \
    } while ( 0 );
#define BLATHER3(b,a1,a2,a3) do {                                          \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3);           \
    }while(0);
#define BLATHER4(b,a1,a2,a3,a4) do {                                       \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4);       \
    } while ( 0 );
#define BLATHER5(b,a1,a2,a3,a4,a5) do {                                    \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5);   \
    } while ( 0 );
#define BLATHER6(b,a1,a2,a3,a4,a5,a6) do {                                 \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6);                                                 \
    } while ( 0 );
#define BLATHER7(b,a1,a2,a3,a4,a5,a6,a7) do {                              \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7);                                             \
    } while ( 0 );
#define BLATHER8(b,a1,a2,a3,a4,a5,a6,a7,a8) do {                           \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7, a8);                                         \
    } while ( 0 );
#define BLATHER9(b,a1,a2,a3,a4,a5,a6,a7,a8,a9) do {                        \
        if ( debug )                                                       \
            logit ( trace_file, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7, a8, a9);                                     \
    } while ( 0 );


/*
 * Globals
 */
static char   *program      = NULL;
static int     debug        =    0;
static FILE  *trace_file    = NULL;


/*
 * Prototypes
 */
static void         byebye  ( int val );
static void         adios   ( WHENCE, const char *fmt, ... );
static void         helpful ( void );
static int          check_db_err ( void *db, const char *op, BOOL bExp );
static const char  *printable ( const char *p, int len );


static void
byebye ( int val )
{
    if ( trace_file != NULL )
        fclose ( trace_file );

    exit ( val );
}


static void
adios ( WHENCE, const char *fmt, ... )
{
    va_list ap;

    fprintf  ( stderr, "%s: ", program );
    va_start ( ap, fmt );
    vfprintf ( stderr, fmt, ap );
    fprintf  ( stderr, "\n" );
    va_end   ( ap );

    if ( debug ) {
        va_start ( ap, fmt );
        vlogit   ( trace_file, POP_DEBUG, fn, ln, fmt, ap );
        va_end   ( ap );
    }

    byebye ( 1 );
}


static void
helpful ( void )
{
    fprintf  ( stderr, "usage: %s [ -debug | -trace <tracefile> ] [ <action> ]\n"
                       "<action>: -init [ -safe ] \n"
                       "          -list [ <user> | ALL ] | -only\n"
                       "          -user <user> [ <password> ]\n"
                       "          -delete <user>\n"
                       "          -help\n"
                       "          -version\n",
               program );

    byebye ( 1 );
}


/*
 * Check for db errors.
 *
 * Parameters:
 *     db:          result of *dbm_open.
 *     op:          text string of operation (e.g., "fetch").
 *     bExp:        if an error (such as "item not found" is
 *                      expected and not anything to get excited
 *                      about.
 */
static int
check_db_err ( void *db, const char *op, BOOL bExp )
{
    int   db_err = 0;
    char *db_str = NULL;
    int   sv_err = errno;
    int   vNotFound = -1;
    int   vEmptyDb  = -1;


#ifdef GDBM
    (void) db;
    db_err    = (int) gdbm_errno;
    db_str    = (char *) gdbm_strerror ( gdbm_errno );
    vNotFound = GDBM_ITEM_NOT_FOUND;
    vEmptyDb  = GDBM_EMPTY_DATABASE;
#else
    db_err = dbm_error  ( (DBM *) db );
    db_str = strerror   ( db_err );
#endif /* GDBM */

    if ( db_err != 0 ) {
        if ( bExp && ( db_err == vNotFound || db_err == vEmptyDb ) ) {
            BLATHER5 ( "DB error on %s: %s %d (%d: %s)",
                       op, db_str, db_err, sv_err, strerror(sv_err) );
        } else {
            logit ( trace_file, POP_NOTICE, HERE,
                    "*** DB error on %s: %s %d (%d: %s) ***",
                    op, db_str, db_err, sv_err, strerror(sv_err) );

            fprintf ( stderr, "*** DB error on %s: %s %d (%d: %s) ***\n",
                      op, db_str, db_err, sv_err, strerror(sv_err) );
        }
    } else {
        BLATHER5 ( "DB %s succeeded: %s %d (%d: %s)",
                   op, db_str, db_err, sv_err, strerror(sv_err) );
    }

    return db_err;
}


static char printable_buffer [ 1024 ];

static const char *
printable ( const char *p, int len )
{
    int     left = sizeof ( printable_buffer ) -1;
    char   *q    = printable_buffer;
    char    c    = 0;


    if ( p == NULL ) {
        BLATHER0 ( "printable passed NULL pointer" );
        return NULL;
    }

    /*BLATHER2 ( "printable passed %d (strlen %d)", len, strlen(p) );*/

    while ( left > 0 && len > 0 ) {
        c = *p++;
        len--;
        if ( c >= ' ' && c <= '~' ) {
            *q++  = c;
            left -= 1;
        } else {
            long ic = c & 0x000000ff;
            switch ( c ) {
                case '\0':
                    *q++  = '\\';
                    *q++  = '0';
                    left -= 2;
                    break;
                case '\n':
                    *q++  = '\\';
                    *q++  = 'n';
                    left -= 2;
                    break;
                case '\t':
                    *q++  = '\\';
                    *q++  = 't';
                    left -= 2;
                    break;
                case '\r':
                    *q++  = '\\';
                    *q++  = 'r';
                    left -= 2;
                    break;
                case '\a':
                    *q++  = '\\';
                    *q++  = 'a';
                    left -= 2;
                    break;
                case '\b':
                    *q++  = '\\';
                    *q++  = 'b';
                    left -= 2;
                    break;
                case '\v':
                    *q++  = '\\';
                    *q++  = 'v';
                    left -= 2;
                    break;
                default:
                    sprintf ( q, "\\%#lx", ic );
                    q    += 5;
                    left -= 5;
            } /* switch */
        } /* unprintable */
    } /* while */

    *q = '\0';
    /*BLATHER2 ( "printable returning %d: '%s'", strlen(printable_buffer), printable_buffer );*/
    return printable_buffer;
}


#ifndef HAVE_STRDUP
#include <stddef.h>

char *
strdup(str)
        char *str;
{
    int len;
    char *copy;

    len = strlen(str) + 1;
    if ( ! ( copy = malloc ( (u_int) len ) ) )
        return ( (char *) NULL );
    bcopy ( str, copy, len );
    return ( copy );
}
#endif /* not HAVE_STRDUP */


/*
 * Obscure password so a cleartext search doesn't come up with
 * something interesting.
 *
 */
char *
obscure ( string )
char *string;
{
    unsigned char *cp, *newstr;

    cp = newstr = (unsigned char *) strdup ( string );

    while ( *cp ) {
        *cp++ ^= 0xff;
    }

    return ( (char *) newstr );
}


/* Use GNU_PASS for longer passwords on systems that support termios */

#ifndef GNU_PASS
char *getpass();
#else /* not GNU_PASS */

/* Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.

 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
/* It is desireable to use this bit on systems that have it.
    The only bit of terminal state we want to twiddle is echoing, which is
   done in software; there is no need to change the state of the terminal
   hardware.  */

#  include <stdio.h>
#  include <termios.h>

#  ifdef HAVE_SYS_UNISTD_H
#    include <sys/unistd.h>
#  endif /* HAVE_SYS_UNISTD_H */

#  ifdef HAVE_UNISTD_H
#    include <unistd.h>
#  endif /* include <unistd.h> */

#  ifndef TCSASOFT
#    define TCSASOFT 0
#  endif

#  ifdef SSIZET
typedef SSIZET ssize_t;
#  endif


char *
getpass (prompt)
#  if defined(HPUX)
char *prompt;
#  else
const char *prompt;
#  endif /* defined(HPUX) */
{
  FILE *in, *out;
  struct termios t;
  int echo_off;
  static char *buf = NULL;
  static size_t bufsize = 0;
  ssize_t nread;

  /* Try to write to and read from the terminal if we can.
     If we can't open the terminal, use stderr and stdin.  */

  in = fopen ("/dev/tty", "w+");
  if (in == NULL)
    {
      in = stdin;
      out = stderr;
    }
  else
    out = in;

  /* Turn echoing off if it is on now.  */

  if (tcgetattr (fileno (in), &t) == 0)
    {
      if (t.c_lflag & ECHO)
    {
      t.c_lflag &= ~ECHO;
      echo_off = tcsetattr (fileno (in), TCSAFLUSH|TCSASOFT, &t) == 0;
      t.c_lflag |= ECHO;
    }
      else
    echo_off = 0;
    }
  else
    echo_off = 0;

  /* Write the prompt.  */
  fputs (prompt, out);
  fflush (out);

  /* Read the password.  */
#  ifdef NO_GETLINE
  bufsize = 256;
  buf = (char *)malloc(256);
  nread = (fgets(buf, (size_t)bufsize, in) == NULL) ? 1 : strlen(buf);
  rewind(in);
  fputc('\n', out);
#  else
  nread = __getline (&buf, &bufsize, in);
#  endif /* NO_GETLINE */

  if ( nread < 0 && buf != NULL )
    buf[0] = '\0';
  else if ( buf[nread - 1] == '\n' )
    /* Remove the newline.  */
    buf[nread - 1] = '\0';

  /* Restore echoing.  */
  if (echo_off)
    (void) tcsetattr (fileno (in), TCSAFLUSH|TCSASOFT, &t);

  if (in != stdin)
    /* We opened the terminal; now close it.  */
    fclose (in);

  return buf;
}
#endif /* not GNU_PASS */


/* ARGSUSED */

int
main ( argc, argv )
int argc;
char   *argv[];
{
    UID_T          myuid                    = -1;
    int            flags                    = 0,
                   i                        = 0,
                   apoplen                  = 0,
                   scramlen                 = 0;
    BOOL           delesw                   = FALSE,
                   initsw                   = FALSE,
                   safesw                   = FALSE,
                   onlysw                   = FALSE,
                   passtype                 = FALSE,
                   insist                   = FALSE,
                   listsw                   = FALSE,
                   usersw                   = FALSE,
                   dumpsw                   = FALSE,
                   popuser                  = FALSE,    /* TRUE: we are the db owner */
                   user_found               = FALSE;    /* TRUE: user to be changed exists */
    switchType     v                        = BAD_SW;
    char          *cp                       = NULL,
                  *userid                   = NULL,
                  *givenpassword            = NULL,
                  *db_name                  = NULL,
                   apopfld       [ BUFSIZ ] = "",
                   scramfld      [ BUFSIZ ] = "",
                   outfld        [ BUFSIZ ] = "",
                   buf           [ 100 ]    = "",
                   obuf          [ 100 ]    = "";
    struct passwd *pw                       = NULL,
                   pop_pw,
                   my_pw;
    datum          key,
                   value;
    
#ifdef SCRAM
    SCRAM_MD5_VRFY  scram_verifiers;
#endif /* SCRAM */

#ifdef GDBM
    GDBM_FILE      db;
    char           auth_file[BUFSIZ];
#else /* not GDBM */
    DBM           *db;
    char           auth_dir[BUFSIZ];
#  ifndef BSD44_DBM
    char           auth_pag[BUFSIZ];
#  endif /* BSD44_DBM */
#endif /* GDBM */

    int            f,
                   mode;


    memset  ( &pop_pw, 0, sizeof(pop_pw) );
    memset  ( &my_pw,  0, sizeof(my_pw)  );
    srandom ( (unsigned int) time ( (TIME_T *) 0) );   /* seed random with the
                                                          current time */

    cp = program = argv[0];
    while ( *program )
        if ( *(program++) == '/' )
            cp = program;
    program = argv[0];
    
    /*  
     * Open the log file 
     */
#ifdef SYSLOG42
    openlog ( program, 0 );
#else
    openlog ( program, POP_LOGOPTS, POP_FACILITY );
#endif

    /*
     * See if we're in APOP, SCRAM, or default mode
     */
    mode = 0;
    while ( modes[mode].name &&
              strcmp(cp, modes[mode].name ) )
        mode++;

    if ( mode >= OTHER ) 
        adios ( HERE, "Unknown mode: %s", cp );
    else if ( mode == POP_AUTH || mode == APOP_AUTH ) {
        passtype = 0;
        onlysw   = TRUE;   /* popauth always change ONLY the apop 
                              password */
    }
    else if ( mode == SCRAM_AUTH ) {
        passtype = 1; 
        onlysw   = FALSE;    /* scramauth by default also clears the apop 
                                password */
    }

    BLATHER3 ( "mode=%s; passtype=%d; onlysw=%s",
               modes[mode].name, passtype, onlysw ? "true" : "false" );

    /*
     * Process arguments
     */
    argv++;
    argc--;

    while ( argc > 0 ) {
        cp = argv[0];

        if ( *cp != '-' ) {
            fprintf ( stderr, "%s: unrecognized switch \"%s\"", program, cp );
            helpful();
        }

        cp++;
        i = 0;
        v = BAD_SW;
        for ( i = 0; switches[i].name; i++ ) {
            if ( strcmp ( cp, switches[i].name ) == 0 ) {
                v = switches[i].val;
                break;
            }
        }

        if ( switches[i].param > 0 ) {
            if ( argc < 2 || argv[1][0] == '-' )
                adios ( HERE, "missing argument for %s", argv[0] );
        }

        BLATHER1 ( "...processing switch '-%s'", cp );

        switch ( v ) {
            default:
                fprintf ( stderr, "%s: \"-%s\" unknown option\n", program, cp );
                helpful();
            case TRACESW:
                debug++;
                trace_file = fopen ( argv[1], "a+" );
                if ( trace_file == NULL )
                    adios ( HERE, 
                            "Unable to open trace file \"%s\": %s (%d)\n",
                            argv[1], STRERROR(errno), errno );
                BLATHER1 ( "Trace and Debug destination is file \"%s\"",
                           argv[1] );
                argc--;
                argv++;
                break;
            case DEBUGSW:
                debug = TRUE;
                break;
            case VERSNSW:
                printf ( "%s v%s\n", program, VERSION );
                byebye ( 0 );
                break;
            case INITSW:
                initsw = TRUE;
                listsw = FALSE;
                delesw = FALSE;
                break;
            case SAFESW:
                safesw = TRUE;
                break;
            case DUMPSW:
                dumpsw = TRUE;
            case LISTSW:
                listsw = TRUE;
                initsw = FALSE;
                delesw = FALSE;
                if ( argc >= 2 && argv[1][0] != '-' ) {
                    userid = argv[1];
                    argc--;
                    argv++;
                }
                break;
            case ONLYSW:
                onlysw = TRUE;
                break;
            case DELESW:
                delesw = TRUE;
                initsw = FALSE;
                listsw = FALSE;
                userid = argv[1];
                argc--;
                argv++;
                break;
            case USERSW:
                usersw = TRUE;
                userid = argv[1];
                if ( argc >= 3 && argv[2][0] != '-' ) {
                    givenpassword = argv[2];
                    argc--;
                    argv++;
                }
                argc--;
                argv++;
                break;
            case HELPSW:
                helpful();
                break;
        } /* switch (v) */

        argc--;
        argv++;
    } /* while argc > 0 */


/* Expand the rest iff SCRAM or APOP and favor SCRAM to hold auth db name */

#ifdef   SCRAM
#  define  AUTHON
#  define  AUTHDB SCRAM
#else
#  ifdef   APOP
#    define  AUTHON
#    define  AUTHDB APOP
#  endif /* APOP */
#endif /* SCRAM */

#ifndef  AUTHON
   adios ( HERE, "not compiled with either SCRAM or APOP options" );
#else
#  ifndef SCRAM
   if ( mode == SCRAM_AUTH )
     adios ( HERE, "not compiled with SCRAM option" );
#  endif
#  ifndef APOP
   if ( (mode == APOP_AUTH) || (mode == POP_AUTH) )
        adios ( HERE, "not compiled with APOP option" );
#  endif

    myuid = getuid();
    pw = getpwuid ( myuid );
    if ( pw == NULL)
        adios ( HERE, "Sorry, don't know who you (uid %d) are.", myuid );
    my_pw = *pw;
    my_pw.pw_name = strdup ( pw->pw_name );

    pw = getpwnam ( POPUID );
    if ( pw == NULL)
        adios ( HERE, "\"%s\": userid unknown", POPUID );
    pop_pw = *pw;

    if ( pop_pw.pw_uid == myuid )
        popuser = TRUE;

    if ( myuid != 0 && popuser == FALSE
         && ( delesw || ( userid != NULL && strcmp(userid, my_pw.pw_name) != 0 )
                     || ( usersw && userid != NULL ) )
       )
        adios ( HERE,
                "Only superuser or user \"%s\" can perform the requested function",
                POPUID );

    if ( myuid != 0 && ( initsw || dumpsw ) )
        adios ( HERE, "Only superuser can perform the requested function" );

    if ( delesw )
        fprintf ( stderr, "Warning: deleting user \"%s\"\n",
                  userid );

#ifdef GDBM
    strlcpy  ( auth_file, AUTHDB, sizeof(auth_file) - 1 );
    BLATHER1 ( "GDBM: auth_file=%s", auth_file );
    db_name = auth_file;
#else /* not GDBM */
    strlcpy  ( auth_dir, AUTHDB, sizeof(auth_dir) - 5 );
#  ifdef BSD44_DBM
    strcat   ( auth_dir, ".db" );
    BLATHER1 ( "BSD44_DBM: auth_dir=%s", auth_dir );
    db_name = auth_dir;
#  else /* non-BSD44 DBM */
    strlcpy  ( auth_pag, AUTHDB, sizeof(auth_pag) - 5 );
    strcat   ( auth_pag, ".pag" );
    strcat   ( auth_dir, ".dir" );
    BLATHER2 ( "xDBM: auth_pag='%s'; auth_dir='%s'", auth_pag, auth_dir );
    db_name = malloc ( sizeof ( auth_pag ) + 15 );
    if ( db_name != NULL ) {
        strcpy ( db_name, auth_pag );
        strcat ( db_name, " (and .dir )" );
    } else
        db_name = auth_pag;
#  endif /* BSD44_DBM */
#endif /* GDBM */

    if ( delesw ) {
        if ( myuid && !popuser )
            adios ( HERE, "Only root or %s may delete entries", POPUID );

#ifdef GDBM
        db = gdbm_open ( auth_file, 512, GDBM_WRITER, 0, 0 );
#else
        db = dbm_open  ( AUTHDB, O_RDWR, 0 );
#endif

        if ( db == NULL )
            adios ( HERE, 
                    "%s: unable to open POP authentication DB: %s (%i) [%i]", 
                    AUTHDB, strerror(errno), errno, __LINE__ );

        key.dptr  = userid;
        key.dsize = strlen ( key.dptr ) + 1;
        BLATHER2 ( "...fetching: (%d) '%s'",
                   key.dsize, printable ( key.dptr, key.dsize ) );

#ifdef GDBM
        value = gdbm_fetch ( db, key );
#else
        value = dbm_fetch  ( db, key );
#endif

        check_db_err ( db, "fetch", TRUE );

        if ( value.dptr == NULL )
            adios ( HERE, "User '%s' not found in authentication database", userid );

#ifdef GDBM
        if ( gdbm_delete ( db, key ) < 0 )
#else
        if ( dbm_delete  ( db, key ) < 0 )
#endif
        adios ( HERE, "Unable to delete user '%s' from authentication database",
                userid );

#ifdef GDBM
        gdbm_close ( db );
#else
        dbm_close  ( db );
#endif

    logit  ( trace_file, POP_NOTICE, HERE,
             "popauth: user \"%s\" deleted by \"%s\" from db \"%s\"",
             userid, my_pw.pw_name, AUTHDB );
    byebye ( 0 );
    } /* delesw */

    if ( initsw ) {
        struct stat st;

        setuid ( myuid );

#ifdef GDBM
        if ( stat ( auth_file, &st ) != -1 ) 
#else
        if ( stat ( auth_dir,  &st ) != -1 ) 
#endif
        {
            char ibuf [ 30 ];

            if ( safesw == TRUE ) {
                fprintf  ( stderr, "POP authentication DB not "
                                   "initialized (exists): %s\n",
                           db_name );
                byebye ( 0 );
            }
            printf ( "Really initialize POP authentication DB? " );
            if ( fgets ( ibuf, sizeof(ibuf), stdin ) == NULL || ibuf[0] != 'y' )
                byebye ( 1 );
#ifdef GDBM
            (void) unlink ( auth_file );
#else
            (void) unlink ( auth_dir );
#  ifndef BSD44_DBM
            (void) unlink ( auth_pag );
#  endif
#endif
            logit  ( trace_file, POP_NOTICE, HERE,
                     "popauth: user \"%s\" removed existing db \"%s\"",
                     my_pw.pw_name, AUTHDB );
        }

#ifdef GDBM
        db = gdbm_open  ( auth_file, 512, GDBM_WRCREAT, 0600, 0 );
        if ( db == NULL )
            adios ( HERE, 
                    "unable to create POP authentication DB %s: %s (%d) [%i] ", 
                    auth_file, strerror(errno), errno, __LINE__ );
        gdbm_close ( db );
        if ( chown ( auth_file, pop_pw.pw_uid, pop_pw.pw_gid ) == -1)
            adios ( HERE, 
                    "error setting ownership of POP authentication for %s: %s (%d) [%d]", 
                    auth_file, strerror(errno), errno, __LINE__ );
#else
    db = dbm_open ( AUTHDB, O_RDWR | O_CREAT, 0600 );
    if ( db == NULL )
        adios ( HERE, "unable to create POP authentication DB: %s (%d) [%d]", 
                strerror(errno), errno, __LINE__ );
    dbm_close ( db );
    if ( chown ( auth_dir, pop_pw.pw_uid, pop_pw.pw_gid ) == -1 
#  ifndef BSD44_DBM
         || chown (auth_pag, pop_pw.pw_uid, pop_pw.pw_gid) == -1
#  endif
       )
        adios ( HERE, "error setting ownership of POP authentication DB: %s (%d) [%d]", 
                strerror(errno), errno, __LINE__ );
#endif /* GDBM */

    logit  ( trace_file, POP_NOTICE, HERE,
             "popauth: user \"%s\" initialized db \"%s\"",
             my_pw.pw_name, AUTHDB );
    byebye ( 0 );
    } /* initsw */

#ifdef GDBM
    db = gdbm_open ( auth_file, 512, GDBM_READER, 0, 0 );
    if ( db == NULL)
        adios ( HERE, "unable to open POP authentication DB %s:\n\t%s (%i) [%i]", 
                auth_file, strerror(errno), errno, __LINE__ );
#else
    db = dbm_open ( AUTHDB, O_RDONLY, 0 );
    if ( db == NULL )
        adios ( HERE, "unable to open POP authentication DB %s:\n\t%s (%i) [%i]", 
                AUTHDB, strerror(errno), errno, __LINE__ );
#endif

#ifdef GDBM
    f = open ( auth_file, listsw ? O_RDONLY : O_RDWR );
    if ( f == -1 )
        adios ( HERE, "%s: unable to open POP authentication DB:\n\t%s (%i) [%i]", 
                auth_file, strerror(errno), errno, __LINE__ );
#else
    f = open ( auth_dir, listsw ? O_RDONLY : O_RDWR );
    if ( f == -1 )
        adios ( HERE, "%s: unable to open POP authentication DB:\n\t%s (%d) [%d]",
                auth_dir, strerror(errno), errno, __LINE__ );
    if ( flock ( f, LOCK_SH ) == -1 )
        adios ( HERE, "%s: unable to lock POP authentication DB:\n\t%s (%d) [%d]",
                auth_dir, strerror(errno), errno, __LINE__ );
#endif

    if ( listsw ) {
        if ( userid == NULL )
            userid = my_pw.pw_name;

        if ( strcmp ( userid, "ALL" ) != 0 ) { /* list one user */
            key.dptr  = userid;
            key.dsize = strlen ( key.dptr ) + 1;
            BLATHER2 ( "...fetching: (%d) '%s'",
                       key.dsize, printable ( key.dptr, key.dsize ) );

#ifdef GDBM
            value = gdbm_fetch ( db, key );
#else
            value = dbm_fetch  ( db, key );
#endif

            check_db_err ( db, "fetch", TRUE );

            if ( value.dptr == NULL )
                adios ( HERE, "no entry for \"%s\" in POP authentication DB", userid );

            strcpy ( apopfld, value.dptr ); 
            if ( (size_t) value.dsize > strlen(apopfld) + 1 )
                strcpy ( scramfld, &((char *) value.dptr) [strlen(apopfld) + 1] );
            else
                scramfld[0] = '\0';
            apoplen  = strlen (  apopfld ) + 1;
            scramlen = strlen ( scramfld ) + 1;

            if ( dumpsw ) {
                printf ( "%.16s:%*s %.62s\n",
                         key.dptr,
                         16 - (int)strlen(key.dptr), " ",
                         printable ( value.dptr, value.dsize ) );
                BLATHER5 ( "Dumping \"%s\": (%d)'%s'; APOP %d; SCRAM %d",
                           key.dptr,
                           value.dsize, printable ( value.dptr, value.dsize ),
                           apoplen, scramlen );
            } else {
                printf ( "%-16s: %s %s\n", key.dptr,
                         ( apoplen  > 1 ) ? "APOP"  : "    ",
                         ( scramlen > 1 ) ? "SCRAM" : ""    );
                BLATHER3 ( "Listing user \"%s\": APOP %s; SCRAM %s",
                           key.dptr,
                           ( apoplen  > 1 ) ? "yes" : "no",
                           ( scramlen > 1 ) ? "yes" : "no" );
            }

        } /* list one user */
    else { /* list all users */
        BLATHER0 ( "Listing all users... " );
#ifdef GDBM
        for ( key = gdbm_firstkey ( db ); key.dptr; key = gdbm_nextkey ( db, key ) ) 
#else
        for ( key = dbm_firstkey  ( db ); key.dptr; key = dbm_nextkey  ( db      ) ) 
#endif
        {
            BLATHER2 ( "...fetching: (%d)'%s'",
                       key.dsize, printable ( key.dptr, key.dsize ) );

#ifdef GDBM
            value = gdbm_fetch ( db, key );
#else
            value = dbm_fetch  ( db, key );
#endif

            check_db_err ( db, "fetch", FALSE );

            if ( value.dptr == NULL ) {
                printf   ( "%.16s:%*s *** no information?!?\n",
                           key.dptr, 16 - (int)strlen(key.dptr), " " );
                BLATHER1 ( "*** No information for user \"%s\"", buf );
            }
            else {
                strcpy ( apopfld, value.dptr ); 
                if ( (size_t) value.dsize > strlen(apopfld) + 1 )
                    strcpy ( scramfld, &((char *) value.dptr) [strlen(apopfld) + 1] );
                else
                    scramfld[0] = '\0';
                apoplen  = strlen (  apopfld ) + 1;
                scramlen = strlen ( scramfld ) + 1;

                if ( dumpsw ) {
                    printf ( "%.16s:%*s %.62s\n",
                             key.dptr,
                             16 - (int)strlen(key.dptr), " ",
                             printable ( value.dptr, value.dsize ) );
                    BLATHER5 ( "Dumping \"%s\": (%d)'%s'; APOP %d; SCRAM %d",
                               key.dptr,
                               value.dsize, printable ( value.dptr, value.dsize ),
                               apoplen, scramlen );
                } else {
                    printf ( "%-16s: %s %s\n", key.dptr,
                             ( apoplen  > 1 ) ? "APOP"  : "    ",
                             ( scramlen > 1 ) ? "SCRAM" : ""    );
                    BLATHER3 ( "Listing user \"%s\": APOP %s; SCRAM %s",
                               key.dptr,
                               ( apoplen  > 1 ) ? "yes" : "no",
                               ( scramlen > 1 ) ? "yes" : "no" );
                }

            }
        } /* for loop */

        /*
         * Check if we terminated normally or on an error
         */
        check_db_err ( db, "first/next key", TRUE );
        
    } /* list all users */

#ifdef GDBM
    gdbm_close ( db );
#else
    dbm_close  ( db );
#endif

    byebye ( 0 );
    } /* listsw */

    /*
     * If we're here, we must be doing a password change (usersw)
     */
    if ( userid == NULL ) {
        userid = my_pw.pw_name;
    } else {
        pw = getpwnam ( userid );
        if ( pw == NULL )
            adios ( HERE, "Sorry, don't know who user %s is.", userid );
        userid = pw->pw_name;
    }

    key.dptr  = userid;
    key.dsize = strlen ( key.dptr ) + 1;
    BLATHER2 ( "...fetching: (%d) '%s'",
               key.dsize, printable ( key.dptr, key.dsize ) );

#ifdef GDBM
    value = gdbm_fetch ( db, key );
#else
    value = dbm_fetch  ( db, key );
#endif

    check_db_err ( db, "fetch", TRUE );
    user_found = ( value.dptr != NULL );

    if ( user_found ) {
        strcpy ( apopfld, value.dptr ); 
        if ( (size_t) value.dsize > strlen(apopfld) + 1 )
            strcpy ( scramfld, &((char *) value.dptr) [strlen(apopfld) + 1] );
        else
            scramfld[0] = '\0';

        BLATHER3 ( "user %s exists in db (%d/%d)",
                   userid, strlen(apopfld), strlen(scramfld) );
    }
    else {
        apopfld[0] = scramfld[0] = '\0';
        BLATHER1 ( "user %s doesn't exist -- adding", userid );
    }

    fprintf ( stderr, "%s %s%s %s for %s.\n",
              user_found           ? "Changing"     : "Adding",
              onlysw               ? "only "        : "",  
              (mode == SCRAM_AUTH) ? "SCRAM"        : "APOP",
              passtype             ? "pass phrase"  : "password",
              userid );

    apoplen  = strlen (  apopfld ) + 1;
    scramlen = strlen ( scramfld ) + 1;

    /*
     * If user isn't root or the db owner, and user has an existing
     * password for the mode (apop or scarm) being changed, verify old
     * password.
     */
    if ( ( myuid && !popuser ) 
          && (((mode==APOP_AUTH || mode==POP_AUTH) && apopfld[0] )
                ||(mode==SCRAM_AUTH && scramfld[0] )) ) {

        if ( !(i=strlen(strncpy(obuf,
              getpass(passtype?"Old pass phrase:":"Old password:"),sizeof(obuf)))) )
            adios ( HERE, "Sorry, password entered incorrectly\n" );

        if ( mode == APOP_AUTH || mode == POP_AUTH ) {
            if ( ((apoplen - 1) != i) ||
                 (strncmp(obuf, apopfld, i) &&
                  strncmp(obuf, obscure(apopfld), i)) ) 
                adios ( HERE, "Sorry, password entered incorrectly\n" );
        }
        else if ( mode == SCRAM_AUTH ) {
#ifdef SCRAM
            SCRAM_MD5_VRFY  test_verifiers;
        
            scramlen = sizeof( scram_verifiers );

            if (    decode64(   scramfld,                  strlen(scramfld),
                                (char *) &scram_verifiers, &scramlen )
                    || scramlen != sizeof( scram_verifiers ) ) 
                adios ( HERE, "unable to decode SCRAM verifier from the authenication DB" );

            scram_md5_vgen ( &test_verifiers,
                              scram_verifiers.salt,
                              obuf,
                              0,
                              1,
                              NULL );

            if ( memcmp( &scram_verifiers, &test_verifiers, sizeof(scram_verifiers)) ) 
#endif
                adios ( HERE, "Sorry, pass phrase entered incorrectly\n");
        }
    } /* old password verification */

#ifdef GDBM
    gdbm_close ( db );
#else
    dbm_close  ( db );
    if ( flock ( f, LOCK_UN ) == -1)
        adios ( HERE, "%s: unable to unlock POP authentication DB\n\t%s (%d) [%d]",
                auth_dir, strerror(errno), errno, __LINE__ );
#endif /* GDBM */

    for ( insist = 0; insist < 2; insist++ )  {
        int     i;
        char    c;

        if ( insist )
            printf ( "Please use %s.\n",
                     flags == 1 ? "at least one non-numeric character"
                                : ( passtype ? "a longer pass phrase"
                                             : "a longer password" ) );

        /*
         * If new password passed in (givenpassword) use it,
         * otherwise prompt for the new one.
         */
        if ( givenpassword != NULL ) {
            strncpy ( buf, givenpassword, sizeof ( buf ) );
            buf [ sizeof(buf) -1 ] = '\0';
            i = strlen ( buf );
        }
        else {
            strncpy ( buf, 
                      getpass ( passtype ? "New pass phrase:"
                                         : "New password:" ), 
                      sizeof ( buf ) );
            buf [ sizeof(buf) -1 ] = '\0';
            i = strlen ( buf );
            if ( i == 0 || strncmp ( buf, obuf, i ) == 0 ) {
                fprintf ( stderr, passtype ? "Pass phrase unchanged.\n"
                                           : "Password unchanged.\n" );
                byebye (1);
            }
        }

        if ( !strcmp(buf,"*") )
            break;

        flags = 0;
        for ( (cp = buf); (c = *cp++); )
            if ( c >= 'a' && c <= 'z' )
                flags |= 2;
            else
            if ( c >= 'A' && c <= 'Z' )
                flags |= 4;
            else
            if ( c >= '0' && c <= '9' )
                flags |= 1;
            else
                flags |= 8;

        if ( givenpassword != NULL
             || ( flags >= 7 && i >= 4 )
             || ( (flags == 2 || flags == 4 ) && i >= 6 )
             || ( (flags == 3 || flags == 5 || flags == 6 ) && i >= 5 )
           )
            break;
    } /* for loop on insist */


    if ( givenpassword == NULL ) {
        if ( strcmp ( buf,
                      getpass ( passtype ? "Retype new pass phrase:"
                                          : "Retype new password:" ) ) ) {
            fprintf ( stderr,
                      passtype ? "Mismatch -- pass phrase unchanged.\n"
                               : "Mismatch -- password unchanged.\n" );
            byebye ( 1 );
        }
    }
  
    if ( !strcmp(buf, "*") )
        buf[0] = '\0';

#ifdef GDBM
    db = gdbm_open ( auth_file, 512, GDBM_WRITER, 0, 0 );
#else
    db = dbm_open  ( AUTHDB, O_RDWR, 0 );
#endif /* GDBM */
    if ( db == NULL )
        adios ( HERE, "%s: unable to open POP authentication DB:\n\t%s (%i) [%i]", 
                AUTHDB, strerror(errno), errno, __LINE__ );

#ifdef FLOCK_EX
#  ifndef GDBM
    if ( flock ( f, LOCK_EX ) == - 1 )
        adios ( HERE, "%s: unable to lock POP authentication DB\n\t%s (%i) [%i]", 
                auth_dir, strerror(errno), errno, __LINE__ ); 
#  endif
#endif /* FLOCK_EX */

    key.dsize = strlen ( key.dptr = userid ) + 1;

    if ( mode == APOP_AUTH || mode == POP_AUTH ) {
        strcpy ( apopfld, obscure(buf) );
    }

    else if ( mode == SCRAM_AUTH ) {
        scramlen = 0;

      if ( buf[0] ) {

#ifdef SCRAM
        UINT4          key[4];
        UINT4          data[4];
        unsigned char  salt[ SCRAM_MD5_SALTSIZE ];
        int            x;
        unsigned char  digest[ HMAC_MD5_SIZE ];
    
        key[ 0 ] = (UINT4) random();         /* seeded with start up time */
        key[ 1 ] = (UINT4) time((time_t *)0);/* include current time */
    
        /* seed random with the current time to nearest second    */
        srandom( (unsigned int) time((TIME_T *)0) );
    
        key[ 3 ] = (UINT4) random();       
        key[ 4 ] = (UINT4) random();       
    
        data[ 0 ] = (UINT4) getpid();      /* pid+uid+most of user name */
        data[ 1 ] = myuid;
        strncpy( (char *)&data[2], userid, 2*sizeof(UINT4) );
        /* personal info as data  */
        /* random()& time() as key*/
        hmac_md5( (unsigned char *)data, sizeof( data ), 
                  (unsigned char *)key, sizeof( key ),   
                   digest );
    
        /* zero out the salt           */
        /* and then fold in the digest */
    
        memset(salt, '\0', SCRAM_MD5_SALTSIZE );
        x = HMAC_MD5_SIZE;
        while ( x-- )
          salt[ x % SCRAM_MD5_SALTSIZE ] ^= digest[ x ];
  
        scram_md5_vgen( &scram_verifiers,
                         salt,
                         buf,
                         0,
                         1,
                         NULL );
  
        scramlen = sizeof( scramfld );
  
        if ( encode64( (char *) &scram_verifiers, sizeof(scram_verifiers),
                          scramfld, &scramlen ) || scramlen >= (int) sizeof(scramfld) )
#endif /* SCRAM */
           adios ( HERE, "unable to encode SCRAM verifier for the authenication DB" );
      } /* buf[0] */

      scramfld [ scramlen ] = '\0';

      if ( !onlysw )
        apopfld[0] = '\0';
    } /* mode == SCRAM_AUTH */

    value.dsize  = strlen ( value.dptr = strcpy(outfld, apopfld) ) + 1;
    value.dsize += strlen ( strcpy(&outfld[value.dsize], scramfld) ) + 1;

#  ifdef GDBM
    i = gdbm_store ( db, key, value, GDBM_REPLACE );
    check_db_err ( db, "store", FALSE );
    gdbm_close ( db );
#  else /* not GDBM */
    i = dbm_store  ( db, key, value, DBM_REPLACE );
    check_db_err ( db, "store", FALSE );
    dbm_close ( db );
#  endif /* GDBM */

    check_db_err ( db, "close", FALSE );

    if ( i )
        adios ( HERE, "POP authentication DB %s may be corrupt?!?", db_name );

#endif /* AUTHON */

    logit  ( trace_file, POP_NOTICE, HERE,
             "popauth: %s %s %s%s %s for %s",
             my_pw.pw_name,
             user_found             ? "changed"     : "added",
             onlysw                 ? "only "       : "",  
             (mode == SCRAM_AUTH)   ? "SCRAM"       : "APOP",
             passtype               ? "pass phrase" : "password",
             userid );

    byebye ( 0 );
    return 0; /* avoid a warning */
}


#ifndef HAVE_STRERROR
char *
strerror(e)
    int e;
{
    extern char *sys_errlist[];
    extern int sys_nerr;

    if(e < sys_nerr)
        return(sys_errlist[e]);
    else
        return("unknown error");
}
#endif /* HAVE_STRERROR */
