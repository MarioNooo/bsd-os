/*
 * Id: hmac.h,v 1.1.1.1 1997/03/06 01:54:19 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/hmac.h,v
 */
/*
 *  Copyright Cisco Systems, Incorporated
 *
 *  Cisco Systems grants permission for redistribution and use in source
 *  and binary forms, with or without modification, provided that the
 *  following conditions are met:
 *     1. Redistribution of source code must retain the above copyright
 *        notice, this list of conditions, and the following disclaimer
 *        in all source files.
 *     2. Redistribution in binary form must retain the above copyright
 *        notice, this list of conditions, and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *
 *  "DISCLAIMER OF LIABILITY
 *
 *  THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS, INC. ("CISCO")  ``AS IS''
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CISCO BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE."
 */
#ifndef _HMAC_H_
#define _HMAC_H_

#include "toolkit.h"
#include "md5.h"

typedef struct {
    MD5_CTX md5;
    SHA_CTX sha;
    unsigned char opad[64];
} hmac_CTX; 

#include "hmac_md5.h"
#include "hmac_sha.h"

void shaInit(hmac_CTX *context);
void shaUpdate(hmac_CTX *context, unsigned char *text, int text_len);
void shaFinal(unsigned char *digest, hmac_CTX *context);

void md5Init(hmac_CTX *context);
void md5Update(hmac_CTX *context, unsigned char *text, int text_len);
void md5Final(unsigned char *digest, hmac_CTX *context);

#endif


