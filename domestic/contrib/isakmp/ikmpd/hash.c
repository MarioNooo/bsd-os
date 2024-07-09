/* 
 * Id: hash.c,v 1.4 1997/07/24 19:41:50 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/hash.c,v
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
#include "authdef.h"
#include "hmac.h"

/*
 * hash.c - process and construct hash payloads.
 */

#define HIS 1
#define MINE 2

int compute_hash(SA *sa, unsigned char *hash, int mine_or_his)
{
    hmac_CTX context;
    identity id;

    sa->Inithmac(&context, sa->skeyid, sa->skeyid_len);
    if(mine_or_his == MINE){
	sa->Updhmac(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
	sa->Updhmac(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	id.id_type = ID_IPV4_ADDR;	/* all we support now! */
	id.protocol = IPPROTO_UDP;
	id.port = htons(IKMP_PORT);	/* hash in network order! */
	obtain_id(sa, &id.ipaddr);
    } else {
	sa->Updhmac(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
	sa->Updhmac(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	bcopy((char *)&sa->opposing_id, (char *)&id, sizeof(identity));
    }
    sa->Updhmac(&context, sa->sap_offered, sa->sap_len);
    sa->Updhmac(&context, (char *)&id, sizeof(identity));
    sa->Finhmac(hash, &context);
    return(0);
}

/*
 * handle hash payloads used for authentication in phase 1 processing
 */
int process_hash (SA *sa, unsigned char *payload, 
		unsigned long message_id)
{
    hash_payload hash;
    unsigned char hash_data[SHA_LENGTH];

    LOG((DEB, "processing HASH"));
	/*
	 * if there's a message id then this is quick mode and the hashes
	 * for quick mode are handled below. This should only be called
	 * during phase 1 negotiation for RSA_ENC or PRESHAR authentication.
	 */
    if(!message_id){
	bcopy((char *)payload, (char *)&hash, sizeof(hash_payload));
	ntoh_hash(&hash);
	hash.payload_len -= sizeof(hash_payload);
	if(((sa->hash_alg == HASH_MD5) && (hash.payload_len != MD5_LEN)) ||
	   ((sa->hash_alg == HASH_SHA) && (hash.payload_len != SHA_LENGTH))){
	    LOG((WARN, "Mal-sized hash"));
	    return(1);
	}
	if(compute_hash(sa, hash_data, HIS)){
	    LOG((WARN, "Unable to compute hash!"));
	    return(1);
	}
	if(bcmp((char *)(payload + sizeof(hash_payload)), 
		(char *)hash_data, hash.payload_len)){
	    LOG((ERR, "Hash Payload is incorrect"));
	    return(1);
	}
	sa->state_mask |= PROC_AUTH;
    }
    return(0);
}

/*
 * used for the 1st 2 messages for quick mode-- one from each player. Assumes
 * that space has already been made in the packet for this guy. The final
 * quick mode message is handled inside oakley.c in "oakley_final_qm".
 */
int construct_qm_hash(SA *sa, unsigned long message_id, uchar is_notify)
{
    unsigned char thehash[SHA_LENGTH];	
    unsigned short hashlen, messlen, messtrt;
    hmac_CTX context;
    conn_entry *centry;
    unsigned long mess_id;

    switch(sa->hash_alg){
	case HASH_SHA:
	    hashlen = SHA_LENGTH;
	    break;
	case HASH_MD5:
	    hashlen = MD5_LEN;
	    break;
	default:
	    return(1);
    }
    messtrt = sizeof(isakmp_hdr) + sizeof(hash_payload) + hashlen;
    messlen = sa->packet_len - messtrt;
    mess_id = htonl(message_id);
    sa->Inithmac(&context, sa->skeyid_a, sa->skeyid_len);
    sa->Updhmac(&context, (unsigned char *)&mess_id, sizeof(unsigned long));
    if((sa->init_or_resp == RESPONDER) && (is_notify != TRUE)){
	centry = get_centry_by_mess_id(message_id);
	if(centry == NULL){
	    LOG((ERR, "Unable to get connection entry for %ld!", mess_id));
	    return(1);
	}
	sa->Updhmac(&context, (unsigned char *)centry->nonce_I,
			centry->nonce_I_len);
    }
    sa->Updhmac(&context, (sa->packet + messtrt), messlen);
    sa->Finhmac(thehash, &context);
	/*
	 * per isakmp-oakley, the hash must be the first payload following 
	 * the isakmp header.
	 */
    bcopy((char *)thehash, 
	  (char *)(sa->packet + sizeof(isakmp_hdr) + sizeof(hash_payload)), 
	  hashlen);
    return(0);
}

int verify_qm_hash(SA *sa, isakmp_hdr *hdr, unsigned char **pptr,
		unsigned char *nextp, unsigned long message_id)
{
    unsigned char verifyme[SHA_LENGTH];	/* gcd, better than mallocing again */
    unsigned char zero;
    hash_payload hash;
    hmac_CTX context;
    conn_entry *centry;
    int hashstrt, messtrt, messlen, hashlen;
    unsigned long mess_id;

    bcopy((char *)*pptr, (char *)&hash, sizeof(hash_payload));
    ntoh_hash(&hash);
    switch(sa->hash_alg){
	case HASH_SHA:
	    if((hash.payload_len - sizeof(hash_payload)) != SHA_LENGTH)
		return(1);
	    hashlen = SHA_LENGTH;
	    break;
	case HASH_MD5:
	    if((hash.payload_len - sizeof(hash_payload)) != MD5_LEN)
		return(1);
	    hashlen = MD5_LEN;
	    break;
	default:
	    return(1);
    }
	/*
	 * The message to authenticate begins immediately after the hash
	 * which is immediately after the header (header + hash = start).
	 * The hash to verify is in the body of the hash payload. (don't
	 * forget to advance processing pointer and note the next payload). 
	 */
    *pptr += hash.payload_len;
    *nextp = hash.next_payload;
    hashstrt = sizeof(isakmp_hdr) + sizeof(hash_payload);

    sa->Inithmac(&context, sa->skeyid_a, sa->skeyid_len);
    centry = get_centry_by_mess_id(message_id);
    if(centry == NULL){
	LOG((ERR,"Unable to obtain conn entry w/%ld to verify QM!",message_id));
	return(1);
    }
    mess_id = htonl(message_id);
    switch(sa->state){
	case OAK_QM_IDLE:
	case OAK_MM_KEY_AUTH:
	case OAK_AG_NOSTATE:
	    messtrt = sizeof(isakmp_hdr) + hash.payload_len;
	    messlen = hdr->len - messtrt;
	    sa->Updhmac(&context, (unsigned char *)&mess_id,
			sizeof(unsigned long));
	    if(sa->init_or_resp == INITIATOR){
		sa->Updhmac(&context, (unsigned char *)centry->nonce_I,
			centry->nonce_I_len);
	    }
	    sa->Updhmac(&context, (sa->payload + messtrt), messlen);
	    break;
	case OAK_QM_AUTH_AWAIT:
	case OAK_AG_INIT_EXCH:
	    zero = 0;
	    sa->Updhmac(&context, &zero, sizeof(unsigned char));
	    sa->Updhmac(&context, (unsigned char *)&mess_id, 
			sizeof(unsigned long));
	    sa->Updhmac(&context, centry->nonce_I, centry->nonce_I_len);
	    sa->Updhmac(&context, centry->nonce_R, centry->nonce_R_len);
	    break;
	default:
	    return(1);
    }
    sa->Finhmac(verifyme, &context);
    return(bcmp((char *)verifyme, (char *)(sa->payload + hashstrt), hashlen));
}

int construct_hash (SA *sa, unsigned char nextp, int *pos)
{
    hash_payload hash;
    unsigned char hash_data[SHA_LENGTH];
    int nbytes, hash_len = 0;
 
    LOG((DEB, "constructing HASH"));
    nbytes = sizeof(hash_payload);
    switch(sa->hash_alg){
	case HASH_SHA:
	    hash_len = SHA_LENGTH;
	    break;
	case HASH_MD5:
	    hash_len = MD5_LEN;
	    break;
    }
    nbytes += hash_len;
    bzero((char *)hash_data, hash_len);
    bzero((char *)&hash, sizeof(hash_payload));
    if(compute_hash(sa, hash_data, MINE)){
	LOG((ERR,"Unable to compute hash!"));
	return(1);
    }
    hash.next_payload = nextp;
    hash.payload_len = nbytes;
    hton_hash(&hash);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&hash, (char *)(sa->packet + *pos),
		sizeof(hash_payload));
    *pos += sizeof(hash_payload);

    bcopy((char *)hash_data, (char *)(sa->packet + *pos), hash_len);
    *pos += hash_len;

    return(0);
}

int construct_blank_hash (SA *sa, unsigned char nextp, int *pos)
{
    hash_payload hash;
    int nbytes, hash_len = 0;
 
    switch(sa->hash_alg){
	case HASH_SHA:
	    hash_len = SHA_LENGTH;
	    break;
	case HASH_MD5:
	    hash_len = MD5_LEN;
	    break;
    }
    nbytes = sizeof(hash_payload) + hash_len;
    bzero((char *)&hash, sizeof(hash_payload));

    hash.next_payload = nextp;
    hash.payload_len = nbytes;
    hton_hash(&hash);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&hash, (char *)(sa->packet + *pos),
		sizeof(hash_payload));
    *pos += sizeof(hash_payload);
    *pos += hash_len;

    return(0);
}

