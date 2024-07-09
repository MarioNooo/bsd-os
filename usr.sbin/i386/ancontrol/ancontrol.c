/*
 * Copyright 1997, 1998, 1999
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_types.h>
#ifdef	FreeBSD
#include <net/if_var.h>
#include <net/ethernet.h>
#endif

#include <net/if_802_11.h>
#include <i386/isa/if_anreg.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#ifndef	FreeBSD
#include <ctype.h>
#define	ETHER_ADDR_LEN	6
struct ether_addr {
	u_char	addr[ETHER_ADDR_LEN];
};

struct ether_addr *
ether_aton(char *str)
{
	static struct ether_addr a;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; ++i) {
		if (i && *str++ != ':')
			return (NULL);
		if (!isxdigit(str[0]) || !isxdigit(str[1]))
			return (NULL);
		a.addr[i] = (digittoint(str[0]) << 4) || digittoint(str[1]);
		str += 2;
	}
	if (*str)
		return (NULL);
	return(&a);
}
#endif

#if !defined(lint)
static const char copyright[] = "@(#) Copyright (c) 1997, 1998, 1999\
	Bill Paul. All rights reserved.";
static const char rcsid[] =
  "@(#) $FreeBSD$";
#endif

static void an_getval		__P((char *, struct an_req *, int));
static void an_setval		__P((char *, struct an_req *, int));
static void an_printwords	__P((u_int16_t *, int));
static void an_printspeeds	__P((u_int8_t*, int));
static void an_printbool	__P((int));
static void an_printhex		__P((char *, int));
static void an_printstr		__P((char *, int));
static void an_dumpstatus	__P((char *));
static void an_dumpstats	__P((char *));
static void an_dumpconfig	__P((char *));
static void an_dumpcaps		__P((char *));
static void an_dumpssid		__P((char *));
static void an_dumpap		__P((char *));
static void an_setconfig	__P((char *, int, void *));
static void an_setssid		__P((char *, int, void *));
static void an_setap		__P((char *, int, void *));
static void an_setspeed		__P((char *, int, void *));
static void usage		__P((char *));
int main			__P((int, char **));

#define ACT_DUMPSTATS 1
#define ACT_DUMPCONFIG 2
#define ACT_DUMPSTATUS 3
#define ACT_DUMPCAPS 4
#define ACT_DUMPSSID 5
#define ACT_DUMPAP 6

#define ACT_SET_OPMODE 7
#define ACT_SET_SSID1 8
#define ACT_SET_SSID2 9
#define ACT_SET_SSID3 10
#define ACT_SET_FREQ 11
#define ACT_SET_AP1 12
#define ACT_SET_AP2 13
#define ACT_SET_AP3 14
#define ACT_SET_AP4 15
#define ACT_SET_DRIVERNAME 16
#define ACT_SET_SCANMODE 17
#define ACT_SET_TXRATE 18
#define ACT_SET_RTS_THRESH 19
#define ACT_SET_PWRSAVE 20
#define ACT_SET_DIVERSITY_RX 21
#define ACT_SET_DIVERSITY_TX 22
#define ACT_SET_RTS_RETRYLIM 23
#define ACT_SET_WAKE_DURATION 24
#define ACT_SET_BEACON_PERIOD 25
#define ACT_SET_TXPWR 26
#define ACT_SET_FRAG_THRESH 27
#define ACT_SET_NETJOIN 28
#define ACT_SET_MYNAME 29
#define ACT_SET_MAC 30
#define ACT_SET_AUTHTYPE 31

static void an_getval(iface, areq, type)
	char			*iface;
	struct an_req		*areq;
	int			 type;
{
	int mib[] = { CTL_NET, PF_LINK, 0, CTL_LINK_HWTYPE, AN_CTL_REGISTER, 0};
	int			s;
	size_t			len;

	mib[5] = type;
	mib[2] = if_nametoindex(iface);

	len = areq->an_len;
	if (sysctl(mib, 6, areq, &len, NULL, 0) != 0)
		err(1, "Getting RID 0x%04x", type);
	areq->an_len = len;
}

static void an_setval(iface, areq, type)
	char			*iface;
	struct an_req		*areq;
	int			 type;
{
	int mib[] = { CTL_NET, PF_LINK, 0, CTL_LINK_HWTYPE, AN_CTL_REGISTER, 0};
	int			s;

	mib[5] = type;
	mib[2] = if_nametoindex(iface);

	if (sysctl(mib, 6, NULL, NULL, areq, areq->an_len))
		err(1, "Setting RID 0x%04x", type);
}

static void an_printstr(str, len)
	char			*str;
	int			len;
{
	int			i;

	for (i = 0; i < len - 1; i++) {
		if (str[i] == '\0')
			str[i] = ' ';
	}

	printf("[ %.*s ]", len, str);

	return;
}

static void an_printwords(w, len)
	u_int16_t		*w;
	int			len;
{
	int			i;

	printf("[ ");
	for (i = 0; i < len; i++)
		printf("%d ", w[i]);
	printf("]");

	return;
}

static void an_printspeeds(w, len)
	u_int8_t		*w;
	int			len;
{
	int			i;

	printf("[ ");
	for (i = 0; i < len && w[i]; i++)
		printf("%2.1fMbps ", w[i] * 0.500);
	printf("]");

	return;
}

static void an_printbool(val)
	int			val;
{
	if (val)
		printf("[ On ]");
	else
		printf("[ Off ]");

	return;
}

static void an_printhex(ptr, len)
	char			*ptr;
	int			len;
{
	int			i;

	printf("[ ");
	for (i = 0; i < len; i++) {
		printf("%02x", ptr[i] & 0xFF);
		if (i < (len - 1))
			printf(":");
	}

	printf(" ]");
	return;
}

static void an_strength(iface)
	char			*iface;
{
	struct an_ltv_status	*sts;
	struct an_req		areq;

	areq.an_len = sizeof(areq);
	sts = (struct an_ltv_status *)&areq;

	for (;;) {
		an_getval(iface, &areq, AN_RID_STATUS);
		printf("Signal: %d/%d\n", sts->an_cur_signal_quality,
		    sts->an_rsvd0[9]);
		sleep(1);
	}
}

static void an_dumpstatus(iface)
	char			*iface;
{
	struct an_ltv_status	*sts;
	struct an_req		areq;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_STATUS);

	sts = (struct an_ltv_status *)&areq;

	printf("MAC address:\t\t");
	an_printhex((char *)&sts->an_macaddr, ETHER_ADDR_LEN);
	printf("\nOperating mode:\t\t[ ");
	if (sts->an_opmode & AN_STATUS_OPMODE_CONFIGURED)
		printf("configured ");
	if (sts->an_opmode & AN_STATUS_OPMODE_MAC_ENABLED)
		printf("MAC ON ");
	if (sts->an_opmode & AN_STATUS_OPMODE_RX_ENABLED)
		printf("RX ON ");
	if (sts->an_opmode & AN_STATUS_OPMODE_IN_SYNC)
		printf("synced ");
	if (sts->an_opmode & AN_STATUS_OPMODE_ASSOCIATED)
		printf("associated ");
	if (sts->an_opmode & AN_STATUS_OPMODE_ERROR)
		printf("error ");
	printf("]\n");
	printf("Error code:\t\t");
	an_printhex((char *)&sts->an_errcode, 1);
	printf("\nSignal quality:\t\t");
	an_printhex((char *)&sts->an_cur_signal_quality, 1);
	printf("\nCurrent SSID:\t\t");
	an_printstr((char *)&sts->an_ssid, sts->an_ssidlen);
	printf("\nCurrent AP name:\t");
	an_printstr((char *)&sts->an_ap_name, 16);
	printf("\nCurrent BSSID:\t\t");
	an_printhex((char *)&sts->an_cur_bssid, ETHER_ADDR_LEN);
	printf("\nBeacon period:\t\t");
	an_printwords(&sts->an_beacon_period, 1);
	printf("\nDTIM period:\t\t");
	an_printwords(&sts->an_dtim_period, 1);
	printf("\nATIM duration:\t\t");
	an_printwords(&sts->an_atim_duration, 1);
	printf("\nHOP period:\t\t");
	an_printwords(&sts->an_hop_period, 1);
	printf("\nChannel set:\t\t");
	an_printwords(&sts->an_channel_set, 1);
	printf("\nCurrent channel:\t");
	an_printwords(&sts->an_cur_channel, 1);
	printf("\nHops to backbone:\t");
	an_printwords(&sts->an_hops_to_backbone, 1);
	printf("\nTotal AP load:\t\t");
	an_printwords(&sts->an_ap_total_load, 1);
	printf("\nOur generated load:\t");
	an_printwords(&sts->an_our_generated_load, 1);
	printf("\nAccumulated ARL:\t");
	an_printwords(&sts->an_accumulated_arl, 1);
	printf("\n");
	return;
}

static void an_dumpcaps(iface)
	char			*iface;
{
	struct an_ltv_caps	*caps;
	struct an_req		areq;
	u_int16_t		tmp;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_CAPABILITIES);

	caps = (struct an_ltv_caps *)&areq;

	printf("OUI:\t\t\t");
	an_printhex((char *)&caps->an_oui, 3);
	printf("\nProduct number:\t\t");
	an_printwords(&caps->an_prodnum, 1);
	printf("\nManufacturer name:\t");
	an_printstr((char *)&caps->an_manufname, 32);
	printf("\nProduce name:\t\t");
	an_printstr((char *)&caps->an_prodname, 16);
	printf("\nFirmware version:\t");
	an_printstr((char *)&caps->an_prodvers, 1);
	printf("\nOEM MAC address:\t");
	an_printhex((char *)&caps->an_oemaddr, ETHER_ADDR_LEN);
	printf("\nAironet MAC address:\t");
	an_printhex((char *)&caps->an_aironetaddr, ETHER_ADDR_LEN);
	printf("\nRadio type:\t\t[ ");
	if (caps->an_radiotype & AN_RADIOTYPE_80211_FH)
		printf("802.11 FH");
	else if (caps->an_radiotype & AN_RADIOTYPE_80211_DS)
		printf("802.11 DS");
	else if (caps->an_radiotype & AN_RADIOTYPE_LM2000_DS)
		printf("LM2000 DS");
	else
		printf("unknown (%x)", caps->an_radiotype);
	printf(" ]");
	printf("\nRegulatory domain:\t");
	an_printwords(&caps->an_regdomain, 1);
	printf("\nAssigned CallID:\t");
	an_printhex((char *)&caps->an_callid, 6);
	printf("\nSupported speeds:\t");
	an_printspeeds(caps->an_rates, 8);
	printf("\nRX Diversity:\t\t[ ");
	if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_1_ONLY)
		printf("antenna 1 only");
	else if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_2_ONLY)
		printf("antenna 2 only");
	else if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_1_AND_2)
		printf("antenna 1 and 2");
	printf(" ]");
	printf("\nTX Diversity:\t\t[ ");
	if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_1_ONLY)
		printf("antenna 1 only");
	else if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_2_ONLY)
		printf("antenna 2 only");
	else if (caps->an_rx_diversity == AN_DIVERSITY_ANTENNA_1_AND_2)
		printf("antenna 1 and 2");
	printf(" ]");
	printf("\nSupported power levels:\t");
	an_printwords(caps->an_tx_powerlevels, 8);
	printf("\nHardware revision:\t");
	tmp = ntohs(caps->an_hwrev);
	an_printhex((char *)&tmp, 2);
	printf("\nSoftware revision:\t");
	tmp = ntohs(caps->an_fwrev);
	an_printhex((char *)&tmp, 2);
	printf("\nSoftware subrevision:\t");
	tmp = ntohs(caps->an_fwsubrev);
	an_printhex((char *)&tmp, 2);
	printf("\nInterface revision:\t");
	tmp = ntohs(caps->an_ifacerev);
	an_printhex((char *)&tmp, 2);
	printf("\nBootblock revision:\t");
	tmp = ntohs(caps->an_bootblockrev);
	an_printhex((char *)&tmp, 2);
	printf("\n");
	return;
}

static void an_dumpstats(iface)
	char			*iface;
{
	struct an_ltv_stats	*stats;
	struct an_req		areq;
	caddr_t			ptr;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_32BITS_CUM);

	ptr = (caddr_t)&areq;
	ptr -= 2;
	stats = (struct an_ltv_stats *)ptr;

	printf("RX overruns:\t\t\t\t\t[ %d ]\n", stats->an_rx_overruns);
	printf("RX PLCP CSUM errors:\t\t\t\t[ %d ]\n",
	    stats->an_rx_plcp_csum_errs);
	printf("RX PLCP format errors:\t\t\t\t[ %d ]\n",
	    stats->an_rx_plcp_format_errs);
	printf("RX PLCP length errors:\t\t\t\t[ %d ]\n",
	    stats->an_rx_plcp_len_errs);
	printf("RX MAC CRC errors:\t\t\t\t[ %d ]\n",
	    stats->an_rx_mac_crc_errs);
	printf("RX MAC CRC OK:\t\t\t\t\t[ %d ]\n",
	    stats->an_rx_mac_crc_ok);
	printf("RX WEP errors:\t\t\t\t\t[ %d ]\n",
	    stats->an_rx_wep_errs);
	printf("RX WEP OK:\t\t\t\t\t[ %d ]\n",
	    stats->an_rx_wep_ok);
	printf("Long retries:\t\t\t\t\t[ %d ]\n",
	    stats->an_retry_long);
	printf("Short retries:\t\t\t\t\t[ %d ]\n",
	    stats->an_retry_short);
	printf("Retries exchausted:\t\t\t\t[ %d ]\n",
	    stats->an_retry_max);
	printf("Bad ACK:\t\t\t\t\t[ %d ]\n",
	    stats->an_no_ack);
	printf("Bad CTS:\t\t\t\t\t[ %d ]\n",
	    stats->an_no_cts);
	printf("RX good ACKs:\t\t\t\t\t[ %d ]\n",
	    stats->an_rx_ack_ok);
	printf("RX good CTSs:\t\t\t\t\t[ %d ]\n",
	    stats->an_rx_cts_ok);
	printf("TX good ACKs:\t\t\t\t\t[ %d ]\n",
	    stats->an_tx_ack_ok);
	printf("TX good RTSs:\t\t\t\t\t[ %d ]\n",
	    stats->an_tx_rts_ok);
	printf("TX good CTSs:\t\t\t\t\t[ %d ]\n",
	    stats->an_tx_cts_ok);
	printf("LMAC multicasts transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_lmac_mcasts);
	printf("LMAC broadcasts transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_lmac_bcasts);
	printf("LMAC unicast frags transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_lmac_ucast_frags);
	printf("LMAC unicasts transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_lmac_ucasts);
	printf("Beacons transmitted:\t\t\t\t[ %d ]\n",
	    stats->an_tx_beacons);
	printf("Beacons received:\t\t\t\t[ %d ]\n",
	    stats->an_rx_beacons);
	printf("Single transmit collisions:\t\t\t[ %d ]\n",
	    stats->an_tx_single_cols);
	printf("Multiple transmit collisions:\t\t\t[ %d ]\n",
	    stats->an_tx_multi_cols);
	printf("Transmits without deferrals:\t\t\t[ %d ]\n",
	    stats->an_tx_defers_no);
	printf("Transmits defered due to protocol:\t\t[ %d ]\n",
	    stats->an_tx_defers_prot);
	printf("Transmits defered due to energy detect:\t\t[ %d ]\n",
	    stats->an_tx_defers_energy);
	printf("RX duplicate frames/frags:\t\t\t[ %d ]\n",
	    stats->an_rx_dups);
	printf("RX partial frames:\t\t\t\t[ %d ]\n",
	    stats->an_rx_partial);
	printf("TX max lifetime exceeded:\t\t\t[ %d ]\n",
	    stats->an_tx_too_old);
	printf("RX max lifetime exceeded:\t\t\t[ %d ]\n",
	    stats->an_tx_too_old);
	printf("Sync lost due to too many missed beacons:\t[ %d ]\n",
	    stats->an_lostsync_missed_beacons);
	printf("Sync lost due to ARL exceeded:\t\t\t[ %d ]\n",
	    stats->an_lostsync_arl_exceeded);
	printf("Sync lost due to deauthentication:\t\t[ %d ]\n",
	    stats->an_lostsync_deauthed);
	printf("Sync lost due to disassociation:\t\t[ %d ]\n",
	    stats->an_lostsync_disassociated);
	printf("Sync lost due to excess change in TSF timing:\t[ %d ]\n",
	    stats->an_lostsync_tsf_timing);
	printf("Host transmitted multicasts:\t\t\t[ %d ]\n",
	    stats->an_tx_host_mcasts);
	printf("Host transmitted broadcasts:\t\t\t[ %d ]\n",
	    stats->an_tx_host_bcasts);
	printf("Host transmitted unicasts:\t\t\t[ %d ]\n",
	    stats->an_tx_host_ucasts);
	printf("Host transmission failures:\t\t\t[ %d ]\n",
	    stats->an_tx_host_failed);
	printf("Host received multicasts:\t\t\t[ %d ]\n",
	    stats->an_rx_host_mcasts);
	printf("Host received broadcasts:\t\t\t[ %d ]\n",
	    stats->an_rx_host_bcasts);
	printf("Host received unicasts:\t\t\t\t[ %d ]\n",
	    stats->an_rx_host_ucasts);
	printf("Host receive discards:\t\t\t\t[ %d ]\n",
	    stats->an_rx_host_discarded);
	printf("HMAC transmitted multicasts:\t\t\t[ %d ]\n",
	    stats->an_tx_hmac_mcasts);
	printf("HMAC transmitted broadcasts:\t\t\t[ %d ]\n",
	    stats->an_tx_hmac_bcasts);
	printf("HMAC transmitted unicasts:\t\t\t[ %d ]\n",
	    stats->an_tx_hmac_ucasts);
	printf("HMAC transmissions failed:\t\t\t[ %d ]\n",
	    stats->an_tx_hmac_failed);
	printf("HMAC received multicasts:\t\t\t[ %d ]\n",
	    stats->an_rx_hmac_mcasts);
	printf("HMAC received broadcasts:\t\t\t[ %d ]\n",
	    stats->an_rx_hmac_bcasts);
	printf("HMAC received unicasts:\t\t\t\t[ %d ]\n",
	    stats->an_rx_hmac_ucasts);
	printf("HMAC receive discards:\t\t\t\t[ %d ]\n",
	    stats->an_rx_hmac_discarded);
	printf("HMAC transmits accepted:\t\t\t[ %d ]\n",
	    stats->an_tx_hmac_accepted);
	printf("SSID mismatches:\t\t\t\t[ %d ]\n",
	    stats->an_ssid_mismatches);
	printf("Access point mismatches:\t\t\t[ %d ]\n",
	    stats->an_ap_mismatches);
	printf("Speed mismatches:\t\t\t\t[ %d ]\n",
	    stats->an_rates_mismatches);
	printf("Authentication rejects:\t\t\t\t[ %d ]\n",
	    stats->an_auth_rejects);
	printf("Authentication timeouts:\t\t\t[ %d ]\n",
	    stats->an_auth_timeouts);
	printf("Association rejects:\t\t\t\t[ %d ]\n",
	    stats->an_assoc_rejects);
	printf("Association timeouts:\t\t\t\t[ %d ]\n",
	    stats->an_assoc_timeouts);
	printf("Management frames received:\t\t\t[ %d ]\n",
	    stats->an_rx_mgmt_pkts);
	printf("Management frames transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_mgmt_pkts);
	printf("Refresh frames received:\t\t\t[ %d ]\n",
	    stats->an_rx_refresh_pkts),
	printf("Refresh frames transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_refresh_pkts),
	printf("Poll frames received:\t\t\t\t[ %d ]\n",
	    stats->an_rx_poll_pkts);
	printf("Poll frames transmitted:\t\t\t[ %d ]\n",
	    stats->an_tx_poll_pkts);
	printf("Host requested sync losses:\t\t\t[ %d ]\n",
	    stats->an_lostsync_hostreq);
	printf("Host transmitted bytes:\t\t\t\t[ %d ]\n",
	    stats->an_host_tx_bytes);
	printf("Host received bytes:\t\t\t\t[ %d ]\n",
	    stats->an_host_rx_bytes);
	printf("Uptime in microseconds:\t\t\t\t[ %d ]\n",
	    stats->an_uptime_usecs);
	printf("Uptime in seconds:\t\t\t\t[ %d ]\n",
	    stats->an_uptime_secs);
	printf("Sync lost due to better AP:\t\t\t[ %d ]\n",
	    stats->an_lostsync_better_ap);

	return;
}

static void an_dumpap(iface)
	char			*iface;
{
	struct an_ltv_aplist	*ap;
	struct an_req		areq;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_APLIST);

	ap = (struct an_ltv_aplist *)&areq;
	printf("Access point 1:\t\t\t");
	an_printhex((char *)&ap->an_ap1, ETHER_ADDR_LEN);
	printf("\nAccess point 2:\t\t\t");
	an_printhex((char *)&ap->an_ap2, ETHER_ADDR_LEN);
	printf("\nAccess point 3:\t\t\t");
	an_printhex((char *)&ap->an_ap3, ETHER_ADDR_LEN);
	printf("\nAccess point 4:\t\t\t");
	an_printhex((char *)&ap->an_ap4, ETHER_ADDR_LEN);
	printf("\n");

	return;
}

static void an_dumpssid(iface)
	char			*iface;
{
	struct an_ltv_ssidlist	*ssid;
	struct an_req		areq;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_SSIDLIST);

	ssid = (struct an_ltv_ssidlist *)&areq;
	printf("SSID 1:\t\t\t[ %.*s ]\n", ssid->an_ssid1_len, ssid->an_ssid1);
	printf("SSID 2:\t\t\t[ %.*s ]\n", ssid->an_ssid2_len, ssid->an_ssid2);
	printf("SSID 3:\t\t\t[ %.*s ]\n", ssid->an_ssid3_len, ssid->an_ssid3);

	return;
}

static void an_dumpconfig(iface)
	char			*iface;
{
	struct an_ltv_genconfig	*cfg;
	struct an_req		areq;
	unsigned char		div;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_ACTUALCFG);

	cfg = (struct an_ltv_genconfig *)&areq;

	printf("Operating mode:\t\t\t\t[ ");
	if ((cfg->an_opmode & 0x7) == AN_OPMODE_IBSS)
		printf("ad-hoc");
	if ((cfg->an_opmode & 0x7) == AN_OPMODE_ESS)
		printf("infrastructure");
	if ((cfg->an_opmode & 0x7) == AN_OPMODE_AP)
		printf("access point");
	if ((cfg->an_opmode & 0x7) == AN_OPMODE_AP_REPEATER)
		printf("access point repeater");
	printf(" ]");
	printf("\nReceive mode:\t\t\t\t[ ");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_BC_MC_ADDR)
		printf("broadcast/multicast/unicast");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_BC_ADDR)
		printf("broadcast/unicast");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_ADDR)
		printf("unicast");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_80211_MONITOR_CURBSS)
		printf("802.11 monitor, current BSSID");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_80211_MONITOR_ANYBSS)
		printf("802.11 monitor, any BSSID");
	if ((cfg->an_rxmode & 0x7) == AN_RXMODE_LAN_MONITOR_CURBSS)
		printf("LAN monitor, current BSSID");
	printf(" ]");
	printf("\nFragment threshold:\t\t\t");
	an_printwords(&cfg->an_fragthresh, 1);
	printf("\nRTS threshold:\t\t\t\t");
	an_printwords(&cfg->an_rtsthresh, 1);
	printf("\nMAC address:\t\t\t\t");
	an_printhex((char *)&cfg->an_macaddr, ETHER_ADDR_LEN);
	printf("\nSupported rates:\t\t\t");
	an_printspeeds(cfg->an_rates, 8);
	printf("\nShort retry limit:\t\t\t");
	an_printwords(&cfg->an_shortretry_limit, 1);
	printf("\nLong retry limit:\t\t\t");
	an_printwords(&cfg->an_longretry_limit, 1);
	printf("\nTX MSDU lifetime:\t\t\t");
	an_printwords(&cfg->an_tx_msdu_lifetime, 1);
	printf("\nRX MSDU lifetime:\t\t\t");
	an_printwords(&cfg->an_rx_msdu_lifetime, 1);
	printf("\nStationary:\t\t\t\t");
	an_printbool(cfg->an_stationary);
	printf("\nOrdering:\t\t\t\t");
	an_printbool(cfg->an_ordering);
	printf("\nDevice type:\t\t\t\t[ ");
	if (cfg->an_devtype == AN_DEVTYPE_PC4500)
		printf("PC4500");
	else if (cfg->an_devtype == AN_DEVTYPE_PC4800)
		printf("PC4800");
	else
		printf("unknown (%x)", cfg->an_devtype);
	printf(" ]");
	printf("\nScanning mode:\t\t\t\t[ ");
	if (cfg->an_scanmode == AN_SCANMODE_ACTIVE)
		printf("active");
	if (cfg->an_scanmode == AN_SCANMODE_PASSIVE)
		printf("passive");
	if (cfg->an_scanmode == AN_SCANMODE_AIRONET_ACTIVE)
		printf("Aironet active");
	printf(" ]");
	printf("\nProbe delay:\t\t\t\t");
	an_printwords(&cfg->an_probedelay, 1);
	printf("\nProbe energy timeout:\t\t\t");
	an_printwords(&cfg->an_probe_energy_timeout, 1);
	printf("\nProbe response timeout:\t\t\t");
	an_printwords(&cfg->an_probe_response_timeout, 1);
	printf("\nBeacon listen timeout:\t\t\t");
	an_printwords(&cfg->an_beacon_listen_timeout, 1);
	printf("\nIBSS join network timeout:\t\t");
	an_printwords(&cfg->an_ibss_join_net_timeout, 1);
	printf("\nAuthentication timeout:\t\t\t");
	an_printwords(&cfg->an_auth_timeout, 1);
	printf("\nAuthentication type:\t\t\t[ ");
	if (cfg->an_authtype == AN_AUTHTYPE_NONE)
		printf("no auth");
	if (cfg->an_authtype == AN_AUTHTYPE_OPEN)
		printf("open");
	if (cfg->an_authtype == AN_AUTHTYPE_SHAREDKEY)
		printf("shared key");
	if (cfg->an_authtype == AN_AUTHTYPE_EXCLUDE_UNENCRYPTED)
		printf("exclude unencrypted");
	printf(" ]");
	printf("\nAssociation timeout:\t\t\t");
	an_printwords(&cfg->an_assoc_timeout, 1);
	printf("\nSpecified AP association timeout:\t");
	an_printwords(&cfg->an_specified_ap_timeout, 1);
	printf("\nOffline scan interval:\t\t\t");
	an_printwords(&cfg->an_offline_scan_interval, 1);
	printf("\nOffline scan duration:\t\t\t");
	an_printwords(&cfg->an_offline_scan_duration, 1);
	printf("\nLink loss delay:\t\t\t");
	an_printwords(&cfg->an_link_loss_delay, 1);
	printf("\nMax beacon loss time:\t\t\t");
	an_printwords(&cfg->an_max_beacon_lost_time, 1);
	printf("\nRefresh interval:\t\t\t");
	an_printwords(&cfg->an_refresh_interval, 1);
	printf("\nPower save mode:\t\t\t[ ");
	if (cfg->an_psave_mode == AN_PSAVE_NONE)
		printf("none");
	if (cfg->an_psave_mode == AN_PSAVE_CAM)
		printf("constantly awake mode");
	if (cfg->an_psave_mode == AN_PSAVE_PSP)
		printf("PSP");
	if (cfg->an_psave_mode == AN_PSAVE_PSP_CAM)
		printf("PSP-CAM (fast PSP)");
	printf(" ]");
	printf("\nSleep through DTIMs:\t\t\t");
	an_printbool(cfg->an_sleep_for_dtims);
	printf("\nPower save listen interval:\t\t");
	an_printwords(&cfg->an_listen_interval, 1);
	printf("\nPower save fast listen interval:\t");
	an_printwords(&cfg->an_fast_listen_interval, 1);
	printf("\nPower save listen decay:\t\t");
	an_printwords(&cfg->an_listen_decay, 1);
	printf("\nPower save fast listen decay:\t\t");
	an_printwords(&cfg->an_fast_listen_decay, 1);
	printf("\nAP/ad-hoc Beacon period:\t\t");
	an_printwords(&cfg->an_beacon_period, 1);
	printf("\nAP/ad-hoc ATIM duration:\t\t");
	an_printwords(&cfg->an_atim_duration, 1);
	printf("\nAP/ad-hoc current channel:\t\t");
	an_printwords(&cfg->an_ds_channel, 1);
	printf("\nAP/ad-hoc DTIM period:\t\t\t");
	an_printwords(&cfg->an_dtim_period, 1);
	printf("\nRadio type:\t\t\t\t[ ");
	if (cfg->an_radiotype & AN_RADIOTYPE_80211_FH)
		printf("802.11 FH");
	else if (cfg->an_radiotype & AN_RADIOTYPE_80211_DS)
		printf("802.11 DS");
	else if (cfg->an_radiotype & AN_RADIOTYPE_LM2000_DS)
		printf("LM2000 DS");
	else
		printf("unknown (%x)", cfg->an_radiotype);
	printf(" ]");
	printf("\nRX Diversity:\t\t\t\t[ ");
	div = cfg->an_diversity & 0xFF;
	if (div == AN_DIVERSITY_ANTENNA_1_ONLY)
		printf("antenna 1 only");
	else if (div == AN_DIVERSITY_ANTENNA_2_ONLY)
		printf("antenna 2 only");
	else if (div == AN_DIVERSITY_ANTENNA_1_AND_2)
		printf("antenna 1 and 2");
	printf(" ]");
	printf("\nTX Diversity:\t\t\t\t[ ");
	div = (cfg->an_diversity >> 8) & 0xFF;
	if (div == AN_DIVERSITY_ANTENNA_1_ONLY)
		printf("antenna 1 only");
	else if (div == AN_DIVERSITY_ANTENNA_2_ONLY)
		printf("antenna 2 only");
	else if (div == AN_DIVERSITY_ANTENNA_1_AND_2)
		printf("antenna 1 and 2");
	printf(" ]");
	printf("\nTransmit power level:\t\t\t");
	an_printwords(&cfg->an_tx_power, 1);
	printf("\nRSS threshold:\t\t\t\t");
	an_printwords(&cfg->an_rss_thresh, 1);
	printf("\nNode name:\t\t\t\t");
	an_printstr((char *)&cfg->an_nodename, 16);
	printf("\nARL threshold:\t\t\t\t");
	an_printwords(&cfg->an_arl_thresh, 1);
	printf("\nARL decay:\t\t\t\t");
	an_printwords(&cfg->an_arl_decay, 1);
	printf("\nARL delay:\t\t\t\t");
	an_printwords(&cfg->an_arl_delay, 1);

	printf("\n");

	return;
}


static void usage(p)
	char			*p;
{
	fprintf(stderr, "usage:\t%s [-i iface] command\n", p);
	fprintf(stderr, "Where the command is one of the following:\n");
	fprintf(stderr, "\t-i iface -A (show specified APs)\n");
	fprintf(stderr, "\t-i iface -C (show current config)\n");
	fprintf(stderr, "\t-i iface -I (show NIC capabilities)\n");
	fprintf(stderr, "\t-i iface -N (show specified SSIDss)\n");
	fprintf(stderr, "\t-i iface -S (show NIC status)\n");
	fprintf(stderr, "\t-i iface -T (show stats counters)\n");
	fprintf(stderr, "\t-i iface -a AP -v 1|2|3|4 (specify AP)\n");
	fprintf(stderr, "\t-i iface -b val (set beacon period)\n");
	fprintf(stderr, "\t-i iface -c val (set ad-hoc channel)\n");
	fprintf(stderr, "\t-i iface -d 0|1|2|3 -v 0|1 val (set diversity)\n");
	fprintf(stderr, "\t-i iface -f val (set frag threshold)\n");
	fprintf(stderr, "\t-i iface -j val (set netjoin timeout)\n");
	fprintf(stderr, "\t-i iface -l val (set station name)\n");
	fprintf(stderr, "\t-i iface -m val (set MAC address)\n");
	fprintf(stderr, "\t-i iface -n SSID -v 1|2|3 " "(specify SSID)\n");
	fprintf(stderr, "\t-i iface -o 0|1 (set operating mode)\n");
	fprintf(stderr, "\t-i iface -p val (set tx power)\n");
	fprintf(stderr, "\t-i iface -r val (set RTS threshold)\n");
	fprintf(stderr, "\t-i iface -s 0|1|2|3 (set power same mode)\n");
#if 0
	fprintf(stderr, "\t-i iface -t 0|1|2|3|4 (set TX speed)\n");
#endif
	fprintf(stderr, "\t-i iface -w val (set wake duration (ATIM))\n");
	fprintf(stderr, "\t-i iface -x [0-7] (authtype)\n");
	fprintf(stderr, "\t-h (display this message)\n");

	exit(1);
}

static void an_setconfig(iface, act, arg)
	char			*iface;
	int			act;
	void			*arg;
{
	struct an_ltv_genconfig	*cfg;
	struct an_ltv_caps	*caps;
	struct an_req		areq;
	struct an_req		areq_caps;
	u_int16_t		diversity = 0;
	struct ether_addr	*addr;
	int			i;

	areq.an_len = sizeof(areq);
	an_getval(iface, &areq, AN_RID_GENCONFIG);
	cfg = (struct an_ltv_genconfig *)&areq;

	areq_caps.an_len = sizeof(areq);
	an_getval(iface, &areq_caps, AN_RID_CAPABILITIES);
	caps = (struct an_ltv_caps *)&areq_caps;

	switch(act) {
	case ACT_SET_OPMODE:
		cfg->an_opmode = atoi(arg);
		break;
	case ACT_SET_FREQ:
		cfg->an_ds_channel = atoi(arg);
		break;
	case ACT_SET_PWRSAVE:
		cfg->an_psave_mode = atoi(arg);
		break;
	case ACT_SET_SCANMODE:
		cfg->an_scanmode = atoi(arg);
		break;
	case ACT_SET_AUTHTYPE:
		cfg->an_authtype = atoi(arg);
		break;
	case ACT_SET_DIVERSITY_RX:
	case ACT_SET_DIVERSITY_TX:
		switch(atoi(arg)) {
		case 0:
			diversity = AN_DIVERSITY_FACTORY_DEFAULT;
			break;
		case 1:
			diversity = AN_DIVERSITY_ANTENNA_1_ONLY;
			break;
		case 2:
			diversity = AN_DIVERSITY_ANTENNA_2_ONLY;
			break;
		case 3:
			diversity = AN_DIVERSITY_ANTENNA_1_AND_2;
			break;
		default:
			errx(1, "bad diversity setting: %d", diversity);
			break;
		}
		if (atoi(arg) == ACT_SET_DIVERSITY_RX) {
			cfg->an_diversity &= 0x00FF;
			cfg->an_diversity |= (diversity << 8);
		} else {
			cfg->an_diversity &= 0xFF00;
			cfg->an_diversity |= diversity;
		}
		break;
	case ACT_SET_TXPWR:
		for (i = 0; i < 8; i++) {
			if (caps->an_tx_powerlevels[i] == atoi(arg))
				break;
		}
		if (i == 8)
			errx(1, "unsupported power level: %dmW", atoi(arg));

		cfg->an_tx_power = atoi(arg);
		break;
	case ACT_SET_RTS_THRESH:
		cfg->an_rtsthresh = atoi(arg);
		break;
	case ACT_SET_RTS_RETRYLIM:
		cfg->an_shortretry_limit =
		   cfg->an_longretry_limit = atoi(arg);
		break;
	case ACT_SET_BEACON_PERIOD:
		cfg->an_beacon_period = atoi(arg);
		break;
	case ACT_SET_WAKE_DURATION:
		cfg->an_atim_duration = atoi(arg);
		break;
	case ACT_SET_FRAG_THRESH:
		cfg->an_fragthresh = atoi(arg);
		break;
	case ACT_SET_NETJOIN:
		cfg->an_ibss_join_net_timeout = atoi(arg);
		break;
	case ACT_SET_MYNAME:
		bzero(cfg->an_nodename, 16);
		strncpy((char *)&cfg->an_nodename, optarg, 16);
		break;
	case ACT_SET_MAC:
		addr = ether_aton((char *)arg);

		if (addr == NULL)
			errx(1, "badly formatted address");
		bzero(cfg->an_macaddr, ETHER_ADDR_LEN);
		bcopy((char *)addr, (char *)&cfg->an_macaddr, ETHER_ADDR_LEN);
		break;
	default:
		errx(1, "unknown action");
		break;
	}

	an_setval(iface, &areq, AN_RID_GENCONFIG);
	exit(0);
}

#if 0
static void an_setspeed(iface, act, arg)
	char			*iface;
	int			act;
	void			*arg;
{
	struct an_req		areq;
	struct an_ltv_caps	*caps;
	u_int16_t		speed;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_CAPABILITIES);
	caps = (struct an_ltv_caps *)&areq;

	switch(atoi(arg)) {
	case 0:
		speed = 0;
		break;
	case 1:
		speed = AN_RATE_1MBPS;
		break;
	case 2:
		speed = AN_RATE_2MBPS;
		break;
	case 3:
		if (caps->an_rates[2] != AN_RATE_5_5MBPS)
			errx(1, "5.5Mbps not supported on this card");
		speed = AN_RATE_5_5MBPS;
		break;
	case 4:
		if (caps->an_rates[3] != AN_RATE_11MBPS)
			errx(1, "11Mbps not supported on this card");
		speed = AN_RATE_11MBPS;
		break;
	default:
		errx(1, "unsupported speed");
		break;
	}

	areq.an_len = 6;
	areq.an_val[0] = speed;

	an_setval(iface, &areq, AN_RID_TX_SPEED);
	exit(0);
}
#endif

static void an_setap(iface, act, arg)
	char			*iface;
	int			act;
	void			*arg;
{
	struct an_ltv_aplist	*ap;
	struct an_req		areq;
	struct ether_addr	*addr;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_APLIST);
	ap = (struct an_ltv_aplist *)&areq;

	addr = ether_aton((char *)arg);

	if (addr == NULL)
		errx(1, "badly formatted address");

	switch(act) {
	case ACT_SET_AP1:
		bzero(ap->an_ap1, ETHER_ADDR_LEN);
		bcopy((char *)addr, (char *)&ap->an_ap1, ETHER_ADDR_LEN);
		break;
	case ACT_SET_AP2:
		bzero(ap->an_ap2, ETHER_ADDR_LEN);
		bcopy((char *)addr, (char *)&ap->an_ap2, ETHER_ADDR_LEN);
		break;
	case ACT_SET_AP3:
		bzero(ap->an_ap3, ETHER_ADDR_LEN);
		bcopy((char *)addr, (char *)&ap->an_ap3, ETHER_ADDR_LEN);
		break;
	case ACT_SET_AP4:
		bzero(ap->an_ap4, ETHER_ADDR_LEN);
		bcopy((char *)addr, (char *)&ap->an_ap4, ETHER_ADDR_LEN);
		break;
	default:
		errx(1, "unknown action");
		break;
	}

	an_setval(iface, &areq, AN_RID_APLIST);
	exit(0);
}

static void an_setssid(iface, act, arg)
	char			*iface;
	int			act;
	void			*arg;
{
	struct an_ltv_ssidlist	*ssid;
	struct an_req		areq;

	areq.an_len = sizeof(areq);

	an_getval(iface, &areq, AN_RID_SSIDLIST);
	ssid = (struct an_ltv_ssidlist *)&areq;

	switch(act) {
	case ACT_SET_SSID1:
		bzero(ssid->an_ssid1, sizeof(ssid->an_ssid1));
		bcopy((char *)arg, (char *)&ssid->an_ssid1,
		    strlen((char *)arg));
		ssid->an_ssid1_len = strlen((char *)arg);
		break;
	case ACT_SET_SSID2:
		bzero(ssid->an_ssid2, sizeof(ssid->an_ssid2));
		bcopy((char *)arg, (char *)&ssid->an_ssid2,
		    strlen((char *)arg));
		ssid->an_ssid2_len = strlen((char *)arg);
		break;
	case ACT_SET_SSID3:
		bzero(ssid->an_ssid3, sizeof(ssid->an_ssid3));
		bcopy((char *)arg, (char *)&ssid->an_ssid3,
		    strlen((char *)arg));
		ssid->an_ssid3_len = strlen((char *)arg);
		break;
	default:
		errx(1, "unknown action");
		break;
	}

	an_setval(iface, &areq, AN_RID_SSIDLIST);
}

int anctl(argc, argv)
	int			argc;
	char			*argv[];
{
	int			ch;
	int			cmd = 's';
	char			*iface = "an0";
	int			mib[6];
	char			buf[64];
	size_t			len;

	mib[0] = CTL_NET;
	mib[1] = PF_LINK;
	mib[2] = 8;
	mib[3] = CTL_LINK_LINKTYPE;
	mib[4] = IFT_IEEE80211;
	mib[5] = IEEE_802_11_SCTL_STATION;
	len = sizeof(buf) - 1;
	if (sysctl(mib, 6, buf, &len, NULL, 0) != 0)
		err(1, "sysctl (%d/%s)", len, buf);
	buf[len] = 0;
	printf("%d %s\n", len, buf);
	exit(0);
	while ((ch = getopt(argc, argv, "I:Sais")) != -1) {
		switch (ch) {
		case 'I':
			iface = optarg;
			break;

		case 'a':
		case 'i':
		case 's':
		case 'S':
			cmd = ch;
			break;
		default: usage:
			fprintf(stderr,
			    "usage: anctl [-ais] [-I iface] [SSID [...]]\n");
			exit(1);
		}
	}
	if (cmd == 's') {
		if (optind != argc)
			goto usage;
		an_dumpstatus(iface);
		return (0);
	}
	if (cmd == 'S') {
		if (optind != argc)
			goto usage;
		an_strength(iface);
		return (0);
	}
	if ((optind == argc && cmd != 'i') || optind + 3 < argc)
		goto usage;

	ch = ACT_SET_SSID1;
	while (optind < argc)
		an_setssid(iface, ch++, argv[optind++]);
	while (ch <= ACT_SET_SSID3)
		an_setssid(iface, ch++, "");

	if (cmd == 'a')
		an_setconfig(iface, ACT_SET_OPMODE, "0");
	else
		an_setconfig(iface, ACT_SET_OPMODE, "1");

	return (0);
}

int main(argc, argv)
	int			argc;
	char			*argv[];
{
	int			ch;
	int			act = 0;
	char			*iface = "an0";
	int			modifier = 0;
	void			*arg = NULL;
	char			*p;

	if ((p = strrchr(argv[0], '/')) != NULL)
		++p;
	else
		p = argv[0];

	if (strcmp(p, "anctl") == 0)
		return (anctl(argc, argv));

	if (argc > 1 && strcmp(argv[1], "-ctl") == 0) {
		argv[1] = argv[0];
		return (anctl(argc-1, argv+1));
	}

	while ((ch = getopt(argc, argv,
	    "i:ACINSTha:b:c:d:f:j:l:m:n:o:p:r:s:t:v:w:x:")) != -1) {
		switch(ch) {
		case 'i':
			iface = optarg;
			break;
		case 'A':
			act = ACT_DUMPAP;
			break;
		case 'C':
			act = ACT_DUMPCONFIG;
			break;
		case 'I':
			act = ACT_DUMPCAPS;
			break;
		case 'N':
			act = ACT_DUMPSSID;
			break;
		case 'S':
			act = ACT_DUMPSTATUS;
			break;
		case 'T':
			act = ACT_DUMPSTATS;
			break;
		case 'a':
			switch(modifier) {
			case 0:
			case 1:
				act = ACT_SET_AP1;
				break;
			case 2:
				act = ACT_SET_AP2;
				break;
			case 3:
				act = ACT_SET_AP3;
				break;
			case 4:
				act = ACT_SET_AP4;
				break;
			default:
				errx(1, "bad modifier %d: there "
				    "are only 4 access point settings",
				    modifier);
				usage(p);
				break;
			}
			arg = optarg;
			break;
		case 'b':
			act = ACT_SET_BEACON_PERIOD;
			arg = optarg;
			break;
		case 'c':
			act = ACT_SET_FREQ;
			arg = optarg;
			break;
		case 'd':
			switch(modifier) {
			case 0:
				act = ACT_SET_DIVERSITY_RX;
				break;
			case 1:
				act = ACT_SET_DIVERSITY_TX;
				break;
			default:
				errx(1, "must specift RX or TX diversity");
				break;
			}
			arg = optarg;
			break;
		case 'f':
			act = ACT_SET_FRAG_THRESH;
			arg = optarg;
			break;
		case 'j':
			act = ACT_SET_NETJOIN;
			arg = optarg;
			break;
		case 'l':
			act = ACT_SET_MYNAME;
			arg = optarg;
			break;
		case 'm':
			act = ACT_SET_MAC;
			arg = optarg;
			break;
		case 'n':
			switch(modifier) {
			case 0:
			case 1:
				act = ACT_SET_SSID1;
				break;
			case 2:
				act = ACT_SET_SSID2;
				break;
			case 3:
				act = ACT_SET_SSID3;
				break;
			default:
				errx(1, "bad modifier %d: there"
				    "are only 3 SSID settings", modifier);
				usage(p);
				break;
			}
			arg = optarg;
			break;
		case 'o':
			act = ACT_SET_OPMODE;
			arg = optarg;
			break;
		case 'p':
			act = ACT_SET_TXPWR;
			arg = optarg;
			break;
		case 'q':
			act = ACT_SET_RTS_RETRYLIM;
			arg = optarg;
			break;
		case 'r':
			act = ACT_SET_RTS_THRESH;
			arg = optarg;
			break;
		case 's':
			act = ACT_SET_PWRSAVE;
			arg = optarg;
			break;
#if 0
		case 't':
			act = ACT_SET_TXRATE;
			arg = optarg;
			break;
#endif
		case 'v':
			modifier = atoi(optarg);
			break;
		case 'w':
			act = ACT_SET_WAKE_DURATION;
			arg = optarg;
			break;
		case 'x':
			act = ACT_SET_AUTHTYPE;
			arg = optarg;
			break;
		case 'h':
		default:
			usage(p);
		}
	}

	if (iface == NULL || !act)
		usage(p);

	switch(act) {
	case ACT_DUMPSTATUS:
		an_dumpstatus(iface);
		break;
	case ACT_DUMPCAPS:
		an_dumpcaps(iface);
		break;
	case ACT_DUMPSTATS:
		an_dumpstats(iface);
		break;
	case ACT_DUMPCONFIG:
		an_dumpconfig(iface);
		break;
	case ACT_DUMPSSID:
		an_dumpssid(iface);
		break;
	case ACT_DUMPAP:
		an_dumpap(iface);
		break;
	case ACT_SET_SSID1:
	case ACT_SET_SSID2:
	case ACT_SET_SSID3:
		an_setssid(iface, act, arg);
		break;
	case ACT_SET_AP1:
	case ACT_SET_AP2:
	case ACT_SET_AP3:
	case ACT_SET_AP4:
		an_setap(iface, act, arg);
		break;
#if	0
	case ACT_SET_TXRATE:
		an_setspeed(iface, act, arg);
		break;
#endif
	default:
		an_setconfig(iface, act, arg);
		break;
	}

	exit(0);
}
