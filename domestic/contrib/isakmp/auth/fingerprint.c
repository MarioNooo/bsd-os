/* 
 * Id: fingerprint.c,v 1.1.1.1 1997/03/06 01:53:46 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/fingerprint.c,v
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
#include <string.h>
#include <fcntl.h>
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    union key_pair my_sig;
    union public_key pub;
    SHA_context hash_context;
    unsigned char fingerprint[SHA_LENGTH];
    FILE *fp = NULL;
    int i, keytype, mine, recsize;

    if(argc > 3){
	fprintf(stderr,"USAGE: %s [RSA] [ascii file]\n", argv[0]);
	exit(1);
    }
    /*
     * determine the keytype and whether this is a fingerprint of my
     * public key (mine = 1) or someone else's (mine = 0)
     */
    switch(argc){
	case 1:
	    keytype = DSSKEYS;
	    recsize = sizeof(struct dss_pub_key);
	    mine = 1;
	    break;
	case 2:
	    if(strncmp(argv[1], "RSA", 3) == 0){
#ifdef NON_COMMERCIAL_AND_RSAREF
		keytype = RSAKEYS;
		recsize = sizeof(struct rsa_pub_key);
		mine = 1;
#else
		fprintf(stderr,"%s: RSA is for non-commercial use only.\n", 
			argv[0]);
		exit(1);
#endif
	    } else {
		mine = 0;
		keytype = DSSKEYS;
		recsize = sizeof(struct dss_pub_key);
		fp = fopen(argv[1], "r+");
		if(fp == NULL){
		    fprintf(stderr,"%s: cannot open %s!\n", argv[0], argv[1]);
		    exit(1);
		}
	    }
	    break;
	case 3:
	    if(strncmp(argv[1], "RSA", 3)){
		fprintf(stderr,"USAGE: %s [RSA] [ascii file]\n", argv[0]);
		exit(1);
	    }
#ifdef NON_COMMERCIAL_AND_RSAREF
	    mine = 0;
	    keytype = RSAKEYS;
	    recsize = sizeof(struct rsa_pub_key);
	    fp = fopen(argv[2], "r+");
	    if(fp == NULL){
		fprintf(stderr,"%s: cannot open %s!\n", argv[0], argv[1]);
		exit(1);
	    }
	    break;
#else
	    fprintf(stderr,"%s: RSA is for non-commercial use only.\n",
			argv[0]);
	    exit(1);
#endif
	default:
	    fprintf(stderr,"Weirdness! Bye!\n");
	    exit(1);
    }
    /*
     * zero out the (possibly) unused portion and fill in the pub value
     */
    bzero((char *)&pub, sizeof(union public_key));
    if(mine){
	init_siglib(keytype);
	bzero((char *)&my_sig, sizeof(union key_pair));
	/*
	 * get my private entry and zero the private part and copy the pub
	 */
	if(get_priv_entry(&my_sig)){
	    fprintf(stderr,"%s: Unable to obtain my private key!\n", argv[0]);
	    exit(1);
	}
	switch(keytype){
	    case DSSKEYS:
		bzero((char *)&my_sig.dss_pair.x, SHA_LENGTH);
		bcopy((char *)&my_sig.dss_pair.pub, (char *)&pub.dsspub, 
			sizeof(struct dss_pub_key));
		break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	    case RSAKEYS:
		bzero((char *)&my_sig.rsa_pair.priv, sizeof(R_RSA_PRIVATE_KEY));
		bcopy((char *)&my_sig.rsa_pair.pub, (char *)&pub.rsapub,
			sizeof(struct rsa_pub_key));
		break;
#endif
	    default:
		fprintf(stderr,"%s: Unknown key type!\n", argv[0]);
		exit(1);
	}
	term_siglib();
    } else {
	/*
	 * if !mine then fp will already be opened
	 */
	init_siglib(keytype);
	binarify_pubkey(fp, &pub);
	fclose(fp);
	term_siglib();
    }
    /*
     * fingerprint is a SHA digest of the entire public key record
     */
    SHAInit(&hash_context);
    if(SHAUpdate(&hash_context, (unsigned char *)&pub, recsize) != SUCCESS){
	fprintf(stderr,"%s: Unable to generate hash!\n", argv[0]);
	exit(1);
    }
    if(SHAFinal(&hash_context, fingerprint) != SUCCESS){
	fprintf(stderr,"%s: Unable to generate fingerprint!\n", argv[0]);
	exit(1);
    }
    printf("Fingerprint for %s (%s) is\n", 
		mine == 0 ? argc == 2 ? argv[1] : argv[2] : "your public key",
		keytype == DSSKEYS ? "DSS" : "RSA");
    for(i=0; i<SHA_LENGTH; i++)
	if(fingerprint[i] > 0x0f)
	    printf("%x ", fingerprint[i]);
	else
	    printf("0%x ", fingerprint[i]);
    printf("\n");
    exit(0);
}

