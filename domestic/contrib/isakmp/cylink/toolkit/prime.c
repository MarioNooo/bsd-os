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
*  FILENAME:  prime.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:  Cryptographic Toolkit Internal Functions File
*               Prime Number functions
*  PRIVATE FUNCTIONS:
*
*               int VerPrime( ushort P_bytes, ord *P,
*                             ushort k, ord *RVAL,
*                             YIELD_context *yield_cont )
*               int GenPrime( ushort P_bytes, ord *P,
*                             ushort k, ord *RVAL,
*                             YIELD_context *yield_cont )
*       Copyright (c) Cylink Corporation 1996. All rights reserved.
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
/* #include <stdio.h> */
#endif

/* program files */
#include "toolkit.h"
#include "cencrint.h"

/* bignum include files */
#include "bn.h"

/****************************************************************************
*  PRIVATE FUNCTIONS DEFINITIONS
****************************************************************************/
/****************************************************************************
*  NAME: int VerPrime( ushort P_bytes,
*                      ord    *P,
*                      ushort k,
*                      ord    *RVAL,
*                                         YIELD_context *yield_cont )
*
*  DESCRIPTION: Verify Pseudo Prime number
*
*  INPUTS:
*      PARAMETERS:
*            ushort P_bytes     Number of bytes in array
*            ushort k           Number of testing
*            ord    *RVAL       Pointer to random number generator value
*            YIELD_context *yield_cont  Pointer to yield_cont structure (NULL if not used)
*  OUTPUT:
*      PARAMETERS:
*            ord   *P           Pointer to prime number
*            ord   *RVAL        Pointer to updated value
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_PRIME            Number is not prime
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  08 Dec 94   GKL    Added YIELD_context
*
****************************************************************************/

 int VerPrime( ushort P_bytes,
						 ord    *P,
					  ushort k,
					   ord    *RVAL,
						   YIELD_context *yield_cont ) /*TKL00601*/

{
	int status = SUCCESS;      /*function return status*/
   ord *b, *n, *c, *pp;       /*pointers to working buffers */
	 ord *m, *buf, *P_buf;
   ushort max_bytes;          /*number of maximum bytes*/
  ushort P_longs;            /*number of longs in Y*/
	 ushort i, j, s, k_b;       /*counters*/
 ushort exit ;              /*flag for exit*/
/* product of prime numbers from 3 to 379 (512-bit number) */
#ifdef ORD_32
	ord b1[DSS_LENGTH_MIN/sizeof(ord)]={
		  0x2e30c48fL, 0x0decece9L, 0xbada8f98L, 0x9f7ecb29L,
             0xa4a11de4L, 0x6ef04659L, 0xcbc38405L, 0x233db117L,
             0x03e81187L, 0xc1b631a2L, 0x238bfb99L, 0x077ec3baL,
             0xc5d5f09fL, 0xb0813c28L, 0x7646fa6eL, 0x106aa9fbL };
#else
	ord b1[DSS_LENGTH_MIN/sizeof(ord)]={
              0xc48f, 0x2e30, 0xece9, 0x0dec, 0x8f98, 0xbada, 0xcb29, 0x9f7e,
         0x1de4, 0xa4a1, 0x4659, 0x6ef0, 0x8405, 0xcbc3, 0xb117, 0x233d,
         0x1187, 0x03e8, 0x31a2, 0xc1b6, 0xfb99, 0x238b, 0xc3ba, 0x077e,
		 0xf09f, 0xc5d5, 0x3c28, 0xb081, 0xfa6e, 0x7646, 0xa9fb, 0x106a };
#endif
	if ( P_bytes % sizeof(ord) != 0 )                /* not multiple 4 (32 bit) */
    {
          status = ERR_INPUT_LEN;          /* invalid length for input data */
			return status;
    }
	if ( P_bytes <= DSS_LENGTH_MIN )
    {
			max_bytes = DSS_LENGTH_MIN;
	}
	else
    {
		   max_bytes = P_bytes;
    }
	buf = (ord *)calloc( max_bytes / sizeof(ord), sizeof(ord) );
	P_buf = (ord *)calloc( max_bytes / sizeof(ord), sizeof(ord));
	if( !buf || !P_buf )
    {
 if ( buf )
              free( buf );
    if( P_buf )
			 free( P_buf );
  return ERR_ALLOC;
	}
	memcpy( buf, b1, DSS_LENGTH_MIN );
	memcpy( P_buf, P, P_bytes  );

	if ( (P_buf[0] & 0x1) == 0 )
    {
#ifdef DEBUG1
         printf ("\n P is not pseudoprime");
#endif
			   status = ERR_PRIME;
             free ( buf );
		   free( P_buf );
          return status;
    }
	P_longs = P_bytes / sizeof(ord);
	b = (ord *)calloc( P_longs, sizeof(ord) );
	m = (ord *)malloc( P_longs * sizeof(ord) );
	n = (ord *)calloc( P_longs, sizeof(ord) );
	c = (ord *)calloc( P_longs, sizeof(ord) );
	pp = (ord *)malloc( P_longs * sizeof(ord) );
	if( !b || !m || !n || !c || !pp )
    {
	  if( b )
         free( b );
      if( m )
         free( m );
      if( n )
         free( n );
      if( c )
         free( c );
      if( pp )
				free ( pp );
	}
	memcpy( m, P, P_bytes );
	memcpy( pp, P, P_bytes );
    /* Compute great common divisor(gcd) */
	if ( SteinGCD( P_buf, buf, (ushort)(max_bytes / sizeof(ord)) ) == 0 )
  {
        pp[0] = pp[0] - 1;                     /* Initialized work buffer */
        m[0] = m[0] - 1;
           s = RShiftMostBit( m, (ushort)(P_bytes / sizeof(ord)) ); /* Right shift by number of*/
        exit = 0;                               /* zero bits at rigth    */
		k_b = 0;
        while( k_b != k )
		{
                     MyGenRand( 4, b, RVAL );    /* generate random number */
            if ( SteinGCD( P_buf, b , (ushort)(P_bytes / sizeof(ord)) ) ) /* check gcd */
            {
#ifdef DEBUG1
							 printf ("\n P is not pseudoprime");
#endif
							   status = ERR_PRIME;
                             break;
			}
            k_b++;                          /* increment counter */
		  if ( ( status = Expo ( 4, b, P_bytes, m,
                                P_bytes, P, c, yield_cont) ) != SUCCESS )    /* c=b^m mod(P) */ /*TKL00601*/
                    {
                               free( b );
                              free( m );
                              free( n );
                              free( c );
                              free( pp );
                             free (buf );
							free( P_buf );
                          return status;
				  }
                       if ( c[0] == 1 )    /* if c==1 number is pseudo prime */
						{
                               for ( i = 1; i < P_bytes / sizeof(ord); i++ )
                {
                    if ( c[i] != 0 )
                    {
                                         break;
                                  }
							   }
                               if ( i == P_bytes / sizeof(ord) )
							   {
                                       if (yield_cont)  /*TKL00601*/
#ifdef VXD
                                         if ( VXD_Yield (yield_cont->yield_proc) )
#else
                                          if ( yield_cont->yield_proc(0xFFFF) )
#endif

											 {
													   status = ERR_CANCEL;
                                                    free( b );
													  free( m );
                                                      free( n );
                                                      free( c );
                                                      free( pp );
                                                     free( P_buf );
                                                  free (buf );
                                                    return status;
                                          }
#ifdef DEBUG1
								  printf ("\n P is a pseudoprime %d",k_b);
#endif
								  if ( k_b == k )
								 {
                                               break;
								  }
                               }
            }
                  else
            {
                for ( j = 1; j <= s; j++ )
				{
                    for ( i = 0; i < P_bytes / sizeof(ord); i++ )    /* if c==pp number is pseudo prime */
					{
                                             if ( c[i] != pp[i] )
                        {
                                           break;
                                          }
                                       }
                                       if ( i == P_bytes / sizeof(ord) )
                                       {
                                               if (yield_cont) /*TKL00601*/
#ifdef VXD
                                          if ( VXD_Yield (yield_cont->yield_proc) )
#else
                                                  if ( yield_cont->yield_proc(0xFFFF) )
#endif

                                                     {
                                                               status = ERR_CANCEL;
                                                            free( b );
                                                              free( m );
                                                              free( n );
															  free( c );
                                                              free( pp );
															 free( P_buf );
                                                          free (buf );
                                                            return status;
                                                  }
#ifdef DEBUG1
                                          printf ("\n P is a pseudoprime %d",k_b);
#endif
                                          break;
                                  }
									   if ( j == s )
                                   {
#ifdef DEBUG1
                                          printf ("\n P is not pseudoprime");
#endif
                                               status = ERR_PRIME;
                                             exit = 1;
                                               break;
                                  }
                                       else
                                    {
											   if ( (status = Square(P_bytes, c, /*P_bytes,
																					c,*/ P_bytes, P, c) )
																   != SUCCESS )     /* c=c^2mod(p) */
                                              {
                                                       free( b );
                                                      free( m );
                                                      free( n );
                                              free( c );
                                              free( pp );
                                             free( P_buf );
                                          free (buf );
											return status;
                                          }
									   }
                }
                      }
            if ( exit == 1 )            /* Exit */
            {
                                break;
                  }
               }
       }
	   else
    {
			   if (yield_cont)  /*TKL00601*/
#ifdef VXD
         if ( VXD_Yield (yield_cont->yield_proc) )
#else
          if ( yield_cont->yield_proc(0xFFFF) )
#endif

                     {
                               status = ERR_CANCEL;
							free( b );
                              free( m );
							  free( n );
                              free( c );
                              free( pp );
                             free( P_buf );
                          free (buf );
                            return status;
                  }
#ifdef DEBUG1
          printf ("\n P is not pseudoprime");
#endif
			   status = ERR_PRIME;
	 }
	   free( b );
	  free( m );
	  free( n );
	  free( c );
	  free( pp );
	 free( P_buf );
  free (buf );
	return status;
}

/****************************************************************************
*  NAME: int GenPrime( ushort P_bytes,
*                      ord    *P,
*                      ushort k,
*                      ord    *RVAL,
*                                           YIELD_context *yield_cont )
*
*  DESCRIPTION: Generate Random Pseudo Prime number
*
*  INPUTS:
*      PARAMETERS:
*            ushort P_bytes     Number of bytes in array
*            ushort k           Number of testing
*            ord    *RVAL       Pointer to random number generator value
*            YIELD_context *yield_cont  Pointer to yield_cont structure (NULL if not used)
*  OUTPUT:
*      PARAMETERS:
*            ord   *P           Pointer to prime number
*            ord   *RVAL        Pointer to updated value
*
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_PRIME            Number is not prime
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  08 Dec 94   GKL     Added YIELD_context
*
****************************************************************************/

 int GenPrime( ushort P_bytes,
					   ord    *P,
                      ushort k,
                       ord    *RVAL,
                           YIELD_context *yield_cont ) /*TKL00601*/
{
    int status = SUCCESS;      /* function return status */
 if ( P_bytes % sizeof(ord) != 0 )    /* not multiple 4 (32 bit) */
      {
               status = ERR_INPUT_LEN;          /* invalid length for input data */
			return status;
  }
	   do
      {
               MyGenRand( P_bytes, P, RVAL );    /* generate random number */
          P[0] |= 1;
              P[(P_bytes/sizeof(ord))-1] |= ((ord)1 << (BITS_COUNT-1));
               status = VerPrime( P_bytes, P, k, RVAL, yield_cont); /*TKL00601*/
       } while ((status != SUCCESS) && (status != ERR_CANCEL)); /*TKL00601*/
   return status;
}

