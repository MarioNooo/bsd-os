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
*  FILENAME:  sha.h           PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Internal Functions Header File
*
*  USAGE:           File should be included in Toolkit functions files
*
*
*       Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*  24 Sep 94  KPZ             Initial release
*
****************************************************************************/
#ifndef SHA_H
#define SHA_H
#include "cylink.h"


#define       SHS_BLOCKSIZE      64
/*
#define FSHA(x,y,z) ( ( x & y ) | ( ~x & z ) )
#define GSHA(x,y,z) ( x ^ y ^ z )
#define HSHA(x,y,z) ( ( x & y ) | ( x & z ) | ( y & z ) )
#define ISHA(x,y,z) (x ^ y ^ z)
*/
/*#define f1(x,y,z)     ( (x & y) | (~x & z) )          // Rounds  0-19 */
#define f1(x,y,z)     ( z ^ (x & (y ^ z) ) )          /* Rounds  0-19 */
#define f2(x,y,z)     ( x ^ y ^ z )                   /* Rounds 20-39 */
/*#define f3(x,y,z)   ( (x & y) | (x & z) | (y & z) ) // Rounds 40-59 */
#define f3(x,y,z)     ( (x & y) | (z & (x | y) ) )    /* Rounds 40-59 */
#define f4(x,y,z)     ( x ^ y ^ z )                   /* Rounds 60-79 */


#define RotateLeft(x,n) (( x << n )|( x >> (32-n) ) )  /*Circular left shift operation*/

/*SHS Constants */
#define k1SHA  0x5a827999L
#define k2SHA  0x6ed9eba1L
#define k3SHA  0x8f1bbcdcL
#define k4SHA  0xca62c1d6L

/*SHS initial value */
#define h0SHA  0x67452301L
#define h1SHA  0xefcdab89L
#define h2SHA  0x98badcfeL
#define h3SHA  0x10325476L
#define h4SHA  0xc3d2e1f0L

/*The initial expanding function*/
#define expand(count) \
   {\
        W[count] = W[count-3] ^ W[count-8] ^ W[count-14] ^ W[count-16];\
        W[count] = RotateLeft( W[count], 1 );\
        }

/*New variant */
#define subRound(a, b, c, d, e, f, k, data) \
  ( e += RotateLeft(a,5) + f(b, c, d) + k + data, b = RotateLeft( b,30) )



/*The four sub_rounds*/
/*
#define subR1(count) \
  {\
   temp=RotateLeft(A,5) + FSHA(B,C,D) + E +W[count] +k1SHA;\
   E = D; \
   D = C; \
   C = RotateLeft(B,30); \
   B = A; \
   A = temp; \
  }

#define subR2(count) \
  {\
   temp=RotateLeft(A,5) + GSHA(B,C,D) + E +W[count] +k2SHA;\
   E = D; \
   D = C; \
   C = RotateLeft(B,30);\
   B = A; \
   A = temp; \
  }

#define subR3(count) \
  {\
   temp=RotateLeft(A,5) + HSHA(B,C,D) + E +W[count] +k3SHA;\
   E = D; \
   D = C; \
   C = RotateLeft(B,30);\
   B = A; \
   A = temp; \
  }

#define subR4(count) \
  {\
   temp=RotateLeft(A,5) + ISHA(B,C,D) + E + W[count] +k4SHA;\
   E = D; \
   D = C; \
   C = RotateLeft(B,30);\
   B = A; \
   A = temp; \
  }
*/
#endif  /* SHA_H */

