/*
 * Id: process.c,v 1.2 1997/04/12 01:02:24 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/process.c,v
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
#else
#include <types.h>
#include <socket.h>
#endif /* MULTINET */
#include <netinet/in.h>
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

struct protopak {
	int (*process)(SA *sa, unsigned char *payload, 
			unsigned long mess_id);
	char procname[4];
} ikdpak[] = {
	{ process_err,	  "err " },
	{ process_sa,	  "sa  " },
	{ process_noop,	  "prop" },	/* should be taken care of by sa */
	{ process_noop,	  "tran" },	/* ditto */
	{ process_ke,	  "ke  " }, 
	{ process_id,	  "id  " },
	{ process_cert,	  "cert" },
	{ process_cert,	  "creq" },
	{ process_hash,	  "hash" },
	{ process_sig,	  "sig " },
	{ process_nonce,  "non " },
	{ process_notify, "not " },
	{ process_delete, "del " }
};

/*
 * recursively process all payloads in the megapayload. 
 */
int process(unsigned char paktype, SA *sa, unsigned char *payload,
	unsigned long mess_id)
{
    generic_payload dummy;
    int len, status = 0;

    if(paktype > ISA_DELETE){
	construct_header(sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	len = sizeof(isakmp_hdr);
	construct_notify(sa, 0, INVALID_PAYLOAD, &paktype,
			sizeof(unsigned char), &len);
	return(1);
    }
    bcopy((char *)payload, (char *)&dummy, sizeof(generic_payload));
    ntoh_dummy(&dummy);
    if((*ikdpak[paktype].process)(sa, payload, mess_id)){
	LOG((ERR,"Error processing %d\n", paktype));
	return(1);
    }
    len = dummy.payload_len;
    if(dummy.next_payload){
	status = process(dummy.next_payload, sa, (payload + len), mess_id);
    }
    return(status);
}

