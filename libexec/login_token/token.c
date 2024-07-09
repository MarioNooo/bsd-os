/*	BSDI token.c,v 1.4 2003/08/21 22:29:27 polk Exp */
/*-
//////////////////////////////////////////////////////////////////////////
//									//
//   Copyright (c) 1995 Migration Associates Corp. All Rights Reserved	//
//									//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MIGRATION ASSOCIATES	//
//	The copyright notice above does not evidence any   		//
//	actual or intended publication of such source code.		//
//									//
//	Use, duplication, or disclosure by the Government is		//
//	subject to restrictions as set forth in FAR 52.227-19,		//
//	and (for NASA) as supplemented in NASA FAR 18.52.227-19,	//
//	in subparagraph (c) (1) (ii) of Rights in Technical Data	//
//	and Computer Software clause at DFARS 252.227-7013, any		//
//	successor regulations or comparable regulations of other	//
//	Government agencies as appropriate.				//
//									//
//		Migration Associates Corporation			//
//			19935 Hamal Drive				//
//			Monument, CO 80132				//
//									//
//	A license is granted to Berkeley Software Design, Inc. by	//
//	Migration Associates Corporation to redistribute this software	//
//	under the terms and conditions of the software License		//
//	Agreement provided with this distribution.  The Berkeley	//
//	Software Design Inc. software License Agreement specifies the	//
//	terms and conditions for redistribution.			//
//									//
//	UNDER U.S. LAW, THIS SOFTWARE MAY REQUIRE A LICENSE FROM	//
//	THE U.S. COMMERCE DEPARTMENT TO BE EXPORTED.  IT IS YOUR	//
//	REQUIREMENT TO OBTAIN THIS LICENSE PRIOR TO EXPORTATION.	//
//									//
//////////////////////////////////////////////////////////////////////////
*/

/*
 * DES functions for one-way encrypting Authentication Tokens.
 * All knowledge of DES is confined to this file.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <openssl/des.h>
#include "token.h"
#include "tokendb.h"

/*
 * Define a union of various types of arguments to DES functions.
 * All native DES types are modulo 8 bytes in length. Cipher text
 * needs a trailing null byte.
 */

typedef	union {
	des_cblock	cb;
	char		ct[9];
	unsigned long	ul[2];
} TOKEN_CBlock;

/*
 * Static definition of random number challenge for token.
 * Challenge length is 8 bytes, left-justified with trailing null byte.
 */

static	TOKEN_CBlock tokennumber;

/*
 * Static function prototypes
 */

static	long	tokenrandomnumber(void);
static	void	tokenseed(des_cblock);
static	void	lcase(char *);
static	void	h2d(char *);
static	void	h2cb(char *, TOKEN_CBlock *);
static	void	cb2h(TOKEN_CBlock, char *);

/*
 * Generate random DES cipherblock seed. Feedback key into
 * new_random_key from KerberrosIV to strengthen.
 */

extern void
tokenseed(des_cblock cb)
{
	static int first_time = 1;

	if (first_time) {
		first_time = 0;
		des_random_key(cb);
	}
	des_random_key(cb);
}

/*
 * Generate a random key of eight decimal digits. Actually,
 * with the CRYPTOCard, this could be up to 64 digits.
 * This string must be zero filled
 * and padded to a 64-bit boundary with a trailing null byte.
 * It could also be hex, but decimal is easier for the user
 * to enter into the token.
 */

static long
tokenrandomnumber(void)
{
	TOKEN_CBlock seed;

	tokenseed(seed.cb);
	return (((seed.ul[0] ^ seed.ul[1]) % 99999999));
}

/*
 * Send a random challenge string to the token. The challenge
 * is always base 10 as there are no alpha keys on the keyboard.
 */

extern void
tokenchallenge(char *user, char *challenge, int size, char *card_type)
{
	TOKENDB_Rec tr;
	TOKEN_CBlock cb;
	des_key_schedule ks;
	int r, c;

	r = 1;	/* no reduced input mode by default! */

	if ((tt->modes & TOKEN_RIM) &&
	    tokendb_getrec(user, &tr) == 0 &&
	    (tr.mode & TOKEN_RIM)) {
		c = 0;
        	while ((r = tokendb_lockrec(user, &tr, TOKEN_LOCKED)) == 1) {
			if (c++ >= 60)
				break;
                	sleep(1);
		}
		tr.flags &= ~TOKEN_LOCKED;
		if (r == 0 && tr.rim[0]) {
			h2cb(tr.secret, &cb);
			des_fixup_key_parity(&cb.cb);
        		des_key_sched(&cb.cb, ks);
			des_ecb_encrypt(&tr.rim, &cb.cb, ks, DES_ENCRYPT);
			memcpy(tr.rim, cb.cb, 8);
			for (r = 0; r < 8; ++r) {
				if ((tr.rim[r] &= 0xf) > 9)
					tr.rim[r] -= 10;
				tr.rim[r] |= 0x30;
			}
			r = 0;		/* reset it back */
			memcpy(tokennumber.ct, tr.rim, 8);
			tokennumber.ct[8] = 0;
			tokendb_putrec(user, &tr);
		}
	}
	if (r != 0 || tr.rim[0] == '\0') {
		memset(tokennumber.ct, 0, sizeof(tokennumber));
		snprintf(tokennumber.ct, sizeof(tokennumber.ct), "%08.8lu",
				tokenrandomnumber());
		if (r == 0) {
			memcpy(tr.rim, tokennumber.ct, 8);
			tokendb_putrec(user, &tr);
		}
	}

	snprintf(challenge, size, "%s Challenge \"%s\"\r\n%s Response: ",
			card_type, tokennumber.ct, card_type);
}

/*
 * Verify response from user against token's predicted cipher
 * of the random number challenge.
 */

extern int
tokenverify(char *username, char *challenge, char *response)
{
	char	*state;
	TOKENDB_Rec tokenrec;
	TOKEN_CBlock tmp;
	TOKEN_CBlock cmp_text;
	TOKEN_CBlock user_seed;
	TOKEN_CBlock cipher_text;
	des_key_schedule key_schedule;


	memset(cmp_text.ct, 0, sizeof(cmp_text));
	memset(user_seed.ct, 0, sizeof(user_seed));
	memset(cipher_text.ct, 0, sizeof(cipher_text));
	memset(tokennumber.ct, 0, sizeof(tokennumber));

	state = strtok(challenge, "\"");
	state = strtok(NULL, "\"");
	sscanf(state, "%lu", &tmp.ul[0]);
	snprintf(tokennumber.ct, sizeof(tokennumber.ct), "%08.8lu",tmp.ul[0]);

	/*
	 * Retrieve the db record for the user. Nuke it as soon as
	 * we have translated out the user's shared secret just in
	 * case we (somehow) get core dumped...
	 */

	if (tokendb_getrec(username, &tokenrec))
		return (-1);

	h2cb(tokenrec.secret, &user_seed);
	memset((char*)&tokenrec.secret, 0, sizeof(tokenrec.secret));

	if (!(tokenrec.flags & TOKEN_ENABLED))
		return (-1);

	/*
	 * Compute the anticipated response in hex. Nuke the user's
	 * shared secret asap.
	 */

	des_fixup_key_parity(&user_seed.cb);
	des_key_sched(&user_seed.cb, key_schedule);
	memset(user_seed.ct, 0, sizeof(user_seed.ct));
	des_ecb_encrypt(&tokennumber.cb, &cipher_text.cb, key_schedule,
	    DES_ENCRYPT);

	/*
	 * The token thinks it's descended from VAXen.  Deal with i386
	 * endian-ness of binary cipher prior to generating ascii from first
	 * 32 bits.
	 */

	HTONL(cipher_text.ul[0]);
	snprintf(cmp_text.ct, sizeof(cmp_text.ct), "%08.8lx", cipher_text.ul[0]);

	if (tokenrec.mode & TOKEN_PHONEMODE) {
		/*
		 * If we are a CRYPTOCard, we need to see if we are in
		 * "telephone number mode".  If so, transmogrify the fourth
		 * digit of the cipher.  Lower case response just in case
		 * it's * hex.  Compare hex cipher with anticipated response
		 * from token.
		 */

		lcase(response);

		if (response[3] == '-')
			cmp_text.ct[3] = '-';
	}

	if ((tokenrec.mode & TOKEN_HEXMODE) && !strcmp(response, cmp_text.ct))
		return (0);

	/*
	 * No match against the computed hex cipher.  The token could be
	 * in decimal mode.  Pervert the string to magic decimal equivalent.
	 */

	h2d(cmp_text.ct);

	if ((tokenrec.mode & TOKEN_DECMODE) && !strcmp(response, cmp_text.ct))
		return (0);

	return (-1);
}

/*
 * Initialize a new user record in the token database.
 */

extern	int
tokenuserinit(int flags, char *username, unsigned char *usecret, unsigned mode)
{
	TOKENDB_Rec tokenrec;
	TOKEN_CBlock secret;
	TOKEN_CBlock nulls;
	TOKEN_CBlock checksum;
	TOKEN_CBlock checktxt;
	des_key_schedule key_schedule;

	/*
	 * If no user secret passed in, create one
	 */

	if ( (flags & TOKEN_GENSECRET) )
		tokenseed(secret.cb);
	else {
		memset(&secret, 0, sizeof(secret));
		memcpy(&secret, usecret, sizeof(des_cblock));
		des_fixup_key_parity(&secret.cb);
	}

	/*
	 * Check if the db record already exists.  If no
	 * force-init flag and it exists, go away. Else,
	 * create the user's db record and put to the db.
	 */


	if (!(flags & TOKEN_FORCEINIT) &&
	    tokendb_getrec(username, &tokenrec) == 0)
		return (1);

	memset(&tokenrec, 0, sizeof(tokenrec));
	strncpy(tokenrec.uname, username, sizeof(tokenrec.uname));
	cb2h(secret, tokenrec.secret);
	tokenrec.mode = 0;
	tokenrec.flags = TOKEN_ENABLED | TOKEN_USEMODES;
	tokenrec.mode = mode;
	memset(tokenrec.reserved_char1, '0', sizeof(tokenrec.reserved_char1));
	memset(tokenrec.reserved_char2, '0', sizeof(tokenrec.reserved_char2));

	if (tokendb_putrec(username, &tokenrec))
		return (-1);

	/*
	 * Check if the shared secret was generated here. If so, we
	 * need to inform the user about it in order that it can be
	 * programmed into the token. See tokenverify() (above) for
	 * discussion of cipher generation.
	 */

	if (!(flags & TOKEN_GENSECRET))
		return (0);

	printf("Shared secret for %s\'s token: "
	    "%03o %03o %03o %03o %03o %03o %03o %03o\n",
	    username, secret.cb[0], secret.cb[1], secret.cb[2], secret.cb[3],
	    secret.cb[4], secret.cb[5], secret.cb[6], secret.cb[7]); 


	des_key_sched(&secret.cb, key_schedule);
	memset(&nulls, 0, sizeof(nulls));
	des_ecb_encrypt(&nulls.cb, &checksum.cb, key_schedule, DES_ENCRYPT);
	HTONL(checksum.ul[0]);
	snprintf(checktxt.ct, sizeof(checktxt.ct), "%08.8lx", checksum.ul[0]);
	printf("Hex Checksum: \"%s\"", checktxt.ct);

	h2d(checktxt.ct);
	printf("\tDecimal Checksum: \"%s\"\n", checktxt.ct);

	return (0);
}

/*
 * Magically transform a hex character string into a decimal character
 * string as defined by the token card vendor. The string should have
 * been lowercased by now.
 */

static	void
h2d(char *cp)
{
	int	i;

	for (i=0; i<sizeof(des_cblock); i++, cp++) {
		if (*cp >= 'a' && *cp <= 'f')
			*cp = tt->map[*cp - 'a'];
	}
}

/*
 * Translate an hex 16 byte ascii representation of an unsigned
 * integer to a des_cblock.
 */

static	void
h2cb(char *hp, TOKEN_CBlock *cb)
{
	sscanf(hp,   "%08lx", &cb->ul[0]);
	sscanf(hp+8, "%08lx", &cb->ul[1]);
}

/*
 * Translate a des_cblock to an 16 byte ascii hex representation.
 */

static	void
cb2h(TOKEN_CBlock cb, char* hp)
{
	char	scratch[17];

	snprintf(scratch,   9, "%08.8lx", cb.ul[0]);
	snprintf(scratch+8, 9, "%08.8lx", cb.ul[1]);
	memcpy(hp, scratch, 16);
}

/*
 * Lowercase possible hex response
 */

static	void
lcase(char *cp)
{
	while (*cp) {
		if (isupper(*cp))
			*cp = tolower(*cp);
		cp++;
	}
}
