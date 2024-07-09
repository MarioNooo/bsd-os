/* 
 * Id: utils.c,v 1.3 1997/07/24 19:42:15 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/utils.c,v
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
#include <unistd.h>
#else
#include <types.h>
#include <socket.h>
#endif /* MULTINET */
#include <netinet/in.h>
#include <errno.h>
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"
#include "authdef.h"
#include "hmac.h"

/*
 * list of weak and semi-weak DES keys 
 *    from Applied Cryptography: Protocols, Algorithms, and Source Code in C,
 *		first edition (yea, I've still got the 1st edition)
 *	   by Bruce Schneier
 */
#define NUM_WEAK_KEYS 16
unsigned char weak_keys[][DES_KEYLEN] = {
		/* the weak keys */
	{ 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe },
	{ 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f },
	{ 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0 },
		/* the semi-weak keys */
	{ 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe },
	{ 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0 },
	{ 0x01, 0xe0, 0x01, 0xe0, 0x01, 0xe0, 0x01, 0xe0 }, 
	{ 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe }, 
	{ 0x01, 0x1f, 0x01, 0x1f, 0x01, 0x1f, 0x01, 0x1f },
	{ 0xe0, 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xe0, 0xfe },
	{ 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01 },
	{ 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f },
	{ 0xe0, 0x01, 0xe0, 0x01, 0xe0, 0x01, 0xe0, 0x01 },
	{ 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f },
	{ 0x1f, 0x01, 0x1f, 0x01, 0x1f, 0x01, 0x1f, 0x01 },
	{ 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xe0 }
};

/*
 * utils.c -- generic utilities. hton- and ntoh- routines for payloads,
 *	ISAKMP packet growth routine and DES is-/make- odd parity routines.
 */

#ifdef i386
/*
 * host <==> network routines for header
 */
void hton_hdr (isakmp_hdr *hdr)
{
    hdr->len = htonl(hdr->len);
    hdr->mess_id = htonl(hdr->mess_id);
}
void ntoh_hdr (isakmp_hdr *hdr)
{
    hdr->len = ntohl(hdr->len);
    hdr->mess_id = ntohl(hdr->mess_id);
}

/*
 * host <==> network routines for sa_payload
 */
void hton_sa (sa_payload *sa_sa)
{
    sa_sa->payload_len = htons(sa_sa->payload_len);
    sa_sa->doi = htonl(sa_sa->doi);
    sa_sa->situation = htonl(sa_sa->situation);
}
void ntoh_sa (sa_payload *sa_sa)
{
    sa_sa->payload_len = ntohs(sa_sa->payload_len);
    sa_sa->doi = ntohl(sa_sa->doi);
    sa_sa->situation = ntohl(sa_sa->situation);
}

void hton_dummy (generic_payload *dummy)
{
    dummy->payload_len = htons(dummy->payload_len);
}
void ntoh_dummy (generic_payload *dummy)
{
    dummy->payload_len = ntohs(dummy->payload_len);
}

/*
 * host <==> network routines for key exchange payload
 */
void hton_key (key_x_payload *key)
{
    key->payload_len = htons(key->payload_len);
}
void ntoh_key (key_x_payload *key)
{
    key->payload_len = ntohs(key->payload_len);
}

void hton_sig (sig_payload *sig)
{
    sig->payload_len = htons(sig->payload_len);
}
void ntoh_sig (sig_payload *sig)
{
    sig->payload_len = ntohs(sig->payload_len);
}

void hton_nonce (nonce_payload *nonce)
{
    nonce->payload_len = htons(nonce->payload_len);
}
void ntoh_nonce (nonce_payload *nonce)
{
    nonce->payload_len = ntohs(nonce->payload_len);
}

void hton_hash (hash_payload *hash)
{ 
    hash->payload_len = htons(hash->payload_len);
}
void ntoh_hash (hash_payload *hash)
{
    hash->payload_len = ntohs(hash->payload_len);
}

void hton_id (id_payload *id)
{
    id->payload_len = htons(id->payload_len);
    id->port = htons(id->port);
}
void ntoh_id (id_payload *id)
{
    /*
     * don't convert the port since it's hashed in network order and if
     * we convert it we can't authenticate
     */
    id->payload_len = ntohs(id->payload_len);
}

void hton_notify (notify_payload *notify)
{
    notify->payload_len = htons(notify->payload_len);
    notify->notify_message = htons(notify->notify_message);
    notify->doi = htonl(notify->doi);
}
void ntoh_notify (notify_payload *notify)
{
    notify->payload_len = ntohs(notify->payload_len);
    notify->notify_message = ntohs(notify->notify_message);
    notify->doi = ntohl(notify->doi);
}

void hton_delete (delete_payload *del)
{
    del->payload_len = htons(del->payload_len);
}

void ntoh_delete (delete_payload *del)
{
    del->payload_len = ntohs(del->payload_len);
}

/*
 * host <==> network routines for proposals
 */
void hton_proposal (proposal_payload *prop)
{
    prop->payload_len = htons(prop->payload_len);
}
void ntoh_proposal (proposal_payload *prop)
{
    prop->payload_len = ntohs(prop->payload_len);
}

void hton_transform (trans_payload *trans)
{
    trans->payload_len = htons(trans->payload_len);
}

void ntoh_transform (trans_payload *trans)
{
    trans->payload_len = ntohs(trans->payload_len);
}

/*
 * host <==> network routines for attributes
 */
void hton_att (sa_attribute *att)
{
    ushort blah;

    bcopy((char *)att, (char *)&blah, sizeof(ushort));
    blah = htons(blah);
    bcopy((char *)&blah, (char *)att, sizeof(ushort));
    att->att_type.basic_value = htons(att->att_type.basic_value);

}

void ntoh_att (sa_attribute *att)
{
    ushort blah;

    bcopy((char *)att, (char *)&blah, sizeof(ushort));
    blah = ntohs(blah);
    bcopy((char *)&blah, (char *)att, sizeof(ushort));
    att->att_type.basic_value = ntohs(att->att_type.basic_value);
}

void hton_payload (char *p, int len)
{
    int i;
    char temp;

    for(i=0; i<len; i+=2){
	temp = p[i];
	p[i] = p[i+1];
	p[i+1] = temp;
    }
}

#endif

/*
 * this grows the output packet of the SA as we create more piggybacked
 * payloads.
 */
void expand_packet (SA *sa, int position, int numbytes)
{
    int n;
    unsigned char *temp;

    if(sa->packet_len > (position + numbytes))
	return;
    n = (position + numbytes) - sa->packet_len;
    n = ROUNDUP(n);
    if(sa->packet_len == 0){
	sa->packet_len = n;
	sa->packet = (unsigned char *) malloc (sa->packet_len);
    } else {
	sa->packet_len += n;
	temp = (unsigned char *) realloc (sa->packet, sa->packet_len);
	sa->packet = temp;
    }
    bzero((char *)(sa->packet + position), (sa->packet_len - position));
}

int compute_quick_mode_iv (SA *sa, unsigned long message_id)
{
    hmac_CTX context;
    unsigned char iv_stuff[SHA_LENGTH];
    conn_entry *centry;
    unsigned long mess_id;

    centry = get_centry_by_mess_id(message_id);
    if(centry == NULL){
	LOG((ERR, "Can't find conn entry for message id %ld", message_id));
	return(1);
    }
    if(centry->phase2_iv_len){
	LOG((WARN, "Already have a phase 2 IV!"));
	return(1);
    }
    centry->phase2_iv = (unsigned char *) malloc (sa->crypt_iv_len);
    if(centry->phase2_iv == NULL){
	LOG((ERR, "Out Of Memory. Can't create QM IV!"));
	return(1);
    }
    mess_id = ntohl(message_id);
    centry->phase2_iv_len = sa->crypt_iv_len;
    sa->InitHash(&context);
    sa->UpdHash(&context, sa->crypt_iv, sa->crypt_iv_len);
    sa->UpdHash(&context, (unsigned char *)&mess_id, sizeof(unsigned long));
    sa->FinHash(iv_stuff, &context);
    bcopy((char *)iv_stuff, centry->phase2_iv, centry->phase2_iv_len);
    return(0);
}

/*
 * key[i] = odd_parity[key[i])] would've been preferable but
 * since the odd_parity array has been copyrighted (thanks alot!)
 * we'll do if(is_odd(key[i]) make_odd(&key[i]);
 */
static void make_odd (unsigned char *component)
{
    if((*component)%2)
	*component &= 0xfe;
    else
	*component |= 0x01;
}

static int is_odd (unsigned char component)
{
    int i=0;

    while(component){
	if(component%2)
	    i++;
	component = component>>1;
    }
    return(i%2);
}

/*
 * take 1st DES_KEYLEN bytes of keybits which aren't in the weak key list
 */

static int copy_des_key(unsigned char *keybits, int keybitlen, 
			unsigned char *key)
{
    unsigned char des_key[DES_KEYLEN];
    int i,j, status = 1;

    if((keybits == NULL) || (key == NULL) || (keybitlen < DES_KEYLEN))
	return(status);

    for(i=0; i<(keybitlen - DES_KEYLEN); i++){
	status = 0;
	bcopy((char *)(keybits + i), des_key, DES_KEYLEN);
	for(j=0; j<DES_KEYLEN; j++)
	    if(!is_odd(des_key[j]))
		make_odd(&des_key[j]);
	for(j=0; j<NUM_WEAK_KEYS; j++)
	    if(bcmp(des_key, weak_keys[j], DES_KEYLEN) == 0){
		status = 1;
		break;
	    }
	if(status == 0){
	    bcopy(des_key, key, DES_KEYLEN);
	    break;
	}
    }
    return(status);
}

/*
 * generate SKEYID as defined in Oakley 
 */
void gen_skeyid (SA *sa)
{
    hmac_CTX context;
    union public_key rec;
    unsigned char val, *key, iv_stuff[SHA_LENGTH];

    if(sa->hash_alg == HASH_MD5){
	sa->skeyid_len = MD5_LEN;
    } else {
	sa->skeyid_len = SHA_LENGTH;
    }
    sa->skeyid = (unsigned char *) malloc (sa->skeyid_len);
    if(sa->skeyid == NULL){
	LOG((CRIT, "Out Of Memory"));
	return;
    }
    sa->skeyid_d = (unsigned char *) malloc (sa->skeyid_len);
    if(sa->skeyid_d == NULL){
	LOG((CRIT, "Out Of Memory"));
	return;
    }
    sa->skeyid_e = (unsigned char *) malloc (sa->skeyid_len);
    if(sa->skeyid_e == NULL){
	LOG((CRIT, "Out Of Memory"));
	return;
    }
    sa->skeyid_a = (unsigned char *) malloc (sa->skeyid_len);
    if(sa->skeyid_a == NULL){
	LOG((CRIT, "Out Of Memory"));
	return;
    }
    bzero((char *)sa->skeyid, sa->skeyid_len);
    bzero((char *)sa->skeyid_d, sa->skeyid_len);
    bzero((char *)sa->skeyid_a, sa->skeyid_len);
    bzero((char *)sa->skeyid_e, sa->skeyid_len);
    switch(sa->auth_alg){
	case PRESHRD:
	    init_siglib(PRESHAR);
	    if(sa->init_or_resp == INITIATOR){
		if(get_pub_rec_by_addr(&sa->dst.sin_addr, &rec, 0)){
		    LOG((ERR, "no pre-shared key for remote host!"));
		    term_siglib();
		    return;
		}
	    } else {
		if(get_pub_rec_by_addr(&sa->src.sin_addr, &rec, 0)){
		    LOG((ERR, "no pre-shared key for remote host!"));
		    term_siglib();
		    return;
		}
	    }
	    sa->Inithmac(&context, rec.preshr.key, rec.preshr.keylen);
	    sa->Updhmac(&context, sa->nonce_I, sa->nonce_I_len);
	    sa->Updhmac(&context, sa->nonce_R, sa->nonce_R_len);
	    break;
	case DSS_SIG:
	case RSA_SIG:
	    key = (unsigned char *)malloc(sa->nonce_I_len + sa->nonce_R_len);
	    if(key == NULL){
		LOG((ERR, "Out Of Memory. Cannot create skeyid!"));
		return;
	    }
	    bcopy((char *)sa->nonce_I, key, sa->nonce_I_len);
	    bcopy((char *)sa->nonce_R, (key+sa->nonce_I_len), sa->nonce_R_len);
	    sa->Inithmac(&context, key, (sa->nonce_I_len + sa->nonce_R_len));
	    sa->Updhmac(&context, sa->dh.g_to_xy, sa->dh.dh_len);
	    free(key);
	    break;
	case RSA_ENC:
	    key = (unsigned char *)malloc(sa->nonce_I_len + sa->nonce_R_len);
	    if(key == NULL){
		LOG((ERR, "Out Of Memory. Cannot create skeyid!"));
		return;
	    }
	    bcopy((char *)sa->nonce_I, key, sa->nonce_I_len);
	    bcopy((char *)sa->nonce_R, (key+sa->nonce_I_len), sa->nonce_R_len);
	    sa->Inithmac(&context, key, (sa->nonce_I_len + sa->nonce_R_len));
	    if(sa->init_or_resp == INITIATOR){
		sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
		sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	    } else {
		sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
		sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	    }
	    free(key);
	    break;
	default:
	    LOG((ERR, "Unknown authentication method!"));
	    return;
    }
    sa->Finhmac(sa->skeyid, &context);

    val = 0;
    sa->Inithmac(&context, sa->skeyid, sa->skeyid_len);
    sa->Updhmac(&context, sa->dh.g_to_xy, sa->dh.dh_len);
    if(sa->init_or_resp == INITIATOR){
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
    } else {
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
    }
    sa->Updhmac(&context, &val, sizeof(unsigned char));
    sa->Finhmac(sa->skeyid_d, &context);

    val = 1;
    sa->Inithmac(&context, sa->skeyid, sa->skeyid_len);
    sa->Updhmac(&context, sa->skeyid_d, sa->skeyid_len);
    sa->Updhmac(&context, sa->dh.g_to_xy, sa->dh.dh_len);
    if(sa->init_or_resp == INITIATOR){
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
    } else {
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
    }
    sa->Updhmac(&context, &val, sizeof(unsigned char));
    sa->Finhmac(sa->skeyid_a, &context);

    val = 2;
    sa->Inithmac(&context, sa->skeyid, sa->skeyid_len);
    sa->Updhmac(&context, sa->skeyid_a, sa->skeyid_len);
    sa->Updhmac(&context, sa->dh.g_to_xy, sa->dh.dh_len);
    if(sa->init_or_resp == INITIATOR){
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
    } else {
	sa->Updhmac(&context, sa->his_cookie, COOKIE_LEN);
	sa->Updhmac(&context, sa->my_cookie, COOKIE_LEN);
    }
    sa->Updhmac(&context, &val, sizeof(unsigned char));
    sa->Finhmac(sa->skeyid_e, &context);

    switch(sa->encr_alg){
	case ET_DES_CBC:
	    copy_des_key(sa->skeyid_e, sa->skeyid_len, sa->crypt_key);
	    sa->InitHash(&context);
	    if(sa->init_or_resp == INITIATOR){
		sa->UpdHash(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
		sa->UpdHash(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
	    } else {
		sa->UpdHash(&context, sa->dh.his_DH_pub_val, sa->dh.dh_len);
		sa->UpdHash(&context, sa->dh.DH_pub_val, sa->dh.dh_len);
	    }
	    sa->FinHash(iv_stuff, &context);
	    bcopy((char *)iv_stuff, (char *)sa->crypt_iv, DES_KEYLEN);
	    break;
	default:
	    LOG((ERR,"Unknown crypto-alg"));
	    return;
    }
    return;
}

/*
 * generate the key the results from a Quick Mode exchange
 */
int gen_qm_key (SA *sa, unsigned char *nonce_I, unsigned short nonce_I_len,
		unsigned char *nonce_R, unsigned short nonce_R_len, 
		sa_list *node, int direction)
{
    hmac_CTX context;
    unsigned char *key;
    int hashlen, keylen = 0, req_keylen = 0, lastbunch = 0;
    unsigned long spi;

    switch(node->other_sa.type){
	case PROTO_IPSEC_AH:
	    switch(node->other_sa.ah.transform){
		case AH_MD5_HMAC:
		    req_keylen = MD5_LEN;
		    break;
		case AH_SHA_HMAC:
		    req_keylen = SHA_LENGTH;
		    break;
		case AH_MD5_KPDK:
		    req_keylen = MD5_LEN;
		    break;
	    }
	    break;
	case PROTO_IPSEC_ESP:
	    switch(node->other_sa.esp.transform){
		case ESP_DES_CBC:
		case ESP_DES:
		    req_keylen = DES_KEYLEN;
		    break;
		case ESP_3DES:
		    req_keylen = (3 * DES_KEYLEN);
		    break;
	    }
	    switch(node->other_sa.esp.hmac_alg){
		case ESP_HMAC_ALG_MD5:
		    req_keylen += MD5_LEN;
		    break;
		case ESP_HMAC_ALG_SHA:
		    req_keylen += SHA_LENGTH;
		    break;
	    }
	    break;
    }
    if(sa->hash_alg == HASH_MD5){
	keylen = hashlen = MD5_LEN;
    } else {
	keylen = hashlen = SHA_LENGTH;
    } 
    if(keylen < req_keylen){
	keylen += hashlen;	/* a little slop on the end don't hurt */
    }
    if(direction == INBOUND){
	node->inbound_crypt_key = (unsigned char *) malloc (keylen);
	if(node->inbound_crypt_key == NULL){
	    LOG((CRIT, "Out Of Memory"));
	    return(1); 
	}
	key = node->inbound_crypt_key;
	node->inbound_crypt_keylen = req_keylen;
    } else {
	node->outbound_crypt_key = (unsigned char *) malloc (keylen);
	if(node->outbound_crypt_key == NULL){
	    LOG((CRIT, "Out Of Memory"));
	    return(1); 
	}
	key = node->outbound_crypt_key;
	node->outbound_crypt_keylen = req_keylen;
    }
    keylen = lastbunch = 0;
	/*
	 * expand the key if necessary
	 */
    do {
	sa->Inithmac(&context, sa->skeyid_d, sa->skeyid_len);
	if(keylen){
	    sa->Updhmac(&context, (key + lastbunch), hashlen);
	    lastbunch += hashlen;
	}
	if((((conn_entry *)node->parent))->flags & (PFS_ACCOMPLISHED)){
	    sa->Updhmac(&context, ((conn_entry *)(node->parent))->dh.g_to_xy,
			((conn_entry *)(node->parent))->dh.dh_len);
	}
	sa->Updhmac(&context, &node->other_sa.type, sizeof(unsigned char));
	if(direction == INBOUND){
	    spi = htonl(node->spi);
	} else {
	    spi = htonl(node->other_spi);
	}
	sa->Updhmac(&context, (unsigned char *)&spi, sizeof(unsigned long));
	sa->Updhmac(&context, nonce_I, nonce_I_len);
	sa->Updhmac(&context, nonce_R, nonce_R_len);
	sa->Finhmac((key + keylen), &context);
	keylen += hashlen;
    } while(keylen < req_keylen);
#ifdef KEY_DEBUG
printf("in the %s direction:\n", direction == INBOUND ? "inbound" : "outbound");
print_vpi("base IPSec key", key, keylen);
#endif
	/*
	 * for combined transforms the encryption key is first so we know
	 * what to copy if the cipher needs copying. DES does.
	 */
    if(node->other_sa.type == PROTO_IPSEC_ESP){
	switch(node->other_sa.esp.transform){
	    case ESP_DES_CBC:
	    case ESP_DES:
		copy_des_key(key, DES_KEYLEN, key);
		break;
	    default:
	/*
	 * when adding crypto algs remember to inc val if need be
	 */
		LOG((ERR,"Unknown crypto-alg"));
		break;
	}
    }
    return(0);
}

