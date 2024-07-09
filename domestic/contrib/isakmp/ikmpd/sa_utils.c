/* 
 * Id: sa_utils.c,v 1.3 1997/07/24 19:42:11 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/sa_utils.c,v
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
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"
#include "toolkit.h"
#include "hmac.h"
#include "sa_atts.h"

/*
 * sa_utils.c -- utilities for construction and parsing of attribute
 *	proposals.
 */

static unsigned char *buf = 0;
static unsigned char *pl = 0;

#define ADVANCE_PTR(x) { buf += (x); if(buf > pl) return(0); }

#ifdef DEBUG
void print_vpi(char *str, unsigned char *vpi, int vpi_len)
{
    int i, j;

    printf("%s\n  ",str);
    for(j=1, i=0; i<vpi_len; i++){
	printf("0x%x ",vpi[i]);
	if(j == 16){
	    j = 0;
	    printf("\n  ");
	}
	j++;
    }
    printf("\n");
}

void print_hdr(isakmp_hdr *hdr)
{
    printf("Header for version %d.%d is:\n", hdr->majver, hdr->minver);
    print_vpi("I-COOKIE", hdr->init_cookie, COOKIE_LEN);
    print_vpi("R-COOKIE", hdr->resp_cookie, COOKIE_LEN);
    printf("payload is %ld bytes in length\n", hdr->len);
    printf("the exchange is ");
    switch(hdr->exch){
        case OAK_MM:
            printf("Oakley Main Mode\n");
            break;
	case OAK_AG:
	    printf("Oakley Aggressive Mode\n");
	    break;
        case OAK_QM:
            printf("Oakley Quick Mode\n");
            break;
        case OAK_NG:
            printf("Oakley New Group Mode\n");
            break;
	case ISAKMP_INFO:
	    printf("ISAKMP Informational Exchange\n");
	    break;
        default:
            printf("unknown! (%d)\n",hdr->exch);
    }
    printf("the flags are %x\n", hdr->flags);
    printf("the next payload is ");
    switch(hdr->next_payload){
	case ISA_SA:
	    printf("ISA_SA\n");
	    break;
	case ISA_KE:
	    printf("ISA_KE\n");
	    break;
	case ISA_ID:
	    printf("ISA_ID\n");
	    break;
	case ISA_CERT:
	    printf("ISA_CERT\n");
	    break;
	case ISA_HASH:
	    printf("ISA_HASH\n");
	    break;
	case ISA_SIG:
	    printf("ISA_SIG\n");
	    break;
	case ISA_NONCE:
	    printf("ISA_NONCE\n");
	    break;
	case ISA_NOTIFY:
	    printf("ISA_NOTIFY\n");
	    break;
	case ISA_DELETE:
	    printf("ISA_DELETE\n");
	    break;
	default:
	    printf("unknown\n");
    }
}
#endif

/*
 * We don't have prior knowledge of the size of an attribute payload
 * (we could but that would make extensibility difficult).
 * Therefore these routines which add basic and vpi attributes will
 * size the payload up in chunks of CHUNK and return the length of
 * the malloc'd space (payload_len) and the length of real data in
 * that space (len).
 */
void basic_att(int class, int type, unsigned char **p, int *len,
				int *payload_len)
{
  sa_attribute sa_att;

    if((*len + sizeof(sa_attribute)) > *payload_len){
	unsigned char *tmp;
	tmp = (unsigned char *) realloc (*p, (*payload_len + CHUNK));
	*p = tmp;
	*payload_len += CHUNK;
    }
    bzero((char *)&sa_att, sizeof(sa_attribute));
    sa_att.type = 1;			/* basic */
    sa_att.att_class = class;
    sa_att.att_type.basic_value = type;
    hton_att(&sa_att);
    bcopy((char *)&sa_att, (char *)(*p + *len), sizeof(sa_attribute));
    *len += sizeof(sa_attribute);
}

void vpi_att(int class, unsigned char *type, int sz, unsigned char **p,
			int *len, int *payload_len)
{
    sa_attribute sa_att;
    int nbytes;

	/* make sure the atts are in 4-octet units */
    nbytes = sz;
    while(nbytes%4)
	nbytes++;

    if((*len + sizeof(sa_attribute) + nbytes) > *payload_len){
	unsigned char *tmp;
	tmp = (unsigned char *) realloc (*p, (*payload_len + CHUNK + nbytes));
	*p = tmp;
	*payload_len += (CHUNK + nbytes);
    }
    bzero((char *)(*p + *len), (nbytes + sizeof(sa_attribute)));
    bzero((char *)&sa_att, sizeof(sa_attribute));
    sa_att.type = 0;			/* VPI */
    sa_att.att_class = class;
    sa_att.att_type.vpi_length = nbytes;
	/* 
	 * to encode a VPI, "nbytes-sz" bytes of zeros are prepended to the 
	 * actual bits-- actually the area is bzero'd and the bcopy'ing starts 
	 * "nbytes-sz" into it.
	 */
    hton_att(&sa_att);
    bcopy((char *)&sa_att, (char *)(*p + *len), sizeof(sa_attribute));
    *len += sizeof(sa_attribute);
    bcopy((char *)type, (char *)(*p + *len + (nbytes - sz)), nbytes);
    *len += nbytes;
}

int set_oakley_atts(SA *sa, unsigned char **payload, int *payload_len)
{
    unsigned char *p, *g;
    unsigned short plen, glen; 
    int len = 0;

    bzero((char *)*payload, (*payload_len * sizeof(unsigned char)));
    if(sa->encr_alg){
	switch(sa->encr_alg){
	    case ET_DES_CBC:
		basic_att(OAK_ENCR_ALG, sa->encr_alg, payload, &len, 
				payload_len);
		break;
	    default:
		LOG((WARN,"Whoa! Unknown Cipher Alg!!"));
	}
    }
    if(sa->hash_alg){
	basic_att(OAK_HASH_ALG, sa->hash_alg, payload, &len, payload_len);
    }
    if(sa->dh.group_desc){
	basic_att(OAK_GROUP_DESC, sa->dh.group_desc,payload,&len,payload_len);
    }
    if(sa->auth_alg){
	basic_att(OAK_AUTH_METHOD, sa->auth_alg, payload, &len, payload_len);
    }
    if(sa->dh.group.type){
	basic_att(OAK_GROUP_TYPE, sa->dh.group.type,payload,&len,payload_len);
	/*
	 * if we're not describing the group with a type, we must send
	 * p and g
	 */
	if(sa->dh.group_desc == 0){
	    switch(sa->dh.group.type){
	    case GP_MODP:
		if(sa->init_or_resp == INITIATOR){
		    if(dh_modp_set(0, &p, &plen, &g, &glen)){
			return(-1);
		    }
		    vpi_att(OAK_PRIME_P, p, plen, payload, &len, payload_len);
		    vpi_att(OAK_GENERATOR_G1, g, glen, payload, &len, 
				payload_len);
		    free(p);
		    free(g);
		} else {
		    vpi_att(OAK_PRIME_P, sa->dh.group.modp.p, 
				sa->dh.group.modp.prime_len,
				payload, &len, payload_len);
		    vpi_att(OAK_GENERATOR_G1, sa->dh.group.modp.g, 
				sa->dh.group.modp.gen_len, payload, &len, 
				payload_len);
		}
		break;
	    default:
		bzero((char *)*payload, (*payload_len * sizeof(unsigned char)));
		return(-1);
	    }
	}
    }
    *payload_len = len;
    return(0);
}

static int set_ah_atts(struct ah_proposal *ah, unsigned char **payload, 
		int *payload_len, struct dh_stuff *dh)
{
    int len = 0;
    unsigned short blah;

    bzero((char *)*payload, (*payload_len * sizeof(unsigned char)));

    if(ah->ah_life_type && ah->ah_life_len){
	basic_att(IPSEC_SA_LIFE_TYPE, ah->ah_life_type, payload, &len, 
		payload_len);
	if(ah->ah_life_len <= 2){
	    blah = (unsigned short)*ah->ah_life_dur;
	    basic_att(IPSEC_SA_LIFE_DURATION, blah, payload, &len, payload_len);
	} else {
	    vpi_att(IPSEC_SA_LIFE_DURATION, ah->ah_life_dur, 
			ah->ah_life_len, payload, &len, payload_len);
	}
    }
    if(ah->tunnel){
	basic_att(IPSEC_ENCAPSULATION_MODE, ah->tunnel, payload, &len, 
		payload_len);
    }
    if(dh->group_desc){
	basic_att(IPSEC_GROUP_DESC, dh->group_desc, payload, &len,
		payload_len);
    }
    *payload_len = len;
    return(0);
}

static int set_esp_atts(struct esp_proposal *esp, unsigned char **payload, 
		int *payload_len, struct dh_stuff *dh)
{
    int len = 0;
    unsigned short blah;

    bzero((char *)*payload, (*payload_len * sizeof(unsigned char)));

    if(esp->esp_life_type && esp->esp_life_len){
	basic_att(IPSEC_SA_LIFE_TYPE, esp->esp_life_type, payload, &len, 
		payload_len);
	if(esp->esp_life_len <= 2){
	    blah = (unsigned short)*esp->esp_life_dur;
	    basic_att(IPSEC_SA_LIFE_DURATION, blah, payload, &len, payload_len);
	} else {
	    vpi_att(IPSEC_SA_LIFE_DURATION, esp->esp_life_dur, 
			esp->esp_life_len, payload, &len, payload_len);
	}
    }
    if(esp->tunnel){
	basic_att(IPSEC_ENCAPSULATION_MODE, esp->tunnel, payload, &len, 
		payload_len);
    }
    if(esp->hmac_alg){
	basic_att(IPSEC_HMAC_ALG, esp->hmac_alg, payload, &len, payload_len);
    }
    if(dh->group_desc){
	basic_att(IPSEC_GROUP_DESC, dh->group_desc, payload, &len,
		payload_len);
    }
    *payload_len = len;
    return(0);
}

int set_transform(SA *sa, int fixpos, int *pos, unsigned char nextp,
		  unsigned char transnum, unsigned char transform, 
		  unsigned char *atts, int attslen)
{
    trans_payload trans;
    int nbytes;

    nbytes = sizeof(trans_payload) + attslen;
    bzero((char *)&trans, sizeof(trans_payload));
    trans.next_payload = nextp;
    trans.payload_len = nbytes;
    trans.trans_num = transnum;
    trans.trans_id = transform;
    hton_transform(&trans);

    expand_packet(sa, *pos, nbytes);
    *pos += nbytes;
    bcopy((char *)&trans, (char *)(sa->packet + fixpos), sizeof(trans_payload));
    fixpos += sizeof(trans_payload);
    if(attslen)
	bcopy((char *)atts, (char *)(sa->packet + fixpos), attslen);
    return(nbytes);
}

int set_proposal(SA *sa, int fixpos, int *pos, unsigned char protocol, 
		 sa_list *ipsec_sa_list, unsigned short propnum)
{
    proposal_payload sa_prop;
    unsigned char *atts, nextp;
    int i, nbytes, ntrans, attslen, offset = 0;
    sa_list *node = NULL;
    unsigned long spi;

    if(protocol == PROTO_ISAKMP){
	ntrans = 1;
	nextp = 0;
    } else {
	if(ipsec_sa_list == NULL)
	    return(0);
	/*
	 * go through the sa specs and determine how many transforms
	 * apply to this proposal. Find out what the next payload will be.
	 */
	for(ntrans = 0, node = ipsec_sa_list; node != NULL; 
		node = node->next){
	    ntrans++;
	    if(node->flags != OR_NEXT_TRANSFORM){
		node = node->next;
		break;
	    }
	}
	nextp = (node == NULL) ? 0 : ISA_PROP;
	node = ipsec_sa_list;
    }
    bzero((char *)&sa_prop, sizeof(proposal_payload));
	/*
	 * set an empty place for the proposal since we don't know its
	 * characteristics-- length furinstance-- yet
	 */
    expand_packet(sa, *pos, sizeof(proposal_payload));
    *pos += sizeof(proposal_payload);
    nbytes = sizeof(proposal_payload);

    if((ipsec_sa_list != NULL) && (protocol != PROTO_ISAKMP)){
	expand_packet(sa, *pos, sizeof(unsigned long));
	*pos += sizeof(unsigned long);
	nbytes += sizeof(unsigned long);
    }
    for(i=1; i<(ntrans+1); i++){
	atts = (unsigned char *) malloc (CHUNK * sizeof(unsigned char));
	attslen = CHUNK;
	/*
	 * go through adding transforms keeping track of length
	 */
	if(protocol == PROTO_ISAKMP){
	    set_oakley_atts(sa, &atts, &attslen);
	    offset = set_transform(sa, (fixpos + nbytes), pos, 0, i, 
					PROTO_ISAKMP, atts, attslen);
	} else {
	    switch(node->other_sa.type){
		case PROTO_IPSEC_AH:
		    set_ah_atts(&node->other_sa.ah, &atts, &attslen,
				&((conn_entry *)(node->parent))->dh);
		    offset = set_transform(sa, (fixpos + nbytes), pos,
				   (i == ntrans ? 0 : ISA_TRANS), i, 
				   node->other_sa.ah.transform, atts, attslen);
		    break;
		case PROTO_IPSEC_ESP:
		    set_esp_atts(&node->other_sa.esp, &atts, &attslen,
				&((conn_entry *)(node->parent))->dh);
		    offset = set_transform(sa, (fixpos + nbytes), pos,
				   (i == ntrans ? 0 : ISA_TRANS), i, 
				   node->other_sa.esp.transform, atts, attslen);
		    break;
	    }
	    node = node->next;
	}
	nbytes += offset;
	free(atts);
    }
	/*
	 * now copy the proposal 
	 */
    sa_prop.next_payload = nextp;
    sa_prop.payload_len = nbytes;
    sa_prop.prop_num = propnum;
    sa_prop.protocol_id = protocol;
    sa_prop.num_transforms = ntrans;
	/*
	 * if ipsec_sa_list != NULL then we're doing IPsec and ALL NODES IN
	 * THE LIST MUST HAVE THE SAME SPI IF THEY'RE OR_NEXT_TRANSFORM
	 * (by definition!) so we can just use the spi from the first. 
	 */
    if((ipsec_sa_list != NULL) && (protocol != PROTO_ISAKMP)){
	spi = htonl(ipsec_sa_list->spi);
	sa_prop.spi_size = sizeof(ulong);
    }
    hton_proposal(&sa_prop);
    bcopy((char *)&sa_prop, (sa->packet + fixpos), sizeof(proposal_payload));
    if((ipsec_sa_list != NULL) && (protocol != PROTO_ISAKMP)){
	bcopy((char *)&spi, (sa->packet + fixpos + sizeof(proposal_payload)),
		sizeof(unsigned long));
    }
    return(nbytes);
}

int set_ipsec_proposals(SA *sa, int fixpos, int *pos, sa_list *ipsec_sa_list)
{
    int totbytes = 0, nbytes = 0, propnum = 1;
    sa_list *node;

    node = ipsec_sa_list;
    do {
	nbytes = set_proposal(sa, (fixpos + totbytes), pos, node->other_sa.type,
				node, propnum);
	totbytes += nbytes;
	/*
	 * next transforms will get taken care of in set_proposal so skip over
	 * them, if that's all there is then bye, otherwise add another
	 * proposal, OR_NEXT increments the proposal number, AND_NEXT will
	 * use the same one.
	 */
	while((node != NULL) && (node->flags == OR_NEXT_TRANSFORM))
	    node = node->next;
	if(node == NULL)
	    break;
	if(node->flags == OR_NEXT_PROPOSAL){
	    propnum++;
	}
	node = node->next;
    } while(node != NULL);
    return(totbytes);
}

int find_att(unsigned int att_class, unsigned int att_type)
{
    sa_attribute saa;
    while(1){
	bcopy((char *)buf, (char *)&saa, sizeof(sa_attribute));
	ntoh_att(&saa);
	if(saa.att_class == att_class){
	/*
	 * a value of a VPI cannot be specified in a protection suite. 
	 * Therefore if att_type is non-zero the matching attribute be basic. 
	 */
	    if(att_type){ 
		/* 
		 * if we've specified a value and it's a VPI or the values 
		 * don't match 
		 */
		if(!saa.type || (att_type != saa.att_type.basic_value)){
		    return(0);
		}
	    }
 	    return(1);
	}
	ADVANCE_PTR(sizeof(sa_attribute));
	if(!saa.type){
	    ADVANCE_PTR(saa.att_type.vpi_length);
	}
    }
}

int check_trans_atts(unsigned char *ptr, unsigned short len, 
			unsigned char trans, unsigned char atts[][2])
{
    int i = 0, j, natts, acceptable = 0;
	/*
	 * loop through specified attributes for this transform-- see sa_atts.h
	 */
    while((atts[i][0] != 0) && !acceptable){
	natts = atts[i][1];
	if(atts[i][0] == trans){
	    if(natts == 0)		/* if no atts specified then any atts */
		return(1);		/* offered will be ok */
	    if(len == 0)		/* if atts are specified but are not */
		return(0);		/* offered then it's not ok */
	    for(j = i + 1; j < (i + natts); j++){
		buf = ptr;
		pl = ptr + len;
		acceptable = find_att(atts[j][0], atts[j][1]);
		if(!acceptable)
		    break;
	    }
	}
	i += (natts + 1);
    }
    return(acceptable);
}

#ifdef DEBUG
static void print_ipsec_transform(unsigned char *ptr, unsigned short len)
{
    unsigned char *fin;
    sa_attribute saa;

    fin = ptr + len;
    do {
	bcopy((char *)ptr, (char *)&saa, sizeof(sa_attribute));
	ntoh_att(&saa);
	switch(saa.att_class){
	    case IPSEC_SA_LIFE_TYPE:
		switch(saa.att_type.basic_value){
		    case IPSEC_LIFE_SEC:
			printf("	SA life type in seconds\n");
			break;
		    case IPSEC_LIFE_KB:
			printf("	SA life type in kilobytes\n");
			break;
		    default:
			printf("	SA life type unknown. %d\n",
				saa.att_type.basic_value);
		}
		break;
	    case IPSEC_SA_LIFE_DURATION:
		if(saa.type == 0){
		    ptr += sizeof(sa_attribute);
		    print_vpi("		SA life duration", ptr,
				saa.att_type.vpi_length);
		} else {
		    printf("	SA life duration %d\n", 
				saa.att_type.basic_value);
		}
		break;
	    case IPSEC_GROUP_DESC:
		printf("	group description for PFS is %d\n",
				saa.att_type.basic_value);
		break;
	    case IPSEC_ENCAPSULATION_MODE:
		printf("	tunnel mode is %d\n", saa.att_type.basic_value);
		break;
	    case IPSEC_HMAC_ALG:
		printf("	HMAC algorithm is %d\n",
				saa.att_type.basic_value);
		break;
	    case IPSEC_KEY_LENGTH:
		printf("	key length is %d\n", saa.att_type.basic_value);
		break;
	    default:
		printf("        unknown IPSec attribute %d\n", saa.att_class);
		if(!saa.type)
		    ptr += sizeof(sa_attribute);
	}
	if(!saa.type){
	    ptr += saa.att_type.vpi_length;
	} else {
	    ptr += sizeof(sa_attribute);
	}
    } while(ptr < fin);
}
#endif

/*
 * check a single proposal for acceptability. Return the best transform
 * for this proposal in the atts buffer.
 */
int check_prop(unsigned char *ptr, unsigned char protocol, 
		unsigned short ntrans, unsigned char *transform,
		unsigned char **atts, unsigned short *natts)
{
    trans_payload trans;
    int acceptable = 0;

    do {
	bcopy((char *)ptr, (char *)&trans, sizeof(trans_payload));
	ntoh_transform(&trans);
	ptr += sizeof(trans_payload);
	trans.payload_len -= sizeof(trans_payload);
	/*
	 * check each transform of this proposal against the appropriate
	 * protection suite for this protocol. 
	 */
	switch(protocol){
	    case PROTO_ISAKMP:
		acceptable = check_trans_atts(ptr, trans.payload_len, 
					trans.trans_id, oakley_pref);
		break;
	    case PROTO_IPSEC_AH:
#ifdef DEBUG
		print_ipsec_transform(ptr, trans.payload_len);
#endif
		acceptable = check_trans_atts(ptr, trans.payload_len, 
					trans.trans_id, ah_pref);
		break;
	    case PROTO_IPSEC_ESP:
#ifdef DEBUG
		print_ipsec_transform(ptr, trans.payload_len);
#endif
		acceptable = check_trans_atts(ptr, trans.payload_len, 
					trans.trans_id, esp_pref);
		break;
	}
	/*
	 * if this transform is acceptable then ignore the rest. Return this
	 * one in the atts buffer.
	 */
	if(acceptable){
	    *transform = trans.trans_id;
	    *natts = trans.payload_len;
	    if(*natts){		/* could be 0 if no atts for this transform */
		*atts = (unsigned char *) malloc (trans.payload_len);
		if(*atts == NULL){
		    LOG((ERR, "Out Of Memory"));
		    return(0);
		}
		bcopy((char *)ptr, *atts, *natts);
	    }
	}
	ptr += trans.payload_len;
    } while(!acceptable && trans.next_payload == ISA_TRANS);
    return(acceptable);
}

/*
 * recursively check proposals for acceptability. Acceptable proposals
 * are returned as components of the sa_offer linked list. This routine
 * is only called for non-ISAKMP SA offers.
 */
int check_proposal(unsigned char *ptr, sa_offer *offer, int mess_id)
{
    proposal_payload prop, nextprop;
    int acceptable;
    unsigned char *buff;
    unsigned char trans;
    unsigned short buffsize;
    ulong spi;
	/*
	 * check the acceptability of this proposal, if there's more
	 * proposals, note it.
	 */
    bcopy((char *)ptr, (char *)&prop, sizeof(proposal_payload));
    ntoh_proposal(&prop);
    if(prop.spi_size){
	if(prop.spi_size != sizeof(ulong)){
	    LOG((ERR, "Invalid SPI size of %d", prop.spi_size));
	    return(0);
	}
	bcopy((char *)(ptr + sizeof(proposal_payload)), (char *)&spi,
		sizeof(ulong));
	spi = ntohl(spi);
	acceptable=check_prop((ptr + sizeof(proposal_payload) + sizeof(ulong)), 
			prop.protocol_id, prop.num_transforms, &trans, 
			&buff, &buffsize);
    } else {
	spi = 0;
	acceptable = check_prop((ptr + sizeof(proposal_payload)), 
			prop.protocol_id, prop.num_transforms, &trans, 
			&buff, &buffsize);
    }
    if(prop.next_payload == ISA_PROP)
	bcopy((char *)(ptr + prop.payload_len), (char *)&nextprop,
				sizeof(proposal_payload));
    if(acceptable){
	/*
	 * the proposal is acceptable so save the salient info in the next
	 * sa_offer. Yes, yes, then the 1st in the list is unused.
	 */
	offer->next = (sa_offer *) malloc (sizeof(sa_offer));
	if(offer->next == NULL){
	    LOG((ERR, "Out Of Memory"));
	    return(1);
	}
	bzero((char *)offer->next, sizeof(sa_offer));
	offer->next->natts = buffsize;
	if(buffsize){	/* could be 0 if no atts for this transform */
	    offer->next->atts = (unsigned char *) malloc (buffsize);
	    if(offer->next->atts == NULL){
		LOG((ERR, "Out Of Memory"));
		return(1);
	    }
	    bcopy((char *)buff, (char *)offer->next->atts, buffsize);
	    free(buff);
	}
	offer->next->protocol = prop.protocol_id;
	offer->next->spi = spi;
	offer->next->transform = trans;
	offer->next->next = NULL;
	/*
	 * either the last payload or it's a logical OR proposal and since
	 * we've already accepted this one then BCE.
	 * Otherwise it's a logical OR proposal and our success depends on
	 * the acceptability of the next proposal.
	 */
	if((prop.next_payload == 0) || 
	   ((prop.next_payload == ISA_PROP) && 
	    (nextprop.prop_num != prop.prop_num)))
	    return(acceptable);
    } else {
	/*
	 * if this is the last payload of part of a logical AND then failure.
	 * Otherwise we still have hope for the next proposal...
	 */
	if((prop.next_payload == 0) ||
	   ((prop.next_payload == ISA_PROP) &&
	    (nextprop.prop_num == prop.prop_num)))
	    return(0);
    }
    acceptable = check_proposal((ptr + prop.payload_len), offer->next, mess_id);
    return(acceptable);
}

int oakley_atts_to_sa(unsigned char *ptr, int len, SA *sa)
{
    sa_attribute saa;
    unsigned char *fin;

    fin = ptr + len;
    do {
 	bcopy((char *)ptr, (char *)&saa, sizeof(sa_attribute));
 	ntoh_att(&saa);
 	switch(saa.att_class){
	    case OAK_ENCR_ALG:
		sa->encr_alg = saa.att_type.basic_value;
		switch(sa->encr_alg){
		    case ET_DES_CBC:
			sa->crypt_key = (unsigned char *) malloc (DES_KEYLEN);
			if(sa->crypt_key == NULL){
			    LOG((ERR, "Out Of Memory"));
			    return(1);
			}
			sa->crypt_iv = (unsigned char *) malloc (DES_KEYLEN);
			if(sa->crypt_iv == NULL){
			    LOG((ERR, "Out of Memory. Can't create IV!"));
			    free(sa->crypt_key);
			    return(1);
			}
			sa->crypt_iv_len = DES_KEYLEN;
			break;
		    default:
			LOG((WARN,"Unknown crypt alg accepted!"));
		}
		break;
	    case OAK_AUTH_METHOD:
		sa->auth_alg = saa.att_type.basic_value;
		break;
      	    case OAK_HASH_ALG:
		sa->hash_alg = saa.att_type.basic_value;
		switch(saa.att_type.basic_value){
		    case HASH_MD5:
			sa->InitHash = md5Init;
			sa->UpdHash = md5Update;
			sa->FinHash = md5Final;
			sa->Inithmac = hmac_md5Init;
			sa->Updhmac = hmac_md5Update;
			sa->Finhmac = hmac_md5Final;
			break;
		    case HASH_SHA:
			sa->InitHash = shaInit;
			sa->UpdHash = shaUpdate;
			sa->FinHash = shaFinal;
			sa->Inithmac = hmac_shaInit;
			sa->Updhmac = hmac_shaUpdate;
			sa->Finhmac = hmac_shaFinal;
			break;
		    default:
			LOG((WARN,"Unknown Hash algorithm!"));
		}
		break;
	    case OAK_GROUP_TYPE:
		sa->dh.group.type = saa.att_type.basic_value;
		break;
	    case OAK_GROUP_DESC:
		sa->dh.group_desc = saa.att_type.basic_value;
		if(dh_group_find(saa.att_type.basic_value, &sa->dh)){
		    LOG((ERR, "Unable to find Oakley Group %d", 
				saa.att_type.basic_value));
		    return(1);
		}
		break;
	    case OAK_PRIME_P:
		if(sa->dh.group.modp.prime_len)
		    break;
		sa->dh.group.modp.prime_len = saa.att_type.vpi_length;
		ptr += sizeof(sa_attribute);
		sa->dh.group.modp.p = 
			(unsigned char *)malloc(sa->dh.group.modp.prime_len *
					sizeof(unsigned char));
		if(sa->dh.group.modp.p == NULL){
		    LOG((ERR, "Out Of Memory"));
		    return(1);
		}
		bcopy((char *)ptr, (char *)sa->dh.group.modp.p, 
					sa->dh.group.modp.prime_len);
		break;
	    case OAK_GENERATOR_G1:
		if(sa->dh.group.modp.gen_len)
		    break;
		sa->dh.group.modp.gen_len = saa.att_type.vpi_length;
		ptr += sizeof(sa_attribute);
		sa->dh.group.modp.g = (unsigned char *)
		    malloc(sa->dh.group.modp.gen_len * sizeof(unsigned char));
		if(sa->dh.group.modp.g == NULL){
		    LOG((ERR, "0ut Of Memory"));
		    return(1);
		}
		bcopy((char *)ptr, (char *)sa->dh.group.modp.g, 
					sa->dh.group.modp.gen_len);
		break;
	    case OAK_CURVE_A:
	    case OAK_CURVE_B:
		ptr += sizeof(sa_attribute);
		break;
		/* 
		 * on the todo list
		 */
	    case OAK_LIFE_TYPE:
		break;
	    case OAK_LIFE_DUR:
		if(saa.type == 0)
		    ptr += sizeof(sa_attribute);
		break;
	    default:
		LOG((WARN,"I don't know what attribute %d is!",saa.att_class));
	}
	if(!saa.type){
	    ptr += saa.att_type.vpi_length;
	} else {
	    ptr += sizeof(sa_attribute);
    	}
    } while(ptr < fin);
    return(0);
}

static void ah_atts_to_sa(unsigned char *ptr, int len, struct ah_proposal *ah,
			unsigned short *dh_group_desc)
{
    sa_attribute saa;
    unsigned char *fin;

    if(len == 0)
	return;
    fin = ptr + len;
    do {
	bcopy((char *)ptr, (char *)&saa, sizeof(sa_attribute));
	ntoh_att(&saa);
	switch(saa.att_class){
	    case IPSEC_SA_LIFE_TYPE:
		ah->ah_life_type = saa.att_type.basic_value;
		break;
	    case IPSEC_SA_LIFE_DURATION:
		if(saa.type == 0){
		    ah->ah_life_len = saa.att_type.vpi_length;
		    ptr += sizeof(sa_attribute);
		    ah->ah_life_dur = (unsigned char *)malloc(ah->ah_life_len);
		    bcopy((char *)ptr, (char *)ah->ah_life_dur, 
						ah->ah_life_len);
		} else {
		    ah->ah_life_len = sizeof(unsigned short);
		    ah->ah_life_dur = (unsigned char *)
					    malloc(sizeof(unsigned short));
		    bcopy((char *)&saa.att_type.basic_value, 
			  (char *)ah->ah_life_dur, sizeof(unsigned short));
		}
		break;
	    case IPSEC_GROUP_DESC:
		*dh_group_desc = saa.att_type.basic_value;
		break;
	    case IPSEC_ENCAPSULATION_MODE:
		ah->tunnel = saa.att_type.basic_value;
		break;
	    default:
		if(!saa.type)
		    ptr += sizeof(sa_attribute);
    	}
	if(!saa.type)
	    ptr += saa.att_type.vpi_length;
	else
	    ptr += sizeof(sa_attribute);
    } while(ptr < fin);
}

static void esp_atts_to_sa(unsigned char *ptr, int len, 
		struct esp_proposal *esp, unsigned short *dh_group_desc)
{
    sa_attribute saa;
    unsigned char *fin;

    if(len == 0)
	return;
    fin = ptr + len;
    do {
	bcopy((char *)ptr, (char *)&saa, sizeof(sa_attribute));
	ntoh_att(&saa);
	switch(saa.att_class){
	    case IPSEC_SA_LIFE_TYPE:
		esp->esp_life_type = saa.att_type.basic_value;
		break;
	    case IPSEC_SA_LIFE_DURATION:
		if(saa.type == 0){
		    esp->esp_life_len = saa.att_type.vpi_length;
		    ptr += sizeof(sa_attribute);
		    esp->esp_life_dur = (unsigned char *)
						malloc(esp->esp_life_len);
		    bcopy((char *)ptr, (char *)esp->esp_life_dur, 
						esp->esp_life_len);
		} else {
		    esp->esp_life_len = sizeof(unsigned short);
		    esp->esp_life_dur = (unsigned char *)
					    malloc(sizeof(unsigned short));
		    bcopy((char *)&saa.att_type.basic_value, 
			  (char *)esp->esp_life_dur, sizeof(unsigned short));
		}
		break;
	    case IPSEC_GROUP_DESC:
		*dh_group_desc = saa.att_type.basic_value;
		break;
 	    case IPSEC_ENCAPSULATION_MODE:
		esp->tunnel = saa.att_type.basic_value;
		break;
	    case IPSEC_HMAC_ALG:
		esp->hmac_alg = saa.att_type.basic_value;
		break;
	    default:
		if(!saa.type)
		    ptr += sizeof(sa_attribute);
   	}
	if(!saa.type)
	    ptr += saa.att_type.vpi_length;
	else
	    ptr += sizeof(sa_attribute);
    } while(ptr < fin);
}

int ah_esp_atts_to_sa(SA *sa, sa_offer *offer, unsigned long mess_id)
{
    sa_list *leaf;
    conn_entry *node = NULL;
    unsigned short dh_group_desc;

    do {
	leaf = get_sa_by_protocol(sa, offer->protocol, mess_id);
	if(leaf == NULL){
	    LOG((ERR, "No SA from mess_id %ld and protocol %d", mess_id, 
			leaf->other_sa.type));
	    return(1);
	}
	dh_group_desc = 0;
	switch(offer->protocol){
	    case PROTO_IPSEC_AH:
		leaf->other_sa.ah.transform = offer->transform;
		ah_atts_to_sa(offer->atts, offer->natts, 
			((struct ah_proposal *)(&leaf->other_sa.ah)),
			&dh_group_desc);
		break;
	    case PROTO_IPSEC_ESP:
		leaf->other_sa.esp.transform = offer->transform;
		/*
		 * if other symmetric ciphers are added that aren't 64 bit
		 * then this must change to some case statement
		 */
		leaf->other_sa.esp.crypt_ivlen = CYPHER_BLOCKLEN;
		esp_atts_to_sa(offer->atts, offer->natts,
			((struct esp_proposal *)(&leaf->other_sa.esp)),
			&dh_group_desc);
		break;
	    default:
		LOG((ERR, "Unknown protocol %d", offer->protocol));
		return(1);
	}
	/*
	 * make sure that the DH group for PFS gets assigned to the
	 * parent connection ID for this list of offers. Also make 
	 * sure that they all have the same DH group!
	 */
	if(node == NULL){
	    if((node = get_centry_by_mess_id(mess_id)) == NULL){
		LOG((ERR, "Unable to get conn_entry for %ld", mess_id));
		return(1);
	    }
	    node->dh.group_desc = dh_group_desc;
	} else {
	    if(node->dh.group_desc != dh_group_desc){
		LOG((ERR, "Can't change the DH group for PFS!"));
		return(1);
	    }
	}
	offer = offer->next;
    } while(offer != NULL);

    return(0);
}

