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
*  FILENAME:  test.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Test Functions File
*
*  PUBLIC FUNCTIONS:
*                             int DSSFunctionsTest ( )
*                             int DESModesTest ( )
*                             int DHFuctionsTest ( )
*                             int SFDHFuctionsTest ( )
*
*   Copyright (c) Cylink Corporation 1996. All rights reserved.
*
****************************************************************************/

/****************************************************************************
*  INCLUDE FILES
****************************************************************************/

/* system files */
#ifdef NetBSD
#include <sys/types.h>
#endif
#include <string.h>
#include "cylink.h"
#include <time.h>
#include <math.h>
/* #include <stdio.h> */
/* program files */
#include "toolkit.h"
#define DH_LENGTH 64
#pragma align 4 (DH_base,DH_modulus)

    uchar DH_base[64] =
	 { 0xc9, 0x07, 0x86, 0xbf, 0x92, 0x6c, 0x1e, 0x51,
       0xa5, 0xb6, 0xe6, 0xe1, 0x9d, 0x0b, 0xc6, 0x50,
       0xab, 0x49, 0x77, 0xe6, 0x3c, 0xc7, 0x32, 0x1c,
       0x3f, 0x24, 0xb3, 0x2d, 0xd6, 0x22, 0x40, 0x01,
	   0x32, 0x19, 0x34, 0x35, 0x15, 0xdf, 0xa5, 0x63,
       0x33, 0xe1, 0x35, 0xc1, 0x7e, 0x98, 0xf1, 0x92,
       0x0f, 0xc5, 0x6c, 0xf4, 0x3f, 0x73, 0x4e, 0xf6,
       0x9d, 0x9d, 0xf5, 0xd0, 0xd6, 0x06, 0x9a, 0x3c};

    uchar DH_modulus[64] =
     { 0xff, 0xe2, 0x14, 0x9a, 0xfd, 0xcd, 0x25, 0x47,
       0x1c, 0xaf, 0x1a, 0x7c, 0xd4, 0xeb, 0xdd, 0xf8,
       0xc0, 0x88, 0x66, 0xbe, 0xf2, 0x61, 0xb6, 0xe5,
       0x1f, 0x2b, 0xf0, 0x5c, 0xb4, 0x94, 0x9d, 0xa9,
       0x30, 0x85, 0xe4, 0xe4, 0xed, 0x4d, 0x4d, 0xcb,
       0x2a, 0xe4, 0x7e, 0x5f, 0x6e, 0xef, 0x39, 0xf9,
       0xb5, 0xb6, 0xca, 0x25, 0x5f, 0xcf, 0x3b, 0x2f,
	   0x70, 0x8b, 0xa0, 0x64, 0x72, 0xa8, 0x49, 0xfb};

uchar text[]="Now is the time for all ";
uchar key[8]={0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
uchar iv[8] = {0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};
uchar iv_enc[8];
uchar iv_dec[8];
uchar expand_key[136];
uchar out[64],temp[8];


void DESModesTest ( )
{
    uchar ECB_mode[] = { 0x3f, 0xa4, 0x0e, 0x8a, 0x98, 0x4d, 0x48, 0x15,
         0x6a, 0x27, 0x17, 0x87, 0xab, 0x88, 0x83, 0xf9,
         0x89, 0x3d, 0x51, 0xec, 0x4b, 0x56, 0x3b, 0x53 };
    uchar CBC_mode[] = { 0xe5, 0xc7, 0xcd, 0xde, 0x87, 0x2b, 0xf2, 0x7c,
         0x43, 0xe9, 0x34, 0x00, 0x8c, 0x38, 0x9c, 0x0f,
         0x68, 0x37, 0x88, 0x49, 0x9a, 0x7c, 0x05, 0xf6 };
    uchar CFB1_mode[] = { 0xcd, 0x1e, 0xc9, 0x59, 0xad, 0xd4, 0x80, 0xf1,
         0x1e, 0xe4, 0x0c, 0x51, 0x7f, 0x29, 0xfb, 0x52,
         0xb2, 0x82, 0x94, 0x6f, 0x94, 0x76, 0x5a, 0x13 };
    uchar CFB8_mode[] = { 0xf3, 0x1f, 0xda, 0x07, 0x01, 0x14, 0x62, 0xee,
         0x18, 0x7f, 0x43, 0xd8, 0x0a, 0x7c, 0xd9, 0xb5,
         0xb0, 0xd2, 0x90, 0xda, 0x6e, 0x5b, 0x9a, 0x87 };
    uchar CFB64_mode[] = { 0xf3, 0x09, 0x62, 0x49, 0xc7, 0xf4, 0x6e, 0x51,
         0xa6, 0x9e, 0x83, 0x9b, 0x1a, 0x92, 0xf7, 0x84,
         0x03, 0x46, 0x71, 0x33, 0x89, 0x8e, 0xa6, 0x22 };
    uchar OFB1_mode[] = { 0xe3, 0xd3, 0x4b, 0x2c, 0x6f, 0xdc, 0xd8, 0x4f,
         0xc2, 0x68, 0x58, 0x81, 0x83, 0xa0, 0xa3, 0xab,
         0x6c, 0x3d, 0x8a, 0xa1, 0x1e, 0x1e, 0x94, 0xad };
    uchar OFB8_mode[] = { 0xf3, 0x4a, 0x28, 0x50, 0xc9, 0xc6, 0x49, 0x85,
         0xd6, 0x84, 0xad, 0x96, 0xd7, 0x72, 0xe2, 0xf2,
         0x43, 0xea, 0x49, 0x9a, 0xbe, 0xe8, 0xae, 0x95 };
    uchar OFB64_mode[] = { 0xf3, 0x09, 0x62, 0x49, 0xc7, 0xf4, 0x6e, 0x51,
         0x35, 0xf2, 0x4a, 0x24, 0x2e, 0xeb, 0x3d, 0x3f,
         0x3d, 0x6d, 0x5b, 0xe3, 0x25, 0x5a, 0xf8, 0xc3 };

    DESKeyExpand ( key, expand_key );

    DESEncrypt ( NULL, expand_key, ECB_MODE, text, out, strlen((char *)text));
	if ( memcmp( out, ECB_mode, strlen((char *)text)) != 0 )
    {
        printf( "\t Encryption in ECB mode is bad\n" );
    }
    DESDecrypt( NULL, expand_key, ECB_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for ECB mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in  ECB mode is bad\n" );
    }

    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, CBC_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, CBC_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in CBC mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, CBC_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for CBC mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in CBC mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, CFB1_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, CFB1_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in CFB1 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, CFB1_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for CFB1 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in CFB1 mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, CFB8_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, CFB8_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in CFB8 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, CFB8_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for CFB8 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in CFB8 mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, CFB64_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, CFB64_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in CFB64 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, CFB64_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for CFB64 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in CFB64 mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, OFB1_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, OFB1_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in OFB1 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, OFB1_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for OFB1 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in OFB1 mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, OFB8_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, OFB8_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in OFB8 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, OFB8_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for OFB8 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in OFB8 mode is bad\n" );
    }
    memcpy ( iv_enc, iv , 8 );
    DESEncrypt ( iv_enc, expand_key, OFB64_MODE, text, out, strlen((char *)text));
    if ( memcmp( out, OFB64_mode, strlen((char *)text) ) != 0 )
    {
        printf( "\t Encryption in OFB64 mode is bad\n" );
    }  
    memcpy ( iv_dec, iv , 8 );
    DESDecrypt( iv_dec, expand_key, OFB64_MODE, out, strlen((char *)text) );
    if ( memcmp( text, out, strlen((char *)text) ) == 0 )
    {
        printf( "\tTest result for OFB64 mode is good\n" );
    }
    else
    {
        printf( "\tDecryption in OFB64 mode is bad\n" );
    }
}

/****************************************************************************
*           NAME:   int DSSFunctionsTest ( )
*
*
*  DESCRIPTION:  Tests DSS algorithm functions.
*
*  INPUTS:
*      PARAMETERS:
*
*  OUTPUT:
*         PARAMETERS:
*
*           RETURN:
*                SUCCESS                No error
*           < 0                   Error
*  REVISION HISTORY:
*
*  14 Oct 94      KPZ             Initial release
*
****************************************************************************/
/*//clock_t t1,t2;*/

int DSSFunctionsTest ( )
{
/*hrtime_t hr_start, hr_end;*/
	uchar RVAL [SHA_LENGTH];
	uchar dss_p [DSS_LENGTH_MIN];
	uchar dss_q [SHA_LENGTH];
	uchar dss_g [DSS_LENGTH_MIN];
	uchar dss_x [SHA_LENGTH];
	uchar dss_y [DSS_LENGTH_MIN];
	uchar message [2 * DSS_LENGTH_MIN];
	uchar dss_s [SHA_LENGTH];
	uchar dss_r [SHA_LENGTH];
	uchar dss_k [SHA_LENGTH];
	uchar digest [SHA_LENGTH];
	uchar hash_result [SHA_LENGTH];
	SHA_context digest_context;     /*SHA context structure*/
	ushort i;
	int status = SUCCESS;      /* function return status */
	uchar seed []="This is the new initialization vector";
											  /* counter */
 /* Initialize random number generator */
	InitRand( (ushort)strlen((char*)seed), seed, RVAL );
 /* Generate DSS common parameters*/
	if( (status = GenDSSParameters( DSS_LENGTH_MIN, dss_p, dss_q,
			   dss_g, RVAL, NULL)) != SUCCESS )
	{
		return status;
	}
 /* Compute a DSS Public/Private number pair */
	if ( (status = GenDSSKey( DSS_LENGTH_MIN,/*NULL*/ dss_p,/*NULL*/ dss_q,/*NULL*/ dss_g,
		dss_x, dss_y, RVAL )) != SUCCESS)
	{
		return status;
    }
 /* Compute a DSS per message secret number */
    if ( (status = GenDSSNumber( dss_k, /*NULL*/dss_q, RVAL )) != SUCCESS )
    {
        return status;
    }
 /* Generate random number */
    if ( (status = GenRand( 2 * DSS_LENGTH_MIN, message, RVAL)) != SUCCESS )
    {
        return status;
    }
 /* Compute secure hash function */
    if ( (status = SHA( message,2 *  DSS_LENGTH_MIN, digest)) != SUCCESS )
    {
        return status;
    }
 /* Compute a DSS signature */
/*//  t1 = clock();*/
/*hr_start =gethrvtime();*/

    if ( (status = GenDSSSignature( DSS_LENGTH_MIN,/*NULL*/dss_p,/*NULL*/dss_q,/*NULL*/dss_g, dss_x,
             dss_k, dss_r, dss_s, digest)) != SUCCESS )
    {
        return status;
    }
/*//  t2 = clock();   /
// printf( " %f",(t2 -t1)/CLK_TCK); */
 /* Initialize  secure hush function */
    SHAInit ( &digest_context );
    for ( i = 0; i < (ushort)(2* DSS_LENGTH_MIN /16 ); i++ )
    {
 /* Update Secure hush function */
        if ( (status = SHAUpdate( &digest_context, message + i * 16,
                 16 )) != SUCCESS )
		{
            return status;
        }
    }
 /* Finalize secure hush function */
    if ((status = SHAFinal (&digest_context, hash_result)) != SUCCESS )
    {
        return status;
    }
 /* Compare obtained results */
    if ( (status = memcmp( digest, hash_result, SHA_LENGTH )) != SUCCESS )
    {
        return status;
    }
 /* Verify a DSS signature */
/* // t1 = clock();*/
    if ( ( status = VerDSSSignature( DSS_LENGTH_MIN,/*NULL*/ dss_p,/* NULL*/dss_q,/*NULL*/dss_g,
				dss_y, dss_r, dss_s, hash_result)) != SUCCESS )
    {
	    return status;
    }
/*//  t2 = clock();*/
/*// printf( "\n %f",(t2 -t1)/CLK_TCK);*/
/*
hr_end =gethrvtime();
hr_end = hr_end - hr_start;
hr_end = hr_end / (pow(10,6));
printf("gethrvtime = %lld msec\n", hr_end );
*/

    return status;
}



/****************************************************************************
*           NAME:   int DHFuctionsTest ( )
*
*
*  DESCRIPTION:  Tests DH algorithm functions.
*
*  INPUTS:
*         PARAMETERS:
*
*  OUTPUT:
*         PARAMETERS:
*
*           RETURN:
*                SUCCESS                No error
*           < 0                   Error
*  REVISION HISTORY:
*
*  14 Oct 94      KPZ             Initial release
*
****************************************************************************/


int DHFunctionsTest ( )
{
    uchar RVAL [SHA_LENGTH];
    uchar DH_public_A [DH_LENGTH];
    uchar DH_secret_A [DH_LENGTH];
    uchar DH_public_B [DH_LENGTH];
    uchar DH_secret_B [DH_LENGTH];
    uchar DH_shared_A [DH_LENGTH];
    uchar DH_shared_B [DH_LENGTH];
    uchar message [DH_LENGTH];
    uchar newmess [DH_LENGTH];
    uchar oldmess [DH_LENGTH];
    uchar Key_A [8];
    uchar Key_B [8];
    uchar DES_Key_A [128];
    uchar DES_Key_B [128];
    uchar key_iv[8]={0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef};
    int status = SUCCESS;      /* function return status */
    uchar seed[]="This is the initialization vector";

 /* Initialize random number generator */
    InitRand( (ushort)strlen((char*)seed), seed, RVAL );
 /* Alice is compute a Diffie-Hellman paire */
    if ( (status = GenDHPair( DH_LENGTH, DH_secret_A, DH_public_A,
        (uchar *)DH_base, (uchar *)DH_modulus, RVAL)) != SUCCESS)
    {
        return status;
    }
 /* Bob is compute a Diffie-Hellman paire */
    if ( (status = GenDHPair( DH_LENGTH, DH_secret_B, DH_public_B,
         (uchar *)DH_base, (uchar *)DH_modulus, RVAL)) != SUCCESS)
    {
        return status;
    }
 /* Alice is compute a Diffie-Hellman Shared number */
    if ( (status = GetDHSharedNumber( DH_LENGTH, DH_secret_A, DH_public_B,
           DH_shared_A, (uchar *)DH_modulus)) != SUCCESS)
    {
        return status;
    }
 /* Alice is sets DES/KAPPPA key by Diffie_Hellman shared number */
    if ( (status = SetCipherKey( DH_LENGTH, DH_shared_A, Key_A, DES56_ECB ))
                       !=SUCCESS)
    {
        return status;
    }
 /* Generate random number */
    if ((status = GenRand( DH_LENGTH, message, RVAL)) != SUCCESS )
    {
        return status;
    }
 /* Alice key schedule */
    DESKeyExpand( Key_A, DES_Key_A );
 /* Alice encrypt a block of data with single DES */
    if ( (status = DESEncrypt( key_iv, DES_Key_A, ECB_MODE, message,
             newmess, DH_LENGTH )) != SUCCESS )
    {
        return status;
    }
 /* Bob is compute a Diffie-Hellman Shared number */
    if ( (status = GetDHSharedNumber( DH_LENGTH, DH_secret_B, DH_public_A,
            DH_shared_B, (uchar *)DH_modulus)) != SUCCESS)
    {
        return status;
    }
    if ( (status = SetCipherKey( DH_LENGTH, DH_shared_B, Key_B, DES56_ECB ))
                       !=SUCCESS)
    {
        return status;
    }
 /* Bob key schedule */
    DESKeyExpand( Key_B, DES_Key_B );
 /* Bob decrypt a block of data with single DES */
    if ( (status = DESDecrypt( key_iv, DES_Key_B, ECB_MODE, newmess,
                 DH_LENGTH )) != SUCCESS )
    {
        return status;
    }
 /* Compare plaintext with decrepted text */
    if ( (status = memcmp( newmess, message, DH_LENGTH ))
             != SUCCESS )
    {
        return status;
    }
 /* One-Time-Pad signature with a Diffie-Hellman shared number */
    if ( (status = DHOneTimePad( DH_LENGTH, DH_shared_A, message, newmess))
                 != SUCCESS )
    {
        return status;
    }
 /* One-Time-Pad signature with a Diffie-Hellman shared number */
    if ( (status = DHOneTimePad( DH_LENGTH, DH_shared_A, newmess, oldmess))
            != SUCCESS )
    {
        return status;
    }
 /* Compare obtained results */
    if ( (status = memcmp( message, oldmess, DH_LENGTH ))
        != SUCCESS )
    {
        return status;
    }
    return status;
}

/****************************************************************************
*           NAME:   int SFDHFunctionsTest ( )
*
*
*  DESCRIPTION:  Tests SFDH algorithm functions.
*
*  INPUTS:
*    PARAMETERS:
*
*  OUTPUT:
*         PARAMETERS:
*
*           RETURN:
*                SUCCESS                No error
*           < 0                   Error
*  REVISION HISTORY:
*
*  14 Oct 94      KPZ             Initial release
*
****************************************************************************/

int SFDHFunctionsTest ( )
{
    uchar RVAL [SHA_LENGTH];
    uchar dss_y_A [DSS_LENGTH_MIN];
    uchar dss_x_A [SHA_LENGTH];
    uchar dss_y_B [DSS_LENGTH_MIN];
    uchar dss_x_B [SHA_LENGTH];
    uchar random_public [DSS_LENGTH_MIN];
    uchar DH_shared_A [DSS_LENGTH_MIN];
    uchar DH_shared_B [DSS_LENGTH_MIN];
    uchar dss_s [SHA_LENGTH];
    uchar dss_r [SHA_LENGTH];
    uchar dss_k [SHA_LENGTH];
    uchar digest [SHA_LENGTH];
    uchar hash_result [SHA_LENGTH];
    int status = SUCCESS;      /* function return status */
    uchar seed []="This is the seed value";


/* Initialize random number generator */
    InitRand( (ushort)strlen((char*)seed), seed, RVAL );

 /* Compute Alice's DSS Public/Private number pair */
    if ( (status = GenDSSKey( DSS_LENGTH_MIN, NULL, NULL, NULL,
          dss_x_A, dss_y_A, RVAL )) != SUCCESS)
    {
        return status;
    }
 /* Compute Bob's DSS Public/Private number pair */
    if ( (status = GenDSSKey( DSS_LENGTH_MIN, NULL, NULL, NULL,
           dss_x_B, dss_y_B, RVAL )) != SUCCESS)
    {
        return status;
    }

/* Alice Computes a DH secret shared number */
    if ( ( status = SFDHEncrypt( DSS_LENGTH_MIN, NULL, NULL,
            dss_y_B, random_public, DH_shared_A, RVAL )) != SUCCESS)
    {
        return status;
    }

/* Alice Computes a DSS signature. Compute secure hash function */
    if ( (status = SHA( random_public, DSS_LENGTH_MIN, digest)) != SUCCESS )
    {
        return status;
    }
    if ( (status = GenDSSNumber( dss_k, NULL, RVAL )) != SUCCESS )
    {
        return status;
    }
    if ( (status = GenDSSSignature( DSS_LENGTH_MIN, NULL, NULL, NULL, dss_x_A,
                dss_k, dss_r, dss_s, digest)) != SUCCESS )
    {
        return status;
    }

/* Bob Verifies a DSS signature */
    if ( (status = SHA( random_public, DSS_LENGTH_MIN,hash_result )) != SUCCESS )
    {
        return status;
    }
       
    if ( ( status = VerDSSSignature( DSS_LENGTH_MIN, NULL, NULL, NULL,
         dss_y_A, dss_r, dss_s, hash_result)) != SUCCESS )
    {
        return status;
    }

/* Bob Computes a DH secret shared number */
    if ( ( status = SFDHDecrypt( DSS_LENGTH_MIN, NULL, dss_x_B,
          random_public, DH_shared_B )) != SUCCESS)
    {
        return status;
    }

/* Compare obtained results */
    if ( (status = memcmp( DH_shared_A, DH_shared_B, DSS_LENGTH_MIN )) != SUCCESS )
    {
        return status;
    }

    return status;
}
