/* 
 * Id: initiator.c,v 1.3 1997/04/12 01:02:13 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/initiator.c,v
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
#include <sys/time.h>
#else
#include <types.h>
#include <socket.h>
#include <netinet/in.h>
#endif /* MULTINET */
#ifndef KEY2
#include <netkey/key.h>
#endif
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * initiator.c -- routines specific to the initiator portion of ISAKMP
 *	protocol. Since it proposes and the responder accepts (or rejects)
 *	these are initiator-specific.
 */

int init_construct_init (SA *sa)
{
    int pos;

	/*
	 * these algs should, ideally, come from the PF_KEY, and they should
	 * have some implied preference and logical construction. For now just
	 * stick 'em here in plain serial, logical OR.
	 */

    sa->hash_alg = HASH_SHA;
    sa->encr_alg = ET_DES_CBC;
    sa->auth_alg = PRESHRD;
    sa->dh.group_desc = 1;		/* the default Oakley group */

    construct_header(sa, OAK_MM, 0, ISA_SA);
    pos = sizeof(isakmp_hdr);
    construct_isakmp_sa(sa, 0, &pos);
    return(0);
}

SA *initiator (struct sockaddr_in *to_addr, 
	       struct sockaddr_in *src, 
	       struct sockaddr_in *proxy_dst,
		unsigned short type)
{
    SA *security_assoc;
    int status;
	/*
	 * create entry in db 
	 */
    LOG((DEB,"INITITATOR...."));
    status =isadb_is_outstanding_kernel_req(to_addr, type);
    switch(status){
	case 0:
	    break;
	case -1:
	    LOG((WARN,"Kernel is starving."));
	    return(NULL);
	case 1:
	/*
	 * cookie exists, but not that type. If we've authenticated the SA
	 * already we can skip right to oakley_init_qm(), otherwise just
	 * return, the new type has been recorded.
	 */
	    security_assoc = isadb_get_entry_by_addr(to_addr);
	    if((security_assoc != NULL) &&
		(security_assoc->state == OAK_QM_IDLE)){
		/*
		 * revert to key auth state to transition from idle to active
		 * quick mode. Also prevents two successive key acquire
		 * messages from resulting in 2 QM's initiated for 2 SA's!
		 */
		if(security_assoc->packet_len){
		    free(security_assoc->packet);
		    security_assoc->packet_len = 0;
		}
		security_assoc->init_or_resp = INITIATOR;
		security_assoc->state = OAK_MM_KEY_AUTH;
		delay_idle_qm_transition(security_assoc);
	    }
	    return(NULL);
	default:
	    return(NULL);
    }
    security_assoc = isadb_create_entry(src, INITIATOR, type);
    gen_cookie(to_addr, security_assoc->my_cookie);
    bcopy((char *)to_addr, (char *)&security_assoc->dst, 
			sizeof(struct sockaddr_in));
    if(init_construct_init(security_assoc)){
	LOG((CRIT, "Unable to initiate protocol!"));
	return(NULL);
    }
    security_assoc->send_dir = BIDIR;
    /*
     * If a proxy endpoint destination was specified, note this 
     * in the association for Quick Mode identity processing.
     */
    if (proxy_dst && proxy_dst->sin_family == AF_INET)
	bcopy(proxy_dst, &security_assoc->proxy_dst, sizeof(struct sockaddr_in));
    return(security_assoc);
}

