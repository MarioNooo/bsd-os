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
*  FILENAME:  cencrint.h      PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Internal Functions Header File
*
*  USAGE:           File should be included to use Toolkit Functions
*
*
*         Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*  23 Aug 94  KPZ     Initial release
*  24 Sep 94    KPZ     Added prototypes of internal functions
*  14 Oct 94    GKL     Second version (big endian support)
*  08 Dec 94    GKL             Added YIELD_context to Expo, VerPrime and GenPrime
*
****************************************************************************/

#ifndef CENCRINT_H     /* Prevent multiple inclusions of same header file */
#define CENCRINT_H

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/

/* system files */
/*#include <stdio.h>*/
#include "cylink.h"
#include "endian.h"
#include "toolkit.h"

#ifdef  __cplusplus
extern  "C" {
#endif

/* Compute a modulo */
 int PartReduct( ushort X_bytes, ord   *X,
                 ushort P_bytes, ord   *P,
               ord   *Z );

/* Compute a modulo product */
 int Mul( ushort X_bytes, ord   *X,
     ushort Y_bytes, ord   *Y,
       ushort P_bytes, ord   *P,
       ord   *Z );
/*Compute a modulo squaring*/
int Mul_Squr( ushort X_bytes, ord    *X,
            ushort P_bytes, ord    *P,
              ord    *Z );
/*Compare two array*/
int Comp_Mont ( ord *X, ord *P, ushort P_longs );
/*Compute invers element*/
ord Inv_Mont ( ord x );
/*Modulo by the Mongomery*/
void PartReduct_Mont( ord *X, ushort P_bytes, ord  *P, ord inv );
/*Computes squaring by the Mongomery modulo*/
int Mul_Squr_Mont(ord    *X, ushort P_bytes,
            ord    *P, ord    *Z,
           ord     inv );
/*Computes multiply by the montgomery modulo*/
int Mul_Mont( ord    *X, ord    *Y,
             ushort P_bytes, ord    *P,
              ord    *Z, ord     inv );

/* Compute a modulo exponent */
 int Expo( ushort X_bytes, ord   *X,
         ushort Y_bytes, ord   *Y,
       ushort P_bytes, ord   *P,
       ord   *Z,       YIELD_context *yield_cont ); /*TKL00601*/
/* Compute a modulo inverse element */
 int Inverse( ushort X_bytes, ord   *X,
             ushort P_bytes, ord   *P,
       ord   *Z );

/* Verify Pseudo Prime number */
 int VerPrime( ushort P_bytes, ord    *P,
             ushort k,       ord    *RVAL,
           YIELD_context *yield_cont ); /*TKL00601*/

/* Generate Random Pseudo Prime number */
 int GenPrime( ushort P_bytes, ord    *P,
             ushort k,       ord    *RVAL,
           YIELD_context *yield_cont ); /*TKL00601*/

/* Transfer bytes to ulong */
 void  ByteLong( uchar *X,
          ushort X_bytes,
                 ulong *Y );

/* Transfer ulong to bytes */
 void  LongByte( ulong *X,
              ushort X_bytes,
                 uchar  *Y );

/* Transfer bytes to ord */
  void  ByteOrd( uchar *X,
               ushort X_bytes,
                 ord *Y );

/* Transfer ord to bytes */
  void  OrdByte( ord *X,
            ushort X_bytes,
                 uchar *Y );

/* Find the left most non zero bit */
 int LeftMostBit ( ord X );

/* Find the left most element */
 int LeftMostEl( ord *X,
             ushort len_X );

/* Shift array to rigth by n_bit */
 void  RShiftL( ord   *X,
            ushort  len_X,
          ushort  n_bit );

/* Shifts array to left by n_bit */
 void  LShiftL( ord  *X,
             ushort len_X,
           ushort n_bit );

/* Find the value of bit */
 int BitValue( ord *X,
               ushort n_bits );

/* Perform byte reversal on an array of ordinar type (longword or shortword) */
 void ByteSwap( uchar  *X,
                                ushort X_len );

/* Perform byte reversal on an array of longword */
 void ByteSwap32( uchar  *X,
                            ushort X_len );

/* Perform short reversal on an array of longword */
 void WordSwap( uchar  *X,
                         ushort X_len );

/* Perform  SHS transformation */
 void shaTransform( ulong *state,
                                       uchar *block );

/* Compute modulo addition */
 int Add( ord    *X,
                   ord    *Y,
              ushort P_len,
                   ord    *P,
              ord    *Z );

/*  Initialize Secure Hash Function for generate
 random number for DSS                                           */
 void SHAInitK( SHA_context *hash_context );

/* Set parity bits */
 void SetKeyParity( uchar *key );

/*  Find a least significant non zero bit
      and sfift array to right            */
 int RShiftMostBit( ord *a, ushort len );

/*Compute great common divisor */
 int SteinGCD( ord *m, ord *b, ushort len );

/* Compute a modulo and divisor */
 int DivRem( ushort X_bytes, ord    *X,
             ushort P_bytes, ord    *P,
             ord    *Z,      ord    *D );

/* Generate random number */
int MyGenRand( ushort A_bytes, ord    *A,
                               ord    *RVAL);

/* Compute a Secure Hash Function */
int MySHA( uchar   *message,
    ushort message_bytes,
           uchar  *hash_result );

/* Finalize Secure Hash Function */
 int MySHAFinal( SHA_context *hash_context,
          uchar       *hash_result );

 void shaTransform_new( ulong *state,
                   uchar *block );


#ifdef  __cplusplus
}
#endif


#endif /* CENCRINT_H */

