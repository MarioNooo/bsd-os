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
*  FILENAME:  keyshare.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Functions File
*                   Digital Signature Algorithms
*  PUBLIC FUNCTIONS:
*                                   uchar  *RVAL )
*       int GenShamirTriplet( ushort Secretnumber_bytes, uchar *SecretNumber,
*                             uchar *first_value,       uchar *second_value,
*                             uchar *third_value,      uchar *RVAL )
*       int GetNumberShamirDuplex( ushort Secretnumber_bytes, uchar  *value_A,
*                                   ushort A_position,        uchar  *value_B,
*                                   ushort B_position,        uchar  *SecretNumber )
*
*   Copyright (c) Cylink Corporation 1996. All rights reserved.
*
*  REVISION  HISTORY:
*
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

#define  BEGIN_PROCESSING do {
#define  END_PROCESSING  } while (0);
#define  ERROR_BREAK break
#define  CONTINUE continue

#define  BEGIN_LOOP do {
#define  END_LOOP  } while (1);
#define  BREAK break

/****************************************************************************
*  NAME: int GenShamirTriplet( ushort Secretnumber_bytes,
*                              uchar *SecretNumber,
*                              uchar *first_value,
*                              uchar *second_value,
*                              uchar *third_value
*                              uchar *RVAL )
*
*  DESCRIPTION: Produce a Shamir Key-Sharing Triplet for Secret Number
*
*  INPUTS:
*      PARAMETERS:
*            ushort SecretNumber_bytes    Number of bytes in secret number
*            uchar  *SecretNumber         Pointer to secret number
*            uchar  *RVAL                 Pointer to random number generator
*                                          value
*  OUTPUT:
*      PARAMETERS:
*            uchar *first_value        Pointer to N+1 byte number
*            uchar *second_value       Pointer to N+1 byte number
*            uchar *third_value        Pointer to N+1 byte number
*            uchar *RVAL               Pointer to updated value
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_ALLOC            Insufficient memory
*  REVISION HISTORY:
*
*  10 Oct 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Sep 95   AAB     Mul_big is changed by the Mul_big_1
****************************************************************************/


int GenShamirTriplet( ushort SecretNumber_bytes,
			   uchar *SecretNumber,
		       uchar *first_value,
		       uchar *second_value,
		       uchar *third_value,
			   uchar *RVAL )

{
    int status = SUCCESS;           /* function return status */
    ord *temp;                    /* random number */
    ord *buf, *temp1, *temp2;     /* work buffers */
    ord *first_value_a;
    ord *second_value_a;
    ord *third_value_a;
    ord *RVAL_a;
    ord two = 2;
	ord three = 3;

	if ( SecretNumber_bytes == 0 )   /* invalid length for input data */
    {                                /* (zero bytes) */
	status = ERR_INPUT_LEN;
		return status;
	}
    if ( SecretNumber_bytes % sizeof(ord) != 0 )    /* not multiple 4 (32 bit) */
	{
	status = ERR_INPUT_LEN;          /* invalid length for input data */
	return status;
    }
    CALLOC(temp, ord, SecretNumber_bytes + sizeof(ord));
    CALLOC(temp1, ord, SecretNumber_bytes + sizeof(ord));
    CALLOC(temp2, ord, SecretNumber_bytes + sizeof(ord));
    CALLOC(buf, ord, SecretNumber_bytes + sizeof(ord));
    ALIGN_CALLOC_MOVE(RVAL, RVAL_a, SHA_LENGTH);
	ALIGN_CALLOC(first_value,first_value_a,SecretNumber_bytes + sizeof(ord));
    ALIGN_CALLOC(second_value,second_value_a,SecretNumber_bytes + sizeof(ord));
	ALIGN_CALLOC(third_value,third_value_a,SecretNumber_bytes + sizeof(ord));
    if ( status !=  SUCCESS )
    {
	if( buf )
		free( buf );
	if( temp )
		free( temp );
	if( temp1 )
	    free( temp1 );
	if ( temp2 )
	    free( temp2 );
	if( first_value_a )
	{
		 ALIGN_FREE(first_value_a);
	}
	if( second_value_a )
	{
		 ALIGN_FREE(second_value_a);
	}
	if( third_value_a )
	{
		 ALIGN_FREE(third_value_a);
	}
	if( RVAL_a )
	{
		 ALIGN_MOVE_FREE(RVAL_a,RVAL,SHA_LENGTH);
	}
	return status;     /* ERR_ALLOC   insufficient memory */
    }
    ByteOrd(SecretNumber,SecretNumber_bytes,buf);
    MyGenRand( SecretNumber_bytes, temp, RVAL_a );   /* generate random number */
/* Compute first_value = temp + buf */
	Sum_big( buf,temp,first_value_a,
	     SecretNumber_bytes / sizeof(ord) + 1);
/* temp1 = 2 * temp */
    Mul_big_1( two, temp, temp1, (ushort)(SecretNumber_bytes / sizeof(ord) - 1));
/* second_value = temp1 + buf */
	Sum_big( buf, temp1, second_value_a,
	     SecretNumber_bytes / sizeof(ord) + 1);
/* temp2 = 3 * temp1 */
    Mul_big_1( three, temp, temp2, (ushort)(SecretNumber_bytes / sizeof(ord) - 1));
/*third_value = temp2 + buf */
    Sum_big( buf, temp2, third_value_a,
		 SecretNumber_bytes /sizeof(ord) + 1);
    free( buf );
	free( temp );
    free( temp1 );
    free( temp2 );
    OrdByte(first_value_a, (ushort)(SecretNumber_bytes+1), first_value);
	OrdByte(second_value_a, (ushort)(SecretNumber_bytes+1) ,second_value);
    OrdByte(third_value_a, (ushort)(SecretNumber_bytes+1), third_value);
	ALIGN_FREE(first_value_a);
    ALIGN_FREE(second_value_a);
    ALIGN_FREE(third_value_a);
    ALIGN_MOVE_FREE(RVAL_a,RVAL,SHA_LENGTH);
    return status;
}


/****************************************************************************
*  NAME: int GetNumberShamirDuplex( ushort Secretnumber_bytes,
*                                   uchar  *value_A,
*                                   ushort A_posotion,
*                                   uchar  *value_B,
*                                   ushort B_position,
*                                   uchar  *SecretNumber )
*
*  DESCRIPTION: Reconstract a Secret Number from Shamir
*               Key-Sharing Duplex
*
*  INPUTS:
*      PARAMETERS:
*            ushort SecretNumber_bytes    Number of bytes in secret number
*            uchar  *value_A              Pointer to N+1-byte number
*            ushort A_position            Position in triplet (1,2 or3)
*            uchar  *value_B              Pointer to N+1-byte number
*            ushort B_position            Position in triplet (1,2 or3)
*  OUTPUT:
*      PARAMETERS:
*            uchar *SecretNumber           Pointer to SecretNumber number
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_POSITION         Invalid value of  triplet_position
*          ERR_ALLOC            Insufficient memory
*  REVISION HISTORY:
*
*  10 Oct 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Sep 95   AAB     Mul_big is changed by Mul_big_1
****************************************************************************/

 int GetNumberShamirDuplex( ushort SecretNumber_bytes,
							uchar  *value_A,
                            ushort A_position,
							uchar  *value_B,
                            ushort B_position,
                            uchar  *SecretNumber )
{
	int status = SUCCESS;           /* function return status */
    ord *temp, *temp1, *buf;
	ord *value_A_a;
    ord *value_B_a;
    ord two = 2;
	if ( SecretNumber_bytes == 0 )   /* invalid length for input data */
    {                                /* (zero bytes) */
        status = ERR_INPUT_LEN;
        return status;
    }
    if ( SecretNumber_bytes % sizeof(ord) != 0 )    /* not multiple 4 (32 bit) */
	{
        status = ERR_INPUT_LEN;          /* invalid length for input data */
		return status;
    }
    if ( (A_position > 3) || (B_position > 3 ) ||
         (A_position == B_position) )
	{
		status = ERR_POSITION;           /* invalid value of  triplet_position  */
		return status;
    }
    CALLOC(buf,ord,SecretNumber_bytes + sizeof(ord));
    CALLOC(temp,ord,SecretNumber_bytes + sizeof(ord));
    CALLOC(temp1,ord,SecretNumber_bytes + sizeof(ord));
    ALIGN_CALLOC_COPY(value_A, value_A_a, SecretNumber_bytes + sizeof(ord));
    ALIGN_CALLOC_COPY(value_B, value_B_a, SecretNumber_bytes + sizeof(ord));
    if ( status !=  SUCCESS )
    {
	if( value_A_a )
	{
		  ALIGN_FREE(value_A_a);
	}
	if(value_B_a)
	{
		  ALIGN_FREE(value_B_a);
	}
	if( temp1 )
	     free ( temp1 );
	if( temp )
	     free ( temp );
	if( buf )
	     free ( buf );
	return status;     /* ERR_ALLOC   insufficient memory */
    }
    if ( A_position > B_position )
	 {
        if ( A_position == 2 )
		{
            /* temp = value_A(D2) - value_B(D1) */
            Sub_big( value_A_a, value_B_a, temp,
                     SecretNumber_bytes / sizeof(ord) + 1);
			/* SecretNumber = value_B(D1) - temp */
            Sub_big( value_B_a, temp, buf,
						SecretNumber_bytes / sizeof(ord) + 1);
        }
        else
	{
            if ( B_position == 2 )
            {
                /* temp = value_A(D3) - value_B(D2) */
                Sub_big( value_A_a, value_B_a, temp,
                        SecretNumber_bytes / sizeof(ord) + 1 );
		/* temp = 2 * temp */
	       /*	Mul_big( temp, &two, temp1,
			 SecretNumber_bytes / sizeof(ord) - 1, 0,
			 SecretNumber_bytes / sizeof(ord) , 1 );*/
		 Mul_big_1( two, temp, temp1, (ushort)(SecretNumber_bytes / sizeof(ord) - 1));
		/* SecretNumber = value_B(D2) - temp */
		Sub_big( value_B_a, temp1, buf ,
			SecretNumber_bytes / sizeof(ord) + 1 );
		}
	    else
	    {
		/* temp = value_A(D3) - value_B(D1) */
		Sub_big( value_A_a, value_B_a, temp,
			SecretNumber_bytes / sizeof(ord) + 1 );
	       /* temp = temp / 2 */
		RShiftL_big( temp, (ushort)(SecretNumber_bytes / sizeof(ord) + 1), 1);
		/* SecretNumber = value_B(D1) - temp */
		Sub_big( value_B_a, temp, buf,
			 SecretNumber_bytes / sizeof(ord) + 1 );
		}
	}
     }
     else
	 {
	if ( B_position == 2 )
		{
	    /* temp = value_B(D2) - value_A(D1) */
	    Sub_big( value_B_a, value_A_a, temp,
		    SecretNumber_bytes / sizeof(ord) + 1 );
	    /* SecretNumber = value_B(D1) - temp */
	    Sub_big( value_A_a, temp, buf,
		    SecretNumber_bytes/ sizeof(ord) + 1 );
	}
	else
	{
	    if ( A_position == 2 )
		{
		/* temp = value_B(D3) - value_A(D2) */
		Sub_big( value_B_a, value_A_a, temp,
		    SecretNumber_bytes / sizeof(ord) + 1 );
		/* temp = 2 * temp */
	       /*	Mul_big( temp, &two, temp1,
			 SecretNumber_bytes / sizeof(ord) - 1, 0,
			 SecretNumber_bytes / sizeof(ord), 1 );*/
		 Mul_big_1( two, temp, temp1, (ushort)(SecretNumber_bytes / sizeof(ord) - 1));
		/* SecretNumber = value_A(D2) - temp */
		Sub_big( value_A_a, temp1, buf,
			SecretNumber_bytes / sizeof(ord) + 1 );
	    }
	    else
	    {
		/* temp = value_B(D3) - value_A(D1) */
		Sub_big( value_B_a, value_A_a, temp,
			SecretNumber_bytes / sizeof(ord) + 1 );
	       /* temp = temp / 2 */
		RShiftL_big( temp, (ushort)(SecretNumber_bytes / sizeof(ord) + 1), 1);
		/* SecretNumber = value_B(D1) - temp */
		Sub_big( value_A_a, temp, buf,
			 SecretNumber_bytes / sizeof(ord) + 1 );
		}
	}
     }
     OrdByte(buf, SecretNumber_bytes, SecretNumber);
     ALIGN_FREE(value_A_a);
     ALIGN_FREE(value_B_a);
     free ( temp1 );
     free ( temp );
     free ( buf );
	 return status;
}
