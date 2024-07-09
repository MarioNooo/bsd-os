/* 
 * Id: authdef.h,v 1.1.1.1 1997/03/06 01:53:47 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/auth/authdef.h,v
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
#ifndef _AUTHDEF_H
#define _AUTHDEF_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "toolkit.h"
#ifdef NON_COMMERCIAL_AND_RSAREF
#define RSA_IFIED_ALREADY
#include "global.h"
#include "rsaref.h"
#include "rsa.h"
#endif

#define KEYPATH "/var/db"
#define DSSKEYS	1
#define RSAKEYS	2
#define PRESHAR 3

#define MAXCERTLEN 2048

struct dss_pub_key {
    struct in_addr addr;
    char name_o_key[64];
    unsigned int id_type;
    unsigned char p[DSS_LENGTH_MIN];
    unsigned char q[SHA_LENGTH];
    unsigned char g[DSS_LENGTH_MIN];
    unsigned char y[DSS_LENGTH_MIN];
};

struct dss_signature {
    struct dss_pub_key pub;
    unsigned char x[SHA_LENGTH];
};

struct pre_share_key {
    struct in_addr addr;
    unsigned short keylen;
    unsigned char key[128];
};

#ifdef NON_COMMERCIAL_AND_RSAREF
struct rsa_pub_key {
    struct in_addr addr;
    char name_o_key[64];
    unsigned int id_type;
    R_RSA_PUBLIC_KEY pubkey;
#define FORSIGS	0x01
#define FORENCR	0x02
    unsigned char useage;
};

struct rsa_key_pair {
    struct rsa_pub_key pub;
    R_RSA_PRIVATE_KEY priv;
};

union public_key {
    struct in_addr addr;
    struct dss_pub_key dsspub;
    struct rsa_pub_key rsapub;
    struct pre_share_key preshr;
};

union key_pair {
    struct dss_signature dss_pair;
    struct rsa_key_pair rsa_pair;
    struct in_addr myaddress;		/* preshared identity-- no priv key */
};

#else

union public_key {
    struct in_addr addr;
    struct dss_pub_key dsspub;
    struct pre_share_key preshr;
    char rsapub;
};

union key_pair {
    struct dss_signature dss_pair;
    struct in_addr myaddress;		/* preshared identity-- no priv key */
    char rsa_pair;
};

#define FORSIGS	0
#define FORENCR	0

#endif

void init_siglib(int keytype);

void term_siglib(void);

int get_priv_entry(union key_pair *myrec);

int get_pub_rec_by_addr(struct in_addr *addr, union public_key *rec, 
			unsigned char usage);

int get_pub_rec_by_name(char *hostname, union public_key *rec, 
			unsigned char useage);

int add_pub_rec(union public_key *rec);

void asciify_pubkey(int keytype, union public_key *rec);

void binarify_pubkey(FILE *fp, union public_key *rec);

int get_next_pub_rec(union public_key *rec);

int i_have_a_cert(void);

int get_my_cert(unsigned char *);

int is_valid_cert(unsigned char *, int);

void add_pub_key_from_cert(unsigned char *, int);

#endif

