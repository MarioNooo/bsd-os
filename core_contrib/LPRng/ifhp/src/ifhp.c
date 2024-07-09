/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****/
#include "patchlevel.h"
 static char *const _id = "$Id: ifhp.c,v 1.46 2000/04/14 21:33:47 papowell Exp papowell $"
 " VERSION " PATCHLEVEL;

#include "ifhp.h"
#include "debug.h"

/*
 * Main program:
 * 1. does setup - checks mode
 * 2. initializes connection
 * 3. initializes printer
 * 4. checks printer for idle status
 * 5. gets pagecount
 * 6.
 *   if IF mode {
 *       determines job format
 *         - binary, PCL, PostScript, Text
 *       if PCL then sets up PCL
 *       if PostScript then sets up PostScript
 *       if Text then
 *          sets up Text
 *          if text converter,  runs converter
 *       puts job out to printer
 *   }
 *   if OF mode {
 *       reads input line;
 *       if suspend string, then suspend
 *       else pass through
 *   }
 * 7. terminates job
 * 8. checks printer for end of job status
 * 9. gets page count
 */


 char *Outbuf;	/* buffer */
 int Outmax;		/* max buffer size */
 int Outlen;		/* length to output */
 char *Inbuf;	/* buffer */
 int Inmax;		/* max buffer size */
 char *Inplace;	/* current input location */
 int Inlen;		/* total input from Inplace */
 char *Monitorbuf;	/* buffer */
 int Monitormax;		/* max buffer size */
 char *Monitorplace;	/* current input location */
 int Monitorlen;		/* total input from Monitorplace */
 int Nonblock_io;	/* status of output fd blocking */

 char *PJL_UEL_str =  "\033%-12345X";
 char *PJL_str =  "@PJL\n";


 int main(int argc,char *argv[], char *envp[])
{
	struct stat statb;
	char *s, *dump;
	int i, fd, c;
	struct line_list l;

	Init_line_list(&l);

	Debug = Trace_on_stderr = 0;
	dump = 0;

	Argc = argc;
	Argv = argv;
	Envp = envp;
	if( argc ){
		Name = argv[0];
		if( (s = strrchr( Name, '/' )) ){
			Name = s+1;
		}
	} else {
		Name = "????";
	}

	time( &Start_time );
	/* 1. does setup - checks mode */

	/* set up the accounting FD for output */
	if( fstat(0,&statb) == -1 ){
		Errorcode = JABORT;
		fatal( "ifhp: stdin is not open");
	}
	if( fstat(1,&statb) == -1 ){
		Errorcode = JABORT;
		fatal( "ifhp: stdout is not open");
	}
	if( fstat(2,&statb) == -1 ){
		if( (fd = open( "/dev/null", O_WRONLY )) != 2 ){
			Errorcode = JABORT;
			logerr_die( "ifhp: open /dev/null" );
		}
	}
	Accounting_fd = -1;
	if( fstat( 3, &statb ) == 0 ){
		Accounting_fd = 3;
	}

	/* initialize input and output buffers */
	Init_outbuf();
	Init_inbuf();
	Init_monitorbuf();

	/* check the environment variables */
	if( (s = getenv("PRINTCAP_ENTRY")) ){
		DEBUG3("main: PRINTCAP_ENTRY '%s'", s );
		Split( &l, s, ":", 1, Value_sep, 1, 1, 0  );
		if(DEBUGL3)Dump_line_list("main: PRINTCAP_ENTRY", &l );
		if( (s = Find_str_value(&l,"ifhp",Value_sep)) ){
			DEBUG1("main: getting PRINTCAP info '%s'", s );
			Split( &Topts, s, ",", 1, Value_sep, 1, 1, 0  );
		}
		Free_line_list(&l);
	}
	if(DEBUGL3)Dump_line_list("main: PRINTCAP Topts", &Topts );

	/* get the argument lists */
	getargs( argc, argv );
	/* we extract the debug information */
	Trace_on_stderr = Find_flag_value( &Topts, "trace", Value_sep );
	if( Debug == 0 && (s = Find_str_value( &Topts, "debug", Value_sep )) ){
		Debug = atoi(s);
	}
	DEBUG1("main: Debug '%d'", Debug );
	/* check for config file */
	if( !(Config_file = Find_str_value(&Topts,"config",Value_sep)) ){
		Config_file = CONFIG_FILE;
	}

	/* find the model id. First we check the Toptions, then
	 * we check to see if we use a command line flag
	 */
	Model_id = Find_str_value(&Topts,"model",Value_sep);
	dump = Find_exists_value(&Topts,"dump",Value_sep);
	DEBUG1("main: dump %s",dump );
	if( !Model_id &&
		(s = Find_str_value(&Topts,"model_from_option",Value_sep)) ){
		DEBUG1("main: checking model_from_option '%s'", s );
		for( ; !Model_id && *s; ++s ){
			c = cval(s);
			if( isupper(c) ) Model_id = Upperopts[c-'A'];
			if( islower(c) ) Model_id = Loweropts[c-'a'];
		}
	}
	DEBUG1("main: Model_id '%s'", Model_id );

	/* read the configuration file(s) */
	if( Read_file_list( &Raw, Config_file, "\n", 0, 0, 0, 0, 1, 0, 1 ) == 0 ){
		Errorcode = JABORT;
		fatal("main: no readable config file found in '%s'", Config_file );
	}
	/* no configuration? big problems */
	if( Raw.count == 0 ){
		fatal("main: no config file information in '%s'", Config_file );
	}
	if(DEBUGL5)Dump_line_list("main: Raw", &Raw );

	/* now we get the default values */
	DEBUG1("main: scanning Raw for default, then model '%s'", Model_id );
	Select_model_info( &Model, &Raw, "default", 0 );
	if(DEBUGL5)Dump_line_list("main: Model after defaults", &Model );

	/* now we get the values for the printer model, overriding defaults */
	if( Model_id && cval(Model_id) ){
		DEBUG1("main: scanning for model '%s'", Model_id  );
		Select_model_info( &Model, &Raw, Model_id, 1 );
		if(DEBUGL5)Dump_line_list("main: Model after Raw", &Model );
	}

	/* now we add in the Topts, overriding defaults AND printer */
	Merge_list( &Model, &Topts, Value_sep, 1, 1 );
	if(DEBUGL5)Dump_line_list("main: Model after Topts", &Model );

	/* handy way to get a dump of the configuration for the printer */
	if( dump ){
		fprintf(stdout,"Configuration for model='%s'\n",
			Model_id?Model_id:"default");
		for( i = 0; i < Model.count; ++i ){
			if( (s = Model.list[i]) ){
				fprintf( stdout, "[%3d]  %s\n", i, s );
			}
		}
		exit(0);
	}

	/* set the named parameter variables with values */
	if(DEBUGL1) Dump_line_list( "main: Model information", &Model );
	Initialize_parms(&Model, Valuelist );

	/* check for -Zlogall allowed, and set it */
	if( (s = Find_str_value( &Model, "user_opts", Value_sep)) ){
		lowercase(s);
		Split( &User_opts, s, List_sep, 1, Value_sep, 1, 1, 0  );
		if(DEBUGL4)Dump_line_list("main: user_opts", &User_opts);
		if( Find_flag_value( &Zopts, "logall", Value_sep )
			&& Find_exists_value( &User_opts, "logall", Value_sep ) ){
			Logall = 1;
		}
	}

	/* clean up statusfile information */
	if( Status_fd > 0 ){
		Errorcode = JABORT;
		fatal("main: you opened the statusfile too early!!!");
	}
	if( (s = Loweropts['s'-'a']) ) Statusfile = s;

	logmsg("main: using model '%s'", Model_id?Model_id:"DEFAULT" );

	DEBUG1("main: Debug level %d", Debug );
	if(DEBUGL1)Dump_line_list("Zopts",&Zopts);
	if(DEBUGL1)Dump_line_list("Topts",&Topts);
	/* if we have version request, print out the version */
	if( Debug || Find_flag_value(&Topts,"version",Value_sep)
		|| Find_flag_value(&Zopts,"version",Value_sep) ){
		logmsg( "Version %s", _id );
	}
	if(DEBUGL3) Dump_parms( "main- from list", Valuelist );
	if(DEBUGL3) Dump_line_list( "main: Model", &Model );

	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGCHLD, SIG_DFL);

	/* handle the case where we do not get status */
	Set_block_io(1);

	/* set the keepalive options */
	Set_keepalive(1);

	/* 2. initializes connection */
	if( Appsocket && !Device ){
		fatal( "appsocket and no 'dev=...' option" );
	}
	if( Device ){
		if( Appsocket && !No_udp_monitor && Status ){
			// Open_monitor( Device );
		}
		Open_device( Device );
	}
	if( Status && !Fd_readable(1) && !Force_status ){
		DEBUG1( "main: device cannot provide status" );
		Status = 0;
	}

	if( Status ){
		Set_nonblock_io(1);
	}

	Process_job();
	Errorcode = 0;
	Set_block_io(1);
	cleanup(0);
	return(0);
}

void cleanup(int sig)
{
    DEBUG3("cleanup: Signal '%s', Errorcode %d", Sigstr(sig), Errorcode );
	Set_nonblock_io( 1 );
	close(0);
	close(1);
	close(2);
    exit(Errorcode);
}

/*
 * getargs( int argc, char **argv )
 *  - get the options from the argument list
 */

void getargs( int argc, char **argv )
{
	int i, flag;
	char *arg, *s, m[2];

	/* logmsg("testing"); */
	DEBUG3("getargs: starting, debug %d", Debug);
	for( i = 1; i < argc; ++i ){
		arg = argv[i];
		if( *arg++ != '-' ) break;
		flag = *arg++;
		if( flag == '-' ){
			++i;
			break;
		}
		if( flag == 'c' ){
			 arg = "1";
		}
		if( *arg == 0 ){
			if( i < argc ){
				arg = argv[++i];
			} else {
				fatal( "missing argument for flag '%c'", flag );
			}
		}
		/* we duplicate the strings */
		DEBUG3("getargs: flag '%c'='%s'", flag, arg);
		if( islower(flag) ){
			Loweropts[flag-'a'] = arg;
		} else if( isupper(flag) ){
			switch( flag ){
			case 'T': case 'Z':
				if( (s = Upperopts[flag-'A']) ){
					Upperopts[flag-'A'] = 
						safestrdup3(s,",",arg,__FILE__,__LINE__);
					if( s ) free(s);
				} else {
					Upperopts[flag-'A'] = safestrdup(arg,__FILE__,__LINE__);
				}
				break;
			default:
				Upperopts[flag-'A'] = arg;
				break;
			}
		}
	}
	if( i < argc ){
		Accountfile = argv[i];
	} else if( (s = Loweropts['a'-'a']) ){
		Accountfile = s;
	}
	DEBUG1("getargs: finished options");
	/* set the Topts and Zopts values */
	memset(m,sizeof(m),0);
	m[0] = m[1] = 0;
	for( i = 'a'; i <= 'z'; ++i ){
		m[0] = i;
		if( (s = Loweropts[i-'a']) ) Set_str_value(&Topts,m,s);
	}
	for( i = 'A'; i <= 'Z'; ++i ){
		m[0] = i;
		if( (s = Upperopts[i-'A']) ) Set_str_value(&Topts,m,s);
	}
	if( ((s = Upperopts['F'-'A']) && *s == 'o')
		|| strstr( Name, "of" ) ){
		OF_Mode = 1;
	}
	Fix_special_user_opts(&Topts, Upperopts['T'-'A'] );
	Fix_special_user_opts(&Zopts, Upperopts['Z'-'A'] );
	if(DEBUGL1)Dump_line_list("getargs: T opts", &Topts);
	if(DEBUGL1)Dump_line_list("getargs: Z opts", &Zopts);
}

void Fix_special_user_opts( struct line_list *opts, char *line )
{
	struct line_list v;
	int i, append, c;
	char *key, *value, *s;

	DEBUG1("Fix_special_user_opts: '%s'", line );
	Init_line_list( &v );
	Split( &v, line, ",", 0, 0, 0, 1, 0 );
	for( i = 0; i < v.count; ++i ){
		key = v.list[i];
		c = 0;
		if( (value = strpbrk( key, Value_sep )) ){
			c = *value;
			*value = 0;
		}
		lowercase(key);
		DEBUG1("Fix_special_user_opts: '%s'='%s'", key, value );
		if( (append =  !strcasecmp( key, "font")) && c == '=' ){
			if( (s = Find_str_value(opts,key,Value_sep)) ){
				DEBUG1("Fix_special_user_opts: old '%s'='%s'", key, s );
				s = safestrdup3(s,",",value+1,__FILE__,__LINE__);
				DEBUG1("Fix_special_user_opts: new '%s'='%s'", key, s );
				Set_str_value(opts,key,s);
				if(s) free(s); s = 0;
			} else {
				DEBUG1("Fix_special_user_opts: init '%s'='%s'", key, value+1 );
				Set_str_value(opts,key,value+1);
			}
		}
		if( value ){
			*value = c;
		}
		if( !append ){
			Add_line_list(opts,key,Value_sep,1,1);
		}
	}
	Free_line_list( &v );
	if(DEBUGL1)Dump_line_list("Fix_special_user_opts", opts );
}

/*
 * Output buffer management
 *  Set up and put values into an output buffer for
 *  transmission at a later time
 */
void Init_outbuf()
{
	DEBUG1("Init_outbuf: Outbuf 0x%lx, Outmax %d, Outlen %d",
		(long)Outbuf, Outmax, Outlen );
	if( Outmax <= 0 ) Outmax = OUTBUFFER;
	if( Outbuf == 0 ) Outbuf = realloc_or_die( Outbuf, Outmax+1,__FILE__,__LINE__);
	Outlen = 0;
	Outbuf[0] = 0;
}

void Put_outbuf_str( char *s )
{
	if( s && *s ) Put_outbuf_len( s, strlen(s) );
}

void Put_outbuf_len( char *s, int len )
{
	DEBUG4("Put_outbuf_len: starting- Outbuf 0x%lx, Outmax %d, Outlen %d, len %d",
		(long)Outbuf, Outmax, Outlen, len );
	if( s == 0 || len <= 0 ) return;
	if( Outmax - Outlen <= len ){
		Outmax += ((OUTBUFFER + len)/1024)*1024;
		Outbuf = realloc_or_die( Outbuf, Outmax+1,__FILE__,__LINE__);
		DEBUG4("Put_outbuf_len: update- Outbuf 0x%lx, Outmax %d, Outlen %d, len %d",
		(long)Outbuf, Outmax, Outlen, len );
		if( !Outbuf ){
			Errorcode = JABORT;
			logerr_die( "Put_outbuf_len: realloc %d failed", len );
		}
	}
	memmove( Outbuf+Outlen, s, len );
	Outlen += len;
	Outbuf[Outlen] = 0;
}

/*
 * Input buffer management
 *  Set up and put values into an input buffer for
 *  scanning purposes
 */
void Init_inbuf()
{
	Inmax = LARGEBUFFER;
	Inbuf = realloc_or_die( Inbuf, Inmax+1,__FILE__,__LINE__);
	Inbuf[Inmax] = 0;
	Inplace = Inbuf;
	Inlen = 0;
	Inbuf[0] = 0;
}

void Put_inbuf_str( char *str )
{
	int len = strlen( str );
	Put_inbuf_len( str, len );
}

void Put_inbuf_len( char *str, int len )
{
	Inlen = strlen(Inplace);
	if( Inplace != Inbuf ){
		/* we readjust the locations */
		memmove( Inbuf, Inplace, Inlen+1 );
		Inplace = Inbuf;
	}
	if( Inmax - Inlen <= (len+1) ){
		Inmax += ((LARGEBUFFER + len+1)/1024)*1024;
		Inbuf = realloc_or_die( Inbuf, Inmax+1,__FILE__,__LINE__);
		if( !Inbuf ){
			Errorcode = JABORT;
			logerr_die( "Put_outbuf_len: realloc %d failed", len );
		}
	}
	memmove( Inplace+Inlen, str, len+1 );
	Inlen += len;
	Inbuf[Inlen] = 0;
	DEBUG4("Put_inbuf_len: buffer '%s'", Inbuf );
}

/*
 * char *Get_inbuf_str()
 * return a pointer to a complete line in the input buffer
 *   This will, in effect, remove the line from the
 *   buffer, and it should be saved.
 * Any of the Put_inbuf or Get_inbuf calls will destroy
 *   this line.
 */
char *Get_inbuf_str()
{
	char *s = 0;
	/* DEBUG4("Get_inbuf_str: buffer '%s'", Inplace); */
	Inlen = strlen(Inplace);
	if( Inplace != Inbuf ){
		/* we readjust the locations */
		memmove( Inbuf, Inplace, Inlen+1 );
		Inplace = Inbuf;
	}
	/* remove \r */
	while( (s = strchr( Inbuf, '\r' ) ) ){
		memmove( s, s+1, strlen(s)+1 );
	}
	s = strpbrk( Inbuf, Line_ends );
	if( s ){
		*s++ = 0;
		Inplace = s;
		s = Inbuf;
		/* DEBUG4("Get_inbuf_str: found '%s', buffer '%s'", s, Inplace); */
		Pr_status( s, 0 );
	}
	Inlen = strlen(Inplace);
	return( s );
}


/*
 * Monitorput buffer management
 *  Set up and put values into an input buffer for
 *  scanning purposes
 */
void Init_monitorbuf()
{
	Monitormax = LARGEBUFFER;
	Monitorbuf = realloc_or_die( Monitorbuf, Monitormax+1,__FILE__,__LINE__);
	Monitorbuf[Monitormax] = 0;
	Monitorplace = Monitorbuf;
	Monitorlen = 0;
	Monitorbuf[0] = 0;
}

void Put_monitorbuf_str( char *str )
{
	int len = strlen( str );
	Put_monitorbuf_len( str, len );
}

void Put_monitorbuf_len( char *str, int len )
{
	Monitorlen = strlen(Monitorplace);
	if( Monitorplace != Monitorbuf ){
		/* we readjust the locations */
		memmove( Monitorbuf, Monitorplace, Monitorlen+1 );
		Monitorplace = Monitorbuf;
	}
	if( Monitormax - Monitorlen <= (len+1) ){
		Monitormax += ((LARGEBUFFER + len+1)/1024)*1024;
		Monitorbuf = realloc_or_die( Monitorbuf, Monitormax+1,__FILE__,__LINE__);
		if( !Monitorbuf ){
			Errorcode = JABORT;
			logerr_die( "Put_outbuf_len: realloc %d failed", len );
		}
	}
	memmove( Monitorplace+Monitorlen, str, len+1 );
	Monitorlen += len;
	Monitorbuf[Monitorlen] = 0;
	DEBUG4("Put_monitorbuf_len: buffer '%s'", Monitorbuf );
}

/*
 * char *Get_monitorbuf_str()
 * return a pointer to a complete line in the input buffer
 *   This will, in effect, remove the line from the
 *   buffer, and it should be saved.
 * Any of the Put_monitorbuf or Get_monitorbuf calls will destroy
 *   this line.
 */
char *Get_monitorbuf_str()
{
	char *s = 0;
	/* DEBUG4("Get_monitorbuf_str: buffer '%s'", Monitorplace); */
	Monitorlen = strlen(Monitorplace);
	if( Monitorplace != Monitorbuf ){
		/* we readjust the locations */
		memmove( Monitorbuf, Monitorplace, Monitorlen+1 );
		Monitorplace = Monitorbuf;
	}
	/* remove \r */
	while( (s = strchr( Monitorbuf, '\r' ) ) ){
		memmove( s, s+1, strlen(s)+1 );
	}
	s = strpbrk( Monitorbuf, Line_ends );
	if( s ){
		*s++ = 0;
		Monitorplace = s;
		s = Monitorbuf;
		/* DEBUG4("Get_monitorbuf_str: found '%s', buffer '%s'", s, Monitorplace); */
		Pr_status( s, 1 );
	}
	Monitorlen = strlen(Monitorplace);
	return( s );
}

/*
 * Printer Status Reporting
 * PJL:
 *  We extract info of the form:
 *  @PJL XXXX P1 P2 ...
 *  V1
 *  V2
 *  0
 *    we then create a string 
 *  P1.P2.P3=xxxx
 *  - we put this into the 'current status' array
 * PostScript:
 *  We extract info of the form:
 *  %%[ key: value value key: value value ]%%
 *    we then create strings
 *     key=value value
 *  - we put this into the 'current status' array
 *
 */

void Pr_status( char *str, int monitor_status )
{
	char *s, *t;
	int c, i, pjl_line, found;
	char *ps_str;
	struct line_list l;
	static char *pjlvar, *eq_line;
	char *value;

	c = pjl_line = found = 0;
	ps_str = value = eq_line = 0;
	while( str && isspace( cval(str) ) ) ++str;
	DEBUG4("Pr_status: start str '%s', pjlvar '%s'", str, pjlvar);
	/* if(DEBUGL4)Dump_line_list("Pr_status - before Devstatus", &Devstatus); */

	Init_line_list(&l);
	if( Logall && str ){
		logmsg( "Pr_status: printer status '%s'", str );
	}
	/* if the previous line was a PJL line,  then the
	 * last entry may be a variable name and the next
	 * line a value for it.
	 */
	if( (ps_str = strstr( str,"%%[" )) ){
		ps_str += 3;
	}
	if( (s = strpbrk(str,Whitespace)) ){
		c = cval(s);
		*s = 0;
	}
	pjl_line = (strstr( str, "@PJL") != 0 );
	eq_line = strchr( str, '=' );
	if( s ) *s = c;

	/* now we check to see if we have a value for pjl variable */
	if( pjlvar && (pjl_line || eq_line || ps_str ) ){
		free(pjlvar); pjlvar = 0;
	}
	if( pjlvar ){
		DEBUG4("Pr_status: setting PJL var '%s' to '%s'", pjlvar, str);
		Set_str_value( &Devstatus, pjlvar, str );
		free(pjlvar); pjlvar = 0;
		goto done;
	}

	/* now we check for xx=value entries */
	if( eq_line ){
		*eq_line = 0;
		lowercase(str);
		*eq_line = '=';
		Check_device_status(str);
		DEBUG4("Pr_status: setting '%s'", str);
		Add_line_list( &Devstatus, str, Value_sep, 1, 1 );
		goto done;
	}
	if( pjl_line ){
		DEBUG4("Pr_status: doing PJL status on '%s'", str);
		if( (s = strstr( str, " ECHO ") ) ){
			/* we have the echo value */
			Set_str_value(&Devstatus,"echo",s+6);
			DEBUG4("Pr_status: found echo '%s'", s+6 );
		} else {
			Split(&l, str, Whitespace, 0, 0, 0, 1, 0 );
			pjlvar = safestrdup(l.list[l.count-1],__FILE__,__LINE__);
			Free_line_list( &l );
			lowercase(pjlvar);
			DEBUG4("Pr_status: PJL var '%s'", pjlvar);
		}
	} else if( monitor_status || ps_str ){
		/* we do Postscript status */
		if( !ps_str ) ps_str = str;
		Split(&l,ps_str,Whitespace,0,0,0,1,0 );
		DEBUG4("Pr_status: PostScript info '%s'", ps_str );
		if(DEBUGL4)Dump_line_list("Process_job: PostScript", &l);
		for( i = 0; i < l.count; ++i ){
			s = l.list[i];
			found = 0;
			t = 0;
			DEBUG4("Pr_status: list [%d]='%s', value %s", i, s, value );
			if( strstr( s, "]%%" ) || strstr( s, "%%[" )
				|| (found = (strlen(s) > 1
						&& (t = strchr(s,':')) && t[1] == 0)) ){
				if( value ){
					DEBUG4("Pr_status: found ps status '%s'", value );
					Check_device_status(value);
					Add_line_list( &Devstatus, value, Value_sep, 1, 1 );
					free( value ); value = 0;
				}
				c = 0;
			}
			if( found ){
				*t = '=';
				value = safestrdup(s,__FILE__,__LINE__);
				lowercase(value);
				c = 0;
			} else if( value ){
				t = value;
				value = safestrdup3(value,c?" ":"",s,__FILE__,__LINE__);
				free(t);
				c = 1;
			}
		}
		if( value ){
			DEBUG5("Pr_status: found ps status '%s'", value );
			Check_device_status(value);
			Add_line_list( &Devstatus, value, Value_sep, 1, 1 );
			free( value ); value = 0;
		}
		Free_line_list( &l );
	}
  done:
	/* if(DEBUGL4)Dump_line_list("Pr_status - Devstatus", &Devstatus); */
	;
}

/*
 * Check_device_status( char *line )
 *  - the line has to have the form DEVICE="nnn"
 *  We log the message IF it is different from the last one
 */

void Check_device_status( char *line )
{
	char *old, *value;
	int c = 0;

	DEBUG4("Check_device_status: '%s'", line );
	if( (value = strpbrk( line, Value_sep )) ){
		c = *value;
		*value++ = 0;
		DEBUG4("Check_device_status: key '%s', value '%s'", line, value );
		if( !(old = Find_str_value( &Devstatus, line, Value_sep ))
			|| strcmp(old,value) ){
			if( !strcasecmp(line,"code") ){
				DEBUG4("Check_device_status: CODE '%s'", value );
				/* we now have to check to see if we log this */
				if( Find_first_key( &Pjl_quiet_codes, value, Value_sep, 0 ) ){
					/* ok, we log it */
					char *t;
					t = Check_code( value );
					DEBUG4("Check_device_status: code '%s' = '%s'", value, t);
					logmsg("Check_device_status: code = %s, '%s'", value, t );
				}
			} else if( strstr(line,"status")
				|| strstr(line,"error") || strstr(line,"offending") ){
				DEBUG4("Check_device_status: old status '%s', new '%s'",
					old, value );
				logmsg("Check_device_status: %s = '%s'", line, value );
			}
		}
		value[-1] = c;
	}
}


/*
 * void Initialize_parms( struct line_list *list, struct keyvalue *valuelist )
 *  Initialize variables from values in a parameter list
 *  This list has entries of the form key=value
 *  If the variable is a flag,  then entry of the form 'key' will set
 *    it,  and key@ will clear it.
 *
 *  list = key list, i.e. - strings of form key=value
 *  count = number of entries in key list
 *  valuelist = variables and keys to use to set them
 */

void Initialize_parms( struct line_list *list, struct keyvalue *valuelist )
{
	struct keyvalue *v;
	char *arg, *convert;
	int n;
	if( valuelist == 0 ) return;
	for( v = valuelist; v->key; ++v ){
		if( v->key[0] == 0 ) continue;
		if( Find_first_key( list, v->key, Value_sep, 0) ) continue;
		arg = Find_exists_value( list, v->key, Value_sep );
		DEBUG4("Initialize_parms: key '%s'='%s'", v->key, arg );
		if( arg ) switch( v->kind ){
		case STRV: *(char **)v->var = arg; break;
		case INTV:
		case FLGV:
			convert = arg;
			n = strtol( arg, &convert, 10 );
			if( convert != arg ){
				*(int *)v->var = n;
			} else {
				*(int *)v->var = ( !strcasecmp( arg, "yes" )
				|| !strcasecmp( arg, "on" ));
			}
			break;
		}
	}
}

/*
 * Dump_parms( char *title, struct keyvalue *v )
 *  Dump the names, config file tags, and current values of variables
 *   in a value list.
 */
void Dump_parms( char *title, struct keyvalue *v )
{
	logDebug( "Dump_parms: %s", title );
	for( v = Valuelist; v->key; ++v ){
		switch( v->kind ){
		case STRV:
			logDebug( " '%s' (%s) STRV = '%s'", v->varname, v->key, *(char **)v->var ); break;
		case INTV:
			logDebug( " '%s' (%s) INTV = '%d'", v->varname, v->key, *(int *)v->var ); break;
		case FLGV:
			logDebug( " '%s' (%s) FLGV = '%d'", v->varname, v->key, *(int *)v->var ); break;
		}
	}
}

/*
 * Process the job  - we do this for both IF and OF modes 
 */
void Process_job()
{
	char *s;
	struct line_list l;
	time_t current_t;
	int elapsed, startpagecount = 0, endpagecount = 0;

	Init_line_list( &l );
	Init_outbuf();

	logmsg( "Process_job: setting up printer");

	if( (s = Find_str_value( &Model, "pjl_only", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_only, s, List_sep, 1, 0, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_only", &Pjl_only);
	}
	if( (s = Find_str_value( &Model, "pjl_except", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_except, s, List_sep, 1, 0, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_except", &Pjl_except);
	}
	if( (s = Find_str_value( &Model, "pjl_vars_set", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_vars_set, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_vars_set", &Pjl_vars_set);
	}
	if( (s = Find_str_value( &Model, "pjl_vars_except", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_vars_except, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_vars_except", &Pjl_vars_except);
	}
	if( (s = Find_str_value( &Model, "pjl_user_opts", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_user_opts, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_user_opts", &Pjl_user_opts);
	}
	if( (s = Find_str_value( &Model, "pcl_user_opts", Value_sep)) ){
		lowercase(s);
		Split( &Pcl_user_opts, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pcl_user_opts", &Pcl_user_opts);
	}
	if( (s = Find_str_value( &Model, "ps_user_opts", Value_sep)) ){
		lowercase(s);
		Split( &Ps_user_opts, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Ps_user_opts", &Ps_user_opts);
	}
	if( (s = Find_str_value( &Model, "pjl_error_codes", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_error_codes, s, "\n", 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_error_codes", &Pjl_error_codes);
	}
	if( (s = Find_str_value( &Model, "pjl_quiet_codes", Value_sep)) ){
		lowercase(s);
		Split( &Pjl_quiet_codes, s, List_sep, 1, Value_sep, 1, 1, 0 );
		if(DEBUGL4)Dump_line_list("Process_job: Pjl_quiet_codes", &Pjl_quiet_codes);
	}


	DEBUG1("Process_job: sync and pagecount");
	if( Status ){
		Do_sync(Sync_timeout);
		startpagecount = Do_pagecount(Pagecount_timeout);
		if( Appsocket && Device ){
			/* we shut down the socket connection */
			DEBUG1("Process_job: Appsocket device close");
			shutdown(1,1);
			Read_until_eof(Sync_timeout);
			Open_device( Device );
			DEBUG1("Process_job: Appsocket device open done");
		}
	}
	time( &current_t );
	elapsed = current_t - Start_time;
	Do_accounting(1, 0, startpagecount, 0 );


	DEBUG1("Process_job: doing 'init'");

	Init_outbuf();
	if( !Find_first_key( &Model, "init", Value_sep, 0) ){
		s  = Find_str_value( &Model, "init", Value_sep);
		DEBUG1("Process_job: 'init'='%s'", s);
		Split( &l, s, List_sep, 0, 0, 0, 1, 0 );
		Resolve_list( "", &l, Put_pcl );
		Free_line_list( &l );
	}
	if( Pjl ){
		DEBUG1("Process_job: doing pjl");
		Put_outbuf_str( PJL_UEL_str );
		Put_outbuf_str( PJL_str );
		Pjl_job();
		Pjl_console_msg(1);
		if( !Find_first_key( &Model, "pjl_init", Value_sep, 0) ){
			s  = Find_str_value( &Model, "pjl_init", Value_sep);
			DEBUG1("Process_job: 'pjl_init'='%s'", s);
			Split( &l, s, List_sep, 0, 0, 0, 1, 0 );
			Resolve_list( "pjl_", &l, Put_pjl );
			Free_line_list( &l );
		}
		if( !OF_Mode ){
			DEBUG1("Process_job: 'pjl' and Zopts");
			Resolve_user_opts( "pjl_", &Pjl_user_opts, &Zopts, Put_pjl );
			DEBUG1("Process_job: 'pjl' and Topts");
			Resolve_user_opts( "pjl_", &Pjl_user_opts, &Topts, Put_pjl );
		}
	}
	if( Write_out_buffer( Outlen, Outbuf, Job_timeout ) ){
		Errorcode = JFAIL;
		fatal("Process_job: timeout");
	}
	Init_outbuf();
	if( OF_Mode ){
		int fd;
		logmsg( "Process_job: starting OF mode passthrough" );
		while( (fd = Process_OF_mode( Job_timeout )) >= 0 ){
			if( Status_fd > 0 ){
				close( Status_fd );
				Status_fd = -2;
			}
			Set_nonblock_io(1);
			kill(getpid(), SIGSTOP);
			logmsg( "Process_OF_mode: active again");
			if( Appsocket ){
				Open_device( Device );
			}
			Set_nonblock_io(1);
			if( Status ){
				Set_nonblock_io(1);
			}
		}
		logmsg( "Process_job: ending OF mode passthrough" );
	} else {
		logmsg( "Process_job: sending job file" );
		Send_job();
		logmsg( "Process_job: sent job file" );
	}
	logmsg( "Process_job: doing cleanup");
	Init_outbuf();
	if( Pjl ){
		DEBUG1("Process_job: doing pjl at end");
		Put_outbuf_str( PJL_UEL_str );
		Put_outbuf_str( PJL_str );
		Pjl_console_msg(1);
		Pjl_eoj();
		if( !Find_first_key( &Model, "pjl_term", Value_sep, 0) ){
			s  = Find_str_value( &Model, "pjl_term", Value_sep);
			DEBUG1("Process_job: 'pjl_term'='%s'", s);
			Split( &l, s, List_sep, 0, 0, 0, 1, 0 );
			Resolve_list( "pjl_", &l, Put_pjl );
			Free_line_list( &l );
		}
	}
	if( !Find_first_key( &Model, "term", Value_sep, 0) ){
		s  = Find_str_value( &Model, "term", Value_sep);
		DEBUG1("Process_job: 'term'='%s'", s);
		Split( &l, s, List_sep, 0, 0, 0, 1, 0 );
		Resolve_list( "", &l, Put_pcl );
		Free_line_list( &l );
	}
	if( Write_out_buffer( Outlen, Outbuf, Job_timeout ) ){
		Errorcode = JFAIL;
		fatal("Process_job: timeout");
	}
	Init_outbuf();
	DEBUG1("Process_job: end sync and pagecount");
	if( Status ){
		/* terminate reading from the socket
		 * this should cause the printer to simply send back status 
		 */
		Do_waitend(Job_timeout);
		endpagecount = Do_pagecount(Job_timeout);
	}
	if( Pjl && Pjl_console ){
		Init_outbuf();
		Pjl_console_msg(0);
		DEBUG1("Process_job: doing pjl at end");
		Put_outbuf_str( PJL_UEL_str );
		/* Put_outbuf_str( PJL_str ); */
		if( Write_out_buffer( Outlen, Outbuf, Job_timeout ) ){
			Errorcode = JFAIL;
			fatal("Process_job: timeout");
		}
		Init_outbuf();
	}

	time( &current_t );
	elapsed = current_t - Start_time;
	Do_accounting(0, elapsed, endpagecount, endpagecount-startpagecount );
	logmsg( "Process_job: done" );
}

/*
 * Put_pjl( char *s )
 *  write pjl out to the output buffer
 *  check to make sure that the line is PJL code only
 *  and that the PJL option is in a list
 */

void Put_pjl( char *s )
{
	struct line_list l, wl;
	int i;

	Init_line_list(&l);
	Init_line_list(&wl);

	DEBUG4("Put_pjl: orig '%s'", s );
	if( s == 0 || *s == 0 ) return;
	s = Fix_option_str( s, 0, 1, 0 );
	Strip_leading_spaces(&s);
	Split( &l, s, "\n", 0, 0, 0, 1, 0 );
	if( s ){ free(s); s = 0; }
	for( i = 0; i < l.count; ++i ){
		/* check for valid PJL */
		Split( &wl, l.list[i], Whitespace, 0, 0, 0, 0, 0 );
		if( wl.count == 0 || strcmp("@PJL", wl.list[0]) ){
			continue;
		}
		/* key must be in PJL_ONLY list if supplied, and NOT in
		 * PJL_ACCEPT
		 */
		s = wl.list[1];
		lowercase(s);
		DEBUG4("Put_pjl: trying '%s'", s );
		if( wl.count > 1 &&
			(  Find_first_key( &Pjl_only, s, 0, 0)
			||  !Find_first_key( &Pjl_except, s, 0, 0)
			) ){
			continue;
		}
		DEBUG4("Put_pjl: '%s'", l.list[i] );
		Put_outbuf_str( l.list[i] );
		Put_outbuf_str( "\n" );
		Free_line_list( &wl );
	}
	Free_line_list(&l);
}

void Put_pcl( char *s )
{
	DEBUG4("Put_pcl: orig '%s'", s );
	s = Fix_option_str( s, 1, 1, 0 );
	DEBUG4("Put_pcl: final '%s'", s );
	Put_outbuf_str( s );
	if(s)free(s);s=0;
}


void Put_ps( char *s )
{
	DEBUG4("Put_fixed: orig '%s'", s );
	s = Fix_option_str( s, 0, 0, 0 );
	DEBUG4("Put_fixed: final '%s'", s );
	Put_outbuf_str( s );
	if(s)free(s);s=0;
	Put_outbuf_str( "\n" );
}


void Put_fixed( char *s )
{
	DEBUG4("Put_fixed: orig '%s'", s );
	s = Fix_option_str( s, 0, 0, 0 );
	DEBUG4("Put_fixed: final '%s'", s );
	Put_outbuf_str( s );
	if(s)free(s);s=0;
}


/*
 * Set_non_block_io(fd)
 * Set_block_io(fd)
 *  Set blocking or non-blocking IO
 *  Dies if unsuccessful
 * Get_nonblock_io(fd)
 *  Returns O_NONBLOCK flag value
 */

int Get_nonblock_io( int fd )
{
	int mask;
	/* we set IO to non-blocking on fd */

	if( (mask = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
		Errorcode = JABORT;
		logerr_die( "ifhp: fcntl fd %d F_GETFL failed", fd );
	}
	DEBUG4("Get_nonblock_io: fd %d, current flags 0x%x, O_NONBLOCK=0x%x",fd, mask, O_NONBLOCK);
	return( mask & O_NONBLOCK);
}

int Set_nonblock_io( int fd )
{
	int mask, omask;
	/* we set IO to non-blocking on fd */

	if( (omask = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
		Errorcode = JABORT;
		logerr_die( "Set_nonblock_io: fcntl fd %d F_GETFL failed", fd );
	}
	omask |= O_NONBLOCK;
	if( (mask = fcntl( fd, F_SETFL, omask ) ) == -1 ){
		DEBUG4( "Set_nonblock_io: fcntl fd %d F_SETFL failed", fd );
	}
	DEBUG4( "Set_nonblock_io: F_SETFL 0x%x, result 0x%x", omask, mask );
	if( (mask = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
		Errorcode = JABORT;
		logerr_die( "Set_nonblock_io: fcntl fd %d F_GETFL failed", fd );
	}
	DEBUG4( "Set_nonblock_io: setting 0x%x, result 0x%x", omask, mask );
	return( mask & O_NONBLOCK );
}

int Set_block_io( int fd )
{
	int mask, omask;
	/* we set IO to blocking on fd */

	if( (omask = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
		Errorcode = JABORT;
		logerr_die( "Set_nonblock_io: fcntl fd %d F_GETFL failed", fd );
	}
	omask &= ~O_NONBLOCK;
	if( (mask = fcntl( fd, F_SETFL, omask ) ) == -1 ){
		DEBUG1( "Set_block_io: fcntl fd %d F_SETFL failed", fd );
	}
	DEBUG4( "Set_block_io: F_SETFL 0x%x, result 0x%x", omask, mask );
	if( (mask = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
		Errorcode = JABORT;
		logerr_die( "Set_block_io: fcntl fd %d F_GETFL failed", fd );
	}
	DEBUG4( "Set_block_io: setting 0x%x, result 0x%x", omask, mask );
	return( mask & O_NONBLOCK );
}

/*
 * Read_write_timeout( int readfd, int *flag,
 *	int writefd, char *buffer, int len, int timeout,
 *	int monitorfd )
 *  Write the contents of a buffer to the file descriptor
 *  int readfd: 
 *  int writefd: write file descriptor
 *  char *buffer, int len: buffer and number of bytes
 *  int timeout:  timeout in seconds
 *  Returns: numbers of bytes left to write
 *  - if we are writing (len > 0), then end of write terminates
 *  - if we are only reading (len == 0, flag != 0),
 *    then we wait for the readtimeout length
 */

int Read_write_timeout( int readfd, int *flag,
	int writefd, char *buffer, int len, int timeout,
	int monitorfd )
{
	time_t start_t, current_t;
	int elapsed, m, err, done;
	struct timeval timeval, *tp;
    fd_set readfds, writefds; /* for select */

	DEBUG4(
	"Read_write_timeout: write(fd %d, len %d) timeout %d, read fd %d, monitor fd %d",
		writefd, len, timeout, readfd, monitorfd);

	time( &start_t );

	*flag = 0;
	done = 0;
	/* do we do only a read ? */
	if( Status == 0 && len == 0 ){
		Errorcode = JABORT;
		fatal( "Read_write_timeout: TURKEY WARNING!!! reading status when Status '%d' and len = '%d'",
			Status, len );
	}
	if( Status && len == 0 && readfd == 0 && monitorfd == 0 ){
		Errorcode = JABORT;
		fatal( "Read_write_timeout: TURKEY WARNING!!! Only need status and readfd '%d' and monitorfd = '%d'",
			readfd, monitorfd );
	}
	while( !done ){
		if( timeout > 0 ){
			time( &current_t );
			elapsed = current_t - start_t;
			m = timeout - elapsed;
			DEBUG4("Read_write_timeout: timeout left %d", m );
			if( m <= 0 ){
				done = 1;
				break;
			}
			memset( &timeval, 0, sizeof(timeval) );
			timeval.tv_sec = m;
			tp = &timeval;
		} else {
			tp = 0;
		}
		FD_ZERO( &writefds );
		FD_ZERO( &readfds );
		m = 0;
		if( len > 0 && writefd >= 0 ){
			FD_SET( writefd, &writefds );
			DEBUG4("Read_write_timeout: setting write fd %d", writefd );
			if( m <= writefd ) m = writefd+1;
		}
		if( readfd >= 0 ){
			FD_SET( readfd, &readfds );
			DEBUG4("Read_write_timeout: setting read fd %d", readfd );
			if( m <= readfd ) m = readfd+1;
		}
		if( monitorfd >= 0 ){
			FD_SET( monitorfd, &readfds );
			DEBUG4("Read_write_timeout: setting monitor fd %d", monitorfd );
			if( m <= monitorfd ) m = monitorfd+1;
		}
		DEBUG4("Read_write_timeout: starting rw select - max %d", m );
		errno = 0;
		m = select( m,
			FD_SET_FIX((fd_set *))&readfds,
			FD_SET_FIX((fd_set *))&writefds,
			FD_SET_FIX((fd_set *))0, tp );
		err = errno;
		DEBUG4("Read_write_timeout: reading and writing, select returned %d, errno %d",
			m, err );
		if( m < 0 ){
			/* error */
			if( err != EINTR ){
				Errorcode = JFAIL;
				logerr_die( "Read_write_timeout: select error" );
			}
			continue;
		} else if( m == 0 ){
			/* timeout */
			done = 1;
			break;
		}
		if( readfd >=0 && FD_ISSET( readfd, &readfds ) ){
			DEBUG4("Read_write_timeout: read possible on fd %d", readfd );
			*flag |= 1;
			done = 1;
		}
		if( monitorfd >=0 && FD_ISSET( monitorfd, &readfds ) ){
			DEBUG4("Read_write_timeout: monitor possible on fd %d", monitorfd );
			*flag |= 2;
			done = 1;
		}
		if( !done && (writefd>=0 && FD_ISSET( writefd, &writefds )) ){
			DEBUG4("Read_write_timeout: write possible on fd %d, nonblocking %d",
				writefd, Get_nonblock_io(writefd) );
			m = write( writefd, buffer, len );
			err = errno;
			DEBUG4("Read_write_timeout: wrote %d", m );
			if( m < 0 ){
				if(
#ifdef EWOULDBLOCK
					err == EWOULDBLOCK
#else
# ifdef EAGAIN
					err == EAGAIN
# else
#error  No definition for EWOULDBLOCK or EAGAIN
# endif
#endif
								){
					DEBUG4( "Read_write_timeout: write would block but select says not!" );
					errno = err;
				} else if( err == EINTR ){
					/* usually timeout stuff */
					;
				} else {
					logerr( "Read_write_timeout: write error" );
					len = -1;
					done = 1;
					errno = err;
				}
			} else {
				len -= m;
				buffer += m;
				if( len == 0 ){
					done = 1;
				}
			}
		}
	}
	DEBUG4("Read_write_timeout: returning %d, flag %d",
		len, *flag);
	return( len );
}

/*
 * Read status information from FD
 */
int Read_status_line( int fd )
{
	int count;
	char monitorbuff[SMALLBUFFER];

	monitorbuff[0] = 0;
	/* we read from stdout */
	count = read( fd, monitorbuff, sizeof(monitorbuff) - 1 );
	if( count > 0 ){
		monitorbuff[count] = 0;
		Put_inbuf_str( monitorbuff );
	}
	DEBUG2("Read_status_line: read fd %d, count %d, '%s'",
		fd, count, monitorbuff );
	return( count );
}

/*
 * Read monitor status information from FD
 */
int Read_monitor_status_line( int fd )
{
	int count;
	char monitorbuff[SMALLBUFFER];

	monitorbuff[0] = 0;
	/* we read from stdout */
	count = read( fd, monitorbuff, sizeof(monitorbuff) - 2 );
	if( count > 0 ){
		monitorbuff[count++] = '\n';
		monitorbuff[count] = 0;
		Put_monitorbuf_str( monitorbuff );
	} else if( count < 0 ){
		DEBUG1("Read_monitor_status_line: error '%s'", Errormsg(errno) );
	}
	DEBUG2("Read_monitor_status_line: read fd %d, count %d, '%s'",
		fd, count, monitorbuff );
	return( count );
}

/*
 * int Write_out_buffer( int outlen, char *outbuf, int maxtimeout )
 * We output the buffer to fd1, but we also read the status line
 * information as well from fd 1 if Status is set
 */

int Write_out_buffer( int outlen, char *outbuf, int maxtimeout )
{
	time_t current_time;
	int elapsed, timeout, len, flag, read_fd = 1, n;

	if( Status == 0 ){
		read_fd = -1;
	}

	DEBUG1("Write_out_buffer: write len %d, read_fd %d, maxtimeout %d",
		outlen, read_fd, maxtimeout );
	while( outlen > 0 && outbuf ){ 
		time( &current_time );
		elapsed = current_time - Start_time;
		timeout = 0;
		if( maxtimeout > 0 ){
			timeout = maxtimeout - elapsed; 
			if( timeout <= 0 ) break;
		}
		DEBUG1("Write_out_buffer: timeout %d, len %d", timeout, outlen );
		len = Read_write_timeout( read_fd, &flag,
			1, outbuf, outlen, timeout, Monitor_fd );
		DEBUG1("Write_out_buffer: left to write %d, flag %d", len, flag );
		if( len < 0 ){
			outlen = len;
		} else if( len >= 0 ){
			outbuf += (outlen - len);
			outlen = len;
		}
		if( flag & 1 ){
			n = Read_status_line( read_fd );
			DEBUG1("Write_out_buffer: Read_status_line returned %d", n );
			if( n < 0 ){
				logerr("Write_out_buffer: read from printer failed");
				break;
			} else if( (n == 0 && !Ignore_eof) ){
				logmsg("Write_out_buffer: EOF while reading status from printer");
				break;
			} else if( n > 0 ) {
				/* get the status */
				while( Get_inbuf_str() );
			}
		}
		if( flag & 2 ){
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				/* get the status */
				while( Get_monitorbuf_str() );
			}
		}
	}
	DEBUG1("Write_out_buffer: done, returning %d", outlen );
	return( outlen );
}

/*
 * void Resolve_key_val( char *prefix, char *key_val,
 *	Wr_out routine )
 * - determine what to do for the specified key and value
 *   we first check to see if it is a builtin
 *   if it is, then we do not need to process further
 */

 struct line_list setvals;

void Resolve_key_val( char *prefix, char *key_val, Wr_out routine )
{
	char *value, *id = 0, *prefix_id = 0, *s;
	int c = 0, done = 0;
	int depth = setvals.count;

	if( (value = strpbrk( key_val, Value_sep )) ){
		c = *value; *value = 0;
	}
	/* we expand the key here */
	id = Fix_option_str( key_val, 0, 0, 0);
	lowercase( id );
	if( value ){
		*value++ = c;
		Check_max(&setvals,1);
		setvals.list[setvals.count++]
			= safestrdup3(id,"=",value,__FILE__,__LINE__);
	}
	/* decide if it is a built-in function or simply forcing a string out */
	DEBUG4("Resolve_key_val: prefix '%s', id '%s'='%s'",
		prefix,id,value);
	if( !done ){
		done = Builtin( prefix, id, value, routine);
		DEBUG4("Resolve_key_val: '%s' is builtin '%d'", id, done );
	}
	if( !done && !strncasecmp(prefix,"pjl", 3) ){
		done = Pjl_setvar( prefix, id, value, routine);
		DEBUG4("Resolve_key_val: '%s' is PJL '%d'", id, done );
	}
	if( !done ){
		prefix_id = safestrdup2( prefix, id,__FILE__,__LINE__);
		if( (s = Find_str_value( &Model, prefix_id, Value_sep )) ){
			DEBUG4("Resolve_key_val: string value of id '%s'='%s'", id, s);
			if( *s == '[' ){
				struct line_list l;

				Init_line_list( &l );
				Split( &l, s, List_sep, 0, 0, 0, 1, 0 );
				Resolve_list( prefix, &l, routine );
				Free_line_list( &l );
				done = 1;
			} else {
				routine( s );
				done = 1;
			}
		}
		if( prefix_id ) free(prefix_id); prefix_id = 0;
	}
	if( !done && value && *value && !Is_flag(value, 0) ){
		routine( Fix_val(value) );
		done = 1;
	}
	for( c = depth; c < setvals.count; ++c ){
		if( (s = setvals.list[c]) ) free(s);
		setvals.list[c] = 0;
	}
	setvals.count = depth;
	if(id)free(id); id = 0;
}

int Is_flag( char *s, int *v )
{
	int n = 0;
	if( s && *s && strlen(s) == 1 && isdigit(cval(s)) &&
		(n = (*s == '0' || *s == '1' || *s == '@')) && v ){
		if( *s == '@' ){
			*v = 0;
		} else {
			*v = *s - '0';
		}
	}
	return( n );
}

/*
 * void Resolve_list( char *prefix, char **list, int count, W_out routine )
 * Resolve the actions specified by a list
 * List has the form [ xx xx xx or [xx xx xx
 */

void Resolve_list( char *prefix, struct line_list *l, Wr_out routine )
{
	int i;
	char *s;
	DEBUG4("Resolve_list: prefix '%s', count %d", prefix, l->count );
	if(DEBUGL4)Dump_line_list("Resolve_list",l);
	for( i = 0; i < l->count; ++i ){
		s = l->list[i];
		DEBUG4("Resolve_list: prefix '%s', [%d]='%s'", prefix, i, s );
		if( (*s == '[') || (*s == ']')) ++s;
		if( *s ){
			Resolve_key_val( prefix, s, routine );
		}
	}
}

/*
 * void Resolve_user_opts( char *prefix, struct line_list *only, *opts,
 *		W_out routine )
 * Resolve the actions specified by a the user and allowed as options
 * 
 */

void Resolve_user_opts( char *prefix, struct line_list *only,
	struct line_list *l, Wr_out routine )
{
	int i, c = 0, cmp;
	char *id = 0, *s, *value;
	DEBUG4("Resolve_user_opts: prefix '%s'", prefix );
	if(DEBUGL4)Dump_line_list("Resolve_user_opts - only",only);
	if(DEBUGL4)Dump_line_list("Resolve_user_opts - list",l);
	for( i = 0; i < l->count; ++i ){
		if( id ) free(id); id = 0;
		s = l->list[i];
		if( s == 0 || *s == 0 ) continue;
		id = safestrdup(s,__FILE__,__LINE__);
		if( (value = strpbrk( id, Value_sep )) ){ c = *value; *value = 0; }
		cmp = Find_first_key( only, id, Value_sep, 0 );
		DEBUG4("Resolve_user_opts: id '%s', Find_first=%d, value '%s'",
			id, cmp, value?(value+1):0 );
		if( value ){ *value = c; }
		if( cmp == 0 ){
			Resolve_key_val( prefix, id, routine );
		}
	}
	if( id ) free(id); id = 0;
}


/*
 * char *Fix_option_str( char *str, int remove_ws, int trim, int one_line )
 *  if( remove_ws ) then remove all isspace() characters
 *  if( trim) find and remove all white space before and after \n
 *  if( one_line) change \n to space
 *  do escape substitutions
 */

char *Fix_option_str( char *str, int remove_ws, int trim, int one_line )
{
	char *s, *t, *value;
	char num[SMALLBUFFER];
	char fmt[127];
	int c, insert, rest, len, lookup;
	char *sq, *eq;

	if( str == 0 ){ return str; }
	str = safestrdup( str,__FILE__,__LINE__);
	DEBUG5("Fix_option_str: orig '%s', remove_ws %d, trim %d, one_line %d",
		str, remove_ws, trim, one_line );
	/* now we do escape sequences */
	for( s = str; (s = strchr( s, '\\' )); ){
		c = s[1];
		len = 1;
		insert = 0;

		DEBUG4("Fix_option_str: escape at '%s', '%c'", s, c );
		s[0] = c;
		switch( c ){
		case 'r': s[0] = '\r'; break;
		case 'n': s[0] = '\n'; break;
		case 't': s[0] = '\t'; break;
		case 'f': s[0] = '\f'; break;
		}
		if( isdigit( c ) ){
			for( len = 0;
				len < 3 && (num[len] = s[1+len]) && isdigit(cval(num+len));
				++len );
			num[len] = 0;
			s[0] = rest = strtol( num, 0, 8 );
			DEBUG5("Fix_option_str: number '%s', len %d, convert '%d'",
				num, len, rest );
		}
		memmove( s+1, s+len+1, strlen(s+len)+1 );
		s += 1;
		DEBUG5("Fix_option_str: after simple escape c '%c', '%s', next '%s'",
			c, str, s );
		/* we have \%fmt{key} */
		if( c == '%' ){
			--s;
			DEBUG5("Fix_option_str: var sub into '%s'", s );
			if( (sq = strpbrk( s, "[{" )) ){
				lookup = c = sq[0];
				*sq++ = 0;
				eq = strpbrk( sq, "]}" );
				if( eq ){
					*eq++ = 0;
				} else {
					eq = sq + strlen(sq);
				}
				SNPRINTF( fmt, sizeof(fmt), "%%%s", s+1 );
				DEBUG5("Fix_option_str: trying fmt '%s'", fmt );
				t = fmt+strlen(fmt)-1;
				c = *t;
				if( !islower(cval(t)) ){
					strcpy(fmt,"%d");
					c = 'd';
				}
				value = Find_sub_value( lookup, sq, c == 's' );
				DEBUG3("Fix_option_str: using fmt '%s', type '%c', value '%s'",
					fmt, c, value );
				while(isspace(cval(value))) ++value;
				value = safestrdup(value,__FILE__,__LINE__);
				if( value[0] == '[' ){
					char *t;
					value[0] = ' ';
					if( (t = strrchr( value, ']')) ) *t = ' ';
					DEBUG3("Fix_option_str: fixing value '%s'", value );
					t = Fix_option_str( value, remove_ws, trim, one_line );
					free(value);
					value = t;
					/* remove leading and trailing white space */
					DEBUG3("Fix_option_str: fixed value '%s'", value );
				}
				num[0] = 0;
				switch( c ){
				case 'o': case 'd': case 'x':
					SNPRINTF( num, sizeof(num), fmt, strtol(value, 0, 0) );
					break;
				case 'f': case 'g': case 'e':
					SNPRINTF( num, sizeof(num), fmt, strtod(value, 0));
					break;
				case 's':
					SNPRINTF( num, sizeof(num), fmt, value );
					break;
				}
				if( value ) free(value); value = 0;
				*s = 0;
				len = strlen(str)+strlen(num);
				DEBUG5("Fix_option_str: first '%s', num '%s', rest '%s'",
					str, num, eq );
				s = str;
				str = safestrdup3(str,num,eq,__FILE__,__LINE__);
				free(s);
				s = str+len;
				DEBUG5("Fix_option_str: result '%s', next '%s'", str, s );
			}
		}
	}
	if( remove_ws ){
		for( s = t = str; (*t = *s); ++s ){
			if( !isspace(cval(t)) ) ++t;
		}
		*t = 0;
	}
	if( trim ){
		while( isspace(cval(str)) ) memmove( str, str+1, strlen(str+1)+1 );
		for( s = str; (s = strpbrk( s, "\n" )); s = t ){
			for( t = s; t > str && isspace(cval(t-1)); --t );
			if( t != s ){
				memmove( t, s, strlen(s)+1 );
				s = t;
			}
			for( t = s+1; isspace(cval(t)); ++t );
			if( t != s+1 ){
				memmove( s+1, t, strlen(t)+1 );
				t = s+1;
			}
		}
	}
	if( one_line ){
		for( s = str; (s = strpbrk( s, "\t\n" )); ++s ){
			*s = ' ';
		}
	}
	DEBUG4("Fix_option_str: returning '%s'", str );
	return( str );
}

/*
 * char *Find_sub_value( int c, char *id, char *sep )
 *  if c == {, try the user -Z opts
 *  try user -T opts
 *  if id is single character try options
 *  try Model config
 *  default value is 0
 */

char *Find_sub_value( int c, char *id, int strval )
{
	char *s = 0;
	int i;
	DEBUG4("Find_sub_value: type '%c', id '%s'", c, id );
	if( s == 0 && !strcasecmp(id,"pid") ){
		static char pid[32];
		SNPRINTF(pid, sizeof(pid),"%d",getpid());
		s = pid;
	}
	if( s == 0 && setvals.count ){
		char *t, *u;
		int n;
		n = strlen(id);
		for( i = 0; i < setvals.count; ++i ){
			if( !strncasecmp( (t = setvals.list[i]), id, n) ){
				u = strpbrk(t,Value_sep);
				if( (u - t) == n ){
					s = u+1;
					DEBUG4("Find_sub_value: from setvals '%s', value '%s'",
						u,s);
					break;
				}
			}
		}
	}
	if( s == 0 && c == '{' ){
		s = Find_exists_value( &Zopts, id, Value_sep );
		DEBUG4("Find_sub_value: from Zopts '%s'", s );
	}
	if( s == 0 ){
		s = Find_exists_value( &Model, id, Value_sep );
		DEBUG4("Find_sub_value: from Model '%s'", s );
	}
	if( s == 0 ){
		if( strval ){
			s = "";
		} else {
			s = "0";
		}
	}
	DEBUG4("Find_sub_value: type '%c', id '%s'='%s'", c, id, s );
	return( s );
}

/*
 * int Builtin( char* prefix, char *pr_id, char *value, Wr_out routine)
 *  - check to see if the pr_id is for a builtin operation
 *  - call the built-in with the parameters
 *  - if found, the invoked return should return return 1 
 *        so no further processing is done
 *    if not found or the return value is 0, then further processing is
 *    done.
 */

int Builtin( char* prefix, char *id, char *value, Wr_out routine)
{
	int cmp=1;
	char *s, *prefix_id = 0;
	Builtin_func r;
	struct keyvalue *v;

	DEBUG4("Builtin: looking for '%s'", id );
	for( v = Builtin_values;
		(s = v->key) && (cmp = strcasecmp(s, id));
		++v );
	if( cmp && prefix && *prefix ){
		prefix_id = safestrdup2( prefix, id,__FILE__,__LINE__);
		DEBUG4("Builtin: looking for '%s'", prefix_id );
		for( v = Builtin_values;
			(s = v->key) && (cmp = strcasecmp(s, prefix_id));
			++v );
	}
	if( cmp == 0 ){
		DEBUG4("Builtin: found '%s'", s );
		r = (Builtin_func)(v->var);
		cmp = (r)( prefix, id, value, routine);
	} else {
		cmp = 0;
	}
	DEBUG4("Builtin: returning '%d'", cmp );
	if( prefix_id){ free(prefix_id); prefix_id = 0; }
	return( cmp );
}

int Font_download( char* prefix, char *id, char *value, Wr_out routine)
{
	char *dirname = 0, *filename, *s;
	char buffer[LARGEBUFFER];
	int n, fd, i;
	struct line_list fontnames;

	DEBUG2("Font_download: prefix '%s', id '%s', value '%s'",prefix, id, value );

	Init_line_list(&fontnames);
	s = safestrdup2(prefix,"fontdir",__FILE__,__LINE__);
	DEBUG4("Font_download: directory name '%s'", s );
	dirname = Find_str_value( &Model, s, Value_sep );
	if(s) free(s); s = 0;
	if( !dirname ){
		logerr("Font_download: no value for %sfontdir", prefix);
		return(1);
	}

	/* if there is a value, then use it */
	if( (filename = Fix_option_str(value,1,1,1)) ){
		DEBUG2("Font_download: fixed value '%s'", filename );
		Split(&fontnames,filename,Filesep,0,0,0,0,0);
		if( filename ) free(filename); filename = 0;
	} else {
		/* if there is a 'font' value, then use it */
		filename = Find_sub_value( '{', "font", 1 );
		Split(&fontnames,filename,Filesep,0,0,0,0,0);
	}
	if( fontnames.count == 0 ){
		logmsg("Font_download: no 'font' value");
		return(1);
	}
	filename = 0;
	for( i = 0; i < fontnames.count; ++i ){
		if( filename ) free(filename); filename = 0;
		if( (s = strrchr(fontnames.list[i],'/')) ){
			++s;
		} else {
			s = fontnames.list[i];
		}
		filename = safestrdup3(dirname,"/",s,__FILE__,__LINE__);
		/* get rid of // */
		DEBUG4("Font_download: filename '%s'", filename );
		if( (fd = open( filename, O_RDONLY )) >= 0 ){
			while( (n = read(fd, buffer,sizeof(buffer))) >0 ){
				Put_outbuf_len( buffer, n );
			}
			close(fd);
		} else {
			logerr("Font_download: cannot open '%s'", filename);
		}
	}
	if( filename ) free(filename); filename = 0;
	Free_line_list(&fontnames);
	return(1);
}

/*
 * int Pjl_job()
 *  - put out the JOB START string
 */

 char *Jobstart_str="@PJL JOB NAME = \"%s\"";
 char *Job_display=" DISPLAY = \"%s\" ";
 char Jobname[SMALLBUFFER];
 char *Jobend_str="@PJL EOJ NAME = \"%s\"";

void Pjl_job()
{
	char *s, buffer[SMALLBUFFER];
	int n = 0, len;

	if( (s = Find_value(&Model,"pjl_job",Value_sep )) ){
		Is_flag(s, &n);
	}
	if( n && ( Find_first_key( &Pjl_only, "job", 0, 0)
			|| !Find_first_key( &Pjl_except, "job", 0, 0)) ){
		n = 0;
	}
	DEBUG2("Pjl_job: Pjl %d, job '%s', flag %d", Pjl, s, n );
	if( Pjl == 0 || n == 0 ){
		return;
	}
	SNPRINTF( Jobname, sizeof(Jobname), "PID %d", getpid() );
	SNPRINTF( buffer, sizeof(buffer), Jobstart_str, Jobname );

	if( Pjl_console ){
		DEBUG2("Pjl_job: pjl_console '%d'", Pjl_console );
		s = Loweropts['n'-'a'];
		if( !s ) s = Upperopts['J'-'A'];
		if( !s ) s = "????";
		if( s ){
			len = strlen(buffer);
			SNPRINTF( buffer+len, sizeof(buffer)-len, Job_display, s );
		}
	}

	if( (s = Find_exists_value( &Zopts, "startpage", Value_sep))
		|| (s = Find_exists_value( &Topts, "startpage", Value_sep)) ){
		n = atoi( s );
		if( n > 0 ){
			len = strlen(buffer);
			SNPRINTF(buffer+len, sizeof(buffer)-len, " START = %d", n );
		}
	}
	if( (s = Find_exists_value( &Zopts, "endpage", Value_sep))
		|| (s = Find_exists_value( &Topts, "endpage", Value_sep)) ){
		n = atoi( s );
		if( n > 0 ){
			len = strlen(buffer); 
			SNPRINTF(buffer+len, sizeof(buffer)-len, " END = %d", n );
		}
	}
	DEBUG2("Pjl_job: final = '%s'", buffer );
	Put_pjl( buffer );
}

/*
 * int Pjl_eoj(char *prefix, char*id, char *value, Wr_out routine)
 *  - put out the JOB EOJ string
 */

void Pjl_eoj()
{
	char buffer[SMALLBUFFER];
	char *s;
	int n = 0;
	if( (s = Find_value(&Model,"pjl_job",Value_sep )) ){
		n = atoi(s);
	}
	DEBUG2("Pjl_eoj: Pjl %d, job '%s', flag %d", Pjl, s, n );
	if( Pjl == 0 || n == 0 ){
		return;
	}
	SNPRINTF( buffer, sizeof(buffer), Jobend_str, Jobname );
	Put_pjl( buffer );
}

/*
 * PJL RDYMSG option
 *  console    enables messages on console
 *  console@   disables or erases messages on console
 */

 char *PJL_RDYMSG_str  = "@PJL RDYMSG DISPLAY = \"%s\" ";

void Pjl_console_msg( int start )
{
	char *s, buffer[SMALLBUFFER], name[SMALLBUFFER];

	DEBUG2("Pjl_console: flag %d, start %d, ", Pjl_console, start );
	if( Pjl == 0 || Pjl_console == 0 ){
		return;
	}
	s = "";
	if( start ){
		s = Loweropts['n'-'a'];
		if( !s ) s = Upperopts['J'-'A'];
		if( !s ){
			s = name;
			SNPRINTF(name,sizeof(name), "PID %d", getpid());
		}
		if( strlen(s) > 8 ) s[8] = 0;
	}
	SNPRINTF( buffer, sizeof(buffer), PJL_RDYMSG_str, s );
	Put_pjl( buffer );
}

/*
 * PJL SET VAR=VALUE
 *  1. The variable must be in the Pjl_vars_set list
 *     This list has entries of the form:
 *        var=value var=value
 *        var@  -> var=OFF  
 *        var   -> var=ON  
 *  2. The variable must not be in the Pjl_vars_except list
 *     This list has entries of the form
 *        var  var var
 */

 char *PJL_SET_str = "@PJL SET %s = %s";

int Pjl_setvar(char *prefix, char*id, char *value, Wr_out routine)
{
	int found = 0, i;
	char buffer[SMALLBUFFER], *s;

	DEBUG3( "Pjl_setvar: prefix '%s', id '%s', value '%s'",
		prefix, id, value );
	if( !Find_first_key( &Pjl_vars_set, id, Value_sep, &i ) ){
		s = strpbrk( Pjl_vars_set.list[i], Value_sep );
		DEBUG3( "Pjl_setvar: found id '%s'='%s'", id, s );
		if( Find_first_key( &Pjl_vars_except, id, Value_sep, 0 ) ){
			DEBUG3( "Pjl_setvar: not in the except list" );
			if( value == 0 || *value == 0 ){
				value = s;
			}
			if( value == 0 || *value == 0 ){
				value = "ON";
			} else if( *value =='@' ){
				value = "OFF";
			} else if( *value == '=' || *value == '#' ){
				/* skip over = and # */
				++value;
			}
			DEBUG3( "Pjl_setvar: setting '%s' = '%s'", id, value );
			SNPRINTF(buffer,sizeof(buffer),PJL_SET_str,id,value);
			uppercase(buffer);
			routine(buffer);
			found = 1;
		}
	}
	return(found);
}

int Pcl_setvar(char *prefix, char*id, char *value, Wr_out routine )
{
	int found = 0, i;
	char *s;

	DEBUG3( "Pcl_setvar: prefix '%s', id '%s', value '%s'",
		prefix, id, value );
	if( !Find_first_key( &Pcl_vars_set, id, Value_sep, &i ) ){
		s = strpbrk( Pcl_vars_set.list[i], Value_sep );
		DEBUG3( "Pcl_setvar: found id '%s'='%s'", id, s );
		if( Find_first_key( &Pcl_vars_except, id, Value_sep, 0 ) ){
			DEBUG3( "Pcl_setvar: not in the except list" );
			value = s;
			if( value == 0 || *value == 0 ){
				value = 0;
			} else if( *value =='@' ){
				value = 0;
			} else if( *value == '=' || *value == '#' ){
				/* skip over = and # */
				++value;
			}
			DEBUG3( "Pcl_setvar: setting '%s' = '%s'", id, value );
			if( value ){
				routine(value);
			}
			found = 1;
		}
	}
	return(found);
}

/*
 * Do_sync()
 *  actual work is done here later
 *  sync=pjl  enables pjl
 *  sync=ps   enables postscript
 *  sync@     disables sync
 *  default is to use the prefix value
 */

 char *PJL_ECHO_str = "@PJL ECHO %s";
 char *CTRL_T = "\024";
 char *CTRL_D = "\004";

/*
 * void Do_sync( int sync_timeout )
 * - get a response from the printer
 */

void Do_sync( int sync_timeout )
{
	char buffer[SMALLBUFFER], name[SMALLBUFFER], *s, *sync_str;
	int len, flag, elapsed, timeout, sync, use, use_ps, use_pjl,
		use_appsocket, cx, output_fd = 1, input_fd = 1, attempt, n;
	time_t start_t, current_t, interval_t;

	time( &start_t );

	cx = 0;
	sync_str = 0;

	s = Sync;
	DEBUG2("Do_sync: sync is '%s'", Sync );

	/* we see if we can do sync */
	use = use_pjl = use_ps = use_appsocket = 0;
	if( !Is_flag( s, &use ) ){
		/* we check to see if the string is specifying */
		if( !strcasecmp(s,"pjl") ){
			use_pjl = Pjl;
		}
		if( !strcasecmp(s,"ps") ){
			use_ps = Ps;
		}
		use = 1;
	} else if( use ){
		if( Appsocket && Monitor_fd > 0 ){
			output_fd = Monitor_fd;
			input_fd = -1;
			use_appsocket = 1;
			sync_str = "appsocket";
		} else {
			use_ps = Ps;
			use_pjl = Pjl;
		}
	} else {
		return;
	}
	if( use_pjl
		&& ( Find_first_key( &Pjl_only, "echo", 0, 0)
			|| !Find_first_key( &Pjl_except, "echo", 0, 0)) ){
		use_pjl = 0;
	}
	if( use_pjl ) use_ps = 0;

	if( !use_ps && !use_pjl && !use_appsocket ){
		Errorcode = JABORT;
		fatal("Do_sync: sync '%s' and method not supported", s );
	}

	if( !sync_str && use_pjl ) sync_str = "pjl echo";
	if( !sync_str && use_ps ) sync_str = "ps";

	logmsg("Do_sync: getting sync using '%s'", sync_str );
	DEBUG2("Do_sync: input fd %d, output fd %d, monitor fd %d",
		input_fd, output_fd, Monitor_fd );

	attempt = 0;
	sync = 0;

 again:

	++attempt;
	DEBUG2("Do_sync: attempt %d", attempt );

	/* get start time */
	time( &interval_t );
	Init_outbuf();
	SNPRINTF( name, sizeof(name), "%d@%s", getpid(), Time_str(0,0) );
	for( s = name; ( s = strchr(s,':')); ++s) *s = '-';
	DEBUG2("Do_sync: Ps_status_code '%s', use_pjl %d, use_ps %d, use_appsocket %d",
			Ps_status_code, use_pjl, use_ps, use_appsocket );
	if( use_pjl ){
		Put_outbuf_str( PJL_UEL_str );
		Put_outbuf_str( PJL_str );
		SNPRINTF( buffer, sizeof(buffer),
			PJL_ECHO_str, name );
		Put_pjl( buffer );
		Put_outbuf_str( PJL_UEL_str );
	} else if( use_ps ){
		if( Ps_status_code == 0 ){
			fatal("Do_sync: sync '%s' and no ps_status_code value", sync_str );
		}
		if(Pjl){
			Put_outbuf_str( PJL_UEL_str );
			if( Pjl_enter ){
				Put_outbuf_str( PJL_str );
				Put_outbuf_str( "@PJL ENTER LANGUAGE = POSTSCRIPT\n" );
			}
		}
		/* watch out for ^D as part of sync */
		if( !No_PS_EOJ ) Put_outbuf_str( CTRL_D );
		if( (s = strstr(Ps_status_code,"NAME")) ){ cx = *s; *s = 0; }
		Put_outbuf_str( Ps_status_code );
		Put_outbuf_str( name );
		if( s ){ *s = cx; s += 4; Put_outbuf_str( s ); };
		/* watch out for ^D as part of sync */
		if( !No_PS_EOJ ) Put_outbuf_str( CTRL_D );
		if(Pjl){
			Put_outbuf_str( PJL_UEL_str );
		}
	} else if( use_appsocket ){
		Put_outbuf_str( "\r\n" );
	} else {
		Errorcode = JABORT;
		fatal("Do_sync: no way to synchronize printer, need PJL, PS, or appsocket" );
	}

	DEBUG2("Do_sync: using sync '%s'", Outbuf );

	while( !sync ){
		flag = 0;
		/* write the string */
		/* check to see if we need to read */
		time( &current_t );
		elapsed = current_t - start_t;

		/* check for master timeout */
		if( sync_timeout > 0 ){
			if( elapsed >= sync_timeout ){
				break;
			}
			timeout = sync_timeout - elapsed;
		} else {
			timeout = 0;
		}
		elapsed = current_t - interval_t;
		DEBUG3("Do_sync: timeout %d, Sync_interval %d, elpased %d",
			timeout, Sync_interval, elapsed );
		if( Sync_interval > 0 ){
			timeout = Sync_interval - elapsed;
			if( timeout <= 0 ){
				goto again;
			}
		}
		DEBUG3("Do_sync: waiting for sync, timeout %d", timeout );
		len = Read_write_timeout( input_fd, &flag,
			output_fd, Outbuf, Outlen, timeout, Monitor_fd );
		DEBUG3("Do_sync: read/write len '%d', flag %d", len, flag );
		if( len < 0 ){
			break;
		} else if( len >= 0 ){
			memmove(Outbuf, Outbuf+(Outlen - len), len+1 );
			Outlen = len;
		}
		if( (flag & 1) ){
			n = Read_status_line( input_fd );
			DEBUG3("Do_sync: Read_status_line_returned '%d'", n );
			if( n < 0 ){
				logerr("Do_sync: read from printer port for sync failed");
				break;
			} else if( (n == 0 && !Ignore_eof ) ){
				logmsg("Do_sync: EOF while reading from printer port");
				break;
			} else if( n > 0 ) {
				/* get the status */
				while( Get_inbuf_str() );
			}
		}
		if( flag & 2 ){
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				/* get the status */
				while( Get_monitorbuf_str() );
			}
		}
		if( use_appsocket 
			&& (s = Find_str_value( &Devstatus, "status",Value_sep))
			&& (strstr(s,"busy") || strstr(s,"idle")) ){
			sync = 1;
		} else if( (s = Find_str_value( &Devstatus, "echo", Value_sep))
			&& strstr( s, name ) ){
			sync = 1;
		}
	}
	DEBUG2("Do_sync: sync %d", sync );
	if( sync == 0 ){
		Errorcode = JFAIL;
		fatal("Do_sync: no response from printer" );
	}
	logmsg("Do_sync: sync done");
}

/*
 * void Do_waitend( int sync_timeout )
 * - get the job status from the printer
 *   and wait for the end of job
 */

 char *PJL_USTATUS_JOB_str = "@PJL USTATUS JOB = ON";

void Do_waitend( int waitend_timeout )
{
	char *sync_str, *s, *t, buffer[SMALLBUFFER];
	int len, flag, elapsed, timeout, waitend,
		use, use_pjl, use_ps, use_appsocket, use_job, c, output_fd = 1,
		input_fd = 1, n, echo_received = 0, interval;
	time_t start_t, current_t, interval_t;

	time( &start_t );
	interval = Waitend_interval;

	s = (Waitend?Waitend:Sync);
	DEBUG1("Do_waitend: waitend '%s', end_ctrl_t '%s'", s, End_ctrl_t );
	if( Is_flag( s, &use ) && use == 0 ){
		return;
	}
	if( Is_flag( End_ctrl_t, &use ) ){
		End_ctrl_t = 0;
	}
	DEBUG1("Do_waitend: fixed end_ctrl_t '%s'", End_ctrl_t );

	/* we see if we can do sync */
	use = use_pjl = use_ps = use_job = use_appsocket = 0;
	sync_str = 0;
	if( Appsocket && Device ){
		DEBUG1("Do_waitend: Appsocket device close");
		shutdown( 1, 1 );
		if( Monitor_fd > 0 ){
			output_fd = Monitor_fd;
			use_appsocket = 1;
		}
		sync_str = "appsocket";
	}
	if( !sync_str ){
		if( !Is_flag( s, &use ) ){
			/* we check to see if the string is specifying */
			if( !strcasecmp(s,"pjl") ){
				use_pjl = Pjl;
			}
			if( !strcasecmp(s,"ps") ){
				use_ps = Ps;
			}
			use = 1;
		} else if( use ){
			use_job = use_pjl = Pjl;
			use_ps = Ps;
		} else {
			return;
		}

		if( use_pjl
			&& ( Find_first_key( &Pjl_only, "job", 0, 0)
				|| !Find_first_key( &Pjl_except, "job", 0, 0)
				) ){
			use_job = 0;
		}
		if( use_pjl && use_job == 0
			&& ( Find_first_key( &Pjl_only, "echo", 0, 0)
				|| !Find_first_key( &Pjl_except, "echo", 0, 0)
				) ){
			use_pjl = 0;
		}
		if( !use_ps && !use_pjl ){
			Errorcode = JABORT;
			fatal("Do_waitend: waitend '%s' and method not supported", s );
		}
		if( use_pjl ) use_ps = 0;
	}
	if( !sync_str && use_job ) sync_str = "pjl job/eoj";
	if( !sync_str && use_pjl ) sync_str = "pjl echo";
	if( !sync_str && use_ps ) sync_str = "ps";

	logmsg("Do_waitend: getting end using '%s'", sync_str );

	waitend = 0;

	/* find if we need to periodically send the waitend value */
	DEBUG3("Do_waitend: use_pjl %d, use_job %d, use_appsocket %d, use_ps %d, timeout %d, interval %d",
		use_pjl, use_job, use_appsocket, use_ps, waitend_timeout, interval );

	/* Free_line_list( &Devstatus ); */

	DEBUG2("Do_waitend: input fd %d, output fd %d, monitor fd %d",
		input_fd, output_fd, Monitor_fd );

 again:

	Init_outbuf();
	SNPRINTF( Jobname, sizeof(Jobname), "%s PID %d",
		Time_str(1,0), getpid() );
	for( s = Jobname; ( s = strchr(s,':')); ++s) *s = '-';
	if( use_pjl ){
		Put_outbuf_str( PJL_UEL_str );
		Put_outbuf_str( PJL_str );
		if( use_job ){
			SNPRINTF( buffer, sizeof(buffer), Jobstart_str, Jobname );
			Put_pjl(buffer);
			Put_pjl( PJL_USTATUS_JOB_str );
			Pjl_eoj();
		} else {
			SNPRINTF( buffer, sizeof(buffer), PJL_ECHO_str, Jobname );
			Put_pjl(buffer);
		}
		Put_outbuf_str( PJL_UEL_str );
	} else if( use_ps ){
		if( Ps_status_code == 0 ){
			fatal("Do_waitend: no ps_status_code value" );
		}
		if(Pjl){
			Put_outbuf_str( PJL_UEL_str );
			if( Pjl_enter ){
				Put_outbuf_str( PJL_str );
				Put_outbuf_str( "@PJL ENTER LANGUAGE = POSTSCRIPT\n" );
			}
		}
		Put_outbuf_str( CTRL_D );
		if( End_ctrl_t ){
			Put_outbuf_str( CTRL_T );
		}
		if( !echo_received ){
			c = 0;
			if( (s = strstr(Ps_status_code,"NAME")) ){ c = *s; *s = 0; }
			Put_outbuf_str( Ps_status_code );
			Put_outbuf_str( Jobname );
			if( s ){ *s = c; s += 4; Put_outbuf_str( s ); };
		}
		Put_outbuf_str( CTRL_D );
		if(Pjl){
			Put_outbuf_str( PJL_UEL_str );
		}
	} else if( use_appsocket && output_fd > 0 ){
		Put_outbuf_str( "\r\n" );
	}

	time( &interval_t );

	DEBUG3("Do_waitend: sending '%s'", Outbuf );
	while( !waitend ){
		flag = 0;
		/* write the string */
		/* check to see if we need to read */
		time( &current_t );
		elapsed = current_t - start_t;

		/* check for master timeout */
		if( waitend_timeout > 0 ){
			DEBUG3("Do_waitend: waitend timeout %d, elapsed %d",
				waitend_timeout, elapsed );
			if( elapsed >= waitend_timeout ){
				break;
			}
			timeout = waitend_timeout - elapsed;
		} else {
			timeout = 0;
		}
		if( interval > 0 ){
			elapsed = current_t - interval_t;
			timeout = interval - elapsed;
			if( timeout <= 0 ) goto again;
		}
		len = Read_write_timeout( input_fd, &flag,
			output_fd, Outbuf, Outlen, timeout, Monitor_fd);
		DEBUG3("Do_waitend: write len %d, flag %d", len, flag );
		if( len < 0 ){
			break;
		} else if( len >= 0 ){
			memmove(Outbuf, Outbuf+(Outlen - len), len);
			Outlen = len;
		}
		if( flag & 1 ){
			n = Read_status_line( input_fd );
			DEBUG3("Do_waitend: Read_status_line returned %d", n );
			if( n < 0 ){
				logerr("Do_waitend: read from printer failed");
				break;
			} else if( n == 0 ){
				DEBUG3("Do_waitend: EOF on input_fd %d, Ignore_eof %d, Appsocket %d, Monitor_fd %d",
					input_fd, Ignore_eof, Appsocket, Monitor_fd );
				if( !Ignore_eof ){
					input_fd = -1;
					if( Appsocket ){
						if( Monitor_fd <= 0 ){
							waitend = 1;
							logmsg("Do_waitend: EOF from printer" );
							break;
						}
					} else {
						logerr("Do_waitend: EOF from printer failed");
						break;
					}
				}
			} else {
				/* get the status */
				while( Get_inbuf_str() );
			}
		}
		if( flag & 2 ){
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				/* get the status */
				while( Get_monitorbuf_str() );
			}
		}
		if( flag ){
			if(DEBUGL3)Dump_line_list("Do_waitend - Devstatus", &Devstatus );
			if( use_pjl ){
				s = Find_str_value( &Devstatus, "job", Value_sep);
				t = Find_str_value( &Devstatus, "name", Value_sep);
				DEBUG4("Do_waitend: job '%s', name '%s', Jobname '%s'", s, t, Jobname );
				if( s && strstr(s,"END") && t && strstr(t,Jobname) ){
					waitend = 1;
				}
				s = Find_str_value( &Devstatus, "echo", Value_sep);
				DEBUG4("Do_waitend: echo '%s'", s );
				if( s && strstr(s,Jobname) ){
					waitend = 1;
				}
			} else if( use_ps ){
				int done_found = 0;
				if( !echo_received ){
					s = Find_str_value( &Devstatus, "echo", Value_sep);
					if( s ) echo_received = (strstr( s, Jobname ) != 0 );
					DEBUG4("Do_waitend: echo '%s', Jobname '%s', echo_received %d",
						s, Jobname, echo_received );
				}
				t = Find_str_value( &Devstatus, "status", Value_sep);
				DEBUG4( "Do_waitend: status '%s', End_ctrl_t '%s'", t, End_ctrl_t);
				if( t && End_ctrl_t ){
					t = strstr( End_ctrl_t, t );
					if( t || End_ctrl_t[0] == '!' ){
						done_found = 1;
					}
				}
				DEBUG4(
	"Do_waitend: echo_received '%d', status '%s', End_ctrl_t '%s', done_found %d",
						echo_received, t, End_ctrl_t, done_found );
				/* this is suitable under all circumstances */
				if( echo_received ){
					/* if we have the done status or no status or Appsocket
					 * we are done */
					if( done_found || End_ctrl_t == 0 || Appsocket ){
						waitend = 1;
					}
					/* we wait only a short amount of time */
					if( End_ctrl_t && Waitend_ctrl_t_interval &&
						interval > Waitend_ctrl_t_interval ){
						interval = Waitend_ctrl_t_interval;
					}
					DEBUG4("Do_waitend: waitend %d", waitend );
				}
			}
		}
	}
	if( waitend == 0 ){
		Errorcode = JFAIL;
		fatal("Do_waitend: no response from printer" );
	}
	logmsg("Do_waitend: end of job detected", sync_str );
	DEBUG3("Do_waitend: end of print job" );
}

/*
 * int Do_pagecount(void)
 * - get the pagecount from the printer
 *  We can use the PJL or PostScript method
 *  If we use PJL, then we send pagecount command, and read status
 *   until we get the value.
 *  If we use PostScript,  we need to set up the printer for PostScript,
 *   send the PostScript code,  and then get the response.
 *
 * INFO PAGECOUNT Option
 * pagecount=pjl   - uses PJL (default)
 * pagecount=ps    - uses PS
 * pagecount@
 *
 */

int Do_pagecount( int pagecount_timeout )
{
	int pagecount = 0, new_pagecount = -1,
		use, use_ps, use_pjl;
	char *s;

	s = Pagecount;
	DEBUG4("Do_pagecount: getting pagecount using '%s'", s );

	/* we see if we can do sync */
	use = use_pjl = use_ps = 0;
	if( !Is_flag( s, &use ) ){
		/* we check to see if the string is specifying */
		if( !strcasecmp(s,"pjl") ){
			use_pjl = 1;
		}
		if( !strcasecmp(s,"ps") ){
			use_ps = 1;
		}
		use = 1;
	} else if( use ){
		use_pjl = Pjl;
		use_ps = Ps;
	} else {
		return 0;
	}
	Strip_leading_spaces( &Ps_pagecount_code );
	if(use_ps && !Ps_pagecount_code){
		use_ps = 0;
	}
	if(use_pjl
		&& (Find_first_key( &Pjl_only, "info", 0, 0)
			||  !Find_first_key( &Pjl_except, "info", 0, 0)) ){
		use_pjl = 0;
	}
	if(use_ps && !Ps_pagecount_code){
		use_ps = 0;
	}

	if( !use_ps && !use_pjl ){
		Errorcode = JABORT;
		fatal("Do_pagecount: pagecount '%s' and method not supported", s );
	}
	if( use_pjl ) use_ps = 0;
	s = use_pjl?"pjl info pagecount":"ps script";

	logmsg("Do_pagecount: getting pagecount using '%s'", s );

	pagecount = Current_pagecount( pagecount_timeout, use_pjl, use_ps );
	if( pagecount >= 0 && Pagecount_poll > 0 ){
		logmsg("Do_pagecount: pagecount %d, polling every %d seconds",
			pagecount, Pagecount_poll );
		do{
			if( Read_until_eof( Pagecount_poll ) ){
				break;
			}
			DEBUG1("Do_pagecount: polling, getting count again");
			pagecount = new_pagecount;
			new_pagecount = Current_pagecount(pagecount_timeout,
				use_pjl, use_ps);
		} while( new_pagecount == 0 || new_pagecount != pagecount );
	}
	logmsg("Do_pagecount: final pagecount %d", pagecount);
	return( pagecount );
}

/*
 * Read_until_eof( max_timeout )
 * read from fd 1 until you get EOF or a timeout
 */

int Read_until_eof( int max_timeout )
{
	int elapsed, timeout, len, flag, n, eof = 0;
	time_t start_t, current_t;

	time( &start_t );
	while(1){
		timeout = 0;
		if( max_timeout > 0 ){
			time( &current_t );
			elapsed = current_t - start_t;
			timeout = max_timeout - elapsed;
			if( timeout <= 0 ){
				break;
			}
		}
		DEBUG1("Read_until_eof: waiting %d", timeout );
		len = Read_write_timeout( 1, &flag,
			0, 0, 0, timeout, Monitor_fd );
		DEBUG1("Read_until_eof: Read_write_timeout returned %d", len );
		if( len < 0 ){
			break;
		}
		if( flag & 1 ){
			/* get the status */
			n = Read_status_line( 1 );
			DEBUG1("Read_until_eof: Read_status_line_returned '%d'", n );
			if( n < 0 || (n == 0 && !Ignore_eof ) ){
				DEBUG1("Read_until_eof: eof from printer");
				eof = 1;
				break;
			} else if( n > 0 ){
				while( Get_inbuf_str() );
			}
		}
		if( flag & 2 ){
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				while( Get_monitorbuf_str() );
			}
		}
	}
	return( eof );
}

 char *PJL_INFO_PAGECOUNT_str = "@PJL INFO PAGECOUNT \n";

int Current_pagecount( int pagecount_timeout, int use_pjl, int use_ps )
{
	int i, len, flag, elapsed, timeout, page, pagecount = 0, found;
	time_t start_t, current_t, interval_t;
	int shutdown_done = 0;
	char *s;

	/* get start time */
	time( & start_t );

 again:

	/* interval time */
	time( & interval_t );

	/*if(DEBUGL4)Dump_line_list("Current_pagecount - Devstatus", &Devstatus);*/
	DEBUG1("Current_pagecount: starting, use_pjl %d, use_ps %d, timeout %d",
		use_pjl, use_ps, pagecount_timeout );

	if( Appsocket && Device ){
		DEBUG1("Current_pagecount: Appsocket device close");
		shutdown(1,1);
		Read_until_eof( pagecount_timeout );
		Open_device( Device );
		DEBUG1("Current_pagecount: Appsocket device open done");
		shutdown_done = 0;
	}

	/* remove the old status */
	Set_str_value(&Devstatus,"pagecount",0);

	Init_outbuf();
	if( use_pjl ){
		Put_outbuf_str( PJL_UEL_str );
		Put_outbuf_str( PJL_str );
		Put_pjl( PJL_INFO_PAGECOUNT_str );
		Put_outbuf_str( PJL_UEL_str );
	} else if( use_ps ){
		if( !Ps_pagecount_code ){
			Errorcode = JABORT;
			fatal("Current_pagecount: no ps_pagecount_code config info");
		}
		if( Pjl ){
			Put_outbuf_str( PJL_UEL_str );
			if( Pjl_enter ){
				Put_outbuf_str( PJL_str );
				Put_outbuf_str( "@PJL ENTER LANGUAGE = POSTSCRIPT\n" );
			}
		}
		/* Watch out for ^D again */
		if( !Appsocket && !No_PS_EOJ ) Put_outbuf_str( CTRL_D );
		Put_outbuf_str( Ps_pagecount_code );
		Put_outbuf_str( "\n" );
		if( !Appsocket ) Put_outbuf_str( CTRL_D );
		if( Pjl ) Put_outbuf_str( PJL_UEL_str );
	}

	DEBUG2("Current_pagecount: using '%s'", Outbuf );

	page = 0;
	found = 0;
	while(!page){
		flag = 0;
		/* write the string */
		/* check to see if we need to read */
		time( &current_t );
		elapsed = current_t - start_t;
		if( pagecount_timeout > 0 ){
			if( elapsed >= pagecount_timeout ){
				break;
			}
			timeout = pagecount_timeout - elapsed;
		} else {
			timeout = 0;
		}
		if( Pagecount_interval > 0 ){
			elapsed = current_t - interval_t;
			if( elapsed > Pagecount_interval ){
				goto again;
			}
			if( timeout == 0 || timeout > Pagecount_interval ){
				timeout = Pagecount_interval;
			}
		}
		len = Read_write_timeout( 1, &flag,
			1, Outbuf, Outlen, timeout, Monitor_fd );
		DEBUG1("Current_pagecount: write len %d", len );
		if( flag & 1 ){
			i = Read_status_line( 1 );
			DEBUG1("Current_pagecount: Read_status_line_returned '%d'", i );
			if( i < 0 || (i == 0 && !Ignore_eof) ){
				logerr("Current_pagecount: read from printer failed");
				break;
			} else if( i > 0 ) {
				while( (s = Get_inbuf_str() ) );
			}
		}

		if( flag & 2 ){
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				while( (s = Get_monitorbuf_str() ) );
			}
		}
		if( len < 0 ){
			break;
		} else if( len >= 0 ){
			memmove(Outbuf, Outbuf+(Outlen - len), len);
			Outlen = len;
			if( len == 0 && Monitor_fd > 0 && shutdown_done == 0 ){
				DEBUG1("Current_pagecount: Monitor_fd %d, shutdown fd 1", Monitor_fd );
				shutdown_done = 1;
				shutdown( 1, 1 );
			}
		}
		if(DEBUGL4)Dump_line_list("Current_pagecount - Devstatus",&Devstatus);
		if( !Find_first_key( &Devstatus, "pagecount", Value_sep, &i )){
			s = Find_str_value( &Devstatus, "pagecount", Value_sep );
			if( s && isdigit(cval(s)) ){
				pagecount = atoi( s );
				page = 1;
			}
		}
	}
	DEBUG1("Current_pagecount: page %d, pagecount %d", page, pagecount );
	if( page == 0 ){
		Errorcode = JFAIL;
		fatal("Current_pagecount: no response from printer" );
	}
	return( pagecount );
}


/*
 * Send_job
 * 1. we read an input buffer
 * 2. we then check to see what type it is
 * 3. we then set up the various configuration
 * 4. we then send the job to the remote end
 * Note: there is a problem when you send language switch commands
 *  to the HP 4M+, and other printers.
 * two functions.
 *
 * The following comments from hp.com try to explain the situation:
 *
 *"What is happening is the printer buffer fills with your switch
 * command and the first parts of your job. The printer does not
 * "prescan" the input looking for the <Esc>%-1234... but instead gets
 * around to it after some time has expired. During the switch, the
 * data that you have already trasmitted is discarded and is picked
 * up whereever your driver happens to be. For PostScript that's a
 * messy situation.
 * 
 * What you need to do is isolate the switch command. You can do this
 * with nulls if you have no control over the timing. 8K of them will
 * fill the largest allocated I/O buffer and then hold off the UNIX
 * system. The switch will be made and the initial remaining nulls
 * will be discarded. If you can control the timing of the data, you'll
 * have to experiment with the correct time.
 * 
 */

 char *PJL_ENTER_str = "@PJL ENTER LANGUAGE = %s\n";
 char *PCL_EXIT_str = "\033E";

void Send_job()
{
	plp_status_t status;
	int len = 0, i, c, pid = 0, n, cnt, tempfd, error_fd[2];
	char *s, *pgm = 0, *t, *u;
	int done = 0;
	char *save_outbuf;
	int save_outlen, save_outmax;
	struct line_list l, match, files;
	char *mode;
	char *language;
	struct stat statb;
	int progress_last, is_file, progress_pc, progress_k;
	double total_size, progress_total;
	char buffer[SMALLBUFFER];

	pgm = 0;
	Init_line_list( &l );
	Init_line_list( &match );
	Init_line_list( &files );
	Init_outbuf();
	
	cnt = SMALLBUFFER;
	if( cnt > Outmax ) cnt = Outmax; 
	logmsg( "Send_job: starting transfer" );
	DEBUG2( "Send_job: want %d", cnt );
	while( Outlen < cnt 
		&& (len = read( 0, Outbuf+Outlen, Outmax - Outlen )) > 0 ){
		Outlen += len;
	}
	Outbuf[Outlen] = 0;
	DEBUG2( "Send_job: read %d from stdin", Outlen );
	if( len < 0 ){
		Errorcode = JFAIL;
		logerr_die( "Send_job: read error on stdin" );
	}
	if( Outlen == 0 ){
		logmsg( "Send_job: zero length job file" );
		return;
	}

	/* defaults */
	language = Find_str_value(&Model,"default_language",Value_sep);
	if( ! language ) language = UNKNOWN;

	/* decide on the job type */
	if( Loweropts['c'-'a'] || Autodetect ){
		language = RAW;
	} else if( (s = Find_str_value(&Zopts,"language",Value_sep)) ){
		language = s;
	} else if( Force_conversion ){
		if((pgm = Find_str_value( &Model, "file_util_path", Value_sep )) == 0 ){
			Errorcode = JABORT;
			fatal( "Send_job: missing file_util_path value");
		}
		DEBUG2( "Send_job: file_util_path '%s'", pgm );
		Make_stdin_file();
		Init_outbuf();
		if( lseek(0,0,SEEK_SET) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: lseek failed");
		}
		Use_file_util(pgm, &match);
		if( match.count ){
			language = match.list[0];
		}
		if( lseek(0,0,SEEK_SET) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: lseek failed");
		}
		pgm = 0;
	} else if( !strncmp( PJL_UEL_str, Outbuf, strlen(PJL_UEL_str)) ){
		language = PJL;
	} else if( Outbuf[0] == '\033' ){
		language = PCL;
		/* remove \033E at start */
		if( Outbuf[1] == 'E' ){
			memmove(Outbuf,Outbuf+2,Outlen-2);
			Outlen -= 2;
		}
	} else if( Outbuf[0] == '\004' ){
		/* remove the ^D at input */
		language = PS;
		memmove(Outbuf,Outbuf+1,Outlen-1);
		--Outlen;
	} else if( !memcmp(Outbuf, "%!", 2) ){
		language = PS;
	}

	logmsg( "Send_job: initial job type '%s'", language );

	if( language && language != RAW
		&& (s = Find_str_value(&Model, "file_output_match",Value_sep)) ){
		/* split the matches up */
		Free_line_list(&l);
		Split(&match,s,Line_ends,0,0,0,0,0);
		if( match.count > 0 && (match.list[0])[0] == '<' ){
			Split(&files,match.list[0],Value_sep,0,0,0,0, 0 );
			for( i = 0; i < files.count; ++i ){
				s = files.list[i];
				if( cval(s) == '<' ) ++s;
				if( !cval(s) ) continue;
				DEBUG2("Send_job: reading file '%s'", s );
				t = Get_file_image( s );
				if( t == 0 || *t == 0 ){
					Errorcode = JABORT;
					logerr_die("Send_job: could not open '%s'", s);
				}
				DEBUG2("Send_job: file '%s'", t );
				Split(&l,t,Line_ends,0,0,0,0, 0 );
				if( t ) free(t); t = 0;
			}
		}
		for( i = 0; i < match.count; ++i ){
			DEBUG2("Send_job: [%d] '%s'", i, match.list[i] );
			Split_count(2,&l,match.list[i],Whitespace, 0, 0, 0, 1, 1 );
			if(DEBUGL2)Dump_line_list("Send_job: checking against", &l );
			if( l.count < 2 ){
				Free_line_list(&l);
				continue;
			}
			URL_decode( l.list[0] );
			if( !Globmatch( l.list[0], language ) ){
				Check_max(&l,2);
				l.list[l.count] = 0;
				l.list[l.count+1] = 0;
				language = l.list[1]; l.list[1] = 0;
				pgm = l.list[2]; l.list[2] = 0;
				break;
			}
			Free_line_list(&l);
		}
		Free_line_list(&l);
	}

	mode = Set_mode_lang(language);

	/* we can treat text as PCL */
	if( pgm == 0 && mode == TEXT && Pcl ){
		mode = PCL;
		language = PCL;
	}


	if( mode == UNKNOWN  ){
		Errorcode = JABORT;
		fatal("Send_job: cannot determine job type");
	}
	if( mode == PJL && !Pjl ){
		Errorcode = JABORT;
		fatal("Send_job: job is PJL and PJL not supported");
	}
	if( mode == PCL && !Pcl ){
		Errorcode = JABORT;
		fatal("Send_job: job is PCL and PCL not supported");
	}
	if( mode == PS && !Ps ){
		Errorcode = JABORT;
		fatal("Send_job: job is PostScript and PostScript not supported");
	}
	if( mode == TEXT && !Text ){
		Errorcode = JABORT;
		fatal("Send_job: job is Text and Text not supported");
	}

	if( pgm ){
		s = pgm;
		pgm = Fix_option_str( pgm, 0, 1, 1 );
		if(s) free(s); s = 0;
		s = pgm;
		while( isspace(cval(pgm)) ) ++pgm;
		if( cval(pgm) == '|' ) ++pgm;
		while( isspace(cval(pgm)) ) ++pgm;
		if( s != pgm ){
			memmove(s, pgm, strlen(pgm)+1 );
			pgm = s;
		}
	}

	if( pgm == 0 || *pgm == 0 ){
		logmsg( "Send_job: job type '%s'", language  );
	} else {
		logmsg( "Send_job: job type '%s', converter '%s'",
			language, pgm );
	}

	if( pgm && *pgm ){
		if( (t = strstr( pgm, "ZOPTS")) ){
			s = Find_str_value(&Topts,"Z",Value_sep);
			*t = 0; 
			t += strlen("ZOPTS");
			u = pgm;
			pgm = safestrdup4( pgm, s?"-Z":"", s, t, __FILE__, __LINE__ );
			if(u) free(u); u = 0;
		}
		if( (t = strstr( pgm, "TOPTS")) ){
			s = Find_str_value(&Topts,"T",Value_sep);
			*t = 0; 
			t += strlen("TOPTS");
			u = pgm;
			pgm = safestrdup4( pgm, s?"-T":"", s, t, __FILE__, __LINE__ );
			if(u) free(u); u = 0;
		}
		if( (t = strstr( pgm, "ARGV" )) ){
			s = Join_vector( Argv+1, Argc, " ");
			*t = 0; 
			t += strlen("ARGV");
			u = pgm;
			pgm = safestrdup3(pgm, s?s:"", t, __FILE__, __LINE__ );
			if(s) free(s); s = 0;
			if(u) free(u); u = 0;
		}
		if( pgm && strpbrk( pgm, "|><" ) ){
			Split(&l, "/bin/sh -c", Whitespace, 0, 0, 0, 1, 0 );
			Check_max(&l,2);
			l.list[l.count++] = pgm;
			l.list[l.count] = 0;
		} else {
			Split_cmd_line( &l, pgm, 0 );
			Check_max(&l,2);
			l.list[l.count] = 0;
		}
		if(DEBUGL2)Dump_line_list("Send_job: process args", &l );
		Make_stdin_file();
		Init_outbuf();
		if( lseek(0,0,SEEK_SET) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: lseek failed");
		}
		if( pipe(error_fd) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: pipe failed");
		}
		tempfd = Make_tempfile(0);

		/* fork the process */
		if( (pid = fork()) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: fork failed");
		} else if( pid == 0 ){
			if( dup2(tempfd,1) == -1 || dup2(error_fd[1],2) == -1 ){
				Errorcode = JABORT;
				logerr_die("Send_job: dup2 failed");
			}
			close_on_exec(3);
			execve( l.list[0], l.list, Envp );
			/* now we exec the filter process */
			Errorcode = JABORT;
			logerr_die( "Send_job: execv failed" );
		}

		close(error_fd[1]);
		logmsg("Send_job: started converter '%s'", pgm);
		len = 0;
		while( len < sizeof(buffer)-1
			&& (n = read(error_fd[0],buffer+len,sizeof(buffer)-1-len)) > 0 ){
			buffer[n+len] = 0;
			while( (s = strchr(buffer,'\n')) ){
				*s++ = 0;
				logmsg("Send_job: '%s' error msg '%s'", l.list[0], buffer );
				memmove(buffer,s,strlen(s)+1);
			}
			len = strlen(buffer);
		}
		if( len ){
			logmsg("Send_job: '%s' error at end '%s'", l.list[0], buffer );
		}
		close(error_fd[0]);
		while( (n = waitpid(pid,&status,0)) != pid );
		DEBUG1("Send_job: converter pid %d, exit '%s'", pid,
			Decode_status( &status ) );
		if( WIFEXITED(status) && (n = WEXITSTATUS(status)) ){
			Errorcode = JABORT;
			fatal("Send_job: converter process '%s' exited with status %d",l.list[0], n);
		} else if( WIFSIGNALED(status) ){
			Errorcode = JABORT;
			n = WTERMSIG(status);
			fatal("Send_job: converter process died with signal %d, '%s'",
				n, Sigstr(n));
		}
		Free_line_list(&l);
		if( fstat(tempfd,&statb) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: dup2 failed");
		}
		logmsg("Send_job: converter done, output %ld bytes",
			(int)(statb.st_size) );
		if( dup2(tempfd,0) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: dup2 failed");
		}
		if( tempfd != 0 ) close(tempfd);
		if( lseek(0,0,SEEK_SET) == -1 ){
			Errorcode = JABORT;
			logerr_die("Send_job: lseek failed");
		}
		/* if you did filtering,  then reopen for the job */
		if( Appsocket && Device ){
			DEBUG1("Send_job: Appsocket device close");
			shutdown(1,1);
			Read_until_eof(Job_timeout);
			Open_device( Device );
			DEBUG1("Send_job: Appsocket device open done");
		}
		len = SMALLBUFFER;
		if( len < Outmax ) len = Outmax;
		if( (len = read( 0, Outbuf+Outlen, len )) > 0 ){
			Outlen += len;
			Outbuf[Outlen] = 0;
			DEBUG2( "Send_job: read %d from stdin", Outlen );
		}
		if( len < 0 ){
			Errorcode = JABORT;
			logerr_die( "Send_job: read error on stdin" );
		}
		if( len == 0 ){
			Errorcode = JABORT;
			logerr_die( "Send_job: zero length conversion output" );
		}
	}

	/* strip off the ^D at start of PostScript jobs */
	if( Outlen && mode == PS && Outbuf[0] == '\004' ){
		memmove(Outbuf,Outbuf+1,Outlen-1);
		Outlen -= 1;
	}
	if( Outlen > 1 && mode == PCL && Outbuf[0] == '\033' && Outbuf[1] == 'E' ){
		memmove(Outbuf,Outbuf+2,Outlen-2);
		Outlen -= 2;
	}
	Outbuf[Outlen] = 0;
	DEBUG2( "Send_job: Outbuf after type and stripping '%s'", Outbuf );

	save_outbuf = Outbuf; Outbuf = 0;
	save_outlen = Outlen; Outlen = 0;
	save_outmax = Outmax; Outmax = LARGEBUFFER;
	Init_outbuf();

	Init_job( mode );

	/* You can now work with the big buffer again */
	if( Outbuf ){ free(Outbuf); };
	Outbuf = save_outbuf;
	Outlen = save_outlen;
	Outmax = save_outmax;

	DEBUG1("Send_job: doing file transfer");
	if( fstat( 0, &statb ) < 0 ){
		Errorcode = JABORT;
		logerr_die( "Send_job: cannot fstat fd 0" );
	}
	is_file = ((statb.st_mode & S_IFMT) == S_IFREG);
	total_size = statb.st_size;

	progress_total = 0;
	progress_last =  progress_pc = progress_k = 0;
	if( (s = Find_str_value(&Model,"progress_pc",Value_sep)) ){
		progress_pc = atoi(s);
	}
	if( (s = Find_str_value(&Model,"progress_k",Value_sep)) ){
		progress_k = atoi(s);
	}
	if( !is_file || progress_pc < 0 || progress_pc > 100 ){
		progress_pc = 0;
	}
	if( progress_k <= 0 ){
		progress_k = 0;
	}
	if( progress_pc == 0 && progress_k == 0 ){
		progress_k = 100;
	}
	DEBUG2( "Send_job: input stat 0%06o, is_file %d, size %0.0f, progress_pc %d, progress_k %d",
		(int)(statb.st_mode & S_IFMT),
		is_file, total_size, progress_pc, progress_k );

	if( mode == PS && Remove_ctrl ){
		for( i = 0; i < strlen(Remove_ctrl); ++i ){
			Remove_ctrl[i] &= 0x1F;
		}
	}

	logmsg( "Send_job: transferring %0.0f bytes", total_size );
	do{
		len = 0;
		progress_total += Outlen;
		if( mode == PS && Tbcp ){
			DEBUG4("Send_job: tbcp");
			for( cnt = 0; cnt < Outlen; ){
				len = 0; c = 0;
				for( i = cnt; c == 0 && i < Outlen; ){
					switch( (c = ((unsigned char *)Outbuf)[i]) ){
					case 0x01: case 0x03: case 0x04: case 0x05:
					case 0x11: case 0x13: case 0x14: case 0x1C:
						break;
					default: c = 0; ++i;
						break;
					}
				}
				/* we now write the string from cnt to count+i-1 */
				n = i - cnt;
				DEBUG1("Send_job: tbcp writing %d", len );
				if( (len = Write_out_buffer( n, Outbuf+cnt, Job_timeout )) ){
					break;
				}
				if( c ){
					char b[2];
					DEBUG1("Send_job: tbcp escape 0x%02x", c );
					b[0] = '\001'; b[1] = c ^ 0x40;
					if( (len = Write_out_buffer( 2, b, Job_timeout )) ){
						break;
					}
					n += 1;
				}
				cnt += n;
			}
		} else if( mode == PS && Remove_ctrl ){
			for( i = 0; i < Outlen; ){
				c = cval(Outbuf+i);
				if( strchr( Remove_ctrl,c ) ){
					memmove( Outbuf+i, Outbuf+i+1,Outlen-i);
					--Outlen;
				} else {
					++i;
				}
			}
			len = Write_out_buffer( Outlen, Outbuf, Job_timeout );
		} else if( (mode == TEXT || mode == PCL) && Crlf ){
			DEBUG4("Send_job: crlf");
			for( cnt = 0; cnt < Outlen; ){
				len = 0; c = 0;
				for( i = cnt; c == 0 && i < Outlen; ){
					switch( (c = ((unsigned char *)Outbuf)[i]) ){
					case '\n':
						break;
					default: c = 0; ++i;
						break;
					}
				}
				/* we now write the string from cnt to count+i-1 */
				n = i - cnt;
				DEBUG1("Send_job: crlf writing %d", n );
				if( (len = Write_out_buffer( n, Outbuf+cnt, Job_timeout )) ){
					break;
				}
				if( c ){
					if( (len = Write_out_buffer( 2, "\r\n", Job_timeout )) ){
						break;
					}
					n += 1;
				}
				cnt += n;
			}
		} else {
			DEBUG1("Send_job: writing %d", Outlen );
			len = Write_out_buffer( Outlen, Outbuf, Job_timeout );
		}
		if( len > 0 ){
			Errorcode = JFAIL;
			fatal( "Send_job: job timed out" );
		} else if( len < 0 ){
			Errorcode = JFAIL;
			logerr_die( "Send_job: job failed during copy" );
		}
		Init_outbuf();
		DEBUG1("Send_job: total writen %0.0f", progress_total );
		/*
		 * now we do progress report
		 */
		if( progress_pc ){
			i = ((double)(progress_total)/total_size)*100;
			DEBUG1("Send_job: pc total %0.0f, new %d", progress_total, i );
			if( i > progress_last ){
				logmsg("Send_job: %d percent done", i);
				progress_last = i;
			}
		} else if( progress_k ){
			i = ((double)(progress_total)/(1024 * (double)progress_k));
			DEBUG1("Send_job: k total %0.0f, new %d", progress_total, i );
			if( i > progress_last ){
				logmsg("Send_job: %d Kbytes done", (int)(progress_total/1024));
				progress_last = i;
			}
		}
		if( Outlen == 0 && !done ){
				DEBUG1("Send_job: read %d", len );
			while( Outlen < Outmax 
					&& (len = read( 0, Outbuf+Outlen, Outmax - Outlen )) > 0 ){
				DEBUG1("Send_job: read %d", len );
				Outlen += len;
			}
			Outbuf[Outlen] = 0;
			DEBUG1("Send_job: read total %d", Outlen );
			if( len < 0 ){
				Errorcode = JFAIL;
				logerr_die( "Send_job: read error on stdin" );
			} else if( len == 0 ){
				done = 1;
			}
		}
	} while( Outlen > 0 );
	DEBUG1( "Send_job: finished file transfer" );

	Term_job( mode );

	Free_line_list(&l);
	Free_line_list(&files);
	Free_line_list(&match);
	logmsg( "Send_job: finished writing file, cleaning up" );
}

void URL_decode( char *s )
{
	if( s ) while( (s = strchr(s, '%')) ){
		if( (s[0] = s[1]) && (s[1] = s[2]) ){
			s[2] = 0;
			s[0] = strtol(s,0,16);
			memmove(s+1,s+3, strlen(s+3)+1);
			++s;
		} else {
			break;
		}
	}
}

/*
 * Builtins
 */

 struct keyvalue Builtin_values[] = {
/* we get the various UEL strings */
{"Font_download","font_download",(void *)Font_download},
{0,0,0}
};


/*
 * Process_OF_mode()
 *  We read from STDIN on a character by character basis.
 *  We then send the output to STDOUT until we are done
 *  If we get a STOP sequence, we suspend ourselves.
 *
 *  We actually have to wait on 3 FD's -
 *  0 to see if we have input
 *  1 to see if we have output
 *  Monitor_fd to see if there is an error
 *
 *  We will put the output in the Outbuf buffer
 *  Note:  if we need to suspend,  we do this AFTER we have
 *   written the output
 */

 static char stop[] = "\031\001";    /* sent to cause filter to suspend */

int Process_OF_mode( int job_timeout )
{
    fd_set readfds, writefds; /* for select() */
	struct timeval timeval, *tp;
	time_t current_t;
	int input, state = 0, suspend = 0, shutdown_done = 0,
		elapsed, count = 0, i, m, err, c, n;
	char *s;
	char buffer[LARGEBUFFER];

	count = 0;
	buffer[count] = 0;


	/* input FD */
	input = 0;
	
	Init_outbuf();

	while( input >= 0 || Outlen > 0 ){
		if( job_timeout > 0 ){
			time( &current_t );
			elapsed = current_t - Start_time;
			if( job_timeout > 0 && elapsed >= job_timeout ){
				break;
			}
			memset( &timeval, 0, sizeof(timeval) );
			timeval.tv_sec = m = job_timeout - elapsed;
			tp = &timeval;
			DEBUG4("Process_OF_mode: timeout now %d", m );
		} else {
			tp = 0;
		}
		FD_ZERO( &readfds );
		FD_ZERO( &writefds );
		m = 0;
		if( !suspend && input >= 0 ){
			FD_SET( input, &readfds );
			if( m <= input ) m = input+1;
		}
		if( Outlen > 0 ){
			FD_SET( 1, &writefds );
			if( m <= 1 ) m = 1+1;
		}
		if( Status ){
			FD_SET( 1, &readfds );
			if( m <= 1 ) m = 1+1;
		}
		if( Monitor_fd > 0 ){
			FD_SET( Monitor_fd, &readfds );
			if( m <= Monitor_fd ) m = Monitor_fd+1;
		}
		if(DEBUGL4){
			logDebug("Process_OF_mode: starting select, %d fds", m );
			for( i = 0; i < m; ++i ){
				if( FD_ISSET(i,&readfds) ) logDebug(" read wait fd %d", i);
			}
			for( i = 0; i < m; ++i ){
				if( FD_ISSET(i,&writefds) ) logDebug(" write wait fd %d", i);
			}
		}
		m = select( m,
			FD_SET_FIX((fd_set *))&readfds,
			FD_SET_FIX((fd_set *))&writefds,
			FD_SET_FIX((fd_set *))0, tp );
		err = errno;
		DEBUG4("Process_OF_mode: select returned %d", m );
		if( m < 0 ){
			/* error */
			if( err != EINTR ){
				Errorcode = JFAIL;
				logerr_die( "Process_OF_mode: select error" );
			}
		} else if( m == 0 ){
			/* timeout */
			break;
		}
		/* we read only if not suspending */
		if( input >= 0 && FD_ISSET( input, &readfds ) ){
			m = read( input, buffer+count, 1 );
			/* we have EOF on input */
			if( m <= 0 ){
				close( input );
				if( (m = open( "/dev/null", O_RDONLY )) < 0 ){
					Errorcode = JABORT;
					logerr_die( "cannot open /dev/null" );
				}
				if( m != input ){
					if( dup2(m, input) == -1 ){
						Errorcode = JABORT;
						logerr_die( "cannot dup2 /dev/null" );
					}
					close(m);
				}
				input = -1;
				/* write the rest out */
				if( count > 0 ) Put_outbuf_len( buffer, count );
				count = 0;
			} else {
				buffer[count+1] = 0;
				DEBUG2("Proces_OF_mode: read from input '%s'", buffer);
				/* now we work the state stuff */
				c = buffer[count];
				if( c && c == stop[state] ){
					/* we consume the escape sequence */
					++state;
					if( stop[state] == 0 ){
						/* flush input line and then stop. Do not do a banner */
						state = 0;
						suspend = 1;
						buffer[count] = 0;
				DEBUG2("Proces_OF_mode: suspend flag, flushing %d to out '%s'",
							count, buffer);
						if( count > 0 ){
							Put_outbuf_len( buffer, count );
						}
						count = 0;
					}
				} else {
					if( state ){
						for( i = 0; i < state; ++i ){
							buffer[count++] = stop[i];
						}
						state = 0;
					}
					buffer[count++] = c;
					buffer[count] = 0;
					DEBUG4("Process_OF_mode: read %d, total %d '%s'",m,count,buffer );
					Put_outbuf_len( buffer, count );
					count = 0;
				}
			}
		}
		/* now we check reading on FD 1 */
		if( FD_ISSET( 1, &readfds ) ){
			/* we have data available on STDOUT */
			n = Read_status_line( 1 );
			DEBUG4("Process_OF_mode: shutdown_done %d, read returned '%d',",
				shutdown_done, n );
			/* this handles the Appsocket case */
			if( n == 0 && shutdown_done ){
				break;
			}
			if( n < 0 || (n == 0 && !Ignore_eof) ){
				Errorcode = JFAIL;
				logerr_die("Process_OF_mode: read from printer failed");
			} else if( n > 0 ){
				while( (s = Get_inbuf_str()) );
			}
		}
		if( Monitor_fd >0 && FD_ISSET( Monitor_fd, &readfds ) ){
			/* we have data available on STDIN */
			if( Read_monitor_status_line( Monitor_fd ) > 0 ){
				while( (s = Get_monitorbuf_str()) );
			}
		}
		if( Outlen > 0 && FD_ISSET( 1, &writefds ) ){
			Outbuf[Outlen] = 0;
			DEBUG4("Process_OF_mode: writing '%s'", Outbuf);
			m = write( 1, Outbuf, Outlen );
			err = errno;
			DEBUG4("Process_OF_mode: wrote '%d'", m);
			if( m < 0 ){
				/* output is closed */
				if(
#ifdef EWOULDBLOCK
					err == EWOULDBLOCK
#else
# ifdef EAGAIN
					err == EAGAIN
# else
#error  No definition for EWOULDBLOCK or EAGAIN
# endif
#endif
								){
					DEBUG4( "Process_OF_mode: write would block but select says not!" );
					errno = err;
				} else if( err == EINTR ){
					/* usually timeout stuff */
					;
				} else {
					Errorcode = JFAIL;
					logerr_die("Process_OF_mode: cannot write to printer");
				}
			} else {
				/* move the strings down */
				memmove( Outbuf, Outbuf+m, Outlen-m );
				Outlen -= m;
				Outbuf[Outlen] = 0;
				DEBUG4("Process_OF_mode: %d left to write '%s'",
					Outlen, Outbuf);
			}
		}
		/* we get a line from the Outbuf and process it */
		if( Outlen == 0 && suspend && !shutdown_done ){
			logmsg( "Process_OF_mode: suspending");
			if( Appsocket && Status ){
				logmsg( "Process_OF_mode: appsocket shutdown fd 1");
				shutdown( 1, 1 );
				shutdown_done = 1;
			} else {
				break;
			}
		}
	}
	return( input ); 
	DEBUG4("Process_OF_mode: returning %d", input );
}

void Add_val_to_list( struct line_list *v, int c, char *key, char *sep )
{
	char *s = 0;
	if( isupper(c) ) s = Upperopts[c-'A'];
	if( islower(c) ) s = Loweropts[c-'a'];
	if( s ){
		s = safestrdup3(key,sep,s,__FILE__,__LINE__);
		DEBUG3("Add_val_to_list: '%s'",s );
		Add_line_list( v, s, sep, 1, 1 );
		free(s); s = 0;
	}
}


/***************************************************************************
 * int Get_max_fd()
 *  get the maximum number of file descriptors
 ***************************************************************************/

int Get_max_fd( void )
{
	int n = 0;	/* We need some sort of limit here */

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_NOFILE)
	struct rlimit pcount;
	if( getrlimit(RLIMIT_NOFILE, &pcount) == -1 ){
		fatal( "Get_max_fd: getrlimit failed" );
	}
	n = pcount.rlim_cur;
#else
# if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX)
	if( n == 0 && (n = sysconf(_SC_OPEN_MAX)) < 0 ){
		fatal( "Get_max_servers: sysconf failed" );
	}
# else
	n = 20;
# endif
#endif

	if( n < 0 || n > 10240 ){
		/* we have some systems that will return a VERY
		 * large or negative number for unlimited FD's.  Well, we
		 * don't want to use a large number here.  So we
		 * will make it a small number.  The actual number of
		 * file descriptors open by processes is VERY conservative.
		 */
		n = 256;
	}

	return( n );
}

/*
 * set the close on exec flag for a reasonable range of FD's
 */
void close_on_exec( int n )
{
    int fd, max = Get_max_fd();

    for( fd = n; fd < max; fd++ ){
		fcntl(fd, F_SETFD, 1);
	}
}

/*
 * Use_file_util(char *pgm, char *matches, struct line_list *args
 * 
 * We will need to use the file utility
 *  pgm = program to invoke
 *  matches = lines in form of 
 *      glob   returntype  program
 *  args = arguments to use if successful
 * RETURN 0 if no match
 *        != 0 if match
 */

void Use_file_util(char *pgm, struct line_list *result)
{
	int i, len, pid, n, fd, error_fd[2];
	plp_status_t status;
	char value[SMALLBUFFER], *s;
	struct line_list l;

	DEBUG1("Use_file_util: pgm '%s'", pgm);
	logmsg("Use_file_util: file program = '%s'", pgm );
	if( pipe(error_fd) == -1 ){
		Errorcode = JABORT;
		logerr_die("Use_file_util: pipe failed");
	}
	Init_line_list( &l );
	Free_line_list( result );
	fd = Make_tempfile(0);
	if( (pid = fork()) == -1 ){
		Errorcode = JABORT;
		logerr_die("Use_file_util: fork failed");
	} else if( pid == 0 ){
		/* child */
		/* grab the passed arguments of the program */
		if( pgm && strpbrk( pgm, "|><" ) ){
			Split(&l, "/bin/sh -c", Whitespace, 0, 0, 0, 1, 0 );
			Check_max(&l,2);
			l.list[l.count++] = pgm;
		} else {
			Split_cmd_line( &l, pgm, 0 );
		}
		Check_max(&l,1);
		l.list[l.count] = 0;
		for( i = 0; i < l.count; ++i ){
			l.list[i] = Fix_option_str( l.list[i], 0, 1, 1 );
		}
		if( dup2(fd,1) == -1 || dup2(error_fd[1],2) == -1 ){
			Errorcode = JABORT;
			logerr_die("Use_file_util: dup2 failed");
		}
		close_on_exec(3);
		execve( l.list[0], l.list, Envp );
		Errorcode = JABORT;
		logerr_die("Use_file_util: child execve failed");
	}
	close( error_fd[1] );
	/* ugly, but effective */
	len = 0;
	while( len < sizeof(value)-1
		&& (n = read(error_fd[0],value+len,sizeof(value)-1-len) > 0) ){
		value[n+len] = 0;
		while( (s = strchr(value,'\n')) ){
			*s++ = 0;
			logmsg("Use_file_util: '%s' error '%s'", pgm, value );
			memmove(value,s,strlen(s)+1);
		}
		len = strlen(value);
	}
	close( error_fd[0] );
	while( (n =  waitpid( pid, &status, 0 )) != pid );
	DEBUG1("Use_file_util: pid %d, exit '%s'", pid,
		Decode_status( &status ) );
	if( WIFEXITED(status) && (n = WEXITSTATUS(status)) ){
		Errorcode = JABORT;
		fatal("Use_file_util: process exited with status %d", n);
	} else if( WIFSIGNALED(status) ){
		Errorcode = JABORT;
		n = WTERMSIG(status);
		fatal("Use_file_util: process died with signal %d, '%s'",
			n, Sigstr(n));
	}
	if( lseek(fd,0,SEEK_SET) == -1 ){
		Errorcode = JABORT;
		logerr_die("Use_file_util: lseek failed");
	}
	value[0] = 0;
	if( (n = read(fd,value,sizeof(value)-1)) <= 0 ){
		Errorcode = JABORT;
		logerr_die("Use_file_util: read failed");
	}
	if( n > 0 ) value[n] = 0;
	close(fd);
	value[n] = 0;
	lowercase(value);
	while( (s = strchr(value,'\n')) ) *s = ' ';
	for( s = value; s[0] && s[1];  ){
		if( isspace(cval(s)) && isspace(cval(s+1))){
			memmove(s,s+1,strlen(s));
		} else {
			++s;
		}
	}
	logmsg("Use_file_util: file information = '%s'", value );
	DEBUG4("Use_file_util: file util done, '%s'", value );
	Add_line_list( result, value,0,0,0);
	Free_line_list(&l);
}



int Make_tempfile( char **retval )
{
	static int count;
	int fd;
	char buffer[32];
	char *tempfile, *t;

	if( !(tempfile = Find_str_value( &Model,"tempfile", Value_sep ))){
		Errorcode = JABORT;
		fatal(
		"Make_tempfile: missing 'tempfile' needed for conversion");
	}
	SNPRINTF(buffer,sizeof(buffer),"%02dXXXXXX", count);
	++count;
	t = safestrdup2(tempfile,buffer,__FILE__,__LINE__);
	DEBUG2( "Make_tempfile: tempfile '%s'", t );
	if( (fd = mkstemp(t)) == -1 ){
		Errorcode = JABORT;
		logerr_die("Make_tempfile: mkstemp failed with '%s%s'", tempfile,buffer);
	}
	DEBUG2( "Make_tempfile: new tempfile '%s', fd %d", t, fd );
	if( retval ){
		*retval = t;
	} else {
		unlink(t);
		free(t); t = 0;
	}
	return( fd );
}

void Make_stdin_file()
{
	int is_file, fd, in, n, i;
	struct stat statb;
	char *s;

	if( fstat( 0, &statb ) < 0 ){
		Errorcode = JABORT;
		logerr_die( "Make_stdin_file: cannot fstat fd 1" );
	}
	is_file = ((statb.st_mode & S_IFMT) == S_IFREG);
	DEBUG2( "Make_stdin_file: input is_file %d, size %d",
		is_file, (int)statb.st_size );
	if( !is_file ){
		fd = Make_tempfile( 0 );
		in = 1;
		do{
			n = 0;
			for( i = Outlen, s = Outbuf;
				i > 0 && (n = write(fd,s,i)) > 0;
				i -= n, s += n);
			if( n < 0 ){
				Errorcode = JABORT;
				logerr_die( "Make_stdin_file: write to tempfile fd %d failed", fd );
			}
			Outlen = 0;
			if( in > 0 ){
				n = 0;
				for( Outlen = 0;
					Outlen < Outmax
					&& (n = read(0,Outbuf+Outlen,Outmax-Outlen)) > 0;
					Outlen+=n);
				if( n < 0 ){
					Errorcode = JABORT;
					logerr_die( "Make_stdin_file: read from stdin" );
				} else if( n == 0 ){
					in = 0;
				}
			}
		} while( Outlen > 0 );
		if( dup2(fd,0) == -1 ){
			Errorcode = JABORT;
			logerr_die( "Make_stdin_file: read from stdin" );
		}
		if( fd != 0 ) close(fd);
		is_file = 1;
	}
	if( lseek(0,0,SEEK_SET) == -1 ){
		Errorcode = JABORT;
		logerr_die("Make_stdin_file: lseek failed");
	}
	Outlen = 0;
}

char * Set_mode_lang( char *s )
{
	char *mode = UNKNOWN;
	if( s ){
		if(!strcasecmp( s, PS )){ mode = PS; }
		else if(!strcasecmp( s, "ps" )){ mode = PS; }
		else if(!strcasecmp( s, "postscript" )){ mode = PS; }
		else if(!strcasecmp( s, PCL )){ mode = PCL; }
		else if(!strcasecmp( s, PJL )){ mode = PJL; }
		else if(!strcasecmp( s, TEXT )){ mode = TEXT; }
		else if(!strcasecmp( s, RAW )){ mode = RAW; }
	}
	return mode;
}

/* Fd_readable - check to see if you can read status from the file descriptor
 *  This is not as simple as you suspect.
 *  FIRST, try select.  If select fails, then you can't get status.
 *  NEXT, if it is a file, then you don't get status
 *  Now at this point you are at the mercy of a read call.
 *  but this will do a read and force you
 */

int Fd_readable( int fd )
{
	int m;
	struct stat statb;
    fd_set readfds;
	struct timeval timeval;

	if( fstat( fd, &statb ) < 0 ){
		Errorcode = JABORT;
		logerr_die( "Fd_readable: cannot fstat fd %d", fd );
	}
	DEBUG2( "Fd_readable: fd %d device 0%o", fd, statb.st_mode & S_IFMT );
	memset(&timeval,0,sizeof(timeval));

	FD_ZERO( &readfds );
	FD_SET( fd, &readfds );
	m = fd+1;

	errno = 0;
	m = select( m,
		FD_SET_FIX((fd_set *))&readfds,
		FD_SET_FIX((fd_set *))0,
		FD_SET_FIX((fd_set *))0, &timeval );
	DEBUG2( "Fd_readable: select %d, with error '%s'", m, Errormsg(errno) );

	m = (m >= 0);
	if( m ){
		m = ((statb.st_mode & S_IFMT) != S_IFREG);
	}
	DEBUG2( "Fd_readable: final value %d", m );
	return( m );
}

void Init_job( char *language )
{
	char buffer[SMALLBUFFER];
	struct line_list l;
	int len, i;
	char *s;

	Init_line_list(&l);

	DEBUG2("Init_job: language '%s'", language );
	if( language == TEXT ) language = PCL;
	if( Pjl && Pjl_enter && (language == PS || language == PCL) ){
		SNPRINTF( buffer, sizeof(buffer),
			PJL_ENTER_str, language );
		Put_pjl( buffer );
		memset( buffer, 0, sizeof(buffer) );
		/* 
		 * force nulls to be output
		 */
		for( i = Null_pad_count; i > 0; i -= len){
			len = sizeof(buffer);
			if( i < len ) len = i;
			Put_outbuf_len( buffer, len );
		}
	}
	DEBUG2("Init_job: after PJL ENTER '%s'", Outbuf );

	/* now we handle the various initializations */
	if( Pcl && language == PCL ){
		DEBUG1("Init_job: doing pcl init");
		if( !No_PCL_EOJ ) Put_outbuf_str( PCL_EXIT_str );
		if( (s = Find_str_value( &Model, "pcl_init", Value_sep)) ){
			DEBUG1("Init_job: 'pcl_init'='%s'", s);
			Free_line_list( &l );
			Split( &l, s, List_sep, 1, 0, 1, 1, 0 );
			Resolve_list( "pcl_", &l, Put_pcl );
			Free_line_list( &l );
		}
		DEBUG1("Init_job: 'pcl' and Topts");
		Resolve_user_opts( "pcl_", &Pcl_user_opts, &Topts, Put_pcl );
		DEBUG1("Init_job: 'pcl' and Zopts");
		Resolve_user_opts( "pcl_", &Pcl_user_opts, &Zopts, Put_pcl );
	} else if( Ps && language == PS ){
		DEBUG1("Init_job: doing ps init");
		if( !No_PS_EOJ && !Appsocket ) Put_outbuf_str(CTRL_D);
		if( Tbcp ){
			DEBUG3("Init_job: doing TBCP");
			Put_outbuf_str("\001M");
		}
		if( (s = Find_str_value(&Model, "ps_level_str",Value_sep)) ){
			Put_ps(s);
		} else {
			Put_ps("%!");
		}
		if( !Find_first_key( &Model, "ps_init", Value_sep, 0) ){
			s  = Find_str_value( &Model, "ps_init", Value_sep);
			DEBUG1("Init_job: 'ps_init'='%s'", s);
			Free_line_list( &l );
			Split( &l, s, List_sep, 1, 0, 1, 1, 0 );
			Resolve_list( "ps_", &l, Put_ps );
			Free_line_list( &l );
		}
		DEBUG1("Init_job: 'ps' and Topts");
		Resolve_user_opts( "ps_", &Ps_user_opts, &Topts, Put_ps );
		DEBUG1("Init_job: 'ps' and Zopts");
		Resolve_user_opts( "ps_", &Ps_user_opts, &Zopts, Put_ps );
	}
	DEBUG2("Init_job: final '%s'", Outbuf );
	len = Write_out_buffer( Outlen, Outbuf, Job_timeout );
	if( len ){
		Errorcode = JFAIL;
		fatal( "Init_job: job timed out language selection" );
	}
	Init_outbuf();
	Free_line_list(&l);
}

void Term_job( char *mode )
{
	int len;

	DEBUG2("Term_job: mode '%s'", mode );
	if( Pcl && ( mode == PCL || mode == TEXT ) ){
		Put_outbuf_str( PCL_EXIT_str );
	} else if( Ps && mode == PS ){
		Put_outbuf_str( CTRL_D );
	}
	len = Write_out_buffer( Outlen, Outbuf, Job_timeout );
	if( len ){
		Errorcode = JFAIL;
		fatal( "Term_job: job error on cleanup strings" );
	}
	Init_outbuf();
}

 extern struct font Font9x8;

void Text_banner(void)
{
	int linecount = 0, pagewidth = 0;
	struct line_list l;
	char buffer[SMALLBUFFER];
	char *s, *t;
	int i, n, pb;

	DEBUG4("Text_banner: starting");
	/* check the defaults, then the overrides */
	Init_line_list(&l);
	if( (s = Loweropts['l'-'a']) ){
		linecount = atoi(s);
	}
	if( linecount == 0 ){
		linecount = Find_flag_value(&Model,"page_length",Value_sep);
	}
	if( linecount == 0 ) linecount = 60;

	if( (s = Loweropts['w'-'a']) ){
		pagewidth = atoi(s);
	}
	if( pagewidth == 0 ){
		pagewidth = Find_flag_value(&Model,"page_width",Value_sep);
	}
	if( pagewidth == 0 ) pagewidth = 80;

	Check_max(&l,linecount+1);
	for( i = 0; i < linecount; ++i ){
		Add_line_list(&l,"",0,0,0);
	}

	/* we add top and bottom breaks */

	for( i = 0; i < sizeof(buffer)-1; ++i ){
		buffer[i] = '*';
	}
	buffer[i] = 0;
	if( pagewidth < sizeof(buffer)-1 ) buffer[pagewidth] = 0;
	if( !(pb = Find_flag_value(&Model,"page_break",Value_sep)) ){
		pb = 3;
	}
	
	for( i = 0; i < pb; ++i ){
		if( i < l.count ){
			free(l.list[i]);
			l.list[i] = safestrdup(buffer,__FILE__,__LINE__);
		}
		n = l.count - i - 1;
		if( n >= 0 ){
			free(l.list[n]);
			l.list[n] = safestrdup(buffer,__FILE__,__LINE__);
		}
	}
	DEBUG4("Text_banner: page width %d, page length %d", pagewidth, linecount);
	if(DEBUGL4)Dump_line_list("Text_banner- pagebreaks", &l);

	n = pb + 2;
	/*
	 * First, the job number, name, then the user name, then the other info
	 */
	if( (s = Loweropts['j'-'a']) ){
		n = bigfont(&Font9x8, s, &l, n);
		n += 2;
	}
	if( (s = Upperopts['J'-'A']) ){
		n = bigfont(&Font9x8, s, &l, n);
		n += 2;
	}

	if(DEBUGL4)Dump_line_list("Text_banner- subject", &l);

	if( (s = Upperopts['L'-'A']) || (s = Upperopts['P'-'A']) ){
		n = bigfont(&Font9x8, s, &l, n);
		n += 2;
	}
	DEBUG4("Text_banner: adding lines at %d", n );

	for( i = 'a'; n < l.count && i <= 'z'; ++i ){
		s = Loweropts[i-'a'];
		t = 0;
		switch(i){
			case 'j': t = "Seq"; break;
			case 'h': t = "Host"; break;
		}
		DEBUG4("Text_banner: '%c' - %s = %s", i, t, s );
		if( s && t ){
			SNPRINTF(buffer,sizeof(buffer),"%-7s: %s",t, s);
			free(l.list[n]);
			l.list[n] = safestrdup(buffer,__FILE__,__LINE__);
			n = n+1;
		}
	}
	for( i = 'A'; n < l.count && i <= 'Z'; ++i ){
		s = Upperopts[i-'A'];
		t = 0;
		switch(i){
			case 'C': t = "Class"; break;
			case 'D': t = "Date"; break;
			case 'H': t = "Host"; break;
			case 'J': t = "Job"; break;
			case 'L': t = "Banner User Name"; break;
			case 'N': t = "Person"; break;
			case 'Q': t = "Queue Name"; break;
			case 'R': t = "Account"; break;
		}
		DEBUG4("Text_banner: '%c' - %s = %s", i, t, s );
		if( s && t ){
			SNPRINTF(buffer,sizeof(buffer),"%-7s: %s",t, s);
			free(l.list[n]);
			l.list[n] = safestrdup(buffer,__FILE__,__LINE__);
			n = n+1;
		}
	}
	if(DEBUGL4)Dump_line_list("Text_banner",&l);
	for( i = 0; i < l.count; ++i ){
		Put_outbuf_str( l.list[i] );
		Put_outbuf_str( "\n" );
	}
	Free_line_list(&l);
}

/***************************************************************************
 * bigfornt( struct font *font, char * line, struct line_list *l,
 *  int startline )
 * print the line in big characters
 * for i = topline to bottomline do
 *  for each character in the line do
 *    get the scan line representation for the character
 *    foreach bit in the string do
 *       if the bit is set, print X, else print ' ';
 *        endfor
 *  endfor
 * endfor
 *
 ***************************************************************************/
void do_char( struct font *font, struct glyph *glyph,
	char *str, int line )
{
	int chars, i, j;
	char *s;

	/* if(debug)fprintf(stderr,"do_char: '%c', width %d\n", glyph->ch ); */
	chars = (font->width+7)/8;	/* calculate the row */
	s = &glyph->bits[line*chars];	/* get start of row */
	for( i = 0; i < chars; ++i ){	/* for each byte in row */
		for( j = 7; j >= 0; --j ){	/* from most to least sig bit */
			if( *s & (1<<j) ){		/* get bit value */
				*str = 'X';
			}
			++str;
		}
		++s;
	}
}


int bigfont( struct font *font, char *line, struct line_list *l, int start )
{
	int i, j, k, len;                   /* ACME Integers, Inc. */
	char *s = 0;

	DEBUG4("bigfont: '%s'", line );
	len = strlen(line);
	for( i = 0; start < l->count && i < font->height; ++i ){
		k = font->width * len;
		free(l->list[start]);
		s = malloc_or_die(k+1,__FILE__,__LINE__);
		l->list[start++] = s;
		memset(s,' ',k);
		s[k] = 0;
		for( j = 0; j < len; ++j){
			do_char( font, &font->glyph[line[j]-' '],
				&s[j * font->width], i );
		}
	}
	if( start < l->count ) ++start;
	return(start);
}

/*
 The font information is provided as entries in a data structure.

 The struct font{} entry specifies the character heights and widths,
 as well as the number of lines needed to display the characters.
 The struct glyph{} array is the set of glyphs for each character.

    
	{ 
	X__11___,
	X__11___,
	X__11___,
	X__11___,
	X__11___,
	X_______,
	X_______,
	X__11___,
	cX_11___},			/ * ! * /

     ^ lower left corner, i.e. - on baseline - x = 0, y = 8

	{
	X_______,
	X_______,
	X_______,
	X_111_1_,
	X1___11_,
	X1____1_,
	X1____1_,
	X1___11_,
	cX111_1_,
	X_____1_,
	X1____1_,
	X_1111__},			/ * g * /

     ^ lower left corner, i.e. - on baseline - x = 0, y = 8

 ***************************************************************************/

#define X_______ 0
#define X______1 01
#define X_____1_ 02
#define X____1__ 04
#define X____11_ 06
#define X___1___ 010
#define X___1__1 011
#define X___1_1_ 012
#define X___11__ 014
#define X__1____ 020
#define X__1__1_ 022
#define X__1_1__ 024
#define X__11___ 030
#define X__111__ 034
#define X__111_1 035
#define X__1111_ 036
#define X__11111 037
#define X_1_____ 040
#define X_1____1 041
#define X_1___1_ 042
#define X_1__1__ 044
#define X_1_1___ 050
#define X_1_1__1 051
#define X_1_1_1_ 052
#define X_11____ 060
#define X_11_11_ 066
#define X_111___ 070
#define X_111__1 071
#define X_111_1_ 072
#define X_1111__ 074
#define X_1111_1 075
#define X_11111_ 076
#define X_111111 077
#define X1______ 0100
#define X1_____1 0101
#define X1____1_ 0102
#define X1____11 0103
#define X1___1__ 0104
#define X1___1_1 0105
#define X1___11_ 0106
#define X1__1___ 0110
#define X1__1__1 0111
#define X1__11_1 0115
#define X1__1111 0117
#define X1_1____ 0120
#define X1_1___1 0121
#define X1_1_1_1 0125
#define X1_1_11_ 0126
#define X1_111__ 0134
#define X1_1111_ 0136
#define X11____1 0141
#define X11___1_ 0142
#define X11___11 0143
#define X11_1___ 0150
#define X11_1__1 0151
#define X111_11_ 0166
#define X1111___ 0170
#define X11111__ 0174
#define X111111_ 0176
#define X1111111 0177

 struct glyph g9x8[] = {
	{ ' ', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* */

	{ '!', 0, 8, {
	X__11___,
	X__11___,
	X__11___,
	X__11___,
	X__11___,
	X_______,
	X_______,
	X__11___,
	X__11___}},			/* ! */

	{ '"', 0, 8, {
	X_1__1__,
	X_1__1__,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* " */

	{ '#', 0, 8, {
	X_______,
	X__1_1__,
	X__1_1__,
	X1111111,
	X__1_1__,
	X1111111,
	X__1_1__,
	X__1_1__,
	X_______}},			/* # */

	{ '$', 0, 8, {
	X___1___,
	X_11111_,
	X1__1__1,
	X1__1___,
	X_11111_,
	X___1__1,
	X1__1__1,
	X_11111_,
	X___1___}},			/* $ */

	{ '%', 0, 8, {
	X_1_____,
	X1_1___1,
	X_1___1_,
	X____1__,
	X___1___,
	X__1____,
	X_1___1_,
	X1___1_1,
	X_____1_}},			/* % */

	{ '&', 0, 8, {
	X_11____,
	X1__1___,
	X1___1__,
	X_1_1___,
	X__1____,
	X_1_1__1,
	X1___11_,
	X1___11_,
	X_111__1}},			/* & */

	{ '\'', 0, 8, {
	X___11__,
	X___11__,
	X___1___,
	X__1____,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* ' */

	{ '(', 0, 8, {
	X____1__,
	X___1___,
	X__1____,
	X__1____,
	X__1____,
	X__1____,
	X__1____,
	X___1___,
	X____1__}},			/* ( */

	{ ')', 0, 8, {
	X__1____,
	X___1___,
	X____1__,
	X____1__,
	X____1__,
	X____1__,
	X____1__,
	X___1___,
	X__1____}},			/* ) */

	{ '*', 0, 8, {
	X_______,
	X___1___,
	X1__1__1,
	X_1_1_1_,
	X__111__,
	X_1_1_1_,
	X1__1__1,
	X___1___,
	X_______}},			/* * */

	{ '+', 0, 8, {
	X_______,
	X___1___,
	X___1___,
	X___1___,
	X1111111,
	X___1___,
	X___1___,
	X___1___,
	X_______}},			/* + */

	{ ',', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X__11___,
	X__11___,
	X__1____,
	X_1_____,
	X_______}},			/* , */

	{ '-', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_______,
	X1111111,
	X_______,
	X_______,
	X_______,
	X_______}},			/* - */

	{ '.', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X__11___,
	X__11___}},			/* . */

	{ '/', 0, 8, {
	X_______,
	X______1,
	X_____1_,
	X____1__,
	X___1___,
	X__1____,
	X_1_____,
	X1______,
	X_______}},			/* / */

	{ '0', 0, 8, {
	X_11111_,
	X1_____1,
	X1____11,
	X1___1_1,
	X1__1__1,
	X1_1___1,
	X11____1,
	X1_____1,
	X_11111_}},			/* 0 */

	{ '1', 0, 8, {
	X___1___,
	X__11___,
	X_1_1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X_11111_}},			/* 1 */

	{ '2', 0, 8, {
	X_11111_,
	X1_____1,
	X______1,
	X_____1_,
	X__111__,
	X_1_____,
	X1______,
	X1______,
	X1111111}},			/* 2 */

	{ '3', 0, 8, {
	X_11111_,
	X1_____1,
	X______1,
	X______1,
	X__1111_,
	X______1,
	X______1,
	X1_____1,
	X_11111_}},			/* 3 */

	{ '4', 0, 8, {
	X_____1_,
	X____11_,
	X___1_1_,
	X__1__1_,
	X_1___1_,
	X1____1_,
	X1111111,
	X_____1_,
	X_____1_}},			/* 4 */

	{ '5', 0, 8, {
	X1111111,
	X1______,
	X1______,
	X11111__,
	X_____1_,
	X______1,
	X______1,
	X1____1_,
	X_1111__}},			/* 5 */

	{ '6', 0, 8, {
	X__1111_,
	X_1_____,
	X1______,
	X1______,
	X1_1111_,
	X11____1,
	X1_____1,
	X1_____1,
	X_11111_}},			/* 6 */

	{ '7', 0, 8, {
	X1111111,
	X1_____1,
	X_____1_,
	X____1__,
	X___1___,
	X__1____,
	X__1____,
	X__1____,
	X__1____}},			/* 7 */

	{ '8', 0, 8, {
	X_11111_,
	X1_____1,
	X1_____1,
	X1_____1,
	X_11111_,
	X1_____1,
	X1_____1,
	X1_____1,
	X_11111_}},			/* 8 */

	{ '9', 0, 8, {
	X_11111_,
	X1_____1,
	X1_____1,
	X1_____1,
	X_111111,
	X______1,
	X______1,
	X1_____1,
	X_1111__}},			/* 9 */

	{ ':', 0, 8, {
	X_______,
	X_______,
	X_______,
	X__11___,
	X__11___,
	X_______,
	X_______,
	X__11___,
	X__11___}},			/* : */


	{ ';', 0, 8, {
	X_______,
	X_______,
	X_______,
	X__11___,
	X__11___,
	X_______,
	X_______,
	X__11___,
	X__11___,
	X__1____,
	X_1_____}},			/* ; */

	{ '<', 0, 8, {
	X____1__,
	X___1___,
	X__1____,
	X_1_____,
	X1______,
	X_1_____,
	X__1____,
	X___1___,
	X____1__}},			/* < */

	{ '=', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1111111,
	X_______,
	X1111111,
	X_______,
	X_______,
	X_______}},			/* = */

	{ '>', 0, 8, {
	X__1____,
	X___1___,
	X____1__,
	X_____1_,
	X______1,
	X_____1_,
	X____1__,
	X___1___,
	X__1____}},			/* > */

	{ '?', 0, 8, {
	X__1111_,
	X_1____1,
	X_1____1,
	X______1,
	X____11_,
	X___1___,
	X___1___,
	X_______,
	X___1___}},			/* ? */

	{ '@', 0, 8, {
	X__1111_,
	X_1____1,
	X1__11_1,
	X1_1_1_1,
	X1_1_1_1,
	X1_1111_,
	X1______,
	X_1____1,
	X__1111_}},			/* @ */

	{ 'A', 0, 8, {
	X__111__,
	X_1___1_,
	X1_____1,
	X1_____1,
	X1111111,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1}},			/* A */

	{ 'B', 0, 8, {
	X111111_,
	X_1____1,
	X_1____1,
	X_1____1,
	X_11111_,
	X_1____1,
	X_1____1,
	X_1____1,
	X111111_}},			/* B */

	{ 'C', 0, 8, {
	X__1111_,
	X_1____1,
	X1______,
	X1______,
	X1______,
	X1______,
	X1______,
	X_1____1,
	X__1111_}},			/* C */

	{ 'D', 0, 8, {
	X11111__,
	X_1___1_,
	X_1____1,
	X_1____1,
	X_1____1,
	X_1____1,
	X_1____1,
	X_1___1_,
	X11111__}},			/* D */

	{ 'E', 0, 8, {
	X1111111,
	X1______,
	X1______,
	X1______,
	X111111_,
	X1______,
	X1______,
	X1______,
	X1111111}},			/* E */

	{ 'F', 0, 8, {
	X1111111,
	X1______,
	X1______,
	X1______,
	X111111_,
	X1______,
	X1______,
	X1______,
	X1______}},			/* F */

	{ 'G', 0, 8, {
	X__1111_,
	X_1____1,
	X1______,
	X1______,
	X1______,
	X1__1111,
	X1_____1,
	X_1____1,
	X__1111_}},			/* G */

	{ 'H', 0, 8, {
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1111111,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1}},			/* H */

	{ 'I', 0, 8, {
	X_11111_,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X_11111_}},			/* I */

	{ 'J', 0, 8, {
	X__11111,
	X____1__,
	X____1__,
	X____1__,
	X____1__,
	X____1__,
	X____1__,
	X1___1__,
	X_111___}},			/* J */

	{ 'K', 0, 8, {
	X1_____1,
	X1____1_,
	X1___1__,
	X1__1___,
	X1_1____,
	X11_1___,
	X1___1__,
	X1____1_,
	X1_____1}},			/* K */

	{ 'L', 0, 8, {
	X1______,
	X1______,
	X1______,
	X1______,
	X1______,
	X1______,
	X1______,
	X1______,
	X1111111}},			/* L */

	{ 'M', 0, 8, {
	X1_____1,
	X11___11,
	X1_1_1_1,
	X1__1__1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1}},			/* M */

	{ 'N', 0, 8, {
	X1_____1,
	X11____1,
	X1_1___1,
	X1__1__1,
	X1___1_1,
	X1____11,
	X1_____1,
	X1_____1,
	X1_____1}},			/* N */

	{ 'O', 0, 8, {
	X__111__,
	X_1___1_,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X_1___1_,
	X__111__}},			/* O */

	{ 'P', 0, 8, {
	X111111_,
	X1_____1,
	X1_____1,
	X1_____1,
	X111111_,
	X1______,
	X1______,
	X1______,
	X1______}},			/* P */

	{ 'Q', 0, 8, {
	X__111__,
	X_1___1_,
	X1_____1,
	X1_____1,
	X1_____1,
	X1__1__1,
	X1___1_1,
	X_1___1_,
	X__111_1}},			/* Q */

	{ 'R', 0, 8, {
	X111111_,
	X1_____1,
	X1_____1,
	X1_____1,
	X111111_,
	X1__1___,
	X1___1__,
	X1____1_,
	X1_____1}},			/* R */

	{ 'S', 0, 8, {
	X_11111_,
	X1_____1,
	X1______,
	X1______,
	X_11111_,
	X______1,
	X______1,
	X1_____1,
	X_11111_}},			/* S */

	{ 'T', 0, 8, {
	X1111111,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___}},			/* T */

	{ 'U', 0, 8, {
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X_11111_}},			/* U */

	{ 'V', 0, 8, {
	X1_____1,
	X1_____1,
	X1_____1,
	X_1___1_,
	X_1___1_,
	X__1_1__,
	X__1_1__,
	X___1___,
	X___1___}},			/* V */

	{ 'W', 0, 8, {
	X1_____1,
	X1_____1,
	X1_____1,
	X1_____1,
	X1__1__1,
	X1__1__1,
	X1_1_1_1,
	X11___11,
	X1_____1}},			/* W */

	{ 'X', 0, 8, {
	X1_____1,
	X1_____1,
	X_1___1_,
	X__1_1__,
	X___1___,
	X__1_1__,
	X_1___1_,
	X1_____1,
	X1_____1}},			/* X */

	{ 'Y', 0, 8, {
	X1_____1,
	X1_____1,
	X_1___1_,
	X__1_1__,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___}},			/* Y */

	{ 'Z', 0, 8, {
	X1111111,
	X______1,
	X_____1_,
	X____1__,
	X___1___,
	X__1____,
	X_1_____,
	X1______,
	X1111111}},			/* Z */

	{ '[', 0, 8, {
	X_1111__,
	X_1_____,
	X_1_____,
	X_1_____,
	X_1_____,
	X_1_____,
	X_1_____,
	X_1_____,
	X_1111__}},			/* [ */

	{ '\\', 0, 8, {
	X_______,
	X1______,
	X_1_____,
	X__1____,
	X___1___,
	X____1__,
	X_____1_,
	X______1,
	X_______}},			/* \ */

	{ ']', 0, 8, {
	X__1111_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X__1111_}},			/* ] */

	{ '^', 0, 8, {
	X___1___,
	X__1_1__,
	X_1___1_,
	X1_____1,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* ^ */

	{ '_', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X1111111,
	X_______}},			/* _ */

	{ '`', 0, 8, {
	X__11___,
	X__11___,
	X___1___,
	X____1__,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* ` */

	{ 'a', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_1111__,
	X_____1_,
	X_11111_,
	X1_____1,
	X1____11,
	X_1111_1}},			/* a */

	{ 'b', 0, 8, {
	X1______,
	X1______,
	X1______,
	X1_111__,
	X11___1_,
	X1_____1,
	X1_____1,
	X11___1_,
	X1_111__}},			/* b */

	{ 'c', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_1111__,
	X1____1_,
	X1______,
	X1______,
	X1____1_,
	X_1111__}},			/* c */

	{ 'd', 0, 8, {
	X_____1_,
	X_____1_,
	X_____1_,
	X_111_1_,
	X1___11_,
	X1____1_,
	X1____1_,
	X1___11_,
	X_111_1_}},			/* d */

	{ 'e', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_1111__,
	X1____1_,
	X111111_,
	X1______,
	X1____1_,
	X_1111__}},			/* e */

	{ 'f', 0, 8, {
	X___11__,
	X__1__1_,
	X__1____,
	X__1____,
	X11111__,
	X__1____,
	X__1____,
	X__1____,
	X__1____}},			/* f */

	{ 'g', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_111_1_,
	X1___11_,
	X1____1_,
	X1____1_,
	X1___11_,
	X_111_1_,
	X_____1_,
	X1____1_,
	X_1111__}},			/* g */

	{ 'h', 0, 8, {
	X1______,
	X1______,
	X1______,
	X1_111__,
	X11___1_,
	X1____1_,
	X1____1_,
	X1____1_,
	X1____1_}},			/* h */

	{ 'i', 0, 8, {
	X_______,
	X___1___,
	X_______,
	X__11___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X__111__}},			/* i */

	{ 'j', 0, 8, {
	X_______,
	X_______,
	X_______,
	X____11_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_____1_,
	X_1___1_,
	X__111__}},			/* j */

	{ 'k', 0, 8, {
	X1______,
	X1______,
	X1______,
	X1___1__,
	X1__1___,
	X1_1____,
	X11_1___,
	X1___1__,
	X1____1_}},			/* k */

	{ 'l', 0, 8, {
	X__11___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X__111__}},			/* l */

	{ 'm', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_1_11_,
	X11_1__1,
	X1__1__1,
	X1__1__1,
	X1__1__1,
	X1__1__1}},			/* m */

	{ 'n', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_111__,
	X11___1_,
	X1____1_,
	X1____1_,
	X1____1_,
	X1____1_}},			/* n */

	{ 'o', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_1111__,
	X1____1_,
	X1____1_,
	X1____1_,
	X1____1_,
	X_1111__}},			/* o */


	{ 'p', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_111__,
	X11___1_,
	X1____1_,
	X1____1_,
	X11___1_,
	X1_111__,
	X1______,
	X1______,
	X1______}},			/* p */

	{ 'q', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_111_1_,
	X1___11_,
	X1____1_,
	X1____1_,
	X1___11_,
	X_111_1_,
	X_____1_,
	X_____1_,
	X_____1_}},			/* q */

	{ 'r', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_111__,
	X11___1_,
	X1______,
	X1______,
	X1______,
	X1______}},			/* r */

	{ 's', 0, 8, {
	X_______,
	X_______,
	X_______,
	X_1111__,
	X1____1_,
	X_11____,
	X___11__,
	X1____1_,
	X_1111__}},			/* s */

	{ 't', 0, 8, {
	X_______,
	X__1____,
	X__1____,
	X11111__,
	X__1____,
	X__1____,
	X__1____,
	X__1__1_,
	X___11__}},			/* t */

	{ 'u', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1____1_,
	X1____1_,
	X1____1_,
	X1____1_,
	X1___11_,
	X_111_1_}},			/* u */

	{ 'v', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_____1,
	X1_____1,
	X1_____1,
	X_1___1_,
	X__1_1__,
	X___1___}},			/* v */

	{ 'w', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1_____1,
	X1__1__1,
	X1__1__1,
	X1__1__1,
	X1__1__1,
	X_11_11_}},			/* w */

	{ 'x', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1____1_,
	X_1__1__,
	X__11___,
	X__11___,
	X_1__1__,
	X1____1_}},			/* x */

	{ 'y', 0, 8, {
	X_______,
	X_______,
	X_______,
	X1____1_,
	X1____1_,
	X1____1_,
	X1____1_,
	X1___11_,
	X_111_1_,
	X_____1_,
	X1____1_,
	X_1111__}},			/* y */

	{ 'z', 0, 8, {
	X_______,
	X_______,
	X_______,
	X111111_,
	X____1__,
	X___1___,
	X__1____,
	X_1_____,
	X111111_}},			/* z */

	{ '}', 0, 8, {
	X___11__,
	X__1____,
	X__1____,
	X__1____,
	X_1_____,
	X__1____,
	X__1____,
	X__1____,
	X___11__}},			/* } */

	{ '|', 0, 8, {
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___,
	X___1___}},			/* | */

	{ '}', 0, 8, {
	X__11___,
	X____1__,
	X____1__,
	X____1__,
	X_____1_,
	X____1__,
	X____1__,
	X____1__,
	X__11___}},			/* } */

	{ '~', 0, 8, {
	X_11____,
	X1__1__1,
	X____11_,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______,
	X_______}},			/* ~ */

	{ 'X', 0, 8, {
	X_1__1__,
	X1__1__1,
	X__1__1_,
	X_1__1__,
	X1__1__1,
	X__1__1_,
	X_1__1__,
	X1__1__1,
	X__1__1_}}			/* rub-out */
};

/*
  9 by 8 font:
  12 rows high, 8 cols wide, 9 lines above baseline
 */
 struct font Font9x8 = {
	12, 8, 9, g9x8 
};

/***************************************************************************
 * Decode_status (plp_status_t *status)
 * returns a printable string encoding return status
 ***************************************************************************/

const char *Decode_status (plp_status_t *status)
{
    static char msg[128];

	int n;
    *msg = 0;		/* just in case! */
    if (WIFEXITED (*status)) {
		n = WEXITSTATUS(*status);
		if( n > 0 && n < 32 ) n += JFAIL-1;
		(void) SNPRINTF (msg, sizeof(msg),
		"exit status %d", WEXITSTATUS(*status) );
    } else if (WIFSTOPPED (*status)) {
		(void) strcpy(msg, "stopped");
    } else {
		(void) SNPRINTF (msg, sizeof(msg), "died%s",
			WCOREDUMP (*status) ? " and dumped core" : "");
		if (WTERMSIG (*status)) {
			(void) SNPRINTF(msg + strlen (msg), sizeof(msg)-strlen(msg),
				 ", %s", Sigstr ((int) WTERMSIG (*status)));
		}
    }
    return (msg);
}

/***************************************************************************
 * plp_usleep() with select - simple minded way to avoid problems
 ***************************************************************************/
int plp_usleep( int i )
{
	struct timeval t;
	DEBUG3("plp_usleep: starting usleep %d", i );
	if( i > 0 ){
		memset( &t, 0, sizeof(t) );
		t.tv_usec = i%1000000;
		t.tv_sec = i/1000000;
		DEBUG3("plp_usleep: %d sec, %d microsec", t.tv_sec, t.tv_usec );
		i = select( 0,
			FD_SET_FIX((fd_set *))(0),
			FD_SET_FIX((fd_set *))(0),
			FD_SET_FIX((fd_set *))(0),
			&t );
		DEBUG3("plp_usleep: select done, status %d, errno %d, '%s'", i, errno, Errormsg(errno) );
	}
	return( i );
}

void Strip_leading_spaces ( char **vp )
{
	char *s = *vp;
	if( s ){
		do{
			while( isspace( cval(s) ) ) memmove(s,s+1,strlen(s+1)+1); 
			if( (s = strchr( s, '\n' )) ) ++s;
		} while(s);
		if( **vp == 0 ) *vp = 0;
	}
}
