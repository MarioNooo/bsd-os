/*
 * Id: hmac_sha.c,v 1.2 1997/04/12 01:02:10 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/hmac_sha.c,v
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
 * hmac_sha = SHA(key ^ opad, SHA(key ^ ipad, text))
 *    where ipad is 64 0x36's, and
 *	    opad is 64 0x5c's
 *
 *  Also contains native SHA wrappers to allow for standard calling
 */

void hmac_shaInit(hmac_CTX *context, unsigned char *key, int keylen)
{
    unsigned char ipad[64], keydigest[SHA_LENGTH];
    int i;

    if((key == NULL) || (keylen == 0))
	return;

    if(keylen > 64){
	SHA_CTX tempctx;

	fipsSHAInit(&tempctx);
	fipsSHAUpdate(&tempctx, key, keylen);
	fipsSHAFinal(&tempctx, keydigest);
	key = keydigest;
	keylen = SHA_LENGTH;
    }

    bzero(ipad, 64);
    bzero(context->opad, 64);
    bcopy(key, ipad, keylen);
    bcopy(key, context->opad, keylen);
    for(i=0; i<64; i++){
	ipad[i] ^= 0x36;
	context->opad[i] ^= 0x5c;
    }

    fipsSHAInit(&context->sha);
    fipsSHAUpdate(&context->sha, ipad, 64);
    return;
}

void hmac_shaUpdate(hmac_CTX *context, unsigned char *text, int textlen)
{
    if((text == NULL) || (textlen == 0))
	return;
    fipsSHAUpdate(&context->sha, text, textlen);
}

void hmac_shaFinal(unsigned char *digest, hmac_CTX *context)
{
    unsigned char temp[SHA_LENGTH];

    fipsSHAFinal(&context->sha, temp);
    fipsSHAInit(&context->sha);
    fipsSHAUpdate(&context->sha, context->opad, 64);
    fipsSHAUpdate(&context->sha, temp, SHA_LENGTH);
    fipsSHAFinal(&context->sha, digest);
    return;
}

void shaInit(hmac_CTX *context)
{
    fipsSHAInit(&context->sha);
}

void shaUpdate(hmac_CTX *context, unsigned char *text, int text_len)
{
    fipsSHAUpdate(&context->sha, text, text_len);
}

void shaFinal(unsigned char *digest, hmac_CTX *context)
{
    fipsSHAFinal(&context->sha, digest);
} 

