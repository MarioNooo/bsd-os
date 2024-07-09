/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.

     File: enriched.c - Reduce text/enriched to plaint text
     Version: 0.2.3, Dec 1997
     Last Edited: Dec 1 08:12

 *   Revision 0.2.4  Feb 11, 1998  [PY]
 *     Added TextEnrichedToHTML()
 *


  ---- */

#ifndef UNIX

#include <Pilot.h>

#else /* UNIX */

#  include <stdio.h>
#  include <string.h>
#  define StrCompare strcmp

#endif /* UNIX */


#include "enriched.h"
#include "mime.h"
#include "lineend.h"
#include "charmangle.h"

struct etags_rec { const char *enriched_tag; const char *html_tag; int html_tag_len; EnrichedTag et; };


/* ----------------------------------------------------------------------
   Structure to keep track of text/enriched parsing across lines, and
   a function to initialize the data. 
   ---- */
typedef struct textEnricheds {
  int       paramct,
            newlinect,
            nofill,
            curPosInToken;
  OutputFn *oFn;
  void     *oFnState;
  char token[kMaxEnrichedToken];
  EnrichedTag lastIncompleteTag;
} textEnrichedS;


/* Warning, this isn't quite reentrant: */
static textEnrichedS TES;
  
void *textEnrichedInit(OutputFn oFn, void *oFnState, TextCharSetType partCharSet,
                       TextCharSetType reqCharSet)
{
  TES.paramct   = 0;
  TES.newlinect = 0;
  TES.nofill    = 0;
  TES.curPosInToken = 0;
  if( partCharSet != reqCharSet ) {
      TES.oFn = CharMangler;
      TES.oFnState = CharManglerInit( oFn, oFnState, partCharSet, reqCharSet );
  }
  else {
      TES.oFn       = oFn;
      TES.oFnState  = oFnState;
  }
  TES.lastIncompleteTag = NONE;
  return((void *)&TES);
}
  

/* Hack macros to keep the code a little neater for next function */
#define REENTRANT
#ifdef REENTRANT
/* This non-reentrant code is a little smaller - it is untested */

#define PARAMCT      ((textEnrichedS *)TESp)->paramct
#define NEWLINECT    ((textEnrichedS *)TESp)->newlinect
#define NOFILL       ((textEnrichedS *)TESp)->nofill
#define TOKENPOS     ((textEnrichedS *)TESp)->curPosInToken
#define TOKEN        ((textEnrichedS *)TESp)->token
#define OUTPUT(x, y) (*((textEnrichedS *)TESp)->oFn)\
                     (((textEnrichedS *)TESp)->oFnState, (x), (y))

#else /* REENTRANT */

/* Warning: this code is not reentrant */

#define PARAMCT      TES.paramct
#define NEWLINECT    TES.newlinect
#define NOFILL       TES.nofill
#define TOKENPOS     TES.curPosInToken
#define TOKEN        TES.token
#define OUTPUT(x, y) TES.oFn(TES.oFnState, (x), (y))

#endif /* REENTRANT */


/* ----------------------------------------------------------------------
   MIME type filter that strips text/enriched according to algorithm
   in RFC 1896.  If we wanted we could implement a few things like
   <paraindent>....

   Args: TESp  - our state/context pointer
         inBuf - input buffer - NOTE - THIS IS MODIFIED HERE
         len   - length of buffer. 

   Returns: nothing
 
   Note, this assumes the input lines are NULL terminated. The MIME
   parser does guarantee this for non-binary data.

   ---- */
void textEnrichedToPlain(
    void       *TESp,
    char       *inBuf,
    long        len)
{
  static char token[kMaxEnrichedToken];
                 /* Make it static to keep it off the stack */
  char       *p; /* Run along the token */
  (void)      len;


  if(inBuf == NULL) return;
  while(*inBuf) {
    if(*inBuf == '<') {
      if(NEWLINECT == 1)
        OUTPUT(" ", 1);
      NEWLINECT = 0;
      if(*++inBuf == '<') {
        if(PARAMCT <= 0)    
          OUTPUT(inBuf, 1);
      } else {
        for(p = token; *inBuf && *inBuf != '>'; inBuf++) {
          if (p < &token[kMaxEnrichedToken])
            *p++ = *inBuf | 0x20;
            /* Can fold *all* to lower - we only care about compares below */
        }
        *p = '\0';
        if(!*inBuf)
          break;
        if(!StrCompare(token, "param"))
          PARAMCT++;
        else if(! StrCompare(token, "nofill"))
          NOFILL++;
        else if(! StrCompare(token, "/param"))
          PARAMCT--;
        else if(! StrCompare(token, "/nofill"))
          NOFILL--;
      }
    } else {
      if(PARAMCT >0)
        ; /* Ignoring parameters */
      else if(EOLCHECK(inBuf) && NOFILL <= 0) {
        if(++NEWLINECT > 1) 
          OUTPUT(inBuf, 1);
      } else {
        if(NEWLINECT == 1)
          OUTPUT(" ", 1);
        NEWLINECT = 0;
        OUTPUT(inBuf, 1);
      }
    }
    inBuf++;
  }
  return;
}
/*
 * textEnrichedToHTMLHead() : 
 *  - sends <html> Tag that is to precede the data.
 */
void textEnrichedToHTMLHead(
    void       *TESp,
    char       *inBuf,
    long        len)
{
    (void) inBuf;
    (void) len;


    OUTPUT("<html>", 6);
    return;
}
/*
 * textEnrichedToHTMLTail() :
 *  - sends </html> Tag to succeed all the html data.
 */
extern int not_outputting;
void textEnrichedToHTMLTail(
    void       *TESp,
    char       *inBuf,
    long        len)
{
    (void) inBuf;
    (void) len;


    OUTPUT("</html>", 7);
    return;
}
/*
 <TODO>
 * textEnrichedToHTML():
 * (a) ParaIndent is ignored.
 </TODO>
 */
void textEnrichedToHTML (
    void       *TESp,
    char       *inBuf,
    long        len )
{
    const struct etags_rec *pETag;
    static char scColorBuf[kInBufSize] = {0};
    (void) len;


    if(inBuf == NULL) return;
    for(; *inBuf; inBuf++) {
        if(*inBuf == '<' && TOKENPOS == 0) {
            TOKEN[TOKENPOS++] = '<';
            inBuf++;
        }
        if(TOKENPOS) {
            if(*(inBuf) == '<' && TOKENPOS == 1) TOKEN[TOKENPOS++] = '<';
            else {
                for(;*inBuf && *inBuf != '>'; inBuf++) {
                    if (TOKENPOS < kMaxEnrichedToken)
                        TOKEN[TOKENPOS++] = *inBuf | 0x20;
                }
                if(*inBuf == '>') TOKEN[TOKENPOS++] = '>';
            }
            if(*inBuf == '\0') /* Token overflowed */
                return;
        }
        else {
            switch(*inBuf) {
            case '&':
            case '>':
                TOKEN[TOKENPOS++] = *inBuf;
            }
        }
        if(TOKENPOS){ /* Token has been found */
            if(NEWLINECT == 1)OUTPUT("\n", 1);
            NEWLINECT = 0;
            TOKEN[TOKENPOS] = '\0';
            pETag = etags_lookup(TOKEN, TOKENPOS);
            if(pETag){
                switch(pETag->et){
                case XPARAM:
                    PARAMCT--;
                    switch(((textEnrichedS *)TESp)->lastIncompleteTag) {
                    case PARAINDENT:
                        ((textEnrichedS *)TESp)->lastIncompleteTag = NONE;
                        break;
                    case COLOR:
                        if(strchr(scColorBuf, ',')) {
                            int r,g,b;
                            sscanf(scColorBuf,"%x,%x,%x", &r, &g, &b);
                            r = (r >> 8) & 0xff;
                            g = (g >> 8) & 0xff;
                            b = (b >> 8) & 0xff;
                            memset(scColorBuf,0,strlen(scColorBuf));
                            sprintf(scColorBuf,"#%02X%02X%02X", r, g, b);
                        }
                        OUTPUT(scColorBuf, strlen(scColorBuf));
                        memset(scColorBuf,0,strlen(scColorBuf));
                    default:
                        OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                        ((textEnrichedS *)TESp)->lastIncompleteTag = NONE;
                    }
                    break;
                case PARAM:
                    PARAMCT++;
                    break;
                case FONTFAMILY:
                    ((textEnrichedS *)TESp)->lastIncompleteTag = FONTFAMILY;
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                    break;
                case COLOR:
                    ((textEnrichedS *)TESp)->lastIncompleteTag = COLOR;
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                    break;
                case PARAINDENT:
                    ((textEnrichedS *)TESp)->lastIncompleteTag = PARAINDENT;
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                    break;
                case NOFILLV:
                    NOFILL++;
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                    break;
                case XNOFILLV:
                    NOFILL--;
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                    break;
                default:
                    OUTPUT((char *)pETag->html_tag,pETag->html_tag_len);
                }
            }
        }
        else {
            if(*inBuf=='\n' && NOFILL <= 0 && PARAMCT <= 0) {
                if( ++NEWLINECT > 1) OUTPUT("<br>\n",5);
            }
            else {
                if(NEWLINECT == 1) OUTPUT("\n",1);
                NEWLINECT = 0;
                if(PARAMCT <= 0)
                    OUTPUT(inBuf,1);
                else {
                    switch(((textEnrichedS *)TESp)->lastIncompleteTag) {
                    case COLOR:    /* Collecting COLOR string */
                        if(strlen(scColorBuf) < kInBufSize) {
                            scColorBuf[strlen(scColorBuf)] = *inBuf;
                        }
                        break;
                    case FONTFAMILY:
                        OUTPUT(inBuf, 1);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        TOKENPOS = 0;
    }
    return ;
}
