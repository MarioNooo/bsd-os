/* 
 * Id: sig.c,v 1.5 1997/07/24 19:42:13 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/sig.c,v
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
 * sig.c - routines to handle the processing and construction of sig
 *	payloads.
 */

#define HIS 1
#define MINE 2

/*
 * DSS is tied to SHA so don't let negotiation of MD5 for HMAC reasons
 * screw up authentication. We want to do SHA regardless for this.
 */
static void compute_dss_hash(SA *sa, unsigned char *digest, int mine_or_his)
{
    hmac_CTX context;

    hmac_shaInit(&context, sa->skeyid, sa->skeyid_len);
    if(mine_or_his == MINE){
	hmac_shaUpdate(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
	hmac_shaUpdate(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
	hmac_shaUpdate(&context, sa->my_cookie, COOKIE_LEN);
	hmac_shaUpdate(&context, sa->his_cookie, COOKIE_LEN);
    } else {
	hmac_shaUpdate(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
	hmac_shaUpdate(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
	hmac_shaUpdate(&context, sa->his_cookie, COOKIE_LEN);
	hmac_shaUpdate(&context, sa->my_cookie, COOKIE_LEN);
    }
    hmac_shaUpdate(&context, sa->sap_offered, sa->sap_len);
    if(((sa->init_or_resp == INITIATOR) && (mine_or_his == MINE)) ||
       ((sa->init_or_resp == RESPONDER) && (mine_or_his == HIS))){
	hmac_shaUpdate(&context, (unsigned char *)&sa->dst.sin_addr, 
			sizeof(struct in_addr));
    } else {
	hmac_shaUpdate(&context, (unsigned char *)&sa->src.sin_addr, 
			sizeof(struct in_addr));
    }
    hmac_shaFinal(digest, &context);
    return;
}

#ifdef NON_COMMERCIAL_AND_RSAREF
static int process_rsa_sig (SA *sa, struct rsa_pub_key *rec,
			unsigned char *payload, unsigned long mess_id)
{
    sig_payload sig;
    unsigned char digest[SHA_LENGTH];	/* is the GCD of SHA_LENGTH & MD5_LEN */
    unsigned char decrypted[MAX_RSA_MODULUS_LEN];
    int digest_len, sig_len, len;

    LOG((DEB,"processing RSA sig"));
    switch(sa->hash_alg){
	case HASH_MD5:
	    digest_len = MD5_LEN;
	    break;
	case HASH_SHA:
	    digest_len = SHA_LENGTH;
	    break;
	default:
	    LOG((WARN, "Unknown hash algorithm"));
	    return(1);
    }
    /*
     * extract and validate sig payload 
     */
    bcopy((char *)payload, (char *)&sig, sizeof(sig_payload));
    ntoh_sig(&sig);
    sig_len = sig.payload_len - sizeof(sig_payload);
    /*
     * use negotiated hash function to compute digest
     */
    compute_hash(sa, digest, HIS);

    if(RSAPublicDecrypt(decrypted, &len,
		(payload + sizeof(sig_payload)), sig_len,
		&rec->pubkey)){
	LOG((WARN,"Invalid signature"));
	construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	len = sizeof(isakmp_hdr); 
	construct_notify(sa, 0, INVALID_SIGNATURE, payload, sig.payload_len, 
			&len);
	return(1);
    }
    if(len != digest_len){
	/*
	 * should this be a showstopper on the signature check?
	 */
	LOG((WARN, "Signature decrypted to %d instead of %d",len, digest_len));
    }
    if(bcmp((char *)decrypted, (char *)digest, digest_len)){
	LOG((ERR, "Signature verification failed!"));
	construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	len = sizeof(isakmp_hdr);
	construct_notify(sa, 0, INVALID_SIGNATURE, payload, sig.payload_len,
			&len);
	return(1);
    }
    return(0);
}

#endif	/* non-commercial use and RSAREF */

static int process_dss_sig (SA *sa, struct dss_pub_key *rec, 
			unsigned char *payload, unsigned long mess_id)
{
    sig_payload sig;
    unsigned char digest[SHA_LENGTH];
    unsigned char r[SHA_LENGTH];
    unsigned char s[SHA_LENGTH];
    int status, len;

	/*
	 * extract and validate sig payload 
	 */
    LOG((DEB, "processing DSS sig"));
    bcopy((char *)payload, (char *)&sig, sizeof(sig_payload));
    ntoh_sig(&sig);
    if(sig.payload_len != (sizeof(r) + sizeof(s) + sizeof(sig_payload))){
	LOG((WARN,"Invalid signature payload"));
	construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	len = sizeof(isakmp_hdr); 
	construct_notify(sa, 0, INVALID_SIGNATURE, payload, sig.payload_len, 
			&len);
    }
	/*
	 * extract r and s from signature payload
	 */
    bcopy((char *)(payload + sizeof(sig_payload)), (char *)r, 
		SHA_LENGTH);
    bcopy((char *)(payload + sizeof(sig_payload) + SHA_LENGTH), 
		(char *)s, SHA_LENGTH);
    ntoh_payload((char *)r, SHA_LENGTH);
    ntoh_payload((char *)s, SHA_LENGTH);
	/*
	 * compute the hash and verify the signature
	 */
    compute_dss_hash(sa, digest, HIS);
    status = VerDSSSignature(DSS_LENGTH_MIN, rec->p, rec->q, rec->g, rec->y, 
			r, s, digest);
    if(status != SUCCESS){
        switch(status){
        case ERR_SIGNATURE:
          LOG((WARN,"unable to verify signature!"));
          break;
        case ERR_DSS_LEN:
          LOG((WARN,"invalid length of P"));
          break;
        case ERR_ALLOC:
          LOG((WARN,"unable to verify signature (insufficient memory)"));
          break;
        default:
          LOG((WARN,"unknown return value %d",status));
        }
	LOG((WARN,"Invalid signature!"));
	construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	len = sizeof(isakmp_hdr); 
	construct_notify(sa, 0, INVALID_SIGNATURE, payload, sig.payload_len, 
			&len);
	return(1);
    }
    return(0);
}

int process_sig (SA *sa, unsigned char *payload, unsigned long mess_id)
{
    struct in_addr addr;
    union public_key rec;

    if(sa->state_mask & PROC_AUTH)
	return(0);
    if((sa->state_mask & PROC_KE) == 0)
	return(1);

    LOG((DEB,"processing ISA_SIG"));
	/*
	 * make sure the remote host has an entry in the pub key ring
	 */
    if(sa->auth_alg == DSS_SIG){
	init_siglib(DSSKEYS);
    } else {
#ifdef NON_COMMERCIAL_AND_RSAREF
	init_siglib(RSAKEYS);
#else
	LOG((ERR, "RSA is for non-commercial use only!"));
	return(1);
#endif
    }
    bcopy((char *)&sa->opposing_id.ipaddr, (char *)&addr, 
		sizeof(struct in_addr));
    if(get_pub_rec_by_addr(&addr, &rec, (unsigned char)FORSIGS)){
	LOG((ERR,"no key entry for remote host!"));
	term_siglib();
	return(1);
    }
    term_siglib();
    switch(sa->auth_alg){
	case DSS_SIG:
	    if(process_dss_sig(sa, &rec.dsspub, payload, mess_id))
		return(1);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSA_SIG:
	    if(process_rsa_sig(sa, &rec.rsapub, payload, mess_id))
		return(1);
	    break;
#endif
	default:
	    LOG((WARN, "unknown AUTH method"));
	    return(1);
    }
    sa->state_mask |= PROC_AUTH;
    return(0);
}

static int construct_dss_sig (SA *sa, struct dss_signature *me, 
			unsigned char nextp, int *pos)
{
    sig_payload sig;
    int nbytes;
    unsigned char r[SHA_LENGTH];
    unsigned char s[SHA_LENGTH];
    unsigned char k[SHA_LENGTH];
    unsigned char digest[SHA_LENGTH];
    unsigned char rval[SHA_LENGTH];
 
	/*
	 * construct hash to sign and generate a random K. Not sure how
	 * random K should be. 60 out of the 160 bits should be enough,
	 * maybe it's too much.
	 */
    LOG((DEB, "constructing DSS signature\n"));
    compute_dss_hash(sa, digest, MINE);
    get_rand(rval, 60);
	/*
	 * generate a random k < q,
	 * sign the hash resulting in r and s
	 */
    if(GenDSSNumber(k, me->pub.q, rval) != SUCCESS){
	LOG((ERR,"Unable to obtain random DSS number"));
	return(1);
    }
    if(GenDSSSignature(DSS_LENGTH_MIN, me->pub.p, me->pub.q, me->pub.g, me->x, 
		k, r, s, digest) != SUCCESS){
	LOG((ERR,"Unable to sign hash (DSS)!"));
	return(1);
    }
    bzero((char *)&sig, sizeof(sig_payload));
	/*
	 * fill packet with signature payload
	 */
    nbytes = sizeof(sig_payload) + (sizeof(r) + sizeof(s));
    sig.next_payload = nextp;
    sig.payload_len = nbytes;

    hton_sig(&sig);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&sig, (char *)(sa->packet + *pos), 
			sizeof(sig_payload));
    *pos += sizeof(sig_payload);
    hton_payload((char *)r, SHA_LENGTH);
    bcopy((char *)r, (char *)(sa->packet + *pos), SHA_LENGTH);
    *pos += SHA_LENGTH;
    hton_payload((char *)s, SHA_LENGTH);
    bcopy((char *)s, (char *)(sa->packet + *pos), SHA_LENGTH);
    *pos += SHA_LENGTH;

    return(0);
}

#ifdef NON_COMMERCIAL_AND_RSAREF
static int construct_rsa_sig (SA *sa, struct rsa_key_pair *me, 
			unsigned char nextp, int *pos)
{
    sig_payload sig;
    char digest[SHA_LENGTH], signed_digest[MAX_RSA_MODULUS_LEN];
    unsigned int digest_len, sig_len, nbytes;

    LOG((DEB, "constructing RSA signature"));
    switch(sa->hash_alg){
	case HASH_MD5:
	    digest_len = MD5_LEN;
	    break;
	case HASH_SHA:
	    digest_len = SHA_LENGTH;
	    break;
	default:
	    LOG((WARN, "Unknown hash algorithm"));
	    return(1);
    }
    compute_hash(sa, digest, MINE);
    if(RSAPrivateEncrypt(signed_digest, &sig_len, digest, digest_len,
			&me->priv)){
	LOG((ERR, "Unable to sign hash (RSA)!"));
	return(1);
    }
    bzero((char *)&sig, sizeof(sig_payload));
	/*
	 * fill packet with signature payload
	 */
    nbytes = sizeof(sig_payload) + sig_len;
    sig.next_payload = nextp;
    sig.payload_len = nbytes;

    hton_sig(&sig);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&sig, (char *)(sa->packet + *pos), 
			sizeof(sig_payload));
    *pos += sizeof(sig_payload);
    bcopy((char *)signed_digest, (char *)(sa->packet + *pos), sig_len);
    *pos += sig_len;

    return(0);
}
#endif

int construct_sig (SA *sa, unsigned char nextp, int *pos)
{ 
    union key_pair mykeys;

    if(sa->state_mask & CONST_AUTH)
	return(0);
	/*
	 * get daemon's pub/priv key pair
	 */
    if(sa->auth_alg == DSS_SIG){
	init_siglib(DSSKEYS);
    } else {
#ifdef NON_COMMERCIAL_AND_RSAREF
	init_siglib(RSAKEYS);
#else
	LOG((ERR, "RSA is for non-commercial use only!"));
	return(1);
#endif
    }
    if(get_priv_entry(&mykeys)){
	LOG((ERR, "Unable to obtain public/private key pair!"));
	term_siglib();
	return(1);
    }
    term_siglib();
    switch(sa->auth_alg){
	case DSS_SIG:
	    if(construct_dss_sig(sa, &mykeys.dss_pair, nextp, pos))
		return(1);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSA_SIG:
	    if(construct_rsa_sig(sa, &mykeys.rsa_pair, nextp, pos))
		return(1);
	    break;
#endif
	default:
	    LOG((WARN, "unknown AUTH method!"));
	    return(1);
    }

    sa->state_mask |= CONST_AUTH;
    return(0);
}

