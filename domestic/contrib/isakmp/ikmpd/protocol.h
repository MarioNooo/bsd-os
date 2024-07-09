/* 
 * Id: protocol.h,v 1.3 1997/07/24 19:42:07 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/protocol.h,v
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
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_
#include <stdarg.h>
#include <syslog.h>
#ifdef MULTINET
#include <in.h>
#endif /* MULTINET */

	/*
	 * various lb-defines and function prototypes
	 */
#define CHUNK 10
#define DESKEYLEN 64

#ifndef ROUNDUP
#define ROUNDUP(x) \
	((x) > 0 ? (1 + (((x) -1) | (sizeof(long) - 1))) : sizeof(long))
#endif

#define NIXSTATE { bzero((char *)my_cookie, sizeof(my_cookie)); \
		   bzero((char *)his_cookie, sizeof(his_cookie)); \
		   bzero((char *)&security_assoc, sizeof(SA)); }

#define IS_STATE_SET(m, b) ((m)->state_mask & (1 << (b)))
#define SET_STATE(m, b) ((m)->state_mask |= (1 << (b)))
#define IS_STATE(m, b) ((m)->state_mask & b)

	/* ISAKMP SA protocol and message directions */

#define INITIATOR 1
#define RESPONDER 0
#define OUTBOUND 1
#define INBOUND 0

	/* connection state masks */

#define CONST_SA 	0x0001
#define PROC_SA		0x0002
#define CONST_KE	0x0004
#define PROC_KE		0x0008
#define CONST_AUTH	0x0010
#define PROC_AUTH	0x0020
#define CONST_NEG	0x0040
#define PROC_NEG	0x0080

	/* connection states */

#define INITIATED	0x0003
#define KEY_EXCHANGED 	0x000c
#define AUTHENTICATED	0x0030
#define NEGOTIATED 	0x00c0
#define STUFFED_SAS	0x0100
#define CRYPTO_ACTIVE   0x1000
#define CRYPTO_DELAY	0x2000

#ifdef DEBUG
#ifdef MULTINET
#define LOG(x)
#else
#define LOG(x)		fprintf x;fprintf(stderr,"\n");
#endif /* MULTINET */
#define DEB		stderr
#define WARN		stderr
#define ERR		stderr
#define CRIT		stderr
#else
#ifdef MULTINET
#ifdef __VXD
#define LOG(x)
#else
#define LOG(x)		tgv_log x;
#endif /* __VXD */
#else
#define LOG(x)		syslog x;
#endif /* MULTINET */

#define DEB		LOG_DEBUG
#define WARN		LOG_WARNING
#define ERR		LOG_ERR
#define CRIT		LOG_CRIT
#endif

void throw (
	SA *sa
);

SA *responder (
	isakmp_hdr *hdr,
	struct sockaddr_in *from,
	int fd,
	unsigned char *payload
);

SA *initiator (
	struct sockaddr_in *dst,
	struct sockaddr_in *src,
	struct sockaddr_in *proxy_dst,
	unsigned short type
);

int init_construct_init(
	SA *sa
);

void load_sa(
	SA *secassoc,
	sa_list *node,
	conn_entry *centry,
	int direction
);

void get_unique_spi(
	SA *sa,
	int seq_num,
	int type
);

void init_sa_utils(
	unsigned char *payload,
	int nprops,
	int natts
);

int set_payload(void);

void try_again(
	SA *sa
);

int set_attributes(
	unsigned char att_class, 
	unsigned char *att_types, 
	int natts,
	int more_atts
);

int find_atts(
	unsigned char proposal, 
	unsigned int att_class,
	unsigned char *att_types, 
	int natts
);

void gen_cookie(
	struct sockaddr_in *dst,
	unsigned char *cookie
);

void gen_stamp(
	void
);

int set_oakley_atts(
	SA *sa,
	unsigned char **payload, 
	int *payload_len
);

int set_transform(
	SA *sa,
	int fixpos,
	int *pos,
	unsigned char nextp,
	unsigned char transnum,
	unsigned char transform,
	unsigned char *atts,
	int attslen
);

int set_proposal(
	SA *sa,
	int fixpos,
	int *pos,
	unsigned char protocol,
	sa_list *ipsec_sa_list,
	unsigned short propnum
);

int set_ipsec_proposals(
	SA *sa,
	int fixpos,
	int *pos,
	sa_list *ipsec_sa_list
);

int check_trans_atts(
	unsigned char *ptr,
	unsigned short len,
	unsigned char trans,
	unsigned char atts[][2]
);

int check_prop(
	unsigned char *ptr,
	unsigned char protocol,
	unsigned short ntrans,
	unsigned char *trans,
	unsigned char **atts,
	unsigned short *natts
);

int check_proposal(
	unsigned char *ptr,
	sa_offer *offer,
	int mess_id
);

int dh_group_find(
	int group_desc, 
	struct dh_stuff *dh
);

int dh_modp_set(
	int modp_indx, 
	unsigned char **p, 
	unsigned short *plen, 
	unsigned char **g, 
	unsigned short *glen
);

int oakley_atts_to_sa(
	unsigned char *ptr, 
	int len, 
	SA *sa
);

int ah_esp_atts_to_sa(
	SA *sa,
	sa_offer *offer,
	unsigned long message_id
);

void print_vpi(
	char *str,
	unsigned char *vpi, 
	int vpi_len
);

void get_rand(
	unsigned char *rand,
	int entropy
);

#ifndef MULTINET
unsigned long raw_truerand(
	void
);
#endif /* MULTINET */

void gen_modp_params(
	unsigned char **p, 
	int *plen, 
	unsigned char **g, 
	int *glen);

void EnPuffy(
	unsigned char *inbuf, 
	unsigned char *outbuf, 
	int len
);

void DePuffy(
	unsigned char *inbuf, 
	unsigned char *outbuf, 
	int len
);

int InitializePuffy(
	char *key, 
	int keylength, 
	int randnum
);

int verify_hdr(
	isakmp_hdr *hdr,
	SA *sa
);

int validate_payload(
	int total_length, 
	unsigned char payload_type,
	unsigned char *payload,
	int *actual_length
);

SA *get_sa_by_seq(
	int seq
);

void set_other_spi_by_mess_id(
	SA *sa,
	unsigned long mess_id,
	unsigned int protool,
	unsigned long other_spi
);

int add_seq_to_list(
	SA *sa
);

void remove_seq_from_list(
	int seq
);

#ifdef i386

void hton_hdr(
	isakmp_hdr *hdr
);
void ntoh_hdr(
	isakmp_hdr *hdr
);

void hton_sa(
	sa_payload *sap
);
void ntoh_sa(
	sa_payload *sap
);

void hton_dummy(
	generic_payload *dummy
);
void ntoh_dummy(
	generic_payload *dummy
);

void hton_key(
	key_x_payload *key
);
void ntoh_key(
	key_x_payload *key
);

void hton_sig(
	sig_payload *sig
);
void ntoh_sig(
	sig_payload *sig
);

void hton_nonce(
	nonce_payload *nonce
);
void ntoh_nonce(
	nonce_payload *nonce
);

void hton_id(
	id_payload *id
);
void ntoh_id(
	id_payload *id
);

void hton_notify(
	notify_payload *notify
);
void ntoh_notify(
	notify_payload *notify
);

void hton_hash(
	hash_payload *hash
);
void ntoh_hash(
	hash_payload *hash
);

void hton_proposal(
	proposal_payload *prop
);
void ntoh_proposal(
	proposal_payload *prop
);

void hton_att(
	sa_attribute *att
);
void ntoh_att(
	sa_attribute *att
);

void ntoh_delete(
	delete_payload *del
);

void hton_delete(
	delete_payload *del
);

void hton_payload(
	char *p,
	int len
);

void ntoh_transform(
	trans_payload *trans
);

void hton_transform(
	trans_payload *trans
);

#define hton_cert(x)
#define ntoh_cert(x)

#define ntoh_payload(x, y)	hton_payload(x, y)

#else

#define hton_hdr(x)
#define ntoh_hdr(x)

#define hton_init(x)
#define ntoh_init(x)

#define hton_auth(x)
#define ntoh_auth(x)

#define hton_key(x)
#define ntoh_key(x)

#define hton_proposal(x)
#define ntoh_proposal(x)

#define hton_att(x)
#define ntoh_att(x)

#define hton_sig(x)
#define ntoh_sig(x)

#define hton_nonce(x)
#define ntoh_nonce(x)

#define hton_id(x)
#define ntoh_id(x)

#define hton_notify(x)
#define ntoh_notify(x)

#define hton_cert(x)
#define ntoh_cert(x)

#define hton_hash(x)
#define ntoh_hash(x)

#define hton_payload(x, y)
#define ntoh_payload(x, y)

#define ntoh_dummy(x)
#define hton_dummy(x)

#define ntoh_sa(x)
#define hton_sa(x)

#define ntoh_delete(x)
#define hton_delete(x)

#define ntoh_transform(x)
#define hton_transform(x)

#endif

SA *isadb_create_entry(
	struct sockaddr_in *addr,
	short init_or_resp,
	unsigned short type
);

SA *isadb_get_entry(
	isakmp_hdr *hdr,
	struct sockaddr_in *from
);

SA *isadb_get_entry_by_addr(
	struct sockaddr_in *to
);

int isadb_get_seq(
	void
);

int create_conn_entry(
	SA *sa, 
	unsigned long message_id
);

void isadb_delete_entry(
	SA *delme
);

void isadb_delete_conn_entry(
	SA *sa, 
	unsigned long message_id, 
	int flush
);

void delete_sa_offer(
	sa_offer *offer
);

int isadb_is_outstanding_init_req(
	isakmp_hdr *hdr, 
	struct sockaddr_in *src
);

int isadb_is_outstanding_kernel_req(
	struct sockaddr_in *dst,
	unsigned short type
);

void isadb_free_all(
	SA *sa
);

void isadb_delete_all(
	SA *sa
);

void reap_db(
	void
);

int set_spi_by_seq(
	int seq, 
	unsigned long spi
);

int set_nonce_by_mess_id(
	unsigned long mess_id, 
	unsigned char *nonce, 
	int nonce_len, 
	int init_or_resp
);

int set_to_starve(
	SA *sa,
	unsigned long message_id
);

int delay_conn_entry_deletion(
	SA *sa,
	unsigned long message_id
);

int still_need_spi(
	SA *sa,
	unsigned long message_id
);

sa_list *get_sa_list(
	SA *sa,
	unsigned long message_id
);

sa_list *get_sa_by_protocol(
	SA *sa, 
	unsigned char proto_id,
	unsigned long message_id
);

conn_entry *get_centry_by_mess_id(
	unsigned long message_id
);

int add_sa_to_list(
	SA *sa, 
	unsigned char proto_id, 
	unsigned long mess_id
);

void gen_skeyid(
	SA *sa
);

void expand_packet(
	SA *sa,
	int pos,
	int nbytes
);

int encrypt_payload(
	SA *sa,
	conn_entry *centry,
	unsigned char **payload,
	short offset,
	unsigned short *len
);

int decrypt_payload(
	SA *sa,
	conn_entry *centry,
	unsigned char *payload,
	int len
);

#ifndef MULTINET
void delay_crypto_active(
	SA *sa
);
#endif /* MULTINET */

void delay_idle_qm_transition(
	SA *sa
);

void kernel_delete_spi(
	SA *sa,
	unsigned long spi,
	int type
);

int compute_quick_mode_iv(
	SA *sa, 
	unsigned long message_id
);

int process_err(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_err(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int process_init(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int process_ke(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_ke(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int construct_pfs_ke(
	SA *sa,
	conn_entry *centry,
	unsigned char nextp,
	int *pos
);

int process_cert(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_cert(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int process_sig(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_sig(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int process_hash(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_hash(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int construct_blank_hash(
	SA *sa,
	unsigned char nextp,
	int *pos
);

int construct_qm_hash(
	SA *sa,
	unsigned long mess_id,
	uchar is_notify
);

int verify_qm_hash(
	SA *sa,
	isakmp_hdr *hdr,
	unsigned char **pptr,
	unsigned char *nextp,
	unsigned long message_id
);

int compute_hash(
	SA *sa, 
	unsigned char *hash, 
	int mine_or_his
);

int process_delete(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int process_noop(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

void oakley_begin_qm(
	SA *sa
);

int oakley_const_qm(
	SA *sa, 
	unsigned long mess_id
);

int gen_qm_key(
	SA *sa, 
	unsigned char *nonce_I,
	unsigned short nonce_I_len,
	unsigned char *nonce_R,
	unsigned short nonce_R_len,
	sa_list *node,
	int direction
);

int oakley_process_main_mode(
	isakmp_hdr *hdr,
	SA *sa
);

int oakley_process_aggressive(
	isakmp_hdr *hdr,
	SA *sa
); 

int oakley_process_quick_mode(
	isakmp_hdr *hdr, 
	SA *sa
);

int process(
	unsigned char paktype, 
	SA *sa, 
	unsigned char *payload,
	unsigned long mess_id
);

int process_nonce(
	SA *sa, 
	unsigned char *payload,
	unsigned long mess_id
);

int construct_nonce(
	SA *sa, 
	unsigned char nextp, 
	int *pos
);

int construct_ipsec_nonce(
	SA *sa,
	unsigned long mess_id,
	unsigned char nextp,
	int *pos
);

int process_sa(
	SA *sa, 
	unsigned char *payload,
	unsigned long mess_id
);

int construct_isakmp_sa(
	SA *sa, 
	unsigned char nextp, 
	int *pos
);

int construct_ipsec_sa(
	SA *sa, 
	sa_list *node,
	int *pos
);

int process_id(
	SA *sa, 
	unsigned char *payload,
	unsigned long mess_id
);

int construct_id(
	SA *sa, 
	unsigned char nextp, 
	int *pos
);

int construct_proxy_id(
	SA *sa, 
	struct identity *whom, 
	uchar nextp, 
	int *pos
);

int obtain_id(
	SA *sa,
	struct in_addr *addr
);

void construct_header(
	SA *sa, 
	unsigned char exch, 
	unsigned long mess_id,
	unsigned char nextp
);

int process_notify(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_notify(
	SA *sa, 
	unsigned char nextp, 
	unsigned short mess_type, 
	void *data, 
	int data_len, 
	int *pos
);

int process_delete(
	SA *sa,
	unsigned char *payload,
	unsigned long mess_id
);

int construct_delete(
	SA *sa, 
	unsigned char nextp, 
	int *pos
);

#ifdef DEBUG
void print_hdr(
	isakmp_hdr *hdr
);

void print_vpi(
	char *str,
	unsigned char *vpi,
	int vpi_len
);
#endif

#endif

