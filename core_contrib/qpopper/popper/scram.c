
/***************************************************************************
* Derived from Appendix C of:                                              *
* Network Working Group                                          C. Newman *
* Internet Draft: SCRAM-MD5 SASL Mechanism                        Innosoft *
* Document: draft-newman-auth-scram-02.txt                    January 1998 *
*                                                    Expires in six months *
***************************************************************************/


/* scram.c -- routines for SCRAM-MD5 calculations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "md5.h"
#include "hmac-md5.h"
#include "scram.h"

void scram_md5_vgen(SCRAM_MD5_VRFY *vptr,
                    const unsigned char *salt,
                    const char *pass, int passlen, int plainflag,
                    unsigned char *clientkey)
{
    HMAC_MD5_CTX hctx;

    if (clientkey == NULL) clientkey = vptr->clidata;

    /* get context */

    if (plainflag) {
        if (passlen == 0) passlen = strlen(pass);
        hmac_md5_init(&hctx, (const unsigned char *) pass,
                      passlen);
    } else {
        hmac_md5_import(&hctx, (HMAC_MD5_STATE *) pass);
    }

    /* generate salted passphrase */

    hmac_md5_update(&hctx, salt, SCRAM_MD5_SALTSIZE);
    hmac_md5_final(vptr->clidata, &hctx);

    /* generate server proof */

    hmac_md5(salt, SCRAM_MD5_SALTSIZE, vptr->clidata,
             sizeof (vptr->clidata), vptr->svrdata);

    /* generate client key and client verifier */

    MD5Init(&hctx.ictx);
    MD5Update(&hctx.ictx, vptr->clidata, sizeof (vptr->clidata));
    MD5Final(clientkey, &hctx.ictx);
    MD5Init(&hctx.ictx);
    MD5Update(&hctx.ictx, clientkey, SCRAM_MD5_DATASIZE);
    MD5Final(vptr->clidata, &hctx.ictx);

    /* copy salt to verifier */

    if (salt != vptr->salt) {
        memcpy(vptr->salt, salt, SCRAM_MD5_SALTSIZE);
    }
}

int scram_md5_generate(const char *cchal, int cchallen,
                       const char *schal, int schallen,
                       const char *secret, int secretlen,
                       int action,
                       SCRAM_MD5_CLIENT *clidata,
                       unsigned char *sproof,
                       unsigned char *integrity_key)
{
    SCRAM_MD5_VRFY verifier, *vptr;
    HMAC_MD5_CTX hctx;
    unsigned char clientkey[HMAC_MD5_SIZE];
    unsigned char sharedkey[HMAC_MD5_SIZE];
    int i, result = 0;

    /* check params */

    if ((action == SCRAM_CREDENTIAL
      && secretlen != sizeof (HMAC_MD5_STATE))
        || (action == SCRAM_VERIFY
            && secretlen != sizeof (verifier))
        || schallen < SCRAM_MD5_SALTSIZE) {
        return (-2);
    }

    /* get verifier */
    if (action == SCRAM_VERIFY) {
        vptr = (SCRAM_MD5_VRFY *) secret;
    } else {
        scram_md5_vgen(&verifier, (const unsigned char *) schal,
                       secret, secretlen, action, clientkey);
        vptr = &verifier;
    }

    /* calculate shared key */
    hmac_md5_init(&hctx, vptr->clidata, sizeof (vptr->clidata));
    hmac_md5_update(&hctx, (unsigned char *) schal, schallen);
    hmac_md5_update(&hctx, (unsigned char *) cchal, cchallen);
    hmac_md5_update(&hctx, clidata->secprops, 4);
    hmac_md5_final(sharedkey, &hctx);

    if (action == SCRAM_VERIFY) {
        /* verify client proof */
        for (i = 0; i < HMAC_MD5_SIZE; ++i) {
         /* XXX: the line which belongs here is omitted due to
            U.S. export regulations, but it exclusive-ors the
            "sharedkey" with the "clidata->cproof" and places the
            result in "clientkey" (see step (c) above) */
            clientkey[i] = sharedkey[i] ^ (clidata->cproof)[i];
        }
        MD5Init(&hctx.ictx);
        MD5Update(&hctx.ictx, clientkey, sizeof (clientkey));
        MD5Final(sharedkey, &hctx.ictx);
        if (memcmp(sharedkey, vptr->clidata,
                   sizeof (sharedkey)) != 0) {
            result = -1;
        }
    } else {
        /* generate client proof */
        for (i = 0; i < HMAC_MD5_SIZE; ++i) {
         /* XXX: the line which belongs here is omitted due to
            U.S. export regulations, but it exclusive-ors the
            "sharedkey" with the "clientkey" and places the
            result in "clidata->cproof" (see step (G) above) */
            (clidata->cproof)[i] = sharedkey[i] ^ clientkey[i];
        }
    }


    /* calculate integrity key */

    if (integrity_key != NULL) {
        hmac_md5_init(&hctx, clientkey, HMAC_MD5_SIZE);
        hmac_md5_update(&hctx, (unsigned char *) schal, schallen);
        hmac_md5_update(&hctx, (unsigned char *) cchal, cchallen);
        hmac_md5_update(&hctx, clidata->secprops, 4);
        hmac_md5_final(integrity_key, &hctx);
    }

    /* calculate server result */

    if (result == 0) {
        hmac_md5_init(&hctx, vptr->svrdata, HMAC_MD5_SIZE);
        hmac_md5_update(&hctx, (unsigned char *) cchal, cchallen);
        hmac_md5_update(&hctx, (unsigned char *) schal, schallen);
        hmac_md5_update(&hctx, clidata->secprops, 4);
        hmac_md5_final(sproof, &hctx);
    }

    /* cleanup workspace */

    memset(clientkey, 0, sizeof (clientkey));
    memset(sharedkey, 0, sizeof (sharedkey));
    if (vptr == &verifier) memset(&verifier, 0, sizeof (verifier));

    return (result);
}


int scram_md5_integrity( const char          *packet, 
                         const int packetlen,
                         SCRAM_MD5_INTEGRITY *icontrols,
                         const int            action)
{
    UINT4         count;
    int           plen;

    if ( ((action!=SCRAM_VERIFY) && (action!=SCRAM_MAC))
                         ||
         ((action == SCRAM_VERIFY ) 
              && (packetlen < SCRAM_MD5_DATASIZE)) )
            return( -2 );

    /* bump the appropriate counter by one */

    count = htonl( (action==SCRAM_VERIFY) ?
                    ++(icontrols->inpktcnt) : ++(icontrols->outpktcnt) );
    memcpy( icontrols->prefix, &count, SCRAM_MD5_COUNTSIZE);

    /* Incoming packet has proof on end, outgoing does not */

    plen  = (action==SCRAM_VERIFY) ?
                packetlen-HMAC_MD5_SIZE : packetlen;

    /* HMAC data portion of the packet: result into iproof */

    hmac_md5( (unsigned char *)packet,        
              plen,
              icontrols->prefix, 
              SCRAM_MD5_COUNTSIZE + HMAC_MD5_SIZE,
              icontrols->iproof );

    /* Do verification checking iff requested */

    if ( (action == SCRAM_VERIFY) &&
             memcmp( icontrols->iproof,
                       &packet[packetlen-HMAC_MD5_SIZE],
                          HMAC_MD5_SIZE ) )
               return( -1 );
        
    return( 0 );
}
