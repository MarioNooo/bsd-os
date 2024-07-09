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
 */
 
/****************************************************************************
*  FILENAME: bit.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:  Bit Utility Functions
*
*  PUBLIC FUNCTIONS:
*
*
*  PRIVATE FUNCTIONS:
*
*  REVISION  HISTORY:
*
*
****************************************************************************/

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/
/* bn files */
#include "bn.h"
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


/****************************************************************************
*  NAME: void  RShiftL( ord   *X,
*                       ushort  len_X,
*                       ushort  n_bit )
*
*  DESCRIPTION:  Shift array to the right by n_bit.
*
*  INPUTS:
*          PARAMETERS:
*            ord  *X            Pointer to array
*            ushort len_X         Length of array
*                        ushort n_bit             Number of bits
*  OUTPUT:
*          PARAMETERS:
*            ord  *X            Pointer to array
*
*          RETURN:
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void  RShiftL_big( ord   *X,
	           ushort  len_X,
  		   ushort  n_bit )
{

struct BigNum dest;
bnInit();
bnBegin(&dest);

dest.ptr = X;
dest.size = len_X;
dest.allocated = len_X;

bnRShift(&dest,n_bit);

}

/****************************************************************************
*  NAME: void  LShiftL( ord   *X,
*                       ushort  len_X,
*                       ushort  n_bit )
*
*  DESCRIPTION:  Shifts array to the left by n_bit.
*
*  INPUTS:
*          PARAMETERS:
*            ord  *X            Pointer to array
*            ushort len_X         Length of array
*                        ushort n_bit             Number of bits
*  OUTPUT:
*          PARAMETERS:
*            ord  *X            Pointer to array
*
*          RETURN:
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void  LShiftL_big( ord *X,
			   ushort len_X,
		   ushort n_bit )
{
struct BigNum dest;
bnInit();
bnBegin(&dest);

dest.ptr = X;
dest.size = len_X;
dest.allocated = len_X;

bnLShift(&dest,n_bit);
}

/************9****************************************************************
*  NAME:     int RShiftMostBit( ord *a,
*                                                       ushort len )
*
*  DESCRIPTION:  Find a least significant non zero bit
*                                and sfift array to the right
*
*  INPUTS:
*          PARAMETERS:
*           ord *a           Pointer to array
*           ushort len         Number of elements in number
*  OUTPUT:
*
*  RETURN:
*            Number of shifted bits
*
*  REVISION HISTORY:
*
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

int RShiftMostBit( ord *a,
			 ushort len )
{

struct BigNum n;
bnInit();
bnBegin(&n);

n.size = len;
n.ptr = a;
n.allocated = len;

return (bnMakeOdd(&n));

}


/****************************************************************************
*  NAME:   void  ByteLong (uchar *X, ushort X_bytes,
*                                    ulong  *Y )
*
*
*  DESCRIPTION:  Transfer bytes to ulong.
*
*  INPUTS:
*          PARAMETERS:
*            uchar  *X            Pointer to byte array
*            ushort X_bytes   Number of bytes in array
*  OUTPUT:
*          PARAMETERS:
*            ulong *Y         Pointer to long arrray
*
*          RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

void  ByteLong( uchar *X,
                ushort X_bytes,
                ulong *Y )
{
    ushort i, j;                    /* counters */
    for ( i = 0, j = 0; j < X_bytes; i++, j += 4)
    {
				Y[i] = ( (ulong)X[j] ) | ( ((ulong)X[j+1]) << 8 ) |
                     ( ((ulong)X[j+2]) << 16 ) | ( ((ulong)X[j+3]) << 24 );
	}
}

/****************************************************************************
*  NAME:   void  ByteOrd (uchar *X, ushort X_bytes,
*                                   ord   *Y )
*
*
*  DESCRIPTION:  Transfer bytes to ord.
*
*  INPUTS:
*          PARAMETERS:
*            uchar  *X            Pointer to byte array
*            ushort X_bytes   Number of bytes in array
*  OUTPUT:
*          PARAMETERS:
*            ord *Y         Pointer to long array
*
*          RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

void  ByteOrd( uchar *X,
               ushort X_bytes,
               ord *Y )
{
    ushort i, j;                    /* counters */
	for ( i = 0, j = 0; j < X_bytes; i++, j += sizeof(ord))
    {
              Y[i] = ( (ord)X[j] ) | ( ((ord)X[j+1]) << 8 )
#ifdef ORD_32
          | ( ((ord)X[j+2]) << 16 ) | ( ((ord)X[j+3]) << 24 )
#endif
            ;
    }
}

/****************************************************************************
*  NAME:   void  OrdByte (ord *X, ushort X_bytes,
*                                 uchar  *Y )
*
*
*  DESCRIPTION:  Transfer ord to bytes.
*
*  INPUTS:
*          PARAMETERS:
*            ord  *X            Pointer to ord array
*            ushort X_bytes     Number of bytes in array
*  OUTPUT:
*          PARAMETERS:
*            uchar *Y         Pointer to byte array
*
*          RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

void  OrdByte( ord *X,
               ushort X_bytes,
               uchar *Y )
{
    ushort i, j;              /* counters */
    for ( i=0, j=0; j < X_bytes; i++, j += sizeof(ord))
    {
			  Y[j] = (uchar ) ( X[i] & 0xff );
				Y[j+1] = (uchar)( (X[i] >> 8) & 0xff );
#ifdef ORD_32
        Y[j+2] = (uchar)( (X[i] >> 16) & 0xff );
        Y[j+3] = (uchar)( (X[i] >> 24) & 0xff );
#endif
    }
}

/****************************************************************************
*  NAME: void  LongByte( ulong  *X,
*                        ushort X_bytes,
*                        uchar  *Y )
*
*  DESCRIPTION:  Transfer ulong to bytes.
*
*  INPUTS:
*          PARAMETERS:
*            ulong  *X            Pointer to long array
*            ushort X_bytes   Number of longs in array
*  OUTPUT:
*          PARAMETERS:
*            uchar *Y         Pointer to bytes array
*
*          RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void  LongByte( ulong *X,
				ushort X_bytes,
                uchar  *Y )
{
    ushort i, j;              /* counters */
    for ( i=0, j=0; j < X_bytes; i++, j += 4)
    {
            Y[j] = (uchar ) ( X[i] & 0xff );
        Y[j+1] = (uchar)( (X[i] >> 8) & 0xff );
        Y[j+2] = (uchar)( (X[i] >> 16) & 0xff );
		Y[j+3] = (uchar)( (X[i] >> 24) & 0xff );
    }
}


