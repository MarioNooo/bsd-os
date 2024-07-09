/* 
 * Id: cookie.c,v 1.3 1997/07/24 19:41:46 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/cookie.c,v
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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#else
#include <types.h>
#endif /* MULTINET */
#ifdef FREEBSD_AND_USE_URANDOM
#include <fcntl.h>
#endif
#include <netinet/in.h>
#include "isakmp.h"
#include "toolkit.h"
#include "isadb.h"
#include "protocol.h"

/*
 * cookie.c -- cookie generation routine and "random" number supplier.
 *	"random" numbers generated using the truerand() function.
 */

unsigned char stamp[SHA_LENGTH] = { 0 };

/*
 * get a SHA_LENGTH-sized random number with "entropy" bits of randomness
 */
void get_rand(unsigned char *rand, int entropy)
{
    int i, ntimes;
    SHA_CTX context;
    unsigned long r;

#ifdef FREEBSD_AND_USE_URANDOM
    static int randfd = -1;

    if(randfd < 0){
	randfd = open("/dev/urandom", O_RDONLY, 0);
	if(randfd < 0)
	    randfd = -1;
    }
    fipsSHAInit(&context);
    if(randfd >= 0){
	char buf[256];

	int count = (entropy + 7) / 8;

	if(count > sizeof buf)
	    count = sizeof buf;
	if(read(randfd, buf, count) != count){
	    perror("reading /dev/urandom");
	    close(randfd);
	    randfd = -1;
	} else {
	    fipsSHAUpdate(&context, buf, count);
	}
    } 
    if(randfd < 0){
#else
    fipsSHAInit(&context);
#endif
	/*
	 * raw_truerand() gives about 12 bits of entropy each time
	 */
    ntimes = (entropy%12) ? (entropy/12 + 1) : entropy/12;
    for(i=0; i<ntimes; i++){
	r = raw_truerand();
	fipsSHAUpdate(&context, (unsigned char *)&r, sizeof(unsigned long));
    }
#ifdef FREEBSD_AND_USE_URANDOM
    }
#endif
    fipsSHAFinal(&context, rand);
}

/*
 * construct a stamp to appy to all cookies we bake.
 */
void gen_stamp(void)
{
    get_rand(stamp, 5);		/* 12 bits ought to be enough */
}

void gen_cookie(struct sockaddr_in *addr, unsigned char *cookie)
{
    SHA_CTX hash_context;
    unsigned char hash_result[SHA_LENGTH];
    int i;

    fipsSHAInit(&hash_context);
    if(fipsSHAUpdate(&hash_context, (unsigned char *)addr, 
		sizeof(struct sockaddr_in)) != SUCCESS){
	LOG((WARN,"Unable to create cookie (addr)"));
	return;
    }
    if(fipsSHAUpdate(&hash_context, (unsigned char *)stamp, 
			SHA_LENGTH) != SUCCESS){
	LOG((WARN,"Unable to create cookie (stamp)"));
	return;
    }
    if(fipsSHAFinal(&hash_context, hash_result) != SUCCESS){
	LOG((WARN,"Unable to create cookie (final)"));
    }
    bcopy((char *)hash_result, (char *)cookie, COOKIE_LEN);
    for(i=0; i<COOKIE_LEN; i++){
	cookie[i] ^= hash_result[COOKIE_LEN+i];
    }
    return;
}

