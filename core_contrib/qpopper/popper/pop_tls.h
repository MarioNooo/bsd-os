/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * File:        pop_tls.h
 *
 * Revisions: 
 *
 * 11/17/00  [rcg]
 *          - Moved prototypes to popper.h
 *
 * 10/24/00  [rcg]
 *          - Added POP * to TLS functions.
 *          - Made library-specific functions.
 *
 * 09/21/99  [lgl]
 *          - File added.
 *
 */

#ifndef ___POP_TLS_INCLUDED
#define ___POP_TLS_INCLUDED


#include "config.h"

#include <stdio.h>

#include "popper.h"

#  ifdef    QPOP_SSLPLUS
#    include <ssl.h>
#    include <ezcrypto.h>
#    include <ezmod.h>

#    include "sslplus_utils.h"
#  endif /* QPOP_SSLPLUS */

#  ifdef    QPOP_OPENSSL
#    include <openssl/ssl.h>
#  endif /* QPOP_OPENSSL */






/*
 * Context for a TLS for the pop session.  Holds everything we need
 * to do TLS including the TLS library's context.  Also holds a pointer
 * to our POP structure, which makes it easier to do things such as
 * logging.
 */

struct _pop_tls {
    FILE       *m_pOutStream;
    int         m_fd;
    POP        *m_pPOP;

#  ifdef    QPOP_SSLPLUS  
    SSLContext *m_pTLSPlus;
    SessDBType *m_pSessDB;
    RandomObj  *m_pRandomObj;
#  endif /* QPOPSSLPLUS */

#  ifdef    QPOP_OPENSSL
    SSL_CTX    *m_OpenSSLctx;
    SSL        *m_OpenSSLconn;
    SSL_METHOD *m_OpenSSLmeth;
#  endif /* QPOP_OPENSSL */
};




/* 
 * Allocate and initalize what's needed for TLS
 * including TLS engine's context.
 * Args: in_fd - file descriptor TLS will read from
 *       pOutStream - FILE TLS will write to
 *
 * Returns allocated context
 */
/* pop_tls *pop_tls_init(POP *pPOP); */

/*
 * Start the TLS handshake. Returns 0 on succes
 */
/* int      pop_tls_handshake(pop_tls *); */


/*
 * The TLS I/O routines.
 */
/* int      pop_tls_write(pop_tls *, char *, int); */
/* void     pop_tls_flush(pop_tls *);              */
/* int      pop_tls_read(pop_tls *, char *, int);  */

/*
 * Closes TLS engine and free all context stuff
 */
/* void     pop_tls_shutdown(POP *pPOP); */


/* ---- library-specific prototypes ---- */
#  ifdef    QPOP_OPENSSL
int openssl_init(pop_tls *pTLS, POP *pPOP);
int openssl_handshake(pop_tls *pTLS);
int openssl_read(pop_tls *pTLS, char *pcBuffer, int nLength);
int openssl_write(pop_tls *pTLS, char *pcBuffer, int nLength);
int openssl_shutdown(pop_tls *pTLS);
#  endif /* QPOP_OPENSSL */

#  ifdef    QPOP_SSLPLUS
int sslplus_init(pop_tls *pTLS, POP *pPOP);
int sslplus_handshake(pop_tls *pTLS);
int sslplus_read(pop_tls *pTLS, char *pcBuffer, int nLength);
int sslplus_write(pop_tls *pTLS, char *pcBuffer, int nLength);
#  endif /* QPOP_SSLPLUS */


#endif /* ___POP_TLS_INCLUDED */
