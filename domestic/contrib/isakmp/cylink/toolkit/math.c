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
*  FILENAME: math.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION: Math Routines for the ToolKit 
*
*  PUBLIC FUNCTIONS:
*
*         int Sum_big (ord *X,
*                      ord *Y,
*                      ord *Z,
*                      ushort len_X )
*
*         int Sub_big (ord *X,
*                      ord *Y,
*                      ord *Z,
*                      ushort len_X )
*
*         void  Mul_big( ord *X, ord *Y,ord *XY,
*                        ushort lx, ushort ly)
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
#include "toolkit.h"

/****************************************************************************
*   NAME: int Sum_big (ord *X,
*                      ord *Y,
*                      ord *Z,
*                      ushort len_X )
*
*  DESCRIPTION:  Compute addition.
*
*  INPUTS:
*     PARAMETERS:
*           ord  *X        Pointer to first array
*           ord  *Y        Pointer to second array
*           int  len_X     Number of longs in X_l
*  OUTPUT:
*     PARAMETERS:
*           ord *Z         Pointer to result arrray
*
*     RETURN:
*            Carry bit
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

 int Sum_big (ord *X,
			  ord *Y,
			  ord *Z,
			  ushort len_X )
{

struct BigNum src2,temp_bn;
ord *temp;

bnInit();
bnBegin(&src2);
bnBegin(&temp_bn);

temp = (ord *) malloc((len_X*sizeof(ord)) + sizeof(ord));
temp_bn.size = len_X;
temp_bn.ptr = temp;
temp_bn.allocated = len_X + 1;

src2.ptr = Y;
src2.size = len_X;
src2.allocated = len_X;

memcpy(temp,X,len_X*sizeof(ord));
bnAdd(&temp_bn,&src2);
memcpy(Z,temp_bn.ptr,len_X*sizeof(ord));
/*bn package increments the size of dest by 1 if the carry bit is 1*/
free(temp);
if (temp_bn.size > len_X)
	return 1;
else
	return 0;
}



/****************************************************************************
*  NAME:  int Sub_big (ord *X,
*                      ord *Y,
*                      ord *Z,
*                      ushort len_X )
*
*
*  DESCRIPTION:  Compute subtraction.
*
*  INPUTS:
*     PARAMETERS:
*           ord   *X        Pointer to first array
*           ord   *Y        Pointer to second array
*           ushort   len_X     Number of longs in X_l
*  OUTPUT:
*     PARAMETERS:
*           ord  *Z         Pointer to result arrray
*
*     RETURN:
*            Carry bit
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

int Sub_big  (ord *X,
			  ord *Y,
			  ord *Z,
			  ushort len_X )
{
/* carry is not returned in bn version */
struct BigNum dest, src;
int status;
ord *temp;
bnInit();
bnBegin(&dest);
bnBegin(&src);

src.ptr = Y;
src.size = len_X;
src.allocated = len_X;

temp = (ord*)malloc(len_X*sizeof(ord));
dest.ptr = temp;
dest.size = len_X;
dest.allocated = len_X;
memcpy(dest.ptr,X,len_X*sizeof(ord));

status = bnSub(&dest,&src);
memcpy(Z,dest.ptr,len_X*sizeof(ord));
free(temp);
return status;
}

/****************************************************************************
*  NAME:   void  Mul_big( ord  *X, ord *Y, ord *XY,
*                         ushort lx, ushort ly)
*
*
*
*  DESCRIPTION:  Compute a product.
*
*  INPUTS:
*     PARAMETERS:
*            ord  *X                 Pointer to first long array
*            ord  *Y                 Pointer to second long array
*            ushort lx               Leftmost non zero element of first array
*            ushort ly               Leftmost non zero element of second array
*  OUTPUT:
*     PARAMETERS:
*            ord  *XY              Pointer to result
*
*     RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  08 Sep 95   AAB     Comment out calloc and discard the elements_in_X,
*                  elements_in_Y
****************************************************************************/
void  Mul_big( ord  *X, ord *Y, ord *XY,
						 ushort lx, ushort ly )
{
struct BigNum dest, src1, src2;
bnInit();
bnBegin(&dest);
bnBegin(&src1);
bnBegin(&src2);

src1.size = lx + 1;
src1.ptr = X;
src1.allocated = lx + 1;

src2.ptr = Y;
src2.size = ly + 1;
src2.allocated = ly + 1;

dest.ptr = XY;
dest.size = lx + ly + 2;
dest.allocated = lx + ly + 2;

/* Call bn routine */
bnMul(&dest, &src1,&src2);
}

/****************************************************************************
*  NAME:   void  Mul_big_1( ord  X, ord *Y, ord *XY,
*                                 ushort lx, ushort ly )
*
*
*
*  DESCRIPTION:  Compute a product.
*
*  INPUTS:
*     PARAMETERS:
*            ord  X                  Number
*            ord  *Y                 Pointer to long array
*            ushort ly               Leftmost non zero element of second array
*  OUTPUT:
*     PARAMETERS:
*            ord  *XY              Pointer to result
*
*     RETURN:
*
*
*  REVISION HISTORY:
*
*  08 Oct 95   AAB     Initial relaese
*
****************************************************************************/
void  Mul_big_1( ord  X, ord *Y, ord *XY,
				ushort ly )
{
struct BigNum dest, src;
bnInit();
bnBegin(&dest);
bnBegin(&src);

src.ptr = Y;
src.size = ly + 1;
src.allocated = ly + 1;

dest.ptr = XY;
dest.size = ly + 2;
dest.allocated = ly + 2;

bnMulQ(&dest, &src, (unsigned)X);

}

/****************************************************************************
*  NAME: int Mul( ushort X_bytes,
*                 ord        *X,
*                 ushort Y_bytes,
*                 ord       *Y,
*                 ushort P_bytes,
*                 ord   *P,
*                 ord   *Z )
*
*  DESCRIPTION:  Compute a modulo product
*
*  INPUTS:
*      PARAMETERS:
*            ord   *X           Pointer to first operand
*            ushort X_bytes     Number of bytes in X
*            ord   *Y           Pointer to second operand
*            ushort Y_bytes     Number of bytes in Y
*            ord   *P           Pointer to modulo
*            ushort P_bytes     Number of bytes in P
*
*  OUTPUT:
*      PARAMETERS:
*            ord   *Z           Pointer to result
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data (zero bytes)
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

int Mul( ushort X_bytes,
         ord *X,
		 ushort Y_bytes,
         ord *Y,
		 ushort P_bytes,
		 ord *P,
		 ord *Z )

{
	int status = SUCCESS;       /*function return status*/
	ushort X_longs;             /*number of longs in X*/
	ushort Y_longs;             /*number of longs in Y*/
	ord *XY;                    /*pointer to product (temporary)*/


struct BigNum dest, src1,src2, mod;
bnInit();
bnBegin(&dest);
bnBegin(&src1);
bnBegin(&src2);
bnBegin(&mod);


src1.size = X_bytes/sizeof(ord);
src1.ptr = X;
src1.allocated = X_bytes/sizeof(ord);

src2.size = Y_bytes/sizeof(ord);
src2.ptr = Y;
src2.allocated =Y_bytes/sizeof(ord);

mod.size = P_bytes/sizeof(ord);
mod.ptr = P;
mod.allocated = P_bytes/sizeof(ord);

    if ( P_bytes == 0 || X_bytes == 0 || Y_bytes == 0 )
	{
	    status = ERR_INPUT_LEN;
		return status;
	}
	if ( (X_bytes % sizeof(ord) != 0) ||
	     (Y_bytes % sizeof(ord) != 0) ||
		 (P_bytes % sizeof(ord) != 0) )
	{
	    status = ERR_INPUT_LEN;
		return status;
	}
	X_longs = X_bytes / sizeof(ord);
	Y_longs = Y_bytes / sizeof(ord);
	XY = (ord *)calloc( X_longs +  Y_longs, sizeof(ord) );
	if( !XY  )
	{
		return ERR_ALLOC;
	}
dest.size = X_longs + Y_longs;
dest.ptr = XY;
dest.allocated = X_longs + Y_longs;

bnMul (&dest,&src1,&src2);

status = bnMod(&dest, &dest, &mod);
memcpy(Z, dest.ptr, P_bytes);
free( XY );
	return status;
}

/****************************************************************************
*  NAME: int Square( ushort X_bytes,
*                         ord    *X,
*                     ushort P_bytes,
*                        ord    *P,
*                     ord   *Z )
*
*  DESCRIPTION:  Compute a modulo square
*
*  INPUTS:
*      PARAMETERS:
*            ord   *X           Pointer to array to be squared
*            ushort X_bytes     Number of bytes in X
*            ord   *P           Pointer to modulo
*            ushort P_bytes     Number of bytes in P
*
*  OUTPUT:
*      PARAMETERS:
*            ord   *Z           Pointer to result
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data (zero bytes)
*
*  REVISION HISTORY:
*
*  1  Sep 95   AAB     Initial release
****************************************************************************/

int Square( ushort X_bytes,
            ord *X,
            ushort P_bytes,
            ord *P,
            ord *Z )

{
    int status = SUCCESS;       /*function return status*/

ord *XY;
struct BigNum dest, src, mod;
bnInit();
bnBegin(&dest);
bnBegin(&src);
bnBegin(&mod);

	if ( P_bytes == 0 || X_bytes == 0 )
	{
	    status = ERR_INPUT_LEN;
		return status;
    }
	if ( (X_bytes % sizeof(ord) != 0) ||
	     (P_bytes % sizeof(ord) != 0) )
    {
	    status = ERR_INPUT_LEN;
		return status;
    }
	XY = (ord *)malloc( 2*X_bytes );
    if( !XY )
    {
	    return ERR_ALLOC;
    }

src.size = X_bytes/sizeof(ord);
src.ptr = X;
src.allocated = X_bytes/sizeof(ord);

dest.size = 2*X_bytes/sizeof(ord);
dest.ptr = XY;
dest.allocated = 2*X_bytes/sizeof(ord);

mod.size = P_bytes/sizeof(ord);
mod.ptr = P;
mod.allocated = P_bytes/sizeof(ord);

status = bnSquare(&dest, &src);
status = bnMod(&dest, &dest, &mod);
memcpy(Z, dest.ptr, P_bytes);
free(XY);
return status;
}


/****************************************************************************
*  NAME: int PartReduct( ushort X_bytes,
*                        ord  *X,
*                        ushort P_bytes,
*                        ord  *P,
*                        ord *Z )
*
*  DESCRIPTION:  Compute a modulo
*
*  INPUTS:
*      PARAMETERS:
*            ord   *X              Pointer to array
*            ushort X_bytes        Number of bytes in X
*            ord   *P              Pointer to modulo
*            ushort P_bytes        Number of bytes in P
*
*  OUTPUT:
*      PARAMETERS:
*            ord   *Z              Pointer to result
*
*      RETURN:
*          SUCCESS             No errors
*          ERR_INPUT_LEN       Invalid length for input data (zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

int PartReduct( ushort X_bytes,
	  ord *X,
	  ushort P_bytes,
	  ord   *P,
	  ord   *Z )
{
	 int status = SUCCESS;       /*function return status */


struct BigNum dest, src, d;
ord *temp;
bnInit();
bnBegin(&dest);
bnBegin(&src);
bnBegin(&d);

src.size = X_bytes/sizeof(ord);
src.ptr = X;
src.allocated = X_bytes/sizeof(ord);

d.size = P_bytes/sizeof(ord);
d.ptr = P;
d.allocated = P_bytes/sizeof(ord);

temp = (ord*)malloc(X_bytes);
dest.size = X_bytes/sizeof(ord);
dest.ptr = temp;
dest.allocated = X_bytes/sizeof(ord);
memcpy(dest.ptr, X, X_bytes);

status = bnMod(&dest, &dest, &d);

memcpy(Z, dest.ptr, P_bytes);
free(temp);

return status;

}

/****************************************************************************
*  NAME: int Expo( ushort X_bytes,
*                  ord    *X,
*                  ushort Y_bytes,
*                  ord    *Y,
*                  ushort P_bytes,
*                  ord    *P,
*                  ord    *Z,
*                  YIELD_context *yield_cont )
*
*  DESCRIPTION:  Compute a modulo exponent
*
*  INPUTS:
*      PARAMETERS:
*            ord   *X           Pointer to base array
*            ushort X_bytes     Number of bytes in base
*            ord   *Y           Pointer to exponent array
*            ushort Y_bytes     Number of bytes in exponent
*            ord   *P           Pointer to modulo
*            ushort P_bytes     Number of bytes in  P
*            YIELD_context *yield_cont  Pointer to yield_cont structure (NULL if not used)
*
*  OUTPUT:
*      PARAMETERS:
*            ord   *Z            Pointer to result
*
*  RETURN:
*          SUCCESS               No errors
*          ERR_INPUT_LEN         Invalid length for input data(zero bytes)
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  08 Dec 94   GKL     Added YIELD_context
*  01 Sep 95           Fast exponentation algorithm
****************************************************************************/

int Expo( ushort X_bytes, ord    *X,
	    ushort Y_bytes, ord    *Y,
	    ushort P_bytes, ord    *P,
	    ord   *Z, YIELD_context *yield_cont )
{

int status = SUCCESS;     /*function return status*/

struct BigNum dest, n, exp, mod;
bnInit();
bnBegin(&dest);
bnBegin(&n);
bnBegin(&exp);
bnBegin(&mod);

n.size = X_bytes/sizeof(ord);
n.ptr = X;
n.allocated = X_bytes/sizeof(ord);

exp.ptr = Y;
exp.size = Y_bytes/sizeof(ord);
exp.allocated = Y_bytes/sizeof(ord);

mod.ptr = P;
mod.size = P_bytes/sizeof(ord);
mod.allocated = P_bytes/sizeof(ord);

dest.ptr = Z;
dest.size = P_bytes/sizeof(ord);
dest.allocated = P_bytes/sizeof(ord);

/* Call bn routine */

status = bnExpMod(&dest, &n,
				  &exp, &mod);

return status;
}
/****************************************************************************
*  NAME: int Inverse( ushort X_bytes,
*                     ord    *X,
*                     ushort P_bytes,
*                     ord    *P,
*                     ord    *Z )
*
*
*
*
*  DESCRIPTION:  Compute a modulo inverse element
*
*  INPUTS:
*      PARAMETERS:
*            ord   *X           Pointer to array
*            ushort X_bytes     Number of bytes in array
*            ord   *P           Pointer to modulo
*            ushort P_bytes     Number of bytes in  P
*
*  OUTPUT:
*      PARAMETERS:
*            ord   *Z           Pointer to result
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_INPUT_VALUE  Invalid input value
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  08 Nov 94   GKL     Added input parameters check
*  01 Sep 95           Improve fuction
****************************************************************************/

int Inverse( ushort X_bytes,
           ord    *X,
           ushort P_bytes,
	       ord    *P,
       	   ord    *Z )
{
int status = SUCCESS;   /* function return status */

struct BigNum dest, src, mod;
bnInit();
bnBegin(&dest);
bnBegin(&src);
bnBegin(&mod);

src.size = X_bytes/sizeof(ord);
src.ptr = X;
src.allocated = X_bytes/sizeof(ord);

mod.ptr = P;
mod.size = P_bytes/sizeof(ord);
mod.allocated = P_bytes/sizeof(ord);

dest.ptr = Z;
dest.size = (P_bytes/sizeof(ord))  ;
dest.allocated = (P_bytes/sizeof(ord)) + 1;
status = bnInv(&dest,&src,&mod);
return status;
}


/****************************************************************************
*  NAME:     void Add( ord    *X,
*                      ord    *Y,
*                      ushort P_len,
*                      ord    *P,
*                      ord    *Z )

*
*  DESCRIPTION:  Compute modulo addition
*
*  INPUTS:
*          PARAMETERS:
*                         ord   *X              Pointer to first operand
*                         ord   *Y              Pointer to second operand
*                         ushort P_len  Length of modulo
*                         ord   *P              Pointer to modulo
*  OUTPUT:
*             ord   *Z          Pointer to result
*          RETURN:
*
*  REVISION HISTORY:
*
*  24 sep 94    KPZ             Initial release
*  10 Oct 94    KPZ     Fixed bugs
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

 int Add( ord *X,
		  ord *Y,
	  ushort P_len,
		  ord *P,
	      ord *Z )
{
	int status = SUCCESS;
	ord *temp;
	struct BigNum dest, src, mod;

bnInit();
bnBegin(&dest);
bnBegin(&src);
bnBegin(&mod);

temp = (ord*)malloc(P_len + sizeof(ord));
memcpy(temp, X, P_len);

dest.size = P_len/sizeof(ord);
dest.ptr = temp;
dest.allocated = P_len/sizeof(ord) + 1;

src.ptr = Y;
src.size = P_len/sizeof(ord);
src.allocated = P_len/sizeof(ord);

mod.ptr = P;
mod.size = P_len/sizeof(ord);
mod.allocated = P_len/sizeof(ord);

status = bnAdd(&dest,&src);
status = bnMod(&dest,&dest,&mod);
memcpy(Z,temp,P_len);
free(temp);
return status;
}


/****************************************************************************
*  NAME:     int SteinGCD( ord *m,
*                          ord *b
*                          ushort len )
*
*  DESCRIPTION:  Compute great common divisor
*
*  INPUTS:
*          PARAMETERS:
*           ord *m           Pointer to first number
*           ord *b           Pointer to second number
*           ushort len       Number of elements in number
*  OUTPUT:
*
*  RETURN:
*           TRUE                   if gcd != 1
*           FALSE                                  if gcd == 1
*  REVISION HISTORY:
*
*
*  24 Sep 94    KPZ     Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*  01 Sep 95    AAB     Speed up
*
****************************************************************************/


/* test if GCD equal 1 */
int  SteinGCD ( ord  *m,
		  ord  *n,
				ushort len )
{

int status;
struct BigNum dest, a, b;
ord *temp;
bnInit();
bnBegin(&dest);
bnBegin(&a);
bnBegin(&b);

a.size = len;
a.ptr = m;
a.allocated = len;

b.size = len;
b.ptr = n;
b.allocated = len;

temp = (ord*)malloc((len+1)*sizeof(ord));
dest.size = len;
dest.ptr = temp;
dest.allocated = len+1;

status = bnGcd(&dest, &a, &b);

if (*(ord *)(dest.ptr) == 0x01 && dest.size == 1)
 status = 0;
else
 status = 1;

free(temp);

return status;

}


/****************************************************************************
*  NAME: int DivRem( ushort X_bytes,
*                    ord    *X,
*                    ushort P_bytes,
*                    ord    *P,
*                    ord    *Z,
*                    ord    *D)
*
*  DESCRIPTION:  Compute a modulo and quotient
*
*  INPUTS:
*          PARAMETERS:
*                    ord   *X              Pointer to array
*                    ushort X_bytes        Number of bytes in X
*                    ord   *P              Pointer to modulo
*                    ushort P_bytes        Number of bytes in  P
*
*  OUTPUT:
*          PARAMETERS:
*            ord   *Z              Pointer to result
*            ord   *D                      Pointer to quotient
*          RETURN:
*                  SUCCESS             No errors
*          ERR_INPUT_LEN       Invalid length for input data (zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ              Initial release
*  10 Oct 94   KPZ      Fixed bugs
*  14 Oct 94   GKL      Second version (big endian support)
*
****************************************************************************/

int DivRem( ushort X_bytes,
		 ord    *X,
	  ushort P_bytes,
		 ord    *P,
	  ord    *Z,
	  ord    *D)
{
	int status = SUCCESS;       /* function return status */

struct BigNum q, r, n, d;
ord *temp;
bnInit();
bnBegin(&q);
bnBegin(&r);
bnBegin(&n);
bnBegin(&d);

n.size = X_bytes/sizeof(ord);
n.ptr = X;
n.allocated = X_bytes/sizeof(ord);

d.size = P_bytes/sizeof(ord);
d.ptr = P;
d.allocated = P_bytes/sizeof(ord);

q.size = (X_bytes/sizeof(ord)) - (P_bytes/sizeof(ord)) + 1;
q.ptr = D;
q.allocated = (X_bytes/sizeof(ord)) - (P_bytes/sizeof(ord)) + 1;

temp = (ord *)malloc(X_bytes);
r.size = X_bytes/sizeof(ord);
r.ptr = temp;
r.allocated = X_bytes/sizeof(ord);
memcpy(r.ptr, X, X_bytes);

status = bnDivMod(&q, &r, &r, &d);

memcpy(Z, r.ptr, P_bytes);
free(temp);

return status;

}
