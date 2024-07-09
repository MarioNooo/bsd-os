/* 
 * Id: isadb.c,v 1.3 1997/07/24 19:41:56 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/isadb.c,v
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
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <types.h>
#include <socket.h>
#include <netinet/in.h>
#endif /* MULTINET */
#include <errno.h>
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * isadb.c -- database routines for both ISAKMP and IPSec security associations
 * 	Each connection has a SA entry (in the form of a struct SA in dblist). 
 * 	When a SA begins negotiation of IPsec SA's another entry is made in
 * 	the connection list (in the form of a struct _conn_entry in conn_list).
 *	Each IPsec SA being negotiated is then put in a linked-list off the
 *	node in the conn_list. (see isadb.h).
 */

SA *dblist = NULL;
conn_entry *conn_list = NULL;

int isadb_get_seq()
{
  static int seq_num = 0;
  if(seq_num > 65500)
    seq_num = 1000;
  return(++seq_num);
}
	/*
	 * routines to control db of ISAKMP "security associations" 
	 */
SA *isadb_create_entry(struct sockaddr_in *addr, short i_or_r,
		unsigned short type)
{
    SA *node;

    node = (SA *) malloc (sizeof(SA));
    if(node == NULL){
	LOG((CRIT,"Out Of Memory"));
	return(NULL);
    }
    bzero((char *)node, sizeof(SA));
	/*
	 * The initiator records which IPsec SA's he's been asked to create
	 * to ignore repeats if negotiation takes a long time.
	 * The responder doesn't care. He'll ignore requests with matching
	 * cookies.
	 */
    if(i_or_r == INITIATOR){
	switch(type){
	    case PROTO_IPSEC_AH:
		node->outstanding_sa |= PROTO_IPSEC_AH_MASK;
		break;
	    case PROTO_IPSEC_ESP:
		node->outstanding_sa |= PROTO_IPSEC_ESP_MASK;
		break;
	    default:
		LOG((WARN, "Asked to create an unknown SA! (%d)",type));
	}
    }
    bcopy((char *)addr, (char *)&node->src, sizeof(struct sockaddr_in));
    node->state_mask = node->state = 0;
	/*
	 * the protocol changes whether you're the initiator or the
	 * responder. Since the ISAKMP SA is bi-directional this may change
	 */
    node->init_or_resp = i_or_r;
	/*
	 * messages are sent out in a certain format (e.g.cookie ordering).
	 * This will never change regardless of the direction of the SA.
	 */
    node->in_or_out = i_or_r;
    node->next = dblist;
    node->fd = -1;
    dblist = node;

    return(node);
}

SA *isadb_get_entry_by_addr (struct sockaddr_in *from)
{
    unsigned char hash[COOKIE_LEN];
    SA *node;

    gen_cookie(from, hash);
    for(node = dblist; node; node = node->next){
	if(!bcmp((char *)hash, (char *)node->my_cookie, COOKIE_LEN)){
	    return(node);
	}
    }
    return(NULL);
}

SA *isadb_get_entry (isakmp_hdr *hdr, struct sockaddr_in *from)
{
    SA *node;

    for(node = dblist; node; node = node->next){
	if(hdr->next_payload == ISA_SA){
	    if(node->init_or_resp == INITIATOR){
		if(!bcmp((char *)from, (char *)&node->dst, 
					sizeof(struct sockaddr_in)) &&
		     !bcmp((char *)hdr->init_cookie, node->my_cookie, 
					COOKIE_LEN)){
		    return(node);
		}
	    }
		/*
		 * shouldn't be called upon receipt of 1st packet 
		 * therefore responder shouldn't return a valid node
		 */
	    continue;
	}
	if(node->in_or_out == OUTBOUND){
	    if(!bcmp((char *)hdr->init_cookie, node->my_cookie, COOKIE_LEN) &&
		!bcmp((char *)hdr->resp_cookie, node->his_cookie, COOKIE_LEN)&&
		!bcmp((char *)from, (char *)&node->dst, 
					sizeof(struct sockaddr_in)))
		return(node);
	} else {
	    if(!bcmp((char *)hdr->resp_cookie, node->my_cookie, COOKIE_LEN) &&
		!bcmp((char *)hdr->init_cookie,node->his_cookie,COOKIE_LEN))
		return(node);
	}
    }
    return(NULL);
}

void isadb_delete_entry(SA *delme)
{
    SA *node;

#ifdef MULTINET
    if(dblist) {
#endif /* MULTINET */
    if(delme == dblist){
	dblist = dblist->next;
    } else {
	for(node = dblist; node->next; node = node->next){
	    if(node->next == delme)
		break;
	}
	if(node->next == NULL){
	    LOG((ERR,"dB entry not found (delete)!"));
	    return;
	}
	node->next = delme->next;
    }
#ifdef MULTINET
    }
#endif /* MULTINET */
    if(delme->packet)
	free(delme->packet);
    if(delme->dh.DH_priv_val)
	free(delme->dh.DH_priv_val);
    if(delme->dh.DH_pub_val)
	free(delme->dh.DH_pub_val);
    if(delme->dh.his_DH_pub_val)
	free(delme->dh.his_DH_pub_val);
    if(delme->dh.g_to_xy)
	free(delme->dh.g_to_xy);
    if(delme->skeyid)
	free(delme->skeyid);
    if(delme->skeyid_d)
	free(delme->skeyid_d);
    if(delme->skeyid_e)
	free(delme->skeyid_e);
    if(delme->skeyid_a)
	free(delme->skeyid_a);
    if(delme->crypt_key)
	free(delme->crypt_key);
    if(delme->crypt_iv)
	free(delme->crypt_iv);
    switch(delme->dh.group.type){
	case GP_MODP:
	    if(delme->dh.group.modp.p)
		free(delme->dh.group.modp.p);
	    if(delme->dh.group.modp.g)
		free(delme->dh.group.modp.g);
	    break;
	case GP_ECP:
	    if(delme->dh.group.ecp.p)
		free(delme->dh.group.ecp.p);
	    if(delme->dh.group.ecp.curve_p1)
		free(delme->dh.group.ecp.curve_p1);
	    if(delme->dh.group.ecp.curve_p2)
		free(delme->dh.group.ecp.curve_p2);
	    if(delme->dh.group.ecp.g1)
		free(delme->dh.group.ecp.g1);
	    if(delme->dh.group.ecp.g2)
		free(delme->dh.group.ecp.g2);
	    break;
	default:
	    LOG((ERR, "Something happened to the Oakley Group!"));
    }
    free(delme);
    return;
}

/*
 * routines to control db of security association <--> sequence 
 * number to maintain outstanding requests for SPIs and hold SAs
 * until they can be given to the kernel.
 */

int create_conn_entry (SA *sa, unsigned long message_id)
{
    conn_entry *node, *prev;

    if(conn_list == NULL){
	conn_list = (conn_entry *) malloc (sizeof(conn_entry));
	if(conn_list == NULL){
	    LOG((CRIT,"Out Of Memory"));
	    return(-1);
	}
	bzero((char *)conn_list, sizeof(conn_entry));
	bcopy((char *)sa->my_cookie, conn_list->my_cookie, COOKIE_LEN);
	bcopy((char *)sa->his_cookie, conn_list->his_cookie, COOKIE_LEN);
	conn_list->message_id = message_id;
	conn_list->nonce_I = NULL;
	conn_list->nonce_I_len = 0;
	conn_list->nonce_R = NULL;
	conn_list->nonce_R_len = 0;
	conn_list->phase2_iv = NULL;
	conn_list->phase2_iv_len = 0;
	conn_list->sa_s = NULL;
	conn_list->next = NULL;
	/*
	 * If a proxy destination address exists, set up the proxy
	 * identity addresses now.
	 */
	if (sa->proxy_dst.sin_family == AF_INET) {
	    conn_list->proxy_src.id_type = ID_IPV4_ADDR;
	    bcopy((char *)&sa->src, (char *)&conn_list->proxy_src.addr, 
				sizeof(struct sockaddr_in));
	    conn_list->proxy_dst.id_type = ID_IPV4_ADDR;
	    bcopy((char *)&sa->proxy_dst, (char *)&conn_list->proxy_dst.addr, 					sizeof(struct sockaddr_in));
	}
	return(0);
    }
	/*
	 * check for existing entry, if there is one return.
	 */
    for(prev = NULL, node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == message_id))
	    return(1);
	prev = node;
    }
	/*
	 * no entry, create one for this ISAKMP conn
	 */
    prev->next = (conn_entry *) malloc (sizeof(conn_entry));
    if(prev->next == NULL){
	LOG((CRIT,"Out Of Memory"));
	return(-1);
    }
    bcopy((char *)sa->my_cookie, prev->next->my_cookie, COOKIE_LEN);
    bcopy((char *)sa->his_cookie, prev->next->his_cookie, COOKIE_LEN);
    prev->next->message_id = message_id;
    prev->next->nonce_I = NULL;
    prev->next->nonce_I_len = 0;
    prev->next->nonce_R = NULL;
    prev->next->nonce_R_len = 0;
    prev->next->phase2_iv = NULL;
    prev->next->phase2_iv_len = 0;
    prev->next->sa_s = NULL;
    prev->next->next = NULL;
	/*
	 * If a proxy destination address exists, set up the proxy
	 * identity addresses now.
	 */
    if (sa->proxy_dst.sin_family == AF_INET) {
	conn_list->proxy_src.id_type = ID_IPV4_ADDR;
	bcopy((char *)&sa->src, (char *)&prev->proxy_src.addr, 
			sizeof(struct sockaddr_in));
	conn_list->proxy_dst.id_type = ID_IPV4_ADDR;
	bcopy((char *)&sa->proxy_dst, (char *)&prev->proxy_dst.addr, 
			sizeof(struct sockaddr_in));
    }
    return(0);
}

int add_sa_to_list (SA *sa, unsigned char proto_id, unsigned long message_id)
{
    conn_entry *node;
    sa_list *ip_sa;
	/*
	 * by default entries are created as AND_NEXT_PROPOSAL. If and
	 * when the ability to specify logical construction to isakmp
	 * arrives this can be changed. For now _everything_ is AND.
	 */

    for(node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == message_id)){
		/*
		 * check whether this entry has a sa list yet, if not create it
		 */
	    if(node->sa_s == NULL){
		node->sa_s = (sa_list *) malloc (sizeof(sa_list));
		if(node->sa_s == NULL){
		    LOG((CRIT, "Out Of Memory"));
		    return(-1);
		}
		bzero((char *)node->sa_s, sizeof(sa_list));
		node->sa_s->next = NULL;
		ip_sa = node->sa_s;
	    } else {
		/*
		 * otherwise add one on the end
		 */
		for(ip_sa = node->sa_s; ip_sa->next != NULL; 
			ip_sa = ip_sa->next){
		    if(ip_sa->other_sa.type == proto_id){
		/*
		 * we're already doing one of these!
		 */
			return(0);
		    }
		}
		ip_sa->next = (sa_list *) malloc (sizeof(sa_list));
		if(ip_sa->next == NULL){
		    LOG((CRIT, "Out Of Memory"));
		    return(-1);
		}
		bzero((char *)ip_sa->next, sizeof(sa_list));
		ip_sa->next->next = NULL;
		ip_sa = ip_sa->next;
	    }
		/*
		 * create the sequence number as a place holder to find again
		 */
	    ip_sa->key_seq = isadb_get_seq();
	    ip_sa->inbound_crypt_key = NULL;
	    ip_sa->inbound_crypt_keylen = 0;
	    ip_sa->outbound_crypt_key = NULL;
	    ip_sa->outbound_crypt_keylen = 0;
	    ip_sa->parent = node;
	    ip_sa->other_sa.type = proto_id;
	    ip_sa->flags |= AND_NEXT_PROPOSAL;
	    return(ip_sa->key_seq);
	}
    }
    return(0);
}

sa_list *get_sa_by_protocol (SA *sa, unsigned char proto_id, 
			unsigned long message_id)
{
    conn_entry *node;
    sa_list *oakley_sa;

    for(node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == message_id)){
	    if(node->sa_s == NULL)
		return(NULL);
	    for(oakley_sa = node->sa_s; oakley_sa != NULL; 
		oakley_sa = oakley_sa->next){
		if(oakley_sa->other_sa.type == proto_id)
		    return(oakley_sa);
	    }
	}
    }
    return(NULL);
}

sa_list *get_sa_list (SA *sa, unsigned long mess_id)
{
    conn_entry *node;

    for(node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == mess_id))
	    return(node->sa_s);
    }
    return(NULL);
}

conn_entry *get_centry_by_mess_id (unsigned long mess_id)
{
    conn_entry *node;

    for(node = conn_list; node != NULL; node = node->next){
	if(node->message_id == mess_id)
	    return(node);
    }
    return(NULL);
}

int still_need_spi (SA *sa, unsigned long mess_id)
{
    conn_entry *node;
    sa_list *spiable_sa;

    for(node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == mess_id)){
	    if(node->sa_s == NULL)
		return(0);
	    for(spiable_sa = node->sa_s; spiable_sa != NULL; 
		spiable_sa = spiable_sa->next){
		if(spiable_sa->spi == 0)
		    return(1);
	    }
	}
    }
    return(0);
}

void set_other_spi_by_mess_id(SA *sa, unsigned long mess_id, 
			unsigned int protocol, unsigned long other_spi)
{
    sa_list *node;

    node = get_sa_by_protocol(sa, protocol, mess_id);
    if(node != NULL){
	node->other_spi = other_spi;
    }
    return;
}

int set_nonce_by_mess_id (unsigned long message_id, unsigned char *nonce, 
			int nonce_len, int init_or_resp)
{
    conn_entry *node;
	/*
	 * check every conn_entry's sa_list 'till a match is found. 
	 */
    for(node = conn_list; node != NULL; node = node->next){
	if(node->message_id == message_id){
	    if(init_or_resp == INITIATOR){
		node->nonce_R=(unsigned char *)malloc(nonce_len);
		if(node->nonce_R == NULL){
		    LOG((CRIT,"Out Of Memory"));
		    return(1);
		}
		node->nonce_R_len = nonce_len;
		bcopy((char *)nonce, node->nonce_R, nonce_len);
		return(0);
	    } else {
		node->nonce_I=(unsigned char *)malloc(nonce_len);
		if(node->nonce_I == NULL){
		    LOG((CRIT,"Out Of Memory"));
		    return(1);
		}
		node->nonce_I_len = nonce_len;
		bcopy((char *)nonce, node->nonce_I, nonce_len);
		return(0);
	    }
	}
    }
    return(1);
}

int set_spi_by_seq (int seq, unsigned long spi)
{
    conn_entry *node;
    sa_list *the_sa;
	/*
	 * check every conn_entry's sa_list 'till a match is found. set spi
	 */
    for(node = conn_list; node != NULL; node = node->next){
	for(the_sa = node->sa_s; the_sa; the_sa = the_sa->next){
	    if(the_sa->key_seq == seq){
		the_sa->spi = spi;
		return(0);
	    }
	}
    }
    return(1);
}

static void sa_free(sa_list *rmme)
{
    if(rmme->inbound_crypt_keylen)
	free(rmme->inbound_crypt_key);
    if(rmme->outbound_crypt_keylen)
	free(rmme->outbound_crypt_key);
    if((rmme->other_sa.type == PROTO_IPSEC_ESP) &&
       (rmme->other_sa.esp.crypt_ivlen))
	free(rmme->other_sa.esp.crypt_iv);
    return;
}

static void destroy_sa_list (SA *sa, sa_list *node, int flush)
{
	/*
	 * recursively destroy the spi list. 1st tell the kernel to delete
	 * its corresponding security association, then remove from our list
	 */
    if(node->next != NULL){
	destroy_sa_list(sa, node->next, flush);
    }
    if(flush)
	kernel_delete_spi(sa, node->spi, node->other_sa.type);
    sa_free(node);
    free(node);
} 

static void isadb_free_conn_entry (SA *sa, conn_entry *conn_node, int flush)
{
    if(conn_node->sa_s != NULL){
	destroy_sa_list(sa, conn_node->sa_s, flush);
    }
    if(conn_node->nonce_I_len)
	free(conn_node->nonce_I);
    if(conn_node->nonce_R_len)
	free(conn_node->nonce_R);
    if(conn_node->phase2_iv_len)
	free(conn_node->phase2_iv);
    if(conn_node->dh.dh_len){
	if(conn_node->dh.DH_priv_val)
	    free(conn_node->dh.DH_priv_val);
	if(conn_node->dh.DH_pub_val)
	    free(conn_node->dh.DH_pub_val);
	if(conn_node->dh.his_DH_pub_val)
	    free(conn_node->dh.his_DH_pub_val);
	if(conn_node->dh.g_to_xy)
	    free(conn_node->dh.g_to_xy);
    }
}

void isadb_delete_conn_entry (SA *sa, unsigned long message_id, int flush)
{
    conn_entry *prev, *node;

    if((!bcmp((char *)sa->my_cookie, conn_list->my_cookie, COOKIE_LEN)) &&
       (!bcmp((char *)sa->his_cookie, conn_list->his_cookie, COOKIE_LEN)) &&
       (conn_list->message_id == message_id)){
	node = conn_list;
	conn_list = conn_list->next;
	isadb_free_conn_entry(sa, node, flush);
	free(node);
	return;
    }
    for(prev = NULL, node = conn_list; node != NULL; node = node->next){
	if((!bcmp((char *)sa->my_cookie, node->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, node->his_cookie, COOKIE_LEN)) &&
	   (node->message_id == message_id)){
	    prev->next = node->next;
	    isadb_free_conn_entry(sa, node, flush);
	    free(node);
	    return;
	}
	prev = node;
    }
    return;
}

int isadb_clean_up (SA *sa, int flush)
{
    conn_entry *conn_node, *prev;

    if (conn_list) {
	if((!bcmp((char *)sa->my_cookie, conn_list->my_cookie, COOKIE_LEN)) &&
	   (!bcmp((char *)sa->his_cookie, conn_list->his_cookie, COOKIE_LEN))){
	    conn_node = conn_list;
	    conn_list = conn_list->next;
	    isadb_free_conn_entry(sa, conn_node, flush);
	    free(conn_node);
	    return(1);
	}
	for(prev = conn_list, conn_node = conn_list->next; conn_node; 
			conn_node = conn_node->next, prev = prev->next){
	    if((!bcmp((char *)sa->my_cookie, conn_node->my_cookie, 
							COOKIE_LEN)) &&
	       (!bcmp((char *)sa->his_cookie, conn_node->his_cookie, 
							COOKIE_LEN))){
		prev->next = conn_node->next;
		isadb_free_conn_entry(sa, conn_node, flush);
		free(conn_node);
		return(1);
	    }
	}
    }
    return(0);
}
 
void isadb_delete_all (SA *sa)
{
    while(isadb_clean_up(sa, 1));
}
 
void isadb_free_all (SA *sa)
{
    while(isadb_clean_up(sa, 0));
}

void reap_db (void)
{
    SA *entry, *victim;

    entry = dblist;
    while(entry){
	victim = entry;
	entry = entry->next;
	if(victim->condition == SA_COND_DEAD){
	    isadb_clean_up(victim, 1);
	    isadb_delete_entry(victim);
	}
    }
}
	/*
	 * for the initiator. He's kicked by the kernel. Check whether
	 * we're already working on this kernel request.
	 */
int isadb_is_outstanding_kernel_req (struct sockaddr_in *dst, 
			unsigned short type)
{
    unsigned char cookie[COOKIE_LEN];
    SA *node;

    if(dblist == NULL)
	return(0);
    gen_cookie(dst, cookie);
    for(node = dblist; node != NULL; node = node->next){
	/*
	 * check whether we're communicating with this guy already and
	 * whether the SA is not flagged for deletion, if so, are we already 
	 * going to negotiate for this type? 
	 */
	if((bcmp((char *)node->my_cookie, cookie, COOKIE_LEN) == 0) &&
	   (node->condition != SA_COND_DEAD)){
	    switch(type){
		case PROTO_IPSEC_AH:
		    if(node->outstanding_sa & PROTO_IPSEC_AH_MASK)
			return(-1);
		    node->outstanding_sa |= PROTO_IPSEC_AH_MASK;
		    break;
		case PROTO_IPSEC_ESP:
		    if(node->outstanding_sa & PROTO_IPSEC_ESP_MASK)
			return(-1);
		    node->outstanding_sa |= PROTO_IPSEC_ESP_MASK;
		    break;
		default:
		    return(-1);
	    }
	    return(1);
	}
    }
    return(0);
}
	/*
	 * responder is kicked by initiator. Check whether we already have
	 * a channel (in _any_ state) to the initiator.
	 */
int isadb_is_outstanding_init_req (isakmp_hdr *hdr, 
				struct sockaddr_in *src)
{
    unsigned char cookie[COOKIE_LEN];
    SA *node;

    if(dblist == NULL)
	return(0);
    gen_cookie(src, cookie);
    for(node = dblist; node != NULL; node = node->next){
	if((bcmp((char *)node->my_cookie, cookie, COOKIE_LEN) == 0) &&
	   (node->condition != SA_COND_DEAD)){
	    switch(node->state){
		/*
		 * if the initiator went down and forget that we had this
		 * existing ISAKP SA we should delete it and start again
		 */
		case OAK_QM_AUTH_AWAIT:
		case OAK_QM_IDLE:
		    isadb_delete_entry(node);
		    return(0);
		default:
		    return(1);
	    }
	}
    }
    return(0);
}

void delete_sa_offer (sa_offer *node)
{
    if(node->next != NULL){
	delete_sa_offer(node->next);
	free(node->next);
    }
    if(node->natts)
	free(node->atts);
}

void isadb_trash_all (void)
{
    SA *d, *dv;

    /*
     * Trash all of the ISAKMP associations.
     */
    d = dblist;
    while (d) {
	dv = d;
	d = d->next;
	isadb_clean_up(dv, 1);
	isadb_delete_entry(dv);
    }
}

