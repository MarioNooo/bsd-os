/* 
 * Id: responder.c,v 1.2 1997/04/12 01:02:26 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/responder.c,v
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <string.h>
#else
#include <types.h>
#include <socket.h>
#endif /* MULTINET */
#include <netinet/in.h>
#ifndef KEY2
#include <netkey/key.h>
#endif
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * responder.c -- routines unique to the responder portion of the ISAKMP
 *	protocol.
 */

SA *responder (isakmp_hdr *hdr, struct sockaddr_in *from, int fd,
			unsigned char *payload)
{
    SA *security_assoc;
#ifndef MULTINET
    int sinlen;
    struct sockaddr_in sin;
#endif /* MULTINET */

    if(isadb_is_outstanding_init_req(hdr, from)){
	LOG((WARN,"Cookie exists already! Boy are we slow."));
	return(NULL);
    }
	/*
	 * create an isadb entry. Type is zero because the responder 
	 * doesn't know yet.
	 */
    security_assoc = isadb_create_entry(from, RESPONDER, 0);
    security_assoc->fd = fd;
    gen_cookie(from, security_assoc->my_cookie);
    bcopy((char *)hdr->init_cookie, (char *)security_assoc->his_cookie,
	sizeof(security_assoc->his_cookie));
    security_assoc->payload = (unsigned char *) malloc 
				(hdr->len * sizeof(unsigned char));
    if(security_assoc->payload == NULL){
	LOG((CRIT,"Out Of Memory"));
	return(NULL);
    }
    security_assoc->payload_len = hdr->len;
    bcopy((char *)payload, (char *)security_assoc->payload, hdr->len);
    security_assoc->send_dir = BIDIR;
	/*
	 * if we can't get the address just do localhost and probably fail
	 */
#ifndef MULTINET
    security_assoc->dst.sin_len = sizeof(struct sockaddr_in);
#endif /* MULTINET */
    security_assoc->dst.sin_family = AF_INET;
    security_assoc->dst.sin_port = htons(IKMP_PORT);
#ifndef MULTINET
    sinlen = sizeof(sin);
    if (getsockname(fd, (struct sockaddr *)&sin, &sinlen) == 0 &&
	    sin.sin_addr.s_addr != INADDR_ANY)
	security_assoc->dst.sin_addr = sin.sin_addr;
    else
	LOG((ERR, "Unable to determine my address!"));
#endif /* MULTINET */
    /*
     * the address will be set after processing the ID either directly 
     * if the ID is an address or via DNS lookup if it's a name. Note:
     * once the port is used or further granularity of policy is allowed--
     * like username-- then more stuff will need to be recorded and stuffed.
     * For now, the NRL code doesn't do it.
     */

    return(security_assoc);
}

