/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
     The license.txt file specifies the terms for use, modification, and 
     redistribution.

     File: mangle.c - Functions called back by mime parser to do mangling
     Version: 0.2.4, Jan 2001
     Last Edited: Jan 29 08:20
   
     These are the call back functions we supply to the MIME parser. They
     implement all the specific handling of parts that reduced MIME down
     to plain text.

     Revisions:
            Jun 20, 2000.  [rg]
            - Asking for an unsupported transormation (such as text/enriched)
              gets you text/plain.
            Jun 10, 2000.  [rg]
            - FillMangleInfo now returns 0 on success instead of random.
            Apr 28, 2000.  [rg]
            - Ignore external body parts.
            Feb 03, 2000.  [rg]
            - Removed K&R syntax from FillMangleInfo to make HP compiler 
              happier.
            May 01, 1998.  [py]
            - Handling for M/Alternate and M/Report
            - Handling of headers for Message/ parts.
            Mar 20, 1998.  [py]
            - Added support for Mangle Extension.
            - Modified headerHandler to display required Mime Headers.
  ---- */


#include "mime.h"
#include "lineend.h"
#include "enriched.h"
#include "striphtml.h"
#include "mangle.h"
#include "charmangle.h"
#include "utils.h"

#ifdef PILOT
#  include <Pilot.h>
#else /* UNIX */
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  define StrCompare strcmp
#  define StrCopy    strcpy
#  define StrCat     strcat
#  define StrLen     strlen
#  define StrStr     strstr
#endif /* PILOT */


static char *textAttributes[] = { "plain", "html", "enriched" };
static char *charsetAttributes[] = { "us-ascii",   "iso-8859-1", "iso-8859-2",
                                     "iso-8859-3", "iso-8859-4", "iso-8859-5",
                                     "iso-8859-6", "iso-8859-7", "iso-8859-8",
                                     "iso-8859-9", "iso-8859-10","iso-8859-11",
                                     "iso-8859-12","iso-8859-13","iso-8859-15",
                                     "iso-8859-15","utf-8",      "cp1252",
                                     "iso_2022-jp" };

#ifdef    _DEBUG
#  include "popper.h"
extern POP *global_debug_p;
#  define DBG_PRM    global_debug_p, POP_DEBUG, HERE
#  define DEBUGIT(X) pop_log X /* do { pop_log X; } while FALSE */
#else
  #define DBG_PRM ""
  #define DEBUGIT(X)
#endif /* _DEBUG */


/* ----------------------------------------------------------------------
   822 header handler call back

   Args: state  - pointer to our state/context
         header - buffer containing header
         len    - len of header (it is not necessarily NULL terminated)
         isContintuation - indicates buffer is continuation of last buffer

   Returns: nothing

   All we do here is output the headers we want to show to the user. Also
   output the blank line.

   Note that at this time the Mime parser decodes 2047 formatting of headers
   for us into ISO-8859-1 text. 

   BUG: we don't know when we're called for nested message/ entities
   and thus can't omit whole message/ entities! This is probably a bug
   in the core MIME parser and needs to be rethought.
   ---- */
void headerHandler ( void *state, char *header, long len, unsigned char isContinuation )
{
  char       *h;
  ManglerStatePtr mS = (ManglerStatePtr)state;
  static unsigned char cEntry = 0;   /* The first one doesn't undergo transformation
                                     * but the rest would go because they are part of 
                                     * message body.*/
  size_t i;

  /* Header separator is not supposed to have anything but a line end, but..*/
  for(h = header; h < header + len; h++) {
    if(*h != ' ' || *h != '\t')
      break;
  }
  if( h - header + len >= EOLLEN && EOLCHECK(h) && !isContinuation ) {
      if(!(mS->rqInfo.m_bNoContent)) {
          mS->outFn(mS->outFnState, "Mime-version: 1.0\n",18);
          switch(mS->rqInfo.m_text) {
          case TextHTML:
              mS->outFn(mS->outFnState, "Content-Type: text/html;",24);
              break;
          case TextPlain:
              mS->outFn(mS->outFnState, "Content-Type: text/plain;",25);
              break;
          case TextEnriched:
              mS->outFn(mS->outFnState, "Content-Type: text/enriched;",28);
              break;
          default:
              break;
          }
          mS->outFn(mS->outFnState, "charset=\"", 9);
          mS->outFn(mS->outFnState, charsetAttributes[mS->rqInfo.m_char_set], 
                        strlen(charsetAttributes[mS->rqInfo.m_char_set]));
          mS->outFn(mS->outFnState, "\"\n",2);
          mS->outFn(mS->outFnState, header, len);
      }
      cEntry++;
  } else if( isContinuation ) {
    /* --- It's a continuation; output based on disposition of start of it */
    if(mS->lastWasGoodHeader)
      mS->outFn(mS->outFnState, header, len);
  } else {
    /* --- Start of header, see if it is the one we want --- */
    mS->lastWasGoodHeader = 1; 
    for(i = 0; i < mS->rqInfo.m_headers->size; i++) {
        if(StrBeginsI(mS->rqInfo.m_headers->elem[i], header)) {
            mS->outFn(mS->outFnState, header, len);
            return;
        }
    }
    /* remember we don't want any part of this header in case we get 
       continuation lines
     */
    mS->lastWasGoodHeader = 0; 
  }
}



/* ----------------------------------------------------------------------
   Main type handling call back for the MIME parser

   Args: ourState    - pointer to our state/context set up in MimeInit()
         mError      - mime parsing error code from Mime Parser
         mType       - structure describing the MIME type
         multAttribs - structure describing multipart nesting for current part
         outFn       - place to return the output function to call
         outFnState  - place to put context pointer for output function

   Returns: nothing

   We have to keep track of a little state here and do so in the below
   structure. We track which upcoming MIME entities we should omit, 
   usually for multipart/alternative.
   ---- */
static char *MIMEMajor[] =
{"", "unknown",  "audio", "image",  "video", "model", "multipart",
 "application", "message", "text"};

static char  *appIgnore[] = { "vnd.ms-tnef", NULL};

void MangleMapper( ourState, mError, mType, multAttribs, outFnConstructor, outFn,
                   outFnDestructor, outFnState, headerFn, headerFnState )
  void           *ourState;
  MimeErrorKind   mError;
  MimeTypePtr     mType;
  MultAttribsPtr  multAttribs;
  OutputFn      **outFnConstructor;
  OutputFn      **outFn;
  OutputFn      **outFnDestructor;
  void          **outFnState;
  Rfc822Fn      **headerFn;
  void          **headerFnState;
{
    char          **m           = NULL; 
    char           *tbuf        = NULL;
    char           *typeInfo    = NULL;
    char           *typeMatch   = NULL;
    char           *types       = NULL;
    TextCharSetType charset     = (TextCharSetType) 0;
    int             commaCount;
    char            tempBuf[100];
    ManglerStatePtr mS = (ManglerStatePtr)ourState;

    /* --- Assume this header handler unless something below changes it --- */
    *headerFn      = (Rfc822Fn *)headerHandler;
    *headerFnState = ourState;

    /* --- All that was wanted was the header handler --- */
    if ( mType == NULL ) 
        return;

    *outFn   = NULL;    /* Omit part unless we explicitly configure handler */
    typeInfo = NULL;

    /* --- Start building up a text error message that we might need --- */
    tbuf     = tempBuf;   
    StrCopy(tbuf, lineEnd);
    StrCat(tbuf, "[");

    /* --- Text messages for MIME parsing errors --- */
    switch(mError) {
        case merrorTooDeep:
            StrCat(tbuf, "message too complex; ");
            mType->contentDisp = dispInline;
            goto dontShow;
            break;

        case merrorNoBoundary:
            StrCat(tbuf, "invalid MIME formatting; ");
            mType->contentDisp = dispInline; /* Make error text better */
            goto dontShow;
            break;

        default:
            break;
    }

    /* --- Figure out if we're skipping this part because 
           it's some particular part of a multipart. If it
           is a multipart that we want to omit, flag 
           all its omit bit components later                    --- */
    if ((0x01 << (multAttribs->attribs[multAttribs->nestLevel].partNum - 1)) &
        mS->omitMap[multAttribs->nestLevel] &&
       mType->majorType != majorTypeMulti) {
        return; /* Omit this body */
    }
    if ( multAttribs->attribs[multAttribs->nestLevel].mult == multAlt ) {
        if ( multAttribs->attribs[multAttribs->nestLevel].LookAHead == 1 ) {
            /* If collecting information... */
            if ( mType->majorType == majorTypeText ) {
                if ( !StrCompare(mType->minorType, "enriched") ) {
                    mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextEnriched;
                } else if ( !StrCompare(mType->minorType, "html") ) {
                    mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextHTML;
                } else if ( !StrCompare(mType->minorType, "plain") ){
                    mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextPlain;
                } else {
                    mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextUnknown;
                }
            } /* majorTypeText */
            else if ( multAttribs->attribs[multAttribs->nestLevel+1].mult == multRelated ) {
                if ( mType->paramIndex[paramType] != kParamIndexUnused ) {
                    typeMatch = NULL;
                    types = mType->param[mType->paramIndex[paramType]].value;
                    /* See if it is a text part control */
                    if ( !StrCompare(types, "text/html") ) 
                        mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextHTML;
                    else if ( !StrCompare(types, "text/plain") )
                        mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextPlain;
                    else if ( !StrCompare(types, "text/enriched") )
                        mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextEnriched;
                    else if ( !StrCompare(types, "text/") )
                        mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextUnknown;
                    else 
                        mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextNot;
                }
                else {
                    /* Netscape mail client doesn't obey rfc2112 with 'type' param. */
                    mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextHTML;
                }
                mS->omitMap[multAttribs->nestLevel+1] = 0xffff; /* Skip in look ahead */
            } else {
                mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextNot;
            }
            mS->partCount = multAttribs->attribs[multAttribs->nestLevel].partNum;
            return;
        }
        else if ( multAttribs->attribs[multAttribs->nestLevel].LookAHead == -1 ) {
            int i;
            /* Decide what is to be sent and set omitMap. */
            for ( i = 0; i < mS->partCount; i++ ) {
                if ( mS->parts[i] == mS->rqInfo.m_text ) 
                    break;
            }
            if ( i == mS->partCount ) /* Pass the first part and omit rest */
                i = 0;
            multAttribs->attribs[multAttribs->nestLevel].LookAHead = 0;
            mS->omitMap[multAttribs->nestLevel] =~(0x1 << i);
            if ( i ) 
                return;
        }
    } /* multipart/alternate */

    /* 
     * --- Main switch on the major MIME type --- 
     */
    switch ( mType->majorType ) {
        case majorTypeMulti:
            if ( (0x01 << (multAttribs->attribs[multAttribs->nestLevel].partNum - 1)) &
                  mS->omitMap[multAttribs->nestLevel] ) {
                mS->omitMap[multAttribs->nestLevel+1] = 0xffff;
                return;
            }

            if ( multAttribs->attribs[multAttribs->nestLevel    ].mult == multAlt && 
                 multAttribs->attribs[multAttribs->nestLevel + 1].mult != multRelated ) {
                /* Lets omit any multi's inside a multAlt except Related */
                mS->omitMap[multAttribs->nestLevel + 1] = 0xffff;
                mS->parts[multAttribs->attribs[multAttribs->nestLevel].partNum - 1] = TextUnknown;
                mS->partCount = multAttribs->attribs[multAttribs->nestLevel].partNum;
                return;
            }

            switch ( multAttribs->attribs[multAttribs->nestLevel+1].mult ) {
                case multAlt:
                    typeMatch = NULL;
                    /* The types parameters is an experiment proposed by LGL
                       for multipart/alternative entities to list their 
                       components up front.
                     */
                    if ( mType->paramIndex[paramTypes] != kParamIndexUnused ) {
                        types = mType->param[mType->paramIndex[paramTypes]].value;
                        switch ( mS->rqInfo.m_text ) {
                            case TextPlain:
                                if ( ( typeMatch = StrStr(types, "text/plain") ) == NULL ) 
                                    if ( ( typeMatch = StrStr(types, "text/html") ) == NULL ) 
                                        typeMatch = StrStr(types, "text/enriched");
                                break;
                            case TextHTML:
                                if ( ( typeMatch = StrStr(types, "text/html") ) == NULL ) 
                                    if ( (typeMatch = StrStr(types, "text/plain") ) == NULL ) 
                                        typeMatch = StrStr(types, "text/enriched");
                                break;
                            default:
                                if ( ( typeMatch = StrStr(types, "text/html") ) == NULL )
                                    typeMatch = StrStr(types, "text/enriched");
                        } /* switch mS->rqInfo.m_text */
                    }

                    if ( typeMatch == NULL ) {
                        /* Pass through all for LookAHead*/
                        mS->omitMap[multAttribs->nestLevel+1] = 0x0000;
                        multAttribs->attribs[multAttribs->nestLevel+1].LookAHead = 1;
                    } else {
                        commaCount=0;
                        while ( *types && types != typeMatch ) {
                            if ( *types++ == ',' ) {
                                commaCount++;
                            }
                        }
                        mS->omitMap[multAttribs->nestLevel+1] =~(0x1 << commaCount);
                    }
                    break;
                case multRelated:
                    mS->omitMap[multAttribs->nestLevel+1] = 0x0000;
                    break;
                default:
                    break;
            } /* switch multAttribs->attribs[multAttribs->nestLevel+1].mult */
            return;
            /* Never output anything for multiparts, unless we want to 
               ignore the whole thing which would require some more flags
               and detailed checking here  */
            break;

        case majorTypeNone:
        case majorTypeText:
            if ( mType->contentDisp == dispAttach )
                goto dontShow;
            if ( mType->paramIndex[paramCharset] != kParamIndexUnused ) {
                int i, n = sizeof(charsetAttributes)/sizeof(charsetAttributes[0]);
                for ( i = 0; i < n; i++ ) {
                    if ( strcmp ( charsetAttributes[i], 
                                  mType->param[mType->paramIndex[paramCharset]].value ) == 0 ) {
                    charset = (TextCharSetType) i;
                    break;
                    }
                    if ( i >= n ) {
                        StrCat ( tbuf, "Unhandled charset; may display incorrectly ]" );
                        goto splat;
                    }
                } /* for loop */
            }

            /* for text body parts, we map based on the requested text type */
            switch ( mS->rqInfo.m_text ) {
                case TextEnriched:
                    /*
                     * We don't handle conversion to text/enriched, so
                     * if that's what you ask for, you'll get text/plain.
                     */
                default:
                    /*
                     * If you ask for something weird, you'll get text/plain.
                     */
                case TextPlain:
                    *outFnConstructor = *outFnDestructor = NULL;
                    if ( !StrCompare(mType->minorType, "enriched") ) {
                        *outFn = textEnrichedToPlain;
                        *outFnState = textEnrichedInit ( mS->outFn, mS->outFnState, 
                                                         charset, 
                                                         mS->rqInfo.m_char_set );
                    } else if ( !StrCompare(mType->minorType, "html") ) {
                        *outFn = textHTMLToPlain;
                        *outFnState = textHTMLInit ( mS->outFn, mS->outFnState, 
                                                     charset,
                                                     mS->rqInfo.m_char_set );
                    } else {
                        if ( charset != mS->rqInfo.m_char_set ) {
                            *outFn = CharMangler;
                            *outFnState = CharManglerInit ( mS->outFn, mS->outFnState, 
                                                            charset,
                                                            mS->rqInfo.m_char_set );
                        }
                        else {
                            *outFn = mS->outFn;
                            *outFnState = mS->outFnState;
                        }
                    }
                    break;
                case TextHTML:
                    if ( !StrCompare(mType->minorType, "enriched") ) {
                        *outFnConstructor = textEnrichedToHTMLHead;
                        *outFn = textEnrichedToHTML;
                        *outFnDestructor = textEnrichedToHTMLTail;
                        *outFnState = textEnrichedInit(mS->outFn, mS->outFnState, 
                                                       iso_8859_1, iso_8859_1);
                    }
                    else {
                        if ( !StrCompare(mType->minorType, "plain") 
                             || mType->minorType[0] == '\0' ) {
                            if ( mType->paramIndex[paramFormat] != kParamIndexUnused &&
                                 StrCompare(mType->param[mType->paramIndex[paramFormat]].value, "flowed") == 0 ) {
                                *outFnState       = textFlowedToHTMLInit ( mS->outFn, mS->outFnState );
                                *outFnConstructor = textFlowedToHTMLHead;
                                *outFnDestructor  = textFlowedToHTMLTail;
                                *outFn            = textFlowedToHTML;
                            } else {
                                *outFnState       = textHTMLInit ( mS->outFn, mS->outFnState,
                                                                   iso_8859_1, iso_8859_1 );
                                *outFnConstructor = textPlainToHTMLHead;
                                *outFnDestructor  = textPlainToHTMLTail;
                                *outFn            = textPlainToHTML;
                              }
                        }
                        else {
                            *outFnConstructor  = *outFnDestructor = NULL;
                            *outFnState        = mS->outFnState;
                            *outFn             = mS->outFn;
                        }
                    }
                    break;
            } /* switch mS->rqInfo.m_text */
            return;
            break;

    case majorTypeMsg:
        if ( !StrCompare(mType->minorType, "rfc822") ||
             !StrCompare(mType->minorType, "news") ) {
            if ( multAttribs->attribs[multAttribs->nestLevel].mult == multReport ) {
                /* --- Omit message body in a multipart/report --- */
                /* flip the right bit in the omit map */
                mS->omitMap[multAttribs->nestLevel] |=
                  0x01 << (multAttribs->attribs[multAttribs->nestLevel].partNum-1);
                StrCat(tbuf, "Header of message follows:]");
            } else {
                StrCat(tbuf, "Included message follows:]");
            }
            goto splat;
        } else if ( !StrCompare(mType->minorType, "delivery-status") ) {
            /* --- ignore deliver-status, because some text comes with it --- */
            *headerFn = NULL; /* Ignore 822 headers */
            return;
        } else if ( !StrCompare(mType->minorType, "external-body") ) {
            char *acTp = NULL;

            /* For now, just ignore external body parts.  They are only
               generated by the IETF mailing list, as far as I know. */
            *headerFn = NULL;
            *outFn = NULL;
            return;

            /* The following code attempts to convert external body parts
                into URLs, but needs some work.  The 'return' above keeps
                us from hitting it, for now. */
            
            /* --- convert external-body info to URLs --- */
            if ( mType->paramIndex[paramExtType] != kParamIndexUnused ) {
                acTp = mType->param[mType->paramIndex[paramExtType]].value;
                if ( StrCompare(acTp, "ftp")         == 0 ||
                     StrCompare(acTp, "anon-ftp")    == 0    ) {
                    if ( mType->paramIndex[paramExtSite] != kParamIndexUnused  &&
                         mType->paramIndex[paramExtName] != kParamIndexUnused ) {
                        if ( mS->rqInfo.m_text == TextHTML )
                           StrCat ( tbuf, "<p>" );
                        StrCat ( tbuf, "Click the link to fetch this " );
                        if ( mType->paramIndex[paramExtSize] != kParamIndexUnused ) {
                            StrCat ( tbuf, mType->param[mType->paramIndex[paramExtType]].value );
                            StrCat ( tbuf, " octet" );
                        }
                        StrCat ( tbuf, "body part: <" );
                        if ( mS->rqInfo.m_text == TextHTML )
                            StrCat ( tbuf, "a href=\"" );
                        StrCat ( tbuf, "ftp://" );
                        StrCat ( tbuf, mType->param[mType->paramIndex[paramExtSite]].value );
                        StrCat ( tbuf, "/" );
                        if ( mType->paramIndex[paramExtDir] != kParamIndexUnused ) {
                            StrCat ( tbuf, mType->param[mType->paramIndex[paramExtDir]].value );
                            StrCat ( tbuf, "/" );
                        }
                        StrCat ( tbuf, mType->param[mType->paramIndex[paramExtName]].value );
                        if ( mS->rqInfo.m_text == TextHTML ) {
                            StrCat ( tbuf, "\"> " );
                            StrCat ( tbuf, mType->param[mType->paramIndex[paramExtName]].value );
                            StrCat ( tbuf, " by FTP</a></p>" );
                        }
                        else
                            StrCat ( tbuf, ">]" );
                        goto splat;
                    } /* required params present */
                    else {
                        StrCat ( tbuf, "external body omitted]" );
                    }
                } /* known access-type */
                else {
                    StrCat ( tbuf, "external " );
                    StrCat ( tbuf, acTp );
                    StrCat ( tbuf, " body omitted]" );
                }
            goto splat;
            } /* we have an access-type */
            else {
                StrCat ( tbuf, "external body omitted]" );
                goto splat;
            }
        } else {
            /* --- pass it through --- */
            *outFn      = mS->outFn;
            *outFnState = mS->outFnState;
        }
        /* Bug, need to treat this like app/octet-stream according to 2045 */
        return;
        break;

    case majorTypeApp:
        for ( m = appIgnore; *m; m++ ) {
            if ( !StrCompare(mType->minorType, *m) ) {
                *outFn = NULL;
                return;
            }
        } /* for loop */
        if ( !StrCompare(mType->minorType, "applefile") &&
             multAttribs->attribs[multAttribs->nestLevel].mult == multAppleII ) {
            return;
        }
        if ( !StrCompare(mType->minorType, "octet-stream") ) {
            goto dontShow;
        } else {
            typeInfo = mType->minorType;
            goto dontShow;
        }
        typeInfo = "unknown";
        break;

    case majorTypeImage:
    case majorTypeAudio: 
    case majorTypeVideo:
    case majorTypeModel:
    case majorTypeUnknown:
        typeInfo = MIMEMajor[mType->majorType];
        break;
    }


dontShow:
    /* --- We have decied not to show this part, generate text message --- */
    *outFn = NULL;
    /* --- Include suggested filename in message --- */
    if ( mType->paramIndex[paramFilename] != kParamIndexUnused ) {
        StrNCat0 ( tbuf,
                   mType->param[mType->paramIndex[paramFilename]].value,
                   30 );
        StrCat ( tbuf, "]");
        StrCat ( tbuf, lineEnd);
        StrCat ( tbuf, "[ ");
    }
    /* --- Include some text about the type in the message --- */
    if ( typeInfo ) {
      StrNCat0 ( tbuf, typeInfo, 20 );
      StrCat   ( tbuf, " " );
    }

    /* --- say whether it's an attachment or a part of the message --- */
    StrCat ( tbuf, 
             mType->contentDisp == dispInline ?
             "part omitted]" : "attachment omitted]" );

splat:
    /* --- Output the message we've constructed and finish --- */
    StrCat ( tbuf, lineEnd );
    StrCat ( tbuf, lineEnd );
    {
        OutputFn      *out_FnCtor, *out_Fn, *out_FnDtor;
        void          *out_state;
        switch ( mS->rqInfo.m_text ) {
            case TextHTML:
                out_state  = textHTMLInit ( mS->outFn, mS->outFnState, 
                                            iso_8859_1, iso_8859_1 );
                out_FnCtor = textPlainToHTMLHead;
                out_FnDtor = textPlainToHTMLTail;
                out_Fn     = textPlainToHTML;
                break;
            default:
                out_state  = mS->outFnState;
                out_Fn     = mS->outFn;
                out_FnCtor = out_FnDtor = NULL;
        }
        if ( out_FnCtor ) 
            (*out_FnCtor)(out_state, 0, 0);
        (*out_Fn)(out_state, tbuf, StrLen(tbuf));
        if ( out_FnDtor ) 
            (*out_FnDtor)(out_state, 0, 0);
    }
}

/*
 *  FreeMangleInfo : Because the header vector is on heap, this call is
 *      necessary for every FillMangleInfo.
 */
void FreeMangleInfo ( MangleInfoType *infoStruct )
{
    size_t i;
    if(!infoStruct->m_headers) return;
    for(i = 0; i < infoStruct->m_headers->size; i++)
        free(infoStruct->m_headers->elem[i]);
    free(infoStruct->m_headers);
    return;
}
/*
 * FillMangleInfo : Parse the commandLine and fill the MangleInfoType
 *         structure. Every field in MangleInfoType gets a default value.
 * Parameters : 
 *        commandLine : the mangle command "mangle(attr=value;...)"
 *        bNoContent  : if true we don't want any message content
 *                      there fore don't output MIME headers
 *        infoStruct  : Parsed result is placed.
 *
 * Returns :
 *      0 on success; -1 on Parse error.
 */
int FillMangleInfo ( char *commandLine, char bNoContent, 
                     MangleInfoType *infoStruct )
{
    char *vHeaders[] = {"to:", "cc:", "subject:", "from:", "date:", 
                       "reply-to:", "x-Priority:", "x-UIDL:"};

    int nLen;
    char *p;
    size_t i;

    /* Fill the macros from MDEF Array */

    infoStruct->m_bNoContent = bNoContent;
    

    nLen = strlen(commandLine);

    DEBUGIT (( DBG_PRM, "...FillMangleInfo; commandline(%d)=%.50s",
                 nLen, commandLine ));

    /* Initially filled with default values */
    infoStruct->m_text = TextPlain;
    infoStruct->m_char_set = us_ascii;
    if(strstr(commandLine,"headers=") == NULL) {
        infoStruct->m_headers 
            = (StringVectorType *)malloc(sizeof(StringVectorType));
        if(infoStruct->m_headers) {
            infoStruct->m_headers->size 
                = sizeof(vHeaders)/sizeof(vHeaders[0]);
            infoStruct->m_headers->elem 
                = (char **)malloc(infoStruct->m_headers->size * sizeof(char *));
            if(infoStruct->m_headers->elem) {
                for(i=0; i < infoStruct->m_headers->size; i++){
                    infoStruct->m_headers->elem[i] = strdup(vHeaders[i]);
                }
            }
        }
    }
    commandLine += 6;

    if((p = strstr(commandLine,"text=")) != NULL){
        char *q;
        int i, n = sizeof(textAttributes)/sizeof(textAttributes[0]);
        p += 5;
        if((q=strchr(p,';')) != NULL || (q=strchr(p,')')) != NULL) {
            DEBUGIT (( DBG_PRM, "...parsing text attribute (%d) \"%.*s\"",
                       q-p, q-p, p ));
            for(i=0;i<n;i++){
                if(strncmp(textAttributes[i],p,q-p) == 0){
                    infoStruct->m_text = (TextAttrType) i;
                    DEBUGIT (( DBG_PRM, "...matched %s",
                                textAttributes[i] ));
                    break;
                }
            }
            if (i == n) {
                return -1;
                DEBUGIT (( DBG_PRM, "...returning -1 (error)" ));
            }
        }
    }
    if((p = strstr(commandLine,"charset=")) != NULL) {
        size_t n = sizeof(charsetAttributes)/sizeof(charsetAttributes[0]);
        char *q;
        p += 8;
        if((q=strchr(p,';')) != NULL || (q=strchr(p,')')) != NULL) {
            DEBUGIT (( DBG_PRM, "...parsing charset attribute (%d) \"%.*s\"",
                        q-p, q-p, p ));
            for(i=0;i<n;i++) {
                if(strncmp(charsetAttributes[i],p,q-p) == 0) {
                    infoStruct->m_char_set = (TextCharSetType) i;
                    DEBUGIT (( DBG_PRM, "...matched %s",
                                 charsetAttributes[i] ));
                    break;
                }
            }
        }
    }
    if((p = strstr(commandLine,"lang=")) != NULL){
        DEBUGIT (( DBG_PRM, "...ignoring lang= spec" ));
    }
    if((p = strstr(commandLine,"wrap=")) != NULL){
        DEBUGIT (( DBG_PRM, "...ignoring wrap= spec" ));
    }
    if((p = strstr(commandLine,"headers=")) != NULL){
        DEBUGIT (( DBG_PRM, "...parsing headers spec" ));
        infoStruct->m_headers
            = (StringVectorType *)malloc(sizeof(StringVectorType));
        if(infoStruct->m_headers) {
            char *q, *StrNDup(char *, const int);
            int i;
            p += 8;
            for(i=1,q = p; (*q && *q != ';' && *q != ')'); q++)
                if(*q == ',' && *(q+1) != '\0') i++;
            infoStruct->m_headers->size = i;
            infoStruct->m_headers->elem = (char **)malloc(i * sizeof(char *));
            if(infoStruct->m_headers->elem) {
                for(i = 0; (q = strchr(p,',')) != NULL 
                        || (q = strchr(p,';')) != NULL
                        || (q = strchr(p,')')) != NULL ; i++){
                    infoStruct->m_headers->elem[i] 
                            = StrNDup(p,q-p);
                    p = q+1;
                    if(*q == ';' || *q == ')') break;
                }
            }
        }
    }
    return 0;
}



