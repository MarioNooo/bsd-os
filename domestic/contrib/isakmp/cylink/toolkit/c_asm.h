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
*  FILENAME:  c_asm.h        PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     C / ASM Header File
*
*  USAGE:           File should be included to use Toolkit Functions
*
*
*   Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*  14 Oct 94    GKL     For Second version (big endian support)
*  26 Oct 94    GKL     (alignment for big endian support )
*
****************************************************************************/
#if !defined( C_ASM_H )
#define C_ASM_H

#include "cylink.h"
#include "endian.h"

#ifdef  __cplusplus
extern  "C" {
#endif


 int Sum_big (ord *X,
              ord *Y,
         ord *Z,
         int len_X );

  int Sub_big (ord *X,
              ord *Y,
         ord *Z,
         int len_X );
/*
  void  Mul_big( ord *X, ord *Y,ord *XY,
            ushort lx, ushort ly,
           ushort  elements_in_X,
          ushort  elements_in_Y);*/
  void  Mul_big( ord *X, ord *Y,ord *XY,
               ushort lx, ushort ly);

  void  PReLo_big( ord *X, ord *P,
                    ushort len_X, ushort el);

  void  Div_big( ord *X, ord *P,
           ushort len_X, ushort el,
                ord *div);

int LeftMostBit_big ( ord X );
int LeftMostEl_big( ord *X, ushort len_X );
void  RShiftL_big( ord  *X, ushort len_X, ushort  n_bit );
void  LShiftL_big( ord *X, ushort len_X, ushort n_bit );
int BitValue_big( ord  *X, ushort n_bits );
int BitsValue_big( ord  *X, ushort n_bits, ushort bit_count );
void ByteSwap32_big( uchar  *X, ushort X_len );
void Complement_big( ord *X, ushort X_longs);
void Diagonal_big (ord *X, ushort X_len, ord *X2);
void Square_big( ord *X, ushort X_len, ord *X2);
void  Mul_big_1( ord  X, ord *Y, ord *XY, ushort ly );




/* In-place DES encryption */
  void DES_encrypt(uchar *keybuf, uchar *block);

/* In-place DES decryption */
  void DES_decrypt(uchar *keybuf, uchar *block);

/* In-place KAPPA encryption */
  void KAPPA_encrypt(uchar *a, uchar *k, ushort r);

/* In-place KAPPA decryption */
  void KAPPA_decrypt(uchar *a, uchar *k, ushort r);

#ifdef  __cplusplus
}
#endif


#endif   /*C_ASM_H*/

