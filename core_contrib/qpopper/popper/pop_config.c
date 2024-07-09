/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     08/06/02  [rcg]
 *             - Added ability to set OpenSSL options (SSL_OP_* values).
 *               Note that unlike other options, if this is set more
 *               than once, the values are added together.
 *
 *     06/01/01 [RCG]
 *             - Added 'uw-kludge' as synonym for 'uw-kluge'.
 *
 *     02/12/01 [RCG]
 *             - Now saving trace file name.
 *
 *     02/08/01 [RCG]
 *             - Added 'max-bulletins' integer option.
 *
 *     02/02/01 [RCG]
 *             - Can now change trace file.
 *             - Added 'chunky-writes'.
 *             - Added 'no-atomic-open' Boolean option.
 *             - Added 'log-facility' mnemonic option.
 *             - Added 'log-login' string option.
 *
 *     01/30/01 [RCG]
 *             - Now using p->pCfg_spool_dir insted of POP_MAILDIR.
 *
 *     01/12/01 [RCG]
 *             - login_delay and expire now in p.
 *
 *     12/27/00  [RCG]
 *             - Now processing home directory config files before
 *               spool directory files, to allow admin to override
 *               user settings.
 *
 *     12/21/00  [RCG]
 *             - Added 'tls-support'
 *             - Deleted 'enable-stls'
 *             - Deleted 'alternate-port-tls'
 *
 *     11/14/00  [RCG]
 *             - Added 'tls-version'.
 *             - Recognize 'clear-text-password = tls' for TLS/SSL.
 *             - Added 'trim-domain'.
 *             - Added mnemonic options.
 *
 *     11/09/00  [RCG]
 *             - Added 'tls-cipher-list'.
 *
 *     10/31/00  [RCG]
 *             - Added 'tls-server-cert-file'.
 *             - Added 'tls-private-key-file'.
 *
 *     10/21/00  [RCG]
 *             - Now handling quoted strings as values.
 *             - Added 'alternate-port-tls'.
 *             - Added 'enable-stls'.
 *             - Added 'tls-identity-file'.
 *             - Added 'tls-passphrase'.
 *
 *     10/02/00  [RCG]
 *             - Added 'fast-update'.
 *             - Added 'spool-opts'.
 *
 *     08/17/00  [RCG]
 *             - Added 'bulldb-nonfatal'.
 *             - Added 'bulldb-max-tries'.
 *             - Added ability to restrict options so users can't set them.
 *
 *     06/27/00  [RCG]
 *             - Added guts.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#ifdef        QPOP_OPENSSL
#  include <openssl/ssl.h>
#endif     /* QPOP_OPENSSL */

#include "popper.h"
#include "utils.h"
#include "string_util.h"
#include "snprintf.h"


#define COMMENT_CHAR    '#'
#define MAX_RECURSION   100



/*---------------------------------------------------------------------
 * To add a new configuration file option requires in most cases three
 * steps: 1.  Add an 'opt_val_type' enumeration.  Specify the command-
 *            line switch, if any, in the comment.
 *        2.  Add an entry in the 'options' table.  This specifies the
 *            option name, the type, and any restrictions.  (It also
 *            maps this information to the enumeration.)  The type
 *            choices are integer, Boolean, string, or mnemonic.  The
 *            restiction can be none, user (can't be set in a user-
 *            controlled file), or init (must be set during
 *            initialization).
 *        3.  Add an case to the switch in the 'get_opt_ptr' function.
 * 
 * Mnemonic options require three additional steps:
 *        1.  Create an enumeration (usually in popper.h) for the
 *            option's values.
 *        2.  Create a nmemonic-mapping table in this file, in section
 *            marked "Maps for specific options..." below.
 *        3.  Add an entry to associate the nmemonic-mapping table with
 *            the option, in the 'cfg_mnem_table' table.
 *
 * If the option requires any special handling (such as only being valid
 * with a compile-time option or library, or if any anything extra needs
 * to be done before or after setting it), add a case in the
 * 'handle_value' function.
 *---------------------------------------------------------------------*/



/*
 * The possible errors.
 */
typedef enum
{
    kER_SUCCESS = 1,
    kER_NOFILE,
    kER_LINE_LEN,
    kER_EXP_SET,
    kER_UNKN_OPT,
    kER_UNKN_MNEM,
    kER_RESTR,
    kER_NOT_BOOL,
    kER_EXP_EQL,
    kER_BAD_VAL,
    kER_EXP_BOOL,
    kER_EXP_INT,
    kER_EXP_STR,
    kER_EXP_MNEM,
    kER_EXP_EOL,
    kER_RECURS,
    kER_CONNECTED,
    kER_NOT_COMPILED_WITH,
    kER_ALREADY_SET,
    kER_HANDLED,
    kER_SYSTEM,
    kER_INTERNAL,
    
    kER_LAST_ERROR
} error_code_type;


/*
 * The values of the recognized options.
 */
typedef enum
{
    kBULLDIR    = 1,        /* -b bulldir       */
    kBULLDBNONFATAL,        /* -B               */
    kBULLDBMAXTRIES,        /*    (no flag)     */
    kDCASEUSER,             /* -c               */
    kDEBUG,                 /* -d               */
    kDRACHOST,              /* -D drac-host     */
    kANNLOGINDELAY,         /* -eLOGIN_DELAY=x  */
    kANNEXPIRE,             /* -eEXPIRE=x       */
    kCONFIGFILE,            /* -f config-file   */
    kFASTUPDATE,            /* -F               */
    kKERBEROS,              /* -k               */
    kKERBSERVICE,           /* -K service-name  */
    kTLS_SUPPORT,           /* -l 0|1|2         */
    kLOCKCHECK,             /* -L lock-refresh  */
    kCLEARPASS,             /* -p 0|1|2|3|4     */
    kREVLOOKUP,             /* -R               */   /* SENSE REVERSED! */
    kSTATS,                 /* -s               */
    kSERVER,                /* -S               */
    kTRACENAME,             /* -t logfile       */
    kTRIMDOMAIN,            /* -C               */
    kTIMEOUT,               /* -T timeout       */
    kUSEROPTS,              /* -u               */
    kSPOOLOPTS,             /* -U               */
    kTLS_IDENT,             /*    (no flag)     */
    kTLS_PASSPHRASE,        /*    (no flag)     */
    kTLS_SERVCERTFILE,      /*    (no flag)     */
    kTLS_PRIVKEYFILE,       /*    (no flag)     */
    kTLS_CIPHERLIST,        /*    (no flag)     */
    kTLS_VERSION,           /*    (no flag)     */
    kTLS_WORKAROUNDS,       /*    (no flag)     */
    kTLS_OPTIONS,           /*    (no flag)     */
    kDO_TIMING,             /*    (no flag)     */
    kCHECK_OLD_SPOOL_LOC,   /*    (no flag)     */
    kCHECK_HASH_DIR,        /*    (no flag)     */
    kCHECK_PW_MAX,          /*    (no flag)     */
    kUPDATE_STATUS_HDRS,    /*    (no flag)     */
    kUPDATE_ON_ABORT,       /*    (no flag)     */
    kAUTO_DELETE,           /*    (no flag)     */
    kGROUP_BULLS,           /*    (no flag)     */
    kHASH_SPOOL,            /*    (no flag)     */
    kHOME_DIR_MAIL,         /*    (no flag)     */
    kOLD_STYLE_UID,         /*    (no flag)     */
    kUW_KLUGE,              /*    (no flag)     */
    kKEEP_TEMP_DROP,        /*    (no flag)     */
    kGRP_SERV_MODE,         /*    (no flag)     */
    kGRP_NO_SERV_MODE,      /*    (no flag)     */
    kAUTHFILE,              /*    (no flag)     */
    kNONAUTHFILE,           /*    (no flag)     */
    kSHY,                   /*    (no flag)     */
    kMAIL_COMMAND,          /*    (no flag)     */
    kCFG_SPOOL_DIR,         /*    (no flag)     */
    kCFG_TEMP_DIR,          /*    (no flag)     */
    kCFG_TEMP_NAME,         /*    (no flag)     */
    kCFG_CACHE_DIR,         /*    (no flag)     */
    kCFG_CACHE_NAME,        /*    (no flag)     */
    kCHUNKY_WRITES,         /*    (no flag)     */
    kNO_ATOMIC_OPEN,        /*    (no flag)     */
    kLOG_FACILITY,          /* -y facility      */
    kLOG_LOGIN,             /*    (no flag)     */
    kMAXBULLS,              /*    (no flag)     */


    LAST_OPT_VAL
} opt_val_type;


/*
 * Table of recognized options.
 */
static config_table options [] =
{
/*    name                             type      restricted  value                */
    { "announce-expire"              , CfgInt  , CfgResNone, kANNEXPIRE          },
    { "announce-login-delay"         , CfgInt  , CfgResNone, kANNLOGINDELAY      },
    { "auth-file"                    , CfgStr  , CfgResInit, kAUTHFILE           },
    { "auto-delete"                  , CfgBool , CfgResUser, kAUTO_DELETE        },
    { "bulldb-max-tries"             , CfgInt  , CfgResNone, kBULLDBMAXTRIES     },
    { "bulldb-nonfatal"              , CfgBool , CfgResNone, kBULLDBNONFATAL     },
    { "bulldir"                      , CfgStr  , CfgResNone, kBULLDIR            },
    { "cache-dir"                    , CfgStr  , CfgResUser, kCFG_CACHE_DIR      },
    { "cache-name"                   , CfgStr  , CfgResUser, kCFG_CACHE_NAME     },
    { "check-hash-dir"               , CfgBool , CfgResNone, kCHECK_HASH_DIR     },
    { "check-old-spool-loc"          , CfgBool , CfgResNone, kCHECK_OLD_SPOOL_LOC},
    { "check-password-expired"       , CfgBool , CfgResInit, kCHECK_PW_MAX       },
    { "chunky-writes"                , CfgMnem , CfgResNone, kCHUNKY_WRITES      },
    { "clear-text-password"          , CfgMnem , CfgResUser, kCLEARPASS          },
    { "config-file"                  , CfgStr  , CfgResNone, kCONFIGFILE         },
    { "debug"                        , CfgBool , CfgResNone, kDEBUG              },
    { "downcase-user"                , CfgBool , CfgResUser, kDCASEUSER          },
    { "drac-host"                    , CfgStr  , CfgResNone, kDRACHOST           },
    { "fast-update"                  , CfgBool , CfgResNone, kFASTUPDATE         },
    { "group-bulletins"              , CfgBool , CfgResUser, kGROUP_BULLS        },
    { "group-no-server-mode"         , CfgStr  , CfgResUser, kGRP_NO_SERV_MODE   },
    { "group-server-mode"            , CfgStr  , CfgResUser, kGRP_SERV_MODE      },
    { "hash-spool"                   , CfgInt  , CfgResUser, kHASH_SPOOL         },
    { "home-dir-mail"                , CfgStr  , CfgResUser, kHOME_DIR_MAIL      },
    { "keep-temp-drop"               , CfgBool , CfgResUser, kKEEP_TEMP_DROP     },
    { "kerberos"                     , CfgBool , CfgResUser, kKERBEROS           },
    { "kerberos-service"             , CfgStr  , CfgResUser, kKERBSERVICE        },
    { "log-facility"                 , CfgMnem , CfgInit   , kLOG_FACILITY       },
    { "log-login"                    , CfgStr  , CfgInit   , kLOG_LOGIN          },
    { "mail-command"                 , CfgStr  , CfgResUser, kMAIL_COMMAND       },
    { "mail-lock-check"              , CfgInt  , CfgResNone, kLOCKCHECK          },
    { "max-bulletins"                , CfgInt  , CfgResNone, kMAXBULLS           },
    { "no-atomic-open"               , CfgBool , CfgResUser, kNO_ATOMIC_OPEN     },
    { "nonauth-file"                 , CfgStr  , CfgResInit, kNONAUTHFILE        },
    { "old-style-uid"                , CfgBool , CfgResNone, kOLD_STYLE_UID      },
    { "reverse-lookup"               , CfgBool , CfgResUser, kREVLOOKUP          },
    { "server-mode"                  , CfgBool , CfgResNone, kSERVER             },
    { "shy"                          , CfgBool , CfgResInit, kSHY                },
    { "spool-dir"                    , CfgStr  , CfgResUser, kCFG_SPOOL_DIR      },
    { "spool-options"                , CfgBool , CfgResUser, kSPOOLOPTS          },
    { "statistics"                   , CfgBool , CfgResNone, kSTATS              },
    { "temp-dir"                     , CfgStr  , CfgResUser, kCFG_TEMP_DIR       },
    { "temp-name"                    , CfgStr  , CfgResUser, kCFG_TEMP_NAME      },
    { "timeout"                      , CfgInt  , CfgResNone, kTIMEOUT            },
    { "timing"                       , CfgBool , CfgResNone, kDO_TIMING          },
    { "tls-cipher-list"              , CfgStr  , CfgResInit, kTLS_CIPHERLIST     },
    { "tls-identity-file"            , CfgStr  , CfgResInit, kTLS_IDENT          },
    { "tls-passphrase"               , CfgStr  , CfgResInit, kTLS_PASSPHRASE     },
    { "tls-private-key-file"         , CfgStr  , CfgResInit, kTLS_PRIVKEYFILE    },
    { "tls-server-cert-file"         , CfgStr  , CfgResInit, kTLS_SERVCERTFILE   },
    { "tls-support"                  , CfgMnem , CfgResInit, kTLS_SUPPORT        },
    { "tls-version"                  , CfgMnem , CfgResInit, kTLS_VERSION        },
    { "tls-workarounds"              , CfgBool , CfgResUser, kTLS_WORKAROUNDS    },
    { "tls-options"                  , CfgInt  , CfgResUser, kTLS_OPTIONS        },
    { "tracefile"                    , CfgStr  , CfgResNone, kTRACENAME          },
    { "trim-domain"                  , CfgBool , CfgResUser, kTRIMDOMAIN         },
    { "update-on-abort"              , CfgBool , CfgResNone, kUPDATE_ON_ABORT    },
    { "update-status-headers"        , CfgBool , CfgResNone, kUPDATE_STATUS_HDRS },
    { "user-options"                 , CfgBool , CfgResUser, kUSEROPTS           },
    { "UW-kluge"                     , CfgBool , CfgResNone, kUW_KLUGE           },
    { "UW-kludge"                    , CfgBool , CfgResNone, kUW_KLUGE           },

    { NULL                           , CfgBad  , CfgResNone, LAST_OPT_VAL        }
};


/*
 * Mnemonic handling structures
 */

/*
 * Generic mnemonic name to value mapping
 */
typedef struct {
    char *name;
    int   value;
} mnemonic_map;

/*
 * Maps config option to its mnemonic map table
 */
typedef struct {
    int           cfg_value;          /* unique config value (for case) */
    mnemonic_map *cfg_map;            /* table mapping mnemonic name to value */
} _config_mnemonic_table;

/*
 * Maps for specific options...
 */

/* ...clear-text-passwords */
static mnemonic_map mnem_map_clear_text_pwd [] =
{
    { "default"         , ClearTextDefault      },
    { "never"           , ClearTextNever        },
    { "TLS"             , ClearTextTLS          },
    { "SSL"             , ClearTextTLS          },
    { "always"          , ClearTextAlways       },
    { "local"           , ClearTextLocal        },

    { NULL              , ClearTextDefault      }
};


/* ...tls-support */
static mnemonic_map mnem_map_tls_sup [] =
{
    { "default"         , QPOP_TLS_NONE         }, 
    { "none"            , QPOP_TLS_NONE         },
    { "STLS"            , QPOP_TLS_STLS         }, 
    { "alternate-port"  , QPOP_TLS_ALT_PORT     },
    { NULL              , QPOP_TLS_NONE         }
};

/* ...tls-version */
static mnemonic_map mnem_map_tls_vers [] =
{
    { "default"         , QPOP_TLSvDEFAULT      }, 
    { "SSLv2"           , QPOP_SSLv2            },
    { "SSLv3"           , QPOP_SSLv3            }, 
    { "TLSv1"           , QPOP_TLSv1            },
    { "SSLv23"          , QPOP_SSLv23           },
    { "all"             , QPOP_SSLv23           },

    { NULL              , QPOP_TLSvDEFAULT      }
};

/* ...chunky-writes */
static mnemonic_map mnem_map_chunky_writes [] =
{
    { "default"         , ChunkyAlways          },
    { "always"          , ChunkyAlways          },
    { "TLS"             , ChunkyTLS             },
    { "SSL"             , ChunkyTLS             },
    { "never"           , ChunkyNever           },

    { NULL              , ChunkyAlways          }
};

/* ...log-facility */
static mnemonic_map mnem_map_log_facility [] =
{
    { "mail"            , PopLogMail            },
    { "local0"          , PopLogLocal0          },
    { "local1"          , PopLogLocal1          },
    { "local2"          , PopLogLocal2          },
    { "local3"          , PopLogLocal3          },
    { "local4"          , PopLogLocal4          },
    { "local5"          , PopLogLocal5          },
    { "local6"          , PopLogLocal6          },
    { "local7"          , PopLogLocal7          },

    { NULL              , PopLogMail            }
};
    

/*
 * Map each option to its specific table
 */
static _config_mnemonic_table cfg_mnem_table [] =
{
    { kCLEARPASS        , mnem_map_clear_text_pwd },
    { kTLS_SUPPORT      , mnem_map_tls_sup        },
    { kTLS_VERSION      , mnem_map_tls_vers       },
    { kCHUNKY_WRITES    , mnem_map_chunky_writes  },
    { kLOG_FACILITY     , mnem_map_log_facility   },

    { -1                , NULL                     }
};


/*
 * Union for pointer to item in POP structure
 */
typedef union
{
    int              *pInt;
    BOOL             *pBoo;
    char            **pPtr;
}
    opt_ptr_type;


/*
 * Globals
 */
extern int no_rev_lookup;
extern int pop_timeout;

/*
 * Kluge temporary value for items we map to others
 */
static int gTempVal = 0;


/*
 * Prototypes (static to permit auto-inlining by optimizing compilers)
 */
static void          tilt (POP *p, error_code_type er, char *token, int len, size_t line_no, char *cfile, config_table *ctp, WHENCE);
static config_table *find_option (char *token, long len);
static int           find_mnemonic ( config_table *opt, char *token, long len );
static char         *get_mnemonic ( config_table *opt, int val );
static BOOL          char_is_quote         (char *token, char *buffer);
static long          length_in_quoted_str  (char *token, long line_length);
static long          length_to_white_space (char *token, long line_length);
static opt_ptr_type  get_opt_ptr ( POP *p, opt_val_type item );
static long          get_string_length (char **token, long *line_length, char *buffer);
static long          get_token_length  (char  *token, long  line_length);
static void          skip_white_space  (char **token, long *line_length);
static void          skip_token (config_opt_type ttype, char **token, long *token_len, long *line_len);
static void          show_result (POP *p, config_table *opt, int iVal, const char *sVal, WHENCE);
static error_code_type handle_value (POP *p, config_table *opt, char *pval, int plen, config_call_type CallTime);


/*
 * Log an error message.
 */
static void
tilt ( POP              *p, 
       error_code_type   er, 
       char             *token, 
       int               len, 
       size_t            line_no, 
       char             *cfile,
       config_table     *ctp,
       WHENCE )
{
    switch ( er )
    {
        case kER_NOFILE:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Unable to open configuration file %s: %s (%d)",
                      cfile, strerror(errno), errno );
            break;

        case kER_LINE_LEN:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Config file %s line %d too long",
                      cfile, line_no );
            break;

        case kER_EXP_SET:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Expected \"set\" or \"reset\", found \"%.*s\" "
                      "at line %d of config file %s",
                      MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_UNKN_OPT:
            { /* local block */
            size_t        sz  = 0;
            char         *buf = NULL;

            for ( ctp = options; ctp->name != NULL; ctp++ )
                sz += strlen ( ctp->name ) + 4;
            buf = malloc ( sz + 1 );
            if ( buf != NULL )
            {
                buf[0] = '\0';
                for ( ctp = options; ctp->name != NULL; ctp++ )
                {
                    strcat ( buf, "\""      );
                    strcat ( buf, ctp->name );
                    strcat ( buf, "\""      );
                    strcat ( buf, ", "      );
                }
                buf [ strlen ( buf ) - 2 ] = '\0';
            }
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Unrecognized option; scanning \"%.*s\" at line %d of "
                      "config file %s; valid options are: %s",
                      MIN ( 128, len), token, line_no, cfile, buf );
            } /* local block */
            break;

        case kER_UNKN_MNEM:
        case kER_EXP_MNEM:
            { /* local block */
            size_t                  sz   = 0;
            char                    *buf = NULL;
            _config_mnemonic_table  *cmp = NULL;
            mnemonic_map            *map = NULL;

            for ( cmp = cfg_mnem_table; cmp->cfg_value != -1; cmp++ )
            {
                if ( cmp->cfg_value == ctp->value )
                    break;
            }

            if ( cmp->cfg_value == -1 )
            {
                pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                          "Option \"%s\" does not use mnemonics (line %d "
                          "of config file %s)",
                          ctp->name, line_no, cfile );
                break;
            }

            for ( map = cmp->cfg_map; map->name != NULL; map++ )
                sz += strlen ( map->name ) + 4;
            buf = malloc ( sz + 1 );
            if ( buf != NULL )
            {
                buf[0] = '\0';
                for ( map = cmp->cfg_map; map->name != NULL; map++ )
                {
                    strcat ( buf, "\""      );
                    strcat ( buf, map->name );
                    strcat ( buf, "\""      );
                    strcat ( buf, ", "      );
                }
                buf [ strlen ( buf ) - 2 ] = '\0';
            }
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Unrecognized \"%s\" mnemonic; scanning \"%.*s\" at "
                      "line %d of config file %s; valid mnemonics are: %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile, buf );
            } /* local block */
            break;

        case kER_RESTR:
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "Attempt to use restricted option \"%s\" at line %d of "
                      "user config file %s",
                      ctp->name, line_no, cfile );
            break;

        case kER_NOT_BOOL:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "%s is not a boolean option; scanning \"%.*s\" at "
                      "line %d of config file %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_BAD_VAL:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Bad value for the %s option; scanning \"%.*s\" at "
                      "line %d of config file %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_EXP_BOOL:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Boolean value expected for the %s option; "
                      "scanning \"%.*s\" at line %d of config file %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_EXP_INT:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Integer value expected for the %s option; "
                      "scanning \"%.*s\" at line %d of config file %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_EXP_STR:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "String value expected for the %s option; "
                      "scanning \"%.*s\" at line %d of config file %s",
                      ctp->name, MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_EXP_EQL:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "\"=\" expected; found \"%.*s\" at "
                      "line %d of config file %s",
                      MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_RECURS:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Config file nesting exceeds %d; will not process "
                      "config file %s",
                      MAX_RECURSION, cfile );
            break;

        case kER_EXP_EOL:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Expected comment or end of line; found \"%.*s\" at "
                      "line %d of config file %s",
                      MIN ( 128, len), token, line_no, cfile );
            break;

        case kER_CONNECTED:
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "The \"%s\" option cannot be set after a client has "
                      "connected (line %d of config file %s)",
                      ctp->name, line_no, cfile );
            break;

        case kER_NOT_COMPILED_WITH:
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "The \"%s\" option cannot be used because a required "
                      "compile-time option was not set.  See the Administrator's "
                      "Guide for more information (line %d of config file %s)",
                      ctp->name, line_no, cfile );
            break;

        case kER_ALREADY_SET:
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "The \"%s\" option has already been set and cannot be set "
                      "again (line %d of config file %s)",
                      ctp->name, line_no, cfile );
            break;

        case kER_HANDLED: /* required error message already issued */
            break;

        case kER_SYSTEM:
            {
            int save_err = errno;
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "System error handling the \"%s\" option at line "
                      "%d of config file %s: %s (%d)",
                      ctp->name, line_no, cfile, strerror(save_err), save_err );
            }

        case kER_INTERNAL:
            pop_log ( p, POP_DEBUG, fn, ln, /* fn and ln are part of WHENCE */
                      "Internal error handling the \"%s\" option at line "
                      "%d of config file %s",
                      ctp->name, line_no, cfile );
            break;

        default:
            pop_log ( p, POP_PRIORITY, fn, ln, /* fn and ln are part of WHENCE */
                      "Unknown error; scanning \"%.*s\" at "
                      "line %d of config file %s",
                      MIN ( 128, len), token, line_no, cfile );
            break;
    } /* switch ( er ) */
}


/*
 * Lookup an option table entry by name
 */
static config_table *
find_option ( char *token, long len )
{
    config_table *opt = NULL;


    for ( opt = options; opt->name != NULL; opt++ )
    {
        if ( equal_strings ( opt->name, -1, token, len ) == TRUE )
            return opt;
    }

    return NULL;
}


/*
 * Return the value for a mnemonic, or -1 if not found.
 */
static int
find_mnemonic ( config_table *opt, char *token, long len )
{
    _config_mnemonic_table  *cmp = NULL;
    mnemonic_map            *map = NULL;


    /*
     * Get the map record
     */
    for ( cmp = cfg_mnem_table; cmp->cfg_value != -1; cmp++ )
    {
        if ( cmp->cfg_value == opt->value )
            break;
    }

    if ( cmp->cfg_value == -1 )
        return -1;

    /*
     * Search for the mnemonic name
     */
    for ( map = cmp->cfg_map; map->name != NULL; map++ )
    {
        if ( equal_strings ( map->name, -1, token, len ) == TRUE )
            return map->value;
    }

    return -1;
}


/*
 * Return the mnemonic for a value
 */
static char *
get_mnemonic ( config_table *opt, int val )
{
    _config_mnemonic_table  *cmp = NULL;
    mnemonic_map            *map = NULL;


    /*
     * Get the map record
     */
    for ( cmp = cfg_mnem_table; cmp->cfg_value != -1; cmp++ )
    {
        if ( cmp->cfg_value == opt->value )
            break;
    }

    if ( cmp->cfg_value == -1 )
        return NULL;

    /*
     * Search for the mnemonic value
     */
    for ( map = cmp->cfg_map; map->name != NULL; map++ )
    {
        if ( map->value == val )
            return map->name;
    }

    return NULL;
}


/*
 * Is character a quote character?
 */
static BOOL
char_is_quote ( char *token, char *buffer )
{
    if ( *token != '\"' && *token != '`' && *token != '\'' )
        return FALSE;
    if ( ( token - buffer ) > 0 && *(token - 1) == '\\' )
        return FALSE;
    return TRUE;
}


/*
 * Return number of characters in quoted string.
 */
static long
length_in_quoted_str ( char *token, long line_length )
{
    char *orig_ptr = token;
    char  qchar    = *token;


    if ( qchar != '\"' && qchar != '`' && qchar != '\'' )
        return 0;

    token++; /* skip past open quote */
    line_length--;
    while ( line_length > 0 && ( *token != qchar || *(token - 1) == '\\' )  )
    {
        line_length--;
        token++;
    }
    
    return ( token - orig_ptr - 1 );
}


/*
 * Return number of characters until white space.
 */
static long
length_to_white_space ( char *token, long line_length )
{
    char *orig_ptr = token;


    while ( line_length > 0 && *token != ' ' && *token != '\t' )
    {
        line_length--;
        token++;
    }
    
    return ( token - orig_ptr );
}


/*
 * Return pointer to desired item in POP structure
 */
static opt_ptr_type
get_opt_ptr ( POP *p, opt_val_type item )
{
    opt_ptr_type ret_val;

#define R__INT(x) ret_val.pInt=(x);break
#define R__BOO(x) ret_val.pBoo=(x);break
#define R__PTR(x) ret_val.pPtr=(x);break
#define R__MNM(x) ret_val.pInt=((int*)(x));break

    /*
     * First reset the temp global, in case we return a pointer to it.
     */
    gTempVal = 0;

    switch ( item )
    {
        case kBULLDIR:             R__PTR ( &p->bulldir );
#ifdef    BULLDB
        case kBULLDBNONFATAL:      R__BOO ( &p->bulldb_nonfatal );
        case kBULLDBMAXTRIES:      R__INT ( &p->bulldb_max_tries );
#endif /* BULLDB */
        case kDCASEUSER:           R__BOO ( &p->bDowncase_user );
        case kDEBUG:               R__BOO ( &p->debug );
#ifdef    DRAC_AUTH
        case kDRACHOST:            R__PTR ( &p->drac_host );
#endif /* DRAC_AUTH */
        case kANNLOGINDELAY:       R__INT ( &p->login_delay );
        case kANNEXPIRE:           R__INT ( &p->expire );
        case kCONFIGFILE:          R__PTR ( NULL );
#ifdef    KERBEROS
        case kKERBEROS:            R__BOO ( &p->bKerberos );
        case kKERBSERVICE:         R__PTR ( &p->kerberos_service );
#endif /* KERBEROS */
        case kLOCKCHECK:           R__INT ( &p->check_lock_refresh );
        case kCLEARPASS:           R__MNM ( &p->AllowClearText );
        case kREVLOOKUP:           R__INT ( &no_rev_lookup );
        case kSTATS:               R__BOO ( &p->bStats );
        case kSERVER:              R__BOO ( &p->server_mode );
        case kTRACENAME:           R__PTR ( &p->trace_name );
        case kTIMEOUT:             R__INT ( &pop_timeout );
        case kUSEROPTS:            R__BOO ( &p->bUser_opts );
        case kSPOOLOPTS:           R__BOO ( &p->bSpool_opts );
        case kFASTUPDATE:          R__BOO ( &p->bFast_update );
        case kTRIMDOMAIN:          R__BOO ( &p->bTrim_domain );
        case kTLS_SUPPORT:         R__MNM ( &p->tls_support );
        case kTLS_IDENT:           R__PTR ( &p->tls_identity_file );
        case kTLS_PASSPHRASE:      R__PTR ( &p->tls_passphrase );
        case kTLS_SERVCERTFILE:    R__PTR ( &p->tls_server_cert_file );
        case kTLS_PRIVKEYFILE:     R__PTR ( &p->tls_private_key_file );
        case kTLS_CIPHERLIST:      R__PTR ( &p->tls_cipher_list );
        case kTLS_VERSION:         R__MNM ( &p->tls_version );
        case kTLS_WORKAROUNDS:     R__INT ( &gTempVal );
        case kTLS_OPTIONS:         R__INT ( &gTempVal );
        case kDO_TIMING:           R__BOO ( &p->bDo_timing );
        case kCHECK_OLD_SPOOL_LOC: R__BOO ( &p->bCheck_old_spool_loc );
        case kCHECK_HASH_DIR:      R__BOO ( &p->bCheck_hash_dir );
        case kCHECK_PW_MAX:        R__BOO ( &p->bCheck_pw_max );
        case kUPDATE_STATUS_HDRS:  R__BOO ( &p->bUpdate_status_hdrs );
        case kUPDATE_ON_ABORT:     R__BOO ( &p->bUpdate_on_abort );
        case kAUTO_DELETE:         R__BOO ( &p->bAuto_delete );
        case kGROUP_BULLS:         R__BOO ( &p->bGroup_bulls );
        case kHASH_SPOOL:          R__INT ( &p->hash_spool );
        case kHOME_DIR_MAIL:       R__PTR ( &p->pHome_dir_mail );
        case kOLD_STYLE_UID:       R__BOO ( &p->bOld_style_uid );
        case kUW_KLUGE:            R__BOO ( &p->bUW_kluge );
        case kKEEP_TEMP_DROP:      R__BOO ( &p->bKeep_temp_drop );
        case kGRP_SERV_MODE:       R__PTR ( &p->grp_serv_mode );
        case kGRP_NO_SERV_MODE:    R__PTR ( &p->grp_no_serv_mode );
        case kAUTHFILE:            R__PTR ( &p->authfile );
        case kNONAUTHFILE:         R__PTR ( &p->nonauthfile );
        case kSHY:                 R__BOO ( &p->bShy );
        case kMAIL_COMMAND:        R__PTR ( &p->pMail_command );
        case kCFG_SPOOL_DIR:       R__PTR ( &p->pCfg_spool_dir );
        case kCFG_TEMP_DIR:        R__PTR ( &p->pCfg_temp_dir );
        case kCFG_TEMP_NAME:       R__PTR ( &p->pCfg_temp_name );
        case kCFG_CACHE_DIR:       R__PTR ( &p->pCfg_cache_dir );
        case kCFG_CACHE_NAME:      R__PTR ( &p->pCfg_cache_name );
        case kCHUNKY_WRITES:       R__MNM ( &p->chunky_writes );
        case kNO_ATOMIC_OPEN:      R__BOO ( &p->bNo_atomic_open );
        case kLOG_FACILITY:        R__MNM ( &p->log_facility );
        case kLOG_LOGIN:           R__PTR ( &p->pLog_login );
        case kMAXBULLS:            R__INT ( &p->nMaxBulls );

        default:                   R__PTR ( NULL );
    } /* switch ( item ) */

    return ret_val;
}


/*
 * Return length of string type token
 */
static long
get_string_length ( char **token, long *line_length, char *buffer )
{
    int len = 0;


    if ( char_is_quote ( *token, buffer ) )
    {
        len          = length_in_quoted_str ( *token, *line_length );
        *token       = *token + 1;
        *line_length = *line_length - 1;
    }
    else
        len = length_to_white_space ( *token, *line_length );

    return len;
}



/*
 * Return length of current token.
 */
static long
get_token_length ( char *token, long line_length )
{
    char *orig_ptr = token;


    if ( line_length > 0 && CharIsAlnum ( *token ) == FALSE )
        return 1; /* non-alphanumerics are their own token */

    while ( line_length > 0 && ( CharIsAlnum ( *token ) || *token == '-' ) )
    {
        token++;
        line_length--;
    }
    
    return ( token - orig_ptr );
}


/*
 * Skips past white space.
 */
static void
skip_white_space ( char **token, long *line_length )
{
    while ( *line_length > 0 && ( **token == ' ' || **token == '\t' ) )
    {
        *line_length   = *line_length   -1;
        *token         = *token         +1;
    }
}


/*
 * Skips to next token.
 */
static void
skip_token ( config_opt_type ttype, char **token, long *token_len, long *line_len )
{
    if ( ttype == CfgStr && ( **token == '\"' || 
                              **token == '`'  || 
                              **token != '\''   ) )
        *token_len = *token_len + 1; /* account for close quote */

    *token    += *token_len;
    *line_len -= *token_len;
    skip_white_space ( token, line_len );
    *token_len = get_token_length ( *token, *line_len );
}


/*
 * Writes debug entry showing an item set.
 */
static void
show_result ( POP *p, config_table *opt, int iVal, const char *sVal, WHENCE )
{
    switch ( opt->type )
    {
        case CfgBool:
            if ( p->debug )
                pop_log ( p, POP_DEBUG, fn, ln, "Set %s to %s",
                          opt->name,
                          iVal ? "true" : "false" );
            break;

        case CfgInt:
            if ( p->debug )
                pop_log ( p, POP_DEBUG, fn, ln, "Set %s to %d (%#0x)",
                          opt->name, iVal, iVal );
            break;

        case CfgMnem:
            if ( p->debug )
            {
                char *mnem = get_mnemonic ( opt, iVal );
                if ( mnem != NULL )
                    pop_log ( p, POP_DEBUG, fn, ln, "Set %s to %s (%d)",
                              opt->name, mnem, iVal );
                else
                    pop_log ( p, POP_DEBUG, fn, ln,
                              "Set %s to (unknown mnemonic) %d",
                              opt->name, iVal );
            }
            break;

        case CfgStr:
            if ( p->debug )
                pop_log ( p, POP_DEBUG, fn, ln, "Set %s to \"%.255s\"",
                          opt->name, sVal );
            break;

        default:
            /* we should never get here */
            pop_log ( p, POP_PRIORITY, fn, ln,
                      "** INTERNAL ERROR processing %s **",
                      opt->name );
            break;
    }
}


/*
 * Sets a value.
 */
static error_code_type
handle_value ( POP              *p, 
               config_table     *opt_ent,
               char             *pval,
               int               plen,
               config_call_type  CallTime )
{

    long              iVal       = 0;
    BOOL              bVal       = FALSE;
    config_opt_type   val_type   = CfgBad;
    opt_ptr_type      pOpt;


    /*
     * Resolve the value
     */
    if ( isdigit ( (int) *pval ) )
    {
        char *EndPtr = NULL;
        iVal     = strtol ( pval, &EndPtr, 0 );
        if ( iVal == LONG_MAX || iVal == LONG_MIN 
                              || ( iVal == 0 && errno == EINVAL ) )
        {
            pop_log ( p, POP_PRIORITY, HERE,
                      "Invalid value for %s: %.*s",
                      opt_ent->name, plen, pval );
            return kER_BAD_VAL;
        }
        val_type = CfgInt;
    }
    else
    if ( isalnum ( (int) *pval ) || ispunct ( (int) *pval )  )
    {
        val_type = CfgStr;
    }
    else
        return kER_BAD_VAL;

    /*
     * Handle special cases that need to be done before processing the
     * option.
     */
    switch ( opt_ent->value )
    {
#ifndef       BULLDB
        case kBULLDBNONFATAL:
        case kBULLDBMAXTRIES:
            pop_log ( p, POP_PRIORITY, HERE,
                      "BULLDB not set; can't set %s", opt_ent->name );
            return kER_NOT_COMPILED_WITH;
#endif /* not BULLDB */

#ifndef       DRAC_AUTH
        case kDRACHOST:
            pop_log ( p, POP_PRIORITY, HERE, 
                      "DRAC_AUTH not set; can't set %s", opt_ent->name ); 
            return kER_NOT_COMPILED_WITH;
#endif /* not DRAC_AUTH */

        case kCONFIGFILE: /* must fully handle it here */
            *(pval + plen) = '\0';
            if ( pop_config ( p, pval, CallTime ) == kER_UNKN_OPT )
                return kER_UNKN_OPT;
            else
                return kER_SUCCESS;
            break;

#ifndef KERBEROS
        case kKERBEROS:
        case kKERBSERVICE:
            pop_log ( p, POP_PRIORITY, HERE,
                      "KERBEROS not set; can't set %s", opt_ent->name );
            return kER_NOT_COMPILED_WITH;
#endif  /* KERBEROS */

        case kTRACENAME: /* must fully handle it here */
            p->debug = TRUE;
            if ( p->trace != NULL )
            {
                DEBUG_LOG2 ( p, "--- closing trace file; reopening "
                                "using \"%.*s\" ---",
                             plen, pval );
                fclose ( p->trace );
                p->trace = NULL;
            }

            * (pval + plen) = '\0';
            p->trace = fopen ( pval, "a+" );
            if ( p->trace == NULL )
                return kER_SYSTEM;
            p->trace_name = strdup ( pval );

            DEBUG_LOG1 ( p, "Trace and Debug destination is file \"%s\"",
                         p->trace_name );
            return kER_SUCCESS;

#ifndef       QPOP_SSL
        case kTLS_SUPPORT:
        case kTLS_CIPHERLIST:
        case kTLS_VERSION:
            return kER_NOT_COMPILED_WITH;
#endif /* not QPOP_SSL */

#ifndef       QPOP_SSLPLUS
        case kTLS_IDENT:
        case kTLS_PASSPHRASE:
            return kER_NOT_COMPILED_WITH;
#endif /* not QPOP_SSLPLUS */

#ifndef       QPOP_OPENSSL
        case kTLS_SERVCERTFILE:
        case kTLS_PRIVKEYFILE:
        case kTLS_OPTIONS:
            return kER_NOT_COMPILED_WITH;
#endif /* not QPOP_OPENSSL */

#if       ! ( defined ( QPOP_OPENSSL ) && defined ( SSL_OP_ALL ) )
        case kTLS_WORKAROUNDS:
            return kER_NOT_COMPILED_WITH;
#endif /* ! ( defined ( QPOP_OPENSSL ) && defined ( SSL_OP_ALL ) ) */

        default:
            break;
    } /* switch ( opt_ent->value ) */

    /*
     * Set the option
     */
    switch ( opt_ent->type )
    {
        case CfgBool:
            if ( val_type == CfgStr )
            {
                if ( equal_strings ( pval, plen, "true",  4 ) == TRUE )
                {
                    val_type = CfgInt;
                    iVal     = TRUE;
                }
                else
                if ( equal_strings ( pval, plen, "false", 5 ) == TRUE )
                {
                    val_type = CfgInt;
                    iVal     = FALSE;
                }
            }
            
            if ( val_type != CfgInt || iVal < 0 || iVal > 1 )
                return kER_EXP_BOOL;
            bVal = iVal;

            pOpt = get_opt_ptr ( p, opt_ent->value );
            if ( pOpt.pBoo == NULL )
                return kER_INTERNAL;

            *pOpt.pBoo = bVal;
            show_result ( p, opt_ent, *pOpt.pBoo, NULL, HERE );
            break;

        case CfgInt:
            if ( val_type != CfgInt )
                return kER_EXP_INT;

            pOpt = get_opt_ptr ( p, opt_ent->value );
            if ( pOpt.pInt == NULL )
                return kER_INTERNAL;

            *pOpt.pInt  = iVal;
            show_result ( p, opt_ent, *pOpt.pInt, NULL, HERE );
            break;

        case CfgMnem:
            if ( val_type != CfgStr )
                return kER_EXP_MNEM;

            iVal = find_mnemonic ( opt_ent, pval, plen );
            if ( iVal == -1 )
                return kER_UNKN_MNEM;

            pOpt = get_opt_ptr ( p, opt_ent->value );
            if ( pOpt.pInt == NULL )
                return kER_INTERNAL;

            *pOpt.pInt  = iVal;
            show_result ( p, opt_ent, *pOpt.pInt, NULL, HERE );
            break;

        case CfgStr:
            /* we'll take anything as a string */
            pOpt = get_opt_ptr ( p, opt_ent->value );
            if ( pOpt.pPtr == NULL )
                return kER_INTERNAL;

            *pOpt.pPtr = string_copy ( pval, plen );
            if ( *pOpt.pPtr == NULL )
            {
                pop_log ( p, POP_PRIORITY, HERE,
                          "Unable to allocate %d bytes", plen );
                return POP_FAILURE;
            }
            show_result ( p, opt_ent, 0, *pOpt.pPtr, HERE );
            break;

        default:
            /* we should never get here */
            pop_log ( p, POP_PRIORITY, HERE,
                      "Invalid option type for option \"%s\"", opt_ent->name );
            return kER_INTERNAL;
            break;
    } /* switch opt->type */

    /*
     * Handle any special cases that need to be done after processing the
     * option.
     */
    switch ( opt_ent->value )
    {
        case kDEBUG: /* If debug was reset, be sure to close the trace file if it is open */
            if ( p->debug == FALSE && p->trace != NULL )
            {
                p->debug = TRUE; /* so we can write a final record */
                DEBUG_LOG0 ( p, "--- resetting debug tracing ---" );
                fclose ( p->trace );
                p->trace = NULL;
                p->debug = FALSE;
            }
            break;

        case kLOG_FACILITY: /* Close and reopen the log to use new facility */
            closelog();
#ifdef SYSLOG42
            openlog ( p->myname, 0 );
#else
            openlog ( p->myname, POP_LOGOPTS, p->log_facility );
#endif /* SYSLOG42 */
            break;

#ifdef        QPOP_OPENSSL
        case kTLS_WORKAROUNDS: /* Set tls_options to SSL_OP_ALL */
            if ( gTempVal )
            {
                p->tls_options = SSL_OP_ALL;
                DEBUG_LOG1 ( p, "set tls-options to %#0x", p->tls_options );
            }
            break;

        case kTLS_OPTIONS: /* Add current value to options */
            p->tls_options += gTempVal;
            DEBUG_LOG2 ( p, "added %#0x to tls-options (now %#0x)",
                         gTempVal, p->tls_options );
            break;
#endif     /* QPOP_OPENSSL */

        default:
            break;
    } /* switch ( opt_ent->value ) */

    return kER_SUCCESS;
}


/*
 * Reads a config file and processes it.
 *
 * CallTime specifies the stage at which we are being called
 * and if the user has control over the file we are processing.
 */
pop_result
pop_config ( POP *p, char *config_file, config_call_type CallTime )
{
    FILE           *config     = NULL;
    size_t          line_no    = 1;
    char            buffer [ MAXLINELEN ] = "";
    char           *ptok       = NULL;
    long            line_len   = 0;
    long            tok_len    = 0;
    int             bReset     = FALSE;
    config_table   *opt        = NULL;
    pop_result      rslt       = POP_FAILURE;
    pop_result      err        = POP_FAILURE;
    static int      depth      = 0;


    DEBUG_LOG2 ( p, "Processing config file '%.256s'; CallTime=%d",
                 config_file, (int) CallTime );

    if ( depth++ > MAX_RECURSION )
    {
        tilt ( p, kER_RECURS, ptok, tok_len, line_no, config_file, opt, HERE );
        goto Exit;
    }

    config = fopen ( config_file, "r" );
    if ( config == NULL )
    {
        tilt ( p, kER_NOFILE, ptok, tok_len, line_no, config_file, opt, HERE );
        goto Exit;
    }

    ptok = fgets ( buffer, MAXLINELEN -1, config );
    while ( ptok != NULL )
    {
        line_len = strlen ( ptok );
        
        /*
         * Make sure line was not too long for buffer.  Also
         * get rid of trailing newline.
         */
        if ( buffer [ line_len -1 ] != '\n' )
        {
            tilt ( p, kER_LINE_LEN, ptok, tok_len, line_no, config_file, opt, HERE );
            goto Exit;
        }
        buffer [ --line_len ] = '\0';
        
        /*
         * Skip leading white space.
         */
        skip_white_space ( &ptok, &line_len );
        DEBUG_LOG3 ( p, "...read line %d (%lu): %.256s", 
                    line_no, line_len, ptok );
        
        /*
         * Ignore empty and comment lines.
         */
        if ( line_len != 0 && *ptok != COMMENT_CHAR )
        { /* process the line */

            /*
             * Get token length.
             */
            tok_len = get_token_length ( ptok, line_len );

            /*
             * There should be a "set" (or even "reset" if
             * the option is boolean) before the name.
             */
            if ( equal_strings ( ptok, tok_len, "reset", 5 ) == TRUE )
                bReset = TRUE;
            else
            if ( equal_strings ( ptok, tok_len, "set",   3 ) == TRUE )
                bReset = FALSE;
            else
            {
                tilt ( p, kER_EXP_SET, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }
            
            /*
             * Skip to next token.
             */
            skip_token ( CfgBad, &ptok, &tok_len, &line_len );

            /*
             * See if the option is known.
             */
            opt = find_option ( ptok, tok_len );
            if ( opt == NULL )
            {
                tilt ( p, kER_UNKN_OPT, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }
            
            /*
             * See if a restricted option is used in a user's file
             */
            if ( CallTime == CfgUser && opt->restriction == CfgResUser )
            {
                tilt ( p, kER_RESTR, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }

            /*
             * See if an init-only option is used after client traffic
             */
            if ( CallTime == CfgConnected && opt->restriction == CfgResInit )
            {
                tilt ( p, kER_CONNECTED, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }

            /*
             * See if 'reset' used with a non-boolean option.
             */
            if ( bReset == TRUE && opt->type != CfgBool )
            {
                tilt ( p, kER_NOT_BOOL, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }

            /*
             * See if there is an "=" followed by a value.
             */
            skip_token ( CfgBad, &ptok, &tok_len, &line_len );
            if ( ( tok_len == 0 || *ptok != '=' )
                 && opt->type != CfgBool )
            {
                tilt ( p, kER_EXP_EQL, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }

            if ( tok_len == 1 && *ptok == '=' )
            {
                /*
                 * Try and handle the value.
                 */
                skip_token ( CfgBad, &ptok, &tok_len, &line_len );
                if ( opt->type == CfgStr )
                    tok_len = get_string_length ( &ptok, &line_len, buffer );

                err = handle_value ( p, opt, ptok, tok_len, CallTime );
                if ( err != kER_SUCCESS )
                {
                    tilt ( p, err, ptok, tok_len, line_no, config_file, opt, HERE );
                    goto Exit;
                }
                skip_token ( opt->type, &ptok, &tok_len, &line_len ); /* consume the token */
            }
            else
            if ( tok_len == 0 || *ptok == COMMENT_CHAR )
            { /* implicit value */
                if ( bReset )
                    err = handle_value ( p, opt, "false", 5, CallTime );
                else
                    err = handle_value ( p, opt, "true",  4, CallTime );
                if ( err == POP_FAILURE )
                {
                    tilt ( p, kER_BAD_VAL, ptok, tok_len, line_no, config_file, opt, HERE );
                    goto Exit;
                }
            } /* implicit value */
            else
            {
                tilt ( p, kER_EXP_EQL, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }
            
            /*
             * Make sure there is no extra junk after the value
             */
            if ( tok_len != 0 && *ptok != COMMENT_CHAR )
            {
                tilt ( p, kER_EXP_EOL, ptok, tok_len, line_no, config_file, opt, HERE );
                goto Exit;
            }

        } /* process the line */

        /*
         * get the next line, if any
         */
        ptok = fgets ( buffer, MAXLINELEN -1, config );
        line_no++;
    } /* while ( ptok != NULL ) */
    
    rslt = POP_SUCCESS;

Exit:
    if ( rslt != POP_SUCCESS )
    { /* cleanup */
        if ( config != NULL )
        {
            fclose ( config );
            config = NULL;
        }
    } /* cleanup */

    DEBUG_LOG2 ( p, "Finished processing config file '%.256s'; rslt=%d",
                 config_file, rslt );


    return rslt;
}


/*
 * Process qpopper-options files, if requested and present.
 */
void
check_config_files ( POP *p, struct passwd *pwp )
{
    int         rslt;
    char        buf [ 256 ];
    struct stat stat_buf;


    if ( p->bUser_opts ) {
        rslt = Qsnprintf ( buf, sizeof(buf), "%s/.qpopper-options", pwp->pw_dir );
        if ( rslt == -1 )
            pop_log ( p, POP_PRIORITY, HERE, 
                      "Unable to build user options file name for user %s",
                      p->user );
        else {
            rslt = stat ( buf, &stat_buf );
            if ( rslt == 0 ) {
                rslt = pop_config ( p, buf, CfgUser );
                if ( rslt == POP_FAILURE ) {
                    pop_log ( p, POP_PRIORITY, HERE,
                              "Unable to process user options file for user %s",
                              p->user );
                }
            }
        }
    } /* p->user_opts */

    if ( p->bSpool_opts ) {
        rslt = Qsnprintf ( buf, sizeof(buf), "%s/.%s.qpopper-options",
                           p->pCfg_spool_dir, p->user );
        if ( rslt == -1 )
            pop_log ( p, POP_PRIORITY, HERE, 
                      "Unable to build spool options file name for user %s",
                      p->user );
        else {
            rslt = stat ( buf, &stat_buf );
            if ( rslt == 0 ) {
                rslt = pop_config ( p, buf, CfgConnected );
                if ( rslt == POP_FAILURE ) {
                    pop_log ( p, POP_PRIORITY, HERE,
                              "Unable to process spool options file for user %s",
                              p->user );
                }
            }
        }
    } /* p->spool_opts */
}


/*
 * To be called by pop_init to set option values.
 */
pop_result
set_option ( POP *p, char *opt_name, char *opt_value )
{
    config_table   *opt        = NULL;
    pop_result      rslt       = POP_FAILURE;
    pop_result      err        = POP_FAILURE;


    DEBUG_LOG4 ( p, "set_option; opt_name: %d '%s'; opt_value: %d '%s'",
                 strlen(opt_name), opt_name, strlen(opt_value), opt_value );

    /*
     * See if the option is known.
     */
    opt = find_option ( opt_name, strlen(opt_name) );
    if ( opt == NULL )
        goto Exit;

    err = handle_value ( p, opt, opt_value, strlen(opt_value), CfgInit );
    if ( err != kER_SUCCESS )
        goto Exit;

    rslt = POP_SUCCESS;

Exit:
    DEBUG_LOG1 ( p, "set_option returning %d", rslt );

    return rslt;
}
