/* 
 * Id: dnh.c,v 1.2 1997/03/13 01:48:18 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/cylink/toolkit/dnh.c,v
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
#include <stdlib.h>
#include "endian.h"
#include "toolkit.h"
#include "cencrint.h"
#include "bn.h"

/*
 * Wrappers to do Diffie-Hellman with MSB-1st numbers. All relevant input
 * and output is flipped. The Diffie-Hellman secret number is not flipped
 * since this is never transmitted nor used once the shared secret has
 * been derived. If some need for the secret number arises, make sure it's
 * flipped upon leaving genDHParameters() and upon entry and exit of
 * genDHSharedSecret().
 */

#define SET_BIGNUM(a, b, c)	\
		bnBegin(&a);	\
		a.size = c; 	\
		a.ptr = b; 	\
		a.allocated = c

static void flip (unsigned char *p, unsigned short len)
{
    int i, j, halfway;
    unsigned char temp;

    halfway = len/2;
    for(j=len-1, i=0; i<halfway; i++, j--){
	temp = p[i];
	p[i] = p[j];
	p[j] = temp;
    }
}

/*
 * if it's not big endian then there wasn't a malloc+copy and we have
 * to flip the number back, otherwise just call free.
 */
#ifndef B_ENDIAN
#define FLIP_FREE(i,l)	flip(i,l)
#else
#define FLIP_FREE(i,l)	free(i)
#endif				

/*
 * flip the Diffie-Hellman group coming in and going out
 */
int genDHParameters (unsigned short dh_len, unsigned char *secret_val,
		     unsigned char *public_val, unsigned char *dh_base,
		     unsigned char *dh_mod, unsigned char *random_num)
{
    struct BigNum g, x, p, res;
    ord *secret_val_a, *public_val_a, *dh_base_a, *dh_mod_a, *random_num_a;
    unsigned short nord;
    int status = SUCCESS;

    ALIGN_CALLOC_COPY(dh_mod, dh_mod_a, dh_len);
    if(status != SUCCESS){
	return(status);
    }
    ALIGN_CALLOC_COPY(dh_base, dh_base_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(dh_mod_a);
	return(status);
    }
    ALIGN_CALLOC_MOVE(random_num, random_num_a, SHA_LENGTH);
    if(status != SUCCESS){
	ALIGN_FREE(dh_mod_a);
	ALIGN_FREE(dh_base_a);
	return(status);
    }
    ALIGN_CALLOC(secret_val, secret_val_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(dh_mod_a);
	ALIGN_FREE(dh_base_a);
	ALIGN_FREE(random_num_a);
	return(status);
    }
    ALIGN_CALLOC(public_val, public_val_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(dh_mod_a);
	ALIGN_FREE(dh_base_a);
	ALIGN_FREE(random_num_a);
	ALIGN_FREE(secret_val_a);
	return(status);
    }
    MyGenRand(dh_len, secret_val_a, random_num_a);

    flip((uchar *)dh_base_a, dh_len);
    flip((uchar *)dh_mod_a, dh_len);
    nord = dh_len/sizeof(ord);
    bnInit();
    SET_BIGNUM(g, dh_base_a, nord);
    SET_BIGNUM(p, dh_mod_a, nord);
    SET_BIGNUM(x, secret_val_a, nord);
    SET_BIGNUM(res, public_val_a, nord);

    status = bnExpMod(&res, &g, &x, &p);/* public_val = g^x mod p */
    ALIGN_COPY_FREE(secret_val_a, secret_val, dh_len);
    ALIGN_COPY_FREE(public_val_a, public_val, dh_len);
    ALIGN_MOVE_FREE(random_num_a, random_num, SHA_LENGTH);
    FLIP_FREE((unsigned char *)dh_base_a, dh_len);
    FLIP_FREE((unsigned char *)dh_mod_a, dh_len);
    flip(public_val, dh_len);

    return(status);
}

int genDHSharedSecret (unsigned short dh_len, unsigned char *secret_val,
		       unsigned char *other_pub_val, 
		       unsigned char *shared_secret, unsigned char *dh_mod)
{
    struct BigNum y, x, p, res;
    ord *secret_val_a, *other_pub_val_a, *shared_secret_a, *dh_mod_a;
    unsigned short nord;
    int status = SUCCESS;

    ALIGN_CALLOC_COPY(other_pub_val, other_pub_val_a, dh_len);
    if(status != SUCCESS){
	return(status);
    }
    ALIGN_CALLOC_COPY(secret_val, secret_val_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(other_pub_val_a);
	return(status);
    }
    ALIGN_CALLOC_COPY(dh_mod, dh_mod_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(other_pub_val_a);
	ALIGN_FREE(secret_val_a);
	return(status);
    }
    ALIGN_CALLOC(shared_secret, shared_secret_a, dh_len);
    if(status != SUCCESS){
	ALIGN_FREE(other_pub_val_a);
	ALIGN_FREE(secret_val_a);
	ALIGN_FREE(dh_mod_a);
	return(status);
    }

    flip((uchar *)dh_mod_a, dh_len);
    flip((uchar *)other_pub_val_a, dh_len);
    nord = dh_len/sizeof(ord);
    bnInit();
    SET_BIGNUM(y, other_pub_val_a, nord);
    SET_BIGNUM(x, secret_val_a, nord);
    SET_BIGNUM(p, dh_mod_a, nord);
    SET_BIGNUM(res, shared_secret_a, nord);

    status = bnExpMod(&res, &y, &x, &p);	/* ss = y^x mod p */

    FLIP_FREE((unsigned char *)other_pub_val_a, dh_len);
    FLIP_FREE((unsigned char *)dh_mod_a, dh_len);
    ALIGN_FREE(secret_val_a);
    ALIGN_COPY_FREE(shared_secret_a, shared_secret, dh_len);
    flip(shared_secret, dh_len);

    return(status);
}

