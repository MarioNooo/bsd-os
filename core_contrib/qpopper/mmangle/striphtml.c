/* ----------------------------------------------------------------------
     MIME Mangler -- single pass reduction of MIME to plain text

     Brian Kelley,
     Laurence Lundblade <lgl@qualcomm.com>,
     Randy Gellens

     Copyright 2003 QUALCOMM Incorporated.  All rights reserved.
     The file License.txt specifies the terms for use, modification, and 
     redistribution.

     File: striphtml.c
     Version: 0.3.1, May 1999
     Last Edited: 23 Januay 2001

  ---- */
     
/* Original header; before LGL hacking */

/*
|| File       : striphtml.c
|| Date       : 14-Sep-97
|| Description: HTMl Stripper
||
|| Author     : Brian Kelley
|| Copyright  : Qualcomm, Incorporated, 1997
|| Revisions  :
||
|| 14-Sep-97 : Converted ewhtml.ccp (WebPad project) to C and removed most
|| of its functionality.
||
|| Notes:
||
|| - This uses no external functions (no libc or anything) for OS 
||   independence.
||
|| - The main OS dependency here is character set issues: The line delimiter
||   character(s) (haven't provided for two-byte line terminators, but I
||   easily could)
||   and the trademark character differ between Windows and Mac/Pilot.
||
|| - Locating entities and elements with a binary search would be faster.
||
|| - The value for the "trade" entity is different for Macs and the Pilot.
||   I'm not sure what the index is.
||
*/

#ifndef UNIX
#include <Pilot.h>
#else /* UNIX */
#include <stdio.h>
#define StrCompare strcmp
#define StrCopy    strcpy
#define StrCat     strcat
#define StrLen     strlen

#endif /* UNIX */

#include "utils.h"
#include "striphtml.h"
#include "lineend.h"

#ifdef _DEBUG
#  include <stdio.h>
#  include <string.h>
#  include "popper.h"
#endif


/* SGML parsing states */
enum {
   ssData, ssTagOpen, ssTagName, ssTagGap, ssAttrName, ssAttrGap, ssAttrEqGap,
   ssValue, ssValueQuoted, ssAttrComplete, ssEntity, ssEntityBody, ssComment
};


/* Text processing states */
enum {
   tsInWord, tsInSpace, tsTitle, tsComment, tsPre
};

#ifdef _DEBUG
extern POP *global_debug_p;
#endif

static void Html_OpenElem(Html *me, int nTok);
static void Html_CloseElem(Html *me, int nTok);
static void Html_Text(Html *me, CSTR szText, int cbText);


/*-------------   Data   -------------*/


/*
|| Note on entities:
||  -  I think "trade" is standard as of HTML3.0.  153 is the correct 
||     index for MS Windows; the correct index is something different 
||     for Mac/Pilot.
||  -  Entities are NOT case insensitive, but we have some Mosaic legacy 
||     to deal with (GT, gt, LT, lt, AMP, amp).
*/

const char gszEntNames[] = 
   "AElig;AMP;Aacute;Acirc;Agrave;Aring;Atilde;Auml;Ccedil;ETH;Eacute;Ecirc;Egrave;Euml;GT;Iacute;Icirc;"
   "Igrave;Iuml;LT;Ntilde;Oacute;Ocirc;Ograve;Oslash;Otilde;Ouml;QUOT;THORN;Uacute;Ucirc;Ugrave;Uuml;Yacute;"
   "aacute;acirc;acute;aelig;agrave;amp;aring;atilde;auml;brvbar;ccedil;cedil;cent;copy;curren;deg;die;divide;"
   "eacute;ecirc;egrave;eth;euml;frac12;frac14;frac34;gt;iacute;icirc;iexcl;igrave;iquest;iuml;laquo;lt;macr;"
   "micro;middot;nbsp;not;ntilde;oacute;ocirc;ograve;ordf;ordm;oslash;otilde;ouml;para;plusmn;pound;quot;"
   "raquo;reg;sect;shy;sup1;sup2;sup3;szlig;thorn;times;trade;uacute;ucirc;ugrave;uuml;yacute;yen;yuml;"
      ;

unsigned char gacEntValues[] = {
   '\xC6',
   '&',
   '\xC1',
   '\xC2',
   '\xC0',
   '\xC5',
   '\xC3',
   '\xC4',
   '\xC7',
   '\xD0',
   '\xC9',
   '\xCA',
   '\xC8',
   '\xCB',
   '>',
   '\xCD',
   '\xCE',
   '\xCC',
   '\xCF',
   '<',
   '\xD1',
   '\xD3',
   '\xD4',
   '\xD2',
   '\xD8',
   '\xD5',
   '\xD6',
   '"',
   '\xDE',
   '\xDA',
   '\xDB',
   '\xD9',
   '\xDC',
   '\xDD',
   '\xE1',
   '\xE2',
   '\xB4',
   '\xE6',
   '\xE0',
   '&',
   '\xE5',
   '\xE3',
   '\xE4',
   '\xA6',
   '\xE7',
   '\xB8',
   '\xA2',
   '\xA9',
   '\xA4',
   '\xB0',
   '\xA8',
   '\xF7',
   '\xE9',
   '\xEA',
   '\xE8',
   '\xF0',
   '\xEB',
   '\xBD',
   '\xBC',
   '\xBE',
   '>',
   '\xED',
   '\xEE',
   '\xA1',
   '\xEC',
   '\xBF',
   '\xEF',
   '\xAB',
   '<',
   '\xAF',
   '\xB5',
   '\xB7',
   '\xA0',
   '\xAC',
   '\xF1',
   '\xF3',
   '\xF4',
   '\xF2',
   '\xAA',
   '\xBA',
   '\xF8',
   '\xF5',
   '\xF6',
   '\xB6',
   '\xB1',
   '\xA3',
   '"',
   '\xBB',
   '\xAE',
   '\xA7',
   '\xAD',
   '\xB9',
   '\xB2',
   '\xB3',
   '\xDF',
   '\xFE',
   '\xD7',
   153,
   '\xFA',
   '\xFB',
   '\xF9',
   '\xFC',
   '\xFD',
   '\xA5',
   '\xFF',
};



/*
 *  We can ignore all style-level attributes, but we need to know about
 * tags that affect (wrapping on/off, blank lines separating paragraphs, 
 * etc.) Currently, attibutes are ignored, so IMG cannot be used to
 * display ALT text.
 */

char gszElemNames[] = "a;address;blockquote;br;comment;dd;dir;div;dl;dt;hr;h1;h2;h3;h4;h5;h6;img;li;listing;menu;ol;p;plaintext;pre;script;select;title;xmp;";

unsigned char gnElemBreaks[] = {
   0, /* NONE */
   0, /* a */
   2, /* address */
   2, /* blockquote */
   0, /* br */
   0, /* comment */
   1, /* dd */
   1, /* dir */
   1, /* div */
   2, /* dl */
   1, /* dt */
   1, /* hr */
   2, /* h1 */
   2, /* h2 */
   2, /* h3 */
   2, /* h4 */
   2, /* h5 */
   2, /* h6 */
   0, /* img */
   1, /* li */
   2, /* listing */
   2, /* menu */
   2, /* ol */
   2, /* p */
   2, /* plaintext */
   2, /* pre */
   0, /* script */
   0, /* select */
   0, /* title */
   2, /* xmp */
};


enum HtmlElems {
   ELEM_NONE,
   ELEM_A,
   ELEM_ADDRESS,
   ELEM_BLOCKQUOTE,
   ELEM_BR,
   ELEM_COMMENT,
   ELEM_DD,
   ELEM_DIR,
   ELEM_DIV,
   ELEM_DL,
   ELEM_DT,
   ELEM_HR,
   ELEM_H1,
   ELEM_H2,
   ELEM_H3,
   ELEM_H4,
   ELEM_H5,
   ELEM_H6,
   ELEM_IMG,
   ELEM_LI,
   ELEM_LISTING,
   ELEM_MENU,
   ELEM_OL,
   ELEM_P,
   ELEM_PLAINTEXT,
   ELEM_PRE,
   ELEM_SCRIPT,
   ELEM_SELECT,
   ELEM_TITLE,
   ELEM_XMP
};

#ifdef _TRAP_URLS
static char gszURLchars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789+.-%#;/?:@=&$_*'()\n";
#endif


/*------------  Helper functions  -----------*/




/*
 * Find ID of element szElem  (1 ... ELEM_XMP)
 *
 * Return 0 if not found.
 */
static int FindE(char *szE, int what, int *pnLen)
{
   /*
   ||   - First character in gszElemNames is the first character of the
   ||     first entity (number 1)
   ||   - Last character in gszElemNames (before the terminating zero)
   ||      must be ";"
   */
   const char *szSrch = what ? gszEntNames : gszElemNames;
   int id = 1;
   
   while (*szSrch) {
      char *sz = szE;
      while (what ? *szSrch == *sz : CharEQI(*szSrch, *sz)) {
         ++szSrch;
         ++sz;
         if (*szSrch == ';') {
           if (what ? !CharIsAlnum(*sz) : !*sz) {
             *pnLen = sz - szE;
             return id;
            } else
               break;                   /* no good */
         }
      }
      
      while (*szSrch++ != ';')
         ;

      ++id;
   }

   return 0;
}


/*
|| Process an HTML entity reference (&name;) or a numeric character
||  reference (&#nnn;)
||
|| Entities: Find the value by looking the name up in gszEntNames and
|| indexing value in gacEntValues.
|| Numbers:  Evaluate the integer following the '#'
||
|| If found, - set *pnLen to the length of the part of szEnt[] which was used.
||           - return the corresponding item in gacEntValues[]
||
|| If not found,  return 0 and leave *pnLen unmodified.
*/
static char FindEnt(char *szEnt, int *pnLen)
{
  int id;

   if (*szEnt == '#') {
      /* numeric reference */
      char *szNum = szEnt + 1;
      int nNum = ScanInt((CSTR *)&szNum);

      if (*szNum == ';')
         ++szNum;

      *pnLen = szNum - szEnt;
      return nNum;
   } else {


   /*
   ||  - First character in gszEntNames is the first character of the
   ||     first entity
   ||  - Last character in gszEntNames (before the terminating zero) must
   ||      be ";"
   */
     id =  FindE(szEnt, 1, pnLen);
     if(id)
       return gacEntValues[id - 1];
   }
   return 0;
}



/*----------------  Html member functions  ----------------*/



#define DCLEAR()      (me->ndxData = 0)
#define DTERMIN()     (me->szData[me->ndxData] = 0)
#define DLENGTH()     (me->ndxData)
#define DAPPEND(ch)   (void) ( (me->ndxData < DATAMAX-1) \
                                && (me->szData[me->ndxData++] = ch) )

#define AN_CLEAR()    (me->nattrName = 0)
#define AN_TERMIN()   (me->attrName[me->nattrName] = 0)
#define AN_LENGTH()   (me->nattrName)
#define AN_APPEND(ch) (void) ( (me->nattrName < DATAMAX-1) \
                               && (me->attrName[me->nattrName++] = ch) )

#define AV_CLEAR()    (me->nattrVal = 0)
#define AV_TERMIN()   (me->attrVal[me->nattrVal] = 0)
#define AV_LENGTH()   (me->nattrVal)
#define AV_APPEND(ch) (void) ( (me->nattrVal < ATTRMAX-1) \
                                && (me->attrVal[me->nattrVal++] = ch) )


#define OUTCH(ch)          (me->pfOut(me->pvOut, &(ch), 1))
#define OUTBUF(buf, len)   (me->pfOut(me->pvOut, buf, len))

#define TEXTCH(ch)         (Html_Text(me, &ch, 1))
#define TEXTSZ(sz)         (Html_Text(me, sz, StrLen(sz)))
#define TEXTBUF(buf,len)   (Html_Text(me, buf, len))

#define SETXMP(str)        (StrCopy(me->szXMP, str))


/* 
   The output functions sometimes change the buffer we pass them. Don't
   want them messing up our newline string.
  */
static void outNewline(Html *me)
{
  char tmp[3];
  (void) me;


  StrCopy(tmp, lineEnd);
  OUTBUF(tmp, EOLLEN);
}



/* ----------------------------------------------------------------------
    Header for conversion of plain text to HTML
   ---- */
void textPlainToHTMLHead(void *state, char *szText, long len)
{
    Html *me = (Html *)state;
    (void) szText;
    (void) len;


#ifdef _DEBUG
   OUTBUF("<!---generated by textPlainToHTML---->\n", 39);
#endif

    OUTBUF("<html><pre>", 11);
    return;
}


/* ----------------------------------------------------------------------
    Tail for conversion of plain text to HTML
   ---- */
void textPlainToHTMLTail(void *state, char *szText, long len)
{
    Html *me = (Html *)state;
    (void) szText;
    (void) len;


    OUTBUF("</pre></html>\n", 14);
    return;
}


/* ----------------------------------------------------------------------
    Convert body of text from plain to HTML 
   ---- */
void textPlainToHTML(void *state, char *szText, long len)
{
   Html *me = (Html *)state;
   char ch;
   CSTR pc = szText;
   (void) len;


   if(szText == NULL) return;

   while ( ( ch = *pc++ ) != '\0' ) {
     switch ( ch ) {
     case '<':
       OUTBUF("&lt;",4);
       break;
     case '>':
       OUTBUF("&gt;",4);
       break;
     case '&':
       OUTBUF("&amp;",5);
       break;
     default:
       OUTCH( ch );
       break;
     }
   }
   return;
}


/* ----------------------------------------------------------------------
    Header for conversion of flowed text to HTML
   ---- */

void *textFlowedToHTMLInit(OutputFn oFn, void *oFnState)  
{
   static struct Flowed2HTML f2h;

   f2h.pfOut = oFn;
   f2h.pvOut = oFnState;

  return((void *)&f2h);
}

/* ----------------------------------------------------------------------
    Header for conversion of flowed text to HTML
   ---- */

void textFlowedToHTMLHead(void *state, char *szText, long len)
{
   struct Flowed2HTML *me = (struct Flowed2HTML *)state;
   (void) szText;
   (void) len;


#ifdef _DEBUG
   OUTBUF("<!---generated by textFlowedToHTML---->\n", 40);
#endif
 
   OUTBUF("<html>\n", 7);
   OUTBUF("<p>\n", 4);
   me->m_bInPara         = 0;
   me->m_nLastQuoteDepth = 0;
   me->m_nQuoteDepth     = 0;
   me->m_nLineState      = AT_START;
   me->m_nSpaceCount     = 0;
   me->m_nSigSepState    = NO_DASH;

#ifdef _TRAP_URLS
   me->m_cURLbuf[0]      = 0;
   me->m_nURLcount       = 0;
#endif /* _TRAP_URLS */

   return;
}

/* ----------------------------------------------------------------------
    Tail for conversion of flowed text to HTML
   ---- */
void textFlowedToHTMLTail(void *state, char *szText, long len)
{
  struct Flowed2HTML *me = (struct Flowed2HTML *)state;
  (void) szText;
  (void) len;


#ifdef _TRAP_URLS
  if (me->m_nURLcount > 0) 
  {
      /* Process characters in potential URL buffer */

     int count;
     int start;
         
     if (me->m_cURLbuf[0] == '<')
     {
       start = 1;
       OUTBUF("&lt;", 4);
     }
     else
       start = 0;

     count = me->m_nURLcount - start;
     me->m_nURLcount        = 0;
     textFlowedToHTML(state, me->m_cURLbuf + start, count);
  }
#endif /* _TRAP_URLS */

  OUTBUF("</html>\n", 8);
  return;
}


/* ----------------------------------------------------------------------
    Handle start of line content when converting from flowed to HTML
    ---- */
void ffToHTML_lineStart(struct Flowed2HTML *me)
{
   /* Handle quote depth changes */

   if (me->m_nQuoteDepth > me->m_nLastQuoteDepth) 
   {
     /* We're now at a deeper quote depth */
             
     int n;
     for (n = me->m_nQuoteDepth - me->m_nLastQuoteDepth; n; n--)
       OUTBUF("<blockquote type=cite>", 12);
   }
           
   else 
   if (me->m_nQuoteDepth < me->m_nLastQuoteDepth)
   {
     /* We're now at a lower quote depth */
     
     int n;
     for (n = me->m_nLastQuoteDepth - me->m_nQuoteDepth; n; n--)
       OUTBUF("</blockquote>", 13);
   }

   me->m_nLastQuoteDepth = me->m_nQuoteDepth;
   me->m_nQuoteDepth     = 0;
   me->m_nLineState      = IN_CONTENT;
}


/* ----------------------------------------------------------------------
    Convert body of text from flowed to HTML 
   ---- */
void textFlowedToHTML(void *state, char *szText, long len)
{
   struct Flowed2HTML *me = (struct Flowed2HTML *)state;
   char ch;
   CSTR pc = szText;
   int  bCharProcessed  = FALSE;
   (void) len;


   if(szText == NULL) return;

   while ( ( ch = *pc++ ) != '\0' ) 
   { /* main loop */

     bCharProcessed = FALSE;
     
#ifdef _TRAP_URLS
     if (me->m_nURLcount > 0)
     {
       /* we are inside a possible URL */
       
       if (me->m_cURLbuf[0] == '<' && ch == '>')
       {
         /* We've found the end of the URL */

         /* Output the URL as a URL, then again encoded */

         long j;
         char *p;

         
#ifdef _DEBUG
   pop_log(global_debug_p, POP_DEBUG, "found end of URL");
#endif
         p = me->m_cURLbuf;
         j = me->m_nURLcount;
         OUTBUF("<A HREF=\"", 9);
         while (j && p && *p)
         {
           if (*p != '\n')
             OUTCH(*p);
           j--;
           p++;
         }
         OUTBUF("\">", 2);

         p = me->m_cURLbuf;
         j = me->m_nURLcount;
         while (j && p && *p)
         {
           switch(*p)
           {
             case '<':
             OUTBUF("&lt;",4);
             break;
      
             case '>':
             OUTBUF("&gt;", 4);
             break;

             case '&':
             OUTBUF("&amp;",5);
             break;

             case '\n':
             break;
  
             default:
             OUTCH( *p );
             break;
           }
           j--;
           p++;
         }

         OUTBUF("&gt;</A>", 8);

       }
       else
       if (strchr(gszURLchars, ch) != NULL ||
           me->m_nURLcount + 2 == URL_BUF_SIZE)
       {
         /* Must not be in a URL after all.  Output '&lt;' instead of opening  '>',
            add current character to the buffer (so it gets processed), then call
            ourselves to output the remaining buffered characters. */

         int count, start;


#ifdef _DEBUG
   pop_log(global_debug_p, POP_DEBUG, "...decided we don't have a URL after all");
#endif
         
         if (me->m_cURLbuf[0] == '<')
         {
           start = 1;
           OUTBUF("&lt;", 4);
         }
         else
           start = 0;

         count = me->m_nURLcount;
         me->m_cURLbuf[count++] = ch;
         me->m_nURLcount        = 0;

         textFlowedToHTML(state, me->m_cURLbuf + start, count - start);
       }
       else
       {
         /* We must be still inside the URL */

         me->m_cURLbuf[me->m_nURLcount++] = ch;        
       }
     } /* me->m_nURLcount > 0 */
     else
#endif /* _TRAP_URLS */


     if (me->m_nLineState == AT_START)
     {
       switch (ch)
       {
         case '>':
           me->m_nQuoteDepth++; /* count quote depth of this line */
           bCharProcessed = TRUE;
           break;

         case ' ': 
           bCharProcessed = TRUE; /* ignore 'stuffed' space */
           ffToHTML_lineStart(me);
           break;

         case '-':
           /* Treat signature separator line specially: '-- ' is not flowed.
              To do this, we note here a line which starts with '-', and 
              in line content handling (below) we check for the second dash
              and the space */
           me->m_nSigSepState = FIRST_DASH;
           ffToHTML_lineStart(me);
           OUTBUF("-", 1);
           bCharProcessed = TRUE;
           break;

         default:
           ffToHTML_lineStart(me);
           break;

       } /* switch */
     } /* LineState == AT_START */

     if (bCharProcessed == FALSE && me->m_nSigSepState != NO_DASH)
     {
       switch (me->m_nSigSepState)
       {
         case FIRST_DASH:
           me->m_nSigSepState = (ch == '-') ? SECOND_DASH : NO_DASH;
           break;

         case SECOND_DASH:
           me->m_nSigSepState = (ch == ' ') ? DASH_DASH_SPACE : NO_DASH;
           break;

         case DASH_DASH_SPACE:
           me->m_nSigSepState = (ch == '\n') ? DASH_DASH_SPACE : NO_DASH;
           break;

         case NO_DASH:
           break;
       }
     }

     if (bCharProcessed == FALSE)
     {
       if (ch == ' ')
       {
         me->m_nSpaceCount++;
         bCharProcessed = TRUE;
       }
       
       else if (me->m_nSpaceCount > 0)
       {
         for (me->m_nSpaceCount--; me->m_nSpaceCount; me->m_nSpaceCount--)
           OUTBUF("&nbsp;", 6);
         OUTBUF(" ", 1);
           
         if (ch == '\n' && me->m_nSigSepState != DASH_DASH_SPACE)
         { 
           me->m_bInPara = 1; /* Soft new line */
           me->m_nLineState = AT_START;
           me->m_nSigSepState = NO_DASH;
           bCharProcessed = TRUE;
         }
       }
     }


#ifdef _TRAP_URLS
     else if (ch == '<')
     {
       /* Might be start of URL, or might be just a '>' character.  Buffer characters
          until we know (or can guess) */
       
       if (me->m_nURLcount == 0)
       {
         me->m_cURLbuf[0] = ch;
         me->m_nURLcount  = 1;
       }

     } /* open angle-bracket */
#endif /* TRAP_URLS */


     if (bCharProcessed == FALSE)
     {
       switch(ch) 
       {
         case '<':
           OUTBUF("&lt;",4);
           break;

         case '>':
           OUTBUF("&gt;",4);
           break;

         case '&':
           OUTBUF("&amp;",5);
           break;

         case '\n':
           if (me->m_bInPara)
             OUTBUF("<P>\n", 4);
           else
             OUTBUF("<BR>\n", 5);

           me->m_bInPara      = 0;
           me->m_nLineState   = AT_START;
           me->m_nSigSepState = NO_DASH;
           bCharProcessed     = TRUE;
           break;
  
         default:
           OUTCH( ch );
           break;
       } /* switch */

     } /* bCharProcessed == FALSE */

   } /* main (character) loop */
   return;
 }




void textHTMLToPlain(void *state, char *szText, long len)
{
   CSTR pc = szText;
   char ch;
   Html *me = (Html *)state;
   int   junk;
   (void) len;


   if(szText == NULL) return;

   while ( ( ch = *pc++ ) != '\0' ) {

   immediate_transition:

      switch (me->nState) {
      case ssData:
         if (ch == '<') {
            DCLEAR();
            me->nState = ssTagOpen;
         }

         else if (ch == '&' && !me->szXMP[0]) {
            DCLEAR();
            me->nState = ssEntity;
         }

         else
            TEXTCH(ch);
         break;

      case ssComment:
         if (ch == '>'
             && me->cntDashes >= 2) {/* standard SGML use "(me->cntDashes % 4) < 2" */
            /* comment terminated */
            me->nState = ssData;
            break;
         }
         if (ch == '-')
            ++me->cntDashes;
         else
            me->cntDashes = 0;
         break;

         /*
         || ssTagOpen : First character following '<'
         */
      case ssTagOpen:
         if (CharIsAlpha(ch) || ch == '!' || ch == '/') {
            /* Moscape takes only alpha, "!" and "/" as tag opening characters */
            /* All other "tags" will be passed through verbatim. */
            DAPPEND(ch);
            me->nState = ssTagName;
            AN_CLEAR();
         } else {
            TEXTBUF("<", 1);    /* ssTagOpen is entered only from ssData */
            me->nState = ssData;
            goto immediate_transition;
         }
         break;


         /*
         || ssTagName : Characters in a tag name
         */
      case ssTagName:
         if (CharIsSgmlTok(ch)) {
            DAPPEND(ch);
         } else {
            DAPPEND(0);/* terminate the tag name, go to next buffer position */

            /*
            || If in XMP all tags should be ignored except a matching close tag. 
            || In PLAINTEXT, all tags are ignored, period.
            */
            if (me->szXMP[0]
                && !(me->szData[0] == '/' && StrEQI(me->szData+1, me->szXMP))) {
                /* Not a matching close tag => spit it out as it was received. */
                TEXTBUF("<", 1);
                me->nState = ssData;
                TEXTSZ(me->szData);
                goto immediate_transition;         /* process ch also */
            }
            else if (StrBeginsI("!--", me->szData)) {
               /* Begin comment */
               me->cntDashes = 2;
               me->nState = ssComment;

               /* Process characters after "!--" that we've already received */
               /* [The only chars that could be of importance here are       */
               /* dashes; this ensures an accurate count.]                   */
               TEXTBUF(me->szData + 3, DLENGTH() - 4);
                      /* 4 = 3 for "!--"  +  1 for DAppend(0) */
               goto immediate_transition;       /* and process ch also */
            }
            goto enter_ssTagGap;
         } /* else ignore */
         break;

         /*
         || ssTagGap:  wait for attribute or end of tag
         */

      enter_ssTagGap:
         me->nState = ssTagGap;
         /* fallthrough */
      case ssTagGap:
         if (ch == '>') {
            /*
            || End of open or close tag
            */
            BOOL bClose = (me->szData[0] == '/');
            int id = FindE(me->szData + bClose, 0, &junk);

            if (bClose)
               Html_CloseElem(me, id);
            else
               Html_OpenElem(me, id);

            me->nState = ssData;
         } else if (CharIsSgmlTok(ch)) {
            AN_CLEAR();
            AV_CLEAR();
            goto enter_ssAttrName;
         }
         /* else ignore */
         break;

         /*
         || ssAttrName : get attribute name.  Entered when a token-start
         || is encountered
         */

      enter_ssAttrName:
         me->nState = ssAttrName;
         /* fallthrough */
      case ssAttrName:
         if (CharIsSgmlTok(ch)) {
            /* part of the attribute name; */
            AN_APPEND(ch);
            break;
         } else if (ch > 32 && ch != '>' && ch != '=') {
            break;  /* ignore */
         }

         me->nState = ssAttrGap;
         /* fallthrough: goto immediate_transition to ssTagAttrGap */

      case ssAttrGap:
        AN_TERMIN();
        if(AN_LENGTH() && (
            StrEQI(me->attrName, "alt") || StrEQI(me->attrName, "href"))){
          /* Got an attribute we care about */
          AV_CLEAR();
        } else {
          AN_CLEAR();
        }
        if (ch == '=') {
          me->nState = ssAttrEqGap;
        } else if (ch == '>') {
          goto enter_ssAttrComplete;
        }
        /* else ignore */
        break;

      case ssAttrEqGap:         /* after attr and "=" */
         if (ch ==  '"' || ch == '\'' || ch == '[') {
            me->chQuote = (ch == '[' ? ']' : ch);
            me->nState = ssValueQuoted;
         } else if (ch == '>') {
            goto enter_ssAttrComplete;
         } else if (ch > 32) {
            me->nState = ssValue;
         }
         break;

         /*
         || Inside an unquoted attribute value
         */
      case ssValue:
         if (ch != '>' && ch > 32) {
           if(AN_LENGTH())
          AV_APPEND(ch);
            break;
         }
         /* fallthrough */

         /*
         || ssAttrComplete : Complete the attribute and process ch
         */

      enter_ssAttrComplete:
      case ssAttrComplete:
      if(AV_LENGTH())
        AV_TERMIN();
     
      me->nState = ssTagGap;
         goto immediate_transition;


      case ssValueQuoted:
         if (ch == me->chQuote) {
            me->nState = ssAttrComplete;
         } else {
            /* character is part of the quoted value */
            if(AN_LENGTH())
          AV_APPEND(ch);
         }
         break;

         /*
         || First character of entity or numeric reference (maybe)
         */
      case ssEntity:
        if (ch == '#' || CharIsAlpha(ch)) {
           /* Could be a valid entity */
            DAPPEND(ch);
            me->nState = ssEntityBody;
         } else {
            /* False alarm; regurgitate '&' */
            TEXTBUF("&", 1);
            me->nState = ssData;
            goto immediate_transition;  /* process ch right now */
         }
         break;

      case ssEntityBody:
         if (CharIsAlnum(ch) && me->ndxData <= 6) {
            DAPPEND(ch);
         } else {
            /* couldn't be a valid, properly-terminated entity (unless ch == ';') */
            DTERMIN();
            {
             int nLen = 0;
             char chEnt = FindEnt(me->szData, &nLen);

             if (nLen)
                TEXTCH(chEnt);
             else
                TEXTBUF("&", 1);

             /* Output the characters we've read which aren't part of the entiy. */
             me->nState = ssData;

             /* If there were any unused characers OR if ch is not an acceptable terminator, */
             /*    output those characters and the terminator. */
             if (nLen < me->ndxData || ch != ';') {
                while (nLen < me->ndxData)
                   TEXTCH(me->szData[nLen++]);
                goto immediate_transition;
             }
            }
         }
         break;
      }
   }
}


/*
||  Output nBreaks line breaks.
*/
static void Html_WriteBreaks(Html *me, int nBreaks)
{
   me->nBreaks += nBreaks;

   while(nBreaks--)
     outNewline(me);

   if (me->nTextState == tsInWord)
     me->nTextState = tsInSpace;
}


/*
|| Output line breaks until we have output nEnsure consecutive line breaks.
*/
static void Html_EnsureBreak(Html *me, int nEnsure)
{
   if (nEnsure > me->nBreaks)
      Html_WriteBreaks(me, Constrain(nEnsure - me->nBreaks, 0, 2));
}


/*
|| Called when an open tag is encountered.
*/
static void Html_OpenElem(Html *me, int nTok)
{
   Html_EnsureBreak(me, gnElemBreaks[nTok]);

   switch (nTok) {
   case ELEM_BR:
      Html_WriteBreaks(me, 1);
      break;

   case ELEM_TITLE:
   case ELEM_COMMENT:
   case ELEM_SCRIPT:
   case ELEM_SELECT:
      me->nTextState = tsComment;
      break;

   case ELEM_LI:
     /*      OUTBUF("\225  ", 3); */
      OUTBUF("*  ", 3);
      me->nBreaks = 0;
      break;


      /* breaks==2 for these: */
   case ELEM_PLAINTEXT:
   case ELEM_XMP:
      SETXMP(nTok == ELEM_XMP ? "XMP" : " ");   /* " " cannot be matched => PLAINTEXT never exits */
      /* FALLTHROUGH */

   case ELEM_PRE:
   case ELEM_LISTING:
      me->nCol = 0;
      me->nTextState = tsPre;
      break;
     
   case ELEM_IMG:
     if(AN_LENGTH() && StrEQI(me->attrName, "alt") && AV_LENGTH()) {
       OUTBUF("[ Image ", 8);
       OUTBUF(me->attrVal, AV_LENGTH());
       OUTBUF(" ]", 2);
       AN_CLEAR();
     }
     break;
   
   case ELEM_A:
     if(AN_LENGTH() && StrEQI(me->attrName, "href") && AV_LENGTH())
       OUTBUF(" <", 2);
       OUTBUF(me->attrVal, AV_LENGTH());
       OUTBUF("> ", 2);
     AN_CLEAR();
     break;
     
   case ELEM_HR:
     OUTBUF("---------------------", 20);
     outNewline(me);
     break;
   }
}


/*
|| Called when a close tag is encountered.
*/
static void Html_CloseElem(Html *me, int nTok)
{
   switch (nTok) {
   case ELEM_XMP:
   case ELEM_PLAINTEXT:
      SETXMP("");
     /* FALLTHROUGH */

   case ELEM_PRE:
   case ELEM_LISTING:
     me->nBreaks = (me->nCol == 0);       /* If me->nCol==0, at least one break preceded */
     me->nTextState = tsInSpace;            /* should probably "pop" state */
     Html_EnsureBreak(me, 2);
     break;

   case ELEM_TITLE:
   case ELEM_COMMENT:
   case ELEM_SCRIPT:
   case ELEM_SELECT:
     me->nTextState = tsInSpace;    /* should probably "pop" state */
     break;
   }

   Html_EnsureBreak(me, gnElemBreaks[nTok]);
}



static void Html_Text(Html *me, CSTR szText, int cbText)
{
   int nn;
   for (nn = 0; nn < cbText; ++nn) {
      char ch = szText[nn];

      switch (me->nTextState) {
      case tsInWord:
         if (CHARISSPACE(ch)) {
            OUTBUF(" ", 1);
            me->nTextState = tsInSpace;
         }
         else OUTCH(ch);
         break;

      case tsInSpace:
         if ( !CHARISSPACE(ch) ) {
            me->nBreaks = 0;
            OUTCH(ch);
            me->nTextState = tsInWord;
         }
         break;

      case tsPre:
         if (ch == '\n') {
            outNewline(me);
            me->nCol = 0;
         } else if (ch == '\t') {
            /* 
            ||     We are not expanding tabs, but nCol is still useful for determining whether
            ||     we are at the start of a line.
            || do
            ||   OUTCH(' ');
            || while ((++me->nCol % 8) != 0);
            */
            OUTCH(ch);
            ++me->nCol;
         } else if (((unsigned char)ch) >= 32) {
            OUTCH(ch);
            ++me->nCol;
         }
         break;

      case tsTitle:
      case tsComment:
         /* Ignore, for now. */
         break;

      }
   }
}



void *textHTMLInit(OutputFn oFn, void *oFnState, TextCharSetType partCharSet,
                   TextCharSetType reqCharSet)  
{
  static Html html;

  if( partCharSet != reqCharSet ) {
      html.pvOut = CharManglerInit( oFn, oFnState, partCharSet, reqCharSet );
      html.pfOut = CharMangler;
  }
  else {
      html.pfOut = oFn;
      html.pvOut = oFnState;
  }

   html.nState = ssData;
   html.ndxData = 0;
   html.szXMP[0] = 0;

   html.nTextState = tsInSpace;
   html.nBreaks = 2;
   html.nCol = 0;

  return((void *)&html);
}
  
                                                                                                        
