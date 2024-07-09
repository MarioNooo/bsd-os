/*
 * Id: oakley.c,v 1.3 1997/07/24 19:42:05 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/oakley.c,v
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#else
#include <types.h>
#endif /* MULTINET */
#include <netinet/in.h>
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"
#include "authdef.h"
#include "hmac.h"

#ifdef MULTINET
extern void delay_stall_flush (SA *sa);
#endif /* MULTINET */

/*
 * oakley.c - as defined by draft-ietf-ipsec-isakmp-oakley-01.txt
 */

/*
 * called by initiator to begin a Quick Mode exchange. When kicked by the
 * kernel, requests for SA's are stored in the outstanding_sa bitmask.
 * Now go through that mask and generate _sa_list entries (see isadb.c)
 * for each potential SA
 */
void oakley_begin_qm(SA *sa)
{
    int did_something = 0, seq_num;
    sa_list *sa_ipsec;
    char rand[SHA_LENGTH];
    unsigned long mess_id;
	/*
	 * generate a pseudo-random message id
	 */
    get_rand(rand, 12);
    bcopy((char *)rand, (char *)&mess_id, sizeof(unsigned long));
    if(create_conn_entry(sa, mess_id)){
	LOG((ERR, "Can't create conn entry to begin Quick Mode"));
	return;
    }
	/*
	 * need to know whether to start PFS here. If we do then this
	 * is the place we compute our DH public number-- and set
	 * conn_entry->dh stuff.
	 */
    if(compute_quick_mode_iv(sa, mess_id)){
	LOG((ERR, "Can't create Quick Mode IV!"));
	isadb_delete_conn_entry(sa, mess_id, 0);
	return;
    }
	/*
	 * check all known protocols for outstanding SAs to negotiate.
	 * For each, request a spi, and unset the mask.
	 */
    if((sa->outstanding_sa & PROTO_IPSEC_AH_MASK) == PROTO_IPSEC_AH_MASK){
	seq_num = add_sa_to_list(sa, PROTO_IPSEC_AH, mess_id);
	sa_ipsec = get_sa_by_protocol(sa, PROTO_IPSEC_AH, mess_id);
	if(sa_ipsec == NULL){
	    LOG((ERR, "Just added a SA to the db and now it's gone!\n"));
	    return;
	}
	/*
	 * this would ideally be obtained via the PF_KEY and set upon
	 * creation of the IPSEC SA
	 */
	sa_ipsec->other_sa.ah.transform = AH_MD5_HMAC;
	get_unique_spi(sa, seq_num, PROTO_IPSEC_AH);
	sa->outstanding_sa ^= PROTO_IPSEC_AH_MASK;
	if (sa->proxy_dst.sin_family != 0)
	    sa_ipsec->other_sa.ah.tunnel = 1;
	did_something = 1;
    }
    if((sa->outstanding_sa & PROTO_IPSEC_ESP_MASK) == PROTO_IPSEC_ESP_MASK){
	seq_num = add_sa_to_list(sa, PROTO_IPSEC_ESP, mess_id);
	sa_ipsec = get_sa_by_protocol(sa, PROTO_IPSEC_ESP, mess_id);
	if(sa_ipsec == NULL){
	    LOG((ERR, "Just added a SA to the db and now it's gone!\n"));
	    return;
	}
	sa_ipsec->other_sa.esp.transform = ESP_DES_CBC;
	sa_ipsec->other_sa.esp.hmac_alg = ESP_HMAC_ALG_MD5;
	get_unique_spi(sa, seq_num, PROTO_IPSEC_ESP);
	sa->outstanding_sa ^= PROTO_IPSEC_ESP_MASK;
	if (sa->proxy_dst.sin_family != 0)
	    sa_ipsec->other_sa.esp.tunnel = 1;
	did_something = 1;
    }
    if(did_something){
	set_to_starve(sa, mess_id);
    }
}

static int oakley_final_qm (SA *sa, unsigned long message_id)
{
    hmac_CTX context;
    unsigned char hash_data[SHA_LENGTH], val;
    conn_entry *centry;
    hash_payload hash;
    int pos, hashlen = 0;
    unsigned long mess_id;

    bzero((char *)&hash, sizeof(hash_payload));
    centry = get_centry_by_mess_id(message_id);
    if(centry == NULL){
	LOG((ERR, "cannot obtain state for message id %ld\n", message_id));
	return(1);
    }
    mess_id = htonl(message_id);
    val = 0;
    sa->Inithmac(&context, sa->skeyid_a, sa->skeyid_len);
    sa->Updhmac(&context, &val, sizeof(unsigned char));
    sa->Updhmac(&context, (unsigned char *)&mess_id, sizeof(unsigned long));
    sa->Updhmac(&context, centry->nonce_I, centry->nonce_I_len);
    sa->Updhmac(&context, centry->nonce_R, centry->nonce_R_len);
    sa->Finhmac(hash_data, &context);

    switch(sa->hash_alg){
	case HASH_SHA:
	    hashlen = SHA_LENGTH;
	    break;
	case HASH_MD5:
	    hashlen = MD5_LEN;
	    break;
    }
    hash.next_payload = 0;
    hash.payload_len = hashlen + sizeof(hash_payload);
    hton_hash(&hash);
    construct_header(sa, OAK_QM, message_id, ISA_HASH);
    expand_packet(sa, sizeof(isakmp_hdr), (hashlen + sizeof(isakmp_hdr)));
    pos = sizeof(isakmp_hdr);
    bcopy((char *)&hash, (char *)(sa->packet + pos), sizeof(hash_payload));
    pos += sizeof(hash_payload);
    bcopy((char *)hash_data, (char *)(sa->packet + pos), hashlen);
    return(0);
}

/*
 * construct a Quick Mode message. Same for init--> resp and back again.
 * The last Quick Mode message is handled above.
 */
int oakley_const_qm (SA *sa, unsigned long mess_id)
{
    sa_list *leaf;
    conn_entry *node;
    int pos = sizeof(isakmp_hdr);

    node = get_centry_by_mess_id(mess_id);
    if(node == NULL){
	LOG((ERR, "Unable to do Quick Mode, no state!"));
	return(1);
    }
    leaf = get_sa_list(sa, mess_id);
    if(leaf == NULL){
	LOG((ERR,"Cannot Do Quick Mode. No SA's to negotiate!"));
	return(1);
    }
    construct_header(sa, OAK_QM, mess_id, ISA_HASH);
    construct_blank_hash(sa, ISA_SA, &pos);
    while(leaf != NULL){
	construct_ipsec_sa(sa, leaf, &pos);
	/*
	 * if the next proposals have flags they will have been taken care of
	 * in set_ipsec_proposals-- we don't want to create a seperate
	 * sa for these.
	 */
	while((leaf->next) && (leaf->next->flags))
	    leaf = leaf->next;
	leaf = leaf->next;
    }
    /*
     * need a check here for whether PFS is desired for the key of this SA.
     * Another thing that should be sent via PF_KEY.
     */
    if(node->proxy_dst.id_type && node->proxy_src.id_type){
	if(node->dh.group_desc){
	    construct_ipsec_nonce(sa, mess_id, ISA_KE, &pos);
	    construct_pfs_ke(sa, node, ISA_ID, &pos);
	} else {
	    construct_ipsec_nonce(sa, mess_id, ISA_ID, &pos);
	}
	construct_proxy_id(sa, &node->proxy_src, ISA_ID, &pos);
	construct_proxy_id(sa, &node->proxy_dst, 0, &pos);
    } else {
	if(node->dh.group_desc){
	    construct_ipsec_nonce(sa, mess_id, ISA_KE, &pos);
	    construct_pfs_ke(sa, node, 0, &pos);
	} else {
	    construct_ipsec_nonce(sa, mess_id, 0, &pos);
	}
    }
    construct_qm_hash(sa, mess_id, FALSE);
    sa->send_dir = BIDIR;
    return(0);
}

/*
 * konyets! We have SA's. Stuff them into the kernel
 */
static void load_all_sas(SA *sa, unsigned long mess_id)
{
    conn_entry *centry;
    sa_list *node;

    centry = get_centry_by_mess_id(mess_id);
    if(centry == NULL){
	LOG((ERR,"Cannot Store <NULL> security associations!"));
	return;
    }
    node = centry->sa_s;
    while(node != NULL){
	if(node->spi && node->other_spi){
	    if(gen_qm_key(sa, centry->nonce_I, centry->nonce_I_len, 
			   centry->nonce_R, centry->nonce_R_len, 
			   node, OUTBOUND) == 0){
		load_sa(sa, node, centry, OUTBOUND);
	    }
	    if(gen_qm_key(sa, centry->nonce_I, centry->nonce_I_len, 
			   centry->nonce_R, centry->nonce_R_len, 
			   node, INBOUND) == 0){
		load_sa(sa, node, centry, INBOUND);
	    }
	}
	node = node->next;
    }
}

/*
 * process a packet in the Main Mode exchange
 */
int oakley_process_main_mode (isakmp_hdr *hdr, SA *sa)
{
    unsigned char *pptr;
    unsigned long mess_id; 
    int pos;

	/*
	 * ignore any message id that may be sent in main mode
	 */
    mess_id = 0;
    pptr = sa->payload + sizeof(isakmp_hdr);
    switch(sa->state){
	case OAK_MM_NO_STATE:
	    if(process(hdr->next_payload, sa, 
			(sa->payload + sizeof(isakmp_hdr)), mess_id)){
		return(1);
	    }
	    if((sa->state_mask & PROC_SA) != PROC_SA){
		return(1);
	    }
	    sa->state = OAK_MM_SA_SETUP;
	    pos = sizeof(isakmp_hdr);
	    if(sa->init_or_resp == INITIATOR){
		construct_header(sa, OAK_MM, 0, ISA_KE);
		construct_ke(sa, ISA_NONCE, &pos);
		construct_nonce(sa, 0, &pos);
	    } else {
		construct_header(sa, OAK_MM, 0, ISA_SA);
		construct_isakmp_sa(sa, 0, &pos);
	    }
	    sa->send_dir = BIDIR;
	    break;
	case OAK_MM_SA_SETUP:
	    if(process(hdr->next_payload, sa,
			(sa->payload + sizeof(isakmp_hdr)), mess_id)){
		return(1);
	    }
	    if((sa->state_mask & PROC_KE) != PROC_KE){
		return(1);
	    }
	    sa->state = OAK_MM_KEY_EXCH;
	    pos = sizeof(isakmp_hdr);
	    if(sa->init_or_resp == INITIATOR){
		construct_header(sa, OAK_MM, 0, ISA_ID);
		switch(sa->auth_alg){
		    case DSS_SIG:
		    case RSA_SIG:
			construct_id(sa, ISA_SIG, &pos);
			construct_sig(sa, 0, &pos);
			break;
		    case RSA_ENC:
		    case PRESHRD:
			construct_id(sa, ISA_HASH, &pos);
			construct_hash(sa, 0, &pos);
			break;
		}
	    } else {
		construct_header(sa, OAK_MM, 0, ISA_KE);
		construct_ke(sa, ISA_NONCE, &pos);
		construct_nonce(sa, 0, &pos);
	    }
	    sa->send_dir = BIDIR;
	    break;
	case OAK_MM_KEY_EXCH:
	    if(process(hdr->next_payload, sa,
			(sa->payload + sizeof(isakmp_hdr)), mess_id)){
		return(1);
	    }
	    if((sa->state_mask & PROC_AUTH) != PROC_AUTH){
		return(1);
	    }
	    sa->state = OAK_MM_KEY_AUTH;
	    sa->condition = SA_COND_ALIVE;
	    if(sa->init_or_resp == INITIATOR){
		/* 
		 * for every outstanding SA, request a SPI. Control is lost
		 * here: don't bother setting send_dir, it'll be set in
		 * oakley_const_qm().
		 */
		oakley_begin_qm(sa);
	    } else {
		pos = sizeof(isakmp_hdr);
		construct_header(sa, OAK_MM, 0, ISA_ID);
		switch(sa->auth_alg){
		    case DSS_SIG:
		    case RSA_SIG:
			construct_id(sa, ISA_SIG, &pos);
			construct_sig(sa, 0, &pos);
			break;
		    case RSA_ENC:
		    case PRESHRD:
			construct_id(sa, ISA_HASH, &pos);
			construct_hash(sa, 0, &pos);
			break;
		}
		sa->send_dir = BIDIR;
	    }
	    break;
	case OAK_MM_KEY_AUTH:
	    break;
	default:
	    return(1);
    }
    return(0);
}

/*
 * that's right! this hasn't been completed and doesn't work. if you
 * fill in the blanks and implement this please send your changes back.
 */
int oakley_process_aggressive (isakmp_hdr *hdr, SA *sa)
{
    unsigned char *pptr;
    unsigned long mess_id;
    unsigned short expected_state;
    int pos;

    pptr = sa->payload + sizeof(isakmp_hdr);
    mess_id = hdr->mess_id;
    switch(sa->state){
	case OAK_AG_NOSTATE:
	    if(process(hdr->next_payload, sa, 
			(sa->payload + sizeof(isakmp_hdr)), mess_id)){
		return(1);
	    }
	    expected_state = PROC_SA | PROC_KE;
	    if((sa->state_mask & expected_state) != expected_state){
		return(1);
	    }
	    pos = sizeof(isakmp_hdr);
	    if(sa->init_or_resp == INITIATOR){
		if((sa->state_mask & PROC_AUTH) != PROC_AUTH){
		    return(1);
		}
		switch(sa->auth_alg){
		    case DSS_SIG:
		    case RSA_SIG:
			construct_header(sa, OAK_AG, 0, ISA_SIG);
			construct_sig(sa, 0, &pos);
			break;
		    case RSA_ENC:
		    case PRESHRD:
			construct_header(sa, OAK_AG, 0, ISA_HASH);
			construct_hash(sa, 0, &pos);
			break;
		}
		sa->state = OAK_AG_AUTH;
		sa->send_dir = UNIDIR;
	    } else {
		pos = sizeof(isakmp_hdr);
		construct_header(sa, OAK_AG, 0, ISA_SA);
		construct_isakmp_sa(sa, ISA_KE, &pos);
		construct_ke(sa, ISA_NONCE, &pos);
		construct_nonce(sa, ISA_ID, &pos);
		switch(sa->auth_alg){
		    case DSS_SIG:
		    case RSA_SIG:
			construct_id(sa, ISA_SIG, &pos);
			construct_sig(sa, 0, &pos);
			break;
		    case RSA_ENC:
		    case PRESHRD:
			construct_id(sa, ISA_HASH, &pos);
			construct_hash(sa, 0, &pos);
			break;
		}
		sa->state = OAK_AG_INIT_EXCH;
		sa->send_dir = BIDIR;
	    }
	    break;
	case OAK_AG_INIT_EXCH:
	    if(process(hdr->next_payload, sa, 
			(sa->payload + sizeof(isakmp_hdr)), mess_id)){
		return(1);
	    }
	    expected_state = PROC_AUTH;
	    if((sa->state_mask & expected_state) != expected_state){
		return(1);
	    }
	    if(sa->init_or_resp == RESPONDER){
		sa->state = OAK_AG_AUTH;
	    }
	    break;
	case OAK_AG_AUTH:
	    break;
    }
    return(0);
}

/*
 * process a packet in the Quick Mode exchange
 */
int oakley_process_quick_mode (isakmp_hdr *hdr, SA *sa)
{
    unsigned char *pptr, nextp;
    unsigned long mess_id;
    conn_entry *centry;

    pptr = sa->payload + sizeof(isakmp_hdr);
    mess_id = hdr->mess_id;
	/*
	 * the 1st payload of quick mode MUST be a hash. If it's not, or if
	 * the hash is incorrect then delete the conn_entry if we just
	 * created it (to get the IV to decrypt it in the 1st place!).
	 */
    if(hdr->next_payload != ISA_HASH){
	if((sa->state == OAK_QM_IDLE) || (sa->state == OAK_MM_KEY_AUTH))
	    isadb_delete_conn_entry(sa, mess_id, 0);
	return(1);
    }
    if(verify_qm_hash(sa, hdr, &pptr, &nextp, mess_id)){
	if((sa->state == OAK_QM_IDLE) || (sa->state == OAK_MM_KEY_AUTH))
	    isadb_delete_conn_entry(sa, mess_id, 0);
	return(1);
    }
    switch(sa->state){
	case OAK_QM_IDLE:	/* responder getting another QM */
	    sa->init_or_resp = RESPONDER;
	case OAK_MM_KEY_AUTH:
	case OAK_AG_AUTH:

	    if(process(nextp, sa, pptr, mess_id)){
		return(1);
	    }
	    if((sa->state_mask & PROC_NEG) == 0){
		return(1);
	    }
	    if((centry = get_centry_by_mess_id(mess_id)) == NULL){
		LOG((ERR, "Unable to get conn_entry after processing QM!"));
		return(1);
	    }
	    if((centry->dh.group_desc) && 
	      ((centry->flags & PFS_ACCOMPLISHED) == 0)){
		LOG((ERR, "No KE payload for PFS for %ld", mess_id));
		return(1);
	    }
	    if(sa->init_or_resp == INITIATOR){
		load_all_sas(sa, mess_id);
		oakley_final_qm(sa, mess_id);
#ifdef MULTINET
		delay_stall_flush(sa);
#endif /* MULTINET */
		delay_conn_entry_deletion(sa, mess_id);
		sa->send_dir = UNIDIR;
		sa->state = OAK_QM_IDLE;
	    } else {
		/*
		 * we successfully processed a QM message, therefore we're
		 * guaranteed to have one or more outstanding SAs. Wait for 
		 * spi. Control is lost as in main mode, don't set send_dir.
		 */
		set_to_starve(sa, mess_id);
		sa->state = OAK_QM_AUTH_AWAIT;
	    }
	    break;
	case OAK_QM_AUTH_AWAIT:
	    /*
	     * shouldn't be anything else to process, but if this guy did
	     * send something then let's humor him...
	     */
	    if(nextp){
		if(process(nextp, sa, pptr, mess_id)){
		    return(1);
		}
	    }
	    if(sa->init_or_resp == RESPONDER){
		load_all_sas(sa, mess_id);
		isadb_delete_conn_entry(sa, mess_id, 0);
		sa->state = OAK_QM_IDLE;
	    }
	    break;
    }
    return(0);
}

