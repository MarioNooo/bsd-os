/* 
 * Id: notify.c,v 1.3 1997/07/24 19:42:04 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/notify.c,v
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

int process_notify (SA *sa, unsigned char *payload, 
		unsigned long mess_id)
{
    notify_payload notify;
    SA *secassoc;
    sa_list *node;
    unsigned char *p;
    int len;

    bcopy((char *)payload, (char *)&notify, sizeof(notify_payload));
    ntoh_notify(&notify);
    if(notify.doi != IPSEC_DOI){
	LOG((WARN, "Invalid DOI in notify message, %ld", notify.doi));
	return(1);
    }
    p = payload + sizeof(notify_payload);
    switch(notify.notify_message){
	case INVALID_COOKIE:	/* data is an isakmp_hdr */
	/*
	 * we want to remove this SA from the database since it seems to
	 * have gone stale. But we are in the midst of processing payloads
	 * for this SA so we have to put off the deletion for a tad.
	 */
	    LOG((WARN, "Received INVALID_COOKIE message"));
		/*
		 * if there were outstanding SA requests associated with this
		 * we're going to have to start from scratch. Create another
		 * entry (just say AH to prevent an error message) and jump 
		 * in as the initiator.
		 */
	    node = get_sa_list(sa, mess_id);
	    if(node){
		secassoc = isadb_create_entry(&sa->src, INITIATOR, 
					PROTO_IPSEC_AH);
		if(secassoc == NULL){
		    LOG((ERR, "Unable to create SA! Cannot fulfil request!"));
		    return(0);		/* it's a local error, don't return 1 */
		}
		gen_cookie(&sa->dst, secassoc->my_cookie);
		bcopy((char *)&sa->dst, (char *)&secassoc->dst, 
				sizeof(struct sockaddr_in));
		/*
		 * assign outstanding SA's from old secassoc to new one.
		 */
		secassoc->outstanding_sa = 0;
		while(node){
		    switch(node->other_sa.type){
			case PROTO_IPSEC_AH:
			    secassoc->outstanding_sa |= PROTO_IPSEC_AH_MASK;
			    break;
			case PROTO_IPSEC_ESP:
			    secassoc->outstanding_sa |= PROTO_IPSEC_ESP_MASK;
			    break;
			default:
			    LOG((WARN, "Unknown SA type: %d", 
				node->other_sa.type));
		    }
		    node = node->next;
		}
		if(init_construct_init(secassoc)){
		    LOG((CRIT, "Unable to initiator protocol!"));
		    return(0);		/* ditto */
		}
		secassoc->send_dir = BIDIR;
	    }
	    /*
	     * let the reaper take care of this guy
	     */
	    sa->condition = SA_COND_DEAD;
	    isadb_delete_all(sa);
	    break;
	case LIKE_HELLO:
	    construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	    len = sizeof(isakmp_hdr);
	    construct_notify(sa, 0, SHUT_UP, NULL, 0, &len);
	    sa->send_dir = UNIDIR;
	    break;
	case INVALID_SPI:	/* data is an unsigned long: a spi */
	    LOG((WARN, "Received INVALID_SPI message"));
#ifndef MULTINET
	    printf("invalid spi of %ld\n", (unsigned long)(*p));
#endif
	    node = get_sa_list(sa, mess_id);
	    kernel_delete_spi(sa, ((unsigned long)(*p)), node->other_sa.type);
	    break;
	/*
	 * right now just resend packet. We may want to do something special
	 * depending on our policy-- e.g. if we've got more proposals that
	 * we didnt send but could, then reconstruct the proposal. Just
	 * assume for now that the packet was mangled in transit, if it is 
	 * truely bad then the err_counter will put a merciful end to this.
	 */
	case DOI_NOT_SUPPORTED:
	case NO_PROPOSAL_CHOSEN:
	case PAYLOAD_MALFORMED:
	case DECRYPTION_FAILED:
	case INVALID_SIGNATURE:
	    try_again(sa);
	    break;
	default:
	    LOG((WARN,"Unknown Notify Message %d\n", notify.notify_message));
    }
    return(0);
}

int construct_notify (SA *sa, unsigned char nextp, 
	unsigned short mess_type, void *data, int data_len, int *pos)
{
    notify_payload notify;
    int nbytes;
 
    nbytes = sizeof(notify_payload) + data_len;
    bzero((char *)&notify, sizeof(notify_payload));

    notify.doi = IPSEC_DOI;
    notify.next_payload = nextp;
    notify.notify_message = mess_type;

    notify.payload_len = nbytes;
    hton_notify(&notify);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&notify, (char *)(sa->packet + *pos),
		sizeof(notify_payload));
    *pos += sizeof(notify_payload);
    if(data_len && (data != NULL)){
	bcopy((char *)data, (char *)(sa->packet + *pos), data_len);
	*pos += data_len;
    }
    return(0);
}

