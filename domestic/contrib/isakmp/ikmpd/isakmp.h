/* 
 * Id: isakmp.h,v 1.3 1997/07/24 19:41:59 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/isakmp.h,v
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
#ifndef _ISAKMP_H_
#define _ISAKMP_H_

#ifdef MULTINET
#define i386
#include "param.h"
extern int new_tcp_iss();
#define raw_truerand() new_tcp_iss()
#endif /* MULTINET */
#include "toolkit.h"

	/* the version number */

#define ISAKMP_MAJOR_VERSION 	1
#define ISAKMP_MINOR_VERSION 	0

	/* ISAKMP well-known port */

#define IKMP_PORT		500

	/* message types */

#define ISA_SA			1
#define ISA_PROP		2
#define ISA_TRANS		3
#define ISA_KE			4
#define ISA_ID			5
#define ISA_CERT		6
#define ISA_CERT_REQ		7
#define ISA_HASH		8
#define ISA_SIG			9
#define ISA_NONCE		10
#define ISA_NOTIFY		11
#define ISA_DELETE		12

	/* Oakley Modes */

#define OAK_MM			2	/* ISAKMP ID protect */
#define OAK_AG			4	/* ISAKMP Aggressive */
#define OAK_QM			32
#define OAK_NG			33
#define ISAKMP_INFO		5

	/* Oakley states */

#define OAK_MM_NO_STATE		0
#define OAK_MM_SA_SETUP		1
#define OAK_MM_KEY_EXCH		2
#define OAK_MM_KEY_AUTH		3

#define OAK_AG_NOSTATE		4
#define OAK_AG_INIT_EXCH	5
#define OAK_AG_AUTH		6

#define OAK_QM_SA_ACCEPT	7
#define OAK_QM_AUTH_AWAIT	8
#define OAK_QM_IDLE		9

	/* Protocol Numbers */

#define PROTO_ISAKMP		1
#define PROTO_IPSEC_AH		2
#define PROTO_IPSEC_AH_MASK	0x0001
#define PROTO_IPSEC_ESP		3
#define PROTO_IPSEC_ESP_MASK	0x0002

	/* DOI Numbers */

#define IPSEC_DOI		1

	/* Encryption Transform numbers */

#define ET_DES_CBC		1
#define ET_IDEA_CBC		2
#define ET_BLOWFISH_CBC		3 

	/* misc lengths */

#define CYPHER_BLOCKLEN		8  /* same for DES and puffy-- 64 bits */
#define DES_KEYLEN		8
#define BLOWFISH_KEYLEN		56
#define COOKIE_LEN		8

	/* Hash numbers */

#define HASH_MD5		1
#define HASH_SHA		2
#define HASH_TIGER		3

	/* Authentication Methods */

#define PRESHRD			1
#define DSS_SIG			2
#define RSA_SIG			3
#define RSA_ENC			4

	/* IPsec DOI Identification Types */

#define ID_IPV4_ADDR		1
#define ID_FQDN			2
#define ID_USER_FQDN		3
#define ID_IPV4_ADDR_SUBNET	4
#define ID_IPV6_ADDR		5
#define ID_IPV6_ADDR_SUBNET	6
#define ID_IPV4_ADDR_RANGE	7
#define ID_IPV6_ADDR_RANGE	8 
#define ID_DER_ASN1_DN		9
#define ID_DER_ASN1_GN		10

	/* AH and ESP attribute classes */

#define IPSEC_SA_LIFE_TYPE	1
#define IPSEC_SA_LIFE_DURATION	2
#define IPSEC_GROUP_DESC	3
#define IPSEC_ENCAPSULATION_MODE 4
#define IPSEC_HMAC_ALG		5
#define IPSEC_KEY_LENGTH	6
#define IPSEC_KEY_ROUNDS	7
#define IPSEC_COMP_DICT_SIZE	8
#define IPSEC_COMP_PRIV_ALG	9

#define IPSEC_LIFE_SEC		1
#define IPSEC_LIFE_KB		2

	/* IPsec DOI magic numbers */

#define AH_MD5_KPDK		1
#define AH_MD5_HMAC		2
#define AH_SHA_HMAC		3

#define ESP_DES_CBC		1
#define ESP_DES			2
#define ESP_3DES		3

#define ESP_HMAC_ALG_MD5	1
#define ESP_HMAC_ALG_SHA	2

	/* Oakley attribute classes  */

#define OAK_ENCR_ALG		1
#define OAK_HASH_ALG		2
#define OAK_AUTH_METHOD		3
#define OAK_GROUP_DESC		4
#define OAK_GROUP_TYPE		5
#define OAK_PRIME_P		6
#define OAK_GENERATOR_G1	7
#define OAK_GENERATOR_G2	8
#define OAK_CURVE_A		9
#define OAK_CURVE_B		10
#define OAK_LIFE_TYPE		11
#define OAK_LIFE_DUR		12
#define OAK_PRF			13
#define OAK_KEY_LENGTH		14

	/* types of SA life */

#define OAK_LIFE_SEC		1
#define OAK_LIFE_KB		2

	/* path of isakmp messages: uni-directional or bi-directional */

#define BIDIR			0
#define UNIDIR			1

	/* Group Descriptors Identifiers */

#define GP_MODP				1
#define GP_ECP				2

        /* Oakley Group Descriptors */
 
struct modp_group {
  unsigned char type;
  unsigned short prime_len;
  unsigned char *p;
  unsigned short gen_len;
  unsigned char *g;
};
struct ecp_group {
  unsigned char type;
  unsigned short prime_len;
  unsigned char *p;
  unsigned short cp1_len;
  unsigned char *curve_p1;
  unsigned short cp2_len;
  unsigned char *curve_p2;
  unsigned short gen1_len;
  unsigned char *g1;
  unsigned short gen2_len;
  unsigned char *g2;
};

struct dh_stuff {
    union {
        unsigned char type;
        struct modp_group modp;
        struct ecp_group ecp;
    } group;
    unsigned short group_desc;
    unsigned short dh_len;
    unsigned char *DH_priv_val;
    unsigned char *DH_pub_val;
    unsigned char *his_DH_pub_val;
    unsigned char *g_to_xy;
};

typedef struct _identity {
  unsigned char id_type;
  unsigned char protocol;
  unsigned short port;
  struct in_addr ipaddr;
} identity;

typedef void (*hash_fcn)();

	/* a security association for oakley-isakmp (not ipsec) */

typedef struct SA_ {
  struct SA_ *next;
  /*
   * connection info: buffers, addresses, direction of communication
   */
  int fd;
  unsigned char *packet;
  unsigned short packet_len;
  unsigned char  send_dir;
  unsigned char *payload;
  unsigned short payload_len;
  unsigned char  condition;
  struct sockaddr_in dst;
  struct sockaddr_in src;
  short err_counter;
  short init_or_resp;		/* protocol specific direction (may change) */
  short in_or_out;		/* message specific direction (never changes) */
  unsigned short outstanding_sa;
  short to_id;
#define SA_COND_LARVAL	0
#define SA_COND_ALIVE	1
#define SA_COND_DEAD	2
  /*
   * ISAKMP SA policy and state information
   */
  unsigned char my_cookie[COOKIE_LEN];
  unsigned char his_cookie[COOKIE_LEN];

  identity opposing_id;
  struct sockaddr_in proxy_dst;

  unsigned char *nonce_I;
  unsigned short nonce_I_len;
  unsigned char *nonce_R;
  unsigned short nonce_R_len;
  unsigned char *sap_offered;
  unsigned short sap_len;

  unsigned char *skeyid;
  unsigned char *skeyid_d;
  unsigned char *skeyid_a;
  unsigned char *skeyid_e;
  unsigned short skeyid_len;

  unsigned char *crypt_key;
  unsigned char *crypt_iv;
  unsigned short crypt_iv_len;
  unsigned short state_mask;
  unsigned short state;
	/* 
	 * required for Oakley
	 */
  unsigned int encr_alg;
  unsigned int auth_alg;
  unsigned int hash_alg;
	/*
	 * hash functions in native and hmac mode 
	 */
  hash_fcn InitHash;
  hash_fcn UpdHash;
  hash_fcn FinHash;
  hash_fcn Inithmac;
  hash_fcn Updhmac;
  hash_fcn Finhmac;
  struct dh_stuff dh;
} SA;
	/* ISAKMP header */

typedef struct isakmp_hdr_ {
  unsigned char init_cookie[COOKIE_LEN];
  unsigned char resp_cookie[COOKIE_LEN];
  unsigned char next_payload;
#ifdef i386
  unsigned char minver:4,
		majver:4;
#else
  unsigned char majver:4,
		minver:4;
#endif
  unsigned char exch;
#define ISAKMP_HDR_ENCR_BIT	0x0001
#define ISAKMP_HDR_COLL_BIT	0x0002
  unsigned char flags;
  unsigned long mess_id;
  unsigned long len;
} isakmp_hdr;

	/* SA payload */

typedef struct sa_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned long doi;
  unsigned long situation;
} sa_payload;

	/* proposal payload */

typedef struct proposal_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned char prop_num;
  unsigned char protocol_id;
  unsigned char spi_size;
  unsigned char num_transforms;
} proposal_payload;

	/* transform payload */

typedef struct trans_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned char trans_num;
  unsigned char trans_id;
  unsigned short reserved2;
} trans_payload;

	/* attribute */

typedef struct sa_attribute_ {
#ifdef i386
  unsigned short att_class:15,
	 	type:1;
#else
  unsigned short type:1,
		att_class:15;
#endif
  union {
	unsigned short basic_value;
	unsigned short vpi_length;
  } att_type;
} sa_attribute;

typedef struct generic_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
} generic_payload;

typedef struct id_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned char id_type;
  unsigned char protocol;
  unsigned short port;
} id_payload;

typedef struct key_x_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
} key_x_payload;

typedef struct sig_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
} sig_payload;

typedef struct cert_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned char cert_spec;		/* encoding or type */
} cert_payload, cert_req_payload;

typedef struct nonce_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
} nonce_payload;

typedef struct hash_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
} hash_payload;

	/* ISAKMP Notification Message */

typedef struct notify_payload_ {
  unsigned char next_payload;
  unsigned char reserved;
  unsigned short payload_len;
  unsigned long doi;
  unsigned char protocol_id;
  unsigned char spi_size;
  unsigned short notify_message;
} notify_payload;

	/* ISAKMP SA Delete Message */

typedef struct delete_payload_ {
  unsigned char next_payload;
  unsigned char reserved1;
  unsigned short payload_len;
  unsigned long doi;
  unsigned char protocol_id;
  unsigned char spi_size;
  unsigned short nspis;
} delete_payload;

	/*
	 * notify messages
	 */
#define INVALID_PAYLOAD			1
#define DOI_NOT_SUPPORTED		2
#define SITUATION_NOT_SUPPORTED		3
#define INVALID_COOKIE			4
#define INVALID_MAJOR_VERSION		5
#define INVALID_MINOR_VERSION		6
#define INVALID_EXCHANGE_TYPE		7
#define INVALID_FLAGS			8
#define INVALID_MESSAGE_ID		9
#define INVALID_PROTOCOL_ID		10
#define INVALID_SPI			11
#define INVALID_TRANSFORM_ID		12
#define ATTRIBUTES_NOT_SUPPORTED	13
#define NO_PROPOSAL_CHOSEN		14
#define PROPOSAL_BAD_SYNTAX		15
#define PAYLOAD_MALFORMED		16
#define INVALID_KEY_INFO		17
#define INVALID_ID_INFO			18
#define INVALID_CERT_ENCODING		19
#define INVALID_CERT			20
#define BAD_CERT_REQ_SYNTAX		21
#define INVALID_CERT_AUTH		22
#define INVALID_HASH_INFO		23
#define AUTH_FAILED			24
#define INVALID_SIGNATURE		25
#define DECRYPTION_FAILED		26
	/*
	 * results of richardson
	 */
#define LIKE_HELLO			30000
#define when_would_THAT_ever_be_right	30001
#define SHUT_UP				30002

#define CONNECTED			16385

#endif

