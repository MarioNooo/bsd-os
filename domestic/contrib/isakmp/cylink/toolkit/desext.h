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
*  FILENAME:  desext.h        PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     DES and KAPPA EXTERNAL Tables Header File
*
*  USAGE:      File should be #included to use Toolkit Functions
*
*
*        Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*  24 Sep 94  KPZ             Initial release
*
****************************************************************************/
#ifndef DESEXT_H
#define DESEXT_H

#define KAPPA_ROUNDS       7

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/

/* system files */
#include "cylink.h"

/* table for KAPPA */
extern uchar exptab[512];

/* initial permutation */
extern uchar ip[];

/* final permutation IP^-1 */
extern uchar fp[];

/* expansion operation matrix
 * This is for reference only; it is unused in the code
 * as the f() function performs it implicitly for speed
 */
extern uchar ei[];

/* permuted choice table (key) */
extern uchar pc1[];

/* number left rotations of pc1 */
extern uchar totrot[];

/* permuted choice key (table) */
extern uchar pc2[];

/* The (in) famous S-boxes */
extern uchar si[8][64];

/* 32-bit permutation function P used on the output of the S-boxes */
extern uchar p32i[];
/* Lookup tables initialized once only at startup by desinit() */
extern ulong sp[8][64];         /* Combined S and P boxes */

extern uchar iperm[16][16][8];  /* Initial and final permutations */
extern uchar fperm[16][16][8];

/* bit 0 is left-most in byte */
extern int bytebit[];
extern int nibblebit[];

#endif /*DESEXT_H*/

