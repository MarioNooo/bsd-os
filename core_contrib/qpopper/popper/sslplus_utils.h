/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * File: sslplus_utils.h
 *
 * Revisions:
 *
 *     09/05/99  [LGL]
 *             - File added.
 *
 */


/* ======================================================================
     This is just a concatenation of functions to support SSLPlus. We
     put them all here and #ifdef it on SSLPlus to have only one file
     to include and to not have to vary the files included in the 
     Makefile.  If you're not using SSLPlus from Certicom this 
     file compiles to nothing.
   ====================================================================== */


#include "config.h"

#ifdef QPOP_SSLPLUS
#  include "ssl.h"

/*
 * Opaque type for session DB
 */
typedef struct sSessDBType SessDBType;


/*
 * Set up the call backs open the DB ...
 * Returns NULL on failure (out of memory) in which case the call
 * backs will be unregistered.
 */
SessDBType *OpenSessDB(SSLContext *pCtx);


/*
 * Close the DB; we're all done! This must be called. You may pass NULL.
 */
void CloseSessDB(SessDBType *pSessDB);



SSLErr ConfigureContextDH( SSLContext *ctx );


/* ======================================================================
     SSLPLus sample random code. The MUTEX stuff from the original 
     is removed since we're single threaded here
   ====================================================================== */
/*+
Release: SSL Plus 3.0.2 (July 6, 1999) 
 
Certicom, the Certicom logo, SSL Plus, and Security Builder are trademarks of
Certicom Corp.  Copyright (c) 1997-1999 Certicom Corp.  Portions are 
Copyright 1997-1998, Consensus Development Corporation, a wholly owned
subsidiary of Certicom Coryp.  All rights reserved.  
Contains an implementation of NR signatures, licensed under U.S.
patent 5,600,725.  Protected by U.S. patents 5,787,028; 4,745,568;
5,761,305.  Patents pending.
-*/

#  ifndef _SRANDOM_H_
#    define _SRANDOM_H_ 1

#    include "ssl.h"

typedef struct {
    EZCryptoObj     ezRandom;
} RandomObj;


int CreateRandomObj(RandomObj **rand);
int DestroyRandomObj(RandomObj *rand);

SSLErr ConfigureContextForRandom( SSLContext *ctx, const char *randomPoolFilename, RandomObj **randomObj );
SSLErr RandomCallback(SSLBuffer data, void *randomRef);
int UpdateRandomObject(void *data, unsigned int length, RandomObj *rand);

#  endif /* _SRANDOM_H_ */


#endif /* QPOP_SSLPLUS */
