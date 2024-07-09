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
*  FILENAME:  cencrint.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:             Cryptographic Toolkit Internal Functions File
*
*  PRIVATE FUNCTIONS:
*
*
*               void shaTransform( ulong *state, uchar *block )
*               void SHAInitK( SHA_context *hash_context )
*               int MySHA( uchar *message, ushort message_bytes,
*                          uchar *hash_result )
*               int MySHAFinal( SHA_context *hash_context, uchar *hash_result )
*
*
*       Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
*
*  24 Sep 94   KPZ   Initial release
*  10 Oct 94   KPZ   Fixed bugs in Add(), DivRem()
*  12 Oct 94   KPZ   Modified shaTransform()
*  14 Oct 94   GKL   Second version (big endian support)
*  26 Oct 94   GKL   (alignment for big endian support & ERR_ALLOC)
*  08 Nov 94   GKL      Added input parameters check to Inverse
*  08 Dec 94   GKL   Added YIELD_context to Expo, VerPrime and GenPrime
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
/* #include <stdio.h> */
#endif

/* program files */
#include "toolkit.h"
#include "cencrint.h"
#include "c_asm.h"
#include "sha.h"

/* bignum include files */
#include "bn.h"
#include "bn32.h"


/****************************************************************************
*  NAME:  int SHA( uchar   *message,
*                  ushort message_bytes,
*                  uchar  *hash_result )
*
*  DESCRIPTION:  Compute a Secure Hash Function.
*
*  INPUTS:
*      PARAMETERS:
*         uchar *message          Pointer to message
*         ushort message_bytes    Number of bytes in message
*         uchar *hash_result      Pointer to message digest
*
*  OUTPUT:
*      PARAMETERS:
*         uchar *hash_result      Message digest
*
*      RETURN:
*          SUCCESS                 No errors
*          ERR_INPUT_LEN           Invalid length for input data(zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ     Initial release
*
****************************************************************************/

int SHA( uchar   *message,
         ushort message_bytes,
         uchar  *hash_result )
{
     SHA_context hash_context;     /* SHA context structure */
    int status = SUCCESS;         /* function return status */
    if (message_bytes == 0 )
    {
        status = ERR_INPUT_LEN;
        return status;            /* invalid length for input data */
    }
    SHAInit ( &hash_context );    /* initialize SHA */
    if ( (status = SHAUpdate( &hash_context, message, message_bytes ))
          != SUCCESS )
    {
       return status;            /* error */
    }
    if ((status=SHAFinal (&hash_context, hash_result)) != SUCCESS )
    {
       return status;           /* error */
    }
    return status;
}

/****************************************************************************
*  PRIVATE FUNCTIONS DEFINITIONS
****************************************************************************/


/****************************************************************************
*  NAME:     void shaTransform( ulong *state,
*                               uchar *block )
*
*  DESCRIPTION:  Perform  SHS transformation.
*
*  INPUTS:
*          PARAMETERS:
*           SHA_context *hash_context  Pointer to SHA_context structure
*  OUTPUT:
*
*           SHA_context *hash_context  Pointer to SHA_context structure
*                                      (updated)
*  REVISION HISTORY:
*
*  24 sep 94    KPZ     Initial release
*  12 Oct 94    KPZ     Modified buffers copy
*  14 Oct 94    GKL     Second version (big endian support)
*  1  Sep 95    AAB     Speedup the function
****************************************************************************/

 void shaTransform( ulong *state,
					uchar *block )
{
   ulong W[80];
   ulong A,B,C,D,E;  /*,temp;*/
   memcpy( W, block, 64);                 /*TKL00201*/
#ifdef L_ENDIAN                      /*TKL00201*/
      ByteSwap32_big( (uchar *)W, 64);        /*TKL00201*/
#endif                                    /*TKL00201*/
   /* Expand the 16 words into 80 words */
   expand(16);expand(17);expand(18);expand(19);expand(20);expand(21);
   expand(22);expand(23);expand(24);expand(25);expand(26);expand(27);
   expand(28);expand(29);expand(30);expand(31);expand(32);expand(33);
   expand(34);expand(35);expand(36);expand(37);expand(38);expand(39);
   expand(40);expand(41);expand(42);expand(43);expand(44);expand(45);
   expand(46);expand(47);expand(48);expand(49);expand(50);expand(51);
   expand(52);expand(53);expand(54);expand(55);expand(56);expand(57);
   expand(58);expand(59);expand(60);expand(61);expand(62);expand(63);
   expand(64);expand(65);expand(66);expand(67);expand(68);expand(69);
   expand(70);expand(71);expand(72);expand(73);expand(74);expand(75);
   expand(76);expand(77);expand(78);expand(79);
  /*Set up first buffer*/
         A = state[0];
   B = state[1];
   C = state[2];
   D = state[3];
   E = state[4];

 /* Heavy mangling, in 4 sub-rounds of 20 iterations each. */
    subRound( A, B, C, D, E, f1, k1SHA, W[ 0] );
    subRound( E, A, B, C, D, f1, k1SHA, W[ 1] );
    subRound( D, E, A, B, C, f1, k1SHA, W[ 2] );
    subRound( C, D, E, A, B, f1, k1SHA, W[ 3] );
    subRound( B, C, D, E, A, f1, k1SHA, W[ 4] );
	subRound( A, B, C, D, E, f1, k1SHA, W[ 5] );
    subRound( E, A, B, C, D, f1, k1SHA, W[ 6] );
	subRound( D, E, A, B, C, f1, k1SHA, W[ 7] );
    subRound( C, D, E, A, B, f1, k1SHA, W[ 8] );
    subRound( B, C, D, E, A, f1, k1SHA, W[ 9] );
    subRound( A, B, C, D, E, f1, k1SHA, W[10] );
    subRound( E, A, B, C, D, f1, k1SHA, W[11] );
    subRound( D, E, A, B, C, f1, k1SHA, W[12] );
    subRound( C, D, E, A, B, f1, k1SHA, W[13] );
    subRound( B, C, D, E, A, f1, k1SHA, W[14] );
    subRound( A, B, C, D, E, f1, k1SHA, W[15] );
	subRound( E, A, B, C, D, f1, k1SHA, W[16] );
    subRound( D, E, A, B, C, f1, k1SHA, W[17] );
	subRound( C, D, E, A, B, f1, k1SHA, W[18] );
    subRound( B, C, D, E, A, f1, k1SHA, W[19] );

    subRound( A, B, C, D, E, f2, k2SHA, W[20]);
     subRound( E, A, B, C, D, f2, k2SHA, W[21]);
     subRound( D, E, A, B, C, f2, k2SHA, W[22]);
     subRound( C, D, E, A, B, f2, k2SHA, W[23]);
     subRound( B, C, D, E, A, f2, k2SHA, W[24]);
     subRound( A, B, C, D, E, f2, k2SHA, W[25]);
	 subRound( E, A, B, C, D, f2, k2SHA, W[26]);
     subRound( D, E, A, B, C, f2, k2SHA, W[27]);
	 subRound( C, D, E, A, B, f2, k2SHA, W[28]);
     subRound( B, C, D, E, A, f2, k2SHA, W[29]);
     subRound( A, B, C, D, E, f2, k2SHA, W[30]);
     subRound( E, A, B, C, D, f2, k2SHA, W[31]);
     subRound( D, E, A, B, C, f2, k2SHA, W[32]);
     subRound( C, D, E, A, B, f2, k2SHA, W[33]);
     subRound( B, C, D, E, A, f2, k2SHA, W[34]);
     subRound( A, B, C, D, E, f2, k2SHA, W[35]);
     subRound( E, A, B, C, D, f2, k2SHA, W[36]);
	 subRound( D, E, A, B, C, f2, k2SHA, W[37]);
     subRound( C, D, E, A, B, f2, k2SHA, W[38]);
	 subRound( B, C, D, E, A, f2, k2SHA, W[39]);

     subRound( A, B, C, D, E, f3, k3SHA, W[40]);
     subRound( E, A, B, C, D, f3, k3SHA, W[41]);
     subRound( D, E, A, B, C, f3, k3SHA, W[42]);
     subRound( C, D, E, A, B, f3, k3SHA, W[43]);
     subRound( B, C, D, E, A, f3, k3SHA, W[44]);
     subRound( A, B, C, D, E, f3, k3SHA, W[45]);
     subRound( E, A, B, C, D, f3, k3SHA, W[46]);
	 subRound( D, E, A, B, C, f3, k3SHA, W[47]);
     subRound( C, D, E, A, B, f3, k3SHA, W[48]);
	 subRound( B, C, D, E, A, f3, k3SHA, W[49]);
     subRound( A, B, C, D, E, f3, k3SHA, W[50]);
     subRound( E, A, B, C, D, f3, k3SHA, W[51]);
     subRound( D, E, A, B, C, f3, k3SHA, W[52]);
     subRound( C, D, E, A, B, f3, k3SHA, W[53]);
     subRound( B, C, D, E, A, f3, k3SHA, W[54]);
     subRound( A, B, C, D, E, f3, k3SHA, W[55]);
     subRound( E, A, B, C, D, f3, k3SHA, W[56]);
     subRound( D, E, A, B, C, f3, k3SHA, W[57]);
	 subRound( C, D, E, A, B, f3, k3SHA, W[58]);
     subRound( B, C, D, E, A, f3, k3SHA, W[59]);

     subRound( A, B, C, D, E, f4, k4SHA, W[60]);
     subRound( E, A, B, C, D, f4, k4SHA, W[61]);
     subRound( D, E, A, B, C, f4, k4SHA, W[62]);
     subRound( C, D, E, A, B, f4, k4SHA, W[63]);
     subRound( B, C, D, E, A, f4, k4SHA, W[64]);
     subRound( A, B, C, D, E, f4, k4SHA, W[65]);
     subRound( E, A, B, C, D, f4, k4SHA, W[66]);
     subRound( D, E, A, B, C, f4, k4SHA, W[67]);
	 subRound( C, D, E, A, B, f4, k4SHA, W[68]);
     subRound( B, C, D, E, A, f4, k4SHA, W[69]);
	 subRound( A, B, C, D, E, f4, k4SHA, W[70]);
     subRound( E, A, B, C, D, f4, k4SHA, W[71]);
     subRound( D, E, A, B, C, f4, k4SHA, W[72]);
     subRound( C, D, E, A, B, f4, k4SHA, W[73]);
     subRound( B, C, D, E, A, f4, k4SHA, W[74]);
     subRound( A, B, C, D, E, f4, k4SHA, W[75]);
     subRound( E, A, B, C, D, f4, k4SHA, W[76]);
     subRound( D, E, A, B, C, f4, k4SHA, W[77]);
     subRound( C, D, E, A, B, f4, k4SHA, W[78]);
	 subRound( B, C, D, E, A, f4, k4SHA, W[79]);

	 state[0] += A;
  state[1] += B;
  state[2] += C;
  state[3] += D;
  state[4] += E;

}




/****************************************************************************
*  NAME:  void SHAInitK( SHA_context *hash_context )
*
*  DESCRIPTION: Initialize Secure Hash Function for generate
*               random number for DSS.
*
*  INPUTS:
*          PARAMETERS:
*              SHA_context *hash_context   SHA context structure
*  OUTPUT:
*          PARAMETERS:
*             SHA_context *hash_context    Initialized SHA context structure
*
*          RETURN:
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void SHAInitK( SHA_context *hash_context )
{
	ushort i;                        /*counter*/
/*Set up first buffer*/
    hash_context->state[0] = 0xEFCDAB89L;
   hash_context->state[1] = 0x98BADCFEL;
   hash_context->state[2] = 0x10325476L;
   hash_context->state[3] = 0xC3D2E1F0L;
   hash_context->state[4] = 0x67452301L;
/*Initialise buffer */
    for ( i = 0; i < SHS_BLOCKSIZE / sizeof(ulong); i++ )
    {
        hash_context->buffer[i] = 0;
	}
/*Initialise bit count*/
      for ( i = 0; i < 2; i++ )
    {
		hash_context->count[i] = 0;
    }

}


/****************************************************************************
*  NAME:  int MySHA( uchar   *message,
*                    ushort message_bytes,
*                    uchar  *hash_result )
*
*  DESCRIPTION:  Compute a Secure Hash Function.
*
*  INPUTS:
*          PARAMETERS:
*                 uchar *message          Pointer to message
*                 ushort message_bytes    Number of bytes in message
*         uchar *hash_result      Pointer to message digest
*
*  OUTPUT:
*          PARAMETERS:
*         uchar *hash_result      Message digest
*
*          RETURN:
*                  SUCCESS                 No errors
*          ERR_INPUT_LEN           Invalid length for input data(zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*
****************************************************************************/
int MySHA( uchar   *message,
           ushort message_bytes,
      uchar  *hash_result )
{
       SHA_context hash_context;     /* SHA context structure */
       int status = SUCCESS;         /* function return status */
	  if (message_bytes == 0 )
        {
               status = ERR_INPUT_LEN;
         return status;            /* invalid length for input data */
   }
       SHAInit ( &hash_context );    /* initialize SHA */
#ifdef B_ENDIAN
    ByteSwap(message,message_bytes);
#endif
    status = SHAUpdate( &hash_context, message, message_bytes );
#ifdef B_ENDIAN
	ByteSwap(message,message_bytes);
#endif
    if ( status != SUCCESS )
    {
			   return status;            /* error */
   }
    if ((status=MySHAFinal (&hash_context, hash_result)) != SUCCESS )
  {
               return status;           /* error */
    }
       return status;
}

/****************************************************************************
*  NAME:  int MySHAFinal( SHA_context *hash_context,
*                         uchar       *hash_result )
*  DESCRIPTION:  Finalize Secure Hash Function
*
*  INPUTS:
*          PARAMETERS:
*              SHA_context *hash_context    SHA context structure
*                uchar *hash_result     Pointer to hash
*  OUTPUT:
*          PARAMETERS:
*              uchar *hash_result        Final value
*          RETURN:
*                  SUCCESS               No errors
*          ERR_INPUT_LEN         Invalid length for input data (zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ         Initial release
*  10 Oct 94   KPZ     Modified for arbitrary message length
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/
  int MySHAFinal( SHA_context *hash_context,
             uchar       *hash_result )
{
   int status = SUCCESS;         /* function return status */
      uchar bits[8];
  ushort index, padLen;
   ulong ex;
       uchar PADDING[64] = {         /* padding string */
              0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
 };

	  if ( hash_context->count[0] == 0 && hash_context->count[1] == 0 )
       {
          status= ERR_INPUT_LEN;
          return status;
       }
 /* Save number of bits */
     LongByte( &hash_context->count[1] , 4, bits );
  LongByte( &hash_context->count[0] , 4, bits + 4 );
      ByteSwap32_big( bits, 8 );
 /* Pad out to 56 mod 64.*/
   index = (ushort )((hash_context->count[0] >> 3) & 0x3f);
		padLen = (index < 56) ? (56 - index) : (120 - index);
   SHAUpdate( hash_context, PADDING, padLen );

 /* Append length (before padding) */
    SHAUpdate (hash_context, bits, 8);

 /* Set order of hash_context */
	ex = hash_context->state[0];
  hash_context->state[0] = hash_context->state[4];
    hash_context->state[4] = ex;
    ex = hash_context->state[1];
	hash_context->state[1] = hash_context->state[3];
    hash_context->state[3] = ex;
  /* Store state in digest */
    memcpy(hash_result,hash_context->state,SHA_LENGTH);
  /* Zeroize sensitive information.*/
    memset( hash_context, 0, sizeof(hash_context) );
#if defined ( ORD_16 )  && defined( B_ENDIAN )
	WordSwap(hash_result,SHA_LENGTH);
#endif
    return status;
}


/****************************************************************************
*  NAME:  int SHAUpdate( SHA_context *hash_context,
*                        uchar        *message,
*                        ushort      message_bytes )
*  DESCRIPTION:  Update Secure Hash Function
*
*  INPUTS:
*      PARAMETERS:
*          SHA_context *hash_context        SHA context structure
*          uchar       *message             Pointer to message
*          ushort       message_bytes       Number of bytes
*  OUTPUT:
*      PARAMETERS:
*          SHA_context  *hash_context       Updated SHA context structure
*
*      RETURN:
*          SUCCESS          No errors
*          ERR_INPUT_LEN    Invalid length for input data (zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ      Initial release
*  10 Oct 94   KPZ      Modified for arbitrary message length
*
****************************************************************************/

int SHAUpdate( SHA_context *hash_context,
            uchar        *message,
          ushort      message_bytes )

{
    int status = SUCCESS;         /* function return status */
    ushort i, index, partLen;
    if ( message_bytes == 0 )
    {
        status = ERR_INPUT_LEN;   /*invalid length for input data (zero bytes)*/
        return status;
    }

  /* Compute number of bytes mod 64 */
    index = (ushort)((hash_context->count[0] >> 3) & 0x3F);

  /* Update number of bits */
    if ( (hash_context->count[0] += ((ulong )message_bytes << 3))
              < ((ulong )message_bytes << 3) )
    {
   hash_context->count[1]++;
    }
    hash_context->count[1] += ((ulong )message_bytes >> 29);

    partLen = 64 - index;
  /* Transform as many times as possible.*/
    if ( message_bytes >= partLen )
    {
  memcpy( &hash_context->buffer[index], message, partLen );
       shaTransform( hash_context->state, hash_context->buffer );

      for ( i = partLen; (ushort)(i + 63) < message_bytes; i += 64 )
  {
           shaTransform ( hash_context->state, &message[i] );
  }
       index = 0;
    }
    else
    {
     i = 0;
    }
  /* Buffer remaining input */
    memcpy( &hash_context->buffer[index], &message[i],
             message_bytes - i );
    return status;
}


/****************************************************************************
*  NAME:  void SHAInit( SHA_context *hash_context )
*
*  DESCRIPTION:  Initialize Secure Hash Function
*
*  INPUTS:
*      PARAMETERS:
*              SHA_context *hash_context   SHA context structure
*  OUTPUT:
*      PARAMETERS:
*             SHA_context *hash_context    Initialized SHA context structure
*
*      RETURN:
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ     Initial release
*
****************************************************************************/

void SHAInit( SHA_context *hash_context )
{
    ushort i;                        /*counter*/
/*Set up first buffer*/
    hash_context->state[0] = h0SHA;
    hash_context->state[1] = h1SHA;
    hash_context->state[2] = h2SHA;                  
    hash_context->state[3] = h3SHA;
    hash_context->state[4] = h4SHA;
/* Initialise buffer */
    for ( i = 0; i < SHS_BLOCKSIZE / sizeof(ulong); i++ )
    {
        hash_context->buffer[i] = 0;
    }
 /*Initialize bit count*/
    hash_context->count[0] = hash_context->count[1] = 0;
}

/****************************************************************************
*  NAME:  int SHAFinal( SHA_context *hash_context,
*                       uchar       *hash_result )
*  DESCRIPTION:  Finalize Secure Hash Function
*
*  INPUTS:
*      PARAMETERS:
*          SHA_context *hash_context    SHA context structure
*                uchar *hash_result     Pointer to hash
*  OUTPUT:
*      PARAMETERS:
*              uchar *hash_result        Final value
*      RETURN:
*          SUCCESS               No errors
*          ERR_INPUT_LEN         Invalid length for input data (zero bytes)
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  10 Oct 94   KPZ     Modified for arbitrary message length
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/


  int SHAFinal( SHA_context *hash_context,
                                uchar       *hash_result )
{
  int status = SUCCESS;         /* function return status */
  status =  MySHAFinal( hash_context, hash_result );
#ifdef B_ENDIAN
  if (status == SUCCESS)
    {
          ByteSwap(hash_result, SHA_LENGTH);
   }
#endif
  return status;
}
