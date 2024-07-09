/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI des.c,v 1.5 2003/08/21 22:29:27 polk Exp
 */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define NCOMPAT
#include <openssl/des.h>

#include "include.h"

typedef struct {
	des_key_schedule	key;
	u_char	iblock[8];
	u_char	oblock[8];
	u_long	icrc;		/* input cksum */
	u_long	ocrc;		/* output cksum */
	u_long	rcrc;		/* crc read in from network */
	int	ic;
	int	oc;
	int	is;
} des_t;

static des_t * des_init(u_char *);
static void traceblock(char *, u_char *);

static u_long crctab[] = {
	0x0,
	0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
	0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
	0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
	0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
	0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
	0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
	0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
	0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
	0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
	0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
	0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
	0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
	0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
	0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
	0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
	0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
	0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
	0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
	0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
	0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
	0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
	0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
	0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
	0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
	0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
	0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
	0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
	0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
	0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
	0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
	0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
	0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
	0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
	0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
	0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
	0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
	0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
	0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
	0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
	0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
	0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
	0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

#define	CRC(crc, ch)	(crc) = (crc) << 8 ^ crctab[(crc) >> 24 ^ (ch)]

int
my_des_encrypt(u_char *buf, int blen, void *vp)
{
	des_t *d = (des_t *)vp;
	int r, s, t;
	u_char *src, x;

	r = 0;

	/*
	 * We know the buffer is at least 4x our size
	 * move the data into the top quarter so we can
	 * stuff random garbage into it.
	 */
	src = buf + blen * 3;
	memcpy(src, buf, blen);

#define	STUFF(x) { \
		if (d->oc == 8) { \
			des_ecb_encrypt(d->oblock, d->oblock, d->key, 1); \
			d->oc = 0; \
		} \
		++r; \
		*buf++ = (d->oblock[d->oc++] ^ (x)); \
	}

	s = random_auth() % (blen * 2);	/* number of bytes to stuff */

	while (blen-- > 0) {
		if (*src == '\n') {
			STUFF('\0')
			STUFF('\200')
			if (trace)
				fprintf(trace, "SEND CRC %08lx\n", d->ocrc);
			STUFF((d->ocrc >> 24) & 0xff)
			STUFF((d->ocrc >> 16) & 0xff)
			STUFF((d->ocrc >>  8) & 0xff)
			STUFF((d->ocrc      ) & 0xff)
			d->ocrc = ~0;
			src++;
			continue;
		}
		if (s > 2) {
			t = (random_auth() % s) + 1;
			if (t > 127)
				t = 127;
			if (t + 2 > s)
				t = s - 2;
			CRC(d->ocrc, '\0');
			STUFF('\0')
			CRC(d->ocrc, t);
			STUFF(t)
			s -= t + 2;
			while (t-- > 0) {
				x = random_auth();
				CRC(d->ocrc, x);
				STUFF(x)
			}
		}
		if (*src == '\0') {	/* dont confuse the nulls */
			CRC(d->ocrc, '\0');
			STUFF('\0')
		}
		CRC(d->ocrc, *src);
		STUFF(*src++)
	}

	return (r);
}

int
my_des_decrypt(u_char *buf, int blen, void *vp)
{
	u_char *src, *dst, x;
	des_t *d = (des_t *)vp;


	src = dst = buf;

	while (blen-- > 0) {
		if (d->ic == 8) {
			des_ecb_encrypt(d->iblock, d->iblock, d->key, 1);
			d->ic = 0;
		}
		x = *src++ ^ d->iblock[d->ic++];
		/* d->iblock[d->ic++] = *src++; only for OFB mode */
		if (d->is == -1) {
			if (x < 0200) {
				CRC(d->icrc, 0);
				CRC(d->icrc, x);
			}
			if ((d->is = x) == 0) {
				*dst++ = '\0';
			}
		} else if (d->is == 0200) {
			d->rcrc = x << 24;
			++d->is;
		} else if (d->is == 0201) {
			d->rcrc |= x << 16;
			++d->is;
		} else if (d->is == 0202) {
			d->rcrc |= x << 8;
			++d->is;
		} else if (d->is == 0203) {
			d->rcrc |= x;
			d->is = 0;
			if (trace)
				fprintf(trace, "READ CRC %08lx\n", d->rcrc);
			if (d->rcrc != d->icrc)
				return (-1);
			*dst++ = '\n';
			d->icrc = ~0;
		} else if (d->is > 0) {
			CRC(d->icrc, x);
			d->is--;
		} else if (x == '\0') {
			d->is = -1;
		} else {
			CRC(d->icrc, x);
			*dst++ = x;
		}
	}
	return (dst - buf);
}

/*
 * client side.
 */
int
des_initiate(int s, void **vpp)
{
	des_t *d;
	u_char a[8];
	u_char *data = (u_char *)*vpp;

	if ((d = des_init(data)) == NULL)
		return (-1);
	*vpp = d;

	/*
	 * Send our session key encrypted with the shared secret
	 * Expect to get it back offset by 1 encrypted with the shared secret
	 */
	des_ecb_encrypt(d->iblock, a, d->key, 1);
	write(s, a, sizeof(a));
	traceblock("SEND", a);
	if (read(s, a, 8) != 8) {
		free(d);
		return (-1);
	}
	traceblock("READ", a);
	des_ecb_encrypt(a, a, d->key, 0);
	a[7]--;
	if (memcmp(a, d->iblock, sizeof(d->iblock)) != 0) {
		free(d);
		return (-1);
	}

	/*
	 * Now compute the key schedule for the session key
	 * Create a random noise vector which we will both encrypt
	 * then read the other sides noise vector
	 */
	des_key_sched(d->iblock, d->key);
        des_random_key(d->iblock);
	write(s, d->iblock, sizeof(d->iblock));
	traceblock("SEND", d->iblock);

	if (read(s, d->oblock, 8) != 8) {
		free(d);
		return (-1);
	}
	traceblock("READ", d->oblock);
	return (0);
}

/*
 * server side.
 */
int
des_verify(int s, void **vpp)
{
	u_char *data = (u_char *)*vpp;
	u_char a[8];
	des_t *d;

	if ((d = des_init(data)) == NULL) {
		return (-1);
	}
	*vpp = d;

	/*
	 * Read our session key encrypted with the shared secret
	 * Decrypt it offset it 1,  encrypt it, send it back
	 */
	if (read(s, d->oblock, 8) != 8) {
		free(d);
		return (-1);
	}
	traceblock("READ", d->oblock);
	des_ecb_encrypt(d->oblock, a, d->key, 0);
	a[7]++;
	des_ecb_encrypt(a, d->oblock, d->key, 1);
	a[7]--;
	write(s, d->oblock, sizeof(d->oblock));
	traceblock("SEND", d->oblock);

	/*
	 * Now compute the key schedule for the session key
	 * Create a random noise vector which we will both encrypt
	 * but first read the other sides noise vector
	 */
	des_key_sched(a, d->key);

	if (read(s, d->oblock, 8) != 8) {
		free(d);
		return (-1);
	}
	traceblock("READ", d->oblock);

	write(s, d->iblock, sizeof(d->iblock));
	traceblock("SEND", d->iblock);
	return (0);
}

static des_t *
des_init(u_char *data)
{
	des_t *d;
	u_char key[8];
	int i;

	while (isspace(*data))
		++data;

	for (i = 0; i < 8; ++i) {
		key[i] = (digittoint(data[0]) << 4) | digittoint(data[1]);
		if (((key[i] >> 4) & 0xf) == 0 && data[0] != '0' &&
		    (key[i] & 0xf) == 0 && data[1] != '0')
			return (NULL);
		data += 2;
	}

	if ((d = malloc(sizeof(des_t))) == NULL)
		return (NULL);

	des_fixup_key_parity(key);
	des_key_sched(key, d->key);

	d->is = 0;
	d->ic = 8;
	d->oc = 8;
	d->ocrc = ~0;
	d->icrc = ~0;
	d->rcrc = 0;

	des_random_key(d->iblock);
	des_random_seed(d->iblock);
        des_random_key(d->iblock);
	des_fixup_key_parity(d->iblock);
	return (d);
}

static void
traceblock(char *msg, u_char *data)
{
	int i;
	if (trace) {
		fprintf(trace, "%s: ", msg);
		for (i = 0; i < 8; ++i)
			fprintf(trace, "%02x", data[i]);
		fprintf(trace, "\n");
		fflush(trace);
	}
}
