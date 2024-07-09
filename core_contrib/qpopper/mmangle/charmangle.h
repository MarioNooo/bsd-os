/*
 * Copyright (c) 2003 QUALCOMM Incorporated.
 * All rights reserved. The License.txt specifies the License terms
 * QPOPPER.
 *
 * Revisions :
 *  07/22/98 [py]
 *           Created.
 *
 */
#ifndef _CHARMANGLE_H_
#define _CHARMANGLE_H_
#include "config.h"
#include <mime.h>
typedef enum { us_ascii=0,  iso_8859_1,  iso_8859_2,  iso_8859_3, 
	       iso_8859_4,  iso_8859_5,  iso_8859_6,  iso_8859_7,
	       iso_8859_8,  iso_8859_9,  iso_8859_10, iso_8859_11,
	       iso_8859_12, iso_8859_13, iso_8859_14, iso_8859_15,
               utf_8, cp1252, iso_2022_jp }TextCharSetType;

void     *CharManglerInit __PROTO((OutputFn oFn, void *oFnState, 
				TextCharSetType partCharSet,
				TextCharSetType reqCharSet ));
OutputFn CharMangler;
#endif /*  _CHARMANGLE_H_ */
