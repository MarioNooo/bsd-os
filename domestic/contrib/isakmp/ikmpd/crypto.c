/* 
 * Id: crypto.c,v 1.3 1997/07/24 19:41:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/crypto.c,v
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#else
#include <types.h>
#include <socket.h>
#endif /* MULTINET */
#include <netinet/in.h>
#ifndef KEY2
#include <netkey/key.h>
#include <netsec/ipsec.h>
#endif
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * crypto.c -- routines to encrypt and decrypt payloads 
 */

int decrypt_payload (SA *sa, conn_entry *centry, unsigned char *payload, 
			int len)
{
    unsigned char exp_key[128];
    unsigned int result;
    int pos;

    switch(sa->encr_alg){
	case ET_DES_CBC:
	    DESKeyExpand(sa->crypt_key, exp_key);
	    if(centry == NULL){
		result = DESDecrypt(sa->crypt_iv, exp_key, CBC_MODE, 
				payload, len);
	    } else {
		result = DESDecrypt(centry->phase2_iv, exp_key, CBC_MODE, 
				payload, len);
	    }
	    if(result != SUCCESS){
		LOG((WARN,"Unable to decrypt payload!"));
		construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
		pos = sizeof(isakmp_hdr);
		construct_notify(sa, 0, DECRYPTION_FAILED, payload, len, &pos);
		sa->send_dir = UNIDIR;
		return(-1);
	    }
	    break;
	default:
	    LOG((WARN,"Unknown crypt algorithm. Neg. payload not decrypted"));
	    return(-1);
    }
    return(0);
}

int encrypt_payload (SA *sa, conn_entry *centry, unsigned char **payload, 
			short offset, unsigned  short *len)
{
    unsigned char *ptr, exp_key[128];
    int result, ncrypt = *len - offset;

	/* pad data to encrypt up to the next cypher blocklen */
    switch(sa->encr_alg){
	case ET_DES_CBC:
	    ncrypt = (((ncrypt / CYPHER_BLOCKLEN) + 1) * CYPHER_BLOCKLEN);
	    break;
	default:
	    LOG((WARN,"Unknown cypher!"));
	    return(1);
    }
    *len = ncrypt + offset;
    ptr = (unsigned char *) realloc (*payload, *len);
    if(ptr == NULL){
	LOG((CRIT,"Out Of Memory"));
	return(-1);
    }
    *payload = ptr;

    ptr = *payload + offset;
    switch(sa->encr_alg){
	case ET_DES_CBC:
	    DESKeyExpand(sa->crypt_key, exp_key);
	    if(centry == NULL){
		result = DESEncrypt(sa->crypt_iv, exp_key, CBC_MODE, ptr, 
				ptr, ncrypt);
	    } else {
		result = DESEncrypt(centry->phase2_iv, exp_key, CBC_MODE, 
				ptr, ptr, ncrypt);
	    }
	    if(result != SUCCESS){
		LOG((WARN,"Unable to encrypt payload!"));
		return(1);
	    }
	    break;
	default:
	    LOG((ERR, "Whoa! Unknown crypt alg!"));
	    return(1);
    }
    return(0);
}

