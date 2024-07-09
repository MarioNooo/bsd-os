/*
 * Id: hmac_md5.c,v 1.2 1997/04/12 01:02:09 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/hmac_md5.c,v
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
#ifndef MULTINET
#include <stdio.h>
#include <string.h>
#else
#include <param.h>
#include <types.h>
#endif /* MULTINET */
#include "hmac.h"

/*
 * hmac_md5 = MD5(key ^ opad, MD5(key ^ ipad, text))
 *    where ipad is 64 0x36's, and
 *	    opad is 64 0x5c's
 *
 *  Also contains native MD5 wrappers which allow for consistent calling
 */

void hmac_md5Init(hmac_CTX *context, unsigned char *key, int keylen)
{
    unsigned char ipad[64], keydigest[MD5_LEN];
    int i;

    if((key == NULL) || (keylen == 0))
	return;

    if(keylen > 64){
	MD5_CTX tempctx;

	MD5Init(&tempctx);
	MD5Update(&tempctx, key, keylen);
	MD5Final(keydigest, &tempctx);
	key = keydigest;
	keylen = MD5_LEN;
    }

    bzero(ipad, 64);
    bzero(context->opad, 64);
    bcopy(key, ipad, keylen);
    bcopy(key, context->opad, keylen);
    for(i=0; i<64; i++){
	ipad[i] ^= 0x36;
	context->opad[i] ^= 0x5c;
    }

    MD5Init(&context->md5);
    MD5Update(&context->md5, ipad, 64);
    return;
}

void hmac_md5Update(hmac_CTX *context, unsigned char *text, int textlen)
{
    if((text == NULL) || (textlen == 0))
	return;
    MD5Update(&context->md5, text, textlen);
}

void hmac_md5Final(unsigned char *digest, hmac_CTX *context)
{
    unsigned char temp[MD5_LEN];

    MD5Final(temp, &context->md5);
    MD5Init(&context->md5);
    MD5Update(&context->md5, context->opad, 64);
    MD5Update(&context->md5, temp, MD5_LEN);
    MD5Final(digest, &context->md5);
    return;
}

void md5Init(hmac_CTX *context)
{
    MD5Init(&context->md5);
}

void md5Update(hmac_CTX *context, unsigned char *text, int text_len)
{
    MD5Update(&context->md5, text, text_len);
}

void md5Final(unsigned char *digest, hmac_CTX *context)
{
    MD5Final(digest, &context->md5);
}

