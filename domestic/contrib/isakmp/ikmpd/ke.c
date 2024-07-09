/* 
 * Id: ke.c,v 1.3 1997/04/12 01:02:18 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/ke.c,v
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
#else
#include <types.h>
#include <socket.h>
#endif /* MULTINET */
#include <netinet/in.h>
#ifndef KEY2
#include <netsec/ipsec.h>
#endif
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * ke.c -- routines to construct and process key exchange payloads 
 */
int process_ke (SA *sa, unsigned char *payload, unsigned long mess_id)
{
    key_x_payload ke;
    conn_entry *centry;

    if(mess_id){
	if((centry = get_centry_by_mess_id(mess_id)) == NULL){
	    LOG((ERR, "Unable to get conn_entry for %ld", mess_id));
	    return(1);
	}
	LOG((DEB,"processing ISA_KE for PFS in phase 2"));
	bcopy((char *)payload, (char *)&ke, sizeof(key_x_payload));
	ntoh_key(&ke);
	bcopy((char *)(payload + sizeof(key_x_payload)), 
			(char *)centry->dh.his_DH_pub_val, 
			(ke.payload_len - sizeof(key_x_payload)));

	if(genDHSharedSecret(centry->dh.group.modp.prime_len, 
			centry->dh.DH_priv_val,
			centry->dh.his_DH_pub_val, centry->dh.g_to_xy, 
			centry->dh.group.modp.p) != SUCCESS){
	    LOG((ERR,"Unable to compute shared secret for PFS in phase 2!"));
	    return(1);
	}
	centry->flags |= PFS_ACCOMPLISHED;
    } else {
	if(sa->state_mask & PROC_KE)
	    return(0);

	LOG((DEB,"processing ISA_KE"));
	bcopy((char *)payload, (char *)&ke, sizeof(key_x_payload));
	ntoh_key(&ke);
	bcopy((char *)(payload + sizeof(key_x_payload)), 
			(char *)sa->dh.his_DH_pub_val, 
			(ke.payload_len - sizeof(key_x_payload)));

	if(genDHSharedSecret(sa->dh.group.modp.prime_len, sa->dh.DH_priv_val,
			sa->dh.his_DH_pub_val, sa->dh.g_to_xy, 
			sa->dh.group.modp.p) != SUCCESS){
	    LOG((ERR,"Unable to compute shared secret!"));
	    return(1);
	}
	sa->state_mask |= PROC_KE;
    }
    return(0);
}

int construct_ke (SA *sa, unsigned char nextp, int *pos)
{
    key_x_payload ke;
    int nbytes;

    if(sa->state_mask & CONST_KE)
	return(0);

    LOG((DEB,"constructing ISA_KE"));
    nbytes = sizeof(key_x_payload) + sa->dh.dh_len;

    bzero((char *)&ke, sizeof(key_x_payload));
    ke.next_payload = nextp;
    ke.payload_len = nbytes;
    hton_key(&ke);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&ke,(char *)(sa->packet + *pos),sizeof(key_x_payload));
    *pos += sizeof(key_x_payload);

    bcopy((char *)sa->dh.DH_pub_val, (char *)(sa->packet + *pos), sa->dh.dh_len);
    *pos += sa->dh.dh_len;

    sa->state_mask |= CONST_KE;
    return(0);
}

int construct_pfs_ke (SA *sa, conn_entry *centry, unsigned char nextp, int *pos)
{
    key_x_payload ke;
    int nbytes;

    LOG((DEB,"constructing ISA_KE for PFS in phase 2"));
    nbytes = sizeof(key_x_payload) + centry->dh.dh_len;

    bzero((char *)&ke, sizeof(key_x_payload));
    ke.next_payload = nextp;
    ke.payload_len = nbytes;
    hton_key(&ke);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&ke,(char *)(sa->packet + *pos),sizeof(key_x_payload));
    *pos += sizeof(key_x_payload);

    bcopy((char *)centry->dh.DH_pub_val, (char *)(sa->packet + *pos), 
		centry->dh.dh_len);
    *pos += centry->dh.dh_len;

    return(0);
}

