/* 
 * Id: ikmpd.c,v 1.3 1997/07/24 19:41:54 dharkins Exp
 * Source: /nfs/cscbz/cryptocvs/isakmp/ikmpd/ikmpd.c,v
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
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#ifndef KEY2
#include <netsec/ipsec.h>
#include <netkey/key.h>
#else
#include <net/pfkeyv2.h>
#include <sys/uio.h>
#endif
#include <net/if.h>
#include <ifaddrs.h>
#include "service.h"
#include "isakmp.h"
#include "isadb.h"
#include "protocol.h"

/*
 * isakmp.c -- main driver for ikmpd.
 */
service_context context;
int is, ks;
pid_t my_pid;
static int send_request (SA *secass);

/*
 * easier than changing the bb service model to allow 2 "data" fields
 * in a timeout and passing a NULL for the 2nd when not applicable, etc.
 */
struct sa_mess_id_blob {
    SA *sa;
    unsigned long mess_id;
};

/*
 * timer functions and their wrappers to the outside world
 */

static void crypto_active_timer (int id, caddr_t data)
{
    SA *sa = (SA *)data;

    gen_skeyid(sa);
    sa->state_mask |= CRYPTO_ACTIVE;
}
void delay_crypto_active (SA *sa)
{
	/*
	 * we don't care about the id because we're not going to want to
	 * cancel this. We just want to guarantee that we go crypto active
	 * _after_ we've sent out the packet. (see ke.c)
	 */
    bb_add_timeout(context, 100000, crypto_active_timer, (caddr_t)sa);
}

static void transition_to_qm (int id, caddr_t data)
{
    SA *sa = (SA *)data;

    oakley_begin_qm(sa);
}
void delay_idle_qm_transition (SA *sa)
{
	/*
	 * wait a 1/3 second to guarantee that the kernel has sent
	 * all requests for keys out. The kernel sends requests serially,
	 * but we want to group them together and do as few QM's as possible.
	 */
    bb_add_timeout(context, 333333, transition_to_qm, (caddr_t)sa);
}

/*
 * every 5 minutes go through the database and remove dead entries
 */
static void reaper (int id, caddr_t data)
{
    reap_db();
    bb_add_timeout(context, 300000000, reaper, data);
}

static void timed_out (int id, caddr_t data)
{
    SA *secass = (SA *)data;
    int pos;

	/* check secass->err_counter. If > 4 delete SA */

#ifdef DEBUG
    printf("message not received! Retransmitting!\n");
#endif
    if(secass->err_counter > 4){
#ifdef DEBUG
	printf("too many errors! Deleting this SA\n");
#endif
	/*
	 * The other guy has stopped listening to us. Delete all potential
	 * IPsec SAs associated with this ISAKMP SA. Out of courtesy, send
	 * the other guy a delete for this ISAKMP SA-- he probably won't get
	 * it but we're good citizens. Reaper destroys this SA.
	 */
#ifdef notdef
	/*
	 * Don't call isadb_delete_entry().  We already set the
	 * condition to SA_COND_DEAD so that Reaper will destroy
	 * the SA.  Doing it here means that we are then referencing
	 * freed memory, since isadb_delete_entry() frees secass.
	 */
	isadb_delete_entry(secass);
#endif /* notdef */
	if(secass->packet_len){
	    free(secass->packet);
	    secass->packet_len = 0;
	}
	pos = sizeof(isakmp_hdr);
	construct_header(secass, 0, 0, ISA_DELETE);
	construct_delete(secass, 0, &pos);
	secass->send_dir = UNIDIR;
	ntoh_hdr((isakmp_hdr *)secass->packet);
	send_request(secass);
	secass->condition = SA_COND_DEAD;
    } else {
	send_request(secass);
	secass->err_counter++;
    }
    return;
}

static int send_request (SA *secass)
{
    int n, fd;
    struct sockaddr *addr;

	/* 
	 * these are the source and destination of the security association
	 * we're negoatiating. Therefore they're reversed for the receiver--
	 * he _sends_ to the "src".
	 */
    if(secass->in_or_out == OUTBOUND)
	addr = (struct sockaddr *)&secass->dst;
    else
	addr = (struct sockaddr *)&secass->src;
    if ((fd = secass->fd) <= 0)
	fd = is;
    if((n = sendto(fd, (char *)secass->packet, secass->packet_len, 0, addr, 
					sizeof(struct sockaddr))) < 0){
	LOG((ERR,"Failed to send %d bytes!", secass->packet_len));
	perror("sendto");
	return(n);
    }
	/*
	 * if we expect a reply, set a timer for 8 seconds. This amount of
	 * time is a guess and should be modified once statistics are
	 * generated on packet loss and round trip + processing times across
	 * the Internet.
	 */
    if(secass->send_dir == BIDIR)
	secass->to_id = bb_add_timeout(context, 30000000, timed_out, 
					(caddr_t)secass);
    return(n); 
}

/*
 * handler to use if the last message was garbled and resulted in a notify
 * message. What to do but try again?
 */
void try_again (SA *sa)
{
    int pos;

    bb_rem_timeout(context, sa->to_id);
    if(sa->err_counter > 4){
	/*
	 * make sure that we're not getting chatter on something that we
	 * know has gone bad already; avoid a notify battle.
	 */
	if(sa->condition == SA_COND_DEAD){
	    return;
	}
#ifdef DEBUG
	printf("too many errors! Deleting this SA\n");
#endif
	/*
	 * This SA has gone bad. Delete all outstanding IPsec SA's, reconstruct
	 * it's packet to be a delete message and tell the other party that
	 * this is bad. Let reaper do actual distruction.
	 */
#ifdef notdef
	/*
	 * Don't call isadb_delete_entry().  We already set the
	 * condition to SA_COND_DEAD so that Reaper will destroy
	 * the SA.  Doing it here means that we are then referencing
	 * freed memory, since isadb_delete_entry() frees secass.
	 */
	isadb_delete_entry(sa);
#endif /* notdef */
	if(sa->packet_len){
	    free(sa->packet);
	    sa->packet_len = 0;
	}
	pos = sizeof(isakmp_hdr);
	construct_header(sa, 0, 0, ISA_DELETE);
	construct_delete(sa, 0, &pos);
	sa->send_dir = UNIDIR;
	ntoh_hdr((isakmp_hdr *)sa->packet);
	send_request(sa);
	sa->condition = SA_COND_DEAD;
    } else {
	sa->err_counter++;
	send_request(sa);
    }
    return;
}
 
static int get_bytes (int fd, unsigned char *data, int len,
		struct sockaddr *from, int *fromlen)
{
    int n;

    *fromlen = sizeof(struct sockaddr);
    n=recvfrom(fd, (char *)data, len, 0, from, fromlen);
    return(n);
}

/*
 * delete a IPsec SA from the kernel identified by the spi
 */
void kernel_delete_spi (SA *sa, unsigned long spi, int type)
{
#ifndef KEY2
    int datalen;
    struct key_msghdr keymhdr;
    unsigned char *keymdata, *ptr;
#else /* KEY2 */
    struct {
      struct sadb_msg msg;
      struct sadb_sa ssa;
      struct {
        struct sadb_address sad;
        struct sockaddr_in sin;
      } sad[2];
    } msg;
#endif /* KEY2 */

#ifndef KEY2
    datalen = sizeof(struct key_msghdr) + (3 * sizeof(struct sockaddr_in));
    keymdata = (unsigned char *) malloc (datalen * sizeof(unsigned char));
    if(keymdata == NULL){
	LOG((CRIT,"Out Of Memory"));
	return;
    }
    bzero((char *)keymdata, datalen);

    bzero((char *)&keymhdr, sizeof(struct key_msghdr));
    keymhdr.key_msgvers = KEY_VERSION;
    keymhdr.key_msgtype = KEY_DELETE;
    keymhdr.key_msglen = datalen;
    keymhdr.key_pid = my_pid;
    keymhdr.spi = spi;
    bcopy((char *)&keymhdr, (char *)keymdata, sizeof(struct key_msghdr));
    ptr = keymdata + sizeof(struct key_msghdr);
    bcopy((char *)&sa->src, (char *)ptr, sizeof(struct sockaddr_in));
    ptr += sizeof(struct sockaddr_in);
    bcopy((char *)&sa->dst, (char *)ptr, sizeof(struct sockaddr_in));
    if(write(ks, (char *)keymdata, datalen) < 0){
	perror("write to PF_KEY");
    }
    free(keymdata);
#else /* KEY2 */
    bzero((char *)&msg, sizeof(msg));

    msg.msg.sadb_msg_version = PF_KEY_V2;
    msg.msg.sadb_msg_pid = getpid ();
    msg.msg.sadb_msg_seq = isadb_get_seq();
    msg.msg.sadb_msg_type = SADB_DELETE;

    if (type == PROTO_IPSEC_AH)
      msg.msg.sadb_msg_satype = SADB_SATYPE_AH;
    else if (type == PROTO_IPSEC_ESP)
      msg.msg.sadb_msg_satype = SADB_SATYPE_ESP;

    msg.ssa.sadb_sa_exttype = SADB_EXT_SA;
    msg.ssa.sadb_sa_len = sizeof (msg.ssa) / 8;
    msg.ssa.sadb_sa_spi = htonl(spi);
    msg.ssa.sadb_sa_flags = SADB_X_EXT_OLD;

    msg.sad[0].sad.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
    msg.sad[0].sad.sadb_address_len = sizeof(msg.sad[0]) / 8;
    msg.sad[0].sin = sa->src;
    msg.sad[0].sin.sin_port = 0;

    msg.sad[1].sad.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
    msg.sad[1].sad.sadb_address_len = (sizeof(msg.sad[1])) / 8;
    msg.sad[1].sin = sa->dst;
    msg.sad[1].sin.sin_port = 0;

    msg.msg.sadb_msg_len = sizeof(msg) / 8;
    if (write(ks, (char *)&msg, sizeof(msg)) < 0) {
	perror("write to PF_KEY: delete");
    }
#endif /* KEY2 */
    return;
}

/*
 * request a spi from the kernel in the range 300-2500000
 */
void get_unique_spi (SA *sa, int seq_num, int type)
{
#ifndef KEY2
    int datalen;
    struct key_msghdr keymhdr;
    unsigned char *keymdata, *ptr;
#else /* KEY2 */
    struct {
      struct sadb_msg msg;
      struct {
        struct sadb_address sad;
        struct sockaddr_in sin;
      } sad[2];
      struct sadb_spirange range;
    } msg;
#endif /* KEY2 */

#ifndef KEY2
    datalen = sizeof(struct key_msghdr) + (3 * sizeof(struct sockaddr_in));
    keymdata = (unsigned char *) malloc (datalen * sizeof(unsigned char));
    if(keymdata == NULL){
	LOG((CRIT,"Out Of Memory"));
	return;
    }
    bzero((char *)keymdata, datalen);

    bzero((char *)&keymhdr, sizeof(struct key_msghdr));
    keymhdr.key_msgvers = KEY_VERSION;
    keymhdr.key_msgtype = KEY_GETSPI;
    keymhdr.key_msglen = datalen;
    keymhdr.key_pid = my_pid;
    keymhdr.key_seq = seq_num;
    keymhdr.lifetime1 = 300;
    keymhdr.lifetime2 = 2500000;
    bcopy((char *)&keymhdr, (char *)keymdata, sizeof(struct key_msghdr));
    ptr = keymdata + sizeof(struct key_msghdr);
    bcopy((char *)&sa->src, (char *)ptr, sizeof(struct sockaddr_in));
    ptr += sizeof(struct sockaddr_in);
    bcopy((char *)&sa->dst, (char *)ptr, sizeof(struct sockaddr_in));
    if(write(ks, (char *)keymdata, datalen) < 0){
	perror("write to PF_KEY");
    }
    free(keymdata);
#else /* KEY2 */
    bzero((char *)&msg, sizeof(msg));

    msg.msg.sadb_msg_version = PF_KEY_V2;
    msg.msg.sadb_msg_pid = getpid ();
    msg.msg.sadb_msg_seq = seq_num;
    msg.msg.sadb_msg_type = SADB_GETSPI;

    if (type == PROTO_IPSEC_AH)
      msg.msg.sadb_msg_satype = SADB_SATYPE_AH;
    else if (type == PROTO_IPSEC_ESP)
      msg.msg.sadb_msg_satype = SADB_SATYPE_ESP;

    msg.sad[0].sad.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
    msg.sad[0].sad.sadb_address_len = sizeof(msg.sad[0]) / 8;
    msg.sad[0].sin = sa->src;
    msg.sad[0].sin.sin_port = 0;

    msg.sad[1].sad.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
    msg.sad[1].sad.sadb_address_len = (sizeof(msg.sad[1])) / 8;
    msg.sad[1].sin = sa->dst;
    msg.sad[1].sin.sin_port = 0;

    msg.range.sadb_spirange_exttype = SADB_EXT_SPIRANGE;
    msg.range.sadb_spirange_len = sizeof (msg.range) / 8;
    msg.range.sadb_spirange_min = 300;
    msg.range.sadb_spirange_max = 2500000;

    msg.msg.sadb_msg_len = sizeof(msg) / 8;
    if (write(ks, (char *)&msg, sizeof(msg)) < 0) {
	perror("write to PF_KEY: SADB_GETSPI");
    }
#endif /* KEY2 */
    return;
}

/*
 * fire off the next volley to the other guy
 */
void throw (SA *secassoc)
{
    isakmp_hdr hdr;
    conn_entry *centry;

    if(secassoc->packet_len){
	if(secassoc->state_mask & CRYPTO_ACTIVE){
	    bcopy((char *)secassoc->packet, (char *)&hdr, sizeof(isakmp_hdr));
	    if(hdr.mess_id){
		centry = get_centry_by_mess_id(hdr.mess_id);
		if(centry == NULL){
		    LOG((ERR, "No connection entry for %ld! Can't encrypt!", 
				hdr.mess_id));
		    return;
		}
		if(encrypt_payload(secassoc, centry,
			&secassoc->packet, sizeof(isakmp_hdr), 
			&secassoc->packet_len)){
		    LOG((WARN,"Unable to encrypt payload!"));
		    return;
		}
	    } else {
		if(encrypt_payload(secassoc, NULL, 
			&secassoc->packet, sizeof(isakmp_hdr), 
			&secassoc->packet_len)){
		    LOG((WARN,"Unable to encrypt payload!"));
		    return;
		}
	    }
	}
	/* 
	 * the payload length may have changed since the header was
	 * written so make sure the correct length is there. I don't
	 * like it any more than you but it's better than remembering
	 * what the first payload was, malloc'ing space and building 
	 * the new payload = header+old_payload here.
	 */
	bcopy((char *)secassoc->packet, (char *)&hdr, 
			sizeof(isakmp_hdr));
	hdr.len = secassoc->packet_len;
	if((secassoc->state_mask & CRYPTO_ACTIVE) == CRYPTO_ACTIVE)
	    hdr.flags |= ISAKMP_HDR_ENCR_BIT;
	ntoh_hdr(&hdr);
	bcopy((char *)&hdr, (char *)secassoc->packet, 
			sizeof(isakmp_hdr));

	if(send_request(secassoc) < 0)
	    LOG((WARN,"Can't send request after processing!"));

    }
}

static void starve (int id, caddr_t data)
{
    struct sa_mess_id_blob *blob = (struct sa_mess_id_blob *)data;

	/*
	 * We have no guarantee on the order of arrival for our SPI
	 * (a new key acquire message could arrive before our requested SPI)
	 * and must therefore poll every 1/2 second checking whether 
	 * pitcher() has received the spi(s) for which we're waiting,
	 * and return control back to the the protocol.
	 */
    if(still_need_spi(blob->sa, blob->mess_id)){
	bb_add_timeout(context, 500000, starve, data);
    } else {
	if(oakley_const_qm(blob->sa, blob->mess_id) == 0)
	    throw(blob->sa);
	free(blob);
    }
}

int set_to_starve (SA *sa, unsigned long message_id)
{
    struct sa_mess_id_blob *blob;

    blob = (struct sa_mess_id_blob *) malloc (sizeof(struct sa_mess_id_blob));
    if(blob == NULL)
	return(0);
    blob->sa = sa;
    blob->mess_id = message_id;
    return(bb_add_timeout(context, 500000, starve, (caddr_t)blob));
}

static void delete_conn_entry (int id, caddr_t data)
{
    struct sa_mess_id_blob *blob = (struct sa_mess_id_blob *)data;

    isadb_delete_conn_entry(blob->sa, blob->mess_id, 0);
    free(blob);
}

int delay_conn_entry_deletion (SA *sa, unsigned long message_id)
{
    struct sa_mess_id_blob *blob;

    blob = (struct sa_mess_id_blob *) malloc (sizeof(struct sa_mess_id_blob));
    if(blob == NULL)
	return(0);
    blob->sa = sa;
    blob->mess_id = message_id;
    return(bb_add_timeout(context, 250000, delete_conn_entry, (caddr_t)blob));
}


/*
 * handler for receiving a packet from the other guy. We catch and then
 * throw.
 */
static void catcher (int fd, caddr_t data)
{
    isakmp_hdr hdr;
    SA *secassoc, blank_sa;
    unsigned char buffer[1000];
    struct sockaddr_in from;
    int fromlen, len;

    if(get_bytes(fd, buffer, 1000, (struct sockaddr *)&from, &fromlen) < 0){
	LOG((ERR,"Error receiving data from socket!"));
	/* delete SA */
	return;
    } 
    bcopy((char *)buffer, (char *)&hdr, sizeof(isakmp_hdr));
    ntoh_hdr(&hdr);
    secassoc = isadb_get_entry(&hdr, &from);
    if(secassoc == NULL){
	/*
	 * there is no db entry yet, it's a SA payload, and an exchange
	 * that's acceptable at this point, so let's respond, shall we?
	 * Unless, of course, the payload is invalid then throw it away.
	 */
	if((hdr.next_payload == ISA_SA) && 
	  ((hdr.exch == OAK_MM) || (hdr.exch == OAK_AG))){
	    if(validate_payload((hdr.len - sizeof(isakmp_hdr)), 
			hdr.next_payload,
			(buffer + sizeof(isakmp_hdr)), &len) == 0){
		LOG((WARN, "Invalid payload. Possible overrun attack!"));
		return;
	    }
	    secassoc = responder(&hdr, &from, fd, buffer);
	    if(secassoc == NULL)
		return;
	    if(oakley_process_main_mode(&hdr, secassoc) == 0){
		throw(secassoc);
	    }
	} else {
	/*
	 * this SA does not exist and this isn't a request to begin the
	 * protocol. One of us is screwed up. Tell the other guy this
	 * SA is no good.
	 */
	    bzero((char *)&blank_sa, sizeof(SA));
	    bcopy((char *)&from, (char *)&blank_sa.src, 
			sizeof(struct sockaddr_in));
	    bcopy((char *)&hdr.init_cookie, (char *)&blank_sa.his_cookie,
			COOKIE_LEN);
	    bcopy((char *)&hdr.resp_cookie, (char *)&blank_sa.my_cookie,
			COOKIE_LEN);
	    blank_sa.in_or_out = INBOUND;
	    blank_sa.send_dir = UNIDIR;
	    blank_sa.state_mask = blank_sa.state = 0;
	    construct_header(&blank_sa, ISAKMP_INFO, 0, ISA_NOTIFY);
	    len = sizeof(isakmp_hdr);
	    construct_notify(&blank_sa, 0, INVALID_COOKIE, &hdr, len, &len);
	    throw(&blank_sa);
	    LOG((WARN,"Header invalid (no SA, msg = %d)", hdr.next_payload));
	    free(blank_sa.packet);
	}
    } else {				/* else next in the series... */
	/* If we haven't set the fd yet, do it now */
	if (secassoc->fd <= 0 && fd != is)
	    secassoc->fd = fd;
	bb_rem_timeout(context, secassoc->to_id);
	if(secassoc->packet_len){
	    free(secassoc->packet);
	    secassoc->packet_len = 0;
	}
	if(verify_hdr(&hdr, secassoc)){
	    LOG((WARN,"Header invalid (verified)! (msg=%d)",hdr.next_payload));
	    return;
	}
	if(hdr.flags & ISAKMP_HDR_ENCR_BIT){
	    if((secassoc->state_mask & CRYPTO_ACTIVE) == 0){
		LOG((WARN, "recv'd an encrypted packet when not crypto active!"));
		return;
	    }
	    if(hdr.mess_id){
		conn_entry *centry;

		centry = get_centry_by_mess_id(hdr.mess_id);
		if(centry == NULL){
		    if(create_conn_entry(secassoc, hdr.mess_id)){
			LOG((ERR, "Can't create conn entry!"));
			return;
		    }
		    if(compute_quick_mode_iv(secassoc, hdr.mess_id)){
			LOG((ERR, "Can't create QM IV!"));
			isadb_delete_conn_entry(secassoc, hdr.mess_id, 0);
			return;
		    }
		    centry = get_centry_by_mess_id(hdr.mess_id);
		    if(centry == NULL){
			LOG((ERR, "Can't get conn entry I just created!"));
			return;
		    }
		}
		if(decrypt_payload(secassoc, centry, 
			(buffer+sizeof(isakmp_hdr)),
			(hdr.len - sizeof(isakmp_hdr)))){
		    LOG((WARN,"Unable to decrypt payload!\n"));
		    return;
		}
	    } else {
		if(decrypt_payload(secassoc, NULL, (buffer+sizeof(isakmp_hdr)),
			(hdr.len - sizeof(isakmp_hdr)))){
		    LOG((WARN,"Unable to decrypt payload!\n"));
		    return;
		}
	    }
	} else {
	    if((secassoc->state_mask & CRYPTO_ACTIVE) != 0){
		LOG((WARN, "received an unencrypted packet when crypto active!!"));
	    }
	}
	/*
 	 * make a sanity check on this payload. Later parsing assumes 
	 * well formed payloads. Make sure that no payloads in here have
	 * bogus sizes that would cause us to lose our mind later on.
	 */
	if(validate_payload((hdr.len - sizeof(isakmp_hdr)), 
			hdr.next_payload,
			(buffer + sizeof(isakmp_hdr)), &len) == 0){
	    LOG((WARN, "Invalid payload. Possible overrun attack!"));
	    if(hdr.exch != ISAKMP_INFO){
		construct_header(secassoc, ISAKMP_INFO, 0, ISA_NOTIFY);
		len = sizeof(isakmp_hdr);
		construct_notify(secassoc, 0, PAYLOAD_MALFORMED, 
			(buffer + sizeof(isakmp_hdr)), 
			(hdr.len - sizeof(isakmp_hdr)), &len);
		secassoc->send_dir = UNIDIR;
		secassoc->err_counter++;
		throw(secassoc);
	    }
	    return;
	}
	/*
	 * We've identified the SA in question and the payload looks good.
	 * Process it according to the exchange specified.
	 */
	secassoc->payload = (unsigned char *) malloc 
				(hdr.len * sizeof(unsigned char));
	if(secassoc->payload == NULL){
	    LOG((CRIT,"Out Of Memory"));
	    return;
	}
	/*
	 * copy over the entire damned thing (just because, that's why)
	 * but note the actual length so QM hash checking will work right
	 */
	secassoc->payload_len = hdr.len;
	bcopy((char *)buffer, (char *)secassoc->payload, hdr.len);
	if((len + sizeof(isakmp_hdr)) < hdr.len){
	    hdr.len = len + sizeof(isakmp_hdr);
	}
	switch(hdr.exch){
	    case OAK_MM:
		if(oakley_process_main_mode(&hdr, secassoc)){
		    LOG((CRIT, "Main Mode processing failed"));
		}
		break;
	    case OAK_AG:
		if(oakley_process_aggressive(&hdr, secassoc)){
		    LOG((CRIT, "Aggressive Mode processing failed"));
		}
		break;
	    case OAK_QM:
		if(oakley_process_quick_mode(&hdr, secassoc)){
		    LOG((CRIT, "Quick Mode processing failed"));
		}
		break;
	    case ISAKMP_INFO:
		/*
		 * no need for an oakley process routine since there
		 * is no state change or response. Just process the
		 * damned thing and that's'nuff.
		 */
		if(process(hdr.next_payload, secassoc, 
			(secassoc->payload + sizeof(isakmp_hdr)),
			hdr.mess_id)){
		    LOG((CRIT,"Unable to process info only exchange"));
		}
		break;
	    default:
		LOG((ERR, "Unknown exchange type, %d", hdr.exch));
		return;
	}
	throw(secassoc);
    }
    if(secassoc && secassoc->payload)
	free(secassoc->payload);
    return;
}

#ifdef	KEY2
/*
 * Find a particular sadb extension header, return NULL
 * if it is not found or is truncated.
 */
struct sadb_ext *
get_sadb_ext(int type, struct sadb_ext *ptr, int len) 
{
    while (len >= ptr->sadb_ext_len * 8) {
	if (ptr->sadb_ext_type == type)
	    return(ptr);
	len -= ptr->sadb_ext_len * 8;
	ptr = (struct sadb_ext *)((char *)ptr + ptr->sadb_ext_len * 8);
    }
    while (len >= ptr->sadb_ext_len * 8) {
	if (ptr->sadb_ext_type == type)
	    return(ptr);
	len -= ptr->sadb_ext_len * 8;
	ptr = (struct sadb_ext *)((char *)ptr + ptr->sadb_ext_len * 8);
    }
    return (NULL);
}
#endif

#ifdef KEY2
char *sadb_names[] = {
	"RESERVED",
	"GETSPI",
	"UPDATE",
	"ADD",
	"DELETE",
	"GET",
	"ACQUIRE",
	"REGISTER",
	"EXPIRE",
	"FLUSH",
	"DUMP",
	"X_PROMISC",
};
char *sadb_name(int n)
{
    static char sadb_name_buf[16];

    if (n >= 0 && n < sizeof(sadb_names)/sizeof(sadb_names[0]))
	return(sadb_names[n]);
    sprintf(sadb_name_buf, "#%d", n);
    return(sadb_name_buf);
}

char *satype_names[] = {
	"UNSPEC",
	"AH",
	"ESP",
	"RSVP",
	"OSPFV2",
	"RIPV2",
	"MIP"
};

char *satype_name(int n)
{
    static char satype_name_buf[16];

    if (n >= 0 && n < sizeof(satype_names)/sizeof(satype_names[0]))
	return(satype_names[n]);
    sprintf(satype_name_buf, "#%d", n);
    return(satype_name_buf);
}

#endif
/*
 * the kernel pitches to us as opposed to the other guy who throws packets 
 * (which we catch).
 */
static void pitcher (int fd, caddr_t data)
{
#ifndef	KEY2
    struct key_msghdr keymhdr;
    struct key_msgdata keymdata;
    char buff[4096], *ptr;
#else /* KEY2 */
    struct sadb_msg *msg;
    struct sadb_sa *ssa;
    char buff[4096];
    struct sadb_ext *ext;
    struct sadb_sa *src_sa, *dst_sa;
#endif /* KEY2 */
    int rlen;
    struct sockaddr_in to_addr; 
    SA *secassoc = NULL;

    if((rlen = read(ks, buff, sizeof(buff))) < 0){
	perror("read from PF_KEY");
	return;
    }
#ifndef KEY2
    bcopy(buff, (char *)&keymhdr, sizeof(struct key_msghdr));
    switch(keymhdr.key_msgtype)
#else /* KEY2 */
    msg = (struct sadb_msg *)buff;
    rlen -= sizeof(struct sadb_msg);
    ext = (struct sadb_ext *)(msg + 1);
    if (msg->sadb_msg_errno) {
	LOG((DEB, "%s failed, seq %d, satype %s: %s",
	    sadb_name(msg->sadb_msg_type), msg->sadb_msg_seq,
	    satype_name(msg->sadb_msg_satype),
	    strerror(msg->sadb_msg_errno)));
    }

    switch(msg->sadb_msg_type)
#endif /* KEY2 */
    {
#ifndef	KEY2
	case KEY_ACQUIRE:
	    LOG((DEB,"kernel sent a key acquire message!"));
	    ptr = buff + sizeof(struct key_msghdr);
	    keymdata.src = (struct sockaddr *)ptr;
	    ptr += ROUNDUP(keymdata.src->sa_len);
	    keymdata.dst = (struct sockaddr *)ptr;
	    ptr += ROUNDUP(keymdata.dst->sa_len);
  
	    bzero((char *)&to_addr, sizeof(struct sockaddr_in));
	    to_addr.sin_len = sizeof(struct sockaddr_in);
	    to_addr.sin_family = AF_INET;
	    to_addr.sin_port = htons(IKMP_PORT);
	    to_addr.sin_addr.s_addr =
                ((struct sockaddr_in *)keymdata.dst)->sin_addr.s_addr;
	    switch(keymhdr.type){
		case KEY_TYPE_AH:
		    secassoc = initiator(&to_addr, 
					(struct sockaddr_in *)keymdata.src, 
					NULL, PROTO_IPSEC_AH);
		    break;
		case KEY_TYPE_ESP:
		    secassoc = initiator(&to_addr, 
					(struct sockaddr_in *)keymdata.src, 
					NULL, PROTO_IPSEC_ESP);
		    break;
	    }
#else /* KEY2 */
	case SADB_ACQUIRE:
	    LOG((DEB,"kernel sent a key acquire message!"));
	    if (msg->sadb_msg_errno)
		break;
	    src_sa = (struct sadb_sa *)get_sadb_ext(SADB_EXT_ADDRESS_SRC, ext, rlen);
	    if (src_sa == NULL) {
		LOG((DEB,"ACQUIRE: missing src addr!"));
		return;
	    }
	    dst_sa = (struct sadb_sa *)get_sadb_ext(SADB_EXT_ADDRESS_DST, ext, rlen);
	    if (dst_sa == NULL) {
		LOG((DEB,"ACQUIRE: missing dst addr!"));
		return;
	    }
  
	    bzero((char *)&to_addr, sizeof(struct sockaddr_in));
	    to_addr.sin_len = sizeof(struct sockaddr_in);
	    to_addr.sin_family = AF_INET;
	    to_addr.sin_port = htons(IKMP_PORT);
	    to_addr.sin_addr.s_addr =
                ((struct sockaddr_in *)(dst_sa+1))->sin_addr.s_addr;
    	    switch(msg->sadb_msg_satype){
		case SADB_SATYPE_AH:
		    secassoc = initiator(&to_addr, 
					(struct sockaddr_in *)(src_sa+1),
					NULL, PROTO_IPSEC_AH);
		    break;
		case SADB_SATYPE_ESP:
		    secassoc = initiator(&to_addr, 
					(struct sockaddr_in *)(src_sa+1),
					NULL, PROTO_IPSEC_ESP);
		    break;
		default:
		    LOG((DEB, "ACQUIRE: unknown satype %d",
			msg->sadb_msg_satype));
	    }
#endif /* KEY2 */
	    if(secassoc != NULL){
		throw(secassoc);
	    }
	    break;
 
#ifndef	KEY2
	case KEY_GETSPI:
	    LOG((DEB,"kernel sent a SPI!"));
	    if(keymhdr.key_pid != my_pid){
		return;
	    }
	    if(set_spi_by_seq(keymhdr.key_seq, keymhdr.spi)){
		LOG((WARN,"Got a SPI for a non-existant SA!"));
	    }
	    break;
#else
	case SADB_GETSPI:
	    LOG((DEB,"kernel sent an SPI"));
	    if (msg->sadb_msg_errno)
		break;
	    if(msg->sadb_msg_pid != my_pid) {
	        LOG((DEB,"Pid's don't match, ignoring (got %d, expect %d)",
		    msg->sadb_msg_pid, my_pid));
		return;
	    }
	    ssa = (struct sadb_sa *)get_sadb_ext(SADB_EXT_SA, ext, rlen);
	    if (ssa == NULL) {
		LOG((WARN,"Missing SADB_EXT_SA!"));
		break;
	    }
	    if(set_spi_by_seq(msg->sadb_msg_seq, ntohl(ssa->sadb_sa_spi)))
		LOG((WARN,"Got a SPI for a non-existant SA!"));
	    break;
#endif

#ifndef	KEY2
	case KEY_ADD:
	    LOG((DEB,"got a KEY_ADD msg for SA with SPI %d", keymhdr.spi));
	    break;
	case KEY_REGISTER:
	    break;
#else
	case SADB_ADD:
	case SADB_UPDATE:
	    if (msg->sadb_msg_errno)
		break;
	    ssa = (struct sadb_sa *)get_sadb_ext(SADB_EXT_SA, ext, rlen);
	    if (ssa == NULL) {
		LOG((WARN,"Missing SADB_EXT_SA!"));
		break;
	    }
	    LOG((DEB,"Got an %s msg for SPI %x",
		sadb_name(msg->sadb_msg_type), ssa->sadb_sa_spi));
	    break;
	case SADB_REGISTER:
	    LOG((DEB,"Got an SADB_REGISTER msg"));
	    break;
#endif
	default:
	    LOG((DEB,"Unknown kernel message #%d!", msg->sadb_msg_type));
	    break;
    }
    return;
}

/*
 * we've finished! We now have a security associaton to drop down to 
 * the kernel. 
 */
void load_sa (SA *secassoc, sa_list *node, conn_entry *centry, int direction)
{
#ifndef KEY2
    struct key_msghdr keymhdr;
    char *key_mess, *ptr;
#else /* KEY2 */
    struct {
      struct sadb_msg msg;
      struct sadb_sa ssa;
    } msg;
    struct sadb_lifetime hard, soft;
    struct {
      struct sadb_address sad;
      struct sockaddr_in sin;
    } sad[3];
    struct sadb_key *hash;
    int hashlen;
    struct sadb_key *key;
    int keylen;
    int len;

    struct iovec iv[6];
    int ivcnt = 0;
#define ADD_IOV(buf, len) \
	{ iv[ivcnt].iov_base = (void *)(buf); iv[ivcnt++].iov_len = len; }
#endif /* KEY2 */
    struct sockaddr src, dst, from;

#ifndef KEY2
    bzero((char *)&keymhdr, sizeof(struct key_msghdr));
    keymhdr.key_msgvers = KEY_VERSION;
    keymhdr.key_msgtype = KEY_ADD;
    keymhdr.key_pid = my_pid;
    keymhdr.key_msglen = sizeof(struct key_msghdr) +
			(3 * sizeof(struct sockaddr_in));
	/*
	 * if direction == INITIATOR, go src->dst, unless the roles have
	 * been swapped (ISAKMP SA is bi-directional), then do dst->src.
	 */
    if(secassoc->init_or_resp != direction){
	bcopy((char *)&secassoc->dst, (char *)&src, sizeof(struct sockaddr));
	bcopy((char *)&secassoc->src, (char *)&dst, sizeof(struct sockaddr));
    } else {
	bcopy((char *)&secassoc->src, (char *)&src, sizeof(struct sockaddr));
	bcopy((char *)&secassoc->dst, (char *)&dst, sizeof(struct sockaddr));
    }
    bcopy((char *)&src, (char *)&from, sizeof(struct sockaddr));
    if(direction == INBOUND){
	keymhdr.spi = node->spi;
	keymhdr.key_msglen += node->inbound_crypt_keylen;
	keymhdr.keylen = node->inbound_crypt_keylen;
    } else {
	keymhdr.spi = node->other_spi;
	keymhdr.key_msglen += node->outbound_crypt_keylen;
	keymhdr.keylen = node->outbound_crypt_keylen;
    }
/*
    bzero((char *)&from, sizeof(struct sockaddr));
    from.sa_len = sizeof(struct sockaddr);
    from.sa_family = AF_INET;
	*
	 * PF_KEY only accepts one additional proxy identity (for some reason)
	 * so just try to guess which should be the right "from": if the
	 * initiator use the proxy destination, if the responder use the 
	 * source. 
	 *
    if(secassoc->init_or_resp == INITIATOR){
	if(centry->proxy_dst.id_type == ID_IPV4_ADDR)
	    bcopy((char *)&centry->proxy_dst.addr, 
		(char *)&from, sizeof(struct sockaddr_in));
    } else {
	if(centry->proxy_src.id_type == ID_IPV4_ADDR)
	    bcopy((char *)&centry->proxy_src.addr, 
		(char *)&from, sizeof(struct sockaddr_in));
    }
*/
	/*
	 * I'm tired of this refrain:
	 *     "Oh, we'll just let key management handle that!" 
	 */
    switch(node->other_sa.type){
	/*
	 * not that negotiation of ESP_HMAC_ALG_foo in conjunction with
	 * an ESP negotiation will be dropped from the SA stuff. The NRL
	 * code doesn't handle new ESP yet. If your code does, fix this.
	 */
	static int ah_doi2pfkey[] = {
		-1,
		IPSEC_ALGTYPE_AH_MD5,		/* AH_MD5_KPDK */
		IPSEC_ALGTYPE_AH_HMACMD5,	/* AH_MD5_HMAC */
		-1				/* AH_SHA_HMAC_REPLAY */
	};
	static int esp_doi2pfkey[] = {
		-1,
		IPSEC_ALGTYPE_ESP_DES_CBC,	/* ESP_DES_CBC */
		IPSEC_ALGTYPE_ESP_DES_CBC,	/* ESP_DEC */
		-1				
	};
	case PROTO_IPSEC_AH:
	    keymhdr.type = KEY_TYPE_AH;
	    keymhdr.algorithm = ah_doi2pfkey[node->other_sa.ah.transform];
	    keymhdr.key_msglen += 8; 		/* make PF_KEY happy */
	    keymhdr.ivlen = 8;
	    break;
	case PROTO_IPSEC_ESP:
	    keymhdr.type = KEY_TYPE_ESP;
	    keymhdr.algorithm = esp_doi2pfkey[node->other_sa.esp.transform];
	    keymhdr.ivlen = node->other_sa.esp.crypt_ivlen;
	    keymhdr.key_msglen += node->other_sa.esp.crypt_ivlen;
	    /*
	     * DES only has 8 byte keys.  If hmac_alg is non-zero,
	     * then the key length includes hmac information, so
	     * strip the key length back to 8, since we only want
	     * to store 8 byte DES keys into the kernel.  (This
	     * makes "pfkey store"/"pfkey load" much happier.)
	     */
	    break;
	default:
	    LOG((ERR,"Unknown protocol. Not adding SA w/spi=%d",keymhdr.spi));
	    return;
    }

    if((key_mess = (char *) malloc (keymhdr.key_msglen)) == NULL){
	LOG((CRIT, "Out Of Memory. Unable to add key"));
	return;
    }
    bzero((char *)key_mess, keymhdr.key_msglen);
	/* 
	 * add the key message header followed by the...
	 */
    bcopy((char *)&keymhdr, key_mess, sizeof(struct key_msghdr));
    ptr = key_mess + sizeof(struct key_msghdr);
	/*
	 * the src, dst, and from addresses
	 */
    bcopy((char *)&src, ptr, sizeof(struct sockaddr));
    ptr += sizeof(struct sockaddr);

    bcopy((char *)&dst, ptr, sizeof(struct sockaddr));
    ptr += sizeof(struct sockaddr);

    bcopy((char *)&from, ptr, sizeof(struct sockaddr));
    ptr += sizeof(struct sockaddr);

	/* the key and iv (if needed) */
    if(keymhdr.keylen){
	if(direction == INBOUND){
	    bcopy((char *)node->inbound_crypt_key, ptr, keymhdr.keylen);
#ifdef KEY_DEBUG
	print_vpi("loading inbound SA with key", node->inbound_crypt_key,
						node->inbound_crypt_keylen);
	print_vpi("message has", ptr, keymhdr.keylen);
#endif
	} else {
	    bcopy((char *)node->outbound_crypt_key, ptr, keymhdr.keylen);
#ifdef KEY_DEBUG
	print_vpi("loading outbound SA with key", node->outbound_crypt_key,
						node->outbound_crypt_keylen);
	print_vpi("message has", ptr, keymhdr.keylen);
#endif
	}
	ptr += keymhdr.keylen;
    }
    switch(node->other_sa.type){
	case PROTO_IPSEC_AH:
	    ptr += 8;			/* make PF_KEY happy */
	    break;
	case PROTO_IPSEC_ESP:
	    if (node->other_sa.esp.crypt_iv != NULL)
		bcopy((char *)node->other_sa.esp.crypt_iv, ptr, 
			 node->other_sa.esp.crypt_ivlen);
	    else
		bzero(ptr, node->other_sa.esp.crypt_ivlen);
	    ptr += node->other_sa.esp.crypt_ivlen;
	    break;
    }
    if(write(ks, key_mess, keymhdr.key_msglen) != keymhdr.key_msglen){
	perror("add key");
    }
    free(key_mess);
#else /* KEY2 */
    bzero((char *)&msg, sizeof(msg));
    msg.msg.sadb_msg_version = PF_KEY_V2;
    msg.msg.sadb_msg_pid = getpid ();
    msg.msg.sadb_msg_seq = isadb_get_seq();
    if (direction == INBOUND) {
	msg.msg.sadb_msg_type = SADB_UPDATE;
	msg.ssa.sadb_sa_spi = htonl(node->spi);
    } else {
	msg.msg.sadb_msg_type = SADB_ADD;
	msg.ssa.sadb_sa_spi = htonl(node->other_spi);
    }
    msg.ssa.sadb_sa_state = SADB_SASTATE_MATURE;
    msg.ssa.sadb_sa_flags = SADB_X_EXT_OLD;

    switch (node->other_sa.type) {
    case PROTO_IPSEC_ESP:
      msg.msg.sadb_msg_satype = SADB_SATYPE_ESP;

      switch (node->other_sa.esp.transform) {
      case ESP_DES:
      case ESP_DES_CBC:
	keylen = 8;
	msg.ssa.sadb_sa_encrypt = SADB_EALG_DESCBC;
	break;

      case ESP_3DES:
	keylen = 24;
	msg.ssa.sadb_sa_encrypt = SADB_EALG_3DESCBC;
	break;
      default:
	keylen = 0;
	break;
      }

      switch (node->other_sa.esp.hmac_alg) {
      case ESP_HMAC_ALG_MD5:
	hashlen = 16;
	msg.ssa.sadb_sa_auth = SADB_AALG_MD5HMAC;
	break;

      case ESP_HMAC_ALG_SHA:
	hashlen = 20;
	msg.ssa.sadb_sa_auth = SADB_AALG_SHA1HMAC;
	break;

      default:
        LOG((ERR,"Unknown HMAC. Not adding SA w/spi=%d",msg.ssa.sadb_sa_spi));
	return;

      case 0:
	hashlen = 0;
	msg.ssa.sadb_sa_auth = SADB_AALG_NONE;
	break;
      }
      break;

    case PROTO_IPSEC_AH:
      keylen = 0;
      switch (node->other_sa.ah.transform) {
      case AH_MD5_HMAC:
	hashlen = 16;
	msg.ssa.sadb_sa_auth = SADB_AALG_MD5HMAC;
	break;

      case AH_SHA_HMAC:
	hashlen = 20;
	msg.ssa.sadb_sa_auth = SADB_AALG_SHA1HMAC;
	break;
      default:
	hashlen = 0;
	break;
      }
      break;

    default:
      LOG((ERR,"Unknown protocol. Not adding SA w/spi=%d",msg.ssa.sadb_sa_spi));
      return;
    }

	/*
	 * if direction == INITIATOR, go src->dst, unless the roles have
	 * been swapped (ISAKMP SA is bi-directional), then do dst->src.
	 */
    if(secassoc->init_or_resp != direction){
	bcopy((char *)&secassoc->dst, (char *)&src, sizeof(struct sockaddr));
	bcopy((char *)&secassoc->src, (char *)&dst, sizeof(struct sockaddr));
    } else {
	bcopy((char *)&secassoc->src, (char *)&src, sizeof(struct sockaddr));
	bcopy((char *)&secassoc->dst, (char *)&dst, sizeof(struct sockaddr));
    }
    ((struct sockaddr_in *)&src)->sin_port = 0;
    ((struct sockaddr_in *)&dst)->sin_port = 0;
    bcopy((char *)&src, (char *)&from, sizeof(struct sockaddr));
    msg.ssa.sadb_sa_exttype = SADB_EXT_SA;
    msg.ssa.sadb_sa_len = sizeof(msg.ssa) / 8;
    ADD_IOV(&msg, sizeof(msg));

    /* Hard limits */
    hard.sadb_lifetime_len = sizeof(hard) / 8;
    hard.sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
    hard.sadb_lifetime_allocations = 0;
    hard.sadb_lifetime_bytes = 0;
    hard.sadb_lifetime_addtime = 0;
    hard.sadb_lifetime_usetime = 0;
    {
	int type, val;
	u_char *ptr;
	if (node->other_sa.type == PROTO_IPSEC_AH) {
	    type = node->other_sa.ah.ah_life_type;
	    len = node->other_sa.ah.ah_life_len;
	    ptr = node->other_sa.ah.ah_life_dur;
	} else /* if (node->other_sa.type == PROTO_IPSEC_ESP) */ {
	    type = node->other_sa.esp.esp_life_type;
	    len = node->other_sa.esp.esp_life_len;
	    ptr = node->other_sa.esp.esp_life_dur;
	}

	val = 0;
	while (len-- > 0)
	    val = (val<<8) | *ptr++;

	    switch (type) {
	    case IPSEC_LIFE_SEC:
		/* hard.sadb_lifetime_usetime = val; */
		hard.sadb_lifetime_addtime = val;
		break;
	    case IPSEC_LIFE_KB:
		hard.sadb_lifetime_bytes = val * 1024;
		break;
	}
    }
    ADD_IOV(&hard, sizeof(hard));
  
    /* Set soft limits at 90% of hard limits */
    soft.sadb_lifetime_len = sizeof(soft) / 8; 
    soft.sadb_lifetime_exttype = SADB_EXT_LIFETIME_SOFT;
    soft.sadb_lifetime_allocations = soft.sadb_lifetime_allocations * 9 / 10;
    soft.sadb_lifetime_bytes = hard.sadb_lifetime_bytes * 9 / 10;
    soft.sadb_lifetime_addtime = hard.sadb_lifetime_addtime * 9 / 10;
    soft.sadb_lifetime_usetime = hard.sadb_lifetime_usetime * 9 / 10;

    ADD_IOV(&soft, sizeof(soft));

/*
    bzero((char *)&from, sizeof(struct sockaddr));
    from.sa_len = sizeof(struct sockaddr);
    from.sa_family = AF_INET;
	*
	 * PF_KEY only accepts one additional proxy identity (for some reason)
	 * so just try to guess which should be the right "from": if the
	 * initiator use the proxy destination, if the responder use the 
	 * source. 
	 *
    if(secassoc->init_or_resp == INITIATOR){
	if(centry->proxy_dst.id_type == ID_IPV4_ADDR)
	    bcopy((char *)&centry->proxy_dst.addr, 
		(char *)&from, sizeof(struct sockaddr_in));
    } else {
	if(centry->proxy_src.id_type == ID_IPV4_ADDR)
	    bcopy((char *)&centry->proxy_src.addr, 
		(char *)&from, sizeof(struct sockaddr_in));
    }
*/
    switch (node->other_sa.type) {
	/*
	 * not that negotiation of ESP_HMAC_ALG_foo in conjunction with
	 * an ESP negotiation will be dropped from the SA stuff. The NRL
	 * code doesn't handle new ESP yet. If your code does, fix this.
	 */
    static int ah_doi2pfkey[] = {
	-1,
	SADB_AALG_MD5HMAC,	/*XXX*/	/* AH_MD5_KPDK */
	SADB_AALG_MD5HMAC,		/* AH_MD5_HMAC */
	-1				/* AH_SHA_HMAC_REPLAY */
    };
    static int esp_doi2pfkey[] = {
	-1,
	SADB_EALG_DESCBC,		/* ESP_DES_CBC */
	SADB_EALG_DESCBC,		/* ESP_DEC */
	-1				
    };
    case PROTO_IPSEC_AH:
	msg.msg.sadb_msg_satype = SADB_SATYPE_AH;
	msg.ssa.sadb_sa_auth = ah_doi2pfkey[node->other_sa.ah.transform];
	break;

    case PROTO_IPSEC_ESP:
	msg.msg.sadb_msg_satype = SADB_SATYPE_ESP;
	msg.ssa.sadb_sa_encrypt = esp_doi2pfkey[node->other_sa.esp.transform];
	break;

    default:
	LOG((ERR,"Unknown protocol. Not adding SA w/spi=%d",
	    msg.ssa.sadb_sa_spi));
	return;
    }

    bzero(&sad, sizeof(sad));
    sad[0].sad.sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
    sad[0].sad.sadb_address_len = sizeof(sad[0]) / 8;
    bcopy((char *)&src, (char *)&sad[0].sin, sizeof(sad[0].sin));

    sad[1].sad.sadb_address_exttype = SADB_EXT_ADDRESS_DST;
    sad[1].sad.sadb_address_len = sizeof(sad[1]) / 8;
    bcopy((char *)&dst, (char *)&sad[1].sin, sizeof(sad[1].sin));

    sad[2].sad.sadb_address_exttype = SADB_EXT_ADDRESS_PROXY;
    sad[2].sad.sadb_address_len = sizeof(sad[2]) / 8;
    bcopy((char *)&from, (char *)&sad[2].sin, sizeof(sad[2].sin));

    ADD_IOV(&sad, sizeof(sad));


#define RND(len, size) (((len) + ((size) - 1)) & ~((size)-1))
	/* the key and iv (if needed) */


    key = NULL;
    if (keylen) {
	len = RND(sizeof(*key) + keylen, 8);
	key = (struct sadb_key *)malloc(len);

	key->sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
	key->sadb_key_len = len / 8;
	key->sadb_key_bits = keylen * 8;
	key->sadb_key_reserved = 0;
	if (direction == INBOUND){
	    bcopy((char *)node->inbound_crypt_key, (char *)(key+1), 
			  node->inbound_crypt_keylen);
#ifdef KEY_DEBUG
	print_vpi("loading inbound SA with key", node->inbound_crypt_key,
						node->inbound_crypt_keylen);
	print_vpi("message has", (char *)(key + 1), node->inbound_crypt_keylen);
#endif
	} else {
	    bcopy((char *)node->outbound_crypt_key, (char *)(key+1), 
			  node->outbound_crypt_keylen);
#ifdef KEY_DEBUG
	print_vpi("loading outbound SA with key", node->outbound_crypt_key,
						node->outbound_crypt_keylen);
	print_vpi("message has", (char *)(key+1), node->outbound_crypt_keylen);
#endif
	}
	ADD_IOV(key, len);
    }

    len = RND(sizeof(*hash) + hashlen, 8);
    hash = malloc(len);

    hash->sadb_key_exttype = SADB_EXT_KEY_AUTH;
    hash->sadb_key_len = len / 8;
    hash->sadb_key_bits = hashlen * 8;
    hash->sadb_key_reserved = 0;
    switch(node->other_sa.type){
	case PROTO_IPSEC_AH:
	    bzero((char *)(hash+1), 8);
	    break;
	case PROTO_IPSEC_ESP:
	    if (node->other_sa.esp.crypt_iv != NULL)
		bcopy((char *)node->other_sa.esp.crypt_iv, (char *)(hash+1), 
			 node->other_sa.esp.crypt_ivlen);
	    else
		bzero((char *)(hash+1), node->other_sa.esp.crypt_ivlen);
	    break;
    }
    ADD_IOV(hash, len);

    /* compute the total length */
    msg.msg.sadb_msg_len = 0;
    for (len = 0; len < ivcnt; len++)
	msg.msg.sadb_msg_len += iv[len].iov_len / 8;

    if (writev(ks, iv, ivcnt) < 0)
	perror("add key");

    if (key)
        free(key);
    free(hash);
#endif /* KEY2 */
}

int main (int argc, char **argv)
{
    struct sockaddr_in server_in;
#ifndef KEY2
    struct key_msghdr keymhdr;
#else
    struct sadb_msg msg;
#endif
    unsigned short bound;
    struct ifaddrs *ifa, *ifa_copy;
    int on = 1;

    chdir("/");			/* don't hog a disk */
#ifndef DEBUG
	/*
	 * the daemonic thing to do... ignore stop signs and have no 
	 * controlling terminal.
	 */
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
    signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif
    if(fork())
	exit(0);
    setpgrp(0, my_pid); 
    openlog("ikmpd", LOG_PID, LOG_DAEMON);
#endif
    my_pid = getpid();
    context = bbCreateContext();

#ifndef KEY2
    ks = socket(PF_KEY, SOCK_RAW, 0);		/* the kernel */
#else
    ks = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);	/* the kernel */
#endif
    if (ks < 0) {
#ifndef KEY2
	LOG((CRIT,"%s: Unable to create PF_KEY socket!", argv[0]));
#else
	LOG((CRIT,"%s: Unable to create PF_KEY_V2 socket!", argv[0]));
#endif
	exit(1);
    }
    is = socket(AF_INET, SOCK_DGRAM, 0);	/* other daemons */

    bzero((char *)&server_in, sizeof(struct sockaddr_in));
    bound = 0;
    server_in.sin_family = AF_INET;
    server_in.sin_port = htons(IKMP_PORT);

    if (getifaddrs(&ifa) == 0) {
	for (ifa_copy = ifa; ifa; ifa = ifa->ifa_next) {
	    if (ifa->ifa_addr->sa_family != AF_INET)
		continue;
	    if (ifa->ifa_flags & IFF_LOOPBACK)
		continue;
	    server_in.sin_addr =
		((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	    if (bind(is, (struct sockaddr *)&server_in, 
				sizeof(struct sockaddr_in)) < 0) {
		continue;
	    }
	    bb_add_input(context, is, NULL, (inputcb)catcher);
	    is = socket(AF_INET, SOCK_DGRAM, 0);
	}
	free(ifa_copy);
    }
    server_in.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(is, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(bind(is, (struct sockaddr *)&server_in, sizeof(struct sockaddr_in)) < 0){
	LOG((CRIT,"%s: Unable to bind socket!",argv[0]));
	exit(1);
    }
	/* 
	 * tell kernel I can negotiate AH and ESP 
	 */
#ifndef KEY2
    bzero((char *)&keymhdr, sizeof(struct key_msghdr));
    keymhdr.key_msgvers = KEY_VERSION;
    keymhdr.key_msgtype = KEY_REGISTER;
    keymhdr.key_msglen = sizeof(struct key_msghdr);
    keymhdr.key_pid = my_pid;
    keymhdr.type = KEY_TYPE_AH;
    if(write(ks, (char *)&keymhdr, sizeof(struct key_msghdr)) < 0){
	perror("write to PF_KEY");
	exit(1);
    }
    keymhdr.type = KEY_TYPE_ESP;
    if(write(ks, (char *)&keymhdr, sizeof(struct key_msghdr)) < 0){
	perror("write to PF_KEY");
	exit(1);
    }
#else
    bzero((char *)&msg, sizeof(msg));
    msg.sadb_msg_version = PF_KEY_V2;
    msg.sadb_msg_pid = getpid ();
    msg.sadb_msg_seq = isadb_get_seq();

    msg.sadb_msg_type = SADB_REGISTER;
    msg.sadb_msg_satype = SADB_SATYPE_AH;
    msg.sadb_msg_len = sizeof(msg) / 8;
    if (write(ks, (char *)&msg, sizeof(msg)) < 0) {
	perror("write to PF_KEY: register AH");
	exit(1);
    }

    msg.sadb_msg_seq = isadb_get_seq();
    msg.sadb_msg_satype = SADB_SATYPE_ESP;
    if (write(ks, (char *)&msg, sizeof(msg)) < 0) {
	perror("write to PF_KEY: register ESP");
	exit(1);
    }
#endif
    gen_stamp();
	/* 
	 * set up handlers, sit back, relax, and wait for input 
	 */
    bb_add_input(context, is, NULL, (inputcb)catcher);
    bb_add_input(context, ks, NULL, (inputcb)pitcher);
    bb_add_timeout(context, 300000000, reaper, NULL);

    bbMainLoop(context);
    exit(0);
}
 
