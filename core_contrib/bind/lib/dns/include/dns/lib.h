/*
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* lib.h,v 1.1 2003/06/06 22:57:03 polk Exp */

#ifndef DNS_LIB_H
#define DNS_LIB_H 1

#include <isc/types.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

extern isc_msgcat_t *dns_msgcat;

void
dns_lib_initmsgcat(void);
/*
 * Initialize the DNS library's message catalog, dns_msgcat, if it
 * has not already been initialized.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_LIB_H */
