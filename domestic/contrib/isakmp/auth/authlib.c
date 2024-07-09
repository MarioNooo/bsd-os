/* 
 * Id: authlib.c,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/authlib.c,v
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
#include "authdef.h"
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include "toolkit.h"

static struct utsname myself;
static short inited = 0;
static int key_type;
static int privfd;
static int pubfd;

void init_siglib(int keytype)
{
    char keyfile[80];

    if(uname(&myself)){
	fprintf(stderr,"Unable to initialize the siglib!\n");
	return;
    }
    switch(keytype){
	case DSSKEYS:
	    sprintf(keyfile, "%s/%s.dsspriv", KEYPATH, myself.nodename);
	    if((privfd = open(keyfile, O_RDONLY)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    sprintf(keyfile, "%s/%s.dsspub", KEYPATH, myself.nodename);
	    if((pubfd = open(keyfile, O_RDWR)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    sprintf(keyfile, "%s/%s.rsapriv", KEYPATH, myself.nodename);
	    if((privfd = open(keyfile, O_RDONLY)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    sprintf(keyfile, "%s/%s.rsapub", KEYPATH, myself.nodename);
	    if((pubfd = open(keyfile, O_RDWR)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    break;
#endif
	case PRESHAR:
	    sprintf(keyfile, "%s/%s.preshr.priv", KEYPATH, myself.nodename);
	    if((privfd = open(keyfile, O_RDWR)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    sprintf(keyfile, "%s/%s.preshr.pub", KEYPATH, myself.nodename);
	    if((pubfd = open(keyfile, O_RDWR)) < 0){
		fprintf(stderr,"Unable to open key file %s\n", keyfile);
		return;
	    }
	    break;
	default:
	    fprintf(stderr, "Unknown key type %d\n", keytype);
	    return;
    }
    lseek(privfd, 0, SEEK_SET);
    lseek(pubfd, 0, SEEK_SET);
    inited = 1;
    key_type = keytype;
}

void term_siglib(void)
{
    if(!inited){
	fprintf(stderr,"You've gotta init the lib before you term it!\n");
	return;
    }
    close(privfd);
    close(pubfd);
    return;
}

int get_priv_entry(union key_pair *myrec)
{
    if(!inited){
	fprintf(stderr,"You must initialize the siglib first!\n");
	return(1);
    }
    if(!myrec){
	fprintf(stderr,"NULL record in which to read\n");
	return(1);
    }
    lseek(privfd, 0, SEEK_SET);
    switch(key_type){
	case DSSKEYS:
	    if(read(privfd, myrec, sizeof(struct dss_signature)) 
			!= sizeof(struct dss_signature)){
		fprintf(stderr,"Unable to obtain priv key record\n");
		return(1);
	    }
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    if(read(privfd, myrec, sizeof(struct rsa_key_pair)) 
			!= sizeof(struct rsa_key_pair)){
		fprintf(stderr,"Unable to obtain priv key record\n");
		return(1);
	    }
	    break;
#endif
	case PRESHAR:
	    if(read(privfd, myrec, sizeof(struct in_addr))
			!= sizeof(struct in_addr)){
		fprintf(stderr,"Unable to obtain priv key record\n");
		return(1);
	    }
	    break;
    }
    return(0);
}

int get_pub_rec_by_addr(struct in_addr *addr, union public_key *rec,
			unsigned char useage)
{
    int recsize = 0;

    if(!inited){
	fprintf(stderr,"You must initialize the siglib first!\n");
	return(1);
    }
    lseek(pubfd, 0, SEEK_SET);
    switch(key_type){
	case DSSKEYS:
	    recsize = sizeof(struct dss_pub_key);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    recsize = sizeof(struct rsa_pub_key);
	    break;
#endif
	case PRESHAR:
	    recsize = sizeof(struct pre_share_key);
	    break;
    }
    while(read(pubfd, rec, recsize) == recsize){
	if(addr->s_addr == rec->addr.s_addr){
#ifdef NON_COMMERCIAL_AND_RSAREF
	    if(key_type == RSAKEYS){
		if((rec->rsapub.useage & useage) == 0)
		   continue; 
	    }
#endif
	    return(0);
	}
    }
    return(1);
}

int get_next_pub_rec(union public_key *rec)
{
    int recsize = 0;

    if(!inited){
	fprintf(stderr,"You must initialize the siglib first!\n");
	return(1);
    }
    if(!rec){
	fprintf(stderr,"NULL record to fill\n");
	return(1);
    }
    switch(key_type){
	case DSSKEYS:
	    recsize = sizeof(struct dss_pub_key);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    recsize = sizeof(struct rsa_pub_key);
	    break;
#endif
	case PRESHAR:
	    recsize = sizeof(struct pre_share_key);
	    break;
    }
    if(read(pubfd, rec, recsize) == recsize)
	return(0);
    else
	return(1);
}

int get_pub_rec_by_name(char *hostname, union public_key *rec,
			unsigned char useage)
{
    struct hostent *him;

    if((him = gethostbyname(hostname)) == NULL){
	fprintf(stderr,"Unable to obtain address for %s\n", hostname);
	return(1);
    }
    return(get_pub_rec_by_addr(((struct in_addr *)him->h_addr),rec, useage));
}

int add_pub_rec(union public_key *rec)
{
    union key_pair me;
    union public_key blah;
    int recsize = 0;

    if(!inited){
	fprintf(stderr,"You must initialize the siglib first!\n");
	return(1);
    }
    if(!rec){
	fprintf(stderr,"NULL record to add\n");
	return(1);
    }
    if(key_type == PRESHAR){
	fprintf(stderr,"Pre-shared keys are added directly\n");
	return(1);
    }
    if(get_priv_entry(&me)){
	fprintf(stderr,"Create your own key file before you add other's\n");
	return(1);
    }
    if(((key_type == DSSKEYS) && 
	(me.dss_pair.pub.addr.s_addr == rec->addr.s_addr))
#ifdef NON_COMMERCIAL_AND_RSAREF
    || ((key_type == RSAKEYS) && 
	(me.rsa_pair.pub.addr.s_addr == rec->addr.s_addr))
#endif
	){
	fprintf(stderr,"Don't add yourself to your public key file!\n");
	return(1);
    }
	/* 
	 * this will advance the file pointer to the end of the public key
	 * file without doing a stat and a lseek. Mere laziness.
	 */
    get_pub_rec_by_name(myself.nodename, &blah, (unsigned char)0);
    switch(key_type){
	case DSSKEYS:
	    recsize = sizeof(struct dss_pub_key);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    recsize = sizeof(struct rsa_pub_key);
	    break;
#endif
    }

    if(write(pubfd, rec, recsize) != recsize){
	fprintf(stderr,"Unable to add pub key record to key file!\n");
	return(1);
    }
    return(0);
}

static void print_val(unsigned char *theval, int thelen)
{
    int i, j;

    for(i=0, j=0; i<thelen; i++, j++){
	if(j == 8){
	    printf("\n");
	    j = 0;
	}
	printf(" %d ", theval[i]);
    }
    if(j)
	printf("\n");
    return;
}

static void read_val(FILE *strm, unsigned char *theval, int thelen)
{
    int i, tmp;

    bzero((char *)theval, thelen);
    for(i=0; i<thelen; i++){
	fscanf(strm, "%d", &tmp);
	theval[i] = tmp;
    }
    return;
}

void asciify_pubkey(int keytype, union public_key *rec)
{
    print_val((unsigned char *)&(rec->addr), sizeof(struct in_addr));
    switch(keytype){
	case DSSKEYS:
	    print_val((unsigned char *)rec->dsspub.name_o_key, 64);
	    print_val((unsigned char *)&rec->dsspub.id_type, 
			sizeof(unsigned int));
	    print_val(rec->dsspub.p, DSS_LENGTH_MIN);
	    print_val(rec->dsspub.q, SHA_LENGTH);
	    print_val(rec->dsspub.g, DSS_LENGTH_MIN);
	    print_val(rec->dsspub.y, DSS_LENGTH_MIN);
	    break;
#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    print_val((unsigned char *)rec->rsapub.name_o_key, 64);
	    print_val((unsigned char *)&rec->rsapub.id_type, 
			sizeof(unsigned int));
	    print_val((unsigned char *)&rec->rsapub.pubkey.bits, 
			sizeof(unsigned int));
	    print_val(rec->rsapub.pubkey.modulus, MAX_RSA_MODULUS_LEN);
	    print_val(rec->rsapub.pubkey.exponent, MAX_RSA_MODULUS_LEN);
	    print_val(&rec->rsapub.useage, sizeof(unsigned char));
	    break;
#endif
	default:
	    fprintf(stderr,"Unknown key type %d\n", key_type);
    }
    return;
}

void binarify_pubkey(FILE *fp, union public_key *rec)
{
    unsigned char dummy[1000];

    if(!inited){
	fprintf(stderr,"You must initialize the siglib first!\n");
	return;
    }
    read_val(fp, dummy, sizeof(struct in_addr));
    bcopy((char *)dummy, (char *)&(rec->addr), sizeof(struct in_addr));

    switch(key_type){
	case DSSKEYS:
	    read_val(fp, dummy, 64);
	    bcopy((char *)dummy, (char *)rec->dsspub.name_o_key, 64);
	    read_val(fp, dummy, sizeof(unsigned int));
	    bcopy((char *)dummy, (char *)&rec->dsspub.id_type, 
				sizeof(unsigned int));
	    read_val(fp, dummy, DSS_LENGTH_MIN);
	    bcopy((char *)dummy, (char *)rec->dsspub.p, DSS_LENGTH_MIN);
	    read_val(fp, dummy, SHA_LENGTH);
	    bcopy((char *)dummy, (char *)rec->dsspub.q, SHA_LENGTH);
	    read_val(fp, dummy, DSS_LENGTH_MIN);
	    bcopy((char *)dummy, (char *)rec->dsspub.g, DSS_LENGTH_MIN);
	    read_val(fp, dummy, DSS_LENGTH_MIN);
	    bcopy((char *)dummy, (char *)rec->dsspub.y, DSS_LENGTH_MIN);
	    break;

#ifdef NON_COMMERCIAL_AND_RSAREF
	case RSAKEYS:
	    read_val(fp, dummy, 64);
	    bcopy((char *)dummy, (char *)rec->rsapub.name_o_key, 64);
	    read_val(fp, dummy, sizeof(unsigned int));
	    bcopy((char *)dummy, (char *)&rec->rsapub.id_type, 
				sizeof(unsigned int));
	    read_val(fp, dummy, sizeof(unsigned int));
	    bcopy((char *)dummy, (char *)&rec->rsapub.pubkey.bits,
			sizeof(unsigned int));
	    read_val(fp, dummy, MAX_RSA_MODULUS_LEN);
	    bcopy((char *)dummy, (char *)rec->rsapub.pubkey.modulus, 
			MAX_RSA_MODULUS_LEN);
	    read_val(fp, dummy, MAX_RSA_MODULUS_LEN);
	    bcopy((char *)dummy, (char *)rec->rsapub.pubkey.exponent,
			MAX_RSA_MODULUS_LEN);
	    read_val(fp, dummy, sizeof(unsigned char));
	    bcopy((char *)dummy, (char *)&rec->rsapub.useage,
			sizeof(unsigned char));
	    break;
#endif
	default:
	    fprintf(stderr,"Unknown key type %d\n", key_type);
    }
    return;
}

