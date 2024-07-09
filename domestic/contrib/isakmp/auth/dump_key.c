/* 
 * Id: dump_key.c,v 1.1.1.1 1997/03/06 01:53:46 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/dump_key.c,v
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
#include <fcntl.h>
#include "toolkit.h"
#include <string.h>
#include "authdef.h"

int main (int argc, char **argv)
{
    struct utsname myself;
    union key_pair my_sig;
    union public_key pubpart;
    int fd, keytype;
    char keyfile[80];

    if(argc > 2){
	fprintf(stderr,"USAGE: %s [RSA]\n", argv[0]);
	exit(1);
    }
    if(uname(&myself)){
	fprintf(stderr,"%s: Unable to obtain hostname!\n", argv[0]);
	exit(1);
    }

    if(argc == 2){
#ifdef NON_COMMERCIAL_AND_RSAREF
	keytype = RSAKEYS;
	sprintf(keyfile, "%s/%s.rsapriv", KEYPATH, myself.nodename);
#else
	fprintf(stderr,"%s: RSAREF is for non-commercial use only.\n", argv[0]);
	exit(1);
#endif
    } else {
	keytype = DSSKEYS;
	sprintf(keyfile, "%s/%s.dsspriv", KEYPATH, myself.nodename);
    }
    if((fd = open(keyfile, O_RDONLY)) < 0){
	fprintf(stderr,"%s: Unable to open key file %s\n", argv[0], keyfile);
	exit(1);
    }
    if(keytype == DSSKEYS){
	if(read(fd,&my_sig,sizeof(struct dss_signature)) != 
			sizeof(struct dss_signature)){
	    fprintf(stderr,"%s: Unable to read key from file %s\n",argv[0],
			keyfile);
	    exit(1);
	}
	bcopy((char *)&my_sig.dss_pair.pub, (char *)&pubpart,
		sizeof(struct dss_pub_key));
#ifdef NON_COMMERCIAL_AND_RSAREF
    } else {
	if(read(fd,&my_sig,sizeof(struct rsa_key_pair)) != 
			sizeof(struct rsa_key_pair)){
	    fprintf(stderr,"%s: Unable to read key from file %s\n",argv[0],
			keyfile);
	    exit(1);
	}
	bcopy((char *)&my_sig.rsa_pair.pub, (char *)&pubpart,
		sizeof(struct rsa_pub_key));
#endif
    }
    close(fd);
    asciify_pubkey(keytype, &pubpart);
    exit(0);
}

