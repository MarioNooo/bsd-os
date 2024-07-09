/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 08/06/02  [rcg]
 *         - Added ability to set options (SSL_OP_* values).
 *         - Added debug trace call to show OpenSSL library version.
 *
 * 05/13/01  [rcg]
 *         - Don't call SSL_shutdown unless we tried to negotiate an
 *           SSL session.  (As suggested by Kenneth Porter.)
 *
 * 03/28/01  [rcg]
 *         - Don't log "OpenSSL Error during shutdown" unless compiled
 *           with debug or run with debug or trace options.

 * 02/14/01  [rcg]
 *         - Slight improvement when /dev/urandom not installed.
 *
 * 11/27/00  [rcg]
 *         - If private key file not specified, assume private key is
 *           in certificate file.
 *
 * 11/14/00  [rcg]
 *         - Added support for new TLS options.
 *
 * 10/28/00  [rcg]
 *         - File added.
 *
 */




#include "config.h"

#include "pop_tls.h"
#include "popper.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */


#ifdef    QPOP_OPENSSL
#    include <openssl/ssl.h>
#    include <openssl/opensslv.h>
#    include <openssl/err.h>
#    include <openssl/rand.h>
#    include <openssl/crypto.h>
#    include <openssl/x509.h>
#    include <openssl/pem.h>
#    include <openssl/rsa.h>
#    include <openssl/md5.h>







/* ---- Configuration options ------ */


/* #define TEST_CLIENT_FAILURE */




/* ---- Globals ---- */
extern volatile BOOL poptimeout;
extern int           pop_timeout;
jmp_buf              env;




/* ---- Private Prototypes ---- */
void log_openssl_err(POP *pPOP, WHENCE, const char *format, ...);
const char *openssl_get_error_code(int nErr);
const char *get_cipher_description(SSL_CIPHER *ciph, char *buf, int len);
static int  tls_ring ( SIGPARAM );





/*
 * Log OpenSSL errors
 */
void
log_openssl_err ( POP *pPOP, WHENCE, const char *format, ... )
{
    unsigned long sErr = ERR_get_error();
    va_list       ap;


    va_start ( ap, format );
    pop_logv ( pPOP, POP_PRIORITY, fn, ln, format, ap ); /* fn & ln are in WHENCE */
    va_end ( ap );

    while ( sErr != 0 ) {
        pop_log ( pPOP, POP_PRIORITY, fn, ln,
                  "...SSL error: %s",
                  ERR_error_string ( sErr, NULL ) );
        sErr = ERR_get_error();
    }
}



static const char *openssl_get_error_names[]={
    "(unknown)",                        /*  0 */
    "SSL_ERROR_NONE",                   /*  1 */
    "SSL_ERROR_ZERO_RETURN",            /*  2 */
    "SSL_ERROR_WANT_READ",              /*  3 */
    "SSL_ERROR_WANT_WRITE",             /*  4 */
    "SSL_ERROR_WANT_X509_LOOKUP",       /*  5 */
    "SSL_ERROR_SYSCALL",                /*  6 */
    "SSL_ERROR_SSL" };                  /*  7 */
/*
 * Return the mnemonic for an SSL_get_error() code.
 */
const char *
openssl_get_error_code ( int nErr )
{
    switch ( nErr ) {
        case SSL_ERROR_NONE:
            return openssl_get_error_names [  1 ];

        case SSL_ERROR_ZERO_RETURN: 
            return openssl_get_error_names [  2 ];

        case SSL_ERROR_WANT_READ:
            return openssl_get_error_names [  3 ];

        case SSL_ERROR_WANT_WRITE:
            return openssl_get_error_names [  4 ];

        case SSL_ERROR_WANT_X509_LOOKUP:
            return openssl_get_error_names [  5 ];

        case SSL_ERROR_SYSCALL:
            return openssl_get_error_names [  6 ];

        case SSL_ERROR_SSL:
            return openssl_get_error_names [  7 ];
        
        default:
            return openssl_get_error_names [  0 ];
    }
}



/*
 * Return a cipher description, with multiple blanks compressed into one.
 */
const char *
get_cipher_description ( SSL_CIPHER *ciph, char *buf, int len )
{
    char *deblanked = malloc ( len );
    char *p         = NULL;
    char *q         = NULL;
    int   n         = len;
    BOOL  blank     = FALSE;


    if ( deblanked == NULL || buf == NULL )
        return buf;

    p = SSL_CIPHER_description ( ciph, buf, len );
    q = deblanked;

    while ( n > 0 && *p != '\0' ) {
        if ( ( blank == FALSE || *p != ' ' ) && *p != '\n' )
            *q++ = *p;
        blank = ( *p == ' ' );
        p++;
        n--;
    }

    *q = '\0';
    strcpy ( buf, deblanked );
    free   ( deblanked      );
    return buf;
}




/* 
 *  Initialize OpenSSL context.
 *  Args: pTLS: the POP TLS context (where SSLPLus context goes)
 *        pPOP: the POP structure.  We use:
 *              input_fd: file descriptor to read input on
 *              output: FILE * to write output to
 *  Returns: -1 on error, 0 on success.
 */

int
openssl_init ( pop_tls *pTLS, POP *pPOP )
{
    int    nErr      =  0;
    int    nResult   = -1; /* Assume the worst */


    pTLS->m_fd         = pPOP->input_fd;
    pTLS->m_pOutStream = pPOP->output;
    pTLS->m_pPOP       = pPOP;

    /*
     * First, initialize the SSL library
     */
    DEBUG_LOG1 ( pPOP, "...Initializing OpenSSL library (version %s)",
                 OPENSSL_VERSION_TEXT );
    SSL_load_error_strings();
    SSL_library_init(); /* always returns 1, no need to check */

    /*
     * We must seed the pseudo random number generator (PRNG).
     * If /dev/urandom is available OpenSSL uses it.  Otherwise
     * we must provide some entropy.
     */
#  ifndef   HAVE_DEV_URANDOM /* /Bug: this is really lame.  Get /dev/urandom. */
    DEBUG_LOG0 ( pPOP, ".../dev/urandom not available; seeding PRNG the hard way" );
    { char          memory_cruft [ 4098 ];
      static char   static_cruft [ 4098 ];
      int           rand_cruft   [   10 ];
      unsigned char buffer       [ 4098 + 16 ];
      int           junkfd = -1;
      int           iosz   =  0;
      int           inx    =  0;
      MD5_CTX       mdContext;

      srand ( time ( 0 ) );
      for ( inx = sizeof(rand_cruft) -1; inx >= 0; inx-- )
        rand_cruft [ inx ] = rand();

      MD5_Init   ( &mdContext );
      MD5_Update ( &mdContext, memory_cruft, sizeof(memory_cruft) );
      MD5_Update ( &mdContext, static_cruft, sizeof(static_cruft) );
      MD5_Update ( &mdContext, rand_cruft,   sizeof(rand_cruft)   );

      junkfd = open ( "/tmp/qpopper.junk.junk", O_RDWR | O_CREAT );
      if ( junkfd != -1 ) {
          iosz = ftruncate ( junkfd, 4098 );
          if ( iosz != -1 ) {
            iosz = read ( junkfd, buffer + 16, sizeof(buffer) - 16 );
            if ( iosz > 0 ) {
                MD5_Update ( &mdContext, buffer + 16, iosz );
            }
            else
                pop_log ( pPOP, POP_PRIORITY, HERE,
                          "Unable to read from temp file: %s (%d)",
                          STRERROR(errno), errno );
          }
          else
              pop_log ( pPOP, POP_PRIORITY, HERE,
                        "Unable to ftruncate() temp file: %s (%d)",
                        STRERROR(errno), errno );
          close ( junkfd );
          unlink ( "/tmp/qpopper.junk" );
      }
      else
          pop_log ( pPOP, POP_PRIORITY, HERE,
                    "Unable to open temp file: %s (%d)",
                    STRERROR(errno), errno );

      MD5_Final  ( buffer, &mdContext );
      RAND_seed  ( buffer, sizeof(buffer) );
    }
#  else
    DEBUG_LOG0 ( pPOP, "...have /dev/urandom; skipping PRNG seeding" );
#  endif /* HAVE_DEV_URANDOM */


    /* OpenSSL_add_all_digests(); */ /* adds all digest algorithms to the table. */

    /* OpenSSL_add_all_algorithms(); */  /* adds all algorithms to the table (digests and ciphers). */ 

    /* OpenSSL_add_all_ciphers(); */

    /*
     * Select the connection method.  We use SSLv23_server_method by default.
     * We use other methods if so directed.
     *
     * A TLS/SSL connection established with this method understands the
     * SSLv2, SSLv3, and TLSv1 protocols.  A client sends out an SSLv2
     * client hello messages and indicates that it also understands SSLv3
     * and TLSv1.  A server understands SSLv2, SSLv3, and TLSv1 client
     * hello messages.  This is the best choice when compatibility is a
     * concern. 
     */
    switch ( pPOP->tls_version ) {
        case QPOP_TLSvDEFAULT:  /* unspecified */
        case QPOP_SSLv23:
            DEBUG_LOG0 ( pPOP, "...setting method to SSLv23_server_method" );
            pTLS->m_OpenSSLmeth = SSLv23_server_method();
            break;

        case QPOP_SSLv2:       /* SSL version 2 only */
            DEBUG_LOG0 ( pPOP, "...setting method to SSLv2_server_method" );
            pTLS->m_OpenSSLmeth = SSLv2_server_method();
            break;

        case QPOP_SSLv3:       /* SSL version 3 only */
            DEBUG_LOG0 ( pPOP, "...setting method to SSLv3_server_method" );
            pTLS->m_OpenSSLmeth = SSLv3_server_method();
            break;

        case QPOP_TLSv1:       /* TLS version 1 only */
            DEBUG_LOG0 ( pPOP, "...setting method to TLSv1_server_method" );
            pTLS->m_OpenSSLmeth = TLSv1_server_method();
            break;
    }

    if ( pTLS->m_OpenSSLmeth == NULL ) {
        log_openssl_err ( pPOP, HERE, "Unable to allocate method" );
        goto Done;
    }

    /*
     * Allocate the OpenSSL context (in stand-alone mode
     * this could be done before a client connection
     * arrives).
     */
    DEBUG_LOG0 ( pPOP, "...allocating OpenSSL context" );
    pTLS->m_OpenSSLctx = SSL_CTX_new ( pTLS->m_OpenSSLmeth );
    if ( pTLS->m_OpenSSLctx == NULL ) {
        log_openssl_err ( pPOP, HERE, "Unable to allocate SSL_CTX" );
        goto Done;
    }

    /*
     * Set desired options
     */
    if ( pPOP->tls_options ) {
        long opts = 0;
        opts = SSL_CTX_set_options ( pTLS->m_OpenSSLctx, pPOP->tls_options );
        DEBUG_LOG2 ( pPOP, "...set options %#0x; options now %#0lx",
                     pPOP->tls_options, opts );
    }

    /*
     * Establish the certificate for our server cert.
     */
    DEBUG_LOG1 ( pPOP, "...setting certificate file %s",
                 pPOP->tls_server_cert_file );
    nErr = SSL_CTX_use_certificate_file ( pTLS->m_OpenSSLctx,
                                          pPOP->tls_server_cert_file,
                                          SSL_FILETYPE_PEM );
    if ( nErr <= 0 ) {
        log_openssl_err ( pPOP, HERE, "Error setting certificate PEM file %s",
                          pPOP->tls_server_cert_file );
        goto Done;
    }

    /*
     * Establish our private key
     */
    if ( pPOP->tls_private_key_file == NULL ) {
        pPOP->tls_private_key_file = pPOP->tls_server_cert_file;
        DEBUG_LOG1 ( pPOP, "...private key file not set; assuming "
                           "private key is in cert (%s)",
                     pPOP->tls_server_cert_file );
    }

    DEBUG_LOG1 ( pPOP, "...setting private key file %s",
                 pPOP->tls_private_key_file );
    nErr = SSL_CTX_use_PrivateKey_file ( pTLS->m_OpenSSLctx,
                                         pPOP->tls_private_key_file,
                                         SSL_FILETYPE_PEM );
    if ( nErr <= 0 ) {
        log_openssl_err ( pPOP, HERE, "Error setting private key PEM file %s",
                          pPOP->tls_private_key_file );
        goto Done;
    }

    /*
     * Verify that the private key matches the public key in the certificate
     */
    DEBUG_LOG0 ( pPOP, "...verifying private key against certificate" );
    nErr = SSL_CTX_check_private_key ( pTLS->m_OpenSSLctx );
    if ( nErr <= 0 ) {
        log_openssl_err ( pPOP, HERE,
                         "Error verifying private key (%s) against "
                         "certificate (%s)",
                         pPOP->tls_private_key_file,
                         pPOP->tls_server_cert_file );
        goto Done;
    }

    /*
     * If desired, specify permitted ciphers
     */
    if ( pPOP->tls_cipher_list != NULL ) {
        DEBUG_LOG1 ( pPOP, "...setting cipher list to %s",
                     pPOP->tls_cipher_list );
        nErr = SSL_CTX_set_cipher_list ( pTLS->m_OpenSSLctx,
                                         pPOP->tls_cipher_list );
        if ( nErr <= 0 ) {
            log_openssl_err ( pPOP, HERE,
                              "Error setting cipher list to %s",
                              pPOP->tls_cipher_list );
            goto Done;
        }
    }
    else
        DEBUG_LOG0 ( pPOP, "...(tls_cipher_list not specified)" );

    /*
     * Setup options, verification settings, timeout settings. 
     */



    /*
     * Allocate an SSL connection object
     */
    DEBUG_LOG0 ( pPOP, "...allocating OpenSSL connection" );
    pTLS->m_OpenSSLconn = SSL_new ( pTLS->m_OpenSSLctx );
    if ( pTLS->m_OpenSSLconn == NULL ) {
        log_openssl_err ( pPOP, HERE, "Unable to allocate SSL" );
        goto Done;
    }

    /*
     * Set the input and output file descriptors
     */
    DEBUG_LOG2 ( pPOP, "...setting input (%d) and output (%d) file descriptors",
                 pPOP->input_fd, fileno(pPOP->output) );
    nErr = SSL_set_rfd ( pTLS->m_OpenSSLconn, pPOP->input_fd );
    if ( nErr <= 0 ) {
        log_openssl_err ( pPOP, HERE, "Unable to set read fd (%d)",
                          pPOP->input_fd );
        goto Done;
    }

    nErr = SSL_set_wfd ( pTLS->m_OpenSSLconn, fileno(pPOP->output) );
    if ( nErr <= 0 ) {
        log_openssl_err ( pPOP, HERE, "Unable to set write fd (%d)",
                          fileno(pPOP->output) );
        goto Done;
    }

    /*
     * Made it through it all
     */
    nResult = 0;
    DEBUG_LOG0 ( pPOP, "...successfully completed OpenSSL initialization" );

  Done:
    return ( nResult );
}




static int 
tls_ring ( SIGPARAM )
{
    poptimeout = TRUE;
    longjmp ( env, TRUE );
    return POP_FAILURE;
}




/*
 * Wait for client to initiate SSL/TLS handshake.
 *
 * Returns 0 if all went well, -1 on error.
 */
int
openssl_handshake ( pop_tls *pTLS )
{
    int         ret       = 0;
    int         nErr      = 0;
    char        buf [512] = "";
    int         al_bits   =  0;
    SSL_CIPHER *ciph      =  0;
    const char *ciph_name = NULL;


    signal ( SIGALRM, VOIDSTAR tls_ring );
    alarm  ( pop_timeout );
    if ( setjmp ( env ) ) {
        pop_log ( pTLS->m_pPOP, POP_NOTICE, HERE,
                  "(v%s) Timeout (%d secs) during SSL/TLS handshake with "
                  "client at %s (%s)",
                  VERSION, pop_timeout, pTLS->m_pPOP->client,
                  pTLS->m_pPOP->ipaddr );
        alarm  ( 0 );
        signal ( SIGALRM, SIG_DFL );
        return -1;
    }

    DEBUG_LOG0 ( pTLS->m_pPOP, "Attempting OpenSSL handshake" );

    ret = SSL_accept ( pTLS->m_OpenSSLconn );
    DEBUG_LOG1 ( pTLS->m_pPOP, "tls accept returned %d", ret );

    alarm  ( 0 );
    signal ( SIGALRM, SIG_DFL );

    nErr = SSL_get_error ( pTLS->m_OpenSSLconn, ret );
    DEBUG_LOG2 ( pTLS->m_pPOP, "SSL_get_error says %s (%d)",
                 openssl_get_error_code(nErr), nErr );
    switch ( nErr ) {
        case SSL_ERROR_NONE: {
            ciph = SSL_get_current_cipher ( pTLS->m_OpenSSLconn );
            if ( ciph != NULL ) {
                ciph_name = SSL_CIPHER_get_name ( ciph );
                pop_log ( pTLS->m_pPOP, POP_NOTICE, HERE,
                          "(v%s) %s handshake with client at %s (%s); "
                          "%s session-id; cipher: %s (%s), %d bits",
                          VERSION, SSL_CIPHER_get_version(ciph),
                          pTLS->m_pPOP->client, pTLS->m_pPOP->ipaddr,
                          ( pTLS->m_OpenSSLconn->hit ? "reused" : "new" ),
                          ( ciph_name != NULL ? ciph_name : "(none)" ),
                          get_cipher_description ( ciph, buf, sizeof(buf) ),
                          SSL_CIPHER_get_bits    ( ciph, &al_bits ) );
            }
            alarm  ( 0 );
            signal ( SIGALRM, SIG_DFL );
            return 0;
            break;
        }

        case SSL_ERROR_ZERO_RETURN: 
            break;

        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;

        case SSL_ERROR_WANT_X509_LOOKUP:
            break;

        case SSL_ERROR_SYSCALL:
            log_openssl_err ( pTLS->m_pPOP, HERE, "TLS handshake Error" );
            break;

        case SSL_ERROR_SSL:
            log_openssl_err ( pTLS->m_pPOP, HERE,
                              "OpenSSL error during handshake" );
            break;
        
        default:
            break;
    }

    alarm  ( 0 );
    signal ( SIGALRM, SIG_DFL );
    return -1;
}





/* 
 * Read from an OpenSSL TLS connection.  Assumes 
 * everything is nicely initialized.
 *
 * Args: pTLS:      our POP TLS context
 *       pcBuffer:  buffer to read into
 *       nLength:   size of buffer
 *
 * Returns: number of bytes read or -1
 */

int
openssl_read ( pop_tls *pTLS, char *pcBuffer, int nLength )
{
    int ret  = 0;
    int nErr = 0;

    DEBUG_LOG2 ( pTLS->m_pPOP, "tls read start %d %p", nLength, pcBuffer );

    ret  = SSL_read ( pTLS->m_OpenSSLconn, pcBuffer, nLength );
    DEBUG_LOG3 ( pTLS->m_pPOP, "tls read %d %x %x",
                 ret, pcBuffer[0], pcBuffer[1] );

    nErr = SSL_get_error ( pTLS->m_OpenSSLconn, ret );
    DEBUG_LOG2 ( pTLS->m_pPOP, "SSL_get_error says %s (%d)",
                 openssl_get_error_code(nErr), nErr );
    switch ( nErr ) {
        /* The TLS/SSL I/O operation completed.  This result code is 
           returned if and only if ret > 0. */
        case SSL_ERROR_NONE:
            return ret;
            break;

        /* The TLS/SSL connection has been closed.  If the protocol
           version is SSL 3.0 or TLS 1.0, this result code is returned
           only if a closure alert has occurred in the protocol, i.e.,
           if the connection has been closed cleanly.  Note that in
           this case SSL_ERROR_ZERO_RETURN does not necessarily indicate
           that the underlying transport has been closed. */
        case SSL_ERROR_ZERO_RETURN: 
            break;

        /* The operation did not complete; the same TLS/SSL I/O function
           should be called again later.  There will be protocol progress
           if, by then, the underlying BIO has data available for reading
           (if the result code is SSL_ERROR_WANT_READ) or allows writing
           data (SSL_ERROR_WANT_WRITE).  For socket BIOs (e.g., when
           SSL_set_fd() was used) this means that select() or poll() on
           the underlying socket can be used to find out when the TLS/SSL
           I/O function should be retried.
           
           Caveat: Any TLS/SSL I/O function can lead to either of
           SSL_ERROR_WANT_READ and SSL_ERROR_WANT_WRITE, i.e., SSL_read()
           may want to write data and SSL_write() may want to read data. */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;

        /* The operation did not complete because an application callback
           set by SSL_CTX_set_client_cert_cb() has asked to be called again.
           The TLS/SSL I/O function should be called again later.  Details
           depend on the application. */
        case SSL_ERROR_WANT_X509_LOOKUP:
            break;

        /* Some I/O error occurred.  The OpenSSL error queue may contain
           more information on the error.  If the error queue is empty
           (i.e., ERR_get_error() returns 0), ret can be used to find out
           more about the error: If ret == 0, an EOF was observed that
           violates the protocol.  If ret == -1, the underlying BIO
           reported an I/O error (for socket I/O on Unix systems, consult
           errno for details). */
        case SSL_ERROR_SYSCALL:
            log_openssl_err ( pTLS->m_pPOP, HERE, "I/O Error" );
            break;

        /* A failure in the SSL library occurred, usually a protocol error.
          The OpenSSL error queue contains more information on the error. */
        case SSL_ERROR_SSL:
            log_openssl_err ( pTLS->m_pPOP, HERE, "OpenSSL Error during read" );
            break;
        
        default:
            break;
    }
    
    return -1;
}




/* 
 *  Write data down an OpenSSL TLS connection (assumed open)
 *
 *  Args: pTLS:     our context
 *        pcBuffer: data to write
 *        nLength:  number of bytes to write
 *
 *  Returns:        -1 on error or number of bytes written
 *
 *  This uses blocking I/O (determined by the underlying file
 *  descriptor).
 */

int
openssl_write ( pop_tls *pTLS, char *pcBuffer, int nLength )
{
    int ret  = 0;
    int nErr = 0;


    DEBUG_LOG2 ( pTLS->m_pPOP, "tls write start %d %p", nLength, pcBuffer );
    ret = SSL_write ( pTLS->m_OpenSSLconn, pcBuffer, nLength );

    DEBUG_LOG3 ( pTLS->m_pPOP, "tls write %d %x %x",
                 ret, pcBuffer[0], pcBuffer[1] );

    nErr = SSL_get_error ( pTLS->m_OpenSSLconn, ret );
    DEBUG_LOG2 ( pTLS->m_pPOP, "SSL_get_error says %s (%d)",
                 openssl_get_error_code(nErr), nErr );
    switch ( nErr ) {
        /* The TLS/SSL I/O operation completed.  This result code is 
           returned if and only if ret > 0. */
        case SSL_ERROR_NONE:
            return ret;
            break;

        /* The TLS/SSL connection has been closed.  If the protocol
           version is SSL 3.0 or TLS 1.0, this result code is returned
           only if a closure alert has occurred in the protocol, i.e.,
           if the connection has been closed cleanly.  Note that in
           this case SSL_ERROR_ZERO_RETURN does not necessarily indicate
           that the underlying transport has been closed. */
        case SSL_ERROR_ZERO_RETURN: 
            break;

        /* The operation did not complete; the same TLS/SSL I/O function
           should be called again later.  There will be protocol progress
           if, by then, the underlying BIO has data available for reading
           (if the result code is SSL_ERROR_WANT_READ) or allows writing
           data (SSL_ERROR_WANT_WRITE).  For socket BIOs (e.g., when
           SSL_set_fd() was used) this means that select() or poll() on
           the underlying socket can be used to find out when the TLS/SSL
           I/O function should be retried.
           
           Caveat: Any TLS/SSL I/O function can lead to either of
           SSL_ERROR_WANT_READ and SSL_ERROR_WANT_WRITE, i.e., SSL_read()
           may want to write data and SSL_write() may want to read data. */
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break;

        /* The operation did not complete because an application callback
           set by SSL_CTX_set_client_cert_cb() has asked to be called again.
           The TLS/SSL I/O function should be called again later.  Details
           depend on the application. */
        case SSL_ERROR_WANT_X509_LOOKUP:
            break;

        /* Some I/O error occurred.  The OpenSSL error queue may contain
           more information on the error.  If the error queue is empty
           (i.e., ERR_get_error() returns 0), ret can be used to find out
           more about the error: If ret == 0, an EOF was observed that
           violates the protocol.  If ret == -1, the underlying BIO
           reported an I/O error (for socket I/O on Unix systems, consult
           errno for details). */
        case SSL_ERROR_SYSCALL:
            log_openssl_err ( pTLS->m_pPOP, HERE, "I/O Error" );
            break;

        /* A failure in the SSL library occurred, usually a protocol error.
          The OpenSSL error queue contains more information on the error. */
        case SSL_ERROR_SSL:
            log_openssl_err ( pTLS->m_pPOP, HERE, "OpenSSL Error during write" );
            break;
        
        default:
            break;
    }
    
    return -1;
}



/*
 * Shutdown an OpenSSL session.
 *
 * Returns 0 on success, -1 on error.
 */
int
openssl_shutdown ( pop_tls *pTLS )
{
    int ret  = 0;
    int rslt = 0;
    int nErr = 0;


    if ( pTLS->m_pPOP->tls_started == TRUE ) {
        ret = SSL_shutdown ( pTLS->m_OpenSSLconn );
        DEBUG_LOG1 ( pTLS->m_pPOP, "tls shutdown returned %d", ret );
    
        nErr = SSL_get_error ( pTLS->m_OpenSSLconn, ret );
        DEBUG_LOG2 ( pTLS->m_pPOP, "SSL_get_error says %s (%d)",
                     openssl_get_error_code(nErr), nErr );
        switch ( nErr ) {
            case SSL_ERROR_NONE:
                rslt = 0;
                break;
    
            case SSL_ERROR_ZERO_RETURN:
                rslt = -1;
                break;
    
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                rslt = -1;
                break;
    
            case SSL_ERROR_WANT_X509_LOOKUP:
                rslt = -1;
                break;
    
            case SSL_ERROR_SYSCALL:
                rslt = -1;
                if ( DEBUGGING || pTLS->m_pPOP->debug )
                    log_openssl_err ( pTLS->m_pPOP, HERE, "TLS shutdown Error" );
                break;
    
            case SSL_ERROR_SSL:
                rslt = -1;
                if ( DEBUGGING || pTLS->m_pPOP->debug )
                    log_openssl_err ( pTLS->m_pPOP, HERE,
                                      "OpenSSL Error during shutdown" );
                break;
            
            default:
                rslt = -1;
                log_openssl_err ( pTLS->m_pPOP, HERE,
                                  "Unknown error during shutdown" );
                break;
        } /* switch ( nErr ) */
    } /* pTLS->m_pPOP->tls_started == TRUE */
    else {
        DEBUG_LOG0 ( pTLS->m_pPOP, "pTLS->m_pPOP->tls_started == false" );
    }

    if ( pTLS->m_OpenSSLconn != NULL ) {
        DEBUG_LOG0 ( pTLS->m_pPOP, "freeing m_OpenSSLconn" );
        SSL_free ( pTLS->m_OpenSSLconn );
        pTLS->m_OpenSSLconn = NULL;
    }

    if ( pTLS->m_OpenSSLctx != NULL ) {
        DEBUG_LOG0 ( pTLS->m_pPOP, "freeing m_OpenSSLctx" );
        SSL_CTX_free ( pTLS->m_OpenSSLctx );
        pTLS->m_OpenSSLctx = NULL;
    }

    DEBUG_LOG1 ( pTLS->m_pPOP, "openssl_shutdown returning %d", rslt );
    return rslt;
}






#endif /* QPOP_OPENSSL */








