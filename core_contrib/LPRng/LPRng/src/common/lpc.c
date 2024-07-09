/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: lpc.c,v 5.8 2000/04/14 20:39:55 papowell Exp papowell $";


/***************************************************************************
 * SYNOPSIS
 *      lpc [ -PPrinter] [-S Server] [-U username ][-V] [-D debug] [command]
 * commands:
 *
 *   status [printer]     - show printer status (default is all printers)
 *   msg   printer  ....  - printer status message
 *   start [printer]      - start printing
 *   stop [printer]       - stop printing
 *   up [printer]         - start printing and spooling
 *   down [printer]       - stop printing and spooling
 *   enable [printer]     - enable spooling
 *   disable [printer]    - disable spooling
 *   abort [printer]      - stop printing, kill server
 *   kill [printer]       - stop printing, kill server, restart printer
 *
 *   topq printer (user [@host] | host | jobnumer)*
 *   hold printer (all | user [@host] | host |  jobnumer)*
 *   release printer (all | user [@host] | host | jobnumer)*
 *
 *   lprm printer [ user [@host]  | host | jobnumber ] *
 *   lpq printer [ user [@host]  | host | jobnumber ] *
 *   lpd [pr | pr@host]   - PID of LPD server
 *   active [pr |pr@host] - check to see if server accepting connections
 *   client [all | pr ]     - show client configuration and printcap info 
 *   server [all |pr ]     - show server configuration and printcap info 
 *   defaultq               - show default queue for LPD server\n\
 *   defaults               - show default configuration values\n\
 *
 * DESCRIPTION
 *   lpc sends a  request to lpd(8)
 *   and reports the status of the command
 ****************************************************************************
 *
 * Implementation Notes
 * Patrick Powell Wed Jun 28 21:28:40 PDT 1995
 * 
 * The LPC program is an extremely simplified front end to the
 * LPC functionality in the server.  The commands send to the LPD
 * server have the following format:
 * 
 * \6printer user command options
 * 
 * If no printer is specified, the printer is the default from the
 * environment variable, etc.
 * 
 */

#include "lp.h"
#include "initialize.h"
#include "getprinter.h"
#include "sendreq.h"
#include "child.h"
#include "control.h"
#include "getopt.h"
#include "patchlevel.h"
#include "errorcodes.h"

/**** ENDINCLUDE ****/


/***************************************************************************
 * main()
 * - top level of LPP Lite.
 *
 ****************************************************************************/


#undef EXTERN
#undef DEFINE
#define EXTERN
#define DEFINE(X) X
#include "lpc.h"

 void use_msg(void);
 void doaction( struct line_list *args );
 static char *Username_JOB;

int main(int argc, char *argv[], char *envp[])
{
	char *s;
	int i;
	char msg[ LINEBUFFER ];
	struct line_list args;


	/* set signal handlers */
	(void) plp_signal (SIGHUP, cleanup_HUP);
	(void) plp_signal (SIGINT, cleanup_INT);
	(void) plp_signal (SIGQUIT, cleanup_QUIT);
	(void) plp_signal (SIGTERM, cleanup_TERM);
	(void) signal( SIGPIPE, SIG_IGN );
	(void) signal( SIGCHLD, SIG_DFL);


	/*
	 * set up the user state
	 */
#ifndef NODEBUG
	Debug = 0;
#endif

	Init_line_list(&args);
	Initialize(argc, argv, envp);
	Setup_configuration();

	/* scan the argument list for a 'Debug' value */

	Get_parms(argc, argv);      /* scan input args */

	DEBUG1("lpc: Printer '%s', Optind '%d', argc '%d'", Printer_DYN, Optind, argc );
	if(DEBUGL1){
		int ii;
		for( ii = Optind; ii < argc; ++ii ){
			logDebug( " [%d] '%s'", ii, argv[ii] );
		}
	}

	if( Username_JOB && OriginalRUID ){
		struct line_list user_list;
		char *str, *t;
		struct passwd *pw;
		int found, uid;

		DEBUG2("lpc: checking '%s' for -U perms",
			Allow_user_setting_DYN );
		Init_line_list(&user_list);
		Split( &user_list, Allow_user_setting_DYN,File_sep,0,0,0,0,0);
		
		found = 0;
		for( i = 0; !found && i < user_list.count; ++i ){
			str = user_list.list[i];
			DEBUG2("lpc: checking '%s'", str );
			uid = strtol( str, &t, 10 );
			if( str == t || *t ){
				/* try getpasswd */
				pw = getpwnam( str );
				if( pw ){
					uid = pw->pw_uid;
				}
			}
			DEBUG2( "lpc: uid '%d'", uid );
			found = ( uid == OriginalRUID );
			DEBUG2( "lpc: found '%d'", found );
		}
		if( !found ){
			Diemsg( _("-U (username) can only be used by ROOT") );
		}
	}
	if( Username_JOB ){
		Set_DYN(&Logname_DYN, Username_JOB);
	}

	if( Optind < argc ){
		for( i = Optind; argv[i]; ++i ){
			Add_line_list(&args,argv[i],0,0,0);
		}
		Check_max(&args,2);
		args.list[args.count] = 0;
		doaction( &args );
	} else while(1){
		fprintf( stdout, "lpc>" );
		fflush( stdout );
		if( fgets( msg, sizeof(msg), stdin ) == 0 ) break;
		if( (s = safestrchr( msg, '\n' )) ) *s = 0;
		DEBUG1("lpc: '%s'", msg );
		Free_line_list(&args);
		Split(&args,msg,Whitespace,0,0,0,0,0);
		Check_max(&args,2);
		args.list[args.count] = 0;
		if(DEBUGL1)Dump_line_list("lpc - args", &args );
		if( args.count == 0 ) continue;
		s = args.list[0];
		if( safestrcasecmp(s,"exit") == 0 || s[0] == 'q' || s[0] == 'Q' ){
			break;
		}
		doaction(&args);
	}
	Free_line_list(&args);
	Errorcode = 0;
	Is_server = 0;
	cleanup(0);
	return(0);
}

void doaction( struct line_list *args )
{
	int action, fd, n, i;
	struct line_list l;
	char msg[SMALLBUFFER];
	char *s, *t, *w, *printcap;

	Init_line_list(&l);
	s = t = w = printcap = 0;
	if( args->count == 0 ) return;
	action = Get_controlword( args->list[0] );
	if(DEBUGL1)Dump_line_list("doaction - args", args );
	if( action == 0 ){
		use_msg();
		return;
	}
	if( args->count > 1 ){
		Set_DYN(&Printer_DYN,args->list[1]);
		Fix_Rm_Rp_info(0,0);
		DEBUG1("doaction: Printer '%s', RemotePrinter '%s', RemoteHost '%s'",
			Printer_DYN, RemotePrinter_DYN, RemoteHost_DYN );
		if( (s = safestrchr(args->list[1],'@')) ) *s = 0;
	} else if( Printer_DYN == 0 ){
		/* get the printer name */
		Get_printer();
		Fix_Rm_Rp_info(0,0);
	} else {
		Fix_Rm_Rp_info(0,0);
	}
	if( Server ){
		DEBUG1("doaction: overriding Remotehost with '%s'", Server );
		Set_DYN(&RemoteHost_DYN, Server );
	}

	DEBUG1("lpc: RemotePrinter_DYN '%s', RemoteHost_DYN '%s'", RemotePrinter_DYN, RemoteHost_DYN );
	if( action == OP_SERVER ){
		Is_server = 1;
		Setup_configuration();
	}
	if( action == OP_DEFAULTS ){
		Is_server = 0;
		Setup_configuration();
		Dump_parms( "Defaults", Pc_var_list );
	} else if( action == OP_CLIENT || action == OP_SERVER ){
		s = Join_line_list(&Config_line_list,"\n :");
		printcap = safestrdup3("Config","\n :",s,__FILE__,__LINE__);
		if( (w = safestrrchr(printcap,' ')) ) *w = 0;
		if( Write_fd_str( 1, printcap ) < 0 ) cleanup(0);
		if( s ) free(s); s = 0;
		if( t ) free(t); t = 0;
		if( printcap ) free(printcap); printcap = 0;

		if( args->count > 1 ){
			Get_all_printcap_entries();
			if( !safestrcasecmp(args->list[1], "all") ){
				if(s) free(s); s = 0;
				if( t ) free(t); t = 0;
				if(printcap) free(printcap); printcap = 0;
				s = Join_line_list(&PC_names_line_list,"\n :");
				printcap = safestrdup3("\nNames","\n :",s,__FILE__,__LINE__);
				if( (w = safestrrchr(printcap,' ')) ) *w = 0;
				if( Write_fd_str( 1, printcap ) < 0 ) cleanup(0);

				if(s) free(s); s = 0;
				if( t ) free(t); t = 0;
				if(printcap) free(printcap); printcap = 0;
				s = Join_line_list(&All_line_list,"\n :");
				printcap = safestrdup3("\nAll","\n :",s,__FILE__,__LINE__);
				if( (w = safestrrchr(printcap,' ')) ) *w = 0;
				if( Write_fd_str( 1, printcap ) < 0 ) cleanup(0);
			}

			if( Write_fd_str( 1,"\nPrintcap Information\n") < 0 ) cleanup(0);
			for( i = 0; i < All_line_list.count; ++i ){
				if( safestrcasecmp(args->list[1], "all")
					&& safestrcasecmp(args->list[1], All_line_list.list[i]) ){
					continue;
				}
				Set_DYN(&Printer_DYN,All_line_list.list[i]);
				Fix_Rm_Rp_info(0,0);
				if( s ) free(s); s = 0;
				if( t ) free(t); t = 0;
				if( printcap ) free(printcap); printcap = 0;
				t = Join_line_list(&PC_alias_line_list,"|");
				s = Join_line_list(&PC_entry_line_list,"\n :");
				if( s && t ){
					if( (w = safestrrchr(t,'|')) ) *w = 0;
					printcap = safestrdup3(t,"\n :",s,__FILE__,__LINE__);
					if( (w = safestrrchr(printcap,' ')) ) *w = 0;
					if( Write_fd_str( 1, printcap ) < 0 ) cleanup(0);
				}
			}
		}
		if( s ) free(s); s = 0;
		if( t ) free(t); t = 0;
		if( printcap ) free(printcap); printcap = 0;
	} else if( action == OP_LPQ || action == OP_LPRM ){
		pid_t pid, result;
		plp_status_t status;
		if( args->count == 1 && Printer_DYN ){
			SNPRINTF(msg,sizeof(msg), "-P%s", Printer_DYN );
			Add_line_list(args,msg,0,0,0);
			Check_max(args,1);
			args->list[args->count] = 0;
		} else if( args->count > 1 ){
			s = args->list[1];
			if( safestrcasecmp(s,"all") ){
				SNPRINTF(msg,sizeof(msg), "-P%s", s );
			} else {
				strcpy(msg, "-a" );
			}
			if( s ) free(s);
			args->list[1] = safestrdup(msg,__FILE__,__LINE__);
		}
		if(DEBUGL1)Dump_line_list("ARGS",args);
		if( (pid = dofork(0)) == 0 ){
			/* we are going to close a security loophole */
			Full_user_perms();
			/* this would now be the same as executing LPQ as user */
			close_on_exec(3);
			execvp( args->list[0],args->list );
			Diemsg( _("execvp failed - '%s'"), Errormsg(errno) );
			exit(0);
		} else if( pid < 0 ) {
			Diemsg( _("fork failed - '%s'"), Errormsg(errno) );
		}
		while( (result = plp_waitpid(pid,&status,0)) != pid ){
			int err = errno;
			DEBUG1("lpc: waitpid(%d) returned %d, err '%s'",
				pid, result, Errormsg(err) );
			if( err == EINTR ) continue; 
			Errorcode = JABORT;
			logerr_die( LOG_ERR, "doaction: waitpid(%d) failed", pid);
		} 
		DEBUG1("lpc: system pid %d, exit status %s",
			result, Decode_status( &status ) );
	} else {
		Add_line_list(&l, Logname_DYN, Value_sep, 0, 0 );
		Add_line_list(&l, args->list[0], Value_sep, 0, 0);
		Remove_line_list(args, 0);
		if( args->count > 0 ) {
			Add_line_list(&l, RemotePrinter_DYN, Value_sep, 0, 0 );
			Remove_line_list(args, 0);
		}
		Merge_line_list(&l, args, 0, 0, 0 );
		Check_max(&l, 1 );
		l.list[l.count] = 0;
		fd = Send_request( 'C', REQ_CONTROL, l.list, Connect_timeout_DYN,
			Send_query_rw_timeout_DYN, 1 );
		if( fd > 0 ){
			while( (n = read(fd, msg, sizeof(msg))) > 0 ){
				if( (write(1,msg,n)) < 0 ) cleanup(0);
			}
		}
		close(fd);
	}
	Free_line_list(&l);
}

/* VARARGS2 */
#ifdef HAVE_STDARGS
 void setstatus (struct job *job,char *fmt,...)
#else
 void setstatus (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    struct job *job;
    char *fmt;
#endif
	char msg[LARGEBUFFER];
    VA_LOCAL_DECL

    VA_START (fmt);
    VA_SHIFT (job, struct job * );
    VA_SHIFT (fmt, char *);

	msg[0] = 0;
	/* if( Verbose ){ */
		(void) VSNPRINTF( msg, sizeof(msg)-2, fmt, ap);
		strcat( msg,"\n" );
		if( Write_fd_str( 2, msg ) < 0 ) cleanup(0);
	/* } */
	VA_END;
	return;
}

 void send_to_logger (int sfd, int mfd, struct job *job,const char *header, char *fmt){;}
/* VARARGS2 */
#ifdef HAVE_STDARGS
 void setmessage (struct job *job,const char *header, char *fmt,...)
#else
 void setmessage (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    struct job *job;
    char *fmt, *header;
#endif
	char msg[LARGEBUFFER];
    VA_LOCAL_DECL

    VA_START (fmt);
    VA_SHIFT (job, struct job * );
    VA_SHIFT (header, char * );
    VA_SHIFT (fmt, char *);

	msg[0] = 0;
	if( Verbose ){
		(void) VSNPRINTF( msg, sizeof(msg)-2, fmt, ap);
		strcat( msg,"\n" );
		if( Write_fd_str( 2, msg ) < 0 ) cleanup(0);
	}
	VA_END;
	return;
}


/***************************************************************************
 * void Get_parms(int argc, char *argv[])
 * 1. Scan the argument list and get the flags
 * 2. Check for duplicate information
 ***************************************************************************/

 void usage(void);

 char LPC_optstr[] 	/* LPC options */
 = "D:P:S:VU:";

/* scan the input arguments, setting up values */

void Get_parms(int argc, char *argv[] )
{
	int option;

	while ((option = Getopt (argc, argv, LPC_optstr )) != EOF) {
		switch (option) {
		case 'D': /* debug has already been done */
			Parse_debug( Optarg, 1 );
			break;
		case 'P': if( Optarg == 0 ) usage();
			Set_DYN(&Printer_DYN,Optarg); break;
		case 'V':
			Verbose = !Verbose;
			break;
		case 'S':
			Server = Optarg;
			break;
		case 'U': Username_JOB = Optarg; break;
		default:
			usage();
		}
	}
	if( Verbose ) fprintf( stderr, "%s\n", Version );
}

 char *msg = N_("\
usage: %s [-Ddebuglevel][-Pprinter][-Shost][-Uusername][-V] [command]\n\
 with no command, reads from stdin\n\
  -Ddebuglevel - debug level\n\
  -Pprinter    - printer or printer@host\n\
  -Shost       - connect to lpd server on host\n\
  -Uuser       - identify command as coming from user\n\
  -V           - increase information verbosity\n\
 commands:\n\
 active    (printer[@host])        - check for active server\n\
 abort     (printer[@host] | all)  - stop server\n\
 class     printer[@host] (class | off)      - show/set class printing\n\
 disable   (printer[@host] | all)  - disable queueing\n\
 debug     (printer[@host] | all) debugparms - set debug level for printer\n\
 down      (printer[@host] | all)  - disable printing and queueing\n\
 enable    (printer[@host] | all)  - enable queueing\n\
 hold      (printer[@host] | all) (name[@host] | job | all)*   - hold job\n\
 holdall   (printer[@host] | all)  - hold all jobs on\n\
 kill      (printer[@host] | all)  - stop and restart server\n\
 lpd       (printer[@host]) - get LPD PID \n\
 lpq       (printer[@host] | all) (name[@host] | job | all)*   - invoke LPQ\n\
 lprm      (printer[@host] | all) (name[@host]|host|job| all)* - invoke LPRM\n\
 msg printer message text  - set status message\n\
 move printer (user|jobid)* target - move jobs to new queue\n\
 noholdall (printer[@host] | all)  - hold all jobs off\n\
 printcap  (printer[@host] | all)  - report printcap values\n\
 quit                              - exit LPC\n\
 redirect  (printer[@host] | all) (printer@host | off )*       - redirect jobs\n\
 redo      (printer[@host] | all) (name[@host] | job | all)*   - release job\n\
 release   (printer[@host] | all) (name[@host] | job | all)*   - release job\n\
 reread    (printer[@host])        - LPD reread database information\n\
 start     (printer[@host] | all)  - start printing\n\
 status    (printer[@host] | all)  - status of printers\n\
 stop      (printer[@host] | all)  - stop  printing\n\
 topq      (printer[@host] | all) (name[@host] | job | all)*   - reorder job\n\
 up        (printer[@host] | all) - enable printing and queueing\n\
   diagnostic:\n\
      defaultq               - show default queue for LPD server\n\
      defaults               - show default configuration values\n\
      client  (printer | all) - client config and printcap information\n\
      server (printer | all) - server config and printcap\n");

void use_msg(void)
{
	fprintf( stderr, _(msg), Name );
}
void usage(void)
{
	use_msg();
	exit(1);
}

int Start_worker( struct line_list *args, int fd )
{
	if(DEBUGL1)Dump_line_list("LPC - Dummy Start_worker",args);
	Errorcode = JABORT;
	fatal(LOG_ERR,"LPC - Dummy Start_worker called");
	return(fd);
}
void Dispatch_input(int *talk, char *input ){}
