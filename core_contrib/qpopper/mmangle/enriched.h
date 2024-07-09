/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.

     File: manglers.h - handlers for individual content types
     Version: 0.2.3, Dec 1997
     Last Edited: Nov 30 22:36

  ---- */
#ifndef _MANGLERSINCLUDED
#define _MANGLERSINCLUDED
#include "config.h"
#include <mime.h>
#include "charmangle.h"

typedef enum { NONE = 0, CHARTAG, PARAM, XPARAM, NOFILLV, XNOFILLV,
               BOLD, XBOLD, ITALIC, XITALIC, FIXED, XFIXED, 
               UNDERLINE, XUNDERLINE, SMALLER, XSMALLER, BIGGER,
               XBIGGER, CENTER, XCENTER, FLUSHLEFT, XFLUSHLEFT,
               FLUSHRIGHT, XFLUSHRIGHT, FLUSHBOTH, XFLUSHBOTH,
               EXCERPT, XEXCERPT, FONTFAMILY, XFONTFAMILY, COLOR, XCOLOR,
               PARAINDENT, XPARAINDENT} EnrichedTag;
void     *textEnrichedInit __PROTO((OutputFn oFn, void *oFnState, TextCharSetType current,
				    TextCharSetType reqCharSet));
OutputFn  textEnrichedToPlain;
OutputFn  textEnrichedToHTML;
OutputFn  textEnrichedToHTMLHead;
OutputFn  textEnrichedToHTMLTail;
struct etags_rec; 
extern const struct etags_rec *etags_lookup __PROTO((const char *, int ));

#define kMaxEnrichedToken  62 /* longest text/enriched token, from rfc 1896 */


#endif  /* _MANGLERSINCLUDED */


