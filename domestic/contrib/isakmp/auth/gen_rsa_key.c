/* 
 * Id: gen_rsa_key.c,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/gen_rsa_key.c,v
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
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "authdef.h"

int main(int argc, char **argv)
{
    struct hostent *me;
    struct utsname myself;
    union key_pair rsakeys;
    R_RSA_PROTO_KEY protokey;
    R_RANDOM_STRUCT ran;
    unsigned int modbits, mo_bytes, yea_or_nea;
    static unsigned char seed = 0;
    int fd;
    char keyfile[80], dummy[10];

    if(uname(&myself)){
	fprintf(stderr,"%s: Unable to obtain hostname!\n", argv[0]);
	exit(1);
    }
    if((me = gethostbyname(myself.nodename)) == NULL){
	fprintf(stderr,"%s: Unable to obtain address!\n", argv[0]);
	exit(1);
    }
    bzero((char *)&rsakeys, sizeof(union key_pair));
    bcopy((char *)me->h_addr, (char *)&rsakeys.rsa_pair.pub.addr, 
		sizeof(struct in_addr));
    R_RandomInit(&ran);
    while(1){
	R_GetRandomBytesNeeded(&mo_bytes, &ran);
	if(!mo_bytes)
	    break;
	R_RandomUpdate(&ran, &seed, 1);
    }
    /*
     * length of modulus is user specified. Public exponent is Fermat4 (65537)
     */
    printf("enter the number of bits for RSA key: "); fflush(stdout);
    fgets(dummy, 10, stdin);
    modbits = atoi(dummy);
    protokey.bits = modbits;
    protokey.useFermat4 = 1;
    /*
     * support separate RSA keys for encryption and signing. Both will be
     * used for authentication in ISAKMP/Oakley but they're used differently
     */
    rsakeys.rsa_pair.pub.useage = 0;
    printf("Is this key for signatures? (0=NO, 1=YES): "); fflush(stdout);
    fgets(dummy, 10, stdin);
    yea_or_nea = atoi(dummy);
    if(yea_or_nea)
	rsakeys.rsa_pair.pub.useage |= FORSIGS;

    printf("Is this key for encryption? (0=NO, 1=YES): "); fflush(stdout);
    fgets(dummy, 10, stdin);
    yea_or_nea = atoi(dummy);
    if(yea_or_nea)
	rsakeys.rsa_pair.pub.useage |= FORENCR;
    if(rsakeys.rsa_pair.pub.useage == 0){
	fprintf(stderr,"A key for nothing? I have better things to do.\n");
	exit(1);
    }
    printf("Please enter the name to be associated with this key.\n");
    printf("It can be up to 64 characters: "); fflush(stdout);
    fgets(rsakeys.rsa_pair.pub.name_o_key, 64, stdin);
    do {
        printf("\nSelect a type for this name:\n\t1 = no name\n");
        printf("\t2 = Fully Qualified Domain Name (FQDN)\n");
        printf("\t3 = user at FQDN (e.g. foo@bar.com)\n");
	printf("ID type is: "); fflush(stdout);
	fgets(dummy, 10, stdin);
	rsakeys.rsa_pair.pub.id_type = atoi(dummy);
    } while(rsakeys.rsa_pair.pub.id_type < 1 || 
	    rsakeys.rsa_pair.pub.id_type > 3);
    /*
     * It can take a helluva long time to generate large keys. Gee it
     * would be nice if RSAREF had a surrender function so periods
     * could be printed out every 10s or so. Of course, one could add one,
     * but then it's not the standard distribution and blahblahblah...
     */
    if(R_GeneratePEMKeys(&rsakeys.rsa_pair.pub.pubkey, &rsakeys.rsa_pair.priv, 
			&protokey, &ran)){
	fprintf(stderr,"Can't generate RSA key pair!\n");
	exit(1);
    }

    sprintf(keyfile, "%s/%s.rsapriv", KEYPATH, myself.nodename);
    if((fd = creat(keyfile, 0600)) < 0){
	fprintf(stderr, "%s: Unable to create private key file!\n", argv[0]);
	exit(1);
    }
    if(write(fd, (char *)&rsakeys, sizeof(struct rsa_key_pair)) != 
			sizeof(struct rsa_key_pair)){
	fprintf(stderr, "%s: Unable to write private key to file!\n", argv[0]);
	exit(1);
    }
    close(fd);

    sprintf(keyfile, "%s/%s.rsapub", KEYPATH, myself.nodename);
    if((fd = creat(keyfile, 0600)) < 0){
	fprintf(stderr, "%s: Unable to create public key file!\n", argv[0]);
	exit(1);
    }
    close(fd);
 
    exit(0); 
}

