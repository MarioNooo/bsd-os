/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 02/28/01  [rcg]
 *         - Fixed typos when SSL Plus used.
 *
 * 10/29/00  [rcg]
 *         - Added POP * to TLS structure.
 *         - Moved functions specific to an SSL library to their
 *           own file, to cut down on #idef clutter.
 *
 * 01/26/00  [lgl]
 *         - File added.
 *
 */




#include "config.h"

#include "pop_tls.h"
#include "popper.h"

#ifndef   QPOP_SSL

/* ======================================================================
    These are stubs that do nothing if we have no SSL compiled in. In 
    most cases they'll never be called and when they are, doing nothing
    is OK.
   ====================================================================== */
pop_tls *pop_tls_init(POP *pPOP)
{
    (void) pPOP;
    return (pop_tls *) 0;
}

int pop_tls_handshake(pop_tls *pTLS)
{
    (void) pTLS;
    return 0;
}

int pop_tls_write(pop_tls *pTLS, char *pcBuffer, int nLength)
{
    (void) pTLS;
    (void) pcBuffer;
    (void) nLength;
    return -1;
}

int pop_tls_read(pop_tls *pTLS, char *pcBuffer, int nLength)
{
    (void) pTLS;
    (void) pcBuffer;
    (void) nLength;
    return -1;
}

void pop_tls_shutdown(POP *pPOP){(void) pPOP;}

int  pop_tls_flush(pop_tls *pTLS){(void) pTLS; return 0;}


#else /* QPOP_SSL is defined */


/* ======================================================================
     The real SSL support code is in a file specific to the SSL support
     library (such as OpenSSL or SSL Plus).
   ====================================================================== */


#  include <stdio.h>

#  ifdef    QPOP_SSLPLUS
#    include <ssl.h>
#    include <ezcrypto.h>
#    include <ezmod.h>
#  endif /* QPOP_SSLPLUS */

#  ifdef    QPOP_OPENSSL
#    include <openssl/ssl.h>
#  endif /* QPOP_OPENSSL */



/* #define TEST_CLIENT_FAILURE */




/* 
 * Public entry point to initialize all TLS related stuff 
 *
 */

pop_tls *
pop_tls_init ( POP *pPOP )
{
    pop_tls *pPSSL;

    pPSSL = (pop_tls *) malloc ( sizeof(pop_tls) );
    if ( pPSSL == NULL )
        return ( NULL );


#  ifdef    QPOP_SSLPLUS
    if ( sslplus_init ( pPSSL, pPOP ) != 0 ) {
        free ( pPSSL );
        pPSSL = NULL;
        goto Done;
    }
#  endif /* QPOP_SSLPLUS */

#  ifdef    QPOP_OPENSSL
    if ( openssl_init ( pPSSL, pPOP ) != 0 ) {
        free ( pPSSL );
        pPSSL = NULL;
        goto Done;
    }
#  endif /* QPOP_OPENSSL */

  Done:
    return ( pPSSL );
}




/* 
 *  Public entry point to kick off TLS handshake
 *
 */

int
pop_tls_handshake ( pop_tls *pTLS )
{
#ifdef    QPOP_SSLPLUS
    return sslplus_handshake ( pTLS );
#endif /* QPOP_SSLPLUS */

#ifdef    QPOP_OPENSSL
    return openssl_handshake ( pTLS );
#endif /* QPOP_OPENSSL */
}




/* 
 * Public entry point to read from TLS connection.  Assumes 
 * everything is nicely initialized.
 *
 * Args: pTLS:      our POP TLS context
 *       pcBuffer:  buffer to read into
 *       nLength:   size of buffer
 *
 * Returns: number of bytes read or -1
 */

int
pop_tls_read ( pop_tls *pTLS, char *pcBuffer, int nLength )
{
#ifdef    QPOP_SSLPLUS
    return sslplus_read ( pTLS, pcBuffer, nLength );
#endif /* QPOP_SSLPLUS */

#ifdef    QPOP_OPENSSL
    return openssl_read ( pTLS, pcBuffer, nLength );
#endif /* QPOP_OPENSSL */
}




/* 
 *  Public entry point to write data down TLS connection (assumed open)
 *
 *  Args: pTLS:     our context
 *        pcBuffer: data to write
 *        nLength:  number of bytes to write
 *
 *  Returns:        -1 on error or number of bytes written
 *
 *  This writes all bytes or fails as it uses non-blocking I/O
 */

int
pop_tls_write ( pop_tls *pTLS, char *pcBuffer, int nLength )
{
#ifdef    QPOP_SSLPLUS
    return sslplus_write ( pTLS, pcBuffer, nLength );
#endif /* QPOP_SSLPLUS */

#ifdef    QPOP_OPENSSL
    return openssl_write ( pTLS, pcBuffer, nLength );
#endif /* QPOP_OPENSSL */
}




/* 
 * Public entry point to flush a TLS connection (ensure all data is
 * written out).
 */
int
pop_tls_flush ( pop_tls *pTLS )
{
#ifdef    QPOP_SSLPLUS
    (void) pTLS;
    return 0;
#endif /* QPOP_SSLPLUS */

#ifdef    QPOP_OPENSSL
    (void) pTLS;
    return 0;
#endif /* QPOP_OPENSSL */
}




/* 
 * Public entry point -- Close down the TLS connection.  Don't close the
 * I/O streams before calling this.
 */
void
pop_tls_shutdown ( POP *pPOP )
{
#ifdef    QPOP_SSLPLUS
    SSLClose ( pPOP->tls_context->m_pTLSPlus );
#endif /* QPOP_SSLPLUS */

#ifdef    QPOP_OPENSSL
    openssl_shutdown ( pPOP->tls_context );
#endif /* QPOP_OPENSSL */

    free ( pPOP->tls_context );
    pPOP->tls_context = NULL;
}


#endif /* !defined(SSLPLUS) and !defined(OPENSSL) */








