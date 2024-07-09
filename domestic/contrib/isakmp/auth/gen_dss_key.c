/* 
 * Id: gen_dss_key.c,v 1.2 1997/04/12 01:01:52 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/gen_dss_key.c,v
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
#include <termios.h>
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    struct hostent *me;
    struct utsname myself;
    unsigned char seed[80], keyfile[80];
    unsigned char rval[SHA_LENGTH];
    union key_pair my_sig;
    struct termios noecho, savterm;
    int fd;
    char dummy[10];

    if(uname(&myself)){
	fprintf(stderr,"%s: Unable to obtain hostname!\n", argv[0]);
	exit(1);
    }
    if((me = gethostbyname(myself.nodename)) == NULL){
	fprintf(stderr,"%s: Unable to obtain address!\n", argv[0]);
	exit(1);
    }
	/*
	 * zero out the sig to guarantee that (possibly) unused portions
	 * are part of the signature
	 */
    bzero((char *)&my_sig, sizeof(union key_pair));
    bcopy((char *)me->h_addr, (char *)&my_sig.dss_pair.pub.addr, 
			sizeof(struct in_addr));

    printf("Please enter the name to be associated with this key.\n");
    printf("It can be up to 64 characters: "); fflush(stdout);
    fgets(my_sig.dss_pair.pub.name_o_key, 64, stdin);
    do {
	printf("\nSelect a type for this name:\n\t1 = no name\n");
	printf("\t2 = Fully Qualified Domain Name (FQDN)\n");
	printf("\t3 = user at FQDN (e.g. foo@bar.com)\n");
	printf("ID type is: "); fflush(stdout);
	fgets(dummy, 10, stdin);
	my_sig.dss_pair.pub.id_type = atoi(dummy);
    } while(my_sig.dss_pair.pub.id_type < 1 || my_sig.dss_pair.pub.id_type > 3);

    if(tcgetattr(0, &noecho) < 0)
	perror("tcgetattr");
    savterm=noecho;
    noecho.c_lflag &= ~ECHO;
    do {
	/*
	 * disable terminal echo so that the random stuff isn't printed
	 * on the screen
	 */
	if(tcsetattr(0, TCSANOW, &noecho) < 0){
	    fprintf(stderr,"%s: Unable to disable terminal echo!\n", argv[0]);
	    perror("tcsetattr");
	}
	printf("enter some random stuff: "); fflush(stdout);
	fgets(seed, 80, stdin);
	if(tcsetattr(0, TCSANOW, &savterm) < 0){
	    fprintf(stderr,"%s: Unable to reset terminal echo. Oops.\n", 
			argv[0]);
	    perror("ioctl");
	}
	printf("\n"); 		/* since this was swallowed */
	if(strlen(seed) < SHA_LENGTH)
	    printf("you must enter at least %d bytes\n", SHA_LENGTH);
    } while(strlen(seed) < SHA_LENGTH);
    InitRand(SHA_LENGTH, seed, rval);

    if(GenDSSParameters(DSS_LENGTH_MIN, my_sig.dss_pair.pub.p, 
			my_sig.dss_pair.pub.q, my_sig.dss_pair.pub.g, 
			rval, NULL) != SUCCESS){
	fprintf(stderr,"%s: Unable to generate DSS parameters!\n", argv[0]);
	exit(1);
    }
    if(GenDSSKey(DSS_LENGTH_MIN, my_sig.dss_pair.pub.p, my_sig.dss_pair.pub.q, 
		my_sig.dss_pair.pub.g,
		my_sig.dss_pair.x, my_sig.dss_pair.pub.y, rval) != SUCCESS){
	fprintf(stderr,"%s: Unable to generate DSS key!\n", argv[0]);
	exit(1);
    }
    /*
     * copy DSS key pair into private ring, create (empty) public ring
     */
    sprintf(keyfile, "%s/%s.dsspriv", KEYPATH, myself.nodename);
    if((fd = creat(keyfile, 0600)) < 0){
	fprintf(stderr,"%s: Unable to create key file %s\n", argv[0], keyfile);
	exit(1);
    }
    if(write(fd, (char *)&my_sig, sizeof(struct dss_signature)) 
			!= sizeof(struct dss_signature)){
	fprintf(stderr,"%s: Unable to write to key file %s\n",argv[0],keyfile);
	exit(1);
    }
    close(fd);
    sprintf(keyfile, "%s/%s.dsspub", KEYPATH, myself.nodename);
    if((fd = creat(keyfile, 0600)) < 0){
	fprintf(stderr,"%s: Unable to create key file %s\n", argv[0], keyfile);
	exit(1);
    }
    close(fd);
    exit(0);
}

