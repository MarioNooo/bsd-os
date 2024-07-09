/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****
$Id: ifhp.h,v 1.14 2000/04/13 10:56:29 papowell Exp papowell $
 **** ENDHEADER ****/

#ifndef _IFHP_H_
#define _IFHP_H_ 1
#ifdef EXTERN
# undef EXTERN
# undef DEFINE
# define EXTERN
# define DEFINE(X) X
#else
# undef EXTERN
# undef DEFINE
# define EXTERN extern
# define DEFINE(X)
#endif

/*****************************************************************
 * get the portability information and configuration 
 *****************************************************************/

#include "portable.h"
#include "debug.h"
#include "errormsg.h"
#include "patchlevel.h"
#include "linelist.h"
#include "globmatch.h"
#include "open_device.h"
#include "plp_snprintf.h"
#include "stty.h"
#include "checkcode.h"
#include "accounting.h"

/*****************************************************************
 * Global variables and routines that will be common to all programs
 *****************************************************************/

/*****************************************************
 * Internationalisation of messages, using GNU gettext
 *****************************************************/

#if HAVE_LOCALE_H
# include <locale.h>
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#define N_(Text) Text

/*
 * data types and #defines
 */

/*
 * struct value - used to set and get values for variables
 */

struct keyvalue{
	char *varname;	/* variable name */
    char *key; /* name of the key */
    char **var; /* variable to set */
    int kind; /* type of variable */
#define INTV 0
#define STRV 1
#define FLGV 2
	char *defval;	/* default value, if any */
};

/*
 * dynamically controlled byte array for IO purposes 
 */
struct std_buffer {
	char *buf;		/* buffer */
	int end;		/* end of buffer */
	int start;		/* start of buffer */
	int max;		/* maximum size */
};

typedef void (*Wr_out)(char *);
typedef	int (*Builtin_func)(char *, char*, char *, Wr_out);

#define SMALLBUFFER 512
#define LARGEBUFFER 1024
#define OUTBUFFER 1024*10

/* get the character value at a char * location */

#define cval(x) (int)(*(unsigned char *)(x))
/* EXIT CODES */

#define JSUCC    0     /* done */
#define JFAIL    32    /* failed - retry later */
#define JABORT   33    /* aborted - do not try again, but keep job */
#define JREMOVE  34    /* failed - remove job */
#define JACTIVE  35    /* active server - try later */
#define JIGNORE  36    /* ignore this job */
/* from 1 - 31 are signal terminations */

/*
 * Constant Strings
 */

EXTERN char *UNKNOWN DEFINE( = "UNKNOWN");
EXTERN char *PCL DEFINE( = "PCL");
EXTERN char *PS DEFINE( = "POSTSCRIPT");
EXTERN char *TEXT DEFINE( = "TEXT");
EXTERN char *RAW DEFINE( = "RAW");
EXTERN char *PJL DEFINE( = "PJL");

EXTERN char *Value_sep DEFINE( = " \t=#@" );
EXTERN char *Whitespace DEFINE( = " \t\n\f" );
EXTERN char *List_sep DEFINE( = "[] \t\n\f" );
EXTERN char *Linespace DEFINE( = " \t" );
EXTERN char *Filesep DEFINE( = " \t,;" );
EXTERN char *Argsep DEFINE( = ",;" );
EXTERN char *Namesep DEFINE( = "|:" );
EXTERN char *Line_ends DEFINE( = "\n\014\004\024" );


#define GLYPHSIZE 15
 struct glyph{
    int ch, x, y;   /* baseline location relative to x and y position */
    char bits[GLYPHSIZE];
};

 struct font{
    int height; /* height from top to bottom */
    int width;  /* width in pixels */
    int above;  /* max height above baseline */
    struct glyph *glyph;    /* glyphs */
};

/*
 * setup values
 */

EXTERN struct line_list Zopts, Topts, Raw, Model,
	Devstatus, Pjl_only, Pjl_except,	/* option variables */
	Pjl_vars_set, Pjl_vars_except,
	Pcl_vars_set, Pcl_vars_except,
	User_opts, Pjl_user_opts, Pcl_user_opts, Ps_user_opts,
	Pjl_error_codes, Pjl_quiet_codes;
EXTERN char *Loweropts[26];	/* lower case options */
EXTERN char *Upperopts[26];	/* upper case options */
EXTERN char **Envp;			/* environment variables */
EXTERN char **Argv;			/* parms variables */
EXTERN int Argc;			/* we have the number of variables */
EXTERN int Monitor_fd DEFINE( = -1 );	/* for monitoring */

EXTERN time_t Start_time;	/* start time of program */

EXTERN int
	Appsocket,	/* accounting fd */
	Qms,	/* QMS printer */
	Accounting_fd,	/* accounting fd */
	Autodetect,	/* let printer autodetect type */
	Crlf,		/* only do CRLF */
	Dev_retries,	/* number of retries on open */
	Dev_sleep DEFINE(=1000),	/* wait between retries in Millisec */
	Errorcode,		/* exit value */
	Force_status,	/* even if device is not socket or tty, allow status */
	Force_conversion,	/* force conversion by utility */
	Full_time,		/* use Full_time format */
	Initial_timeout,	/* initial timeout on first write */
	Ignore_eof,			/* ignore eof on input */
	Job_timeout,	/* timeout for job */
	Logall,		/* log all information back from printer */
	Max_status_size DEFINE(=8),	/* status file max size */
	Min_status_size DEFINE(=2),	/* status file min size */
	No_PS_EOJ,			/* no PS eoj */
	No_PCL_EOJ,			/* no PS eoj */
	Null_pad_count,		/* null padding on PJL ENTER command */
	No_udp_monitor,		/* do not use udp monitor */
	OF_Mode,			/* running in OF mode */
	Pagecount_interval,	/* pagecount polling interval */
	Pagecount_poll,	/* pagecount polling interval */
	Pagecount_timeout,	/* pagecount */
	Pcl,		/* has PCL support */
	Pjl,		/* has PJL support */
	Pjl_console,	/* use the PJL Console */
	Pjl_enter,	/* use the PJL ENTER command */
	Reopen_for_job,	/* open the device read/write */
	Read_write,	/* open the device read/write */
	Ps,			/* has PostScript support */
	Psonly,	/* only recognizes PostScript */
	Quiet,		/* suppress printer status messages */
	Status,		/* any type of status - off = write only */
	Status_fd DEFINE(=-2),	/* status reporting to remote site */
	Sync_interval,	/* sync interval */
	Sync_timeout,	/* sync timeout */
	Tbcp,		/* supports Postscript TBCP */
	Text,		/* supports test files */
	Trace_on_stderr, /* puts out trace on stderr as well */
	Ustatus_on, /* Ustatus was sent, need to send USTATUSOFF */
	Waitend_ctrl_t_interval,	/* wait between sending CTRL T */
	Waitend_interval;	/* wait between sending end status requests */

EXTERN char 
	*Accountfile,		/* accounting file */
	*Accounting_script,	/* accounting script to use */
	*Config_file,	/* config file list */
	*Device,		/* device to open */
	*End_ctrl_t,	/* end status indications from PostScript ctrl_T status */
	*Model_id,		/* printer model */
	*Pagecount,		/* pagecount */
	*Name,			/* program name */
	*Remove_ctrl,	/* remove these control chars */
	*Statusfile,	/* status file */
	*Stty_args,		/* if device is tty, stty values */
	*Sync,			/* synchronize printer */
	*Waitend;		/* wait for end using sync */

/*
 * set by routines
 */
EXTERN char
	*Ps_pagecount_code,		/* how to do pagecount */
	*Ps_status_code;		/* how to get status */

extern struct keyvalue Valuelist[], Builtin_values[];

#if defined DMALLOC
#	include <dmalloc.h>
#endif

/* PROTOTYPES */
void cleanup(int sig);
void getargs( int argc, char **argv );
void Fix_special_user_opts( struct line_list *opts, char *line );
void Init_outbuf();
void Put_outbuf_str( char *s );
void Put_outbuf_len( char *s, int len );
void Init_inbuf();
void Put_inbuf_str( char *str );
void Put_inbuf_len( char *str, int len );
char *Get_inbuf_str();
void Init_monitorbuf();
void Put_monitorbuf_str( char *str );
void Put_monitorbuf_len( char *str, int len );
char *Get_monitorbuf_str();
void Pr_status( char *str, int monitor_status );
void Check_device_status( char *line );
void Initialize_parms( struct line_list *list, struct keyvalue *valuelist );
void Dump_parms( char *title, struct keyvalue *v );
void Process_job();
void Put_pjl( char *s );
void Put_pcl( char *s );
void Put_ps( char *s );
void Put_fixed( char *s );
int Get_nonblock_io( int fd );
int Set_nonblock_io( int fd );
int Set_block_io( int fd );
int Read_write_timeout( int readfd, int *flag,
	int writefd, char *buffer, int len, int timeout,
	int monitorfd );
int Read_status_line( int fd );
int Read_monitor_status_line( int fd );
int Write_out_buffer( int outlen, char *outbuf, int maxtimeout );
void Resolve_key_val( char *prefix, char *key_val, Wr_out routine );
int Is_flag( char *s, int *v );
void Resolve_list( char *prefix, struct line_list *l, Wr_out routine );
void Resolve_user_opts( char *prefix, struct line_list *only,
	struct line_list *l, Wr_out routine );
char *Fix_option_str( char *str, int remove_ws, int trim, int one_line );
char *Find_sub_value( int c, char *id, int strval );
int Builtin( char* prefix, char *id, char *value, Wr_out routine);
int Font_download( char* prefix, char *id, char *value, Wr_out routine);
void Pjl_job();
void Pjl_eoj();
void Pjl_console_msg( int start );
int Pjl_setvar(char *prefix, char*id, char *value, Wr_out routine);
int Pcl_setvar(char *prefix, char*id, char *value, Wr_out routine );
void Do_sync( int sync_timeout );
void Do_waitend( int waitend_timeout );
int Do_pagecount( int pagecount_timeout );
int Read_until_eof( int max_timeout );
int Current_pagecount( int pagecount_timeout, int use_pjl, int use_ps );
void Send_job();
void URL_decode( char *s );
int Process_OF_mode( int job_timeout );
void Add_val_to_list( struct line_list *v, int c, char *key, char *sep );
int Get_max_fd( void );
void close_on_exec( int n );
void Use_file_util(char *pgm, struct line_list *result);
int Make_tempfile( char **retval );
void Make_stdin_file();
char * Set_mode_lang( char *s );
int Fd_readable( int fd );
void Init_job( char *language );
void Term_job( char *mode );
void Text_banner(void);
void do_char( struct font *font, struct glyph *glyph,
	char *str, int line );
int bigfont( struct font *font, char *line, struct line_list *l, int start );
const char *Decode_status (plp_status_t *status);
int plp_usleep( int i );
void Strip_leading_spaces ( char **vp );

#endif
