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
 
#include <string.h>
#ifdef NetBSD
#include <sys/types.h>
#endif
/* #include <stdio.h> */
#include "toolkit.h"
#include "cylink.h"

#include <time.h>
void DESModesTest (void );
int DSSFunctionsTest (void);
int DHFunctionsTest (void);
int SFDHFunctionsTest (void );

#pragma align 4 (seed)
long i;
uchar seed[20] = {0xe0, 0x42, 0x7d, 0xbd, 0x62, 0xba, 0x32, 0x24,
								 0xb6, 0x21, 0x1b, 0x40, 0x60, 0xef, 0x2b,0xa8,
								  0xd5, 0x01, 0x4e, 0x4b};
uchar q1[20];
uchar  q[20] = {0xe5,0x81,0x74,0x57,0x7d,0xf7,0x55,0xba,0x92,0x13,
							   0xfc,0x24,0x66,0x0c,0xdf,0x01,0xb1,0xb0,0x0d,0xb2 };

uchar A[24], B[24], C[24];


/*//extern unsigned _stklen = 8192U;*/

int main ()
{
int i;
  printf("\n****** DES TEST ******\n");
   DESModesTest ( );

  printf("\n****** SFDH FUNCTIONS TEST ******\n");
		i = SFDHFunctionsTest ( );
	  if ( i == 0 )
   {
			   printf( "\tTest result is good \n" );
   }
	   else
	{
			   printf( " \tError %d ", i );
	}

   printf("\n****** DSS FUNCTIONS TEST ******\n");
 i = DSSFunctionsTest();
 if ( i == 0 )
   {
			   printf( "\tTest result is good \n" );
   }
	   else
	{
			   printf( " \tError %d ", i );
	}

  printf("\n****** DH FUNCTIONS TEST ******\n");
  i = DHFunctionsTest();
  if ( i == 0 )
   {
			   printf( "\tTest result is good \n" );
   }
	   else
	{
			   printf( " \tError %d ", i );
	}

   printf("\n ****** KEY SHARING FUNCTION TEST *****\n");
   memset(A, 0, 24);
   memset(B, 0, 24);
   memset(C, 0, 24);
   GenShamirTriplet( 20,  q, A, B, C, seed);
   GetNumberShamirDuplex( 20, B, 2, A, 1 , q1);
   if ( memcmp( q, q1, 20) == 0 )
   {
	   printf( "\tTest result is good \n" );
   }
   else
   {
	  printf( "\tTest result is bad\n");
   }
   return 0;
}
