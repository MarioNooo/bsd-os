/*
 * Id: isadb.h,v 1.3 1997/07/24 19:41:57 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/isadb.h,v
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
struct ah_proposal {
	unsigned int type;
	unsigned char transform;
	unsigned short ah_life_type;
	unsigned char *ah_life_dur;
	unsigned short ah_life_len;
	unsigned char tunnel;
};
struct esp_proposal {
	unsigned int type;
	unsigned char transform;
	unsigned char *crypt_iv;
	unsigned short crypt_ivlen;
	unsigned short esp_life_type;
	unsigned char *esp_life_dur;
	unsigned short esp_life_len;
	unsigned char tunnel;
	unsigned short hmac_alg;
};

/*
 * flags allows for logical construction of transforms. Multiple ipsec_sa's
 * will be grouped into a single proposal if their flag is OR_NEXT_TRANSFORM;
 * they will be grouped into a single sa_payload (with proposal number set
 * accordingly) if their flag is *_NEXT_PROPOSAL. If the flag is NO_RELATION,
 * than a completely different sa_payload is used to offer this ipsec_sa.
 */
union ipsec_sa {
	unsigned int type;
	struct ah_proposal ah;
	struct esp_proposal esp;
};

struct identity {
	unsigned char id_type;
	struct sockaddr_in addr;
	struct in_addr mask;
};
 
typedef struct sa_list_ {
	struct sa_list_ *next;
	int key_seq;

	unsigned long spi;
	unsigned char *inbound_crypt_key;
	unsigned int inbound_crypt_keylen;

	unsigned long other_spi;
	unsigned char *outbound_crypt_key;
	unsigned int outbound_crypt_keylen;

#define NO_RELATION		0
#define OR_NEXT_TRANSFORM	0x0001
#define OR_NEXT_PROPOSAL	0x0002
#define AND_NEXT_PROPOSAL	0x0004
	unsigned short flags;
	union ipsec_sa other_sa;
	void *parent;
} sa_list;

/*
 * For situations where PFS is necessary, the PFS_NEEDED flag is set to
 * signify this. When the other public value has been received and exponent-
 * iation done, PFS_ACCOMPLISHED is set to note that we're done.
 */
typedef struct _conn_entry {
	struct _conn_entry *next;
	unsigned char my_cookie[COOKIE_LEN];
	unsigned char his_cookie[COOKIE_LEN];
	unsigned short init_or_resp;
	unsigned long message_id;
	unsigned char *nonce_I;
	unsigned int nonce_I_len;
	unsigned char *nonce_R;
	unsigned int nonce_R_len;
	unsigned char *phase2_iv;
	unsigned short phase2_iv_len;
#define PFS_REQUESTED		0x1000
#define PFS_ACCOMPLISHED	0x2000
	ushort flags;
	struct dh_stuff dh;
	struct identity proxy_src;
	struct identity proxy_dst;
	sa_list *sa_s;
} conn_entry;

typedef struct _sa_offer {
	struct _sa_offer *next;
	unsigned char protocol;
	unsigned long spi;
	unsigned char transform;
	unsigned char *atts;
	unsigned short natts;
} sa_offer;

