/*
 * Copyright (C) 2001  Internet Software Consortium.
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

/* log.c,v 1.1 2003/06/06 22:57:47 polk Exp */

#include <config.h>

#include <isc/util.h>

#include <isccfg/log.h>

/*
 * When adding a new category, be sure to add the appropriate
 * #define to <isccfg/log.h>.
 */
LIBISCCFG_EXTERNAL_DATA isc_logcategory_t cfg_categories[] = {
	{ "config", 	0 },
	{ NULL, 	0 }
};

/*
 * When adding a new module, be sure to add the appropriate
 * #define to <isccfg/log.h>.
 */
LIBISCCFG_EXTERNAL_DATA isc_logmodule_t cfg_modules[] = {
	{ "isccfg/parser",	 0 },
	{ NULL, 		0 }
};

void
cfg_log_init(isc_log_t *lctx) {
	REQUIRE(lctx != NULL);

	isc_log_registercategories(lctx, cfg_categories);
	isc_log_registermodules(lctx, cfg_modules);
}