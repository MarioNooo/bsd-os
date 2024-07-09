/* 
 * Id: show_preshr.c,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/show_preshr.c,v
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    union public_key rec;
    SHA_context ctx;
    unsigned char keyhash[SHA_LENGTH];
    int i;

    init_siglib(PRESHAR);
    printf("---------------------------------------------\n");
    while(get_next_pub_rec(&rec) == 0){
	SHAInit(&ctx);
	if(SHAUpdate(&ctx, rec.preshr.key, rec.preshr.keylen) != SUCCESS){
	    fprintf(stderr,"%s: Unable to generate hash for key\n", argv[0]);
	    continue;
	}
	if(SHAFinal(&ctx, keyhash) != SUCCESS){
	    fprintf(stderr,"%s: Unable to generate hash for key\n", argv[0]);
	    continue;
	}
	printf("%s\n", inet_ntoa(rec.addr));
	for(i=0; i<SHA_LENGTH; i++)
	    if(keyhash[i] > 0x0f)
		printf("%x ", keyhash[i]);
	    else
		printf("0%x ", keyhash[i]);
	printf("\n");
	printf("---------------------------------------------\n");
    }
    term_siglib();
    exit(0);
}

