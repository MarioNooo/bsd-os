/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 *
 ***************************************************************************/

 static char *const _id =
"$Id: krb5_auth.c,v 5.9 2000/04/14 20:39:53 papowell Exp papowell $";


#include "lp.h"
#include "fileopen.h"
#include "child.h"
#include "getqueue.h"
#include "krb5_auth.h"

#if defined(HAVE_KRB5_H)

#include <krb5.h>
#include <com_err.h>

#undef FREE_KRB_DATA
#if defined(HAVE_KRB5_FREE_DATA_CONTENTS)
#define FREE_KRB_DATA(context,data,suffix) krb5_free_data_contents(context,&data); data suffix = 0
#else
# if defined(HAVE_KRB5_XFREE)
# define FREE_KRB_DATA(context,data,suffix) krb5_xfree(data suffix); data suffix = 0
# else
#  if defined(HAVE_KRB_XFREE)
#  else
#   define FREE_KRB_DATA(context,data,suffix) krb_xfree(data suffix); data suffix = 0
#  error missing krb_xfree value or definition
#  endif
# endif
#endif

 extern krb5_error_code krb5_read_message 
	KRB5_PROTOTYPE((krb5_context,
		   krb5_pointer, 
		   krb5_data *));
 extern krb5_error_code krb5_write_message 
	KRB5_PROTOTYPE((krb5_context,
		   krb5_pointer, 
		   krb5_data *));
/*
 * server_krb5_auth(
 *  char *keytabfile,	server key tab file - /etc/lpr.keytab
 *  char *service,		service is usually "lpr"
 *  int sock,		   socket for communications
 *  char *auth, int len authname buffer, max size
 *  char *err, int errlen error message buffer, max size
 * RETURNS: 0 if successful, non-zero otherwise, error message in err
 *   Note: there is a memory leak if authentication fails,  so this
 *   should not be done in the main or non-exiting process
 */
 extern int des_read( krb5_context context, krb5_encrypt_block *eblock,
	int fd, char *buf, int len, char *err, int errlen );
 extern int des_write( krb5_context context, krb5_encrypt_block *eblock,
	int fd, char *buf, int len, char *err, int errlen );

 /* we make these statics */
 static krb5_context context = 0;
 static krb5_auth_context auth_context = 0;
 static krb5_keytab keytab = 0;  /* Allow specification on command line */
 static krb5_principal server = 0;
 static krb5_ticket * ticket = 0;

 int server_krb5_auth( char *keytabfile, char *service, int sock,
	char **auth, char *err, int errlen, char *file )
{
	int retval = 0;
	int fd = -1;
	krb5_data   inbuf, outbuf;
	struct stat statb;
	int status;
	char *cname = 0;

	DEBUG1("server_krb5_auth: keytab '%s', service '%s', sock %d, file '%s'",
		keytabfile, service, sock, file );
	if( !keytabfile ){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"no server keytab file",
			Is_server?"on server":"on client" );
		retval = 1;
		goto done;
	}
	if( (fd = Checkread(keytabfile,&statb)) == -1 ){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"cannot open server keytab file '%s' - %s",
			Is_server?"on server":"on client",
			keytabfile,
			Errormsg(errno) );
		retval = 1;
		goto done;
	}
	close(fd);
	err[0] = 0;
	if ((retval = krb5_init_context(&context))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_init_context failed - '%s' ",
			Is_server?"on server":"on client",
			error_message(retval) );
		goto done;
	}
	if( keytab == 0 && (retval = krb5_kt_resolve(context, keytabfile, &keytab) ) ){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_kt_resolve failed - file %s '%s'",
			Is_server?"on server":"on client",
			keytabfile,
			error_message(retval) );
		goto done;
	}
	if ((retval = krb5_sname_to_principal(context, NULL, service, 
					 KRB5_NT_SRV_HST, &server))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_sname_to_principal failed - service %s '%s'",
			Is_server?"on server":"on client",
			service, error_message(retval));
		goto done;
	}
	if((retval = krb5_unparse_name(context, server, &cname))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_unparse_name failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}
	DEBUG1("server_krb5_auth: server '%s'", cname );

	if((retval = krb5_recvauth(context, &auth_context, (krb5_pointer)&sock,
				   service , server, 
				   0,   /* no flags */
				   keytab,  /* default keytab is NULL */
				   &ticket))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_recvauth '%s' failed '%s'",
			Is_server?"on server":"on client",
			cname, error_message(retval));
		goto done;
	}

	/* Get client name */
	if((retval = krb5_unparse_name(context, 
		ticket->enc_part2->client, &cname))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_unparse_name failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}
	if( auth ) *auth = safestrdup( cname,__FILE__,__LINE__);
	DEBUG1( "server_krb5_auth: client '%s'", cname );
    /* initialize the initial vector */
    if((retval = krb5_auth_con_initivector(context, auth_context))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_auth_con_initvector failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}

	krb5_auth_con_setflags(context, auth_context,
		KRB5_AUTH_CONTEXT_DO_SEQUENCE);
	if((retval = krb5_auth_con_genaddrs(context, auth_context, sock,
		KRB5_AUTH_CONTEXT_GENERATE_LOCAL_FULL_ADDR |
			KRB5_AUTH_CONTEXT_GENERATE_REMOTE_FULL_ADDR))){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"krb5_auth_con_genaddr failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}
  
	memset( &inbuf, 0, sizeof(inbuf) );
	memset( &outbuf, 0, sizeof(outbuf) );

	fd = Checkwrite( file, &statb, O_WRONLY|O_TRUNC, 1, 0 );
	DEBUG1( "server_krb5_auth: opened for write '%s', fd %d", file, fd );
	if( fd < 0 ){
		SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
			"file open failed: %s",
			Is_server?"on server":"on client",
			Errormsg(errno));
		retval = 1;
		goto done;
	}
	while( (retval = krb5_read_message(context,&sock,&inbuf)) == 0 ){
		if(DEBUGL5){
			char small[16];
			memcpy(small,inbuf.data,sizeof(small)-1);
			small[sizeof(small)-1] = 0;
			logDebug( "server_krb5_auth: got %d, '%s'",
				inbuf.length, small );
		}
		if((retval = krb5_rd_priv(context,auth_context,
			&inbuf,&outbuf,NULL))){
			SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
				"krb5_rd_safe failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			retval = 1;
			goto done;
		}
		if( outbuf.data ) outbuf.data[outbuf.length] = 0;
		status = Write_fd_len( fd, outbuf.data, outbuf.length );
		if( status < 0 ){
			SNPRINTF( err, errlen, "%s server_krb5_auth failed - "
				"write to file failed: %s",
				Is_server?"on server":"on client",
				Errormsg(errno));
			retval = 1;
			goto done;
		}
		/*FREE_KRB_DATA(context,inbuf.data);
		FREE_KRB_DATA(context,outbuf.data);
		*/
		FREE_KRB_DATA(context,inbuf,.data);
		FREE_KRB_DATA(context,outbuf,.data);
		inbuf.length = 0;
		outbuf.length = 0;
	}
	retval = 0;
	close(fd); fd = -1;

 done:
	if( cname )		free(cname); cname = 0;
	if( retval ){
		if( fd >= 0 )	close(fd);
		if( ticket )	krb5_free_ticket(context, ticket);
		ticket = 0;
		if( context && server )	krb5_free_principal(context, server);
		server = 0;
		if( context && auth_context)	krb5_auth_con_free(context, auth_context );
		auth_context = 0;
		if( context )	krb5_free_context(context);
		context = 0;
	}
	DEBUG1( "server_krb5_auth: retval %d, error: '%s'", retval, err );
	return(retval);
}


int server_krb5_status( int sock, char *err, int errlen, char *file )
{
	int fd = -1;
	int retval = 0;
	struct stat statb;
	char buffer[SMALLBUFFER];
	krb5_data   inbuf, outbuf;

	err[0] = 0;
	memset( &inbuf, 0, sizeof(inbuf) );
	memset( &outbuf, 0, sizeof(outbuf) );

	fd = Checkread( file, &statb );
	if( fd < 0 ){
		SNPRINTF( err, errlen,
			"file open failed: %s", Errormsg(errno));
		retval = 1;
		goto done;
	}
	DEBUG1( "server_krb5_status: sock '%d', file size %0.0f", sock, (double)(statb.st_size));

	while( (retval = read( fd,buffer,sizeof(buffer)-1)) > 0 ){
		inbuf.length = retval;
		inbuf.data = buffer;
		buffer[retval] = 0;
		DEBUG4("server_krb5_status: sending '%s'", buffer );
		if((retval = krb5_mk_priv(context,auth_context,
			&inbuf,&outbuf,NULL))){
			SNPRINTF( err, errlen, "%s server_krb5_status failed - "
				"krb5_mk_priv failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			retval = 1;
			goto done;
		}
		DEBUG4("server_krb5_status: encoded length '%d'", outbuf.length );
		if((retval= krb5_write_message(context,&sock,&outbuf))){
			SNPRINTF( err, errlen, "%s server_krb5_status failed - "
				"krb5_write_message failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			retval = 1;
			goto done;
		}
		FREE_KRB_DATA(context,outbuf,.data);
	}
	DEBUG1("server_krb5_status: done" );

 done:
	if( fd >= 0 )	close(fd);
	if( ticket )	krb5_free_ticket(context, ticket);
	ticket = 0;
	if( context && server )	krb5_free_principal(context, server);
	server = 0;
	if( context && auth_context)	krb5_auth_con_free(context, auth_context );
	auth_context = 0;
	if( context )	krb5_free_context(context);
	context = 0;
	DEBUG1( "server_krb5_status: retval %d, error: '%s'", retval, err );
	return(retval);
}

/*
 * client_krb5_auth(
 *  char * keytabfile	- keytabfile, NULL for users, file name for server
 *  char * service		-service, usually "lpr"
 *  char * host			- server host name
 *  char * principal	- server principal
 *  int options		 - options for server to server
 *  char *life			- lifetime of ticket
 *  char *renew_time	- renewal time of ticket
 *  char *err, int errlen - buffer for error messages 
 *  char *file			- file to transfer
 */ 
#define KRB5_DEFAULT_OPTIONS 0
#define KRB5_DEFAULT_LIFE 60*60*10 /* 10 hours */
#define VALIDATE 0
#define RENEW 1

 extern krb5_error_code krb5_tgt_gen( krb5_context context, krb5_ccache ccache,
	krb5_principal server, krb5_data *outbuf, int opt );

int client_krb5_auth( char *keytabfile, char *service, char *host,
	char *server_principal,
	int options, char *life, char *renew_time,
	int sock, char *err, int errlen, char *file )
{
	krb5_context context = 0;
	krb5_principal client = 0, server = 0;
	krb5_error *err_ret = 0;
	krb5_ap_rep_enc_part *rep_ret = 0;
	krb5_data cksum_data;
	krb5_ccache ccdef;
	krb5_auth_context auth_context = 0;
	krb5_timestamp now;
	krb5_deltat lifetime = KRB5_DEFAULT_LIFE;   /* -l option */
	krb5_creds my_creds;
	krb5_creds *out_creds = 0;
	krb5_keytab keytab = 0;
	krb5_deltat rlife = 0;
	krb5_address **addrs = (krb5_address **)0;
	krb5_encrypt_block eblock;	  /* eblock for encrypt/decrypt */
	krb5_data   inbuf, outbuf;
	int retval = 0;
	char *cname = 0;
	char *sname = 0;
	int fd = -1, len;
	char buffer[SMALLBUFFER];
	struct stat statb;

	err[0] = 0;
	DEBUG1( "client_krb5_auth: euid/egid %d/%d, ruid/rguid %d/%d, keytab '%s',"
		" service '%s', host '%s', sock %d, file '%s'",
		geteuid(),getegid(), getuid(),getgid(),
		keytabfile, service, host, sock, file );
	if( !safestrcasecmp(host,LOCALHOST) ){
		host = FQDNHost_FQDN;
		DEBUG1( "client_krb5_auth: using host='%s'", host );
	}
	memset((char *)&my_creds, 0, sizeof(my_creds));
	memset((char *)&outbuf, 0, sizeof(outbuf));
	memset((char *)&eblock, 0, sizeof(eblock));
	options |= KRB5_DEFAULT_OPTIONS;

	if ((retval = krb5_init_context(&context))){
		SNPRINTF( err, errlen, "%s '%s'",
			"%s krb5_init_context failed - '%s' ",
			Is_server?"on server":"on client",
			error_message(retval) );
		goto done;
	}
	if (!valid_cksumtype(CKSUMTYPE_CRC32)) {
		SNPRINTF( err, errlen,
			"valid_cksumtype CKSUMTYPE_CRC32 - %s",
			error_message(KRB5_PROG_SUMTYPE_NOSUPP) );
		retval = 1;
		goto done;
	}

	if(server_principal){
		if ((retval = krb5_parse_name(context,server_principal, &server))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"when parsing name '%s'"
			" - %s",
			Is_server?"on server":"on client",
			server_principal, error_message(retval) );
			goto done;
		}
	} else {
		/* XXX perhaps we want a better metric for determining localhost? */
		if (strncasecmp("localhost", host, sizeof(host)))
			retval = krb5_sname_to_principal(context, host, service,
							 KRB5_NT_SRV_HST, &server);
		else
			/* Let libkrb5 figure out its notion of the local host */
			retval = krb5_sname_to_principal(context, NULL, service,
							 KRB5_NT_SRV_HST, &server);
		if (retval) {
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"when parsing service/host '%s'/'%s'"
			" - %s",
			Is_server?"on server":"on client",
			service,host,error_message(retval) );
			goto done;
		}
	}

	if((retval = krb5_unparse_name(context, server, &sname))){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			" krb5_unparse_name of 'server' failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}
	DEBUG1( "client_krb5_auth: server '%s'", sname );

	my_creds.server = server;

	if( keytabfile ){
		if ((retval = krb5_sname_to_principal(context, NULL, service, 
			 KRB5_NT_SRV_HST, &client))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"when parsing name '%s'"
			" - %s",
			Is_server?"on server":"on client",
			service, error_message(retval) );
			goto done;
		}
		if(cname)free(cname); cname = 0;
		if((retval = krb5_unparse_name(context, client, &cname))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_unparse_name of 'me' failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			goto done;
		}
		DEBUG1("client_krb5_auth: client '%s'", cname );
		my_creds.client = client;
		if((retval = krb5_kt_resolve(context, keytabfile, &keytab))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			 "resolving keytab '%s'"
			" '%s' - ",
			Is_server?"on server":"on client",
			keytabfile, error_message(retval) );
			goto done;
		}
		if ((retval = krb5_timeofday(context, &now))) {
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			 "getting time of day"
			" - '%s'",
			Is_server?"on server":"on client",
			error_message(retval) );
			goto done;
		}
		if( life && (retval = krb5_string_to_deltat(life, &lifetime)) ){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"bad lifetime value '%s'"
			" '%s' - ",
			Is_server?"on server":"on client",
			life, error_message(retval) );
			goto done;
		}
		if( renew_time ){
			options |= KDC_OPT_RENEWABLE;
			if( (retval = krb5_string_to_deltat(renew_time, &rlife))){
				SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"bad renew time value '%s'"
				" '%s' - ",
				Is_server?"on server":"on client",
				renew_time, error_message(retval) );
				goto done;
			}
		}
		if((retval = krb5_cc_default(context, &ccdef))) {
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"while getting default ccache"
			" - %s",
			Is_server?"on server":"on client",
			error_message(retval) );
			goto done;
		}

		my_creds.times.starttime = 0;	 /* start timer when request */
		my_creds.times.endtime = now + lifetime;

		if(options & KDC_OPT_RENEWABLE) {
			my_creds.times.renew_till = now + rlife;
		} else {
			my_creds.times.renew_till = 0;
		}

		if(options & KDC_OPT_VALIDATE){
			/* stripped down version of krb5_mk_req */
			if( (retval = krb5_tgt_gen(context,
				ccdef, server, &outbuf, VALIDATE))) {
				SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"validating tgt"
				" - %s",
				Is_server?"on server":"on client",
				error_message(retval) );
				DEBUG1("%s", err );
			}
		}

		if (options & KDC_OPT_RENEW) {
			/* stripped down version of krb5_mk_req */
			if( (retval = krb5_tgt_gen(context,
				ccdef, server, &outbuf, RENEW))) {
				SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"renewing tgt"
				" - %s",
				Is_server?"on server":"on client",
				error_message(retval) );
				DEBUG1("%s", err );
			}
		}

		if((retval = krb5_get_in_tkt_with_keytab(context, options, addrs,
					0, 0, keytab, 0, &my_creds, 0))){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"while getting initial credentials"
			" - %s",
			Is_server?"on server":"on client",
			error_message(retval) );
			goto done;
		}
		/* update the credentials */
		if( (retval = krb5_cc_initialize (context, ccdef, client)) ){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"when initializing cache"
			" - %s",
			Is_server?"on server":"on client",
			error_message(retval) );
			goto done;
		}
		if( (retval = krb5_cc_store_cred(context, ccdef, &my_creds))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"while storing credentials"
			" - %s",
			Is_server?"on server":"on client",
			error_message(retval) );
			goto done;
		}
	} else {
		/* we set RUID to user */
		if( Is_server ){
			To_ruid( DaemonUID );
		} else {
			To_ruid( OriginalRUID );
		}
		if((retval = krb5_cc_default(context, &ccdef))){
			SNPRINTF( err, errlen, "%s krb5_cc_default failed - %s",
				Is_server?"on server":"on client",
				error_message( retval ) );
			goto done;
		}
		if((retval = krb5_cc_get_principal(context, ccdef, &client))){
			SNPRINTF( err, errlen, "%s krb5_cc_get_principal failed - %s",
				Is_server?"on server":"on client",
				error_message( retval ) );
			goto done;
		}
		if( Is_server ){
			To_daemon();
		} else {
			To_user();
		}
		if(cname)free(cname); cname = 0;
		if((retval = krb5_unparse_name(context, client, &cname))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_unparse_name of 'me' failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			goto done;
		}
		DEBUG1( "client_krb5_auth: client '%s'", cname );
		my_creds.client = client;
	}

	cksum_data.data = host;
	cksum_data.length = strlen(host);



	if((retval = krb5_auth_con_init(context, &auth_context))){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"krb5_auth_con_init failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}

	if((retval = krb5_auth_con_genaddrs(context, auth_context, sock,
		KRB5_AUTH_CONTEXT_GENERATE_LOCAL_FULL_ADDR |
		KRB5_AUTH_CONTEXT_GENERATE_REMOTE_FULL_ADDR))){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"krb5_auth_con_genaddr failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}
  
	retval = krb5_sendauth(context, &auth_context, (krb5_pointer) &sock,
			   service, client, server,
			   AP_OPTS_MUTUAL_REQUIRED,
			   &cksum_data,
			   &my_creds,
			   ccdef, &err_ret, &rep_ret, &out_creds);

	if (retval){
		if( err_ret == 0 ){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_sendauth failed - %s",
				Is_server?"on server":"on client",
				error_message( retval ) );
		} else {
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_sendauth - mutual authentication failed - %*s",
				Is_server?"on server":"on client",
				err_ret->text.length, err_ret->text.data);
		}
		goto done;
	} else if (rep_ret == 0) {
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"krb5_sendauth - did not do mutual authentication",
			Is_server?"on server":"on client" );
		retval = 1;
		goto done;
	} else {
		DEBUG1("client_krb5_auth: sequence number %d", rep_ret->seq_number );
	}
    /* initialize the initial vector */
    if((retval = krb5_auth_con_initivector(context, auth_context))){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"krb5_auth_con_initvector failed: %s",
			Is_server?"on server":"on client",
			error_message(retval));
		goto done;
	}

	krb5_auth_con_setflags(context, auth_context,
		KRB5_AUTH_CONTEXT_DO_SEQUENCE);

	memset( &inbuf, 0, sizeof(inbuf) );
	memset( &outbuf, 0, sizeof(outbuf) );

	fd = Checkread( file, &statb );
	if( fd < 0 ){
		SNPRINTF( err, errlen,
			"%s client_krb5_auth: could not open for reading '%s' - '%s'",
			Is_server?"on server":"on client",
			file,
			Errormsg(errno) );
		retval = 1;
		goto done;
	}
	DEBUG1( "client_krb5_auth: opened for read %s, fd %d, size %0.0f", file, fd, (double)statb.st_size );
	while( (len = read( fd, buffer, sizeof(buffer)-1 )) > 0 ){
		/* status = Write_fd_len( sock, buffer, len ); */
		inbuf.data = buffer;
		inbuf.length = len;
		if((retval = krb5_mk_priv(context, auth_context, &inbuf,
			&outbuf, NULL))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_mk_priv failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			goto done;
		}
		if((retval = krb5_write_message(context, (void *)&sock, &outbuf))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_write_message failed: %s",
				Is_server?"on server":"on client",
				error_message(retval));
			goto done;
		}
		DEBUG4( "client_krb5_auth: freeing data");
		FREE_KRB_DATA(context,outbuf,.data);
	}
	if( len < 0 ){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"client_krb5_auth: file read failed '%s' - '%s'", file,
			Is_server?"on server":"on client",
			Errormsg(errno) );
		retval = 1;
		goto done;
	}
	close(fd);
	fd = -1;
	DEBUG1( "client_krb5_auth: file copy finished %s", file );
	if( shutdown(sock, 1) == -1 ){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"shutdown failed '%s'",
			Is_server?"on server":"on client",
			Errormsg(errno) );
		retval = 1;
		goto done;
	}
	fd = Checkwrite( file, &statb, O_WRONLY|O_TRUNC, 1, 0 );
	if( fd < 0 ){
		SNPRINTF( err, errlen,
			"%s client_krb5_auth: could not open for writing '%s' - '%s'",
			Is_server?"on server":"on client",
			file,
			Errormsg(errno) );
		retval = 1;
		goto done;
	}
	while((retval = krb5_read_message( context,&sock,&inbuf))==0){
		if((retval = krb5_rd_priv(context, auth_context, &inbuf,
			&outbuf, NULL))){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"krb5_rd_priv failed - %s",
				Is_server?"on server":"on client",
				Errormsg(errno) );
			retval = 1;
			goto done;
		}
		if(Write_fd_len(fd,outbuf.data,outbuf.length) < 0){
			SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
				"write to '%s' failed - %s",
				Is_server?"on server":"on client",
				file, Errormsg(errno) );
			retval = 1;
			goto done;
		}
		FREE_KRB_DATA(context,inbuf,.data);
		FREE_KRB_DATA(context,outbuf,.data);
	}
	close(fd); fd = -1;
	fd = Checkread( file, &statb );
	err[0] = 0;
	if( fd < 0 ){
		SNPRINTF( err, errlen,
			"%s client_krb5_auth: could not open for reading '%s' - '%s'",
				Is_server?"on server":"on client",
				file,
				Errormsg(errno) );
		retval = 1;
		goto done;
	}
	DEBUG1( "client_krb5_auth: reopened for read %s, fd %d, size %0.0f", file, fd, (double)statb.st_size );
	if( dup2(fd,sock) == -1){
		SNPRINTF( err, errlen, "%s client_krb5_auth failed - "
			"dup2(%d,%d) failed - '%s'",
			Is_server?"on server":"on client",
			fd, sock, Errormsg(errno) );
	}
	retval = 0;

 done:
	if( fd >= 0 && fd != sock ) close(fd);
	DEBUG4( "client_krb5_auth: freeing my_creds");
	krb5_free_cred_contents( context, &my_creds );
	DEBUG4( "client_krb5_auth: freeing rep_ret");
	if( rep_ret )	krb5_free_ap_rep_enc_part( context, rep_ret ); rep_ret = 0;
	DEBUG4( "client_krb5_auth: freeing err_ret");
	if( err_ret )	krb5_free_error( context, err_ret ); err_ret = 0;
	DEBUG4( "client_krb5_auth: freeing auth_context");
	if( auth_context) krb5_auth_con_free(context, auth_context );
	auth_context = 0;
	DEBUG4( "client_krb5_auth: freeing context");
	if( context )	krb5_free_context(context); context = 0;
	DEBUG1( "client_krb5_auth: retval %d, error '%s'",retval, err );
	return(retval);
}

/*
 * remote_principal_krb5(
 *  char * service		-service, usually "lpr"
 *  char * host			- server host name
 *  char *buffer, int bufferlen - buffer for credentials
 *  get the principal name of the remote service
 */ 
int remote_principal_krb5( char *service, char *host, char *err, int errlen )
{
	krb5_context context = 0;
	krb5_principal server = 0;
	int retval = 0;
	char *cname = 0;

	DEBUG1("remote_principal_krb5: service '%s', host '%s'",
		service, host );
	if ((retval = krb5_init_context(&context))){
		SNPRINTF( err, errlen, "%s '%s'",
			"krb5_init_context failed - '%s' ", error_message(retval) );
		goto done;
	}
	if((retval = krb5_sname_to_principal(context, host, service,
			 KRB5_NT_SRV_HST, &server))){
		SNPRINTF( err, errlen, "krb5_sname_to_principal %s/%s failed - %s",
			service, host, error_message(retval) );
		goto done;
	}
	if((retval = krb5_unparse_name(context, server, &cname))){
		SNPRINTF( err, errlen,
			"krb5_unparse_name failed - %s", error_message(retval));
		goto done;
	}
	strncpy( err, cname, errlen );
 done:
	if( cname )		free(cname); cname = 0;
	if( server )	krb5_free_principal(context, server); server = 0;
	if( context )	krb5_free_context(context); context = 0;
	DEBUG1( "remote_principal_krb5: retval %d, result: '%s'",retval, err );
	return(retval);
}

/*
 * Initialize a credentials cache.
 */

#define KRB5_DEFAULT_OPTIONS 0
#define KRB5_DEFAULT_LIFE 60*60*10 /* 10 hours */

#define VALIDATE 0
#define RENEW 1

/* stripped down version of krb5_mk_req */
 krb5_error_code krb5_tgt_gen( krb5_context context, krb5_ccache ccache,
	 krb5_principal server, krb5_data *outbuf, int opt )
{
	krb5_error_code	   retval;
	krb5_creds		  * credsp;
	krb5_creds			creds;

	/* obtain ticket & session key */
	memset((char *)&creds, 0, sizeof(creds));
	if ((retval = krb5_copy_principal(context, server, &creds.server)))
		goto cleanup;

	if ((retval = krb5_cc_get_principal(context, ccache, &creds.client)))
		goto cleanup_creds;

	if(opt == VALIDATE) {
			if ((retval = krb5_get_credentials_validate(context, 0,
					ccache, &creds, &credsp)))
				goto cleanup_creds;
	} else {
			if ((retval = krb5_get_credentials_renew(context, 0,
					ccache, &creds, &credsp)))
				goto cleanup_creds;
	}

	/* we don't actually need to do the mk_req, just get the creds. */
 cleanup_creds:
	krb5_free_cred_contents(context, &creds);

 cleanup:

	return retval;
}


 char *storage;
 int nstored = 0;
 char *store_ptr;
 krb5_data desinbuf,desoutbuf;

#define ENCBUFFERSIZE 2*LARGEBUFFER

 int des_read( krb5_context context,
	krb5_encrypt_block *eblock,
	int fd, char *buf, int len,
	char *err, int errlen )
{
	int nreturned = 0;
	long net_len,rd_len;
	int cc;
	unsigned char len_buf[4];
	
	if( len <= 0 ) return(len);

	if( desinbuf.data == 0 ){
		desinbuf.data = malloc_or_die(  ENCBUFFERSIZE, __FILE__,__LINE__ );
		storage = malloc_or_die( ENCBUFFERSIZE, __FILE__,__LINE__ );
	}
	if (nstored >= len) {
		memcpy(buf, store_ptr, len);
		store_ptr += len;
		nstored -= len;
		return(len);
	} else if (nstored) {
		memcpy(buf, store_ptr, nstored);
		nreturned += nstored;
		buf += nstored;
		len -= nstored;
		nstored = 0;
	}
	
	if ((cc = read(fd, len_buf, 4)) != 4) {
		/* XXX can't read enough, pipe must have closed */
		return(0);
	}
	rd_len =
		((len_buf[0]<<24) | (len_buf[1]<<16) | (len_buf[2]<<8) | len_buf[3]);
	net_len = krb5_encrypt_size(rd_len,eblock->crypto_entry);
	if ((net_len <= 0) || (net_len > ENCBUFFERSIZE )) {
		/* preposterous length; assume out-of-sync; only
		   recourse is to close connection, so return 0 */
		SNPRINTF( err, errlen, "des_read: "
			"read size problem");
		return(-1);
	}
	if ((cc = read( fd, desinbuf.data, net_len)) != net_len) {
		/* pipe must have closed, return 0 */
		SNPRINTF( err, errlen, "des_read: "
		"Read error: length received %d != expected %d.",
				cc, net_len);
		return(-1);
	}
	/* decrypt info */
	if((cc = krb5_decrypt(context, desinbuf.data, (krb5_pointer) storage,
						  net_len, eblock, 0))){
		SNPRINTF( err, errlen, "des_read: "
			"Cannot decrypt data from network - %s", error_message(cc) );
		return(-1);
	}
	store_ptr = storage;
	nstored = rd_len;
	if (nstored > len) {
		memcpy(buf, store_ptr, len);
		nreturned += len;
		store_ptr += len;
		nstored -= len;
	} else {
		memcpy(buf, store_ptr, nstored);
		nreturned += nstored;
		nstored = 0;
	}
	
	return(nreturned);
}


 int des_write( krb5_context context,
	krb5_encrypt_block *eblock,
	int fd, char *buf, int len,
	char *err, int errlen )
{
	char len_buf[4];
	int cc;

	if( len <= 0 ) return( len );
	if( desoutbuf.data == 0 ){
		desoutbuf.data = malloc_or_die( ENCBUFFERSIZE, __FILE__,__LINE__ );
	}
	desoutbuf.length = krb5_encrypt_size(len, eblock->crypto_entry);
	if (desoutbuf.length > ENCBUFFERSIZE ){
		SNPRINTF( err, errlen, "des_write: "
		"Write size problem - wanted %d", desoutbuf.length);
		return(-1);
	}
	if ((cc=krb5_encrypt(context, (krb5_pointer)buf,
					   desoutbuf.data,
					   len,
					   eblock,
					   0))){
		SNPRINTF( err, errlen, "des_write: "
		"Write encrypt problem. - %s", error_message(cc));
		return(-1);
	}
	
	len_buf[0] = (len & 0xff000000) >> 24;
	len_buf[1] = (len & 0xff0000) >> 16;
	len_buf[2] = (len & 0xff00) >> 8;
	len_buf[3] = (len & 0xff);
	if( Write_fd_len(fd, len_buf, 4) < 0 ){
		SNPRINTF( err, errlen, "des_write: "
		"Could not write len_buf - %s", Errormsg( errno ));
		return(-1);
	}
	if(Write_fd_len(fd, desoutbuf.data,desoutbuf.length) < 0 ){
		SNPRINTF( err, errlen, "des_write: "
		"Could not write data - %s", Errormsg(errno));
		return(-1);
	}
	else return(len); 
}
#endif
