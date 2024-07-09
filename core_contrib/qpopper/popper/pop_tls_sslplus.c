/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 10/28/00  [rcg]
 *         - File split from pop_tls.c.
 *
 */




#include "config.h"

#include "pop_tls.h"
#include "popper.h"

#ifdef    QPOP_SSLPLUS
#  include <stdio.h>

#  include <ssl.h>
#  include <ezcrypto.h>
#  include <ezmod.h>


/* ---- Configuration options ------ */
/* Default is not to use BSAFE from RSA; haven't tested with it */
#  undef SSLPLUS_WITH_BSAFE

/* Default not to  use CRYPTOSWIFT accelerator hardware; not sure if
   it's available for UNIX and it hasn't been tested */
#  undef SSLPLUS_WITH_CRYPTOSWIFT

/* This may need to be a command line option, not a compile time option */
#  define SSL_VERSION SSL_Version_3_0_With_2_0_Hello

/* Size of records not fragmented when send to SSLWrite.  The max is 16K, but
   we make this smaller in considerations of PalmOS-based implementations;
   this probably needs a little more thought */
#  define SSL_MAXRECORD (2048)

/* /Bug: need to figure out a good place for this; Maybe UNIX varient
   dependent.  Also may need locking for it */
#  define RANDOM_POOL_FILENAME "/tmp/random.bin"


/* #define TEST_CLIENT_FAILURE */




/* /Bug: use pop_log / DEBUG_LOGx instead of writing to g_pTrace */

extern FILE *g_pTrace;
extern int errno;




/* ---- Private Prototypes ---- */
static SSLErr sslplus_read_cb(SSLBuffer data, uint32 *puProcessed, void *pCtxt);
static SSLErr sslplus_write_cb(SSLBuffer data, uint32 *puProcessed, void *pCtxt);
void sslplus_ciphers(SSLContext *pCtx);




/* 
 * Called back by SSLPlus to read from socket. 
 */

static SSLErr sslplus_read_cb ( data, puProcessed, pCtxt )
     SSLBuffer data;
     uint32    *puProcessed;
     void      *pCtxt;
{
    int    nAmountRead;
    SSLErr nReturn;

    nAmountRead  = read ( ( (pop_tls *) pCtxt )->m_fd,
                          data.data,
                          data.length);

    *puProcessed = nAmountRead;

    nReturn = SSLNoErr;
    if ( nAmountRead < 0 ) {
        /* Some sort of error */
        nReturn = SSLIOErr;
    } else if ( nAmountRead == 0 ) {
        /* Only was end of file */
        nReturn = SSLIOClosedOverrideGoodbyeKiss;
    }

    fprintf(g_pTrace, "ssl read cb %d %d %d %ld\n", nReturn, nAmountRead, errno,data.length);
    { int i;
      for ( i = 0 ; i < nAmountRead; i++ ) {
          fprintf ( g_pTrace, "ssl read %s\n", data.data + i );
      }
    }
    /* fprintf(g_pTrace, "ssl read %x %x %x %x %x\n", data.data[0], data.data[1], data.data[2], data.data[3], data.data[4]); */
    fflush ( g_pTrace );

    return ( nReturn );
}




/* 
 * Called back by SSLPlus to write encrypted data out to client 
 */

static SSLErr sslplus_write_cb(data, puProcessed, pCtxt)
     SSLBuffer data;
     uint32    *puProcessed;
     void      *pCtxt;
{
    int nAmount;
    int nReturn;

#  ifdef    TEST_CLIENT_FAILURE
    /* Simulate an attack by changing the bits we send out to see how the client
       handles the case */ 
    static long nBytes = 0;

    nBytes += data.length;
    if ( nBytes > 2000 )
      data.data[data.length/2]++; 
#  endif /* TEST_CLIENT_FAILURE */

    fprintf(g_pTrace, "ssl write start %ld %lu %p\n", data.length, *puProcessed, data.data);      

    nAmount = fwrite(data.data, 1, data.length,
                     ((pop_tls *)pCtxt)->m_pOutStream);
    fprintf(g_pTrace, "ssl write result %d %d\n", nAmount, errno);      
    *puProcessed = nAmount;

    nReturn = SSLNoErr;
    if(nAmount == 0) {
        /* Some sort of error */
        nReturn = SSLIOErr;
        if(feof(((pop_tls *)pCtxt)->m_pOutStream))
            /* Only was end of file */
            nReturn= SSLIOClosedOverrideGoodbyeKiss;
    } else {
       fflush(((pop_tls *)pCtxt)->m_pOutStream);
    }

    fprintf(g_pTrace, "ssl write cb %d %d\n", nReturn, nAmount);      
    fflush(g_pTrace);
    return(nReturn);
}




/*
 * Select our cipher suites
 */
void sslplus_ciphers(pCtx)
    SSLContext *pCtx;
{
    /* Just anon DH for testing */
      static uint16 ciphers[] = {SSL_NULL_WITH_NULL_NULL}; 
      /* static uint16 ciphers[] = {SSL_DH_anon_EXPORT_WITH_RC4_40_MD5}; */
  /*   static uint16 ciphers[] = {TLS_ECDHE_ECNRA_WITH_DES_CBC_SHA}; */
      /*   static uint16 ciphers[] = {TLS_ECDH_ECDSA_WITH_RC4_128_SHA}; */

    SSLBuffer ciphersBuf;
    ciphersBuf.data = (uint8*) ciphers;
    ciphersBuf.length = sizeof(ciphers);
    SSLSetCipherSuites ( pCtx, &ciphersBuf );
}




/* 
 *  Initialize SSLPlus -- lots of stuff to do here
 *  Args: pTLS: the POP TLS context (where SSLPLus context goes)
 *        pPOP: the POP structure.  We use:
 *              input_fd: file descriptor to read input on
 *              output: FILE * to write out put too
 *              tls_identity_file: name of identity file
 *              tls_passphrase: to unlock private key in ID file
 *  Returns: -1 on failure, 0 on success
 */

int sslplus_init ( pop_tls *pTLS, POP *pPOP )
{
    SSLErr nErr;
    int    nResult;

    pTLS->m_fd         = pPOP->input_fd;
    pTLS->m_pOutStream = pPOP->output;
    pTLS->m_pPOP       = pPOP;

    /* Assume failure until we get through it all */
    nResult = -1;

    /*
     * Install the native SSL Plus Ezcrypto modules.
     */ 
    EZInstallModule ( EZ_NATIVE_SHA1_MODULE );
    EZInstallModule ( EZ_NATIVE_MD2_MODULE  );
    EZInstallModule ( EZ_NATIVE_MD5_MODULE  );
    EZInstallModule ( EZ_NATIVE_ARC4_MODULE );


    /*
     * Install the Cryptoswift SSL Plus Ezcrypto modules.
     */ 
#  ifdef SSLPLUS_WITH_CRYPTOSWIFT
    EZInstallModule ( EZ_CRYPTOSWIFT_RSA_MODULE  );
    EZInstallModule ( EZ_CRYPTOSWIFT_RSA_MODULE  );
    EZInstallModule ( EZ_CRYPTOSWIFT_PRNG_MODULE );
#  endif

    /*
     * Install the BSAFE SSL Plus Ezcrypto modules.
     */ 
#  ifdef SSLPLUS_WITH_BSAFE
    EZInstallModule ( EZ_BSAFE_RSA_MODULE );
    EZInstallModule ( EZ_BSAFE_DSA_MODULE );
#  endif


    EZInstallModule ( EZ_SECURITY_BUILDER_DSA_MODULE  );
    EZInstallModule ( EZ_SECURITY_BUILDER_DH_MODULE   );
    EZInstallModule ( EZ_SECURITY_BUILDER_DES_MODULE  );
    EZInstallModule ( EZ_SECURITY_BUILDER_3DES_MODULE );
    EZInstallModule ( EZ_SECURITY_BUILDER_ECC_MODULE  );
    EZInstallModule ( EZ_SECURITY_BUILDER_ECDH_MODULE );
    EZInstallModule ( EZ_SECURITY_BUILDER_PRNG_MODULE );


    pTLS->m_pTLSPlus = (SSLContext *) malloc ( SSLContextSize() );
    if ( !pTLS->m_pTLSPlus )
        goto Done;


    /* ---- Some basic initializations ---- */
    SSLInitContext            ( pTLS->m_pTLSPlus                 );
    SSLSetProtocolSide        ( pTLS->m_pTLSPlus, SSL_ServerSide );
    SSLSetProtocolVersion     ( pTLS->m_pTLSPlus, SSL_VERSION    );
    SSLSetMaximumRecordLength ( pTLS->m_pTLSPlus, SSL_MAXRECORD  );
    SSLSetIOSemantics         ( pTLS->m_pTLSPlus, SSL_PartialIO  );


    /* ---- Add the callbacks ---- */
    (void) SSLSetIORef     ( pTLS->m_pTLSPlus, (void *) pTLS   );
    nErr = SSLSetReadFunc  ( pTLS->m_pTLSPlus, sslplus_read_cb );
    if ( nErr != SSLNoErr )
        goto Done;
    nErr = SSLSetWriteFunc ( pTLS->m_pTLSPlus, sslplus_write_cb );
    if ( nErr != SSLNoErr )
        goto Done;


    /* --- Initialize the random number generator and set  up call back --- */
    nErr = ConfigureContextForRandom ( pTLS->m_pTLSPlus,
                                       RANDOM_POOL_FILENAME,
                                       &( pTLS->m_pRandomObj ) );
    if ( nErr != SSLNoErr )
        goto Done;

    /* --- Not sure why we do this.  Maybe it's for DH in TLS v1 ? --- */
    nErr = ConfigureContextDH ( pTLS->m_pTLSPlus );
    if ( nErr != SSLNoErr )
        goto Done;

    /* --- set up our identity --- */
    /* /Bug: this needs work */
    nErr = SSLLoadLocalIdentity ( pTLS->m_pTLSPlus,
                                  pPOP->tls_identity_file,
                                  pPOP->tls_passphrase ); 
    if ( nErr != SSLNoErr ) {
        fprintf ( g_pTrace, "ID error %d\n", nErr );      
        fflush  ( g_pTrace );
        goto Done;
    }

    /* --- Set up cipher suites --- */
    sslplus_ciphers ( pTLS->m_pTLSPlus );

    /* Made it through it all */
    nResult = 0;

  Done:
    return ( nResult );
}


/* 
 *  SSL Plus handshake
 *
 */

int sslplus_handshake ( pop_tls *pTLS )
{
    int nReturn;


    /* Keep session DB open only when handshaking to minimize time during
     * which it is locked against other qpopper processes
     */
    pTLS->m_pSessDB = OpenSessDB ( pTLS->m_pTLSPlus );

    nReturn = SSLHandshake ( pTLS->m_pTLSPlus );


    { SSLErr e;
      uint16 nSuite;

      e =  SSLGetNegotiatedCipher ( pTLS->m_pTLSPlus, &nSuite );
      fprintf ( g_pTrace, "Cipher Suite: %d %x\n:", nSuite, nSuite );
    }
    
    CloseSessDB ( pTLS->m_pSessDB );

    return ( nReturn );
}




/* 
 * Read from an SSL Plus TLS connection.  Assumes 
 * everything is nicely initialized.
 *
 * Args: pTLS:      our POP TLS context
 *       pcBuffer:  buffer to read into
 *       nLength:   size of buffer
 *
 * Returns: number of bytes read or -1
 */

int sslplus_read ( pTLS, pcBuffer, nLength )
     pop_tls *pTLS;
     char    *pcBuffer;
     int      nLength;
{
    uint32 nL = nLength;
    SSLErr nErr;

    fprintf ( g_pTrace, "tls read start %d %p\n", nLength, pcBuffer );
    fflush  ( g_pTrace);

    nErr = SSLRead ( pcBuffer, &nL, pTLS->m_pTLSPlus );

    fprintf ( g_pTrace, "tls read %d %ld %x %x\n", nErr, nL, pcBuffer[0], pcBuffer[1] );
    fflush  ( g_pTrace);

    /* /Bug: these error are not right */
    if ( nErr == SSLNoErr )
        return ( nL );
    else
        return ( -1 );
}




/* 
 *  Write data down an SSL PLus TLS connection (assumed open)
 *
 *  Args: pTLS:     our context
 *        pcBuffer: data to write
 *        nLength:  number of bytes to write
 *
 *  Returns:        -1 on error or number of bytes written
 *
 *  This writes all bytes or fails as it uses non-blocking I/O
 */

int sslplus_write ( pTLS, pcBuffer, nLength )
     pop_tls *pTLS;
     char    *pcBuffer;
     int      nLength;
{
    uint32 nL = nLength;
    SSLErr nErr;


    nErr = SSLWrite ( pcBuffer, &nL, pTLS->m_pTLSPlus );
    if ( nErr )
        return ( -1 );
    else
        return ( nL );
}







#endif /* QPOP_SSLPLUS */








