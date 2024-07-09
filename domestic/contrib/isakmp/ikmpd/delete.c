/* 
 * Id: delete.c,v 1.3 1997/07/24 19:41:49 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/delete.c,v
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

/*
 * del.c - process and create delete payloads
 */

int process_delete (SA *sa, unsigned char *payload, unsigned long message_id)
{
    delete_payload del;
    int del_len;
    unsigned char *ptr;

    bcopy((char *)payload, (char *)&del, sizeof(delete_payload));
    ntoh_delete(&del);
    if(del.doi != IPSEC_DOI){
	LOG((WARN, "Invalid DOI in delete message!"));
	return(1);
    }
    del_len = del.payload_len - sizeof(delete_payload);

    if(message_id){
	/*
	 * I'm not going to accept deletes while in the middle of processing
	 * a message
	 */
	return(0);
    } else {
	switch(del.protocol_id){
	    case PROTO_ISAKMP:
		/* 
		 * local policy decision: I won't let one ISAKMP SA protect a 
		 * delete message for a different ISAKMP SA. It's not an error 
		 * since a delete is really a courtesy on the other guy's part.
		 * I just won't honor the request. Other implementations might.
		 */
		if((del.spi_size != COOKIE_LEN) || (del.nspis != 2)){
		    LOG((WARN, "Invalid SA delete message."));
		    return(0);
		}
		ptr = payload + sizeof(delete_payload);
		if(sa->init_or_resp){
		    if(bcmp((char *)sa->my_cookie, (char *)ptr, COOKIE_LEN)){
			LOG((WARN, "ISAKMP SA delete msg for a different SA!"));
			return(0);
		    }
		    ptr += COOKIE_LEN;
		    if(bcmp((char *)sa->his_cookie, (char *)ptr, COOKIE_LEN)){
			LOG((WARN, "ISAKMP SA delete msg for a different SA!"));
			return(0);
		    }
		} else {
		    if(bcmp((char *)sa->his_cookie, (char *)ptr, COOKIE_LEN)){
			LOG((WARN, "ISAKMP SA delete msg for a different SA!"));
			return(0);
		    }
		    ptr += COOKIE_LEN;
		    if(bcmp((char *)sa->my_cookie, (char *)ptr, COOKIE_LEN)){
			LOG((WARN, "ISAKMP SA delete msg for a different SA!"));
			return(0);
		    }
		}
		/*
		 * reaper will do actual deletion. Just mark it as deleted
		 * and make sure it has no outstanding IPsec SA's.
		 */
		sa->condition = SA_COND_DEAD;
		isadb_delete_all(sa);
		break;
	    case PROTO_IPSEC_AH:
	    case PROTO_IPSEC_ESP:
		/*
		 * ikmpd does not remember which SPIs were created with
		 * which ISAKMP SA's. This may have to be changed. But for
		 * now just don't handle IPsec SA delete messages.
		 */
		break;
	    default:
		LOG((WARN, "Unknown protocol in delete message!"));
	}
    }
    return(0);
}

int construct_delete (SA *sa, unsigned char nextp, int *pos)
{
    delete_payload del;

    bzero((char *)&del, sizeof(delete_payload));
    del.doi = IPSEC_DOI;
    del.next_payload = nextp;
    del.payload_len = sizeof(delete_payload) + (2 * COOKIE_LEN);
    del.protocol_id = PROTO_ISAKMP;
    hton_delete(&del);

    expand_packet(sa, *pos, (sizeof(delete_payload) + (2 * COOKIE_LEN)));
    bcopy((char *)&del, (char *)(sa->packet + *pos),
		sizeof(delete_payload));
    *pos += sizeof(delete_payload);
    if(sa->init_or_resp == INITIATOR){
	bcopy((char *)sa->my_cookie, (char *)(sa->packet + *pos), COOKIE_LEN);
	*pos += COOKIE_LEN;
	bcopy((char *)sa->his_cookie, (char *)(sa->packet + *pos), COOKIE_LEN);
	*pos += COOKIE_LEN;
    } else {
	bcopy((char *)sa->his_cookie, (char *)(sa->packet + *pos), COOKIE_LEN);
	*pos += COOKIE_LEN;
	bcopy((char *)sa->my_cookie, (char *)(sa->packet + *pos), COOKIE_LEN);
	*pos += COOKIE_LEN;
    }
    return(0);
}

