/* 
 * Id: cert.c,v 1.2 1997/04/12 01:02:00 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/cert.c,v
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

/*
 * cert.c -- placeholder for eventual processing and construction of
 *	certificate payloads.
 */

#define BOGUS_CERT_LEN 16;

int process_cert (SA *sa, unsigned char *payload, unsigned long mess_id)
{
    cert_payload cert;
    unsigned char *cert_data;

    LOG((DEB,"processing ISA_CERT"));
    bcopy((char *)payload, (char *)&cert, sizeof(cert_payload));
    ntoh_cert(&cert);
    cert_data = (unsigned char *) malloc (cert.payload_len);
    if(cert_data == NULL){
	LOG((CRIT,"Out Of Memory"));
	return(1);
    }
    bcopy((char *)(payload + cert.payload_len), (char *)cert_data, 
		cert.payload_len);
    free(cert_data);
    return(0);
}

int construct_cert (SA *sa, unsigned char nextp, int *pos)
{
    cert_payload cert;
    int nbytes;
 
    nbytes = sizeof(cert_payload) + BOGUS_CERT_LEN;

    bzero((char *)&cert, sizeof(cert_payload));
    cert.next_payload = nextp;
    cert.payload_len = nbytes;
    hton_cert(&cert);

    expand_packet(sa, *pos, nbytes);
    bcopy((char *)&cert, (char *)(sa->packet + *pos), 
			sizeof(cert_payload));
    *pos += sizeof(cert_payload);
	/* it is bogus so don't copy anything there */
    *pos += BOGUS_CERT_LEN;

    return(0);
}

