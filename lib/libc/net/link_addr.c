/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)linkaddr.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <machine/param.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <ctype.h>
#include <malloc.h>
#include <string.h>

/* Input events */
enum input { 
	EOS=0x100,	/* End of string */ 
	COLON=0x200, 	/* A colon delimiter */
	DELIM=0x300, 	/* A delimiter other than a colon */
	DIGIT=0x400, 	/* A decimal digit */
	XDIGIT=0x500,	/* A hex digit (a-f or A-F) */
	ALPHA=0x600, 	/* A letter (other than a-f and A-F) */
	UNKNOWN=0x700	/* Anything else */
};
/* States */
enum state { 
	START=0x0, 	/* Start state, decide on NAME vs. ADDRESS */
	NAME=0x1, 	/* Alpha part of interface identifier */
	UNIT=0x2, 	/* Numeric part of interface identifier */
	ADDRESS=0x3, 	/* Address */
	FINISH=0x4, 	/* Successful finish */
	ERROR=0x5	/* Unsuccessful finish, must be greater than FINISH */
};

void
link_addr(ap0, sdl)
	const char *ap0;
	struct sockaddr_dl *sdl;
{
	enum state state;
	u_int acc;
	int digits;
	int len;
	int sep;
	const char *ap;
	u_char *cp;
	const u_char *cplim;

	acc = digits = len = sep = 0;
	state = START;
	ap = ap0;
	cp = sdl->sdl_data;
	cplim = sdl->sdl_len + (u_char *)sdl;

	/* Make sure there is some room provided */
	if (cplim <= cp)
		state = ERROR;

	while (state < FINISH) {
		enum input input;
		char ch;
		char base;

		ch = *ap++;

		/* Classify the input */
		if (ch == 0)
			input = EOS;
		else if (ch == ':')
			input = COLON;
		else if (ch == '.' || ch == '-')
			input = DELIM;
		else if (isdigit(ch))
			input = DIGIT;
		else if (isxdigit(ch))
			input = XDIGIT;
		else if (isalpha(ch))
			input = ALPHA;
		else
			input = UNKNOWN;

		/* Apply the event to the state machine */
		switch (state | input) {
			/* 
			 * In this state we are trying to guess if we
			 * are dealing with an interface identifier or
			 * an address.
			 */
		case START | COLON:
			state = ADDRESS;
			sep = ':';
			break;
		case START | DIGIT:
		TryAddress:
			state = ADDRESS;
			ap = ap0;
			break;
		case START | XDIGIT:
		case START | ALPHA:
			state = NAME;
			ap = ap0;
			break;
			
			/*
			 * In this state we are parsing the alpha part
			 * of an interface identifier.  If we get a
			 * premature colon we attempt to reset and
			 * parse as an address.
			 */
		case NAME | COLON:
			/* Could be an address */
			goto TryAddress;
		case NAME | DIGIT:
			/* Have name, get unit */
			state = UNIT;
			break;
		case NAME | XDIGIT:
		case NAME | ALPHA:
			break;

			/*
			 * In this state we are parsing the numeric
			 * part of an interface identifier.  In some
			 * cases a failure will result in a reset and
			 * a parse as an address.
			 */
		case UNIT | EOS:
			state = FINISH;
			goto UnitEnd;
		case UNIT | COLON:
			state = ADDRESS;
		UnitEnd:
			len = ap - ap0 - 1;
			if (len > IFNAMSIZ || len > cplim - cp) {
				state = ERROR;
				break;
			}
			memcpy(cp , ap0, len);
			cp += len;
			break;
		case UNIT | DELIM:
		case UNIT | XDIGIT:
			goto TryAddress;
			break;
		case UNIT | DIGIT:
			break;

			/*
			 * In this state we are parsing an address.
			 * One or two hex digits seperated by colons,
			 * periods or dashes.  Once a specific
			 * delimiter is used it must be the only one
			 * used.
			 */
		case ADDRESS | EOS:
			state = FINISH;
			if (digits == 0)
				break;
			goto StoreAcc;
		case ADDRESS | COLON:
		case ADDRESS | DELIM:
			if (sep == 0)
				sep = ch;
			if (sep != ch)
				state = ERROR;
		StoreAcc:
			if (cp == cplim)
				state = ERROR;
			else
				*cp++ = acc;
			acc = digits = 0;
			break;
		case ADDRESS | DIGIT:
			base = '0';
			goto PickupDigit;
		case ADDRESS | XDIGIT:
			base = 'a' - 10;
		PickupDigit:
			if (++digits > 2) {
				state = ERROR;
				break;
			}
			acc = (tolower(ch) - base) | (acc << 4);
			break;
		case ADDRESS | ALPHA:
			state = ERROR;
			break;

		default:
			state = ERROR;
			break;
		}
	}
	if (state == ERROR) {
		if (cp > (u_char *)sdl)
			memset(sdl, 0, cp - (u_char *)sdl);
		return;
	}

	/* Clear the static structure and any of sdl_data we did not use */
	memset(sdl, 0, sizeof(*sdl) - sizeof(sdl->sdl_data));
	if (cp < cplim)
		memset(cp, 0, cplim - cp);
	/* Set the lengths and other fields */
	sdl->sdl_nlen = len;
	sdl->sdl_alen = cp - (u_char *)sdl->sdl_data - len;
	sdl->sdl_len = cp - (u_char *)sdl;
	sdl->sdl_family = AF_LINK;

	return;
}

static char hexlist[] = "0123456789abcdef";
static char *obuf;
static size_t obuflen;

char *
link_ntoa(sdl)
	const struct sockaddr_dl *sdl;
{
	size_t len;
	const u_char *in;
	const u_char *inlim;
	char *op;

	/* Make sure our buffer is large enough */
	len = sdl->sdl_nlen + (sdl->sdl_alen * 3) + 1;
	if (obuf == NULL || obuflen < len) {
		obuflen = ALIGN(len);
		obuf = realloc(obuf, obuflen);
		if (obuf == NULL)
			return ("?");
	}
	op = obuf;

	/* Print interface name.  If no name and no address, print a colon */
	if (sdl->sdl_nlen > 0) {
		for (in = sdl->sdl_data, inlim = LLADDR(sdl); in < inlim;
		     in++) {
			char ch;

			ch = *in;
			*op++ = isalnum(ch) ? ch : '?';
		}
		if (sdl->sdl_alen)
			*op++ = ':';
	}

	/* Print interface address */
	in = (u_char *)LLADDR(sdl);
	inlim = in + sdl->sdl_alen;
	while (in < inlim) {
		u_char ch;

		if (in > (u_char *)LLADDR(sdl))
			*op++ = '.';
		ch = *in++;
		if (ch > 0xf) {
			op[1] = hexlist[ch & 0xf];
			ch >>= 4;
			*op = hexlist[ch];
			op += 2;
		} else
			*op++ = hexlist[ch];
	}
	*op = 0;

	return (obuf);
}
