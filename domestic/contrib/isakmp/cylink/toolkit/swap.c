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
*  FILENAME: swap.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION: Byte and Word Swap functions 
*
*  PUBLIC FUNCTIONS:
*
*
*  PRIVATE FUNCTIONS:
*
*  REVISION  HISTORY:
*
*  14 Oct 94   GKL     Initial release
*  26 Oct 94   GKL     (alignment for big endian support )
*
****************************************************************************/

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/
/* bn files */
#include "bn.h"
#include "bn32.h"
/* system files */
#ifdef VXD
#include <vtoolsc.h>
#else
#include <stdlib.h>
#include <string.h>
#endif
/* program files */
#include "cylink.h"
#include "endian.h"
#include "desext.h"

/*Reset bytes in long*/
/*extern void ByteSwap32_asm( uchar  *X, ushort X_len );*/ /*kz*/

/****************************************************************************
*  NAME:     void ByteSwap32 (uchar  *array,
*                             ushort X_len )
*
*  DESCRIPTION:  Perform byte reversal on an array of longword.
*
*  INPUTS:
*          PARAMETERS:
*            uchar  *X            Pointer to array
*                        ushort X_len             Number of bytes
*  OUTPUT:
*            uchar  *X            Pointer to array
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/


void ByteSwap32_big( uchar  *X, ushort X_len )
{
/*#if defined ( ASM_FUNCTION )  && defined( L_ENDIAN )
  ByteSwap32_asm ( X, X_len);
#else*/ /*kz*/
   ushort i;        /*counter*/
    uchar a;      /*temporary char*/
        for ( i = 0; i < X_len; i += 4)
 {
               a = X[i];
               X[i] = X[i+3];
          X[i+3] = a;
             a = X[i+1];
             X[i+1] = X[i+2];
                X[i+2] = a;
     }
/*#endif*/ /*kz*/
}


/****************************************************************************
*  NAME:     void ByteSwap (uchar  *array,
*                           ushort X_len )
*
*  DESCRIPTION:  Perform byte reversal on an array of longword or shortword.
*
*  INPUTS:
*          PARAMETERS:
*            uchar  *X            Pointer to array
*            ushort X_len             Number of bytes
*  OUTPUT:
*            uchar  *X            Pointer to array
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void ByteSwap( uchar  *X,
               ushort X_len )
{
#ifdef ORD_16
    ushort i;        /*counter*/
    uchar a;      /*tempriory char for revers*/
	for ( i = 0; i < X_len; i += 2)
    {
		a = X[i];
        X[i] = X[i+1];
        X[i+1] = a;
    }
#endif
#ifdef ORD_32
      ByteSwap32_big(X,X_len);
#endif
}

/*kz longbyte deleted */

/****************************************************************************
*  NAME:     void WordSwap (uchar  *array,
*                           ushort X_len )
*
*  DESCRIPTION:  Perform short reversal on an array of longword.
*
*  INPUTS:
*          PARAMETERS:
*            uchar  *X            Pointer to array
*                        ushort X_len             Number of bytes
*  OUTPUT:
*            uchar  *X            Pointer to array
*
*  REVISION HISTORY:
*
*  14 Oct 94    GKL     Initial release
*
****************************************************************************/
void WordSwap( uchar  *X,
               ushort X_len )
{
	ushort i;        /*counter*/
    ushort a;      /*tempriory ushort*/

    for ( i = 0; i < X_len; i += 4)
    {
        a = *(ushort*)(&X[i]);
        *(ushort*)(&X[i])=*(ushort*)(&X[i+2]);
        *(ushort*)(&X[i+2])=a;
    }
}

