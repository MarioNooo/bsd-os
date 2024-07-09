/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
 
/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */

/*  LINTLIBRARY */

/* 
 *  Header file for the POP programs
 */

#include "config.h"


#ifndef _POPPER_H
#  define _POPPER_H

#include <sys/types.h>
#include <pwd.h>
#include <stdarg.h>

#ifdef    SCRAM
#  include "md5.h"
#endif /* SCRAM */

#define BINMAIL_IS_SETGID 1

#ifdef HAVE_SYSLOG_H
#  include <syslog.h>
#endif /* HAVE_SYSLOG_H */

#ifdef HAVE_SYS_SYSLOG_H
#  include <sys/syslog.h>
#endif /* HAVE_SYS_SYSLOG_H */

#ifdef ISC
#  include <sys/fcntl.h>
#  include <net/errno.h>
#endif /* ISC */

/* BSD param.h defines BSD as year and month */
#ifdef      HAVE_SYS_PARAM_H
#  include <sys/param.h>
#  if (defined(BSD) && (BSD >= 199103))
#    define     BIND43
#  endif /* (defined(BSD) && (BSD >= 199103)) */
#endif /* HAVE_SYS_PARAM_H */

#if defined(__bsdi__)
#  if   _BSDI_VERSION >= 199608
#    include <login_cap.h>
#  endif /* _BSDI_VERSION >= 199608 */
#endif /* __bsdi__ */

#include <sys/socket.h> /* this needs to be after system .h files */

#ifdef BULLDB
#  undef    DBM     /* used by mts.c and ndbm.h */
#  ifdef GDBM
#    include <gdbm.h>
#  else
#    include <ndbm.h>
#  endif /* GDBM */
#endif /* BULLDB */


#include "version.h"
#include "banner.h"
#include "utils.h" /* for TRUE, FALSE, and BOOL */


#ifdef SCRAM
#  include "hmac-md5.h"
#  include "scram.h"
#endif /* SCRAM */


#define QPOP_NAME "Qpopper"


#define NULLCP          ((char *) 0)
#define SPACE           32
#define TAB             9
#define NEWLINE         '\n'

#define MAXUSERNAMELEN  65
#define MAXDROPLEN      256
#define MAXLINELEN      1024
#define MAXMSGLINELEN   MAXLINELEN
#define MAXFILENAME     256
#define MAXCMDLEN       4
#define MAXPARMCOUNT    5
#define MAXPARMLEN      10
#define ALLOC_MSGS      20
#define OUT_BUF_SIZE    512 /* Amount of output to buffer before forcing a write */

#ifndef MAXHOSTNAMELEN
#  define MAXHOSTNAMELEN    64
#endif /* not MAXHOSTNAMELEN */

#ifdef POPSCO
#  include <sys/security.h>
#  include <sys/audit.h>
#  include <prot.h>
#  define VOIDSTAR
#  ifdef SCOR5
#    undef  VOIDSTAR
#    define VOIDSTAR      (void (*)(int))
#  endif
#else /* not POPSCO */
#  if ( defined(AIX) || defined (HPUX) || defined(IRIX) )
#    define VOIDSTAR      (void (*)(int))
#  else
#    define VOIDSTAR      (void *)
#   endif
#endif /* POPSCO */

#if defined(SCOR5) || defined(AUX) || defined(HPUX)
#  define SIGPARAM int foo
#else
#  define SIGPARAM 
#endif /* SCOR5 or AUX or HPUX */


typedef enum {
    POP_PRIORITY    =   LOG_NOTICE,
    POP_WARNING     =   LOG_WARNING,
    POP_NOTICE      =   LOG_NOTICE,
    POP_INFO        =   LOG_INFO,
    POP_DEBUG       =   LOG_DEBUG
} log_level;

#define POP_LOGOPTS     LOG_PID

#ifdef POPSCO
#  define L_SET  0
#  define L_XTND 2
#endif /* POPSCO */


/* Make sure there is an strerror() function */
#ifndef HAVE_STRERROR
#  define STRERROR(X)  (X < sys_nerr) ? sys_errlist[X] : "" 
#else
#  define STRERROR(X) strerror(X)
#endif /* HAVE_STRERROR */


/* Handy to have something to use in if() statements that
 * is always defined */
#ifdef DEBUG
#  define DEBUGGING 1
#else /* not DEBUG */
#  define DEBUGGING 0
#endif /* DEBUG */


/*
 * Some OSes don't have srandom, but do have srand
 */
#ifndef   HAVE_SRANDOM
#  include <stdlib.h>
#  define srandom srand
#  define  random  rand
#endif /* HAVE_SRANDOM */

/*
 * Some OSes don't have EDQUOT defined.  If so, define it
 * to be something unlikely to be used.
 */
#include <errno.h>
#ifndef EDQUOT
#  include <limits.h>
#  define EDQUOT  INT_MIN /* -2147483647-1 on 32-bit systems */
#endif /* no EDQUOT */

/*
 * Some OSes don't have the newer errors defined.  If so, define
 * to be something equivalent
 */
#ifndef ECONNABORTED
#  define ECONNABORTED EPROTO
#endif /* no ECONNABORTED */
#ifndef EPROTO
#  define EPROTO ECONNABORTED
#endif /* no EPROTO */

/*
 * In standalone mode we need to call _exit rather than exit
 */
#ifdef STANDALONE
#  define EXIT _exit
#else
#  define EXIT exit
#endif /* STANDALONE */


            /* ^A */
#define MMDF_SEP_CHAR   '\001'


#ifndef BULLDIR
#  define BULLDIR       NULL
#endif /* BULLDIR */


#ifndef CONTENT_LENGTH
#  define CONTENT_LENGTH 0
#endif /* CONTENT_LENGTH */


#ifndef MIN
#  define MIN(A,B) ( ( (A) < (B) ) ? (A) : (B) )
#endif /* MIN */

#ifndef MAX
#  define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )
#endif /* MAX */


/*
 * Success or failure.  Such a binary world.
 */
#define POP_OK          "+OK"
#define POP_ERR         "-ERR"

typedef enum {
    POP_FAILURE     =  0,
    POP_SUCCESS     =  1
} pop_result;


#define POP_TERMINATE   '.'
#define POP_TERMINATE_S "."

#ifndef POP_TIMEOUT
#  define POP_TIMEOUT 120 /* timeout connection after this many secs */
#endif /* POP_TIMEOUT */

#ifndef BLOCK_UID
#  define BLOCK_UID   10 /* UID's <= this value are not allowed to access email */
#endif /* BLOCK_UID */

#define DIG_SIZE    16

typedef struct _pop_tls             pop_tls;            /* defined in pop_tls.h */

/*
 * We often call pop_write with a literal.  Any half-decent optimizing
 * compiler can calculate the length of a literal string much easier
 * and more accurately than I.
 */
#define POP_WRITE_LIT(P,STR)    pop_write ( P, STR, strlen(STR) )


        /* Set these to the types your OS returns if they are not
           already typedefed for you */
#define OFF_T       off_t
#define PID_T       pid_t
#define UID_T       uid_t
#define GID_T       gid_t
#define TIME_T      time_t
#define SIZE_T      size_t

#ifdef NEXT
#  undef PID_T
#  define PID_T     int
#endif /* NEXT */

#ifdef CHECK_SHELL
#  define WILDCARD_SHELL    "/POPPER/ANY/SHELL/"
#endif /* CHECK_SHELL */

extern int              errno;

#if !( defined(BSD) && (BSD >= 199306) ) && !defined(__USE_BSD)
   extern int              sys_nerr;
#  ifndef FREEBSD
     extern char         *   sys_errlist[];
#    ifndef SYS_SIGLIST_DECLARED
#      ifndef __linux__
         extern char         *   sys_siglist[];
#      endif /* not linux */
#    endif /* not SYS_SIGLIST_DECLARED */
#  endif /* not FREEBSD */
#endif /* not BSD >= 199306 and not __USE_BSD */

extern int               pop_timeout;

extern volatile BOOL     hangup;


#define pop_command         pop_parm[0]     /*  POP command is first token */
#define pop_subcommand      pop_parm[1]     /*  POP XTND subcommand is the 
                                                second token */

typedef enum {                              /*  POP processing states */
    auth1,                                  /*  Authentication: waiting for 
                                                USER/APOP/AUTH  command    */
    auth2,                                  /*  Authentication: waiting for
                                                PASS or RPOP or in AUTH    */
    trans,                                  /*  Transaction                */
    update,                                 /*  Update:  session ended, 
                                                process maildrop changes    */
    halt,                                   /*  (Halt):  stop processing 
                                                and exit                    */
    error                                   /*  (Error): something really 
                                                bad happened                */
} state;


typedef enum {                              /* Authentication Method In Use: */
    noauth,                                 /*     not established           */
#ifdef APOP
    apop,                                   /*     APOP                      */
#endif
#ifdef KERBEROS
    kerberos,                               /*     Kerberos [user/pass or    
                                                             user/rpop]      */
#endif
#ifdef SCRAM
    scram_md5,                              /*     SCRAM-MD5                 */
#endif
    plain                                   /*     plain text [user/pass or  
                                                               user/rpop]    */
} auth_type;


typedef enum {                               /* Authentication Success  Substates */
    auth=0,                                  /*  AUTH  begun                      */
#ifdef SCRAM
    clchg,                                   /*  Client challenge received        */
    clrsp,                                   /*  Client proof success             */
    clok,                                    /*  Client likes the server's stuff  */
#endif
#ifdef APOP
    apopcmd,                                 /*  APOP success under apop          */
#endif
    user,                                    /*  USER under plain or kerberos     */
    pass,                                    /*  PASS under plain or kerberos     */
    rpop,                                    /*  RPOP under plain or kerberos     */
    none                                     /*  not established or don't care    */
} auth_state;


/*
 * Enumeration for chunky (pooled) network writes
 */
typedef enum {                               /* Pool network writes:              */
    ChunkyAlways = 0,                        /* - Always                          */
    ChunkyTLS,                               /* - Only with TLS/SSL               */
    ChunkyNever                              /* - Never                           */
} chunky_type;


/*
 * Enumeration for log-facility option
 */
typedef enum {
    PopLogMail     = LOG_MAIL,
    PopLogLocal0   = LOG_LOCAL0,
    PopLogLocal1   = LOG_LOCAL1,
    PopLogLocal2   = LOG_LOCAL2,
    PopLogLocal3   = LOG_LOCAL3,
    PopLogLocal4   = LOG_LOCAL4, 
    PopLogLocal5   = LOG_LOCAL5,
    PopLogLocal6   = LOG_LOCAL6,
    PopLogLocal7   = LOG_LOCAL7
} log_facility_type;


/*
 * Enumeration for clear text password handling
 */
typedef enum {                               /* Clear text passwords are:         */
    ClearTextDefault = 0,                    /* - OK if nothing better available  */
    ClearTextNever,                          /* - never OK                        */
    ClearTextTLS,                            /* - OK if used with TLS/SSL         */
    ClearTextAlways,                         /* - OK even if APOP password exists */
    ClearTextLocal                           /* - OK on 127.* (local loopback)    */
} clear_text_type;


/*
 * Enumeration for how TLS/SSL is supported
 */
typedef enum {
    QPOP_TLS_NONE = 0,     /* We don't support TLS/SSL */
    QPOP_TLS_ALT_PORT,     /* We initiate TLS immediately */
    QPOP_TLS_STLS          /* We support the STLS command */
} tls_support_type;


/*
 * Enumeration for TLS/SSL versions
 */
typedef enum {
    QPOP_TLSvDEFAULT = 0,   /* unspecified */
    QPOP_SSLv2,             /* SSL version 2 only */
    QPOP_SSLv3,             /* SSL version 3 only */
    QPOP_TLSv1,             /* TLS version 1 only */
    QPOP_SSLv23             /* TLSv1, SSLv3, and SSLv2 */
} tls_vers_type;


/*
 * Enumeration for configuration option types
 */
typedef enum {                               /* Config file option is:         */
    CfgBool = 1,                             /* - boolean                      */
    CfgInt,                                  /* - integer                      */
    CfgMnem,                                 /* - mnemonic                     */
    CfgStr,                                  /* - string                       */
    CfgBad                                   /* - INVALID VALUE                */
} config_opt_type;

/*
 * Enumeration for configuration option restrictions
 */
typedef enum {                               /* Option restrictions:           */
    CfgResNone = 1,                          /* - no restrictions              */
    CfgResUser,                              /* - not valid under user-control */
    CfgResInit                               /* - not valid after nw traffic   */
} config_restr_type;

/*
 * Enumeration for identifying at what stage a config file is being processed
 */
typedef enum {                               /* Config file is being processed: */
    CfgInit =1,                              /* - During our initialization     */
    CfgConnected,                            /* - After clent traffic           */
    CfgUser                                  /* - File is under user control    */
} config_call_type;


typedef struct {                                /*  State information for 
                                                    each POP command */
    state       ValidCurrentState;              /*  The operating state of 
                                                    the command */
    char   *    command;                        /*  The POP command */
    int         min_parms;                      /*  Minimum number of parms 
                                                    for the command */
    int         max_parms;                      /*  Maximum number of parms 
                                                    for the command */
    int         (*function) ();                 /*  The function that process 
                                                    the command */
    state       result[2];                      /*  The resulting state after 
                                                    command processing */
} state_table;

typedef struct {                                /*  Table of extensions */
    char   *    subcommand;                     /*  The POP XTND subcommand */
    int         min_parms;                      /*  Minimum number of parms for
                                                    the subcommand */
    int         max_parms;                      /*  Maximum number of parms for
                                                    the subcommand */
    int         (*function) ();                 /*  The function that processes 
                                                    the subcommand */
} xtnd_table;


typedef struct {                                /*  Table of config file settings */
    char             *name;                     /*  The option name */
    config_opt_type   type;                     /*  The option type */
    config_restr_type restriction;              /*  Restrictions    */
    int               value;                    /*  Value for switch */
} config_table;


typedef struct {
    char *auth_mechanism;                       /* Name of mechanism */
    int (*function)();                          /* Function that handles */
} auth_table;


typedef struct {                                /*  Message information */
    int         number;                         /*  Message number relative to 
                                                    the beginning of list */
    int         visible_num;                    /*  Visible message number (in
                                                    case messages are hidden) */
    long        length;                         /*  Length of message in 
                                                    bytes */
    int         lines;                          /*  Number of (null-terminated)
                                                    lines in the message */
    int         body_lines;                     /*  Number of (null-terminated)
                                                    lines in the body */
    long        offset;                         /*  Offset from beginning of 
                                                    file */
    BOOL        del_flag;                       /*  Flag indicating if message 
                                                    is marked for deletion */
    BOOL        hide_flag;                      /*  Flag indicating if message
                                                    is hidden but not deleted */
    BOOL        retr_flag;                      /*  Flag indicating if message 
                                                    was retrieved */
    BOOL        orig_retr_state;                /*  What was the state at the
                                                    start of this session.
                                                    Used for RSET cmd. */
    char        uidl_str [ (DIG_SIZE * 2) + 2 ];/*  Cache of the UIDL str for
                                                    faster access */
} MsgInfoList;

typedef struct _pop POP;
#define QPSTACKSIZE 2                           /* Chosen for Implementation */
typedef void *(*FP)(POP *);
typedef struct CallStack {
    FP Stack[QPSTACKSIZE];                      /* Function Pointers array */
    int CurP;
} CALLSTACK;

struct _pop {                                   /*  POP parameter block */
    BOOL                debug;                  /*  Debugging requested */
    BOOL                xmitting;               /*  =1 xtnd xmit started */
    BOOL                bStats;                 /*  Stats requested */
    BOOL                dirty;                  /*  Any mailbox changes? */
    BOOL                bKerberos;              /*  Flag to enable kerberos
                                                    authentication */
    char            *   kerberos_service;       /*  Kerberos service
                                                    being used */
    BOOL                server_mode;            /*  Default at compile time */
    int                 check_lock_refresh;     /*  Check for lock refresh every x msgs */
    char            *   myname;                 /*  The name of this POP 
                                                    daemon program */
    char            *   myhost;                 /*  The name of our host 
                                                    computer */
    char            *   client;                 /*  Canonical name of client 
                                                    computer */
    char            *   ipaddr;                 /*  Dotted-notation format of 
                                                    client IP address */
    unsigned short      ipport;                 /*  Client port for privileged 
                                                    operations */
    BOOL                bDowncase_user;         /*  TRUE to downcase user name */
    BOOL                bTrim_domain;           /*  TRUE to trim domain from user name */
    char                user[MAXUSERNAMELEN];   /*  Name of the POP user */
    
#if     defined(__bsdi__) && _BSDI_VERSION >= 199608
    char            *   style;                  /*  style of auth used */
    char            *   challenge;              /*  challenge, if any */
    login_cap_t     *   class;                  /*  user's class info */
#endif /* __bsdi__) && _BSDI_VERSION >= 199608 */

    clear_text_type     AllowClearText;         /*  Setting for clear text passwords */
    state               CurrentState;           /*  The current POP operational
                                                    state */
    auth_type           AuthType;               /*  authentication type used */
    auth_state          AuthState;              /*  curent authentication 
                                                    (sub) state */
    MsgInfoList     *   mlp;                    /*  Message information list */
    int                 msg_count;              /*  Number of messages in 
                                                    the maildrop */
    int                 visible_msg_count;      /*  Number of visible messages */
    int                 first_msg_hidden;       /*  =1 if the first msg is hidden */
    int                 msgs_deleted;           /*  Number of messages flagged 
                                                    for deletion */
    int                 last_msg;               /*  Last message touched by 
                                                    the user */
    long                bytes_deleted;          /*  Number of maildrop bytes 
                                                    flagged for deletion */
    char                drop_name[MAXDROPLEN];  /*  The name of the user's 
                                                    maildrop */
    char                temp_drop[MAXDROPLEN];  /*  The name of the user's 
                                                    temporary maildrop */
    long                drop_size;              /*  Size of the maildrop in
                                                    bytes */
    long                spool_end;              /*  Offset of the end of the
                                                    mailspool */
    FILE            *   drop;                   /*  (Temporary) mail drop */
    int                 input_fd;               /*  Input TCP/IP communication 
                                                    file descriptor */
    char                pcInBuf[2*MAXLINELEN];  /*  Input line buffering */
    char            *   pcInEnd;
    char            *   pcInStart;
    int                 nInBufUsed;             /*  Bytes in input buffer */
    int                 nInBufStart;            /*  Offset to 1st read byte */
    FILE            *   output;                 /*  Output TCP/IP communication
                                                    stream */
    char                pcOutBuf[OUT_BUF_SIZE]; /*  Buffer up output so writes
                                                    to network and TLS layer
                                                    go in nice big chunks */
    int                 nOutBufUsed;            /*  Number of bytes in out
                                                    buffer */
    FILE            *   trace;                  /*  Debugging trace file */
    char            *   trace_name;             /*  Name of debugging trace file */
    FILE            *   hold;                   /*  In SERVER_MODE, this value
                                                    holds the drop FILE */
    CALLSTACK           InProcess;              /*  Call Stack that holds the
                                                    function to call for
                                                    continuation */
    char            *   inp_buffer;             /*  Input Stream */
    char            *   pop_parm[MAXPARMCOUNT]; /*  Parse POP parameter list */
    int                 parm_count;             /*  Number of parameters in 
                                                    parsed list */
    char            *   bulldir;                /*  Bulletin directory */
    int                 nMaxBulls;              /*  Max bulletins for new users */
    
#ifdef BULLDB
#  ifdef GDBM
    GDBM_FILE           bull_db;
#  else
    DBM            *    bull_db;                /*  Central bulletin database*/
#  endif
    BOOL                bulldb_nonfatal;        /*  OK to not open bulldb ? */
    int                 bulldb_max_tries;       /*  Max tries to open bulldb */
#endif

    BOOL                bNo_mime;               /*  turn all MIME into plain
                                                    text */
    char            *   mmdf_separator;         /*  string between messages */
    char                md5str[BUFSIZ];         /*  String used with the shared
                                                    secret to create the md5
                                                    digest, i.e. APOP server
                                                    challenge */
    tls_support_type    tls_support;            /* TLS/SSL supported and how */
    BOOL                tls_started;            /* handshake done */
    struct _pop_tls *   tls_context;            /* General place to put 
                                                   TLS context */
    char            *   tls_identity_file;      /* Single file with all certs, plus
                                                   private key. */
    char            *   tls_server_cert_file;   /* File with our certificate. */
    char            *   tls_private_key_file;   /* File with our private key. */
    char            *   tls_passphrase;         /* To decrypt private key  */
    char            *   tls_cipher_list;        /* To restrict ciphers */
    tls_vers_type       tls_version;            /* To restrict TLS/SSL versions */
    int                 tls_options;            /* Desired SSL_OP_ bit flags */

#ifdef SCRAM
    char                cchal[ MAXLINELEN ];    /* plain text client challenge */ 
    int                 cchallen;
    char                schal[ MAXLINELEN ];    /* plain text server challenge */
    int                 schallen;
    SCRAM_MD5_VRFY      scram_verifiers;        /* SCRAM-MD5 verifiers & salt  */
    SCRAM_MD5_INTEGRITY scram_mac;              /* SCRAM-MD5 MAC controls      */
    char                authid[MAXUSERNAMELEN]; /*  Authentication ID (SASL 
                                                    only) */
#endif /* SCRAM */

    time_t              login_time;             /* Elapsed time to login */
    time_t              init_time;              /* Elapsed time to init drop info */
    time_t              clean_time;             /* Elapsed time to clean up */

#ifdef    DRAC_AUTH
    char            *   drac_host;              /* Host which handles drac */
#endif /* DRAC_AUTH */

    BOOL                bUser_opts;             /* Process ~/.qpopper-options */
    BOOL                bSpool_opts;            /* Process .qpopper-options */
    BOOL                bFast_update;           /* Use rename(2) instead of copying */
    int                 login_delay;            /* Announced minimum login delay */
    int                 expire;                 /* Announced message expiration */
    BOOL                bDo_timing;             /* Generate timing records */
    BOOL                bCheck_old_spool_loc;   /* Check for temp spool in spool dir despite hashed spool or home spool*/
    BOOL                bCheck_hash_dir;        /* Check and create hash dirs */
    BOOL                bCheck_pw_max;          /* Check for expired passwords */
    BOOL                bUpdate_status_hdrs;    /* Update 'Status:' & 'Message-ID:' */
    BOOL                bUpdate_on_abort;       /* Go into UPDATE on abort */
    BOOL                bAuto_delete;           /* Delete msgs */
    BOOL                bGroup_bulls;           /* Bulletins go to groups */
    int                 hash_spool;             /* Using hashed spools? */
    char               *pHome_dir_mail;         /* Home dir is spool loc */
    BOOL                bOld_style_uid;         /* Generate pre-3.x UIDs */
    BOOL                bUW_kluge;              /* Ignore UW status msgs */
    BOOL                bKeep_temp_drop;        /* Keep temp drop file */
    char               *grp_serv_mode;          /* Group for server mode */
    char               *grp_no_serv_mode;       /* Group for no server mode */
    char               *authfile;               /* Authorization file */
    char               *nonauthfile;            /* Non-authorization file */
    BOOL                bShy;                   /* Hide version from non-auth users */
    struct passwd       pw;                     /* Cached info from getpwnam (or elsewhere) */
    gid_t               orig_group;             /* Original primary group ID */
    char               *pMail_command;          /* Command to send mail (e.g., path to sendmail) */
    char               *pCfg_spool_dir;         /* Spool directory */
    char               *pCfg_temp_dir;          /* Temp spool directory */
    char               *pCfg_temp_name;         /* Temp spool file name constant */
    char               *pCfg_cache_dir;         /* Cache directory */
    char               *pCfg_cache_name;        /* Cache file name constant */
    chunky_type        chunky_writes;           /* When to pool network writes */
    BOOL               bNo_atomic_open;         /* open() isn't automic. */
    log_facility_type   log_facility;           /* Which log facility to use */
    char               *pLog_login;             /* String to use when logging log-ins */
};


typedef enum { HANGUP,  /* SIGHUP */ 
               TIMEOUT, 
               ABORT    /* Client closes connection */
             } EXIT_REASON;


#ifdef KERBEROS

#  if defined(KRB4) && defined(KRB5)
#    error you can only use one of KRB4, KRB5
#  endif /* KRB4 && KRB5 */

#  if !defined(KRB4) && !defined(KRB5)
#    error you must use one of KRB4, KRB5
#  endif /* !KRB4 && !KRB5 */

#  ifdef KRB4
#    ifndef KERBEROS_SERVICE
#      define KERBEROS_SERVICE   "rcmd"
#    endif

#    ifdef SOLARIS2
#      include <kerberos/krb.h>
#    else /* not SOLARIS2 */
#      ifdef __bsdi__
#        include <kerberosIV/des.h>
#        include <kerberosIV/krb.h>
#      else /* not __bsdi__ */
#        include <krb.h>
#      endif /* __bsdi__ or not */
#    endif /* SOLARIS2 or not */

   extern AUTH_DAT kdata;

#  endif /* KRB4 */

#  ifdef KRB5
#    ifndef KERBEROS_SERVICE
#      define KERBEROS_SERVICE "pop"
#    endif
#  endif /* KRB5 */

#endif /* KERBEROS */


int  pop_apop         (POP *p);
int  pop_auth         (POP *p);
int  pop_bull         (POP *p, struct passwd *pwp);
int  pop_capa         (POP *p);
int  pop_dele         (POP *p);
int  pop_dropcopy     (POP *p, struct passwd *pwp);
int  pop_epop         (POP *p);
int  pop_euidl        (POP *p);
int  pop_exit         (POP *p, EXIT_REASON e);
int  pop_init         (POP *p, int argcount, char **argmessage);
int  pop_last         (POP *p);
int  pop_list         (POP *p);
int  pop_mdef         (POP *p);
int  pop_noop         (POP *p);
int  pop_parse        (POP *p, char *buf);
int  pop_pass         (POP *p);
int  pop_quit         (POP *p);
int  pop_restore      (POP *p);
int  pop_rpop         (POP *p);
int  pop_rset         (POP *p);
int  pop_scram        (POP *p);
int  pop_send         (POP *p);
int  pop_sendline     (POP *p, char *buffer);
int  pop_stat         (POP *p);
int  pop_stls         (POP *p);
struct _pop_tls *pop_tls_init(POP *pPOP);
int  pop_uidl         (POP *p);
int  pop_updt         (POP *p);
int  pop_user         (POP *p);
void pop_write        (POP *p, char *buffer, int length);
void pop_write_flush  (POP *p);
void pop_write_fmt    (POP *p, const char *format, ...) ATTRIB_FMT(2,3);
void pop_write_line   (POP *p, char *buffer);
int  pop_xlst         (POP *p);
int  pop_xmit         (POP *p);
int  pop_xmit_clean   (POP *p);
int  pop_xmit_exec    (POP *p);
int  pop_xmit_recv    (POP *p, char *buffer);
int  pop_xtnd         (POP *p);

pop_result pop_config (POP *p, char *config_file, config_call_type CallTime);
pop_result delete_msg (POP *p, MsgInfoList *mp);
pop_result undelete_msg(POP *p, MsgInfoList *mp);
pop_result set_option (POP *p, char *opt_name, char *opt_value);

int  decode64(char *instr, int instrlen, char *outstr, int *outstrlen);
int  encode64(char *instr, int instrlen, char *outstr, int *outstrlen);

int pop_log (POP *p, log_level  loglev, const char  *fn, size_t ln, const char *format, ... ) ATTRIB_FMT(5,6);  /* parameters for format string */
int pop_msg (POP *p, pop_result status, const char  *fn, size_t ln, const char *format, ... ) ATTRIB_FMT(5,6);
int pop_logv(POP *p, log_level  loglev, const char  *fn, size_t ln, const char *format, va_list ap ) ATTRIB_FMT(5,0); /* parameters for format string */

int  cache_write   (POP *p, int spool_fd, MsgInfoList *mp);
int  cache_read    (POP *p, int spool_fd, MsgInfoList **mp);
void cache_unlink  (POP *p);

pop_tls *pop_tls_init(POP *pPOP);
int      pop_tls_handshake(pop_tls *);
int      pop_tls_write(pop_tls *, char *, int);
int      pop_tls_flush(pop_tls *);
int      pop_tls_read(pop_tls *, char *, int);
void     pop_tls_shutdown(POP *pPOP);

int  mkstemp       (char *path);
void pop_lower     (char *buf);

int   check_group        (POP *p, struct passwd *pwd, const char *group_name);
int   isfromline         (char *cp);
void  downcase_uname     (POP *p, char *q);
void  trim_domain        (POP *p, char *q);
char *get_state_name     (state curState );
void  check_config_files (POP *p, struct passwd *pwp );
int   checkauthfile      (POP *p);
int   checknonauthfile   (POP *p);
void  do_log_login       (POP *p);

int  qpopper ( int argc, char *argv[] );

extern char *pwerrmsg;

#define pop_auth_fail   pop_msg


int          Push        (CALLSTACK *, FP);
FP           Pop         (CALLSTACK *);
int          StackSize   (CALLSTACK *);
CALLSTACK   *StackInit   (CALLSTACK *);
FP           GetTop      (CALLSTACK *, int *);
FP           GetNext     (CALLSTACK *, int *);


char *get_time();
extern char *ZONE;

struct topper {
  int    lines_to_output;
  void **out_state;
  char   last_char;
  char   in_header;
  char   is_top;
  unsigned long    body_lines;
} ;
extern struct topper topper_global;
#define MSG_LINES (topper_global.body_lines)

typedef struct {
    char *mdef_macro;
    char *mdef_value;
}MDEFRecordType;              /* To maintain MDEF Array defined 
                                 in pop_extend.c. Used by pop_parse
                                 for expansion. */
enum { MAX_MDEF_INDEX = 10 };
enum { MAX_MDEF_NAME  = 100 };


MsgInfoList *msg_ptr ( POP *p, int msg_num );


#define HERE    __FILE__,        __LINE__
#define WHENCE  const char * fn, size_t ln

#ifdef DEBUG

#  define DEBUG_LOG0(a,b) do {                                      \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b);                       \
    } while ( 0 );
#  define DEBUG_LOG1(a,b,a1) do {                                   \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1);                   \
    } while ( 0 );
#  define DEBUG_LOG2(a,b,a1,a2) do {                                \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2);               \
    } while ( 0 );
#  define DEBUG_LOG3(a,b,a1,a2,a3) do {                             \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3);           \
    }while(0);
#  define DEBUG_LOG4(a,b,a1,a2,a3,a4) do {                          \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4);       \
    } while ( 0 );
#  define DEBUG_LOG5(a,b,a1,a2,a3,a4,a5) do {                       \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5);   \
    } while ( 0 );
#  define DEBUG_LOG6(a,b,a1,a2,a3,a4,a5,a6) do {                    \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6);                                          \
    } while ( 0 );
#  define DEBUG_LOG7(a,b,a1,a2,a3,a4,a5,a6,a7) do {                 \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7);                                      \
    } while ( 0 );
#  define DEBUG_LOG8(a,b,a1,a2,a3,a4,a5,a6,a7,a8) do {              \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7, a8);                                  \
    } while ( 0 );
#  define DEBUG_LOG9(a,b,a1,a2,a3,a4,a5,a6,a7,a8,a9) do {           \
        if ( (a)->debug )                                           \
            pop_log ( a, POP_DEBUG, HERE, b, a1, a2, a3, a4, a5,    \
                      a6, a7, a8, a9);                              \
    } while ( 0 );

#else

#  define DEBUG_LOG0(a,b) 
#  define DEBUG_LOG1(a,b,a1) 
#  define DEBUG_LOG2(a,b,a1,a2) 
#  define DEBUG_LOG3(a,b,a1,a2,a3) 
#  define DEBUG_LOG4(a,b,a1,a2,a3,a4) 
#  define DEBUG_LOG5(a,b,a1,a2,a3,a4,a5) 
#  define DEBUG_LOG6(a,b,a1,a2,a3,a4,a5,a6)
#  define DEBUG_LOG7(a,b,a1,a2,a3,a4,a5,a6,a7)
#  define DEBUG_LOG8(a,b,a1,a2,a3,a4,a5,a6,a7,a8)
#  define DEBUG_LOG9(a,b,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif

#ifdef    _DEBUG
#  define _DEBUG_LOG0 DEBUG_LOG0
#  define _DEBUG_LOG1 DEBUG_LOG1
#  define _DEBUG_LOG2 DEBUG_LOG2
#  define _DEBUG_LOG3 DEBUG_LOG3
#  define _DEBUG_LOG4 DEBUG_LOG4
#  define _DEBUG_LOG5 DEBUG_LOG5
#  define _DEBUG_LOG6 DEBUG_LOG6
#  define _DEBUG_LOG7 DEBUG_LOG7
#  define _DEBUG_LOG8 DEBUG_LOG8
#  define _DEBUG_LOG9 DEBUG_LOG9
#else
#  define _DEBUG_LOG0(a,b)
#  define _DEBUG_LOG1(a,b,a1)
#  define _DEBUG_LOG2(a,b,a1,a2)
#  define _DEBUG_LOG3(a,b,a1,a2,a3)
#  define _DEBUG_LOG4(a,b,a1,a2,a3,a4)
#  define _DEBUG_LOG5(a,b,a1,a2,a3,a4,a5)
#  define _DEBUG_LOG6(a,b,a1,a2,a3,a4,a5,a6)
#  define _DEBUG_LOG7(a,b,a1,a2,a3,a4,a5,a6,a7)
#  define _DEBUG_LOG8(a,b,a1,a2,a3,a4,a5,a6,a7,a8)
#  define _DEBUG_LOG9(a,b,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#endif /* _DEBUG */


#endif /* _POPPER_H */
