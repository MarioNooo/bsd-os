/* 
 * poppassd.c
 *
 * A change-password server.
 *
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file license.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *   05/06/01 [RCG]
 *      - Changed UID check to be the same as Qpopper's (which is
 *        that if BLOCK_UID is defined we use that value, otherwise
 *        it defaults to 10).
 *      - Added expect strings for DEC True 64 (sent in by Andres Henckens).
 *
 *   03/15/01 [RCG]
 *      - Integrated into Qpopper build.
 *      - Added debug tracing (-t) option.
 *      - Fixed strings for some Solaris versions.
 *      - Now handles passwd binaries that quietly exit.
 *      - Version number now matches that of Qpopper.
 *      - Added -y option to specify log facility.
 *      - -p and -s options can now specify binaries to execute.
 *      - Added -R option to prevent reverse-lookups.
 *      - Added -l option to specify SSL/TLS handling.
 *
 */


/*
 * John Norstad
 * Academic Computing and Network Services
 * Northwestern University
 * j-norstad@nwu.edu
 *
 * Based on earlier versions by Roy Smith <roy@nyu.edu> and Daniel
 * L. Leavitt <dll.mitre.org>.
 * 
 * Doesn't actually change any passwords itself.  It simply listens for
 * incoming requests, gathers the required information (user name, old
 * password, new password) and executes /bin/passwd, talking to it over
 * a pseudo-terminal pair.  The advantage of this is that we don't need
 * to have any knowledge of either the password file format (which may
 * include dbx files that need to be rebuilt) or of any file locking
 * protocol /bin/passwd and cohorts may use (and which isn't documented).
 *
 * The current version has been tested at NU under SunOS release 4.1.2 
 * and 4.1.3, and under HP-UX 8.02 and 9.01. We have tested the server 
 * with both Eudora 1.3.1 and NUPOP 2.0.
 *
 * Other sites report that this version also works under AIX and NIS,
 * and with PC Eudora.
 *
 * Note that unencrypted passwords are transmitted over the network.  If
 * this bothers you, think hard about whether you want to implement the
 * password changing feature.  On the other hand, it's no worse than what
 * happens when you run /bin/passwd while connected via telnet or rlogin.
 * Well, maybe it is, since the use of a dedicated port makes it slightly
 * easier for a network snooper to snarf passwords off the wire.
 *
 * NOTE: In addition to the security issue outlined in the above paragraph,
 * you should be aware that this program is going to be run as root by
 * ordinary users and it mucks around with the password file.  This should
 * set alarms off in your head.  I think I've devised a pretty foolproof
 * way to ensure that security is maintained, but I'm no security expert and
 * you would be a fool to install this without first reading the code and
 * ensuring yourself that what I consider safe is good enough for you.  If
 * something goes wrong, it's your fault, not mine.
 *
 * The front-end code (which talks to the client) is directly 
 * descended from Leavitt's original version.  The back-end pseudo-tty stuff 
 * (which talks to /bin/password) is directly descended from Smith's
 * version, with changes for SunOS and HP-UX by Norstad (with help from
 * sample code in "Advanced Programming in the UNIX Environment"
 * by W. Richard Stevens). The code to report /bin/passwd error messages
 * back to the client in the final 500 response, and a new version of the
 * code to find the next free pty, is by Norstad.
 *        
 * Should be owned by root, and executable only by root.  It can be started
 * with an entry in /etc/inetd.conf such as the following:
 *
 * epass stream tcp nowait root /usr/sbin/tcpd poppassd
 * 
 * and in /etc/services:
 * 
 * epass        106/tcp poppassd
 *
 * Logs to the local2 facility. Should have an entry in /etc/syslog.conf
 * like the following:
 *
 * local2.err   /var/adm/poppassd-log
 */
 
/* Modification history.
 *
 * 06/09/93. Version 1.0.
 *
 * 06/29/93. Version 1.1.
 * Include program name 'poppassd' and version number in initial 
 *    hello message.
 * Case insensitive command keywords (user, pass, newpass, quit).
 *    Fixes problem reported by Raoul Schaffner with PC Eudora.
 * Read 'quit' command from client instead of just terminating after 
 *    password change.
 * Add new code for NIS support (contributed by Max Caines).
 *
 * 08/31/93. Version 1.2.
 * Generalized the expected string matching to solve several problems
 *    with NIS and AIX. The new "*" character in pattern strings
 *    matches any sequence of 0 or more characters.
 * Fix an error in the "getemess" function which could cause the
 *    program to hang if more than one string was defined in the
 *    P2 array.
 *
 * 08/27/97 V 1.2.01-Linux (Alan Brown, alan@manawatu.gen.nz)
 * Fixed prompt strings to work with Linux shadowed passwd program
 *   supplied with Slackware 3.3 
 * Removed need to run passwd as root, because this version of passwd
 *   doesn't have the same limitations as the SunOs one.
 * Removed several response test bypasses for passwd, as this version
 *   is fairly verbose and behaves much like the non-shadowed one.
 * Added test for UIDs below 1000 to reject changing of system
 *   passwords.
 * Got handling of MD5 crypts sorted out (makefile -lshadow addition,
 *   removal of crypt and pw_encrypt #defines)
 *
 * 09/09/97 - Oliver Jowett <oliver@jowett.manawatu.planet.co.nz>
 * smbpasswd support, added -s (run smbpasswd) and -p (run passwd) switches
 *   - defaults to -p. Lots of code moved to runchild.
 * Tweaks to the expect strings to pass error messages back correctly
 * Cleanups to compile clean with -Wall
 * Length restriction on passwords
 *
 * TODO: Lots! - 
 * Such as allowing passwd length restriction as a runtime parameter
 * 
 */

/* Steve Dorner's description of the simple protocol:
 *
 * The server's responses should be like an FTP server's responses; 
 * 1xx for in progress, 2xx for success, 3xx for more information
 * needed, 4xx for temporary failure, and 5xx for permanent failure.  
 * Putting it all together, here's a sample conversation:
 *
 *   S: 200 hello\r\n
 *   E: user yourloginname\r\n
 *   S: 300 please send your password now\r\n
 *   E: pass yourcurrentpassword\r\n
 *   S: 200 My, that was tasty\r\n
 *   E: newpass yournewpassword\r\n
 *   S: 200 Happy to oblige\r\n
 *   E: quit\r\n
 *   S: 200 Bye-bye\r\n
 *   S: <closes connection>
 *   E: <closes connection>
 */


#define SUCCESS 1
#define FAILURE 0
#define BUFSIZE 512

#define PASSWD_LENGTH 11 /* max length of passwords to accept */

/* LANMAN allows up to 14 char passwords (truncates if longer), but tacacs
   only seems to allow 11. */

#define PASSWD_BINARY "/usr/bin/passwd"         /* TBD: config.h */
#define SMBPASSWD_BINARY "/usr/bin/smbpasswd"   /* TBD: config.h */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#if HAVE_SYS_NETINET_IN_H
#  include <sys/netinet/in.h>
#endif

#if HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif

#include <netdb.h>
#include <arpa/inet.h>

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_SYS_FCNTL_H
#  include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

#ifdef HAVE_SYSLOG_H
#  include <syslog.h>
#endif /* HAVE_SYSLOG_H */

#ifdef HAVE_SYS_SYSLOG_H
#  include <sys/syslog.h>
#endif /* HAVE_SYS_SYSLOG_H */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <errno.h>
#include <stdarg.h>
#include <pwd.h>
#include <string.h>
#include <termios.h>
#include <dirent.h>
/* #include <getopt.h> */

#ifdef    HAVE_SHADOW_H
#  include <shadow.h>
#  ifndef   PW_PPP
#    define PW_PPP PW_LOGIN
#  endif /* PW_PPP */
#endif /* HAVE_SHADOW_H */

#ifndef   BLOCK_UID
#  define BLOCK_UID   10 /* UID's <= this value are not allowed to access email */
#endif /* BLOCK_UID */

#include "popper.h"
#include "logit.h"
#include "snprintf.h"


/*
 * Local Prototypes
 */
int     main (int argc, char *argv[]);
int     dochild (int master, char *slavedev, char *user, int smb);
int     findpty (char **slave);
void    writestring (int fd, char *s);
int     talktochild (int master, char *user, char *oldpass, char *newpass,
                 char *emess, int asroot);
int     match (char *str, char *pat);
int     expect (int master, char **expected, char *buf);
void    getemess (int master, char **expected, char *buf);
void    WriteToClient (char *fmt, ...);
void    ReadFromClient (char *line);
int     chkPass (char *user, char *pass, struct passwd *pw, POP *p);
void    runchild (char *user, char *oldpass, char *newpass, int smb);
void    msg      ( WHENCE, const char *format, ... );
void    err_msg  ( WHENCE, const char *format, ... );
void    err_dump ( WHENCE, const char *format, ... );
void    my_perror   ( void );
char   *sys_err_str ( void );
void    get_client_info ( POP *p, BOOL no_rev_lookup );
char   *debug_str ( char *p, int inLen, int order );


/*
 * External prototypes
 */
pop_result auth_user ( POP *p, char *password, struct passwd *pw,
                       char *err_buf, int err_len );


/* Prompt strings expected from the "passwd" command. If you want
 * to port this program to yet another flavor of UNIX, you may need to add
 * more prompt strings here.
 *
 * Each prompt is defined as an array of pointers to alternate 
 * strings, terminated by an empty string. In the strings, '*'
 * matches any sequence of 0 or more characters. Pattern matching
 * is forced to lower case so enter only lower case letters.
 */

static char *P1[] =
   {
     "changing password for *\nold password: ",  /* shadow */
     "enter login password: ",                   /* Solaris */
     "old smb password: ",                       /* smb */
     ""
   };

static char *P2[] =
   {
     "new password:",                            /* needed to get error msgs */
     "new password: ",                           /* DEC True 64 */
     "*\n*\nnew password: ",                     /* shadow */
     "new password: ",
     "new smb password: ",                       /* smb */
     ""
   };

static char *P3[] =
   {
     "re-enter new password:*",                  /* shadow */
     "re-enter new password: ",
     "retype new password: ",                    /* DEC True 64 */
     "retype new smb password: ",                /* smb */
     ""
   };
    
static char *P4[] =
   {
     "password changed. ", /* shadow */
     "password changed ",  /* smb */
     ""
   };


BOOL           verbose              = FALSE;
struct passwd *pw                   = NULL;
char           userid [ BUFSIZE ]   = "";
char          *pname                = NULL;
char          *trace_name           = NULL;
FILE          *trace_file           = NULL;
char           msg_buf [ 2048 ]     = "";
char          *pwd_binary           = PASSWD_BINARY;
char          *smb_binary           = SMBPASSWD_BINARY;


/*
 * Be careful using TRACE in an 'if' statement!
 */
#define TRACE if ( verbose ) logit

#define RUN_PASSWD 1
#define RUN_SMBPASSWD 2


int main ( int argc, char *argv[] )
{
    char           line    [BUFSIZE]   = "";
    char           oldpass [BUFSIZE]   = "";
    char           newpass [BUFSIZE]   = "";
    int            nopt                = -1;
    static char    options []          = "dl:p:Rs:t:vy:?";
    int            mode                = 0;
    char          *ptr                 = NULL;
    POP            p;
    BOOL           no_rev_lookup       = FALSE;

#ifdef HAS_SHADOW
    struct spwd *spwd;
    struct spwd *getspnam();
#endif


    pname = argv [ 0 ];
    if ( pname == NULL || *pname == '\0' )
        pname = "poppassd";
    else
    {
        ptr = strrchr ( pname, '/' );
        if ( ptr != NULL && strlen(ptr) > 1 )
            pname = ptr + 1;
    }

    openlog ( pname, POP_LOGOPTS, LOG_LOCAL2 );

    /*
     * Set up some stuff in -p- so we can call Qpopper routines
     */
    bzero ( (char *) &p, sizeof ( POP ) );
    p.AuthType            = noauth;
    p.myname              = pname;

    /*
     * Handle command-line options
     */
    nopt = getopt ( argc, argv, options );
    while ( nopt != -1 )
    {
        switch (nopt)
        {
            case '?':
                fprintf ( stderr, "%s [-?] [-d] [-l 0|1|2] [-p [passd-path]] "
                                  "[-R] [-s [smbpasswd-path]]\n\t"
                                  "[-t trace-file] [-v] [-y log-facility]\n",
                          pname );
                exit (1);

           case 'v':
                printf ( "poppassd%.*s%s version %s\n",
                         (strlen(BANNERSFX)>0 ? 1 : 0), " ",
                         BANNERSFX,
                         VERSION );
                
                exit (0);

           case 'd':
                verbose = TRUE;
                break;

           case 's':
                mode |= RUN_SMBPASSWD;
                if ( optarg != NULL && *optarg != '\0' )
                    smb_binary = optarg;
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "Changing SMB passwords using %s", smb_binary );
                break;

           case 'p':
                mode |= RUN_PASSWD;
                if ( optarg != NULL && *optarg != '\0' )
                    pwd_binary = optarg;
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "Changing standard passwords using %s", pwd_binary );
                break;

           case 't':
                verbose    = TRUE;
                trace_name = strdup ( optarg );
                trace_file = fopen ( optarg, "a" );
                if ( trace_file == NULL )
                {
                    err_msg ( HERE, "Unable to open trace file \"%s\"",
                              trace_name );
                    return 1;
                }
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "Opened trace file \"%s\" as %d",
                        trace_name, fileno(trace_file) );
                break;

            case 'l': /* TLS/SSL handling */
                if ( optarg != NULL && *optarg != '\0' ) {
                    switch ( atoi(optarg) ) {
                        case 0:
                            p.tls_support = QPOP_TLS_NONE;
                            break;
                        case 1:
                            p.tls_support = QPOP_TLS_STLS;
                            break;
                        case 2:
                            p.tls_support = QPOP_TLS_ALT_PORT;
                            break;
                        default:
                            err_msg ( HERE, "Unrecognized -l value; 0 = default; "
                                             "1 = stls; 2 = alternate-port" );
                            return 1;
                    }
                    msg ( HERE, "ssl/tls not yet implemented" );
                } else {
                    err_msg ( HERE, "-l value expected; 0 = default; "
                                    "1 = stls; 2 = alternate-port" );
                    return 1;
                }
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "tls-support=%d (-l)", atoi(optarg) );
                break;

            case 'R': /*  Omit reverse lookups on client IP */
                no_rev_lookup = TRUE;
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "Avoiding reverse lookups (-R)" );
                break;

            case 'y': /* log facility */
                if ( optarg == NULL || *optarg == '\0' ) {
                    err_msg ( HERE, "-y value expected" );
                    exit (1);
                }
                if ( !strncasecmp("mail", optarg, 4) )
                    p.log_facility = LOG_MAIL;
                else if ( !strncasecmp("local", optarg, 5)
                          && strlen(optarg) == 6 )
                {
                    switch ( *(optarg+5) )
                    {
                        case '0':
                            p.log_facility = LOG_LOCAL0;
                            break;
                        case '1':
                            p.log_facility = LOG_LOCAL1;
                            break;
                        case '2':
                            p.log_facility = LOG_LOCAL2;
                            break;
                        case '3':
                            p.log_facility = LOG_LOCAL3;
                            break;
                        case '4':
                            p.log_facility = LOG_LOCAL4;
                            break;
                        case '5':
                            p.log_facility = LOG_LOCAL5;
                            break;
                        case '6':
                            p.log_facility = LOG_LOCAL6;
                            break;
                        case '7':
                            p.log_facility = LOG_LOCAL7;
                            break;
                        default:
                            err_msg ( HERE, "Error setting '-y' to %s", optarg );
                            return 1;
                    } /* switch */
                } else
                {
                    err_msg ( HERE, "Error setting '-y' to %s", optarg );
                    return 1;
                }

                TRACE ( trace_file, POP_DEBUG, HERE,
                        "...closing log to switch to facility %s",
                        optarg );
                closelog();
                openlog ( pname, POP_LOGOPTS, p.log_facility );
                TRACE ( trace_file, POP_DEBUG, HERE,
                        "Now logging using facility %s",
                        optarg );
                break;
        }
        nopt = getopt ( argc, argv, options );
    }

    if ( mode == 0 )
        mode = RUN_PASSWD;

    gethostname ( line, sizeof (line) );
    p.myhost = strdup ( line );
    get_client_info ( &p, no_rev_lookup );

    WriteToClient ( "200 %s poppassd v%s hello, who are you?", line, VERSION );

    ReadFromClient ( line );
    sscanf ( line, "user %s", userid ) ;
    if ( strlen (userid) == 0 )
    {
        WriteToClient ( "500 Username required." );
        return 1;
    }
    strlcpy ( p.user, userid, sizeof(p.user) );

    WriteToClient ( "200 your password please." ); /* 300 surely? */
    ReadFromClient ( line );
    sscanf ( line, "pass %s", oldpass );

    if ( strlen (oldpass) == 0 )
    {
        WriteToClient ( "500 Password required." );
        return 1;
    }

    WriteToClient ( "200 your new password please." );
    ReadFromClient ( line );
    sscanf ( line, "newpass %s", newpass );
     
    /* new pass required */
    if ( strlen (newpass) == 0 )
    {
        WriteToClient ("500 New password required.");
        return 1;
    }

    pw = getpwnam ( userid );
    if ( pw == NULL )
    {
        WriteToClient ( "500 Invalid user or password" );
        return 1;
    }

#ifdef HAS_SHADOW
    if ((spwd = getspnam(userid)) == NULL)
          pw->pw_passwd = "";
    else
          pw->pw_passwd = spwd->sp_pwdp;
#endif


    if ( chkPass ( userid, oldpass, pw, &p ) == FAILURE )
    {
        syslog ( LOG_ERR, "password failure for %s", userid );
        WriteToClient ( "500 Invalid user or password" );
        return 1;
    }

    if ( pw->pw_uid <= BLOCK_UID )
 
    {
        syslog ( LOG_ERR, "someone tried to change %s's password", userid );
        WriteToClient ( "500 Not a user account." );
        return 1;
    }

    if ( strlen (newpass) > PASSWD_LENGTH )
    {
        WriteToClient ( "500 New password is too long; limited to %d characters", PASSWD_LENGTH);
        return 1;
    }
         
    if ( mode & RUN_PASSWD )
        runchild ( userid, oldpass, newpass, 0 );

    if (mode & RUN_SMBPASSWD)
        runchild ( userid, oldpass, newpass, 1 );

    /* all done */

    syslog ( LOG_ERR, "password changed for %s", userid );
    WriteToClient ( "200 Password changed, thank-you." );

    ReadFromClient ( line );
    if ( strncmp(line, "quit", 4) != 0 )
    {
        WriteToClient ( "500 Quit required." );
        return 1;
    }

    WriteToClient ( "200 Bye." );
    return 0;
}


/* Run a child process to do the password change */

void runchild ( char *userid, char *oldpass, char *newpass, int smb )
{
    int     master;
    char   *slavedev;
    char    emess[BUFSIZE];
    int     wstat;
    pid_t   pid, wpid;
 
    /* get pty to talk to password program */
    master = findpty ( &slavedev );
    if ( master < 0 )
    {
        syslog ( LOG_ERR, "can't find pty" );
        WriteToClient ( "400 Server busy -- try again later." );
        exit ( 1 );
    }

  /* fork child process to talk to password program */

  pid = fork();
  if ( pid < 0 )     /* Error, can't fork */
  {
    err_msg ( HERE, "can't fork: %d", pid );
    WriteToClient ("400 Server error (can't fork), get help!");
    exit(1);
  }

  if ( pid > 0 )   /* Parent */
  {
    if (talktochild (master, userid, oldpass, newpass, emess, smb) == FAILURE)
    {
      logit ( trace_file, LOG_ERR, HERE, 
              "%s failed for %s", smb ? "smbpasswd" : "passwd", userid );
      WriteToClient ("500 %s",
                     emess[0] ? emess : "Unable to change password");
      exit(1);
    }

    wpid = waitpid ( pid, &wstat, 0 );
    if ( wpid < 0 )
    {
      logit ( trace_file, LOG_ERR, HERE, "wait for child failed" );
      WriteToClient ("500 Server error (wait failed), get help!");
      exit(1);
    }

    if ( pid != wpid )
    {
      logit ( trace_file, LOG_ERR, HERE, "wrong child!" );
      WriteToClient ("500 Server error (wrong child), get help!");
      exit(1);
    }

    if ( WIFEXITED (wstat) == 0 )
    {
      logit ( trace_file, LOG_ERR, HERE, "child killed?" );
      WriteToClient ("500 Server error (funny wstat), get help!");
      exit(1);
    }

    if ( WEXITSTATUS (wstat) != 0 )
    {
      logit ( trace_file, LOG_ERR, HERE, "child exited abnormally" );
      WriteToClient ("500 Server error (abnormal exit), get help!");
      exit(1);
    }

    close ( master ); /* done with the pty */
  }
  else      /* Child */
  {
    dochild ( master, slavedev, userid, smb );
  }
}

/*
 * dochild
 *
 * Do child stuff - set up slave pty and execl as needed
 *
 * Code adapted from "Advanced Programming in the UNIX Environment"
 * by W. Richard Stevens.
 *
 */

int dochild (int master, char *slavedev, char *userid, int smb)
{
   int slave;
   struct termios stermios;

   /* Start new session - gets rid of controlling terminal. */
   
   if (setsid() < 0) {
      err_msg ( HERE, "setsid failed" );
      return(0);
   }

   /* Open slave pty and acquire as new controlling terminal. */

   if ((slave = open(slavedev, O_RDWR)) < 0) {
      err_msg ( HERE, "can't open slave pty" );
      return(0);
   }

   /* Close master. */

   close(master);

   /* Make slave stdin/out/err of child. */

   if (dup2(slave, STDIN_FILENO) != STDIN_FILENO) {
      err_msg ( HERE, "dup2 error to stdin" );
      return(0);
   }

   if (dup2(slave, STDOUT_FILENO) != STDOUT_FILENO) {
      err_msg ( HERE, "dup2 error to stdout" );
      return(0);
   }

   if (dup2(slave, STDERR_FILENO) != STDERR_FILENO) {
      err_msg ( HERE, "dup2 error to stderr" );
      return(0);
   }
   if (slave > 2) close(slave);

   /* Set proper terminal attributes - no echo, canonical input processing,
      no map NL to CR/NL on output. */

   if (tcgetattr(0, &stermios) < 0) {
      err_msg( HERE, "tcgetattr error" );
      return(0);
   }

   stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
   stermios.c_lflag |= ICANON;
   stermios.c_oflag &= ~(ONLCR);
   if (tcsetattr(0, TCSANOW, &stermios) < 0) {
      err_msg ( HERE, "tcsetattr error" );
      return(0);
   }

   /* Do some simple changes to ensure that the daemon does not mess */
   /* things up. */

   chdir ("/");
   umask (0);

/*
 * Become the user and run passwd. Linux shadowed passwd doesn't need
 * to be run as root with the username passed on the command line.
 *
 * If we're changing the samba password, stay as root and force the change
 * so it's always in sync.
 */
   if (!smb)
   {
     TRACE ( trace_file, POP_DEBUG, HERE, "...changing standard password" );
     setregid ( pw->pw_gid, pw->pw_gid );
     setreuid ( pw->pw_uid, pw->pw_uid );

     execl ( pwd_binary, "passwd", NULL );

     err_msg ( HERE, "can't exec %s", pwd_binary );
     exit ( 1 );
   }
   else
   {
     TRACE ( trace_file, POP_DEBUG, HERE, "...changing smb password" );
     execl ( smb_binary, "smbpasswd", userid, NULL );

     err_msg ( HERE, "can't exec %s", smb_binary );
     exit ( 1 );
   }
}


/*
 * findpty()
 *
 * Finds the first available pseudo-terminal master/slave pair.  The master
 * side is opened and a fd returned as the function value.  A pointer to the
 * name of the slave side (i.e. "/dev/ttyp0") is returned in the argument,
 * which should be a char**.  The name itself is stored in a static buffer.
 *
 * A negative value is returned on any sort of error.
 *
 * Modified by Norstad to remove assumptions about number of pty's allocated
 * on this UNIX box.
 */

int findpty (char **slave)
{
   int master;
   static char line[11];
   DIR *dirp;
   struct dirent *dp;

   strcpy(line, "/dev/ptyXX");
   
   dirp = opendir("/dev");
   while ((dp = readdir(dirp)) != NULL) {
      if (strncmp(dp->d_name, "pty", 3) == 0 && strlen(dp->d_name) == 5) {
         line[8] = dp->d_name[3];
         line[9] = dp->d_name[4];
         if ((master = open(line, O_RDWR)) >= 0) {
            line[5] = 't';
            *slave = line;
            closedir(dirp);
            TRACE ( trace_file, POP_DEBUG, HERE, "findpty found %d", master );
            return (master);
         }
      }
   }
   closedir(dirp);
   TRACE ( trace_file, POP_DEBUG, HERE, "findpty found nothing" );
   return (-1);
}

/*
 * writestring()
 *
 * Write a string in a single write() system call.
 */
void writestring (int fd, char *s)
{
     int l;

     l = strlen (s);
     write (fd, s, l);
     TRACE ( trace_file, POP_DEBUG, HERE, "write: %d bytes to client", l );
}

/*
 * talktochild()
 *
 * Handles the conversation between the parent and child (password program)
 * processes.
 *
 * Returns SUCCESS is the conversation is completed without any problems,
 * FAILURE if any errors are encountered (in which case, it can be assumed
 * that the password wasn't changed).
 */
int talktochild (int master, char *userid, char *oldpass, char *newpass,
                 char *emess, int asroot)
{
     char buf[BUFSIZE];
     char pswd[BUFSIZE+1];

     *emess = 0;

     TRACE ( trace_file, POP_DEBUG, HERE,
             "talktochild; master=%d; userid=%s; asroot=%d",
             master, userid, asroot );

     /* only get current password if not root */
     if (!asroot)
     {
       /* wait for current password prompt */
       if (!expect(master, P1, buf)) return FAILURE;

       /* send current password */
       sprintf(pswd, "%s\n", oldpass);
       writestring(master, pswd);
     }

     /* wait for new password prompt */
     if (!expect(master, P2, buf)) return FAILURE;

     /* write new password */
     sprintf(pswd, "%s\n", newpass);
     writestring(master, pswd);

     /* wait for response; grab errors */
     if (!expect(master, P3, buf)) {
        getemess(master, P2, buf);
        strcpy(emess, buf);
        return FAILURE;
     }

     /* write password again */
     writestring(master, pswd);

     /* wait for completion (some systems quietly exit) */
     if ( !expect(master, P4, buf) )
        TRACE ( trace_file, POP_DEBUG, HERE, "no response -- assuming OK" );

     return SUCCESS;
}

/*
 * match ()
 *
 * Matches a string against a pattern. Wild-card characters '*' in
 * the pattern match any sequence of 0 or more characters in the string.
 * The match is case-insensitive.
 *
 * Entry: str = string.
 *        pat = pattern.
 *
 * Exit:  function result =
 *              0 if no match.
 *              1 if the string matches some initial segment of
 *                the pattern.
 *              2 if the string matches the full pattern.
 */
int match (char *str, char *pat)
{
   int result=0;


   TRACE ( trace_file, POP_DEBUG, HERE, "match; str=(%d)'%s'; pat=(%d)'%s'",
           strlen(str), debug_str(str, strlen(str), 0),
           strlen(pat), debug_str(pat, strlen(pat), 1) );

   while (*str && *pat) {
     if (*pat == '*')
       break;

     /* ignore multiple space sequences */
     if (*pat == ' ' && isspace (*str)) {
       ++pat;
       do
         ++str;
       while (isspace (*str) && *str != '\0');
       continue;
     }

     /* ignore multiple newline sequences when found */
     if (*pat == '\n' && (*str == '\n' || *str == '\r')) {
       ++pat;
       do
         ++str;
       while (isspace (*str) && *str != '\0');
       continue;
     }

     /* Process all other characters as a literal */
     if (tolower(*str) != *pat++) {
       TRACE ( trace_file, POP_DEBUG, HERE, "...returning 0" );
       return 0;
     }

     ++str;
   }

   if (*str == 0) {
     TRACE ( trace_file, POP_DEBUG, HERE, "...returning %d", *pat == 0 ? 2 : 1 );
     return *pat == 0 ? 2 : 1;
   }

   if (*pat++ == 0) {
     TRACE ( trace_file, POP_DEBUG, HERE, "...returning 0" );
     return 0;
   }

   while (*str) {
     result = match(str, pat);
     if (result != 0)
       break;
     ++str;
   }

   TRACE ( trace_file, POP_DEBUG, HERE, "...returning %d", result );
   return result;
}


/*
 * expect ()
 *
 * Reads 'passwd' command output and compares it to expected output.
 *
 * Entry: master = fid of master pty.
 *        expected = pointer to array of pointers to alternate expected
 *            strings, terminated by an empty string.
 *        buf = pointer to buffer.
 *
 * Exit:  function result = SUCCESS if output matched, FAILURE if not.
 *        buf = the text read from the slave.
 *
 * Text is read from the slave and accumulated in buf. As long as
 * the text accumulated so far is an initial segment of at least 
 * one of the expected strings, the function continues the read.
 * As soon as one of full expected strings has been read, the
 * function returns SUCCESS. As soon as the text accumulated so far
 * is not an initial segment of or exact match for at least one of 
 * the expected strings, the function returns FAILURE.
 */

int expect (int master, char **expected, char *buf)
{
     int n, m;
     char **s;
     char *p;
     int initialSegment;


     TRACE ( trace_file, POP_DEBUG, HERE, "expect; master=%d; expected=(%d)'%s'...",
             master, strlen(*expected), debug_str(*expected, strlen(*expected), 0) );

     buf[0] = 0;
     while ( 1 ) {
        n = strlen (buf);
        if ( n >= BUFSIZE-1 ) {
           logit ( trace_file, LOG_ERR, HERE, "buffer overflow on read from child" );
           return FAILURE;
        }

        TRACE ( trace_file, POP_DEBUG, HERE, "...reading (%d) from child...",
                BUFSIZ-1-n );

        m = read ( master, &buf[n], BUFSIZ-1-n );
        if ( m < 0 ) {
           err_msg ( HERE, "read error from child" );
           return FAILURE;
        }
        buf [ n + m ] = '\0';

        TRACE ( trace_file, POP_DEBUG, HERE, "...read: (%d) '%.128s'",
                m, buf + n );

        strcat (buf, "\n");

        p = strchr (&buf[n], '\r');
        while ( p != NULL ) {
            *p = ' ';
            p  = strchr (&buf[n], '\r');
        }

        /* Ignore leading whitespace. It gets in the way. */
        p = buf;
        while ( isspace (*p) )
            ++p;

        if ( *p == '\0' )
            continue;

        initialSegment = 0;
        for ( s = expected; **s != 0; s++ ) {
            switch ( match(p, *s) ) {
            case 2:
                TRACE ( trace_file, POP_DEBUG, HERE, "expect: succes" );
                return SUCCESS;
            case 1:
                initialSegment = 1;
            default:
                break;
            }
        }

        if ( !initialSegment ) {
          TRACE ( trace_file, POP_DEBUG, HERE, "expect: failure" );
          return FAILURE;
        }
     } /* while(1) loop */
}


/*
 * getemess()
 *
 * This function accumulates a 'passwd' command error message issued
 * after the first copy of the password has been sent.
 *
 * Entry: master = fid of master pty.
 *        expected = pointer to array of pointers to alternate expected
 *            strings for first password prompt, terminated by an 
 *            empty string.
 *        buf = pointer to buffer containing text read so far.
 *
 * Exit:  buf = the error message read from the slave.
 *
 * Text is read from the slave and accumulated in buf until the text
 * at the end of the buffer is an exact match for one of the expected
 * prompt strings. The expected prompt string is removed from the buffer,
 * returning just the error message text. Newlines in the error message
 * text are replaced by spaces.
 */
void getemess (int master, char **expected, char *buf)
{
   int n, m;
   char **s;
   char *p, *q;


   TRACE ( trace_file, POP_DEBUG, HERE, "getemess; master=%d; expected=%s",
           master, *expected );

   n = strlen(buf);
   while (1) {
      for (s = expected; **s != 0; s++) {
         for (p = buf; *p; p++) {
            if (match(p, *s) == 2) {
               *p = 0;
               for (q = buf; *q; q++) if (*q == '\n') *q = ' ';
               return;
            }
         }
      }
      if (n >= BUFSIZE-1) {
         logit ( trace_file, LOG_ERR, HERE, "buffer overflow on read from child" );
         return;
      }
      m = read(master, buf+n, BUFSIZE+1-n);
      if ( m < 0 ) {
         err_msg ( HERE, "read error from child" );
         return;
      }
      n += m;
      buf[n] = 0;

      TRACE ( trace_file, POP_DEBUG, HERE, "read: %s", buf );
   }
}


void WriteToClient (char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stdout, fmt, ap);
    fputs ("\r\n", stdout );
    fflush (stdout);
    va_end (ap);

    if ( verbose )
    {
        char cbuf [ 1024 ] = "";
        va_start ( ap, fmt );
        Qvsnprintf ( cbuf, sizeof(cbuf), fmt, ap );
        va_end ( ap );
        logit ( trace_file, POP_DEBUG, HERE, "WriteToClient: %s", cbuf );
    }
}


void ReadFromClient (char *line)
{
        char *sp;

        strcpy (line, "");
        fgets (line, BUFSIZE, stdin);
        if ((sp = strchr(line, '\n')) != NULL) *sp = '\0'; 
        if ((sp = strchr(line, '\r')) != NULL) *sp = '\0'; 
        

        if ( verbose )
        {
            if ( !strncasecmp ( "pass", line, 4 ) )
                logit ( trace_file, POP_DEBUG, HERE, "ReadFromClient: pass xxxx" );
            else if ( !strncasecmp ( "newpass", line, 7 ) )
                logit ( trace_file, POP_DEBUG, HERE, "ReadFromClient: newpass xxxx" );
            else
                logit ( trace_file, POP_DEBUG, HERE, "ReadFromClient: %d '%.128s'",
                        strlen(line), line );
        }

        /*
         * convert initial keyword on line to lower case.
         */
        for (sp = line; isalpha(*sp); sp++) *sp = tolower(*sp);
}


int chkPass ( char *userid, char *pass, struct passwd *pw, POP *p )
{
    char err_msg [ 1024 ];


    if ( auth_user ( p, pass, pw, err_msg, sizeof(err_msg) ) == POP_SUCCESS )
        return SUCCESS;
    else
    {
        logit   ( trace_file, POP_NOTICE, HERE, "%s", err_msg ); 
        return FAILURE;
    }
}


/*
 * Logs a message
 */
void
msg ( WHENCE, const char *format, ... )
{
    va_list  ap;                       /* Pointer into stack to extract
                                        *     parameters */
    size_t   left   = sizeof(msg_buf);
    size_t   len    = 0;
    int      iChunk = 0;


    va_start ( ap, format );

    if ( pname != NULL )
    {
        iChunk = Qsnprintf ( msg_buf + len, left, "%s: ", pname );
        left  -= ( iChunk > 0 ? iChunk : left );
        len   += ( iChunk > 0 ? iChunk : left );
    }

    iChunk = Qvsnprintf ( msg_buf + len, left, format, ap );
    left  -= ( iChunk > 0 ? iChunk : left );
    len   += ( iChunk > 0 ? iChunk : left );

    va_end   ( ap );

    logit   ( trace_file, POP_DEBUG, fn, ln, "%s", msg_buf );
}


/*
 * Logs a message, dumps core, terminates.
 */
void
err_dump ( WHENCE, const char *format, ... )
{
    va_list  ap;                       /* Pointer into stack to extract
                                        *     parameters */
    size_t   left   = sizeof(msg_buf);
    size_t   len    = 0;
    int      iChunk = 0;


    va_start ( ap, format );

    if ( pname != NULL )
    {
        iChunk = Qsnprintf ( msg_buf + len, left, "%s: ", pname );
        left  -= ( iChunk > 0 ? iChunk : left );
        len   += ( iChunk > 0 ? iChunk : left );
    }

    iChunk = Qvsnprintf ( msg_buf + len, left, format, ap );
    left  -= ( iChunk > 0 ? iChunk : left );
    len   += ( iChunk > 0 ? iChunk : left );

    iChunk = Qsnprintf ( msg_buf + len, left, ": %s [%s:%d]",
                         sys_err_str(), fn, ln );
    left  -= ( iChunk > 0 ? iChunk : left );
    len   += ( iChunk > 0 ? iChunk : left );

    va_end   ( ap );

    logit   ( trace_file, POP_PRIORITY, fn, ln, "%s", msg_buf );

    if ( trace_file != NULL )
    {
        fclose ( trace_file );
        trace_file = NULL;
    }

    abort();
    exit ( 1 );
}


/*
 * Logs a message, with error information automatically appended.
 */
void
err_msg ( WHENCE, const char *format, ... )
{
    va_list  ap;                       /* Pointer into stack to extract
                                        *     parameters */

    size_t   left   = sizeof(msg_buf);
    size_t   len    = 0;
    int      iChunk = 0;


    va_start ( ap, format );

    if ( pname != NULL )
    {
        iChunk = Qsnprintf ( msg_buf + len, left, "%s: ", pname );
        left  -= ( iChunk > 0 ? iChunk : left );
        len   += ( iChunk > 0 ? iChunk : left );
    }

    iChunk = Qvsnprintf ( msg_buf + len, left, format, ap );
    left  -= ( iChunk > 0 ? iChunk : left );
    len   += ( iChunk > 0 ? iChunk : left );

    va_end   ( ap );

    iChunk = Qsnprintf ( msg_buf + len, left, ": %s [%s:%d]",
                         sys_err_str(), fn, ln );
    left  -= ( iChunk > 0 ? iChunk : left );
    len   += ( iChunk > 0 ? iChunk : left );

    logit   ( trace_file, POP_PRIORITY, fn, ln, "%s", msg_buf );
}


void
my_perror()
{
    fprintf ( stderr, "%s\n", sys_err_str() );
}


char *
sys_err_str()
{
    static char msgstr [ 200 ];
    if ( errno != 0 )
    {
        if ( errno > 0 && errno < sys_nerr )
            Qsnprintf ( msgstr, sizeof(msgstr), "(%d) %s", 
                        errno, sys_errlist [ errno ] );
        else
            Qsnprintf ( msgstr, sizeof(msgstr), "errno = %d", errno );

        msgstr [ sizeof(msgstr) -1 ] = '\0';
    }
    else
        msgstr[0] = '\0';
    
    return ( msgstr );
}


void get_client_info ( POP *p, BOOL no_rev_lookup )
{
    struct hostent      *   hp  = NULL;
    struct sockaddr_in      cs;                 /*  Communication parameters */
    struct hostent      *   ch;                 /*  Client host information */
    int                     len = 0;
    int                     sp  = 0;


#ifdef _DEBUG
    p->ipaddr = strdup("* _DEBUG *");
    p->client = p->ipaddr;
#else
    hp = gethostbyname ( p->myhost );
    if ( hp != NULL ) {
        if ( index ( hp->h_name, '.' ) == NULL ) {         /* FQN not returned */
            /*
             * SVR4 resolver is stupid and returns h_name as whatever
             * you gave gethostbyname.  Thus do a reverse lookup
             * on the first address and hope for the best.
             */
            u_long x = *(u_long *) hp->h_addr_list [ 0 ];
            hp = gethostbyaddr ( (char *) &x, 4, AF_INET );
            if ( hp != NULL ) {
                (void) strncpy ( p->myhost, hp->h_name, MAXHOSTNAMELEN );
                p->myhost [ MAXHOSTNAMELEN ] = '\0';
            }
        } /* index ( hp->h_name, '.' ) == NULL */
        else {
            (void) strncpy ( p->myhost, hp->h_name, MAXHOSTNAMELEN );
            p->myhost [ MAXHOSTNAMELEN ] = '\0';
        }
    }

    /*  
     * Get the address and socket of the client to whom I am speaking 
     */
    len = sizeof(cs);
    if ( getpeername ( sp, (struct sockaddr *) &cs, &len ) < 0 ) {
        err_msg ( HERE, "Unable to obtain socket and address of client" );
    }

    /*
     * Save the dotted decimal form of the client's IP address 
     * in the POP parameter block 
     */
    p->ipaddr = (char *) strdup ( inet_ntoa ( cs.sin_addr ) );

    /*  
     * Save the client's port 
     */
    p->ipport = ntohs ( cs.sin_port );

    if ( no_rev_lookup )
        p->client = p->ipaddr;
    else {
        /*  
         * Get the canonical name of the host to whom I am speaking 
         */
        ch = gethostbyaddr ( (char *) &cs.sin_addr, sizeof(cs.sin_addr), AF_INET );
        if ( ch == NULL ) {
            msg ( HERE,
                  "(v%s) Unable to get canonical name of client %s: %s (%d)",
                  VERSION, p->ipaddr, hstrerror(h_errno), h_errno );
            p->client = p->ipaddr;
        }
        /*  
         * Save the cannonical name of the client host in 
         * the POP parameter block 
         */
        else {

#  ifndef    BIND43
            p->client = (char *) strdup ( ch->h_name );
#  else /* using BIND43 */

#    ifndef   SCOR5
#      include <arpa/nameser.h>
#      include <resolv.h>
#    endif /* SCOR5 */

        /*  Distrust distant nameservers */

#    if !(defined(BSD) && (BSD >= 199103)) && !defined(OSF1) && !defined(HPUX10)
#      if (!defined(__RES)) || (__RES < 19940415)
#        ifdef    SCOR5
        extern struct __res_state       _res;
#        else
        extern struct state         _res;
#        endif /* SCOR5 */
#      endif /* RES */
#    endif /* BSD / OSF1 / HPUX */
        struct hostent      *   ch_again;
        char            *   *   addrp;
        char                    h_name[MAXHOSTNAMELEN + 1];

        /*  We already have a fully-qualified name */
#    ifdef RES_DEFNAMES
        _res.options &= ~RES_DEFNAMES;
#    endif /* RES_DEFNAMES */

        strncpy(h_name, ch->h_name, sizeof(h_name));

        /*  See if the name obtained for the client's IP 
            address returns an address */
        if ( ( ch_again = gethostbyname(h_name) ) == NULL ) {
            msg ( HERE,
                  "Client at \"%s\" resolves to an unknown host name \"%s\"",
                  p->ipaddr, h_name );
            p->client = p->ipaddr;
        }
        else {
            /*  Save the host name (the previous value was 
                destroyed by gethostbyname) */
            p->client = (char *)strdup(ch_again->h_name);

            /*  Look for the client's IP address in the list returned 
                for its name */
            for ( addrp=ch_again->h_addr_list; *addrp; ++addrp )
                if ( bcmp ( *addrp, &(cs.sin_addr), sizeof(cs.sin_addr) ) == 0 ) break;

                if ( !*addrp ) {
                    msg ( HERE,
                          "Client address \"%s\" not listed for its host name \"%s\"",
                          p->ipaddr, h_name );
                    p->client = p->ipaddr;
                }
        }

#    ifdef RES_DEFNAMES
        /* 
         *  Must restore nameserver options since code in crypt uses
         *  gethostbyname call without fully qualified domain name!
         */
        _res.options |= RES_DEFNAMES;
#    endif /* RES_DEFNAMES */

#  endif /* BIND43 */
                }

    } /* no_rev_lookup */

#endif /* _DEBUG */
}


/*--------------------------------------------------------------
 * Copies a string, converting non-printables into codes.
 *
 * Parameters:
 *    p:      Pointer to string (does not have to be null-terminated).
 *    inLen:  Length of string (in bytes).
 *    order:  0, or 1 for nested calls.
 *
 * Result:
 *    Pointer to static buffer.  New string is
 *    null-terminated.
 */
static char debug_str_buf0 [ 512 ];
static char debug_str_buf1 [ 512 ];
char *debug_str ( char *p, int inLen, int order )
    {
    int     outLen = inLen;
    int     j      = 0;
    char   *q      = p;
    char   *r      = NULL;
    char   *s      = NULL;
    

    if ( inLen >= (int) sizeof(debug_str_buf1) )
        inLen = outLen = sizeof(debug_str_buf1);

    if ( p == NULL )
        return NULL;
    
    for ( j = 0; j < inLen; j++ )
        {
        if ( *q < ' ' || *q > '~' )
            outLen += 2;
        q++;
        }

    if ( order == 0 )
        r = debug_str_buf0;
    else
        r = debug_str_buf1;

    s = r;
    q = p;
    for ( j = 0; j < inLen; j++ )
        {
        if ( *q >= ' ' && *q <= '~' )
            *s++ = *q;
        else
            {
            *s++ = '\\';
            switch ( *q )
                {
                case '\n':
                    *s++ = 'n';
                    break;
                    
                case '\r':
                    *s++ = 'r';
                    break;
                    
                case'\f':
                    *s++ = 'f';
                    break;

                case'\t':
                    *s++ = 't';
                    break;
                    
                default:
                    sprintf ( s, "%02X", *q );
                    s += 2;
                } /* end case */
            } /* end *q if unprintable */
        q++;
        } /* end for loop */
    *s = 0;
    return ( r );
    }


