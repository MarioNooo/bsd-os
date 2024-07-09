/*
 * Id: add_preshr_key.c,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/add_preshr_key.c,v
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "toolkit.h"
#include "authdef.h"

int main (int argc, char **argv)
{
    struct hostent *me;
    struct pre_share_key preshr;
    struct utsname myself;
    struct in_addr addr;
    struct stat sb;
    char keyfile[80];
    int fd;

    if(argc != 3){
	fprintf(stderr,"USAGE: %s <address> <key>\n", argv[0]);
	exit(1);
    }
    if(uname(&myself)){
	fprintf(stderr,"%s: Unable to obtain hostname!\n", argv[0]);
	exit(1);
    }
    if((me = gethostbyname(myself.nodename)) == NULL){
	fprintf(stderr,"%s: Unable to obtain address!\n", argv[0]);
	exit(1);
    }
    bcopy((char *)me->h_addr, (char *)&addr, sizeof(struct in_addr));
    if (inet_aton(argv[1], &preshr.addr) == 0) {
	fprintf(stderr,"%s: %s is not an IP address\n", argv[0], argv[1]);
	exit(1);
    }
    preshr.keylen = strlen(argv[2]);
    bcopy((char *)argv[2], (char *)preshr.key, preshr.keylen);

    sprintf(keyfile, "%s/%s.preshr.pub", KEYPATH, myself.nodename);
    if(stat(keyfile, &sb) < 0){
	if(errno != ENOENT){
	    fprintf(stderr,"%s: can't stat %s!\n", argv[0], keyfile);
	    exit(1);
	}
	sprintf(keyfile, "%s/%s.preshr.priv", KEYPATH, myself.nodename);
	if((fd = creat(keyfile, 0600)) < 0){
	    fprintf(stderr,"%s: Unable to preshr key file %s\n", argv[0], 
			keyfile);
	    exit(1);
	}
	if(write(fd, (char *)&addr, sizeof(struct in_addr)) != 
		sizeof(struct in_addr)){
	    fprintf(stderr,"%s: Unable to write address to %s!\n", argv[0],
			keyfile);
	    exit(1);
	}
	close(fd);
	sprintf(keyfile, "%s/%s.preshr.pub", KEYPATH, myself.nodename);
	if((fd = creat(keyfile, 0600)) < 0){
	    fprintf(stderr,"%s: Unable to preshr key file %s\n", argv[0], 
			keyfile);
	    exit(1);
	}
    } else {
	if((fd = open(keyfile, O_RDWR)) < 0){
	    fprintf(stderr, "%s: Unable to open key file %s!\n", argv[0], 
			keyfile);
	    exit(1);
	}
	lseek(fd, 0, SEEK_END);
    }
    if(write(fd, (char *)&preshr, sizeof(struct pre_share_key)) 
			!= sizeof(struct pre_share_key)){
	fprintf(stderr,"%s: Unable to write to key file %s\n",argv[0],keyfile);
	close(fd);
	exit(1);
    }
    close(fd);
    exit(0);
}

