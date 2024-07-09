/* sha.c

Implementation of FIPS 180-1, Secure Hash Standard.

This software package was produced by the National Institute of Standards and
Technology (NIST) as part of its program to promulgate the use of Federal
Information Processing Standards.  This reference software is distributed
within the United States for government and non-commercial use.  Use of this
software is provided soley for demonstration, prototyping, debugging, testing,
and research purposes.  NIST does not support this software and does not
provide any guarantees or warranties as to its performance or fitness for any
particular application.  NIST does not accept any liability associated with
its use or misuse.  By accepting this software the user agrees to the terms
stated herein.

Implementations of cryptography are subject to United States Federal
Government export controls.  Export control regulations are specified in
Title 22, Code of Federal Regulations (CFR), Parts 120-128, the International
Traffic in Arms Regulations (ITAR) and in Title 15, Code of Federal
Regulations, Parts 768-799.

General Export Licensing Symbols for this Code: GTDR, GRX

*/

#include <string.h>
#include "endian.h"
#include "toolkit.h"

/* #define DEBUGSHA */

/* The circular shifts. */

#define CS5(x)  ((((ULONG)x)<<5)|(((ULONG)x)>>27))
#define CS30(x)  ((((ULONG)x)<<30)|(((ULONG)x)>>2))

/* K constants */

#define K0  0x5a827999L
#define K1  0x6ed9eba1L
#define K2  0x8f1bbcdcL
#define K3  0xca62c1d6L

/*#define OLDF1
#define OLDF3 */

#ifdef OLDF1
/* Rounds  0-19, 60-79 */
#define f1(x,y,z)   ( ( x & y ) | ( ~x & z ) )
#else
#define f1(x,y,z)   ((x & (y ^ z)) ^ z)
#endif

#ifdef OLDF3
/* Rounds 40-59 */
#define f3(x,y,z)   ( ( x & y ) | ( x & z ) | ( y & z ) )   
#else
#define f3(x,y,z)   ((x & ( y ^ z )) ^ (z & y))   
#endif

#define f2(x,y,z)   ( x ^ y ^ z )                           /* Rounds 20-39 */

/* The initial expanding function */
/* This update fixes a security problem with the orignal version of the SHA [FIPS 180] */
/*
 * DNH: this was a #define SHA_UPDATE 
 */
#define CS1(x) ((((ULONG)x)<<1)|(((ULONG)x)>>31))
#define  expand(x)  Wbuff[x%16] = CS1(Wbuff[(x - 3)%16 ] ^ Wbuff[(x - 8)%16 ] ^ Wbuff[(x - 14)%16] ^ Wbuff[x%16])
/* 
#else 
#define  expand(x)  Wbuff[x%16] = Wbuff[(x - 3)%16 ] ^ Wbuff[(x - 8)%16 ] ^ Wbuff[(x - 14)%16] ^ Wbuff[x%16]
#endif  SHA_UPDATE
*/

/* Functions used in one of the 4 Rounds */

#define sub1Round1(count)      { \
    temp = CS5(A) + f1(B, C, D) + E + Wbuff[count] + K0; \
    E = D; \
    D = C; \
    C = CS30( B ); \
    B = A; \
    A = temp; \
    } \

#define sub2Round1(count)   \
    { \
    expand(count); \
    temp = CS5(A) + f1(B, C, D) + E + Wbuff[count%16] + K0; \
    E = D; \
    D = C; \
    C = CS30( B ); \
    B = A; \
    A = temp; \
    } \

#define Round2(count)     \
    { \
    expand(count); \
    temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K1;  \
    E = D; \
    D = C; \
    C = CS30( B ); \
    B = A; \
    A = temp;  \
    } \

#define Round3(count)    \
    { \
    expand(count); \
    temp = CS5( A ) + f3( B, C, D ) + E + Wbuff[count%16] + K2; \
    E = D; \
    D = C; \
    C = CS30( B ); \
    B = A; \
    A = temp; \
    }

#define Round4(count)    \
    { \
    expand(count); \
    temp = CS5( A ) + f2( B, C, D ) + E + Wbuff[count%16] + K3; \
    E = D; \
    D = C; \
    C = CS30( B ); \
    B = A; \
    A = temp; \
    }

/***********************************************************************
	This is the routine that implements the SHA.
***********************************************************************/
static void
ProcessBlock(SHA_CTX *shaContext)
{
	ULONG  A, B, C, D, E, temp, Wbuff[16];
	register int j;

	A = shaContext->buffer[0];
	B = shaContext->buffer[1];
	C = shaContext->buffer[2];
	D = shaContext->buffer[3];
	E = shaContext->buffer[4];

        for (j = 0; j < 16;j++) 
          Wbuff[j] = shaContext->Mblock[j]; 

#ifdef DEBUGSHA
printf("a = %lx, b = %lx, c = %lx, d = %lx, e = %lx\n",A,B,C,D,E);
#endif

    sub1Round1( 0 ); sub1Round1( 1 ); sub1Round1( 2 ); sub1Round1( 3 );
    sub1Round1( 4 ); sub1Round1( 5 ); sub1Round1( 6 ); sub1Round1( 7 );
    sub1Round1( 8 ); sub1Round1( 9 ); sub1Round1( 10 ); sub1Round1( 11 );
    sub1Round1( 12 ); sub1Round1( 13 ); sub1Round1( 14 ); sub1Round1( 15 );
    sub2Round1( 16 ); sub2Round1( 17 ); sub2Round1( 18 ); sub2Round1( 19 );

#ifdef DEBUGSHA
printf("a = %lx, b = %lx, c = %lx, d = %lx, e = %lx\n",A,B,C,D,E);
#endif

    Round2( 20 ); Round2( 21 ); Round2( 22 ); Round2( 23 );
    Round2( 24 ); Round2( 25 ); Round2( 26 ); Round2( 27 );
    Round2( 28 ); Round2( 29 ); Round2( 30 ); Round2( 31 );
    Round2( 32 ); Round2( 33 ); Round2( 34 ); Round2( 35 );
    Round2( 36 ); Round2( 37 ); Round2( 38 ); Round2( 39 );

#ifdef DEBUGSHA
printf("a = %lx, b = %lx, c = %lx, d = %lx, e = %lx\n",A,B,C,D,E);
#endif

    Round3( 40 ); Round3( 41 ); Round3( 42 ); Round3( 43 );
    Round3( 44 ); Round3( 45 ); Round3( 46 ); Round3( 47 );
    Round3( 48 ); Round3( 49 ); Round3( 50 ); Round3( 51 );
    Round3( 52 ); Round3( 53 ); Round3( 54 ); Round3( 55 );
    Round3( 56 ); Round3( 57 ); Round3( 58 ); Round3( 59 );

#ifdef DEBUGSHA
printf("a = %lx, b = %lx, c = %lx, d = %lx, e = %lx\n",A,B,C,D,E);
#endif
    Round4( 60 ); Round4( 61 ); Round4( 62 ); Round4( 63 );
    Round4( 64 ); Round4( 65 ); Round4( 66 ); Round4( 67 );
    Round4( 68 ); Round4( 69 ); Round4( 70 ); Round4( 71 );
    Round4( 72 ); Round4( 73 ); Round4( 74 ); Round4( 75 );
    Round4( 76 ); Round4( 77 ); Round4( 78 ); Round4( 79 );

#ifdef DEBUGSHA
printf("a = %lx, b = %lx, c = %lx, d = %lx, e = %lx\n",A,B,C,D,E);
#endif

    shaContext->buffer[0] += A;
    shaContext->buffer[1] += B;
    shaContext->buffer[2] += C;
    shaContext->buffer[3] += D;
    shaContext->buffer[4] += E;
}/* End ProcessBlock */

#ifdef L_ENDIAN
ByteReverse( buffer, byteCount )
ULONG *buffer;
int byteCount;
    {
    ULONG value;
    int count;

    byteCount /= sizeof( ULONG );
    for( count = 0; count < byteCount; count++ )
	{
	value = ( buffer[ count ] << 16 ) | ( buffer[ count ] >> 16 );
	buffer[ count ] = ( ( value & 0xFF00FF00L ) >> 8 ) | ( ( value & 0x00FF00FFL ) << 8 );
	}
    }
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/**********************************************************************
    SHA initialization routine.
**********************************************************************/

int fipsSHAInit(SHA_CTX *shaContext)
{
    ULONG IH[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    int i;

    shaContext->Numblocks[0] = 0;
    shaContext->Numblocks[1] = 0;
    shaContext->Numbytes = 0;
    for (i=0; i<16; i++)
	shaContext->Mblock[i] = (ULONG) 0L;
    for (i=0; i<5 ;i++)
	shaContext->buffer[i] = IH[i];

return 0;
}

/**********************************************************************
  SHAUpdate
**********************************************************************/
int fipsSHAUpdate(SHA_CTX *shaContext, BYTE *buffer, int bytecount)
{
  bytecount += shaContext->Numbytes;

  while ( bytecount >= SHABLOCKLEN ) {
    /* Process full block now */
    if (shaContext->Numblocks[1] == 0xffffffff)
      {
	shaContext->Numblocks[0]++;          
	shaContext->Numblocks[1] = 0L;          
      }
    else 
      shaContext->Numblocks[1]++;

    bcopy(buffer, (BYTE *) (shaContext->Mblock) + shaContext->Numbytes, SHABLOCKLEN-shaContext->Numbytes);

#ifdef L_ENDIAN
    ByteReverse(shaContext->Mblock, SHABLOCKLEN);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

    ProcessBlock(shaContext);
    buffer += SHABLOCKLEN - shaContext->Numbytes;
    bytecount -= SHABLOCKLEN;
    shaContext->Numbytes = 0; 
  }

  if (bytecount) {
    /* Save partial block */
    bcopy(buffer, (BYTE *) (shaContext->Mblock) + shaContext->Numbytes, bytecount-shaContext->Numbytes);
    shaContext->Numbytes = bytecount;
  }

  return 0;
}

/**********************************************************************
    SHAFinal does the hashing of the last block of the message. 
    It is this routine that does the necessary padding of zeros
    and the length of the data at the end.
**********************************************************************/
int fipsSHAFinal(SHA_CTX *shaContext, BYTE *result)
{
    ULONG tempNumbytes;

    tempNumbytes = shaContext->Numbytes;

    if (shaContext->Numbytes < 56) {
       /* append the 1 bit */
       ((BYTE *)(shaContext->Mblock))[tempNumbytes++] = 0x80;

	/* pad to 56 bytes with zeros */
        while (tempNumbytes < 56)
          ((BYTE *)(shaContext->Mblock))[tempNumbytes++] = 0;
    } 
    else {
        /* append the 1 bit */
        ((BYTE *)(shaContext->Mblock))[tempNumbytes++] = 0x80;

	/* pad to 64 bytes with zeros and process block.
           Then pad the next block to 56 bytes. */

        while (tempNumbytes < 64)
          ((BYTE *)(shaContext->Mblock))[tempNumbytes++] = 0;

#ifdef L_ENDIAN
	ByteReverse(shaContext->Mblock, SHABLOCKLEN);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

	ProcessBlock(shaContext);
	tempNumbytes = 0;
	while (tempNumbytes < 56)
          ((BYTE *)(shaContext->Mblock))[tempNumbytes++] = 0;
      }

#ifdef L_ENDIAN
    ByteReverse(shaContext->Mblock, SHABLOCKLEN-8);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

    /* Fill the length byte. */
    shaContext->Mblock[14] = (ULONG) ( (shaContext->Numblocks[0] << 6) 
				      + (shaContext->Numblocks[1] >> 26)) * 8;
    shaContext->Mblock[15] = (ULONG)((shaContext->Numblocks[1] << 6)
                                   +shaContext->Numbytes)*8;

    ProcessBlock(shaContext);
#ifdef L_ENDIAN
    ByteReverse(shaContext->buffer, SHA_LENGTH);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
    memcpy(result, shaContext->buffer, SHA_LENGTH);

    return 0;
}
