/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 * Modified for BSD/OS by Geert Jan de Groot
 *
 *	Id: wicontrol.c,v 1.6 1999/05/22 16:12:49 wpaul Exp 
 * 	BSDI wicontrol.c,v 1.7 2002/03/19 21:44:19 geertj Exp
 *
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_802_11.h>

#include <i386/isa/if_wireg.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#if !defined(lint)
static const char copyright[] = "@(#) Copyright (c) 1997, 1998, 1999\
	Bill Paul. All rights reserved.";
#endif

static void wi_getval		__P((char *, struct wi_ltv *));
static void wi_setval		__P((char *, struct wi_ltv *));
static void wi_printstr		__P((struct wi_ltv *));
static void wi_setstr		__P((char *, int, char *));
static void wi_setbytes		__P((char *, int, char *, int));
static void wi_setword		__P((char *, int, int));
static void wi_sethex		__P((char *, int, char *));
static void wi_printwords	__P((struct wi_ltv *));
static void wi_printbool	__P((struct wi_ltv *));
static void wi_printhex		__P((struct wi_ltv *));
static void wi_dumpinfo		__P((char *));
static void usage		__P((char *));

#define ETHER_ADDR_LEN	6

static void wi_getval(iface, wreq)
	char			*iface;
	struct wi_ltv		*wreq;
{
	struct ifreq		ifr;
	int			s;

	bzero((char *)&ifr, sizeof(ifr));

	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = 0;
	ifr.ifr_data = (caddr_t)wreq;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	if (ioctl(s, SIOCGWAVELAN, (caddr_t)&ifr) < 0)
		err(1, "SIOCGWAVELAN");

	close(s);

	return;
}

static void wi_setval(iface, wreq)
	char			*iface;
	struct wi_ltv		*wreq;
{
	struct ifreq		ifr;
	int			s;

	bzero((char *)&ifr, sizeof(ifr));

	strncpy(ifr.ifr_name, iface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = 0;
	ifr.ifr_data = (caddr_t)wreq;


	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	if (ioctl(s, SIOCSWAVELAN, &ifr) < 0)
		err(1, "SIOCSWAVELAN");

	close(s);

	return;
}

void wi_printstr(wreq)
	struct wi_ltv		*wreq;
{
	char			*ptr;
	int			i;

	if (wreq->wi_type == WI_RID_SERIALNO) {
		ptr = (char *)&wreq->wi_val;
		for (i = 0; i < (wreq->wi_len - 1) * 2; i++) {
			if (ptr[i] == '\0')
				ptr[i] = ' ';
		}
	} else {
		ptr = (char *)&wreq->wi_val[1];
		for (i = 0; i < wreq->wi_val[0]; i++) {
			if (ptr[i] == '\0')
				ptr[i] = ' ';
		}
	}

	ptr[i] = '\0';
	printf("[ %s ]", ptr);

	return;
}

void wi_setstr(iface, code, str)
	char			*iface;
	int			code;
	char			*str;
{
	struct wi_ltv		wreq;

	if (iface == NULL)
		errx(1, "must specify interface name");

	if (str == NULL)
		errx(1, "must specify string");

	bzero((char *)&wreq, sizeof(wreq));

	if (strlen(str) > 30)
		errx(1, "string too long");

	wreq.wi_type = code;
	wreq.wi_len = 18;
	wreq.wi_val[0] = strlen(str);
	bcopy(str, (char *)&wreq.wi_val[1], strlen(str));

	wi_setval(iface, &wreq);

	return;
}

void wi_setbytes(iface, code, bytes, len)
	char			*iface;
	int			code;
	char			*bytes;
	int			len;
{
	struct wi_ltv		wreq;

	if (iface == NULL)
		errx(1, "must specify interface name");

	bzero((char *)&wreq, sizeof(wreq));

	wreq.wi_type = code;
	wreq.wi_len = (len / 2) + 1;
	bcopy(bytes, (char *)&wreq.wi_val[0], len);

	wi_setval(iface, &wreq);

	return;
}

void wi_setword(iface, code, word)
	char			*iface;
	int			code;
	int			word;
{
	struct wi_ltv		wreq;

	bzero((char *)&wreq, sizeof(wreq));

	wreq.wi_type = code;
	wreq.wi_len = 2;
	wreq.wi_val[0] = word;

	wi_setval(iface, &wreq);

	return;
}

void wi_sethex(iface, code, str)
	char			*iface;
	int			code;
	char			*str;
{
	unsigned char		addr[ETHER_ADDR_LEN];
	char			*nxt = str;
	int			i;

	if (str == NULL)
		errx(1, "must specify address");

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		str = strsep(&nxt, ":");
		if (!str)
			errx(1, "Short 802 MAC address");
		addr[i] = strtoul(str, NULL, 16);
	}
	str = strsep(&nxt, ":");
	if (str)
		errx(1, "Invalid MAC address");

	wi_setbytes(iface, code, (char *)addr, ETHER_ADDR_LEN);

	return;
}

void wi_printwords(wreq)
	struct wi_ltv		*wreq;
{
	int			i;

	printf("[ ");
	for (i = 0; i < wreq->wi_len - 1; i++)
		printf("%d ", wreq->wi_val[i]);
	printf("]");

	return;
}

void wi_printbool(wreq)
	struct wi_ltv		*wreq;
{
	if (wreq->wi_val[0])
		printf("[ On ]");
	else
		printf("[ Off ]");

	return;
}

void wi_printhex(wreq)
	struct wi_ltv		*wreq;
{
	int			i;
	unsigned char		*c;

	c = (unsigned char *)&wreq->wi_val;

	printf("[ ");
	for (i = 0; i < (wreq->wi_len - 1) * 2; i++) {
		printf("%02x", c[i]);
		if (i < ((wreq->wi_len - 1) * 2) - 1)
			printf(":");
	}

	printf(" ]");
	return;
}

/*
 * The channel list returns a 16-bit channel mask of available channels
 * where bit 0 is channel 1, bit 2 is channel 2, etc.
 */
void wi_printchanmask(wreq)
	struct wi_ltv		*wreq;
{
	int			i, j, min;

	printf("[ ");
	for (i = 0; i < wreq->wi_len - 1; i++) {
		min = -1;
		for (j = 0; j < 16; j++) {
			if (wreq->wi_val[i] & (1 << j)) {
				if (min < 0) {
					printf("%d", j + 1);
					min = j;
				}
			} else {
				if ((min >= 0) && (j > (min + 1)))
					printf("-%d ", j);
				min = -1;
			}
		}			
	}
	printf("]");
	return;
}


#define WI_STRING		0x01
#define WI_BOOL			0x02
#define WI_WORDS		0x03
#define WI_HEXBYTES		0x04
#define WI_CHANMASK		0x05

struct wi_table {
	int			wi_code;
	int			wi_type;
	char			*wi_str;
};

static struct wi_table wi_table[] = {
	{ WI_RID_SERIALNO, WI_STRING, "NIC serial number:\t\t\t" },
	{ WI_RID_NODENAME, WI_STRING, "Station name:\t\t\t\t" },
	{ WI_RID_OWN_SSID, WI_STRING, "SSID for IBSS creation:\t\t\t" },
	{ WI_RID_CURRENT_SSID, WI_STRING, "Current netname (SSID):\t\t\t" },
	{ WI_RID_DESIRED_SSID, WI_STRING, "Desired netname (SSID):\t\t\t" },
	{ WI_RID_CURRENT_BSSID, WI_HEXBYTES, "Current BSSID:\t\t\t\t" },
	{ WI_RID_CHANNEL_LIST, WI_CHANMASK, "Channel list:\t\t\t\t" },
	{ WI_RID_OWN_CHNL, WI_WORDS, "IBSS channel:\t\t\t\t" },
	{ WI_RID_CURRENT_CHAN, WI_WORDS, "Current channel:\t\t\t" },
	{ WI_RID_COMMS_QUALITY, WI_WORDS, "Comms quality/signal/noise:\t\t" },
	{ WI_RID_PROMISC, WI_BOOL, "Promiscuous mode:\t\t\t" },
	{ WI_RID_PORTTYPE, WI_WORDS, "Port type (1=BSS, 3=ad-hoc):\t\t"},
	{ WI_RID_MAC_NODE, WI_HEXBYTES, "MAC address:\t\t\t\t"},
	{ WI_RID_TX_RATE, WI_WORDS, "TX rate:\t\t\t\t"},
	{ WI_RID_RTS_THRESH, WI_WORDS, "RTS/CTS handshake threshold:\t\t"},
	{ WI_RID_CREATE_IBSS, WI_BOOL, "Create IBSS:\t\t\t\t" },
	{ WI_RID_SYSTEM_SCALE, WI_WORDS, "Access point density:\t\t\t" },
	{ WI_RID_PM_ENABLED, WI_WORDS, "Power Mgmt (1=on, 0=off):\t\t" },
	{ WI_RID_MAX_SLEEP, WI_WORDS, "Max sleep time:\t\t\t\t" },
	{ 0, NULL }
};

static void wi_dumpinfo(iface)
	char			*iface;
{
	static struct wi_ltv		wreq;
	int			i;
	struct wi_table		*w;

	w = wi_table;

	for (i = 0; w[i].wi_type; i++) {
		bzero((char *)&wreq, sizeof(wreq));

		wreq.wi_len = WI_MAX_DATALEN;
		wreq.wi_type = w[i].wi_code;

		wi_getval(iface, &wreq);
		printf("%s", w[i].wi_str);
		switch(w[i].wi_type) {
		case WI_STRING:
			wi_printstr(&wreq);
			break;
		case WI_WORDS:
			wi_printwords(&wreq);
			break;
		case WI_BOOL:
			wi_printbool(&wreq);
			break;
		case WI_HEXBYTES:
			wi_printhex(&wreq);
			break;
		case WI_CHANMASK:
			wi_printchanmask(&wreq);
			break;
		default:
			break;
		}	
		printf("\n");
	}

	return;
}

static void wi_dumpstats(iface)
	char			*iface;
{
	struct wi_ltv		wreq;
	struct wi_counters	*c;

	if (iface == NULL)
		errx(1, "must specify interface name");

	bzero((char *)&wreq, sizeof(wreq));
	wreq.wi_len = WI_MAX_DATALEN;
	wreq.wi_type = WI_RID_IFACE_STATS;

	wi_getval(iface, &wreq);

	c = (struct wi_counters *)&wreq.wi_val;

	printf("Transmitted unicast frames:\t\t%d\n",
	    c->wi_tx_unicast_frames);
	printf("Transmitted multicast frames:\t\t%d\n",
	    c->wi_tx_multicast_frames);
	printf("Transmitted fragments:\t\t\t%d\n",
	    c->wi_tx_fragments);
	printf("Transmitted unicast octets:\t\t%d\n",
	    c->wi_tx_unicast_octets);
	printf("Transmitted multicast octets:\t\t%d\n",
	    c->wi_tx_multicast_octets);
	printf("Single transmit retries:\t\t%d\n",
	    c->wi_tx_single_retries);
	printf("Multiple transmit retries:\t\t%d\n",
	    c->wi_tx_multi_retries);
	printf("Transmit retry limit exceeded:\t\t%d\n",
	    c->wi_tx_retry_limit);
	printf("Transmit discards:\t\t\t%d\n",
	    c->wi_tx_discards);
	printf("Transmit discards due to wrong SA:\t%d\n",
	    c->wi_tx_discards_wrong_sa);
	printf("Received unicast frames:\t\t%d\n",
	    c->wi_rx_unicast_frames);
	printf("Received multicast frames:\t\t%d\n",
	    c->wi_rx_multicast_frames);
	printf("Received fragments:\t\t\t%d\n",
	    c->wi_rx_fragments);
	printf("Received unicast octets:\t\t%d\n",
	    c->wi_rx_unicast_octets);
	printf("Received multicast octets:\t\t%d\n",
	    c->wi_rx_multicast_octets);
	printf("Receive FCS errors:\t\t\t%d\n",
	    c->wi_rx_fcs_errors);
	printf("Receive discards due to no buffer:\t%d\n",
	    c->wi_rx_discards_nobuf);
	printf("Can't decrypt WEP frame:\t\t%d\n",
	    c->wi_rx_WEP_cant_decrypt);
	printf("Received message fragments:\t\t%d\n",
	    c->wi_rx_msg_in_msg_frags);
	printf("Received message bad fragments:\t\t%d\n",
	    c->wi_rx_msg_in_bad_msg_frags);
	printf("Received WEP ICV errors:\t\t%d\n",
	    c->wi_rx_discards_WEPICVerror);
	printf("Received WEP excluded packets:\t\t%d\n",
	    c->wi_rx_discards_WEPexcluded);

	return;
}

static void usage(p)
	char			*p;
{
	fprintf(stderr, "usage:  %s [-i iface]\n", p);
	fprintf(stderr, "\t%s [-i iface] -o\n", p);
	fprintf(stderr, "\t%s [-i iface] -c 0|1\n", p);
	fprintf(stderr, "\t%s [-i iface] -q SSID\n", p);
	fprintf(stderr, "\t%s [-i iface] -d max data length\n", p);
	fprintf(stderr, "\t%s [-i iface] -r RTS threshold\n", p);
	fprintf(stderr, "\t%s [-i iface] -s acces point density\n", p);
	fprintf(stderr, "\t%s [-i iface] -P 0|1\n", p);
	fprintf(stderr, "\t%s [-i iface] -S max sleep duration\n", p);
	fprintf(stderr, "\t%s [-i iface] -Z zero out signal cache\n", p);
	fprintf(stderr, "\t%s [-i iface] -C print signal cache\n", p);

	exit(1);
}

static void wi_zerocache(iface)
	char			*iface;
{
	struct wi_ltv		wreq;

	if (iface == NULL)
		errx(1, "must specify interface name");

	bzero((char *)&wreq, sizeof(wreq));
	wreq.wi_len = 0;
	wreq.wi_type = WI_RID_ZERO_CACHE;

	wi_getval(iface, &wreq);
}

static void wi_readcache(iface)
	char			*iface;
{
	struct wi_ltv		wreq;
	int 			*wi_sigitems;
	struct wi_sigcache 	*sc;
	u_char			ibss[6];
	char			*pt;
	int 			i;
	

	if (iface == NULL)
		errx(1, "must specify interface name");

	bzero((char *)&wreq, sizeof(wreq));
	wreq.wi_len = WI_MAX_DATALEN;
	wreq.wi_type = WI_RID_CURRENT_BSSID;
	wi_getval(iface, &wreq);
	bcopy((unsigned char *) &wreq.wi_val, ibss, 6);

	bzero((char *)&wreq, sizeof(wreq));
	wreq.wi_len = WI_MAX_DATALEN;
	wreq.wi_type = WI_RID_READ_CACHE;
	wi_getval(iface, &wreq);

	wi_sigitems = (int *) &wreq.wi_val; 
	pt = ((char *) &wreq.wi_val);
	pt += sizeof(int);
	sc = (struct wi_sigcache *) pt;

	for (i = 0; i < *wi_sigitems; i++) {
		if (sc->signal == 0) /* empty slot*/
			continue;
		if (bcmp(&sc->macsrc, ibss, 6) == 0)
			printf("* ");
		else
			printf("  ");
		printf("%02x:%02x:%02x:%02x:%02x:%02x "
		    "sig=%ddBm noise=%ddBm s/n=%ddB\n",
		    sc->macsrc[0] & 0xff, sc->macsrc[1] & 0xff,
		    sc->macsrc[2] & 0xff, sc->macsrc[3] & 0xff,
		    sc->macsrc[4] & 0xff, sc->macsrc[5] & 0xff,
		    sc->signal, sc->noise, sc->signal - sc->noise);
		sc++;
	}

	return;
}

int main(argc, argv)
	int			argc;
	char			*argv[];
{
	int			ch;
	char			*iface = "wi0";
	char			*p = argv[0];

	while((ch = getopt(argc, argv,
	    "hoc:d:i:r:s:q:P:S:ZC")) != -1) {
		switch(ch) {
		case 'o':
			wi_dumpstats(iface);
			exit(0);
			break;
		case 'i':
			iface = optarg;
			break;
		case 'c':
			wi_setword(iface, WI_RID_CREATE_IBSS, atoi(optarg));
			exit(0);
			break;
		case 'd':
			wi_setword(iface, WI_RID_MAX_DATALEN, atoi(optarg));
			exit(0);
			break;
		case 'r':
			wi_setword(iface, WI_RID_RTS_THRESH, atoi(optarg));
			exit(0);
			break;
		case 's':
			wi_setword(iface, WI_RID_SYSTEM_SCALE, atoi(optarg));
			exit(0);
			break;
		case 'q':
			wi_setstr(iface, WI_RID_OWN_SSID, optarg);
			exit(0);
			break;
		case 'S':
			wi_setword(iface, WI_RID_MAX_SLEEP, atoi(optarg));
			exit(0);
			break;
		case 'P':
			wi_setword(iface, WI_RID_PM_ENABLED, atoi(optarg));
			exit(0);
			break;
		case 'Z':
			wi_zerocache(iface);
			exit(0);
			break;
		case 'C':
			wi_readcache(iface);
			exit(0);
			break;
		case 'h':
		default:
			usage(p);
			break;
		}
	}

	if (iface == NULL)
		usage(p);

	wi_dumpinfo(iface);

	exit(0);
}
