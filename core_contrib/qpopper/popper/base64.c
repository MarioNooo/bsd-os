/* Radix Control Constants: Base64 -> 6 bits raw with 8 bit encoding */

#define RADIXBITS               6
#define RADIX       (1<<RADIXBITS)
#define RADIXMASK       (RADIX-1)
#define ALPHABITS               8
#define ALPHASIZE   (1<<ALPHABITS) 
#define ALPHAMASK   (ALPHASIZE-1)


/* Alpha-text encoding of raw bit values (6 bits in Radix 64)  */
/* Final Radix position (65th in Radix 64) has pad value ('=') */

static unsigned char radalpha[RADIX+2]   =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/* Inverse of Alpha-text encoding (set in 1st call to decode64()) */

static int initrev = 1;
static unsigned char revalpha[ALPHASIZE];


/* Encode a string to Base64 representation   */ 
/* Return values:  0  everything cool
                  -1  output buffer too short */
 
extern int encode64( instr, instrlen, outstr, outstrlen )
  char * instr;
  int    instrlen;
  char * outstr;
  int  * outstrlen;
{
  int            i, shiftcnt, maxout, pad;
  unsigned long  rawbits, shiftreg = 0;

  maxout     = *outstrlen;
 *outstrlen  =  0;
  pad        =  0;
  rawbits    =  0;
  shiftcnt   =  0;

  /* Keep going until EOS And all raw bits output */

  for ( i = 0;
          (i < instrlen) || (shiftcnt != 0); i++ ) { 

    if ( i < instrlen )
      rawbits = (unsigned long)((unsigned char)instr[i]);
    else 
      rawbits = 0;  /* fake it - just shift bits out */

    /* Add new raw bits to left end of shift register */

    shiftreg <<= ALPHABITS;   
    shiftcnt  += ALPHABITS;
    shiftreg  |= rawbits;

    /* Keep shifting out while enough bits remain */

    while( shiftcnt >= RADIXBITS ) {
      if ( *outstrlen >= maxout )
        return( -1 );       /* ran out of output buffer */
      else {
        shiftcnt -= RADIXBITS;
        outstr[(*outstrlen)++] =  !pad ?
            radalpha[(int)((shiftreg >> shiftcnt)
                           & (unsigned long)RADIXMASK)]
                                       :
            radalpha[ RADIX ]; /* pad out to even multiple */
      }

      /* Only one alpha output past end of original string */

      pad = (i < instrlen) ? 0 : 1;
    }
  }

  return( 0 );
}



/* Translate alpha text back to raw bit values */
/* Return values:  0  everything cool
                  -1  output buffer too short 
                  -2  illegal input character
                  -3  incomplete input string  */
  
extern int decode64( instr, instrlen, outstr, outstrlen )
  char * instr;
  int    instrlen;
  char * outstr;
  int  * outstrlen;
{
  int           maxout, i;
  unsigned long shiftreg = 0, shiftcnt, rawbits;

  /* Initialize inversion string on 1st call */

  if ( initrev ) {
    (void) memset( revalpha, ALPHAMASK, sizeof(revalpha) );
    for ( i=0; i <= RADIX; i++ )
       revalpha[ (int)radalpha[i] ] = i;
    initrev=0;
  }

  maxout     = *outstrlen;
 *outstrlen  =  0;
  rawbits    =  0;
  shiftcnt   =  0;

  /* Process all alpha up to EOS Or 1st pad character ('=') */

  for ( i = 0; i < instrlen; i++ ) { 
    if ( (rawbits 
             = (unsigned long)
                  revalpha[ (int)((unsigned char)instr[i]) ])
                         < RADIX ) {

      /* Add new raw bits to left end of shift register */

      shiftreg <<= RADIXBITS;
      shiftcnt  += RADIXBITS; 
      shiftreg  |= rawbits;

      /* Output next raw alpha bits if enough ready */

      if ( shiftcnt >= ALPHABITS ) {     
        shiftcnt -= ALPHABITS;
        if( *outstrlen >= maxout )
          return( -1 );           /* No room left for output   */
        else
          outstr[ (*outstrlen)++ ] =
            (char)((shiftreg >> shiftcnt)
                   & (unsigned long)ALPHAMASK); 

      }
    }
    else if ( rawbits == RADIX )   /* Pad     character        */
      return( 0 );
    else
      return( -2 );                /* Illegal character        */
  }

  if ( shiftcnt == 0 )            
    return( 0 );                   /* No pad & no danglng bits */
 
  return( -3 );                    /* Left bits on the table   */
}
