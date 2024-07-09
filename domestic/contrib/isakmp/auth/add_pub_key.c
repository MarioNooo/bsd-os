/* 
 * Id: add_pub_key.c,v 1.1.1.1 1997/03/06 01:53:46 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/add_pub_key.c,v
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
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    union public_key rec;
    char *filename;
    int keytype;
    FILE *fp;

    if((argc != 2) && (argc != 3)){
	fprintf(stderr,"Usage: %s [RSA] <filename>\n", argv[0]);
	exit(1);
    }
    if(argc == 2){
	keytype = DSSKEYS;
	filename = argv[1];
    } else {
#ifdef NON_COMMERCIAL_AND_RSAREF
	keytype = RSAKEYS;
	filename = argv[2];
#else
	fprintf(stderr,"%s; RSAREF is for non-commercial use only.\n", argv[0]);
	exit(1);
#endif
    }
    bzero((char *)&rec, sizeof(union public_key));
    fp = fopen(filename, "r+");
    if(fp == NULL){
	fprintf(stderr,"%s: cannot open %s\n", argv[0], filename);
	exit(1);
    }
    init_siglib(keytype);
    binarify_pubkey(fp, &rec);
    if(add_pub_rec(&rec)){
	fprintf(stderr,"%s: Unable to add pub key!\n", argv[0]);
	exit(1);
    }
printf("--------\n");
asciify_pubkey(keytype, &rec);
    term_siglib();
    fclose(fp);
    exit(0);
}

