/*
 * src/lib/krb5/asn.1/asn1_get.h
 * 
 * Copyright 1994 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#ifndef __ASN1_GET_H__
#define __ASN1_GET_H__

/* ASN.1 substructure decoding procedures */

#include "k5-int.h"
#include "krbasn1.h"
#include "asn1buf.h"

asn1_error_code asn1_get_tag_indef
	PROTOTYPE((asn1buf *buf,
		   asn1_class *class,
		   asn1_construction *construction,
		   asn1_tagnum *tagnum,
		   int *retlen, int *indef));
asn1_error_code asn1_get_tag
	PROTOTYPE((asn1buf *buf,
		   asn1_class *class,
		   asn1_construction *construction,
		   asn1_tagnum *tagnum,
		   int *retlen));
/* requires  *buf is allocated
   effects   Decodes the tag in *buf.  If class != NULL, returns
              the class in *class.  Similarly, the construction,
	      tag number, and length are returned in *construction,
	      *tagnum, and *retlen, respectively.
	     If *buf is empty to begin with,
	      *tagnum is set to ASN1_TAGNUM_CEILING.
	     Returns ASN1_OVERRUN if *buf is exhausted during the parse. */

asn1_error_code asn1_get_sequence
	PROTOTYPE((asn1buf *buf, int *retlen, int *indef));
/* requires  *buf is allocated
   effects   Decodes a tag from *buf and returns ASN1_BAD_ID if it
              doesn't have a sequence ID.  If retlen != NULL, the
	      associated length is returned in *retlen. */

/****************************************************************/
/* Private Procedures */

asn1_error_code asn1_get_id
	PROTOTYPE((asn1buf *buf,
		   asn1_class *class,
		   asn1_construction *construction,
		   asn1_tagnum *tagnum));
/* requires  *buf is allocated
   effects   Decodes the group of identifier octets at *buf's
              current position.  If class != NULL, returns the class
              in *class.  Similarly, the construction and tag number
              are returned in *construction and *tagnum, respectively.
	     Returns ASN1_OVERRUN if *buf is exhausted. */

asn1_error_code asn1_get_length
	PROTOTYPE((asn1buf *buf, int *retlen, int *indef));
/* requires  *buf is allocated
   effects   Decodes the group of length octets at *buf's
              current position.  If retlen != NULL, the
	      length is returned in *retlen.
	     Returns ASN1_OVERRUN if *buf is exhausted. */

#endif