/* 
 * Id: id.c,v 1.3 1997/07/24 19:41:52 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/id.c,v
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
 * id.c - process and create ID payloads. Currently only ID_IPV4_ADDR is
 *	supported.
 */
int process_id (SA *sa, unsigned char *payload, unsigned long message_id)
{
    id_payload id;
    conn_entry *centry;
    int id_len;

    bcopy((char *)payload, (char *)&id, sizeof(id_payload));
    ntoh_id(&id);
    id_len = id.payload_len - sizeof(id_payload);

    if(message_id){
	centry = get_centry_by_mess_id(message_id);
	if(centry == NULL){
	    LOG((ERR, "cannot obtain state for message id %ld\n", message_id));
	    return(1);
	}
	/*
	 * by definition it's src then dst so the 1st time through here it's
	 * for the source, 2nd time is for the destination. When the id
	 * payload supports protocol and port set them in the sockaddr_in.
	 */
	if(centry->proxy_src.id_type == 0){
	    centry->proxy_src.id_type = id.id_type;
	    switch(id.id_type){
		case ID_IPV4_ADDR:
		    bcopy((char *)(payload + sizeof(id_payload)),
			(char *)&centry->proxy_src.addr.sin_addr.s_addr, 
			sizeof(struct in_addr));
		    break;
		case ID_IPV4_ADDR_SUBNET:
		    bcopy((char *)(payload + sizeof(id_payload)),
			(char *)&centry->proxy_src.addr.sin_addr.s_addr, 
			sizeof(struct in_addr));
		    bcopy((char *)(payload + sizeof(id_payload) + 
					     sizeof(struct in_addr)),
			  (char *)&centry->proxy_src.mask, 
			  sizeof(struct in_addr));
		    break;
		case ID_FQDN:
		case ID_USER_FQDN:
		case ID_IPV6_ADDR:
		case ID_IPV6_ADDR_RANGE:
		    LOG((ERR, "Cannot handle that id yet!"));
		    return(1);
		default:
		    LOG((ERR, "Unknown proxy src ID type %d", id.id_type));
		    return(1);
	    }
	} else if(centry->proxy_dst.id_type == 0){
	    centry->proxy_dst.id_type = id.id_type;
	    switch(id.id_type){
		case ID_IPV4_ADDR:
		    bcopy((char *)(payload + sizeof(id_payload)),
			(char *)&centry->proxy_dst.addr.sin_addr.s_addr, 
			sizeof(struct in_addr));
		    break;
		case ID_IPV4_ADDR_SUBNET:
		    bcopy((char *)(payload + sizeof(id_payload)),
			(char *)&centry->proxy_dst.addr.sin_addr.s_addr, 
			sizeof(struct in_addr));
		    bcopy((char *)(payload + sizeof(id_payload) + 
					     sizeof(struct in_addr)),
			  (char *)&centry->proxy_dst.mask, 
			  sizeof(struct in_addr));
		    break;
		case ID_FQDN:
		case ID_USER_FQDN:
		case ID_IPV6_ADDR:
		case ID_IPV6_ADDR_RANGE:
		    LOG((ERR, "Cannot handle that id yet!"));
		    return(1);
		default:
		    LOG((ERR, "Unknown proxy dst ID type %d", id.id_type));
		    return(1);
	    }
	}
    } else {
	switch(id.id_type){
	    case ID_IPV4_ADDR:
		bcopy((char *)(payload + sizeof(id_payload)),
			(char *)&sa->opposing_id.ipaddr,
                        sizeof(struct in_addr));
		sa->opposing_id.id_type = ID_IPV4_ADDR;
		sa->opposing_id.port = id.port;
		sa->opposing_id.protocol = id.protocol;
		break;
	    case ID_FQDN:
	    case ID_USER_FQDN:
	    case ID_IPV4_ADDR_RANGE:
	    case ID_IPV4_ADDR_SUBNET:
	    case ID_IPV6_ADDR:
	    case ID_IPV6_ADDR_RANGE:
	    case ID_IPV6_ADDR_SUBNET:
		LOG((ERR, "Cannot handle that id yet!"));
		return(1);
	    default:
		LOG((ERR, "Unknown phase 1 ID type %d", id.id_type));
		return(1);
	}
    }
    return(0);
}

int construct_id (SA *sa, unsigned char nextp, int *pos)
{
    id_payload id;
    union key_pair me;
    int nbytes;

    bzero((char *)&id, sizeof(id_payload));
    nbytes = sizeof(id_payload) + sizeof(struct in_addr);
    id.next_payload = nextp;
    id.payload_len = nbytes;
    id.id_type = ID_IPV4_ADDR;		/* all we do for now... */
    id.protocol = IPPROTO_UDP;
    id.port = IKMP_PORT;
    hton_id(&id);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&id, (char *)(sa->packet + *pos),
		sizeof(id_payload));
    *pos += sizeof(id_payload);

	/*
	 * for now use the ID from the DSS key if pre-shared keys are used
	 * to authenticate. This is really a policy decision that should
	 * be made when the pre-shared keys are established.
	 * Also, since DSS keys are the default for this reference implement-
	 * ation, if RSA isn't installed use the ID from the DSS key.
	 */
    switch(sa->auth_alg){
	case RSA_SIG:
	case RSA_ENC:
#ifdef NON_COMMERCIAL_AND_RSAREF
	    init_siglib(RSAKEYS);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my DSS keys!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.rsa_pair.pub.addr, (char *)(sa->packet + *pos), 
			sizeof(struct in_addr));
	    break;
#endif
	case PRESHRD:
	    init_siglib(PRESHAR);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my identity!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.myaddress, (char *)(sa->packet + *pos),
			sizeof(struct in_addr));
	    break;
	case DSS_SIG:
	default:
	    init_siglib(DSSKEYS);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my DSS keys!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.dss_pair.pub.addr, (char *)(sa->packet + *pos), 
			sizeof(struct in_addr));
	    break;
    }
    term_siglib();
    *pos += sizeof(struct in_addr);

    return(0);
}

int construct_proxy_id (SA *sa, struct identity *whom, uchar nextp, int *pos)
{
    id_payload id;
    int nbytes;

    bzero((char *)&id, sizeof(id_payload));
    switch(whom->id_type){
	case ID_IPV4_ADDR:
	    nbytes = sizeof(id_payload) + sizeof(struct in_addr);
	    break;
	case ID_IPV4_ADDR_SUBNET:
	    nbytes = sizeof(id_payload) + ( 2 * sizeof(struct in_addr));
	    break;
	default:
	    LOG((ERR, "we don't do that yet!"));
	    return(1);
    }
    id.next_payload = nextp;
    id.payload_len = nbytes;
    id.id_type = whom->id_type;
    hton_id(&id);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&id, (char *)(sa->packet + *pos),
		sizeof(id_payload));
    *pos += sizeof(id_payload);

    switch(whom->id_type){
	case ID_IPV4_ADDR:
	    bcopy((char *)&whom->addr.sin_addr.s_addr, 
		  (char *)(sa->packet + *pos),
		  sizeof(struct in_addr));
	    *pos += sizeof(struct in_addr);
	    break;
	case ID_IPV4_ADDR_SUBNET:
	    bcopy((char *)&whom->addr.sin_addr.s_addr, 
		  (char *)(sa->packet + *pos), sizeof(struct in_addr));
	    *pos += sizeof(struct in_addr);
	    bcopy((char *)&whom->mask, (char *)(sa->packet + *pos),
		  sizeof(struct in_addr));
	    *pos += sizeof(struct in_addr);
	    break;
    }
    return(0);
}

int obtain_id (SA *sa, struct in_addr *addr)
{
    union key_pair me;

    switch(sa->auth_alg){
	case RSA_SIG:
	case RSA_ENC:
#ifdef NON_COMMERCIAL_AND_RSAREF
	    init_siglib(RSAKEYS);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my identity!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.rsa_pair.pub.addr, (char *)addr,
			sizeof(struct in_addr));
	    break;
#endif
	case DSS_SIG:
	    init_siglib(DSSKEYS);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my identity!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.dss_pair.pub.addr, (char *)addr,
			sizeof(struct in_addr));
	    break;
	case PRESHRD:
	default:
	    init_siglib(PRESHAR);
	    if(get_priv_entry(&me)){
		LOG((ERR, "Unable to obtain my identity!"));
		term_siglib();
		return(1);
	    }
	    bcopy((char *)&me.myaddress, (char *)addr, sizeof(struct in_addr));
	    break;
    }
    term_siglib();
    return(0);
}

