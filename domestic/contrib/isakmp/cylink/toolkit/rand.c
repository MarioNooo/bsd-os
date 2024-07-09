/*
 * This cryptographic library is offered under the following license:
 * 
 *      "Cylink Corporation, through its wholly owned subsidiary Caro-Kann
 * Corporation, holds exclusive sublicensing rights to the following U.S.
 * patents owned by the Leland Stanford Junior University:
 *  
 *         Cryptographic Apparatus and Method
 *         ("Hellman-Diffie") .................................. No. 
4,200,770
 *  
 *         Public Key Cryptographic Apparatus
 *         and Method ("Hellman-Merkle") .................. No. 4,218, 582
 *  
 *         In order to promote the widespread use of these inventions from
 * Stanford University and adoption of the ISAKMP reference by the IETF
 * community, Cisco has acquired the right under its sublicense from Cylink 
to
 * offer the ISAKMP reference implementation to all third parties on a 
royalty
 * free basis.  This royalty free privilege is limited to use of the ISAKMP
 * reference implementation in accordance with proposed, pending or 
approved
 * IETF standards, and applies only when used with Diffie-Hellman key
 * exchange, the Digital Signature Standard, or any other public key
 * techniques which are publicly available for commercial use on a royalty
 * free basis.  Any use of the ISAKMP reference implementation which does 
not
 * satisfy these conditions and incorporates the practice of public key may
 * require a separate patent license to the Stanford Patents which must be
 * negotiated with Cylink's subsidiary, Caro-Kann Corporation."
 * 
 * Disclaimer of All Warranties  THIS SOFTWARE IS BEING PROVIDED "AS IS", 
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTY OF ANY KIND WHATSOEVER.
 * IN PARTICULAR, WITHOUT LIMITATION ON THE GENERALITY OF THE FOREGOING,  
 * CYLINK MAKES NO REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING 
 * THE MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 *
 * Cylink or its representatives shall not be liable for tort, indirect, 
 * special or consequential damages such as loss of profits or loss of 
goodwill 
 * from the use or inability to use the software for any purpose or for any 
 * reason whatsoever.
 *
 *******************************************************************
 * This cryptographic library incorporates the BigNum multiprecision 
 * integer math library by Colin Plumb.
 *
 * BigNum has been licensed by Cylink Corporation 
 * from Philip Zimmermann.
 *
 * For BigNum licensing information, please contact Philip Zimmermann
 * (prz@acm.org, +1 303 541-0140). 
 *******************************************************************
 * 
/****************************************************************************
*  FILENAME:  rand.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Functions File
*                   Random Number Generation Files
*  PUBLIC FUNCTIONS:
*      int InitRand( ushort SEED_bytes, uchar  *SEED,
*                                       uchar  *RVAL )
*      int GenRand( ushort A_bytes, uchar  *A,
*                                   uchar  *RVAL )
*	int MyGenRand( ushort A_bytes,
*                  ord    *A,
*                  ord    *RVAL )

*   Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  10 Oct 94   KPZ     Added Shamir Key Sharing functions
*  10 Oct 94   KPZ     Modified SHA functions for arbitrary message length
*  12 Oct 94   KPZ     Modified SHA functions (new standard)
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*
****************************************************************************/

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/

#include "endian.h"

/* system files */
#ifdef VXD
#include <vtoolsc.h>
#else
#include <stdlib.h>
#include <string.h>
#endif

/* program files */
#ifdef VXD
#include "tkvxd.h"
#endif
#include "cylink.h"
#include "toolkit.h"
#include "cencrint.h"
#include "c_asm.h"
#include "sha.h"

/****************************************************************************
*  PUBLIC FUNCTIONS DEFINITIONS
****************************************************************************/

/****************************************************************************
*  NAME:    int InitRand( ushort SEED_bytes,
*                         uchar  *SEED,
*                         uchar  *RVAL)
*
*  DESCRIPTION:  Initialize Random number Generator
*
*  INPUTS:
*      PARAMETERS:
*          ushort SEED_bytes  Length of SEED
*          uchar *SEED        Pointer to SEED value
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *RVAL        Pointer to RVAL
*
*      RETURN:
*          SUCCESS            No errors
*          ERR_INPUT_LEN      Invalid length for input data
*          ERR_DATA           Generic data error
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*
****************************************************************************/

int InitRand( ushort SEED_bytes,
              uchar  *SEED,
        uchar  *RVAL )
{
    int  status = SUCCESS;          /* function return status */
    if ( SEED_bytes == 0 )
    {
            status = ERR_INPUT_LEN;
        return status;
    }
    if ( SEED_bytes < SHA_LENGTH )
    {
        status = ERR_DATA;
        return status;
    }
    memcpy( RVAL, SEED, SHA_LENGTH);
      return status;
}


/****************************************************************************
*  NAME:    int GenRand( ushort A_bytes,
*                        uchar  *A,
*                        uchar  *RVAL)
*
*  DESCRIPTION:  Generate random number.
*
*  INPUTS:
*      PARAMETERS:
*          ushort A_bytes       Length of A
*          uchar *A             Pointer to A value
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *RVAL          Pointer to RVAL
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data
*          ERR_DATA             Generic data error
*          ERR_ALLOC            Insufficient memory
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*
****************************************************************************/
int GenRand( ushort A_bytes,
             uchar  *A,
             uchar  *RVAL )
{
   int  status = SUCCESS;          /* function return status */
    ord *RVAL_a;
    SHA_context hash_context;       /* SHA context structure */
    uchar M[DSS_LENGTH_MIN];        /* message block */
#ifdef ORD_16
    ord one[SHA_LENGTH / sizeof(ord)] =
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
    ord one[SHA_LENGTH / sizeof(ord)] =
            { 1, 0, 0, 0, 0 };
#endif
    uchar hash_result[SHA_LENGTH];
    ushort i;
    ushort sha_block;              /* number of sha blocks */
    ushort sha_rem;                /* size of last block */
    if ( A_bytes == 0 )
    {
     status = ERR_INPUT_LEN;
         return status;
    }
    sha_block = A_bytes / SHA_LENGTH;   /* number of sha blocks */
    sha_rem = A_bytes % SHA_LENGTH;     /* size of last block */
    if ( sha_rem == 0 )                 /* last block = SHA_LENGTH */
    {
 sha_block--;
    }
    for ( i = 0; i <= sha_block; i++)
  {
       SHAInit ( &hash_context );
      memcpy( M, RVAL, SHA_LENGTH);
   memset( M + SHA_LENGTH, 0, DSS_LENGTH_MIN - SHA_LENGTH );
       if ( (status = SHAUpdate( &hash_context, M, DSS_LENGTH_MIN ))
        != SUCCESS )
       {
           return status;                        /* error */
   }
               if ( (status=MySHAFinal (&hash_context, hash_result )) != SUCCESS )
     {
           return status;                       /* error */
    }
       ALIGN_CALLOC_COPY(RVAL, RVAL_a, SHA_LENGTH);
    if ( status !=  SUCCESS )
       {
          ALIGN_COPY_FREE(RVAL_a,RVAL,SHA_LENGTH);
        return status;     /* ERR_ALLOC   insufficient memory */
     }
               Sum_big( RVAL_a, one,     /* RVAL=RVAL+1*/
               RVAL_a, SHA_LENGTH / sizeof(ord) );
    Sum_big( RVAL_a,                 /* RVAL=RVAL+hash_result*/
              (ord *)hash_result,
             RVAL_a, SHA_LENGTH / sizeof(ord) );
    ALIGN_COPY_FREE(RVAL_a,RVAL,SHA_LENGTH);
#ifdef B_ENDIAN
       ByteSwap(hash_result,SHA_LENGTH);
#endif
         if ( i == sha_block  && sha_rem != 0 )  /* last block < SHA_LENGTH*/
    {
           memcpy( A + i * SHA_LENGTH, hash_result,
                sha_rem * sizeof (uchar));
  }
       else                                   /* last block = SHA_LENGTH*/
     {
           memcpy( A + i * SHA_LENGTH, hash_result,
               SHA_LENGTH * sizeof (uchar));
                }
       }
    return status;
}



/****************************************************************************
*  NAME:        int MyGenRand( ushort A_bytes,
*                              ord    *A,
*                              ord    *RVAL)
*
*  DESCRIPTION:  Generate random number.
*
*  INPUTS:
*          PARAMETERS:
*                  ushort A_bytes               Length of A
*              ord   *A             Pointer to A value
*
*  OUTPUT:
*          PARAMETERS:
*          ord   *RVAL          Pointer to RVAL
*
*          RETURN:
*                  SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data
*                  ERR_DATA                         Generic data error
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ         Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/
int MyGenRand( ushort A_bytes,
               ord    *A,
               ord    *RVAL )
{
   int  status = SUCCESS;          /* function return status */
    SHA_context hash_context;       /* SHA context structure */
     uchar M[DSS_LENGTH_MIN];        /* message block */
#ifdef ORD_16
    ord one[SHA_LENGTH / sizeof(ord)] =
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
	ord one[SHA_LENGTH / sizeof(ord)] =
     { 1, 0, 0, 0, 0 };
#endif
        uchar hash_result[SHA_LENGTH];
	ushort i;
    ushort sha_block;              /* number of sha blocks */
    ushort sha_rem;                /* size of last block */
    if ( A_bytes == 0 )
    {
         status = ERR_INPUT_LEN;
         return status;
  }
    sha_block = A_bytes / SHA_LENGTH;   /* number of sha blocks */
    sha_rem = A_bytes % SHA_LENGTH;     /* size of last block */
    if ( sha_rem == 0 )                 /* last block = SHA_LENGTH */
	{
           sha_block--;
    }
       for ( i = 0; i <= sha_block; i++)
       {
               SHAInit ( &hash_context );
              memcpy( M, RVAL, SHA_LENGTH);
		   memset( M + SHA_LENGTH, 0, DSS_LENGTH_MIN - SHA_LENGTH );
               if ( (status = SHAUpdate( &hash_context, M, DSS_LENGTH_MIN ))
                         != SUCCESS )
              {
					   return status;                        /* error */
               }
        if ( (status=MySHAFinal (&hash_context, hash_result )) != SUCCESS )
            {
                       return status;                       /* error */
                }
#ifdef B_ENDIAN
		ByteSwap((uchar*)RVAL,SHA_LENGTH);
#endif
        Sum_big( RVAL, one,     /* RVAL=RVAL+1*/
                 RVAL, SHA_LENGTH / sizeof(ord) );
		Sum_big( RVAL,                 /* RVAL=RVAL+hash_result*/
            (ord*)hash_result,
                 RVAL, SHA_LENGTH / sizeof(ord) );
#ifdef B_ENDIAN
          ByteSwap((uchar*)RVAL,SHA_LENGTH);
#endif
                if ( i == sha_block  && sha_rem != 0 )  /* last block < SHA_LENGTH*/
			{
                       memcpy( &A[ i*SHA_LENGTH / sizeof(ord)], hash_result,
                                        sha_rem * sizeof (uchar));
         }
			   else                                   /* last block = SHA_LENGTH*/
             {
                       memcpy( &A[ i*SHA_LENGTH / sizeof(ord)], hash_result,
                                   SHA_LENGTH * sizeof (uchar));
           }
       }
       return status;
}

