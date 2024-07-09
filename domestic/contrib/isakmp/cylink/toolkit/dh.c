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
*  FILENAME:  dh.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Functions File
*                   Diffe-Hellman Functions
*  PUBLIC FUNCTIONS:
*
*      int GetDHSharedNumber( ushort DH_modulus_bytes, uchar  *DH_secret,
*                             uchar  *DH_public,       uchar  *DH_shared,
*                                                      uchar  *DH_modulus )
*                                           uchar  *RVAL )
*      int GenDHPair( ushort DH_modulus_bytes, uchar  *DH_secret,
*                      uchar  *DH_public,       uchar  *DH_base,
*                      uchar  *DH_modulus,      uchar  *RVAL );
*                               uchar  *DH_shared )
*      int SFDHEncrypt( ushort DH_modulus_bytes,
*                       uchar  *DH_modulus, uchar  *DH_base,
*                       uchar  *DH_public, uchar  *DH_random_public,
*                       uchar  *DH_shared, uchar  *RVAL )
*      int SFDHDecrypt( ushort DH_modulus_bytes,
*                       uchar  *DH_modulus,
*                       uchar  *DH_secret, uchar  *DH_random_public,
*                       uchar  *DH_shared )
*
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
/*#define  INITIALIZ_PQG */  /*TKL01101*/
#include "dssnum.h"      /*TKL01101*/
#include <stdlib.h>

#define  BEGIN_PROCESSING do {
#define  END_PROCESSING  } while (0);
#define  ERROR_BREAK break
#define  CONTINUE continue

#define  BEGIN_LOOP do {
#define  END_LOOP  } while (1);
#define  BREAK break

/****************************************************************************
*  PUBLIC FUNCTIONS DEFINITIONS
****************************************************************************/

/****************************************************************************
*  NAME:  int GetDHSharedNumber( ushort DH_modulus_bytes,
*                               uchar  *DH_secret,
*                               uchar  *DH_public,
*                               uchar  *DH_shared,
*                               uchar  *DH_modulus )
*
*  DESCRIPTION:  Compute a Diffie-Hellman Shared number
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_modulus_bytes   Length of DH_modulus
*          uchar *DH_secret          Pointer to secret number
*          uchar *DH_public          Pointer to public number
*          uchar *DH_modulus         Pointer to modulus
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *DH_shared          Pointer to shared number
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DH_LEN                Invalid length for DH_modulus
*          ERR_ALLOC             Insufficient memory
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL     Changed Expo call
*
****************************************************************************/

int GetDHSharedNumber( ushort DH_modulus_bytes,
                      uchar  *DH_secret,
                      uchar  *DH_public,
                      uchar  *DH_shared,
                      uchar  *DH_modulus )
{
    int status = SUCCESS;          /* function return status */
    ord  *DH_secret_a;
    ord  *DH_public_a;
    ord  *DH_modulus_a;
    ord  *DH_shared_a;
    if ( DH_modulus_bytes < DH_LENGTH_MIN )       /* less than minimal */
    {
        status = ERR_DH_LEN;         /* invalid length for DH_modulus */
        return status;
    }
    if ( DH_modulus_bytes & 0x0003 )   /* not multiple 4(32-bit)*/
    {
        status = ERR_DH_LEN;        /* invalid length for DH_modulus */
        return status;
    }
    ALIGN_CALLOC_COPY(DH_public, DH_public_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_secret, DH_secret_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
    ALIGN_CALLOC(DH_shared, DH_shared_a, DH_modulus_bytes);
    if ( status !=  SUCCESS )
    {
        if( DH_public_a )
        {
            ALIGN_FREE(DH_public_a);
        }
        if(DH_secret_a )
        {
            memset(DH_secret_a, 0, DH_modulus_bytes);
            ALIGN_FREE(DH_secret_a);
        }
        if( DH_modulus_a)
        {
            ALIGN_FREE(DH_modulus_a);
        }
        if( DH_shared_a )
        {
            ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
        }
        return status;     /* ERR_ALLOC   insufficient memory */
    }
/* Compute Diffie-Hellman Shared Number */
    status = Expo ( DH_modulus_bytes,  /* DH_shared=DH_public^ */
                    DH_public_a,         /* DH_secret mod(DH_modulus)*/
                    DH_modulus_bytes,
                    DH_secret_a,
                    DH_modulus_bytes,
                    DH_modulus_a,
                    DH_shared_a, NULL );  /*TKL00601*/
    ALIGN_FREE(DH_public_a);
    ALIGN_FREE(DH_secret_a);
    ALIGN_FREE(DH_modulus_a);
    ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
    return status;
}

/****************************************************************************
*  NAME:  int DHOneTimePad( ushort DH_modulus_bytes,
*                           uchar  *DH_shared
*                           uchar  *X,
*                           uchar  *Y )
*
*  DESCRIPTION:  One-Time-Pad Signature with a Diffie-Hellman shared number
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_modulus_bytes   Length of DH_modulus
*          uchar *X                  Pointer to input number
*
*  OUTPUT:
*      PARAMETERS:
*              uchar *Y              Pointer to output number
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DH_LEN                Invalid length for DH_modulus
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*
****************************************************************************/


  int DHOneTimePad( ushort DH_modulus_bytes,
                    uchar  *DH_shared,
                    uchar  *X,
                    uchar  *Y )
{
    int status = SUCCESS;          /* function return status */
	   ushort i;                          /*counter*/
    if ( DH_modulus_bytes < DH_LENGTH_MIN )           /* less than minimal */
    {
            status = ERR_DH_LEN;           /* invalid length for DH_modulus */
            return status;
    }
    if ( DH_modulus_bytes & 0x0003 )   /* not multiple 4(32-bit)*/
    {
            status = ERR_DH_LEN;        /* invalid length for DH_modulus */
			return status;
    }
	for ( i = 0; i < DH_modulus_bytes; i++)
    {
          Y[i] = DH_shared[i] ^ X[i];
    }
    return status;
}


/****************************************************************************
*  NAME:  int GetDHKey( ushort DH_modulus_bytes,
*                       uchar *DH_secret,
*                       uchar *DH_public,
*                       ushort key1_bytes,
*                       uchar *key1,
*                       ushort key2_bytes
*                       uchar *key2,
*                       uchar *DH_modulus )
*
*  DESCRIPTION:  Compute Diffie-Hellman keys
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_modulus_bytes   Length of DH_modulus
*          uchar *DH_secret          Pointer to secret number
*          uchar *DH_public          Pointer to public number
*          ushort key1               Length of key1
*          ushort key2               Length of key2
*          uchar *DH_modulus         Pointer to DH modulus
*  OUTPUT:
*      PARAMETERS:
*              uchar *key1            Pointer to key1
*              uchar *key2            Pointer to key2
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DH_LEN                Invalid length for DH_modulus
*          ERR_ALLOC                 Insufficient memory
*          ERR_NUMBER                Invalid length of key1 or key2
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  25 Mar 95   JLC     Fixed bugs in use of temp (modified ptr free()'d &...
*                      ...search for key2 now starts at mid buffer).
*  24 Jan 96   KPZ     Added semiweak keys check
****************************************************************************/

 int GetDHKey( ushort DH_modulus_bytes,
        uchar *DH_secret,
        uchar *DH_public,
        ushort key1_bytes,
        uchar *key1,
        ushort key2_bytes,
        uchar *key2,
        uchar *DH_modulus )
{
	int status = SUCCESS;          /* function return status */
    ord *DH_secret_a;
    ord *DH_public_a;
    ord *DH_modulus_a;
    uchar *temp,*save;             /* work buffer */
    ushort j;                   /* counter */

    if ( DH_modulus_bytes < DH_LENGTH_MIN )           /* less than minimal */
    {
        status = ERR_DH_LEN;           /* invalid length for DH_modulus */
        return status;
    }
    if ( DH_modulus_bytes & 0x0003 )   /* not multiple 4(32-bit)*/
    {
        status = ERR_DH_LEN;       /* invalid length for DH_modulus */
        return status;
    }
    if ( key1_bytes > (ushort)(DH_modulus_bytes / 2) )
    {
        status = ERR_NUMBER;
        return status;
    }
    if ( key2_bytes > (ushort)(DH_modulus_bytes / 2) )
    {
        status = ERR_NUMBER;
        return status;
    }
    CALLOC(temp,uchar,DH_modulus_bytes);
	ALIGN_CALLOC_COPY(DH_public, DH_public_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_secret, DH_secret_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
    if ( status !=  SUCCESS )
    {
        return status;     /* ERR_ALLOC   insufficient memory */
    }
    status = Expo ( DH_modulus_bytes,  /* temp=DH_public^ */
          DH_public_a,           /* DH_secret mod(DH_modulus)*/
          DH_modulus_bytes,
          DH_secret_a,
          DH_modulus_bytes,
          DH_modulus_a,
          (ord *)temp, NULL );
    ALIGN_FREE(DH_public_a);
	ALIGN_FREE(DH_secret_a);
    ALIGN_FREE(DH_modulus_a);
	if (status != SUCCESS)
    {
        free (temp);
        return status;                                   /* error */
    }
#ifdef B_ENDIAN
	ByteSwap(temp, DH_modulus_bytes );
#endif                                                                          /* <<<<< FIXED HERE >>>>> */
    save = temp;                                                    /* save ptr in case modified */
    if ( key1_bytes == 8 )
    {
        for ( j = 0; j <= (ushort)(DH_modulus_bytes / 2 - 8); j++ )
        {
            if ( (status = CheckDESKeyWeakness( temp )) == SUCCESS)   /* check key weakness*/
            {
			    memcpy( key1, temp, key1_bytes );
                break;
            }
            temp++;
        }
        if (status != SUCCESS)
        {
            free (save);
			return status;         /* weak keys */
        }
    }
    else
    {
        memcpy( key1, temp, key1_bytes);
    }
    if ( key2_bytes == 8 )
    {
        temp = save + DH_modulus_bytes / 2;   /* <<<< FIXED HERE >>>>> */

        for ( j = 0; j <= (ushort)(DH_modulus_bytes / 2 - 8); j++ )
        {
            if ( (status = CheckDESKeyWeakness( temp )) == SUCCESS)   /* check key weakness*/
            {
                memcpy( key2, temp, key2_bytes );
                break;
            }
            temp ++;
        }
        if (status != SUCCESS)
        {
            free (save);
            return status;         /* weak keys */
        }
    }
    else
    {
        memcpy( key2, save + DH_modulus_bytes / 2, key2_bytes);
    }
    free(save);
    return status;
}


/****************************************************************************
*  NAME: int GenDHPair( ushort DH_modulus_bytes,
*                       uchar  *DH_secret,
*                       uchar  *DH_public,
*                       uchar  *DH_base,
*                       uchar  *DH_modulus,
*                       uchar  *RVAL )
*
*  DESCRIPTION: Compute a Diffie-Hellman Pair
*
*  INPUTS:
*      PARAMETERS:
*            ushort DH_modulus_bytes    Number of bytes in DH_modulue array
*            uchar *DH_modulus          Pointer to Diffie-Hellmanr modulus
*            uchar *DH_base             Pointer to Diffie-Hellmanr  base
*            uchar *RVAL                Pointer to random number generator value
*  OUTPUT:
*      PARAMETERS:
*            uchar *DH_secret           Pointer to secret number
*            uchar *DH_public           Pointer to public bytes
*            uchar *RVAL                Pointer to updated random
*                                        number generator value
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_DH_LEN           Invalid length for DH_modulus
*          ERR_ALLOC            Insufficient memory
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL     Changed Expo call
*
****************************************************************************/

int GenDHPair( ushort DH_modulus_bytes,
                          uchar  *DH_secret,
                      uchar  *DH_public,
                      uchar  *DH_base,
                        uchar  *DH_modulus,
                     uchar  *RVAL )
{
    int status = SUCCESS;      /* function return status */
	ord *DH_secret_a;
    ord *DH_public_a;
    ord *DH_base_a;
    ord *DH_modulus_a;
    ord *RVAL_a;
    if ( DH_modulus_bytes < DH_LENGTH_MIN )       /* less than minimal */
	{
      status = ERR_DH_LEN;         /* invalid length for DH_modulus */
        return status;
    }
    if ( DH_modulus_bytes % sizeof(ord) != 0 )    /* not multiple 4 (32 bit) */
    {
     status = ERR_INPUT_LEN;          /* invalid length for input data */
    return status;
    }
	ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_base, DH_base_a, DH_modulus_bytes);
	ALIGN_CALLOC_MOVE(RVAL, RVAL_a, SHA_LENGTH);
    ALIGN_CALLOC(DH_secret, DH_secret_a, DH_modulus_bytes);
    ALIGN_CALLOC(DH_public, DH_public_a, DH_modulus_bytes);
    if ( status !=  SUCCESS )
    {
        if( DH_modulus_a )
	    {
            ALIGN_FREE(DH_modulus_a);
        }
        if( DH_base_a )
 	    {
            ALIGN_FREE(DH_base_a);
  	    }
        if( DH_secret_a )
        {
            memset(DH_secret_a, 0, DH_modulus_bytes);
            ALIGN_COPY_FREE(DH_secret_a, DH_secret, DH_modulus_bytes);
	    }
        if( DH_public_a)
        {
            ALIGN_COPY_FREE(DH_public_a, DH_public, DH_modulus_bytes);
        }
        if( RVAL_a )
		{
            ALIGN_MOVE_FREE(RVAL_a, RVAL, SHA_LENGTH);
        }
        return status;     /* ERR_ALLOC   insufficient memory */
    }

    MyGenRand( DH_modulus_bytes, DH_secret_a, RVAL_a);

      /* DH_public = DH_base^DH_secret modDH_modulus */
	status = Expo( DH_modulus_bytes, DH_base_a, DH_modulus_bytes, DH_secret_a,
            DH_modulus_bytes, DH_modulus_a, DH_public_a, NULL);

    ALIGN_FREE(DH_modulus_a);
    ALIGN_FREE(DH_base_a);
    ALIGN_COPY_FREE(DH_secret_a, DH_secret, DH_modulus_bytes);
    ALIGN_COPY_FREE(DH_public_a, DH_public, DH_modulus_bytes);
    ALIGN_MOVE_FREE(RVAL_a, RVAL, SHA_LENGTH);
    return status;
}

/****************************************************************************
*  NAME:  int SFDHEncrypt( ushort DH_modulus_bytes,
*                               uchar  *DH_modulus, uchar  *DH_base,
*                               uchar  *DH_public, uchar  *DH_random_public,
*                               uchar  *DH_shared, uchar  *RVAL )
*
*  DESCRIPTION:  Encrypt secret a Diffie-Hellman Shared number
*                                            with Store and Forward form
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_modulus_bytes   Length of DH_modulus
*          uchar *DH_modulus         Pointer to modulus (dss_p)
*          uchar *DH_base            Pointer to base (dss_g)
*          uchar *DH_public          Pointer to recipient's public number (dss_y)
*          uchar *RVAL               Pointer to random number generator value
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *DH_random_public   Pointer to random public number
*          uchar *DH_shared          Pointer to secret shared number
*          uchar *RVAL               Pointer to updated value
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DH_LEN                Invalid length for DH_modulus
*          ERR_ALLOC             Insufficient memory
*
*  REVISION HISTORY:
*
*  07 Jul 95   KPZ     Initial release
*
****************************************************************************/

int SFDHEncrypt( ushort DH_modulus_bytes,
                uchar  *DH_modulus,
                uchar  *DH_base,
                uchar  *DH_public,
	            uchar  *DH_random_public,
                             uchar  *DH_shared,
									  uchar  *RVAL )
{
    int status = SUCCESS;          /* function return status */
    ord  *DH_modulus_a;
    ord  *DH_base_a;
    ord  *DH_public_a;
    ord  *DH_random_public_a;
    ord  *DH_shared_a;
    ord  *RVAL_a;
    ord  *random_secret;

    if ( DH_modulus_bytes < DH_LENGTH_MIN )       /* less than minimal */
    {
        status = ERR_DH_LEN;         /* invalid length for DH_modulus */
        return status;
    }
    if ( DH_modulus_bytes & 0x0003 )   /* not multiple 4(32-bit)*/
    {
        status = ERR_DH_LEN;        /* invalid length for DH_modulus */
        return status;
    }
    ALIGN_CALLOC_COPY(DH_public, DH_public_a, DH_modulus_bytes);
/*
  ALIGN_CALLOC_COPY(DH_base, DH_base_a, DH_modulus_bytes);
        ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
*/
    DSS_G_ALIGN_CALLOC_COPY(DH_base, DH_base_a, DH_modulus_bytes);
    DSS_P_ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
    ALIGN_CALLOC_MOVE(RVAL, RVAL_a, SHA_LENGTH);
    ALIGN_CALLOC(DH_shared, DH_shared_a, DH_modulus_bytes);
    ALIGN_CALLOC(DH_random_public, DH_random_public_a, DH_modulus_bytes);
    CALLOC(random_secret, ord, SHA_LENGTH);

    if ( status !=  SUCCESS )
	{
        if( DH_base_a )
            DSS_ALIGN_FREE(DH_base_a,DH_base);
        if( DH_modulus_a )
            DSS_ALIGN_FREE(DH_modulus_a,DH_modulus);
        if( DH_shared_a )
        {
            ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
        }
        if( DH_random_public_a )
        {
            ALIGN_COPY_FREE(DH_random_public_a,DH_random_public,DH_modulus_bytes);
        }
        if(  RVAL_a )
        {
            ALIGN_MOVE_FREE(RVAL_a,RVAL,SHA_LENGTH);
        }
        if( random_secret )
            free(random_secret);
        return status;     /* ERR_ALLOC   insufficient memory */
    }
    BEGIN_PROCESSING
    MyGenRand( SHA_LENGTH, random_secret, RVAL_a );   /* generate random number */
/* Compute Diffie-Hellman random public number */
    if (( status = Expo ( DH_modulus_bytes,  /* DH_random_public=DH_base^ */
          DH_base_a,         /* random_secret mod(DH_modulus)*/
          SHA_LENGTH,
          random_secret,
          DH_modulus_bytes,
          DH_modulus_a,
          DH_random_public_a, NULL)) != SUCCESS ) /*TKL00601*/
    {
	    ERROR_BREAK;
    }
/* Compute Diffie-Hellman secret shared number */
    if ((  status = Expo ( DH_modulus_bytes,  /* DH_shared=DH_public^ */
              DH_public_a,         /* DH_secret mod(DH_modulus)*/
              SHA_LENGTH,
              random_secret,
              DH_modulus_bytes,
	          DH_modulus_a,
              DH_shared_a, NULL)) != SUCCESS ) /*TKL00601*/
    {
        ERROR_BREAK;
    }
    END_PROCESSING
    ALIGN_FREE(DH_public_a);
/*
      ALIGN_FREE(DH_base_a);
  ALIGN_FREE(DH_modulus_a);
*/
	DSS_ALIGN_FREE(DH_base_a,DH_base);
    DSS_ALIGN_FREE(DH_modulus_a,DH_modulus);
    ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
    ALIGN_COPY_FREE(DH_random_public_a,DH_random_public,DH_modulus_bytes);
    ALIGN_MOVE_FREE(RVAL_a,RVAL,SHA_LENGTH);
    free(random_secret);
	return status;
}

/****************************************************************************
*  NAME:  int SFDHDecrypt( ushort DH_modulus_bytes,
*                               uchar  *DH_modulus,
*                               uchar  *DH_secret, uchar  *DH_random_public,
*                               uchar  *DH_shared )
*
*  DESCRIPTION:  Decrypt secret a Diffie-Hellman Shared number
*                                              with Store and Forward form
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_modulus_bytes   Length of DH_modulus
*          uchar *DH_modulus         Pointer to modulus (dss_p)
*          uchar *DH_secret          Pointer to recipient's secret number (dss_x)
*          uchar *DH_random_public   Pointer to sender's random public number
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *DH_shared          Pointer to secret shared number
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DH_LEN                Invalid length for DH_modulus
*          ERR_ALLOC             Insufficient memory
*
*  REVISION HISTORY:
*
*  07 Jul 95   KPZ     Initial release
*
****************************************************************************/

int SFDHDecrypt( ushort DH_modulus_bytes,
                                     uchar  *DH_modulus,
                                     uchar  *DH_secret,
                                      uchar  *DH_random_public,
                                       uchar  *DH_shared )
{
    int status = SUCCESS;          /* function return status */
    ord  *DH_modulus_a;
    ord  *DH_secret_a;
	ord  *DH_random_public_a;
    ord  *DH_shared_a;

    if ( DH_modulus_bytes < DH_LENGTH_MIN )       /* less than minimal */
    {
        status = ERR_DH_LEN;         /* invalid length for DH_modulus */
        return status;
    }
    if ( DH_modulus_bytes & 0x0003 )   /* not multiple 4(32-bit)*/
    {
        status = ERR_DH_LEN;        /* invalid length for DH_modulus */
        return status;
    }
    ALIGN_CALLOC_COPY(DH_random_public, DH_random_public_a, DH_modulus_bytes);
    ALIGN_CALLOC_COPY(DH_secret, DH_secret_a, SHA_LENGTH);
/*        ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);*/
    DSS_P_ALIGN_CALLOC_COPY(DH_modulus, DH_modulus_a, DH_modulus_bytes);
	ALIGN_CALLOC(DH_shared, DH_shared_a, DH_modulus_bytes);

    if ( status !=  SUCCESS )
    {
        if( DH_random_public_a)
        {
            ALIGN_FREE(DH_random_public_a);
        }
        if( DH_secret_a )
        {
            ALIGN_FREE(DH_secret_a);
        }
        if( DH_modulus_a )
            DSS_ALIGN_FREE(DH_modulus_a,DH_modulus);
        if( DH_shared_a )
        {
            ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
        }
        return status;     /* ERR_ALLOC   insufficient memory */
    }
/* Compute Diffie-Hellman secret shared number */
    status = Expo ( DH_modulus_bytes,  /* DH_shared=DH_random_public^ */
             DH_random_public_a,  /* DH_secret mod(DH_modulus)*/
             SHA_LENGTH,
             DH_secret_a,
	         DH_modulus_bytes,
             DH_modulus_a,
             DH_shared_a, NULL); /*TKL00601*/
    ALIGN_FREE(DH_random_public_a);
    ALIGN_FREE(DH_secret_a);
/*      ALIGN_FREE(DH_modulus_a);*/
    DSS_ALIGN_FREE(DH_modulus_a,DH_modulus);
    ALIGN_COPY_FREE(DH_shared_a,DH_shared,DH_modulus_bytes);
    return status;
}


/****************************************************************************
*  NAME:  int SetCipherKey( ushort DH_shared_bytes,
*                           uchar  *DH_shared,
*                           uchar  *Key,
*							ushort cryptoMethod )
*
*  DESCRIPTION:  Set Key by Diffie_Hellman shared number
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_shared_bytes    Length of shared number
*          uchar *DH_shared          Pointer to shared number
*		   ushort cryptoMethod       Algorithm ID
*
*  OUTPUT:
*      PARAMETERS:
*              uchar *Key               Pointer to key
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DATA                Invalid length for DH_shared
*
*  REVISION HISTORY:
*
*  20 Mar 96   KPZ     Initial release
****************************************************************************/
int SetCipherKey( ushort DH_shared_bytes,
				  uchar  *DH_shared,
				 uchar  *Key,
				 ushort cryptoMethod )

{
	 int status = SUCCESS;          /* function return status */

	 switch (cryptoMethod)
	 {
		case DES56_CFB8:
		case DES56_CFB64:
		case DES56_CBC64:
		case DES56_ECB:
			status = SetDES56Key( DH_shared_bytes, DH_shared, Key);
			break;
		case DES40_CFB8:
		case DES40_CFB64:
		case DES40_CBC64:
		case DES40_ECB:
			status = SetDES40Key( DH_shared_bytes, DH_shared, Key);
			break;
		case ONE_TO_ONE:
			memcpy(Key, DH_shared, 8);
			break;
		default:
			status = ERR_UNSUPPORTED;
			break;
	 }

	 return status;
}

