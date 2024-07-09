/* 
 * Id: list_keys.c,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/list_keys.c,v
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
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    union public_key rec;
    int keytype;

    switch(argc){
	case 1:
	    keytype = DSSKEYS;
	    break;
	case 2:
#ifdef NON_COMMERCIAL_AND_RSAREF
	    keytype = RSAKEYS;
	    break;
#else
	    fprintf(stderr,"RSA is for non-commercial use only.\n");
	    exit(1);
#endif
	default:
	    fprintf(stderr,"USAGE: %s [RSA]\n", argv[0]);
	    exit(1);
    }
    init_siglib(keytype);
    printf("---------------------------------------------\n");
    while(get_next_pub_rec(&rec) == 0){
	asciify_pubkey(keytype, &rec);
	printf("---------------------------------------------\n");
    }
    term_siglib();
    exit(0);
}

