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
/**********************************************************************\
*  FILENAME:  endian.h     PRODUCT NAME:
*
*  DESCRIPTION:  header file of defines
*
*  USAGE:      Platform-dependend compilation modes header
*
*
*          Copyright (c) Cylink Corporation 1994. All rights reserved.
*
*  REVISION  HISTORY:
*
\**********************************************************************/

#ifndef ENDIAN_H  /* Prevent multiple inclusions of same header file */
#define ENDIAN_H

/*#define DEBUG1*/
/*#define VXD*/

/* test defined settings */
#if  !defined( ASM_FUNCTION ) && !defined( C_FUNCTION )
#error Not defined ASM or C FUNCTION.
#endif

#if  defined( ASM_FUNCTION ) && defined( C_FUNCTION )
#error Use only one define ASM or C FUNCTION.
#endif

#if  !defined( ORD_32 ) && !defined( ORD_16 )
#error Not defined basic word type ORD_32 or ORD_16.
#endif

#if  defined( ORD_32 ) && defined( ORD_16 )
#error Use only one define basic word type ORD_32 or ORD_16.
#endif

#if  !defined( L_ENDIAN ) && !defined( B_ENDIAN )
#error Not defined CPU type LITTLE or BIG ENDIAN.
#endif

#if  defined( L_ENDIAN ) && defined( B_ENDIAN )
#error Use only one define CPU type LITTLE or BIG ENDIAN.
#endif

#if  ((defined( ASM_FUNCTION ) &&  \
       defined( L_ENDIAN ) && \
       defined( ORD_16 )))          
/*       ||                          \
      (defined( C_FUNCTION ) &&    \
       defined( L_ENDIAN ) && \
       defined( ORD_32 )))
 */
#error  Defined settings are conflicted.
#endif

#ifdef ORD_16
typedef unsigned short ord;
typedef unsigned long  dord;
#define BITS_COUNT 16
#define MAXDIGIT (ord)(0xFFFF)
#endif

#ifdef ORD_32
typedef unsigned long  ord;
#ifdef B_ENDIAN                              /*TKL00701*/
typedef unsigned long long int dord;
#endif /* B_ENDIAN */                        /*TKL00701*/
#define BITS_COUNT 32
#define MAXDIGIT (ord)(0xFFFFFFFF)
#endif /* ORD_32 */



#define CALLOC(var,type,len)                    \
                          var=(type *)calloc(len,1);     \
                                if (var==NULL)                 \
                                         status=ERR_ALLOC
#ifdef B_ENDIAN
#define ALIGN_CALLOC(i,o,l)                     \
                             CALLOC(o,ord,l)
#define ALIGN_CALLOC_COPY(i,o,l)                \
                                CALLOC(o,ord,l);               \
                                if (o) ByteOrd(i,l,o)
#define ALIGN_CALLOC_MOVE(i,o,l)                \
                          CALLOC(o,ord,l);               \
                                if (o) memcpy(o,i,l)
#define ALIGN_FREE(o)                           \
                           free ( o )
#define ALIGN_COPY_FREE(o,i,l)                  \
                             if ((o) && (status==SUCCESS))  \
                                       OrdByte(o,l,i);             \
                            free (o)
#define ALIGN_MOVE_FREE(o,i,l)                  \
                               if ((o) && (status==SUCCESS))  \
                                       memcpy(i,o,l);              \
                                   memset(o,0,l);  \
                                       free (o)
#else
#define ALIGN_CALLOC(i,o,l) o=(ord *)i
#define ALIGN_CALLOC_COPY(i,o,l) o=(ord *)i
#define ALIGN_CALLOC_MOVE(i,o,l) o=(ord *)i
#define ALIGN_FREE(o)  ;
#define ALIGN_COPY_FREE(o,i,l) ;
#define ALIGN_MOVE_FREE(o,i,l) ;
#endif
#define DSS_P_ALIGN_CALLOC_COPY(i,o,l)          \
              if (i)                            \
             { ALIGN_CALLOC_COPY(i,o,l);}      \
              else                              \
                o = &DSS_P_NUMBERS[DSS_NUM_INDEX[(l-DSS_LENGTH_MIN)/LENGTH_STEP]]

#define DSS_G_ALIGN_CALLOC_COPY(i,o,l)          \
              if (i)                            \
              { ALIGN_CALLOC_COPY(i,o,l);}      \
        else                              \
                o = &DSS_G_NUMBERS[DSS_NUM_INDEX[(l-DSS_LENGTH_MIN)/LENGTH_STEP]]

#define DSS_Q_ALIGN_CALLOC_COPY(i,o,l)          \
              if (i)                            \
              { ALIGN_CALLOC_COPY(i,o,l);}      \
              else                              \
                o = DSS_Q_NUMBER

#define DSS_ALIGN_FREE(o,i)           \
              if (i)                  \
              { ALIGN_FREE(o);}
#endif     /* ENDIAN_H */



