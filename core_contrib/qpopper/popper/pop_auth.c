/*
 * SASL authentication mechanisms for qpopper
 *
 * File:      pop_auth.c
 * Copyright: (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 *            The file license.txt specifies the terms for use,
 *            modification, and redistribution.
 */

/*
 * Revisions:
 *
 *     01/29/01  [rcg]
 *             - Now using p->grp_serv_mode and p->grp_no_serv_mode instead
 *               of SERVER_MODE_GROUP_INCLUDE and SERVER_MODE_GROUP_EXCLUDE.
 *             - Now using p->authfile instead of AUTHFILE.
 *
 *     11/14/00  [rcg]
 *             - Now trimming domain name only if requested.
 *
 *     10/02/00  [rcg]
 *             - Now calling check_config_files to check qpopper-options files.
 *
 *     09/09/00  [rcg]
 *             - Now stripping domain name from user name.
 *
 *     06/30/00  [rcg]
 *             - Added support for ~/.qpopper-options.
 *
 *     06/16/00  [rcg]
 *              - Now setting server mode on or off based on group
 *                membership.
 *
 *    05/01/00  [rcg]
 *              - Somehow a call to pop_msg was passing &p instead of p.
 *
 *    03/07/00  [rcg]
 *              - Updated authentication OK message to account for hidden
 *                messages.
 *
 *    01/26/00  [rcg]
 *              - Updated logging calls.
 *
 *    01/07/00  [rcg]
 *              - Updated genpath() call.
 *
 *    11/28/99  [rcg]
 *              - Fixed buffer overrun.
 *
 */


        /* Define to run with internal test data (remove all this later) */
#undef   INTERNAL_TEST

     /* Define to hide whether or not a user id is valid */
#define  NOHINT

#include <sys/types.h>

#include <stdio.h>

#include "config.h"

#if HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#if HAVE_SYS_FILE_H
#  include <sys/file.h>
#endif

#include <string.h>
#include <ctype.h>
#include <flock.h>

#include "popper.h"

     /* Need the following stuff iff SCRAM is being compiled */

#ifdef SCRAM

#include <netinet/in.h>
#include <sys/stat.h>
#include <pwd.h>

#ifdef GDBM
#  include <gdbm.h>
#else
#  include <ndbm.h>
#endif

#define SERVICENAME      "QPOP@"
#define EOL              "\r\n"

extern int pop_scram();
static int pop_client_challenge();
static int pop_client_response();
static int pop_client_ok();

static struct passwd    * pw;

#endif  /* ifdef SCRAM */

extern char    *ERRMSG_PW;

static auth_table mechanisms[] = {
#ifdef SCRAM
    { "scram-md5", pop_scram, },
#endif
    { NULL,        NULL }
};

    /*  Process the Auth Command Initiation */

int pop_auth (p)
POP     *   p;
{
    auth_table *a;

    /* Auth with no args spills the list of supported mechanisms */
    if ( p->parm_count == 0 ) {
        pop_msg ( p, POP_SUCCESS, HERE, "Supported SASL mechanisms:" );
        if ( mechanisms[0].auth_mechanism == NULL )
            pop_write_line ( p, "X-NONE-SO-USE-APOP-OR-STLS" );
        else
        for ( a = mechanisms; a->auth_mechanism; a++ ) {
            pop_write_line(p, a->auth_mechanism);
        }
        pop_write_line  ( p, "." );
        pop_write_flush ( p );
    return ( POP_FAILURE );
    }

    /* Process SCRAM Initiation */
    for (a = mechanisms; a->auth_mechanism; a++) {
        if ( strcasecmp( p->pop_parm[1], a->auth_mechanism) == 0 ) 
            return ( a->function(p) );
    }
        
    return(pop_msg(p, POP_FAILURE, HERE, "Unknown authentication mechanism: %.100s",
                   p->pop_parm[1]) );
}

/* Do the real SCRAM Work */
#ifdef SCRAM
/* Process the next clientinput */
int pop_scram( p )
  POP  * p;
{
    /*
     * SCRAM-MD5 State Transistions using POP p->:
     * AuthType:  scram_md5
     * AuthState: auth (on client challenge)->clchg (on client response)
     *            ->clrsp->clok
     * CurrentState:    auth1  (on AUTH CMD)->auth2 (on AuthState: clok )->trans
     * N.b. Explictily forces CurrentState to "halt" if either AuthState 
     *      unsuccessful.
     */
    static struct {
        auth_state  authstate;
        int        (*function)();
        auth_state  nextauth;
        state       nextstate;
        state       failstate; 
    } authcntl[] =  {
        auth,   pop_client_challenge,  clchg, auth2, halt,
        clchg,  pop_client_response,   clok,  auth2, halt,
        clok,   pop_client_ok,         clok,  trans, halt };

    int     i;
    char    *msg;
    static int     stSize = sizeof(authcntl)/sizeof(authcntl[0]);
    if(p->AuthState = none) {
    p->AuthType    = scram_md5;
    p->AuthState   = auth;
    if ( p->parm_count == 1 ) {
            pop_write_line(p, "+ ");
            pop_write_flush(p);
        Push(&(p->InProcess), (FP)pop_scram);
        return( POP_SUCCESS );
    }
    msg = p->pop_parm[2];
    }
    else {
        int i;               /* SCRAM-MD5 authentication phase */
        /* remove any trailing [CR]LF */
        msg = p->inp_buffer;
        if ( (i=strlen(msg)) && (msg[i-1] == '\n') ) {
            if ( --i && (msg[i-1] ==  '\r') ) i--;
            msg[i] = '\0';
        }
        if ( (i == 1) && (*msg == '*') ) {
            pop_msg(p,POP_FAILURE, HERE,
                    "POP SCRAM-MD5 authentication abort received from client.");
            p->CurrentState = halt;
            return( POP_FAILURE );
        }
    }

    DEBUG_LOG1(p, "SCRAM-MD5 authentication Base64: %s", msg );

    /* Move thru the authentication states     */
    for(i = 0; i < stSize; i++) {
        if ( p->AuthState == authcntl[i].authstate ) {
            if ( (*authcntl[i].function)( p, msg ) ) {
                p->AuthState    = authcntl[i].nextauth;
                p->CurrentState = authcntl[i].nextstate;
                if(p->CurrentState == trans)
                    (void) Pop(&(p->InProcess));
                return( POP_SUCCESS );
            }
            else {
                p->CurrentState = authcntl[i].failstate;
                return( POP_FAILURE );
            }
        }
    }
    p->CurrentState = error;
    return( pop_msg(p,POP_FAILURE, HERE,
                    "Internally inconsistent authentication state: %d.",
                    p->AuthState) );
}



    /* Checkout the client challenge and reply with server challenge */

static int pop_client_challenge( p, msg )
  POP  * p;
  char * msg;
{
  int                i, j, k, clen, outlen;
  char             * chg, outbuf[ BUFSIZ ];
  UINT4              cnt;
  unsigned char    * mac;
  int                f;

#ifdef GDBM
  char        scram_file[BUFSIZ];
#else
  char        scram_dir[BUFSIZ];
#endif
  struct stat st;
  datum       key,
              ddatum;
#ifdef GDBM
  GDBM_FILE   db;
#else
  DBM        *db;
#endif


 /* Remove all INTERNAL_TEST guarded blocks when no longer needed */

#ifdef INTERNAL_TEST
 char tcmess[]="AGNocmlzADx0NG40UGFiOUhCMEFtL1FMWEI3MmVnQGVsZWFub3IuaW5ub3NvZnQuY29tPg=";
 char tuser[]="chris";
 char tsecret[]="secret stuff";
 unsigned char tsalt[]="testsalt";
 char tsrv[]="imap@eleanor.innosoft.com";
 char tsrvnonce[]="jhcNZlRuPbziFp+vLV+NCw";
 int  x;
 char tsmess[]="dGVzdHNhbHQBAAAAaW1hcEBlbGVhbm9yLmlubm9zb2Z0LmNvbQBqaGNOWmxSdVBiemlGcCt2TFYrTkN3";
 unsigned char tbuf[BUFSIZ];
 int  tbuflen;

 scram_md5_vgen( &(p->scram_verifiers),
                       tsalt , tsecret, 0, 1, (unsigned char*)0 ); 
 printf( "\nSCRAM Internal Test Verifier:\n    " );  
 for (x=0; x<sizeof(p->scram_verifiers); x++) printf( x%4?"%02x":" %02x",
                                 *((unsigned char *)(&(p->scram_verifiers))+x) );
 printf( "\n" );
 fflush( stdout );
 tbuflen = sizeof( tbuf );
 decode64( tsmess, strlen( tsmess ), tbuf, &tbuflen );

  msg = tcmess;
#endif


         /* Blow client's Base64 info into raw binary */

  p->cchallen = sizeof( p->cchal )-1;
  if ( decode64( msg, strlen(msg), p->cchal, &(p->cchallen) ) ) {
    p->CurrentState = halt;
    return( pop_msg(p,POP_FAILURE, HERE,
              "Bad SCRAM-MD5 authentication Base64 string - good bye.") );
  }

  (p->cchal)[p->cchallen]='\0';
  chg = (p->cchal); clen =  p->cchallen;
  i = j = 0;

      /* Find Authorization Id as POP3 user */

  while( i < clen ) {
    p->user[ j ] = chg[ i ];
    if ( !chg[ i++ ] || (j++ >= MAXUSERNAMELEN) ) break;
  }

  k = 0;

      /* Find Authentication Id (make POP3 user iff Authid not given) */

  while( i < clen ) {
    p->authid[ k ]      = chg[ i ];
    if ( !j )
       p->user[ k ]     = chg[ i ];
    if ( !chg[ i++ ] || (k++ >= MAXUSERNAMELEN) ) break;
  }

     /* Check everything is within tolerance */

  if( (i >= clen) ||
         (!k || (k >= MAXUSERNAMELEN) || (j >= MAXUSERNAMELEN)) )
    return( pop_msg(p,POP_FAILURE, HERE, "Bad challenge message") );

     /* Rest of the incoming message is the client nonce */

     /* Check for valid user here (different authorization id not supported) */

  if ( strcmp( p->user, p->authid ) )
    return( pop_msg(p,POP_FAILURE, HERE,
         "Authorization id differs from Authentication id") );


     /* Downcase user name if requested */
  if ( p->bDowncase_user )
    downcase_uname(p, p->user);

    /* Strip domain name if requested */
  if ( p->bTrim_domain )
    trim_domain(p, p->user);

     /* Get rid of the unauthorized and the excluded */

  if ( *p->authid == '\0'
       || ( p->authfile    != NULL && checkauthfile    ( p ) != 0 )
       || ( p->nonauthfile != NULL && checknonauthfile ( p ) != 0 )
     )
  {
#ifdef NOHINT
    *p->authid = '\0';
    strcpy( outbuf, "Permission denied." ); 
#else
    return (pop_msg(p,POP_FAILURE, HERE,
            "Permission denied.",p->user));
#endif
  }

    /* Blow away everyone who UNIX knows nothing about */

  pw = p->pw;
  if ( *p->authid &&
        ((pw == NULL) || (pw->pw_passwd == NULL)
          || (*pw->pw_passwd == '\0')) )
  {
#ifdef NOHINT
   *p->authid = '\0';
    strcpy( outbuf, "UNIX password system rejection." );
#else
    return ( pop_auth_fail ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user) );
#endif
  }

       /* block root login like pop_apop(); 
          maybe this all should be more like pop_pass() */

  if( *p->authid && pw->pw_uid == 0)
  {
#ifdef NOHINT
   *p->authid = '\0';
    strcpy ( outbuf, "login for UID 0 denied." ); 
#else
    return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                             "User %s login denied.", 
                             p->user ) );
#endif
  }

  if ( *p->authid ) {
#ifdef GDBM
    if ((db = gdbm_open (SCRAM, 512, GDBM_READER, 0, 0)) == NULL)
#else
    if ((db = dbm_open (SCRAM, O_RDONLY, 0)) == NULL)
#endif
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                "POP authentication DB not available (%s)",
                                p->user, strerror(errno) ) );

#ifdef GDBM
    (void) strncpy(scram_file, SCRAM, sizeof(scram_file) - 1);
        scram_file[sizeof(scram_file)-1] = '\0';
    if (stat (scram_file, &st) != -1
            && (st.st_mode & 0777) != 0600) {
        gdbm_close (db);
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "POP authentication DB has wrong mode (0%o)",
                                 st.st_mode & 0777 ) );
    }
    f = open(scram_file, O_RDONLY);
    if(f == -1) {
        int e = errno;
        gdbm_close (db);
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "unable to lock POP authentication DB (%s)",
                                 strerror(e) ) );
    }
#else
    (void) strncpy(scram_dir, SCRAM, sizeof(scram_dir) - 5);
#if defined(BSD44_DBM)
    (void) strcat(scram_dir, ".db");
#else
    (void) strcat(scram_dir, ".dir");
#endif
    if (stat (scram_dir, &st) != -1
            && (st.st_mode & 0777) != 0600) {
        dbm_close (db);
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "POP authentication DB has wrong mode (0%o)",
                                 st.st_mode & 0777 ) );
    }
    f = open(scram_dir, O_RDONLY);
    if(f == -1) {
        int e = errno;
        dbm_close (db);
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "unable to lock POP authentication DB (%s)",
                                 strerror(e) ) );
    }
#endif
    if (flock (f, LOCK_SH) == -1) {
        int e = errno;
        (void) close(f);
#ifdef GDBM
        gdbm_close (db);
#else
        dbm_close (db);
#endif
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "unable to lock POP authentication DB (%s)",
                                 strerror(e) ) );
    }

    key.dsize = strlen (key.dptr = p->user) + 1;
#ifdef GDBM
    ddatum = gdbm_fetch (db, key);
#else
    ddatum = dbm_fetch (db, key);
#endif
    if (ddatum.dptr == NULL) {
        (void) close(f);
#ifdef GDBM
        gdbm_close (db);
#else
        dbm_close (db);
#endif
#ifdef NOHINT
       *p->authid = '\0';
        strcpy( outbuf, "user unknown in POP authentication database." ); 
#else
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "no SCRAM credentials" ) );
#endif
    }
    else {
      /* check for valid SCRAM credentials */

      i = strlen(ddatum.dptr)+1;
      outlen = sizeof( p->scram_verifiers );

      if ( ((ddatum.dsize <= i) ||
             (ddatum.dsize != (i+strlen(&ddatum.dptr[i])+1))) ||
           (decode64( &ddatum.dptr[i],      ddatum.dsize-i ,
                    &p->scram_verifiers, &outlen))            ||
           (outlen != sizeof( p->scram_verifiers )) )
      {
#ifdef NOHINT
         *p->authid = '\0';
          strcpy( outbuf, "invalid SCRAM credentials." ); 
#else
          return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                   "invalid or missing SCRAM credentials." ) );
#endif
      }
    }

#ifdef GDBM
    gdbm_close(db);
#else
    dbm_close(db);
#endif
    (void) close(f);
  }

    /* bad user id: still here, so make up fake salt */

  if ( *p->authid == '\0' ) {
    UINT4         roughtime;
    unsigned char digest[ SCRAM_MD5_DATASIZE ];
    unsigned char key[ BUFSIZ ];
    int           keylen, x;

    roughtime=(UINT4)time((time_t *)0)/(60*60*24*7)  /* appear to change roughly */
                         % (60*60*24*7);             /* every 7 days             */
    keylen = strlen( p->myhost );
    memcpy( key, p->myhost, keylen );
    memcpy( &(key[ keylen ]), &roughtime, sizeof(UINT4)  );
    keylen += sizeof(UINT4);
      
    hmac_md5( p->user, strlen( p->user ),         /* HMAC with authid as data     */
              key,     keylen,                    /* & host / normalized-week key */
              digest );

    /* zero out the salt           */
    /* and then fold in the digest */

    memset( p->scram_verifiers.salt, '\0', SCRAM_MD5_SALTSIZE );
    x = HMAC_MD5_SIZE;
    while( x-- )
      p->scram_verifiers.salt[ x % SCRAM_MD5_SALTSIZE ] ^= digest[ x ];
  }

     /* Log start up action */

  pop_log ( p, POP_INFO, HERE, "SCRAM-MD5 \"%s\"", p->user );
  if ( !*p->authid )
    pop_log ( p, POP_INFO, HERE,
              "  silent pre-SCRAM failure for \"%s\": %s",
              p->user, outbuf );

     /* Set up MAC controls */

  p->scram_mac.inpktcnt    = p->scram_mac.outpktcnt  =  0;
  p->scram_mac.macinuse    = 0;
  p->scram_mac.maxincipher = MAXLINELEN;

     /* Build up server challenge */


#ifdef INTERNAL_TEST
 scram_md5_vgen( &(p->scram_verifiers),
                       tsalt , tsecret, 0, 1, (unsigned char*)0 ); 
#endif


  memcpy( &(p->schal[0]), p->scram_verifiers.salt, SCRAM_MD5_SALTSIZE );
  i = SCRAM_MD5_SALTSIZE;

  mac = (unsigned char *)&cnt;
  cnt = htonl( p->scram_mac.maxincipher );

#ifdef SCRAM_INTEGRITY
  mac[ 0 ] = '\02';
#else
  mac[ 0 ] = '\01';
#endif


#ifdef INTERNAL_TEST
 memcpy( mac, tbuf+SCRAM_MD5_SALTSIZE, 4 );
#endif


  memcpy( &(p->schal[ i ]), mac, 4 );
  i += 4;
 
  memcpy( &(p->schal[ i ]), SERVICENAME, strlen(SERVICENAME) );
  i += strlen( SERVICENAME );

  memcpy( &(p->schal[ i ]), p->myhost, strlen(p->myhost)+1 );
  i += strlen( p->myhost )+1;


#ifdef INTERNAL_TEST
 memcpy( &(p->schal[ i-strlen(SERVICENAME)-strlen(p->myhost)-1 ]), tsrv, strlen(tsrv) );
 i = i - strlen(SERVICENAME) - strlen(p->myhost) -1 + strlen(tsrv) +1;

 memcpy( &(p->schal[ i ]), tsrvnonce, sizeof(tsrvnonce) );
 p->schallen = i + strlen( tsrvnonce );
 if ( 0 ) 
#endif


    /* generate "random" nonce value here:  spread out random() using the
       influence of two absolute real times by exploiting start up time
       and current time, plus the pid, plus host name; i.e. HMAC with
       random()+time() in binary as the key, and the APOP challenge message
       in text as the data. */

  {
    UINT4         key[4]; 
    unsigned char nonce[ HMAC_MD5_SIZE ];

    key[ 0 ] = (UINT4)random();                    /* seeded with start up time */
    key[ 1 ] = (UINT4)time((time_t *)0);           /* include current time      */

    srandom( (unsigned int) time((TIME_T *)0) );   /* seed random with the current
                                                      time to nearest second */

    key[ 2 ] = (UINT4)random();                    /* two more contributions of */
    key[ 3 ] = (UINT4)random();                    /* random()                  */
    
    hmac_md5( p->md5str, strlen( p->md5str ),      /* <pid.startup_time@myhost> */
              (unsigned char *)key, sizeof( key ), /* random()& time() as key   */
              nonce );

    /* Interleave "current" time into the HMAC digest as the final nonce value */

    memcpy( &(p->schal[ i ]), nonce, 2 );
    i += 2;
    memcpy( &(p->schal[ i ]), (char *)(key+1), 2 );
    i += 2;
    memcpy( &(p->schal[ i ]), nonce+2, 2 );
    i += 2;
    memcpy( &(p->schal[ i ]), (((char *)(key+1))+2), sizeof(UINT4)-2 );
    i += sizeof(UINT4) - 2;
    memcpy( &(p->schal[ i ]), nonce+4, sizeof(nonce)-4 );
    p->schallen = i + sizeof(nonce) - 4;


/***   Used for debugging: remove when no longer needed
{
int x;

printf( "\nnonce:" );  
for (x=0; x<HMAC_MD5_SIZE; x++) printf( x%4?"%02x":" %02x",
                                 *(nonce+x) );
printf( "\n" );
fflush( stdout );
}
 ***/

  }

  outlen = sizeof( outbuf ) - 1;

  if ( encode64( p->schal, p->schallen, outbuf, &outlen ) ) {
    p->CurrentState = halt;
    return(pop_msg(p, POP_FAILURE, HERE,
           "Server internal error encoding base64 challenge."));
  }

  outbuf[ outlen ] = '\0';



/***   Used for debugging: remove when no longer needed
{
  unsigned char  outdbg[ BUFSIZ ];
  int            odlen,x;

  odlen = BUFSIZ;
  decode64( outbuf, outlen, outdbg, &odlen );
  printf( "\nheading:" );
  for (x=0; x<12; x++) {printf( x%4?"%02x":" %02x", *(outdbg+x)&0xFF);}
  printf( ", service id: %s, nonce:", outdbg+12 );   
  for (x=0; x<HMAC_MD5_SIZE; x++) printf( x%4?"%02x":" %02x",
                                 *(outdbg+12+strlen(outdbg+12)+1+x) );

  memcpy( p->schal, outdbg, odlen );
  p->schallen = odlen;

  printf( "\n" );
  fflush( stdout );
}
 ***/


  pop_write(p, "+ ", 2);
  pop_write_line(p, outbuf);
  pop_write_flush(p);


#ifdef INTERNAL_TEST
  if ( strcmp( outbuf, tsmess) )
    printf( "\n\n*** INTERNAL TEST FAILS: Not Expected Server Message:\n  %s\n\n",
          tsmess );
  else
    printf( "\n\n*** INTERNAL TEST PASSES: Server Response as expected.\n\n" );
  fflush( stdout );
#endif

  return( POP_SUCCESS );
}


/* Checkout the client proof and send back the server proof */

static int pop_client_response( p, msg )
  POP  * p;
  char * msg;
{
  SCRAM_MD5_CLIENT crsp;
  int              clen, outlen;
  char             outbuf[ BUFSIZ ];
  unsigned char    sproof[ SCRAM_MD5_DATASIZE ];


#ifdef INTERNAL_TEST
 unsigned char tcans[]="AQAAAMg9jU8CeB4KOfk7sUhSQPs=";
 unsigned char tsans[]="U0odqYw3B7XIIW0oSz65OQ==";

 *p->authid = '*';
 msg=tcans;
#endif


         /* Blow client's Base64 info into raw binary */

  clen = sizeof( crsp );
  if ( decode64( msg, strlen(msg), &crsp, &clen ) 
            || (clen != sizeof( crsp )) ) {
    p->CurrentState = halt;
    return( pop_msg(p,POP_FAILURE, HERE,
              "Bad SCRAM-MD5 authentication Base64 string - good bye.") );
  }

       /* Check out client's requested SCRAM security option */

  p->scram_mac.macinuse = (int)(crsp.secprops[0] & SCRAM_SECMASK);

  if ( (p->scram_mac.macinuse != SCRAM_NOSECURITY)
#ifdef SCRAM_INTEGRITY
            && (p->scram_mac.macinuse != SCRAM_INTEGRITY)
#endif
     )
    return( pop_msg(p,POP_FAILURE, HERE,
              "Unsupported SCRAM-MD5 security option - good bye.") );


      /* Verify   client authentication proof,
         generate server authentication proof, and
         generate MAC integrity key,
         unless the authid has already failed without external warning */

  if( (p->authid[0] == '\0')
           ||
      scram_md5_generate( p->cchal, p->cchallen,
                           p->schal, p->schallen,
                          (char *)&(p->scram_verifiers),
                           sizeof(p->scram_verifiers),
                           SCRAM_VERIFY,
                           &crsp,
                           sproof,
                           &(p->scram_mac.prefix[ SCRAM_MD5_COUNTSIZE ])) )
    return( pop_msg(p,POP_FAILURE, HERE,
              "SCRAM-MD5 authentication fails - good bye.") );

      /* Remember the client's maximum ciphertext length */

  crsp.secprops[ 0 ] = '\0';
  memcpy( &(p->scram_mac.maxoutcipher), crsp.secprops, 4 );
  p->scram_mac.maxoutcipher = ntohl( p->scram_mac.maxoutcipher );
 
      /* Send the server authentication proof to the client */

  outlen = sizeof( outbuf ) - 1;

  if ( encode64( sproof, sizeof( sproof) , outbuf, &outlen ) ) {
    p->CurrentState = halt;
    return( pop_msg(p,POP_FAILURE, HERE,
                     "Server internal error encoding base64 proof.") );
  }

  outbuf[ outlen ] = '\0';
  
  pop_write(p, "+ ", 2);
  pop_write_line(p, outbuf);
  pop_write_flush(p);

  DEBUG_LOG1(p,"SCRAM-MD5 client authentication ok for \"%s\"", p->user);

#ifdef INTERNAL_TEST
 if ( strcmp( outbuf, tsans) )
   printf( "\n\n*** INTERNAL TEST FAILS: Not Expected server znswer:\n  %s\n\n",
          tsans );
 else
   printf( "\n\n*** INTERNAL TEST PASSES: Server answer as expected.\n\n" );
 fflush( stdout );
#endif

  return( POP_SUCCESS );
}




   /* Dispose of client conformation of server proof: accept anything -
      because client abort via "*" would have preempted getting this far */

static int pop_client_ok( p, msg )
  POP  * p;
  char * msg;
{
    DEBUG_LOG1(p,"SCRAM-MD5 authentication accepted by client: \"%s\"", msg);


#ifdef INTERNAL_TEST
 printf( "\n\n*** INTERNAL TEST PASSES: client conformation: %s\n\n", msg );
 printf( "\n\n*** INTERNAL TEST PASSED: bye bye\n\n" );
 fflush( stdout );
 return( POP_FAILURE );
#endif


    /*
     * Check if server mode should be set or reset based on group membership.
     */

    if ( p->grp_serv_mode != NULL
         && check_group ( p, pw, p->grp_serv_mode ) ) {
        p->server_mode = TRUE;
        DEBUG_LOG2 ( p, "Set server mode for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_serv_mode );
    }

    if ( p->grp_no_serv_mode != NULL
         && check_group ( p, pw, p->grp_no_serv_mode ) ) {
        p->server_mode = FALSE;
        DEBUG_LOG2 ( p, "Set server mode OFF for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_no_serv_mode );
    }

    /*
     * Process qpopper-options files, if requested and present.
     */
    check_config_files ( p, pw );

    /*
     * Make a temporary copy of the user's maildrop
     * and set the group and user id
     */
    if ( pop_dropcopy ( p, pw ) != POP_SUCCESS ) 
        return ( POP_FAILURE );

    /*  Initialize the last-message-accessed number */
    p->last_msg = 0;

    if ( p->pLog_login != NULL )
        do_log_login ( p );

    /*  
     * Authorization completed successfully: final server authentication action 
     */
    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "%s has %d visible message%s (%d hidden) in %d octets.",
                        p->user,
                        p->visible_msg_count,
                        p->visible_msg_count == 1 ? "" : "s",
                        (p->msg_count - p->visible_msg_count),
                        p->drop_size ) );
}

#endif
