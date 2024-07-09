/* 
 * Id: nonce.c,v 1.2 1997/04/12 01:02:20 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/nonce.c,v
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <types.h>
#include <socket.h>
#include <netinet/in.h>
#endif /* MULTINET */
#ifndef KEY2
#include <netsec/ipsec.h>
#endif
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"
#include "toolkit.h"
#include "authdef.h"

/*
 * nonce.c 
 */

int process_nonce (SA *sa, unsigned char *payload, 
		unsigned long mess_id)
{
    nonce_payload nonce;
    int nonce_len;
#ifdef NON_COMMERCIAL_AND_RSAREF
    union key_pair mykeys;
    unsigned char decrypted[SHA_LENGTH];
    int outlen;
#endif

    LOG((DEB,"processing a NONCE"));
    bcopy((char *)payload, (char *)&nonce, sizeof(nonce_payload));
    ntoh_nonce(&nonce);
    nonce_len = nonce.payload_len - sizeof(nonce_payload);
	/*
	 * if mess_id is non-zero this is for an IPsec SA. Otherwise, it
	 * is for the ISAKMP SA
	 */
    if(mess_id){
	return(set_nonce_by_mess_id(mess_id, (payload + sizeof(nonce_payload)),
		nonce_len, sa->init_or_resp));
    } else {
#ifdef NON_COMMERCIAL_AND_RSAREF
	if(sa->auth_alg == RSA_ENC){
	    init_siglib(RSAKEYS);
	    if(get_priv_entry(&mykeys)){
		LOG((ERR, "can't get my private RSA keys!"));
		term_siglib();
		return(1);
	    }
	    term_siglib();
	    if(RSAPrivateDecrypt(decrypted, &outlen, 
				(payload + sizeof(nonce_payload)), nonce_len,
				&mykeys.rsa_pair.priv)){
		LOG((ERR, "Unable to decrypt nonce!"));
		return(1);
	    }
	/*
	 * copy decrypted nonce back onto payload and set nonce_len
	 */
	    bcopy((char *)decrypted, (char *)(payload + sizeof(nonce_payload)),
		outlen);
	    nonce_len = outlen;
	}
#endif
	if(sa->init_or_resp == INITIATOR){
	    sa->nonce_R = (unsigned char *) malloc (nonce_len);
	    if(sa->nonce_R == NULL){
		LOG((CRIT, "Out Of Memory"));
		return(1);
	    }
	    sa->nonce_R_len = nonce_len;
	    bcopy((char *)(payload + sizeof(nonce_payload)),
			sa->nonce_R, sa->nonce_R_len);
	} else {
	    sa->nonce_I = (unsigned char *) malloc (nonce_len);
	    if(sa->nonce_I == NULL){
		LOG((CRIT, "Out Of Memory"));
		return(1);
	    }
	    sa->nonce_I_len = nonce_len;
	    bcopy((char *)(payload + sizeof(nonce_payload)),
			sa->nonce_I, sa->nonce_I_len);
	}
    }
	/*
	 * the initiator becomes crypto active as soon as he's processed
	 * a ke and a nonce payloads. The responder becomes crypto immediately 
	 * after he's sent off his ke + nonce payloads. So if responder, delay. 
	 */
    if(sa->init_or_resp == INITIATOR){
	gen_skeyid(sa);
	sa->state_mask |= CRYPTO_ACTIVE;
    } else
	delay_crypto_active(sa);
    return(0);
}

int construct_nonce (SA *sa, unsigned char nextp, int *pos)
{
    unsigned char my_nonce[SHA_LENGTH];
    nonce_payload nonce;
#ifdef NON_COMMERCIAL_AND_RSAREF
    union public_key rec;
    unsigned char seedbyte = 0;
    unsigned int bytesneeded, outlen, inlen;
    char encr_nonce[MAX_RSA_MODULUS_LEN];
    R_RANDOM_STRUCT ran;
#endif

    get_rand(my_nonce, 12);
    bzero((char *)&nonce, sizeof(nonce_payload));
	/*
	 * generate my own nonce and make a payload for it
	 */
    if(sa->init_or_resp == INITIATOR){
	sa->nonce_I_len = SHA_LENGTH;
	sa->nonce_I = (unsigned char *) malloc (SHA_LENGTH);
	if(sa->nonce_I == NULL){
	    LOG((CRIT, "Out Of Memory"));
	    return(1);
	}
	bcopy((char *)my_nonce, (char *)sa->nonce_I, SHA_LENGTH);
    } else {
	sa->nonce_R_len = SHA_LENGTH;
	sa->nonce_R = (unsigned char *) malloc (SHA_LENGTH);
	if(sa->nonce_R == NULL){
	    LOG((CRIT, "Out Of Memory"));
	    return(1);
	}
	bcopy((char *)my_nonce, (char *)sa->nonce_R, SHA_LENGTH);
    }
#ifdef NON_COMMERCIAL_AND_RSAREF
    if(sa->auth_alg == RSA_ENC){
	init_siglib(RSAKEYS);
	if(sa->init_or_resp == INITIATOR){
	    if(get_pub_rec_by_addr(&sa->dst.sin_addr, &rec,
				(unsigned char)FORENCR)){
		LOG((ERR, "no key entry for remote host!"));
		term_siglib();
		return(1);
	    }
	} else {
	    if(get_pub_rec_by_addr(&sa->src.sin_addr, &rec,
				(unsigned char)FORENCR)){
		LOG((ERR, "no key entry for remote host!"));
		term_siglib();
		return(1);
	    }
	}
	term_siglib();
	R_RandomInit(&ran);
	while(1){
	    R_GetRandomBytesNeeded(&bytesneeded, &ran);
	    if(bytesneeded == 0)
		break;
	    R_RandomUpdate(&ran, &seedbyte, 1);
	}
	inlen = SHA_LENGTH;
	if(RSAPublicEncrypt(encr_nonce, &outlen, my_nonce, inlen,
		&rec.rsapub.pubkey, &ran)){
	    LOG((ERR, "unable to encrypt nonce!"));
	    return(1);
	}
	nonce.next_payload = nextp;
	nonce.payload_len = outlen + sizeof(nonce_payload);
	hton_nonce(&nonce);
	expand_packet(sa, *pos, (outlen + sizeof(nonce_payload)));
	bcopy((char *)&nonce, (char *)(sa->packet + *pos),
		sizeof(nonce_payload));
	*pos += sizeof(nonce_payload);

	bcopy((char *)encr_nonce, (char *)(sa->packet + *pos), outlen);
	*pos += outlen;
    } else {
#endif
	nonce.next_payload = nextp;
	nonce.payload_len = SHA_LENGTH + sizeof(nonce_payload);
	hton_nonce(&nonce);
	expand_packet(sa, *pos, (SHA_LENGTH + sizeof(nonce_payload)));
	bcopy((char *)&nonce, (char *)(sa->packet + *pos),
		sizeof(nonce_payload));
	*pos += sizeof(nonce_payload);

	bcopy((char *)my_nonce, (char *)(sa->packet + *pos), SHA_LENGTH);
	*pos += SHA_LENGTH;
#ifdef NON_COMMERCIAL_AND_RSAREF
    }
#endif

    return(0);
}

int construct_ipsec_nonce (SA *sa, unsigned long mess_id, unsigned char nextp, 
			int *pos)
{
    unsigned char my_nonce[SHA_LENGTH];
    nonce_payload nonce;

    get_rand(my_nonce, 5);
    bzero((char *)&nonce, sizeof(nonce_payload));
	/*
	 * send my opposite identity because set_nonce_by_mess_id will
	 * switch it: if initiator it set's nonce-R and vice versa.
	 * Here, if initiator we want to set nonce-I.
	 */
    set_nonce_by_mess_id(mess_id, my_nonce, SHA_LENGTH, 
		(sa->init_or_resp == INITIATOR) ? RESPONDER : INITIATOR);

    nonce.next_payload = nextp;
    nonce.payload_len = SHA_LENGTH + sizeof(nonce_payload);
    hton_nonce(&nonce);
    expand_packet(sa, *pos, (SHA_LENGTH + sizeof(nonce_payload)));
    bcopy((char *)&nonce, (char *)(sa->packet + *pos), sizeof(nonce_payload));
    *pos += sizeof(nonce_payload);

    bcopy((char *)my_nonce, (char *)(sa->packet + *pos), SHA_LENGTH);
    *pos += SHA_LENGTH;

    return(0);
}

