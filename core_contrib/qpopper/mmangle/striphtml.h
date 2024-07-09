/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright 2003 QUALCOMM Incorporated

     File: striphtml.c
     Version: 0.3.0, May 1999
     Last Edited:

  ---- */

/* Original header; before LGL hacking */
/*
|| File:    striphtml.c - HTML-to-text converter.
|| Author:  Brian Kelley, Qualcomm Inc.
|| Notice:  Copyright 1997 Qualcomm, Inc.
|| Date:    14-Sep-97
|| Hacked by: Laurence Lundblade
||
|| Handles:
|| - SGML tags and comments, including quoted ">" in attribute values
|| - SGML numeric references (eg: &#xx;)
|| - HTML entities (eg: &amp;)
|| - stripping redundant spaces
|| - Paragraph breaks caused by HTML open and close elements
|| - HTML preformatted blocks (PRE, LISTING, XMP, PLAINTEXT)
|| - Special processing escapes: XMP and PLAINTEXT
|| - stripping contents of certain HTML elements (COMMENT, SCRIPT,
||    SELECT, TITLE).
|| - LI items marked with a bullet character (not ISO8859, but
||    compatible with Windows, Mac, Pilot ... I hope)
||
|| This does not_ try to:
|| - Wrap text to a given width
|| - Mimic HTML indentation using spaces
*/

#ifndef _STRIPHTMLINCLUDED
#define _STRIPHTMLINCLUDED
#include "config.h"
#include <mime.h>
#include "charmangle.h"
typedef struct Html Html;

#define URL_BUF_SIZE 1024


/*
|| This callback is used by the Html object to output processed text.
*/

/* typedef void (*OutputFn)(void *pv, const char *buf, int len); */


/*
 || For static and automatic allocation of Html objects, the structure
 || definition is included in this header file.  If using dynamic
 || allocation for Html objects, Html_New() and Html_Delete() can be used
 || instead of Html_Ctor() and Html_Dtor(), and the structure definition
 || can be moved out of the header file into striphtml.c.  That way,
 || client code will have absolutely no dependence on Html's internal
 || structure linked into it. 
 */

extern void Html_Ctor __PROTO((Html *me, OutputFn pfOut, void *pvOut));
extern void Html_Dtor __PROTO(());

/*
|| This is the function you use to feed data to the Html object.
*/
extern void Html_Write __PROTO((Html *me, const char *szText, int cbText));



#define DATAMAX 12     /* maximum number of characters to hold element name */

#define ATTRMAX 20     /* Maximum character from a parameter to save */


struct Html {
   OutputFn *pfOut;
   void *   pvOut;

   int      nState;
   int      chQuote;	     /* terminating quote character when in
                                ssValueQuoted */
   int      cntDashes;       /* used in ssComment state to test for
                                termination of comment */
   int      ndxData;         /* next index into szData */
   char     szData[DATAMAX]; /* current tag name */
   char     szXMP[DATAMAX];  /* special non-SGML mode for HTML compatibility */

   /* Text parsing state */
   int      nTextState;
   int      nBreaks;
   int      nCol;
   
   /* Save an attribute */
   int      nattrName;
   char     attrName[DATAMAX];
   int      nattrVal;
   char     attrVal[ATTRMAX];
};


void     *textHTMLInit __PROTO((OutputFn oFn, void *oFnState, 
				TextCharSetType partCharSet,
				TextCharSetType reqCharSet));
OutputFn  textHTMLToPlain;


/* ----------------------------------------------------------------------
   Conversion of text/plain to HTML
   ---- */
OutputFn  textPlainToHTMLHead;
OutputFn  textPlainToHTML;
OutputFn  textPlainToHTMLTail;


/* ----------------------------------------------------------------------
   Conversion of text/plain format=flowed to HTML
   ---- */
struct Flowed2HTML {
  /* Where to send the output next */
  OutputFn *pfOut;
  void *   pvOut;

  /* Are we in a paragraph? */
  int      m_bInPara;

  /* Count quote depth   */
  int      m_nLastQuoteDepth;
  int      m_nQuoteDepth;
  
  int      m_bLook4Quote;

  /* Are we in a line's content, or at its start? */
  enum {AT_START,
        IN_CONTENT} m_nLineState;

  /* Track runs of spaces (including trailing spaces) */
  int      m_nSpaceCount;

  /* Treat signature separator line specially */
  enum {NO_DASH,
        FIRST_DASH,
        SECOND_DASH,
        DASH_DASH_SPACE} m_nSigSepState;

#ifdef _TRAP_URLS
  /* Buffer potential URLs */
  char     m_cURLbuf [ URL_BUF_SIZE ];
  int      m_nURLcount;
#endif

};

void     *textFlowedToHTMLInit  __PROTO((OutputFn oFn, void *oFnState));
OutputFn  textFlowedToHTMLHead;
void      ffToHTML_lineStart    __PROTO((struct Flowed2HTML *me));
OutputFn  textFlowedToHTML;
OutputFn  textFlowedToHTMLTail;


#endif /*_ MIMEMANGLEINCLUDED */

