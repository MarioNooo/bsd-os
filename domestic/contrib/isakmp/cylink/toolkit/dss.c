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
*  FILENAME:  dss.c   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  FILE STATUS:
*
*  DESCRIPTION:     Cryptographic Toolkit Functions File
*                   Digital Signature Algorithms
*  PUBLIC FUNCTIONS:
*                                   uchar  *RVAL )
*      int GenDSSKey( ushort dss_p_bytes, uchar  *dss_p,
*                     uchar *dss_q,       uchar  *dss_g,
*                     uchar  *dss_x,      uchar  *dss_y,
*                                         uchar  *XKEY )
*
*      int GenDSSNumber( uchar *dss_k,   uchar dss_q,
*                                        uchar *KKEY )
*       int GenDSSParameters( ushort dss_p_bytes, uchar  *dss_p,
*                              uchar  *dss_q,      uchar  *dss_g,
*                                                  uchar  *RVAL );
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
#define  INITIALIZ_PQG   /*TKL01101*/
#include "dssnum.h"      /*TKL01101*/
#include "sha.h"

#define  BEGIN_PROCESSING do {
#define  END_PROCESSING  } while (0);
#define  ERROR_BREAK break
#define  CONTINUE continue

#define  BEGIN_LOOP do {
#define  END_LOOP  } while (1);
#define  BREAK break

/****************************************************************************
*  NAME:  int GenDSSSignature( ushort dss_p_bytes,
*                              uchar  *dss_p,
*                              uchar  *dss_q,
*                              uchar  *dss_g,
*                              uchar  *dss_x,
*                              uchar  *dss_k,
*                              uchar  *r,
*                              uchar  *s,
*                              uchar  *hash_result)
*
*  DESCRIPTION:  Compute a DSS Signature
*
*  INPUTS:
*      PARAMETERS:
*          ushort dss_p_bytes  Length of dss_p
*          uchar *dss_p        Pointer to p prime
*          uchar *dss_q        Pointer to q prime
*          uchar *dss_g        Pointer to g
*          uchar *dss_x        Pointer to secret number
*          uchar *dss_k        Pointer to random secret number
*          uchar *hash_result  Pointer to message hashing result
*
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *r           Pointer to r part of signature
*          uchar *s           Pointer to s part of signature
*
*      RETURN:
*          E_SUCCESS         No errors
*          E_DSS_LEN         Invalid length for dss_p
*          ERR_ALLOC         Insufficient memory
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL     Changed Expo call
*
****************************************************************************/

int GenDSSSignature( ushort dss_p_bytes,
                                   uchar  *dss_p,
                                  uchar  *dss_q,
                                  uchar  *dss_g,
                                  uchar  *dss_x,
                                  uchar  *dss_k,
                                  uchar  *r,
                  uchar  *s,
              uchar  *hash_result)

{
    int  status = SUCCESS;          /* function return status */
	ord r_temp[DSS_LENGTH_MAX];      /* r intermidiate value */
    ord k_inverse[SHA_LENGTH+1]; 
    ord temp[SHA_LENGTH];            /* intermidiate values    */
	ord *dss_p_a;
    ord *dss_g_a;
    ord *dss_q_a;
    ord *dss_x_a;
	ord *dss_k_a;
	ord *hash_result_a;
	ord *r_a;
	ord *s_a;

 if ( (dss_p_bytes < DSS_LENGTH_MIN) ||     /* less than minimal */
      (dss_p_bytes > DSS_LENGTH_MAX) )      /* more than maximal */
 {
        status = ERR_DSS_LEN;           /* invalid length for dss_p */
  return status;
 }
 if ( dss_p_bytes & 0x07 )          /* not multiple 8 (64 bit)*/
 {
     status = ERR_DSS_LEN;          /* invalid length for dss_p */
     return status;
 }

/* ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);
 ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);
 ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);*/
 DSS_G_ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);  /*TKL01101*/
 DSS_P_ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);  /*TKL01101*/
 DSS_Q_ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);   /*TKL01101*/

 ALIGN_CALLOC_COPY(dss_x, dss_x_a, SHA_LENGTH);
 ALIGN_CALLOC_COPY(dss_k, dss_k_a, SHA_LENGTH);
 ALIGN_CALLOC_COPY(hash_result, hash_result_a, SHA_LENGTH);
 ALIGN_CALLOC(r, r_a, SHA_LENGTH);
 ALIGN_CALLOC(s, s_a, SHA_LENGTH);
 if ( status !=  SUCCESS )
 {
      if( dss_p_a )
     DSS_ALIGN_FREE(dss_p_a,dss_p);
        if( dss_g_a )
     DSS_ALIGN_FREE(dss_g_a,dss_g);
        if( dss_q_a )
     DSS_ALIGN_FREE(dss_q_a,dss_q);
        if( dss_x_a )
   {
		 memset ( dss_x_a , 0 , SHA_LENGTH );
    ALIGN_FREE(dss_x_a);
  }
       if( dss_k_a )
   {
         ALIGN_FREE(dss_k_a);
  }
       if( hash_result_a )
     {
         ALIGN_FREE(hash_result_a);
    }
       if( r_a )
       {
         ALIGN_COPY_FREE(r_a,r,SHA_LENGTH);
    }
       if( s_a )
       {
		 ALIGN_COPY_FREE(s_a,s,SHA_LENGTH);
    }
         return status;     /* ERR_ALLOC   insufficient memory */
 }
/* Compute DSS r value */
 BEGIN_PROCESSING
    if (( status = Expo ( dss_p_bytes,
                      dss_g_a,
                        SHA_LENGTH,   /* r_temp=(dss_g^dss_k)mod(dss_p)*/
                                               dss_k_a,
                                                dss_p_bytes,
                                            dss_p_a,
                                                r_temp,
                                                 NULL )) != SUCCESS ) /*TKL00601*/
     {
                       ERROR_BREAK;
    }
	   if (( status = PartReduct ( dss_p_bytes,
                                                                r_temp,
                                                         SHA_LENGTH,      /* r=(r_temp)mod(dss_q) */
                             dss_q_a,
                                r_a )) != SUCCESS )
    {
            ERROR_BREAK;
    }
/* Compute k modulo inverse value */
    if (( status = Inverse( SHA_LENGTH,  /* k_inverse=dss_k^(-1)mod(dss_q)*/
                       dss_k_a,
                                                    SHA_LENGTH,
                         dss_q_a,
                        k_inverse )) != SUCCESS  )
    {
         ERROR_BREAK;
    }
/* Compute DSS s value */
    if (( status = Mul ( SHA_LENGTH,    /* temp=(dss_x*r)mod(dss_q) */
                     dss_x_a,
                                                SHA_LENGTH,
                     r_a,
                    SHA_LENGTH,
                     dss_q_a,
                        temp )) != SUCCESS )
    {
          ERROR_BREAK;
    }

    Add( temp, hash_result_a,
       SHA_LENGTH, dss_q_a, temp );  /* temp=(temp+hash_result)mod(dss_q)*/

    if (( status = Mul ( SHA_LENGTH, /* s=(temp*k_inverse)mod(dss_q) */
                    temp,
				   SHA_LENGTH,
                     k_inverse,
                      SHA_LENGTH,
                     dss_q_a,
                        s_a )) != SUCCESS )
    {
            ERROR_BREAK;
    }
  END_PROCESSING
/*
  ALIGN_FREE(dss_p_a);
  ALIGN_FREE(dss_g_a);
  ALIGN_FREE(dss_q_a);
*/
  DSS_ALIGN_FREE(dss_p_a,dss_p);  /*TKL01101*/
  DSS_ALIGN_FREE(dss_g_a,dss_g);  /*TKL01101*/
  DSS_ALIGN_FREE(dss_q_a,dss_q);  /*TKL01101*/
  ALIGN_FREE(dss_x_a);
  ALIGN_FREE(dss_k_a);
  ALIGN_FREE(hash_result_a);
  ALIGN_COPY_FREE(r_a,r,SHA_LENGTH);
  ALIGN_COPY_FREE(s_a,s,SHA_LENGTH);
  return status;
}

/****************************************************************************
*  NAME:  int VerDSSSignature( ushort dss_p_bytes,
*                              uchar  *dss_p,
*                              uchar  *dss_q,
*                              uchar  *dss_g,
*                              uchar  *dss_y,
*                              uchar  *r,
*                              uchar  *s,
*                              uchar  *hash_result)
*
*  DESCRIPTION:  Verify a DSS Signature
*
*  INPUTS:
*      PARAMETERS:
*          ushort dss_p_bytes      Length of dss_p
*          uchar *dss_p            Pointer to p prime
*          uchar *dss_q            Pointer to q prime
*          uchar *dss_g            Pointer to g
*          uchar *dss_y            Pointer to public number
*          uchar *hash_result      Pointer to message hashing result
*  OUTPUT:
*      PARAMETERS:
*
*      RETURN:
*          SUCCESS                  No errors
*          ERR_SIGNATURE            Signature is not valid
*          ERR_DSS_LEN              Invalid length for dss_p
*          ERR_ALLOC                Insufficient memory
*
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL     Changed Expo call
*
****************************************************************************/

int VerDSSSignature( ushort dss_p_bytes,
                                      uchar  *dss_p,
                                  uchar  *dss_q,
                                  uchar  *dss_g,
                                  uchar  *dss_y,
                                  uchar  *r,
                                      uchar  *s,
                                      uchar  *hash_result)
{
  int  status = SUCCESS;          /* function return status */
    ord  w[(SHA_LENGTH / sizeof(ord)) + 1];
    ord u1[SHA_LENGTH / sizeof(ord)];
    ord u2[SHA_LENGTH / sizeof(ord)];
    ord *uu1;
    ord *uu2;
    ord *v;
    ord *dss_p_a;
    ord *dss_g_a;
    ord *dss_q_a;
    ord *dss_y_a;
    ord *hash_result_a;
    ord *r_a;
    ord *s_a;
    if ( (dss_p_bytes < DSS_LENGTH_MIN) ||     /* less than minimal */
         (dss_p_bytes > DSS_LENGTH_MAX) )      /* more than maximal */
    {
            status = ERR_DSS_LEN;           /* invalid length for dss_p */
                        return status;
    }
     if ( dss_p_bytes & 0x07 )          /* not multiple 8 (64 bit)*/
    {
    status = ERR_DSS_LEN;          /* invalid length for dss_p */
   return status;
    }
/*    ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);
    ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);
    ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);*/
    DSS_P_ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);  /*TKL01101*/
    DSS_Q_ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);   /*TKL01101*/
    DSS_G_ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);  /*TKL01101*/

    ALIGN_CALLOC_COPY(dss_y, dss_y_a, dss_p_bytes);
    ALIGN_CALLOC_COPY(hash_result, hash_result_a, SHA_LENGTH);
    ALIGN_CALLOC_COPY(r, r_a, SHA_LENGTH);
    ALIGN_CALLOC_COPY(s, s_a, SHA_LENGTH);

    CALLOC(uu1,ord,dss_p_bytes);
    CALLOC(uu2,ord,dss_p_bytes);
    CALLOC(v,ord,dss_p_bytes);

    if ( status !=  SUCCESS )
    {
       if( dss_p_a )
   {
         DSS_ALIGN_FREE(dss_p_a,dss_p);
        }
       if( dss_g_a )
   {
         DSS_ALIGN_FREE(dss_g_a,dss_g);
        }
       if ( dss_q_a )
  {
         DSS_ALIGN_FREE(dss_q_a,dss_q);
        }
       if( dss_y_a )
   {
                ALIGN_FREE(dss_y_a);
   }
       if( hash_result_a )
     {
                ALIGN_FREE(hash_result_a);
      }
      if( r_a )
       {
               ALIGN_FREE(r_a);
        }
       if( s_a )
       {
               ALIGN_FREE(s_a);
        }
       if( uu1 )
       {
               free ( uu1 );
    }
      if( uu2 )
       {
               free ( uu2 );
   }
       if( v )
 {
               free ( v );
   }
          return status;     /* ERR_ALLOC   insufficient memory */
    }
 BEGIN_PROCESSING
    if (( status = Inverse( SHA_LENGTH,     /* w=dss_k^(-1)mod(dss_q)*/
                            s_a,
                            SHA_LENGTH,
                            dss_q_a,
                            w )) !=SUCCESS  )
    {
        ERROR_BREAK;
    }
    if (( status = Mul ( SHA_LENGTH,    /* u1=(hash_result_*w)mod(dss_q) */
                         hash_result_a,
                   SHA_LENGTH,
                         w,
                  SHA_LENGTH,
                         dss_q_a,
                    u1 )) != SUCCESS )
    {
        ERROR_BREAK;
    }
    if (( status = Mul ( SHA_LENGTH,    /* u2=(r*w)mod(dss_q) */
                         r_a,
                         SHA_LENGTH,
                         w,
                         SHA_LENGTH,
                         dss_q_a,
                       u2 )) != SUCCESS )
    {
        ERROR_BREAK;
    }
    if (( status = Expo ( dss_p_bytes,     /*uu1 = dss_g^u1 mod(dss_p)*/
                          dss_g_a,
                          SHA_LENGTH,
                          u1,
                          dss_p_bytes,
                          dss_p_a,
                                             uu1, NULL )) != SUCCESS ) /*TKL00601*/
        {
               ERROR_BREAK;
    }
       if (( status = Expo ( dss_p_bytes,       /*uu2 = y^u2 mod(dss_p)*/
                                                dss_y_a,
                                                SHA_LENGTH,
                                             u2,
                                             dss_p_bytes,
                                            dss_p_a,
                                                uu2, NULL)) != SUCCESS ) /*TKL00601*/
 {
               ERROR_BREAK;
    }
       if (( status = Mul ( dss_p_bytes,   /* v =(uu1*uu2) mod(dss_p) */
                                                uu1,
                                            dss_p_bytes,
                                            uu2,
                                            dss_p_bytes,
                                            dss_p_a,
                                                v )) != SUCCESS )
      {
               ERROR_BREAK;
    }
       if (( status = PartReduct ( dss_p_bytes,         /*v = v mod(dss_q)*/
                                                           v,
                                                              SHA_LENGTH,
                                                             dss_q_a,
                                                                v )) != SUCCESS )
       {
               ERROR_BREAK;
    }
       if (( status = memcmp( r_a, v, SHA_LENGTH)) != 0)   /*if v=r sign valid */
      {
               status = ERR_SIGNATURE;             /* signature is not valid */
                ERROR_BREAK;
    }
 END_PROCESSING
   free ( uu1 );
   free ( uu2 );
   free ( v );
/*
    ALIGN_FREE(dss_p_a);
    ALIGN_FREE(dss_g_a);
    ALIGN_FREE(dss_q_a);
*/
  DSS_ALIGN_FREE(dss_p_a,dss_p);  /*TKL01101*/
  DSS_ALIGN_FREE(dss_g_a,dss_g);  /*TKL01101*/
  DSS_ALIGN_FREE(dss_q_a,dss_q);  /*TKL01101*/
  ALIGN_FREE(dss_y_a);
  ALIGN_FREE(hash_result_a);
  ALIGN_FREE(r_a);
  ALIGN_FREE(s_a);
  return status;
}


/****************************************************************************
*  NAME:    int GenDSSKey( ushort dss_p_bytes,
*                          uchar  *dss_p,
*                          uchar  *dss_q,
*                          uchar  *dss_g,
*                          uchar  *dss_x,
*                          uchar  *dss_y,
*                          uchar  *XKEY )
*
*
*  DESCRIPTION:  Compute DSS public/secret number pair.
*
*  INPUTS:
*      PARAMETERS:
*          ushort dss_p_bytes     Length of modulo
*          uchar *dss_p           Pointer to modulo
*          uchar *dss_q           Pointer to modulo
*          uchar *dss_g           Pointer to public key
*          uchar *XKEY            Pointer to user supplied random number
*
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *dss_x           Pointer to secret key
*          uchar *dss_y           Pointer to public key
*          uchar *XKEY            Pointer to updated number
*
*      RETURN:
*          SUCCESS               No errors
*          ERR_INPUT_LEN         Invalid length for input data
*          ERR_DATA              Generic data error
*          ERR_ALLOC             Insufficient memory
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL     Changed Expo call
*
****************************************************************************/

int GenDSSKey( ushort dss_p_bytes,
               uchar  *dss_p,
         uchar  *dss_q,
               uchar  *dss_g,
               uchar  *dss_x,
               uchar  *dss_y,
               uchar  *XKEY )
{

    int  status = SUCCESS;          /* function return status */
    SHA_context hash_context;       /* SHA context structure */
    uchar M[DSS_LENGTH_MIN];        /* message block */
    ord *dss_p_a;
    ord *dss_q_a;
    ord *dss_g_a;
    ord *dss_x_a;
    ord *dss_y_a;
    ord *XKEY_a;
#ifdef ORD_16
    ord one[SHA_LENGTH / sizeof(ord)] =
       { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
    ord one[SHA_LENGTH / sizeof(ord)] =
    { 1, 0, 0, 0, 0 };
#endif
    if ( (dss_p_bytes < DSS_LENGTH_MIN) ||     /* less than minimal */
   (dss_p_bytes > DSS_LENGTH_MAX) )       /* more than maximal */
 {
           status = ERR_DSS_LEN;           /* invalid length for dss_p */
                      return status;
    }
    if ( dss_p_bytes & 0x07 )          /* not multiple 8 (64 bit)*/
    {
      status = ERR_DSS_LEN;          /* invalid length for dss_p */
   return status;
    }
/*    ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);
    ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);
    ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);*/
    DSS_P_ALIGN_CALLOC_COPY(dss_p, dss_p_a, dss_p_bytes);  /*TKL01101*/
    DSS_G_ALIGN_CALLOC_COPY(dss_g, dss_g_a, dss_p_bytes);  /*TKL01101*/
    DSS_Q_ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);   /*TKL01101*/
    ALIGN_CALLOC_COPY(XKEY, XKEY_a, SHA_LENGTH);
    ALIGN_CALLOC(dss_x, dss_x_a, SHA_LENGTH);
    ALIGN_CALLOC(dss_y, dss_y_a, dss_p_bytes);
    if ( status !=  SUCCESS )
    {
      if( dss_p_a )
       DSS_ALIGN_FREE(dss_p_a,dss_p);
      if( dss_g_a )
       DSS_ALIGN_FREE(dss_g_a,dss_g);
      if( dss_q_a )
       DSS_ALIGN_FREE(dss_q_a,dss_q);
      if( XKEY_a )
    {
                ALIGN_COPY_FREE(XKEY_a,XKEY,SHA_LENGTH);
       }
       if( dss_x_a )
   {
          memset(dss_x_a, 0, SHA_LENGTH );
        ALIGN_COPY_FREE(dss_x_a,dss_x,SHA_LENGTH);
   }
       if( dss_y_a )
   {
                ALIGN_COPY_FREE(dss_y_a,dss_y,dss_p_bytes);
    }
       return status;     /* ERR_ALLOC   insufficient memory */
    }
  BEGIN_PROCESSING
    SHAInit ( &hash_context );
    memcpy( M, XKEY, SHA_LENGTH);
    memset( M + SHA_LENGTH, 0, DSS_LENGTH_MIN - SHA_LENGTH );
     if ( (status = SHAUpdate( &hash_context, M, DSS_LENGTH_MIN ))
         != SUCCESS )
    {
       ERROR_BREAK;
    }
    if ( (status = MySHAFinal (&hash_context, (uchar *)dss_x_a)) != SUCCESS )
    {
      ERROR_BREAK;
    }
    if (( status = PartReduct ( SHA_LENGTH,         /* dss_x = dss_x mod(dss_q)*/
                                dss_x_a,
                                SHA_LENGTH,
                                dss_q_a,
                                dss_x_a )) != SUCCESS )
    {
        ERROR_BREAK;
    }
    Sum_big( (ord*)XKEY_a, one,    /* XKEY=XKEY+1 */
        (ord*)XKEY_a, SHA_LENGTH / sizeof (ord) );
    Sum_big( XKEY_a, dss_x_a,  /* XKEY=XKEY+dss_x */
          XKEY_a, SHA_LENGTH / sizeof(ord) );
    if (( status = Expo ( dss_p_bytes,     /*dss_y = g^dss_x mod(dss_p)*/
                         dss_g_a,
                        SHA_LENGTH,
                                             dss_x_a,
                        dss_p_bytes,
                    dss_p_a,
                                                dss_y_a, NULL )) != SUCCESS ) /*TKL00601*/
    {
           ERROR_BREAK;
    }
  END_PROCESSING
/*
    ALIGN_FREE(dss_p_a);
    ALIGN_FREE(dss_q_a);
    ALIGN_FREE(dss_g_a);
*/
    DSS_ALIGN_FREE(dss_p_a,dss_p);  /*TKL01101*/
    DSS_ALIGN_FREE(dss_g_a,dss_g);  /*TKL01101*/
    DSS_ALIGN_FREE(dss_q_a,dss_q);  /*TKL01101*/
    ALIGN_COPY_FREE(XKEY_a,XKEY,SHA_LENGTH);
    ALIGN_COPY_FREE(dss_x_a,dss_x,SHA_LENGTH);
    ALIGN_COPY_FREE(dss_y_a,dss_y,dss_p_bytes);
    return status;
}



/****************************************************************************
*  NAME:    int GenDSSNumber( uchar *dss_k,
*                             uchar *dss_q,
*                             uchar *KKEY )
*
*  DESCRIPTION:  Generate secret number
*
*  INPUTS:
*      PARAMETERS:
*          uchar *KKEY      Pointer to input random number
*          uchar *dss_q     Pointer to modulo
*
*
*  OUTPUT:
*      PARAMETERS:
*          uchar *dss_x      Pointer to secret number
*          uchar *KKEY       Pointer to updated KKEY
*
*      RETURN:
*          SUCCESS           No errors
*          ERR_DATA          Generic data error
*          ERR_ALLOC         Insufficient memory
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*
****************************************************************************/

int GenDSSNumber( uchar *dss_k,
                 uchar *dss_q,
                  uchar *KKEY )
{

 int  status = SUCCESS;        /* function return status */
    ord *dss_k_a;
    ord *dss_q_a;
    ord *KKEY_a;
    SHA_context hash_context;     /* SHA context structure*/
    uchar M[DSS_LENGTH_MIN];      /* message block */
#ifdef ORD_16
    ord one[SHA_LENGTH / sizeof(ord)] =
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
    ord one[SHA_LENGTH / sizeof(ord)] =
        { 1, 0, 0, 0, 0 };
#endif
/*    ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);*/
    DSS_Q_ALIGN_CALLOC_COPY(dss_q, dss_q_a, SHA_LENGTH);   /*TKL01101*/
    ALIGN_CALLOC_COPY(KKEY, KKEY_a, SHA_LENGTH);
    ALIGN_CALLOC(dss_k, dss_k_a, SHA_LENGTH);
    if ( status !=  SUCCESS )
    {
       if( dss_q_a )
       DSS_ALIGN_FREE(dss_q_a,dss_q);
      if ( KKEY_a )
   {
                ALIGN_COPY_FREE(KKEY_a,KKEY,SHA_LENGTH);
       }
       if( dss_k_a )
   {
                ALIGN_COPY_FREE(dss_k_a,dss_k,SHA_LENGTH);
     }
          return status;     /* ERR_ALLOC   insufficient memory */
    }
  BEGIN_PROCESSING
    SHAInitK ( &hash_context );
    memcpy( M, KKEY, SHA_LENGTH);
      memset( M + SHA_LENGTH, 0, DSS_LENGTH_MIN - SHA_LENGTH );
    if ( (status = SHAUpdate( &hash_context, M, DSS_LENGTH_MIN ))
         != SUCCESS )
    {
       ERROR_BREAK;
    }
    if ( (status = MySHAFinal (&hash_context, (uchar *)dss_k_a)) != SUCCESS )
    {
       ERROR_BREAK;
    }
    if (( status = PartReduct ( SHA_LENGTH,         /* dss_k = dss_k mod(dss_q)*/
                                dss_k_a,
                                SHA_LENGTH,
                                                              dss_q_a,
                                dss_k_a )) != SUCCESS )
    {
        ERROR_BREAK;
    }
    Sum_big( KKEY_a, one,                        /* KKEY=KKEY+1 */
      KKEY_a, SHA_LENGTH / sizeof(ord) );
    Sum_big( KKEY_a, dss_k_a,                    /* KKEY=KKEY+dss_k*/
        KKEY_a, SHA_LENGTH  / sizeof(ord) );
  END_PROCESSING
/*    ALIGN_FREE(dss_q_a);*/
    DSS_ALIGN_FREE(dss_q_a,dss_q);  /*TKL01101*/

    ALIGN_COPY_FREE(KKEY_a,KKEY,SHA_LENGTH);
    ALIGN_COPY_FREE(dss_k_a,dss_k,SHA_LENGTH);
    return status;
}


/****************************************************************************
*  NAME: int GenDSSParameters( ushort dss_p_bytes,
*                               uchar  *dss_p,
*                               uchar  *dss_q,
*                               uchar *dss_g,
*                               uchar  *RVAL,
*                                                 YIELD_context *yield_cont )
*
*  DESCRIPTION: Generate DSS Common Parameters
*
*  INPUTS:
*      PARAMETERS:
*            ushort dss_p_bytes    Number of bytes in dss_p
*            uchar  *RVAL          Pointer to user supplied random number
*            YIELD_context *yield_cont  Pointer to yield_cont structure (NULL if not used)
*  OUTPUT:
*      PARAMETERS:
*            uchar *dss_p          Pointer to N-byte prime number
*            uchar *dss_q          Pointer to SHA_LENGTH prime number
*            uchar *dss_g          Pointer to N-byte number
*      RETURN:
*          SUCCESS              No errors
*          ERR_INPUT_LEN        Invalid length for input data(zero bytes)
*          ERR_DSS_LEN;         Invalid length for dss_p
*          ERR_ALLOC            Insufficient memory
*  REVISION HISTORY:
*
*  24 Sep 94   KPZ     Initial release
*  14 Oct 94   GKL     Second version (big endian support)
*  26 Oct 94   GKL     (alignment for big endian support & ERR_ALLOC)
*  08 Dec 94   GKL      Added YIELD_context
*
****************************************************************************/

 int GenDSSParameters( ushort dss_p_bytes,
                                     uchar  *dss_p,
                                          uchar  *dss_q,
                                          uchar  *dss_g,
                                          uchar  *RVAL,
                                           YIELD_context *yield_cont ) /*TKL00701*/
{
    int status = SUCCESS;      /* function return status */
    ord seed[SHA_LENGTH / sizeof (ord)];
    ord u[SHA_LENGTH / sizeof (ord)];            /* work buffers */
    ord u1[SHA_LENGTH / sizeof (ord)];
    ord  *dss_p_a;
    ord  *dss_q_a;
    ord  *dss_g_a;
    ord  *RVAL_a;
    ord ofone[SHA_LENGTH / sizeof(ord)];
    ord dss_q2[SHA_LENGTH / sizeof(ord) + 1];  /* dss_q2 = 2 * q */
    ord v[SHA_LENGTH / sizeof(ord)];
    ord *w, *c, *c1, *one, *e;           /* Pointers to work buffers */
    ushort i, n, count, offset, k;          /* counters */
    ushort dss_p_longs;                 /* number of longs */
    if ( dss_p_bytes == 0 )    /* invalid length for input data (zero bytes) */
    {
        status = ERR_INPUT_LEN;
 return status;
    }
    if ( (dss_p_bytes < DSS_LENGTH_MIN) ||     /* less than minimal */
        (dss_p_bytes > DSS_LENGTH_MAX) )      /* more than maximal */
    {
     status = ERR_DSS_LEN;           /* invalid length for dss_p */
  return status;
  }
       if ( dss_p_bytes & 0x07 )          /* not multiple 4 (64 bit)*/
 {
       status = ERR_DSS_LEN;          /* invalid length for dss_p */
   return status;
    }
    n = dss_p_bytes / SHA_LENGTH;       /* SHA passes count */
    dss_p_longs = dss_p_bytes / sizeof(ord);  /* number of long in dss_p */
    CALLOC(w, ord, (n + 1) * SHA_LENGTH);
    CALLOC(c, ord, dss_p_bytes);
    CALLOC(c1, ord, dss_p_bytes);
    CALLOC(one, ord, dss_p_bytes);
    CALLOC(e,ord, dss_p_bytes - SHA_LENGTH + sizeof(ord));
    ALIGN_CALLOC_MOVE(RVAL, RVAL_a, SHA_LENGTH);
    ALIGN_CALLOC(dss_p, dss_p_a, dss_p_bytes);
    ALIGN_CALLOC(dss_q, dss_q_a, SHA_LENGTH);
    ALIGN_CALLOC(dss_g, dss_g_a, dss_p_bytes);
    if ( status !=  SUCCESS )
    {
   if( e )
         free ( e );
     if( one )
               free ( one );
   if( c )
         free ( c );
     if( w )
         free ( w );
     if( c1 )
                free ( c1 );
    if( dss_p_a )
   {
               ALIGN_COPY_FREE(dss_p_a, dss_p, dss_p_bytes);
   }
       if( dss_q_a )
   {
               ALIGN_COPY_FREE(dss_q_a, dss_q, SHA_LENGTH);
    }
       if( dss_g_a )
   {
               ALIGN_COPY_FREE(dss_g_a, dss_g, dss_p_bytes);
   }
       if( RVAL_a )
    {
               ALIGN_MOVE_FREE(RVAL_a, RVAL, SHA_LENGTH);
      }
       return status;     /* ERR_ALLOC   insufficient memory */
     }
    one[0] = 1;
    BEGIN_PROCESSING   /* Generate DSS Common Parameters */

           BEGIN_LOOP   /* Generate prime & dss_p */  /*TKL00501*/
               /* generate prime number of length 160-bit */
                     do
               {
                   MyGenRand( SHA_LENGTH, seed, RVAL_a);  /* generate random number */
                              /* compute message digest from seed */
                   if ( (status = MySHA((uchar *)seed, SHA_LENGTH, (uchar *)u)) != SUCCESS )
                   {
                       break;  /* error */
                   }
            /* u1 = seed + 1 */
                   Sum_big( seed, one, u1, SHA_LENGTH / sizeof(ord) );
               /* compute message digest from u1 */
                   if ( (status = MySHA( (uchar *)u1, SHA_LENGTH,(uchar *)dss_q_a)) != SUCCESS )
                              {
                                       break;  /* error */
                   }
                   for ( i = 0; i < (SHA_LENGTH / sizeof(ord)); i++ )  /* dss_q = dss_q ^ u */
                                   {
                       dss_q_a[i] = dss_q_a[i] ^ u[i];
                   }
                 /* set least and most significant bits */
                   dss_q_a[SHA_LENGTH / sizeof(ord) - 1] |= ((ord)1 << (BITS_COUNT-1));
                            dss_q_a[0] |= 0x01;
                     } while ( VerPrime( SHA_LENGTH, dss_q_a, TEST_COUNT, RVAL_a, yield_cont) /*TKL00701*/
                                                 != SUCCESS );   /* perform a robust primality test */
                     if (status != SUCCESS )
                         {
                             ERROR_BREAK;
                      }
                    /* dss_q2 = 2 * dss_q */
                           memcpy( dss_q2, dss_q_a, SHA_LENGTH );
                          dss_q2[SHA_LENGTH / sizeof(ord)] = 0;
                           LShiftL_big( dss_q2, SHA_LENGTH  / sizeof(ord) +1, 1 );
                         count = 0;
                      offset = 2;
                     memset( ofone, 0, SHA_LENGTH );
                         do   /* find dss_p */
                           {
                            /* generate random number by dss_p bytes */
                                for ( k = 0;  k <= n; k++ )
                             {
                                       ofone[0] = offset + k;
                                       /* v = ofone + seed */
                                     Sum_big( seed, ofone, v, SHA_LENGTH / sizeof(ord) );
                                    if ( (status = MySHA ( (uchar *)v, SHA_LENGTH,
                                                          (uchar *)( w + (SHA_LENGTH / sizeof(ord)) * k )))
                                                               != SUCCESS ) /* compute message digest */
                                       {
                                               break; /* error */
                                      }
                               }
                               if (status != SUCCESS )
                                 {
                                      break; /* error */
                               }
                            /* set most significant bit */
                             w[dss_p_longs - 1] |= ((ord)1 << (BITS_COUNT-1));
                               memcpy( c, w, dss_p_bytes);
                          /* c1 = c mod(dss_q2) */
                                   if( (status = PartReduct( dss_p_bytes, c,
                                                                                     SHA_LENGTH + sizeof(ord),
                                                                                       dss_q2, c1)) != SUCCESS )
                                 {
                                       break; /* error */
                              }
                            /* c1 = c1 - 1*/
                                   Sub_big( c1, one, c1, dss_p_longs );
                         /* dss_p = w - c1 */
                               Sub_big( w, c1, dss_p_a, dss_p_longs );
                                 if ( dss_p_a[dss_p_bytes / sizeof(ord) - 1] >= (ord)((ord)1 << (BITS_COUNT-1)) )
                                {
                                       if ( VerPrime ( dss_p_bytes, dss_p_a, TEST_COUNT, RVAL_a, yield_cont) /*TKL00701*/
                                                                      == SUCCESS ) /* perform a robust primality test */
                                      {
                                               break;
                                          }
                               }
                               count++;
                                offset += n + 1;
                        } while ( count < 4096);
                        if (status != SUCCESS )
                         {
                             ERROR_BREAK;
                      }
                       if (count != 4096)          /*TKL00501*/
                        {
                             BREAK;                    /*TKL00501*/
                    }
       END_LOOP     /* Generate dss_p */   /*TKL00501*/

        if (status != SUCCESS )
         {
              ERROR_BREAK;
     }
       dss_p_a[0] -= 1;   /* dss_p = dss_p - 1 */
      if ( (status= DivRem (dss_p_bytes, dss_p_a, SHA_LENGTH, dss_q_a, u1,
                                                  e )) != SUCCESS )  /* e = dss_p / dss_q */
        {
              ERROR_BREAK;
     }
       dss_p_a[0] += 1;    /* dss_p = dss_p + 1 */

     BEGIN_LOOP   /* Generate dss_g */   /*TKL00501*/
                     MyGenRand( SHA_LENGTH, u, RVAL_a );  /*generate random number*/
                 u[SHA_LENGTH / sizeof(ord) - 1] &= ~((ord)1 << (BITS_COUNT-1));       /* u < dss_q */
                   if ( (status = Expo( SHA_LENGTH, u, (ushort)(dss_p_bytes - SHA_LENGTH +
                                                          sizeof(ord)), e, dss_p_bytes, dss_p_a, dss_g_a, yield_cont)) /*TKL00701*/
                                                               != SUCCESS ) /* dss_g = e ^ u mod(dss_p) */
                    {
                               ERROR_BREAK;
                    }
                       if ( dss_g_a[0] == 1 )   /* check dss_g == 1 */
                 {
                          for ( i = 1; i < (dss_p_bytes / sizeof(ord)); i++ )
                     {
                              if ( dss_g_a[i] != 0 )
                                  {
                                       break;
                                  }
                        }
                       if ( i == (dss_p_bytes / sizeof(ord)) )
                         {
                              CONTINUE;
                        }
                    }
                       BREAK;                                 /*TKL00501*/
        END_LOOP   /* Generate dss_g */             /*TKL00501*/
     END_PROCESSING   /* Generate DSS Common Parameters */
   free ( e );
     free ( one );
   free ( c );
     free ( w );
     free ( c1 );
    ALIGN_COPY_FREE(dss_p_a, dss_p, dss_p_bytes);
   ALIGN_COPY_FREE(dss_q_a, dss_q, SHA_LENGTH);
    ALIGN_COPY_FREE(dss_g_a, dss_g, dss_p_bytes);
   ALIGN_MOVE_FREE(RVAL_a, RVAL, SHA_LENGTH);
      return status;
}

