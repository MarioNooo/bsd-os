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
*  FILENAME: des.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:  DES Functions
*
*  PUBLIC FUNCTIONS:
*                   void DESKeyExpand( uchar *key,
*                            uchar *expanded_key )
*                   int DESEncrypt( uchar  des_iv, uchar  *des_key,
*                         ushort des_mode, uchar *input_array,
*                         uchar  *output_array, ushort input_array_bytes )
*                   int DESDecrypt( uchar  *des_iv, uchar  *des_key,
*                           ushort des_mode, uchar  *data_array,
*                           ushort data_array_bytes )
*
*  PRIVATE FUNCTIONS:
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

/* system files */
#ifdef VXD
#include <vtoolsc.h>
#else
#include <stdlib.h>
#include <string.h>
#endif
/* program files */
#include "cylink.h"
#include "toolkit.h"
#include "cencrint.h"
#include "c_asm.h"
#include "des.h"
#include "endian.h"

/* in-place des encryption */
extern void des_encrypt(uchar *keybuf, uchar *block);
/* in-place des decryption */
extern void des_decrypt(uchar *keybuf, uchar *block);


/****************************************************************************
*  NAME:  void DESKeyExpand( uchar *key,
*                            uchar *expanded_key )
*
*  DESCRIPTION:  Expand DES key
*
*  INPUTS:
*      PARAMETERS:
*          uchar *key     Pointer to 8-byte key
*  OUTPUT:
*      PARAMETERS:
*          uchar *expanded_key  Pointer to expanded [16][8] key
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support )
*
****************************************************************************/

void DESKeyExpand( uchar *key,
				   uchar *K1 )
	/* 64 bits (will use only 56) */
{
#if defined( ASM_FUNCTION ) && defined( B_ENDIAN )
 double K1_a[ (16*8) / sizeof(double)];
 uchar key_a[8];
 if (((ulong)K1 & 7) + ((ulong)key & 7))
 {
   memcpy(&key_a,key,8);
   ExpandKey( K1_a,key_a);
   memcpy(K1,K1_a,16*8);
 }
 else
 {
   ExpandKey((double *)K1,key);
 }
#else
	uchar pc1m[56];    /* place to modify pc1 into */
	uchar pcr[56];     /* place to rotate pc1 into */
	int i, j, l;
	int m;
	uchar *kn = K1;

    /* Clear key schedule */
    for (i = 0; i < 16; i++)
    {
			  for ( j = 0; j < 8; j++)
        {
                       kn[i*8+j] = 0;
		}
    }
	for ( j = 0; j < 56; j++)           /* convert pc1 to bits of key */
    {
        l = pc1[j]-1;                   /* integer bit location */
        m = l & 07;                     /* find bit      */
		pc1m[j] =(uchar )((key[l >> 3] & /* find which key byte l is in */
		 (uchar )bytebit[m])              /* and which bit of that byte */
         ? 1 : 0);                      /* and store 1-st bit result */
    }
	for ( i = 0; i < 16; i++)            /* key chunk for each iteration */
    {
          for ( j = 0; j < 56; j++)       /* rotate pc1 the right amount */
        {
			pcr[j] = pc1m[(l = j + totrot[i]) <
                    (j < 28 ? 28 : 56) ? l: l-28];
        }
	/* rotate left and right halves independently */
        for ( j = 0; j < 48; j++)         /* select bits individually */
		{
   /* check bit that goes to kn[j] */
			if ( pcr[pc2[j] - 1] )
			{
   /* mask it in if it's there */
				l = j % 6;
								kn[i*8+j/6] |= bytebit[l] >> 2;
			}
			}
	}
#endif
}


/****************************************************************************
*  NAME:  int DESEncrypt( uchar  des_iv,
*                         uchar  *des_key,
*                         ushort des_mode,
*                         uchar  *input_array,
*                         uchar  *output_array,
*                         ushort input_array_bytes )
*
*  DESCRIPTION:  Encrypt a block of data with single DES
*
*  INPUTS:
*      PARAMETERS:
*          uchar *des_iv             Pointer to Initialazation vector
*          uchar *des_key            Pointer to expanded encryption key
*          ushort des_mode           Encryption mode
*          uchar *input_array        Pointer to input array
*          ushort input_array_bytes  Number of bytes in input array
*  OUTPUT:
*      PARAMETERS:
*          uchar *output_array       Pointer to encrypted array.
*  RETURN:
*          SUCCESS             No errors
*          ERR_BLOCK_LEN       Invalid length for input block for ECB/CBC
*          ERR_MODE            Invalid value of encryption mode
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

int DESEncrypt( uchar  *des_iv,
				uchar  *des_key,
                ushort des_mode,
                uchar  *input_array,
                uchar  *output_array,
                ushort input_array_bytes )
{
   int status = SUCCESS;       /* function return status */
   uchar a[8], temp[8];        /* work buffers */
   uchar outputb = 0;          /* work byte */
   int  BlockCount;            /* number of blocks */
   int  LastBlock;             /* size of last block */
   ushort i, j, k;                /* counters */

	switch ( des_mode )
    {
		case ECB_MODE:
            BlockCount = input_array_bytes / 8 ;
            LastBlock  = input_array_bytes % 8 ;
            if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
            {
                status = ERR_BLOCK_LEN;
                return status;
            }
			for ( j = 0; j < (ushort)BlockCount; j++)
		{
				for ( i = 0; i < 8; i++)
				{
					a[i] = (input_array + 8 * j)[i];
				}
				DES_encrypt( des_key, a );
				memcpy (output_array + 8 * j, a, 8);
		   }
		   break;
		case CBC_MODE:
			BlockCount = input_array_bytes / 8 ;
			LastBlock  = input_array_bytes % 8 ;
			if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
		{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( j = 0; j < (ushort)BlockCount; j++)
			{
				for (i = 0; i < 8; i++)
				{
					a[i] = (input_array + 8 * j)[i] ^ des_iv[i];
				}
				DES_encrypt( des_key, a );
				memcpy (output_array + 8 * j, a, 8);
				for ( i = 0; i < 8; i++)
		{
					des_iv[i] = a[i];
				}
			}
			break;
		case CFB8_MODE:
			for (i = 0; i < input_array_bytes; i++)
			{
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
				}
				DES_encrypt( des_key, a );
		output_array[i] = input_array[i] ^ a[0];
				for (j = 0; j < 7; j++)
				{
					des_iv[j] = des_iv[j+1];
				}
				des_iv[7] = output_array[i];
			}
			break;
		case CFB64_MODE:
			BlockCount = input_array_bytes / 8 ;
			LastBlock  = input_array_bytes % 8 ;
			if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
			{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( i = 0; i < (ushort)BlockCount; i++)
			{
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
				}
				DES_encrypt( des_key, a );
				for (j = 0; j < 8; j++)
				{
					output_array[j+8*i] = input_array[j+8*i] ^ a[j];
				}
				for (j = 0; j < 8; j++)
				{
					des_iv[j] = output_array[j+8*i];
				}
			}
			break;
		case CFB1_MODE:
			for (i = 0; i < input_array_bytes; i++)
			{
				for ( k = 0; k < 8; k++)
				{
			memcpy( temp, des_iv, 8);
					DES_encrypt( des_key, temp );
					outputb |= (uchar)(((input_array[i] << k) ^ temp[0]) & 0x80) >> k;
					for ( j = 0; j < 7; j++)
					{
						des_iv[j] = (des_iv[j] << 1) |          /*TKL00101*/
									((des_iv[j+1] & (uchar)0x80) >> 7);
					}
					des_iv[7] = (des_iv[7] << 1) |
								   ((outputb >> (7-k)) & 0x01);
				}
				output_array[i] = outputb;
				outputb = 0;
		}
			break;
		case OFB1_MODE:
			for (i = 0; i < input_array_bytes; i++)
			{
				for ( k = 0; k < 8; k++)
				{
			memcpy( temp, des_iv, 8);
					DES_encrypt( des_key, temp );
					outputb |= (uchar)(((input_array[i] << k) ^ temp[0]) & 0x80) >> k;
					for ( j = 0; j < 7; j++)
					{
						des_iv[j] = (des_iv[j] << 1) |          /*TKL00101*/
									((des_iv[j+1] & (uchar)0x80) >> 7);
					}
					des_iv[7] = (des_iv[7] << 1) |
								   ((temp[0] >> 7) & 0x01);
				}
				output_array[i] = outputb;
				outputb = 0;
		}
			break;
		case OFB8_MODE:
			for (i = 0; i < input_array_bytes; i++)
			{
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
				}
				DES_encrypt( des_key, a );
				output_array[i] = input_array[i] ^ a[0];
				for (j = 0; j < 7; j++)
				{
					des_iv[j] = des_iv[j+1];
				}
				des_iv[7] = a[0];
			}
			break;
		case OFB64_MODE:
			BlockCount = input_array_bytes / 8 ;
			LastBlock  = input_array_bytes % 8 ;
			if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
			{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( i = 0; i < (ushort)BlockCount; i++)
			{
				DES_encrypt( des_key, des_iv );
				for (j = 0; j < 8; j++)
				{
					output_array[j+8*i] = input_array[j+8*i] ^ des_iv[j];
				}
			}
			break;
		default:
			return ERR_MODE;
	}
	return status;
}

/****************************************************************************
*   NAME:   int DESDecrypt( uchar  *des_iv,
*                           uchar  *des_key,
*                           ushort des_mode,
*                           uchar  *data_array,
*                           ushort data_array_bytes )
*
*  DESCRIPTION:  Decrypt a block of data with single DES
*
*  INPUTS:
*      PARAMETERS:
*          uchar *des_iv             Pointer to Initialazation vector
*          uchar *des_key            Pointer to expanded encryption key
*          ushort des_mode           Encryption mode
*          uchar *data_array         Pointer to input array
*          ushort input_array_bytes  Number of bytes in input array
*  OUTPUT:
*      PARAMETERS:
*          uchar *data_array         Pointer to decrypted array
*  RETURN:
*          SUCCESS             No errors
*          ERR_BLOCK_LEN       Invalid length for input block for ECB/CBC
*          ERR_MODE            Invalid value of encryption mode
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*
****************************************************************************/

int DESDecrypt( uchar  *des_iv,
                uchar  *des_key,
				ushort des_mode,
				uchar  *data_array,
                ushort data_array_bytes )
{
   int status = SUCCESS;       /* function return status */
   uchar a[8], temp[8];        /* work buffers */
   uchar outputb = 0;          /* work byte */
   int  BlockCount;            /* number of blocks */
   int  LastBlock;             /* size of last block */
   ushort i, j, k;                /* counters */

	switch ( des_mode )
	{
		case ECB_MODE:
			BlockCount = data_array_bytes / 8 ;
            LastBlock  = data_array_bytes % 8 ;
			if ( LastBlock != 0)
			{
				status = ERR_BLOCK_LEN;
				return status;
            }
            for ( j = 0; j < (ushort)BlockCount; j++)
            {
				for ( i = 0; i < 8; i++)
                {
                    a[i] = (data_array + 8 * j)[i];
                }
				DES_decrypt( des_key, a );
                memcpy (data_array + 8 * j, a, 8);
		   }
		   break;
		case CBC_MODE:
			BlockCount = data_array_bytes / 8 ;
			LastBlock  = data_array_bytes % 8 ;
			if ( LastBlock != 0)
			{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( j = 0; j < (ushort)BlockCount; j++)
			{
				for (i = 0; i < 8; i++)
				{
					a[i] = (data_array + 8 * j)[i];
					temp[i] = (data_array + 8 * j)[i];
				}
				DES_decrypt( des_key, a );
				for ( i = 0; i < 8; i++)
				{
					(data_array + 8 *j)[i] = des_iv[i] ^ a[i];
				}
				memcpy (des_iv , temp, 8);
			}
			break;
		case CFB8_MODE:
			for (i = 0; i < data_array_bytes; i++)
			{
				temp[0] = data_array[i];
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
				}
				DES_encrypt( des_key, a );
				data_array[i] = data_array[i] ^ a[0];
				for (j = 0; j < 7; j++)
				{
					des_iv[j] = des_iv[j+1];
				}
				des_iv[7] = temp[0];
			}
			break;
		case CFB64_MODE:
			BlockCount = data_array_bytes / 8 ;
			LastBlock  = data_array_bytes % 8 ;
			if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
			{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( i = 0; i < (ushort)BlockCount; i++)
			{
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
					des_iv[j] = data_array[j+8*i];
				}
				DES_encrypt( des_key, a );
				for (j = 0; j < 8; j++)
				{
					data_array[j+8*i] ^= a[j];
				}
			}
			break;
		case CFB1_MODE:
			for (i = 0; i < data_array_bytes; i++)
			{
				for ( k = 0; k < 8; k++)
				{
					memcpy ( temp, des_iv, 8);
					DES_encrypt( des_key, temp );
					outputb |= (uchar)(((data_array[i] << k) ^ temp[0]) & 0x80) >> k;
					for ( j = 0; j < 7; j++)
					{
						des_iv[j] = (des_iv[j] << 1) |          /*TKL00101*/
									((des_iv[j+1] & (uchar)0x80) >> 7);
					}
					des_iv[7] = (des_iv[7] << 1) |
								   ((data_array[i] >> (7-k)) & 0x01);
				}
				data_array[i] = outputb;
				outputb = 0;
			}
			break;
		case OFB1_MODE:
			for (i = 0; i < data_array_bytes; i++)
			{
				for ( k = 0; k < 8; k++)
				{
					memcpy ( temp, des_iv, 8);
					DES_encrypt( des_key, temp );
					outputb |= (uchar)(((data_array[i] << k) ^ temp[0]) & 0x80) >> k;
					for ( j = 0; j < 7; j++)
					{
						des_iv[j] = (des_iv[j] << 1) |          /*TKL00101*/
									((des_iv[j+1] & (uchar)0x80) >> 7);
					}
					des_iv[7] = (des_iv[7] << 1) |
								   ((temp[0] >> 7) & 0x01);
				}
				data_array[i] = outputb;
				outputb = 0;
			}
			break;
		case OFB8_MODE:
			for (i = 0; i < data_array_bytes; i++)
			{
				for (j = 0; j < 8; j++)
				{
					a[j] = des_iv[j];
				}
				DES_encrypt( des_key, a );
				data_array[i] ^= a[0];
				for (j = 0; j < 7; j++)
				{
					des_iv[j] = des_iv[j+1];
				}
				des_iv[7] = a[0];
			}
			break;
		case OFB64_MODE:
			BlockCount = data_array_bytes / 8 ;
			LastBlock  = data_array_bytes % 8 ;
			if ( LastBlock != 0)    /* not multiple 8 (64 bit) */
			{
				status = ERR_BLOCK_LEN;
				return status;
			}
			for ( i = 0; i < (ushort)BlockCount; i++)
			{
				DES_encrypt( des_key, des_iv );
				for (j = 0; j < 8; j++)
				{
					data_array[j+8*i] ^= des_iv[j];
				}
			}
			break;
		default:
			return ERR_MODE;
	}
  return status;
}

/* Permute inblock with perm for DES functions */
void permute(uchar *inblock, uchar perm[16][16][8], uchar *outblock)
 /* result into outblock,64 bits */
	  /* 2K bytes defining perm. */
{
   register short i, j;
   register uchar *ib, *ob;    /* ptr to input or output block */
   register uchar *p, *q;
   if(perm == NULL){
       /* No permutation, just copy */
         for(i=8; i!=0; i--)
    *outblock++ = *inblock++;
		return;
   }
   /* Clear output block     */
   for (i=8, ob = outblock; i !=0; i--)
       *ob++ = 0;

   ib = inblock;
   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
	ob = outblock;
      p = perm[j][(*ib >> 4) & 017];
      q = perm[j + 1][*ib & 017];
	 for (i =8; i !=0; i--){   /* and each output byte */
   *ob++ |= *p++ | *q++;  /* OR the masks together*/
        }
   }
}

/* The nonlinear function f(r,k), the heart of DES */
long f(unsigned long r, uchar *subkey)
       /* 32 bits */
   /* 48-bit key for this round */
{
   register unsigned long rval,rt;
/*   unsigned long check;       */  /*    checking */

   /* Run E(R) ^ K through the combined S & P boxes
    * This code takes advantage of a convenient regulariti in
    * E, namely that each group of 6 bits in E(R) feeding
	  * a single S-box is a contiguous segment of R.
  */
   rt = (r >> 1) | ((r & 1) ? 0x80000000uL : 0);
   rval = 0;
/*  check= ((rt >> 26) ^ *subkey) & 0x3f;   */
   rval |= sp[0][(short)(((rt >> 26) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 22) ^ *subkey) & 0x3f;  */
   rval |= sp[1][(short)(((rt >>22 ) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 18) ^ *subkey) & 0x3f;  */
   rval |= sp[2][(short)(((rt >>18 ) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 14) ^ *subkey) & 0x3f;  */
   rval |= sp[3][(short)(((rt >>14 ) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 10) ^ *subkey) & 0x3f;  */
   rval |= sp[4][(short)(((rt >>10 ) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 6) ^ *subkey) & 0x3f;   */
   rval |= sp[5][(short)(((rt >>6 ) ^ *subkey++) & 0x3f)];
/*   check= ((rt >> 2) ^ *subkey) & 0x3f;   */
   rval |= sp[6][(short)(((rt >>2 ) ^ *subkey++) & 0x3f)];

   rt = (r << 1) | ((r & 0x80000000uL) ? 1 : 0);
   rval |= sp[7][(short)((rt ^ *subkey) & 0x3f)];
   return rval;
}

/* Do one DES cipher round */
void round(uchar *keybuf, int num, unsigned long *block)
                  /* i.e. the num-th one   */
{
   /* The rounds are numbered from 0 to 15. On even rounds
    * the right half is fed to f() and the result exclusive-ORs
		* the left half; on odd rounds the reverse in done.
	 */
   if(num & 1){
      block[1] ^= f(block[0],&keybuf[8*num]);
   } else {
         block[0] ^= f(block[1],&keybuf[8*num]);
   }
}


/****************************************************************************
*  NAME:   void DES_encrypt(uchar *keybuf, uchar *block)
*
*  DESCRIPTION:  In-place DES encryption
*
*  INPUTS:
*           PARAMETERS:
*           uchar *keybuf        Pointer to expanded key
*           uchar *block         Pointer to input block
*
*  OUTPUT:
*          PARAMETERS:
*           uchar *block         Encrypted block
*
*           RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support )
*
****************************************************************************/

void DES_encrypt(uchar *keybuf, uchar *block)
{
#ifdef ASM_FUNCTION
#ifdef B_ENDIAN
  double keybuf_a[ (16*8) / sizeof(double)];
  if (((ulong)keybuf & 7))
  {
    memcpy(&keybuf_a,keybuf,16*8);
    Encrypt(keybuf_a,block,block);
  }
  else
  {
    Encrypt((double *)keybuf,block,block);
  }
#else
  des_encrypt(keybuf, block);
#endif
#else
   register ushort i;
   unsigned long work[2];       /* Working data storage */
   long tmp;

   permute(block,iperm,(uchar *)work);     /* Initial Permutation */

#ifdef L_ENDIAN
   ByteSwap32_big((uchar*)&work[0],4);
   ByteSwap32_big((uchar*)&work[1],4);
#endif
   /* Do the 16 rounds */
   for (i=0; i<16; i++)
       round(keybuf,i,work);

   /* Left/right half swap */
   tmp = work[0];
   work[0] = work[1];
   work[1] = tmp;

#ifdef L_ENDIAN
   ByteSwap32_big((uchar*)&work[0],4);
   ByteSwap32_big((uchar*)&work[1],4);
#endif

   permute((uchar *)work,fperm,block);  /* Inverse initial permutation */
#endif
}


/****************************************************************************
*  NAME:   void DES_decrypt(uchar *keybuf, uchar *block)
*
*  DESCRIPTION:  In-place DES decryption
*
*  INPUTS:
*        PARAMETERS:
*           uchar *keybuf        Pointer to expanded key
*           uchar *block         Pointer to input block
*
*  OUTPUT:
*          PARAMETERS:
*           uchar *block         Decrypted block
*
*           RETURN:
*
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support )
*
****************************************************************************/
void DES_decrypt(uchar *keybuf, uchar *block)
{
#ifdef ASM_FUNCTION
#ifdef B_ENDIAN
  double keybuf_a[ (16*8) / sizeof(double)];
  if (((ulong)keybuf & 7))
  {
	memcpy(&keybuf_a,keybuf,16*8);
    Decrypt(keybuf_a,block,block);
  }
  else
  {
    Decrypt((double *)keybuf,block,block);
  }
#else
  des_decrypt(keybuf, block);
#endif
#else
   register short i;
   unsigned long work[2];  /* Working data storage */
   long tmp;

   permute(block,iperm,(uchar *)work);    /* Initial permutation */

#ifdef L_ENDIAN
   ByteSwap32_big((uchar*)&work[0],4);
   ByteSwap32_big((uchar*)&work[1],4);
#endif

   /* Left/right half swap */
   tmp = work[0];
   work[0] = work[1];
   work[1] = tmp;

   /* Do the 16 rounds in reverse order */
   for (i=15; i >= 0; i--)
	  round(keybuf,i,work);

#ifdef L_ENDIAN
   ByteSwap32_big((uchar*)&work[0],4);
   ByteSwap32_big((uchar*)&work[1],4);
#endif

   permute((uchar *)work,fperm,block);  /* Inverse initial permutation */
#endif
}


/****************************************************************************
*  NAME:   void SetKeyParity( uchar *key )
*
*  DESCRIPTION: Set parity bits in key
*
*  INPUTS:
*          PARAMETERS:
*              uchar *key     Pointer to key array
*  OUTPUT:
*          PARAMETERS:
*              uchar *key     Pointer to key array (updated)
*
*          RETURN:
*                               None
*
*  REVISION HISTORY:
*
*  24 Sep 94    KPZ             Initial release
*  14 Oct 94    GKL     Second version (big endian support)
*
****************************************************************************/

void SetKeyParity( uchar *key ) /*TKL00801*/
{
  int i, j;
  uchar mask[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
  uchar parity = 0x01;

 for ( i = 0; i < 8; i++ )
	   {
		for ( j = 1; j < 8; j++ )       /*TKL00901*/
		   {
					   if ( key[i] & mask[j] )
				 {
								  parity ^= 0x01;
					  }
			   }
			   if( parity )
			{
					   key[i] |= parity;
			   }
			   else
			{
					   key[i] &= 0xfe;
		 }
			   parity = 0x01;
  }
}



/****************************************************************************
*  NAME:   int CheckDESKeyWeakness( uchar *key )
*
*  DESCRIPTION: Check if the DES key is weak or semiweak
*
*  INPUTS:
*          PARAMETERS:
*              uchar *key     Pointer to 8-byte key array
*  OUTPUT:
*              None
*          RETURN:
*                SUCCESS          Key is strong
*                ERR_WEAK         Key is weak
*
*  REVISION HISTORY:
*
*  24 Jan 96    KPZ             Initial release
****************************************************************************/
int CheckDESKeyWeakness( uchar *key )
{
	 int i;
/* weak keys */
   uchar weak[16][8] = {{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
						{0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe},
						{0x1f, 0x1f, 0x1f, 0x1f, 0x0e, 0x0e, 0x0e, 0x0e},
						{0xe0, 0xe0, 0xe0, 0xe0, 0xf1, 0xf1, 0xf1, 0xf1},
/* semiweak keys */     {0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe},
						{0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1},
						{0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe},
						{0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e},
						{0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe},
						{0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01},
						{0x1f, 0xe0, 0x1f, 0xe0, 0x0e, 0xf1, 0x0e, 0xf1},
						{0xe0, 0x1f, 0xe0, 0x1f, 0xf1, 0x0e, 0xf1, 0x0e},
						{0x01, 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1},
						{0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1, 0x01},
						{0x01, 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e},
						{0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e, 0x01}
					   };

      SetKeyParity( key );   /* set parity bits */
/* check key weakness*/
     for ( i = 0; i < 16; i++ )
	  {
			   if ( memcmp( key, &weak[i][0], 8 ) == 0 )
               {
                       return ERR_WEAK;
                }
       }
       return SUCCESS;
}

/****************************************************************************
*  NAME:  int DES40KeyExpandCheck( uchar *key,
*                            uchar *expanded_key )
*
*  DESCRIPTION:  Expand 40 bit DES key to 64 and check weakness
*
*  INPUTS:
*      PARAMETERS:
*          uchar *key     Pointer to 40 bit (5 byte) key
*  OUTPUT:
*      PARAMETERS:
*          uchar *expanded_key  Pointer to 64 bit (8 byte) key
*  RETURN:
*                SUCCESS          Key is strong
*                ERR_WEAK         Key is weak
*  REVISION HISTORY:
*
*  24 Jan 96   KPZ     Initial release
*
****************************************************************************/

int DES40KeyExpandCheck( uchar *key,
						 uchar *expanded_key )
{
	expanded_key[0] = (key[0] >> 4) & 0x0f;
	expanded_key[1] = (key[0] << 3) | (key[1] >> 5);
	expanded_key[2] = (key[1] >> 2) & 0x0f;
	expanded_key[3] = (key[1] << 5) | (key[2] >> 3);
	expanded_key[4] = key[2] & 0x0f;
	expanded_key[5] = (key[2] << 7) | (key[3] >> 1);
	expanded_key[6] = ((key[3] << 2) | (key[4] >> 6)) & 0x0f;
	expanded_key[7] = key[4] << 1;

	return (CheckDESKeyWeakness( expanded_key ) );
}


/****************************************************************************
*  NAME:  int SetDES40Key( ushort DH_shared_bytes,
*                             uchar  *DH_shared,
*                             uchar  *K )
*
*  DESCRIPTION:  Set Key by Diffie_Hellman shared number
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_shared_bytes    Length of shared number
*          uchar *DH_shared          Pointer to shared number
*
*  OUTPUT:
*      PARAMETERS:
*              uchar *K               Pointer to 8 byte key
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DATA                Invalid length for DH_shared
*
*  REVISION HISTORY:
*
*  24 Jan 96   KPZ     Initial release
****************************************************************************/
int SetDES40Key( ushort DH_shared_bytes,
					  uchar  *DH_shared,
							 uchar  *K )

{
	 int status = SUCCESS;          /* function return status */
	 ushort j;                      /* counters*/
	 uchar *dh_shared;               /* work buffer*/
	 uchar key[8];
	 uchar *temp;                  /* start pointer to work buffer*/ /*TKL01001*/

	 if ( DH_shared_bytes < 5 )    /* less than minimal required */
	 {
				status = ERR_DH_LEN;         /* invalid length for DH_modulus */
				return status;
	 }
	 dh_shared = (uchar *)malloc( DH_shared_bytes * sizeof(uchar) );
	 temp = dh_shared;   /*TKL010001*/
	 memcpy( dh_shared, DH_shared, DH_shared_bytes );
	 for ( j = 0; j < (ushort)(DH_shared_bytes - 8); j++ )
	 {
		  if ( (status = DES40KeyExpandCheck( dh_shared, key )) == SUCCESS)   /* check key weakness*/
		  {
				memcpy( K, key, 8 );
				dh_shared = temp;   /*TKL01001*/
				free( dh_shared );
				return status;
		  }
		  dh_shared ++;
	 }
	 dh_shared = temp;   /*TKL01001*/
	 free( dh_shared );
	 status = ERR_WEAK;
	 return status;
}



/****************************************************************************
*  NAME:  int SetDES56Key( ushort DH_shared_bytes,
*                          uchar  *DH_shared,
*                          uchar  *K )
*
*  DESCRIPTION:  Set Key by Diffie_Hellman shared number
*
*  INPUTS:
*      PARAMETERS:
*          ushort DH_shared_bytes    Length of shared number
*          uchar *DH_shared          Pointer to shared number
*
*  OUTPUT:
*      PARAMETERS:
*              uchar *K               Pointer to key
*
*      RETURN:
*          SUCCESS                   No errors
*          ERR_DATA                Invalid length for DH_shared
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  24 Jan 96   KPZ     Added semiweak keys check and changed first parameter
****************************************************************************/
int SetDES56Key( ushort DH_shared_bytes,
                    uchar  *DH_shared,
                    uchar  *K )

{
    int status = SUCCESS;          /* function return status */
    ushort j;                      /* counter*/
    uchar *dh_shared;               /* work buffer*/
    uchar *temp;                  /* start pointer to work buffer*/ /*TKL01001*/

    if ( DH_shared_bytes < 8 )    /* less than minimal required */
    {
        status = ERR_DH_LEN;         /* invalid length for DH_modulus */
        return status;
    }
    dh_shared = (uchar *)malloc( DH_shared_bytes*sizeof(uchar) );
    temp = dh_shared;   /*TKL010001*/
    memcpy( dh_shared, DH_shared, DH_shared_bytes );
    for ( j = 0; j <= (ushort)(DH_shared_bytes - 8); j++ )
    {
        if ( (status = CheckDESKeyWeakness( dh_shared )) == SUCCESS)   /* check key weakness*/
        {
            memcpy( K, dh_shared, 8 );
            dh_shared = temp;   /*TKL01001*/
            free( dh_shared );
			return status;
        }
        dh_shared ++;
    }
	dh_shared = temp;   /*TKL01001*/
    free( dh_shared );
	status = ERR_WEAK;
	return status;
}

