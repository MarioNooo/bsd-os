/* ----------------------------------------------------------------------
     MIME Parser -- single pass MIME parser

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
     The license.txt file specifies the terms for use, modification, and 
     redistribution.

     File: mime.c -- the core MIME parser
     Version: 0.2.7   April, 2000
     Last Edited: 26 April 2000

     External dependencies for porting:
       MemSet     - standard way to set memory to a particular value
       malloc/free - way to allocate memory
                    (used only in the initialization)
       StrBeginsI - case independent string matcher - in utils.h
       lineEnd    - in lineend.c - what is our new line convention
       strcmp     - standard old string compare
       strcpy     - standard old string copy
       strlen     - standard old string length

     This runs in a fixed amount of memory, no matter how complex the
     input. There is no limitation on the size of the MIME content, or the
     data itself. It can be binary (NULLs and no new lines), very long lines,
     or very large. The stored state for the parse is less than 1Kb, and a
     hundred bytes of stack should suffice.

     Note, it is not possible to handle every MIME construct in fixed
     memory without many more callbacks, because MIME parameters can be
     aribitrarily large. We just truncate them here. It is not expected
     to cause serious problems. (On an OS with VM just up the token buffer
     to 1Kb or so and call it good enough).

     Some of this code is not re-entrant, though it is nearly so.  Just
     need to add some dynamic allocation and pass a few more contexts 
     around.

To do:
 - uuencode decoding (yuck)
 - Possibly some things to be more thorough/general (at the cost of size)
    - actually check mime-version header
    - other MIME headers (content-description, content-id)
    - more parameters or generalize parameters 
    - dynamic sized parameters?
    - more character set stuff for 822 headers (output in unicode?)
    - the MIME parameter character set mechanism
    - the MIME parameter encoding mechanism
    - the MIME parameter continuation mechanism
 
  ---- */
     

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define StrCompare strcmp
#define StrCopy    strcpy
#define StrLen     strlen

#include "popper.h"
#include "mime.h"
#include "lineend.h"
#include "utils.h"

#include "misc.h"


/* ======================================================================
    P R I V A T E   F U N C T I O N S
   ======================================================================*/


/* ======================================================================
   Content Transfer Encoding stuff
   ==== */

/* ----------------------------------------------------------------------
   Initialization for pass through CTE processor
 
   Args: oFn      - the output function to call
         oFnState - state/context passed to output function

   Returns: the state/context to be passed to us
   --- */
static void 
MimeCteQPInit ( OutputFn oFn, OutputFn oFnCtor, OutputFn oFnDtor, void *oFnState, 
                MimeCtePtr state )
{
  state->outFn              = oFn;
  state->outFnCtor          = oFnCtor;
  state->outFnDtor          = oFnDtor;
  state->outFnState         = oFnState;
  state->x.QP.is2047        = 0;
  state->x.QP.lastWasEqual  = 0;
}

static void 
MimeCte2047Init ( MimeCtePtr state )
{
  state->outFn              = NULL;
  state->x.QP.is2047        = 1;
  state->x.QP.lastWasEqual  = 0;
}

static void MimeCteQPCtor ( MimeCtePtr cS, char *buffer, long bufferLen )  
{
    (void) buffer;
    (void) bufferLen;

    if ( cS->outFnCtor )
        (*cS->outFnCtor) ( cS->outFnState, NULL, 0 );
}
static void MimeCteQPDtor ( cS, buffer, bufferLen )  
MimeCtePtr cS;
char *buffer;
long  bufferLen;
{
    (void) buffer;
    (void) bufferLen;

    if ( cS->outFnDtor )
        (*cS->outFnDtor) ( cS->outFnState, NULL, 0 );
}


/* ----------------------------------------------------------------------
   Process line of QP in place
 
   arg: cS        - CTE state pointer
        buffer    - input buffer. Note we modify this buffer!
        bufferLen - length of input buffer. (Ignored)

   We can always decode CTE in place because the result is always smaller.

   Bug: this does not lop off trailing white space that should be lopped off.
   This is fortunately not a serious bug...

   Note: this code ignores the bufferLen, and expects input to be
         NULL terminated (which is supplied by input buffering).
   ---- */
static void MimeCteQPProcess ( cS, buffer, bufferLen )  
MimeCtePtr cS;
char *buffer;
long  bufferLen;
{
    char *i, *o;
    (void) buffer;
    (void) bufferLen;


  if ( buffer == NULL ) {
    /* Called with NULL flush and close; We have no buffering. */
    if ( cS->outFn )
      (cS->outFn) ( cS->outFnState, NULL, 0 );
    return;
  }


  for (i = buffer, o = buffer;  *i; ) {
    if ( *i == '=' || cS->x.QP.lastWasEqual ) {
      /* --- It's an escaped sequence we have to treat special --- */
      cS->x.QP.lastWasEqual = 0;
      if(*i == '=')
        i++;
      if(EOLCHECK(i)) {
        /* --- Bare = at end of line; it's continuation --- */
        i += EOLLEN;
      } else if (*i == '\0') {
        cS->x.QP.lastWasEqual = 1;
        goto Done;
      } else {
        /* Funky code to convert two hex digits in minimal code space */
        int c = *i - '0';
        if(c < 0)
          goto bogus;
        if(c < 10) {
          *o = c;
        } else {
          c -= ('A' - '0');
          if(c < 6 && c >= 0)
            *o = c + 10;
          else
            goto bogus;
        }
        *o <<= 4;
        c = *(i+1) - '0';
        if(c < 0)
          goto bogus;
        if(c < 10) {
          *o += (char)c;
        } else {
          c -= ('A' - '0');
          if(c < 6 && c >= 0)
            *o += c + 10;
          else
            goto bogus;
        }
        o++;
      bogus:
        i += 2;
      }
    } else if(cS->x.QP.is2047 && *i == '_') {
      *o++ = ' ';
      i++;
    } else if(EOLCHECK(i)) {
      /* --- at end of line --- */
      ENDLINE(o);
      i += EOLLEN;
    } else if(*i == '\0') {
      cS->x.QP.lastWasEqual = 0;
      goto Done;
    } else {
      /* --- Just a normal char to pass through --- */
      *o++ = *i++;
    }
  }
 Done:
  *o = '\0';
  if(cS->outFn && (o - buffer))
    (cS->outFn)(cS->outFnState, buffer, o - buffer);
}


/* ----------------------------------------------------------------------
    Process 2047 syntax (8bit headers) in place on a buffer

   arg:  buffer - the buffer which might contain RFC 2047 syntax

   Bug, need to handle some character set stuff...
   --- */
static void  rfc2047( buffer, cS )
char *buffer; 
MimeCteType *cS;
{
  char *i, *o, *t;
  unsigned char y;

  for(i = buffer, o = buffer; *i; ) {
    if(*i == '=' && *(i+1) == '?') {
      /* In funky text */
      i+=2;
      while(*i && *i++ != '?')
        ;
      /* now past char set */
      y = 0;
      if(*i != 'Q') {
        /* trouble... we don't do b64, just blat it out */
        y = 1;
      }
      i+=2; /* Assume the proper '?' */
      for(t = i; *t && !(*t == '?' && *(t+1) == '='); t++)
        ;
      *t = '\0';
      StrCopy(o, i);
      if(!y) {
        MimeCteQPProcess(cS, o, StrLen(o));
      }
      o += StrLen(o);
      i = t+2;
    } else {
      *o++ = *i++;
    }
  }
  *o = '\0';
}


/* ----------------------------------------------------------------------
   Initialization for pass through CTE processor
 
   Args: oFn      - the output function to call
         oFnState - state/context passed to output function

   Returns: the state/context to be passed to us
   --- */
static void MimeCteB64Init( OutputFn *oFn, OutputFn *oFnCtor, OutputFn *oFnDtor, void *oFnState, 
               MimeCtePtr state )
{
  state->outFn            = oFn;
  state->outFnCtor        = oFnCtor;
  state->outFnDtor        = oFnDtor;
  state->outFnState       = oFnState;
  state->x.B64.threeBytes = 0L;
  state->x.B64.shift      = 18;
  state->x.B64.pad        = 0;
}

static void MimeCteB64Ctor( cS, buffer, bufferLen )
MimeCtePtr cS;
char      *buffer;
long       bufferLen;
{
    if(cS->outFnCtor)
    (*cS->outFnCtor)(cS->outFnState, buffer, bufferLen);
}

static void MimeCteB64Dtor( cS, buffer, bufferLen )
MimeCtePtr cS;
char      *buffer;
long       bufferLen;
{
    if(cS->outFnDtor)
    (*cS->outFnDtor)(cS->outFnState, buffer, bufferLen);
}


static unsigned char Base64Map[] = {
 62, 65, 65, 65, 63,              /* 0x2B - 0x2F  ('+' - '/') */
 52, 53, 54, 55, 56, 57, 58, 59,  /* 0x30 - 0x37  ('0' - '7') */
 60, 61, 65, 65, 65,  0, 65, 65,  /* 0x38 - 0x3F  ('8' - '?') */
 65,  0,  1,  2,  3,  4,  5,  6,  /* 0x40 - 0x47  ('@' - 'G') */    
  7,  8,  9, 10, 11, 12, 13, 14,  /* 0x48 - 0x4F  ('H' - 'O') */    
 15, 16, 17, 18, 19, 20, 21, 22,  /* 0x50 - 0x57  ('P' - 'W') */    
 23, 24, 25, 65, 65, 65, 65, 65,  /* 0x58 - 0x5F  ('X' - '_') */    
 65, 26, 27, 28, 29, 30, 31, 32,  /* 0x60 - 0x67  ('`' - 'g') */    
 33, 34, 35, 36, 37, 38, 39, 40,  /* 0x68 - 0x6F  ('h' - 'o') */    
 41, 42, 43, 44, 45, 46, 47, 48,  /* 0x70 - 0x77  ('p' - 'w') */    
 49, 50, 51};                     /* 0x78 - 0x7F  ('x' - 'z') */    
 




/* ----------------------------------------------------------------------
   Process line of base 64 in place
 
   arg: cS        - Pointer to CTE processor state
        buffer    - input buffer. Note we modify this buffer!
        bufferLen - length of input buffer. (Ignored)

   We can always decode CTE in place because the result is always smaller.

   Note: this code ignores the bufferLen, and expects input to be
         NULL terminated (which is supplied by input buffering).
   ---- */
static void MimeCteB64Process( cS, buffer, bufferLen )
MimeCtePtr cS;
char      *buffer;
long       bufferLen;
{
  char         *i, *o;
  unsigned long c;

  if( buffer == NULL ) {
    /* Called with NULL flush and close; We have no buffering.. */
    if(cS->outFn)
      (cS->outFn)(cS->outFnState, buffer, bufferLen);
    return;
  }

  for(i = buffer, o = buffer; *i; i++) {
    if(*i == ' ' || *i == '\t' || *i =='\012' || *i == '\015') {
        continue;
    }   
    
    if(*i >= '+' && *i <= 'z') {
        c = Base64Map[*i-'+'];
      if(*i == '=')
        cS->x.B64.pad++;
        if(c == 65)
                continue;
    } else {
      continue;
    }           
    cS->x.B64.threeBytes |= (c << cS->x.B64.shift);
    cS->x.B64.shift -= 6;
    if(cS->x.B64.shift < 0) {
        *o++ = (cS->x.B64.threeBytes & 0xff0000L) >> 16;
      if(cS->x.B64.pad <= 1)
          *o++ = (cS->x.B64.threeBytes & 0xff00L) >> 8;
      if(cS->x.B64.pad == 0) 
        *o++ = (cS->x.B64.threeBytes & 0xffL);
        cS->x.B64.threeBytes = 0L;
        cS->x.B64.shift = 18;
      cS->x.B64.pad   = 0;
      }
  }     
  
  if(o - buffer)
    cS->outFn(cS->outFnState, buffer, o - buffer);
}




/* ----------------------------------------------------------------------
   Tokenizer for MIME headers
   
   args: i     - the input string to consume, must be NULL terminated
         state - state of the lexer carried from previous call
         dest  - place to copy acquired token to
         destLen - size of buffer for acquired token
   returns: 
         pointer into i where non-white space occurs or end of string
   
   Caller reaches into state struct and sets case sensitivity for token -
   That is, this will down case tokens that are known to be case 
   insensitive.
   ---- */
static char *kRFC822MustBeQuoted = "()<>@,;:\"/[]?=\\ \n\r";

static char *
MimeRfc822Token( char *i, Rfc822LexType *state, char *dest, short destLen )
{
  char *p, *destEnd;

  if(!state->gettingToken) {
    do {
      if(state->inQuotes) {
        /* Keep looking for the closing quote */
        while (*i && *i != '\"') { /* Go 'til end of quote */
          if (*i == '\\' && *(i+1) != '\0')
            i++; /* Escaped character inside quote */
          i++;
        }
        if(*i) {
          state->inQuotes = 0;
          i++;
        }
  
      } else if(state->parenNesting) {
        while (*i && state->parenNesting) {
          switch (*i) {
            case '(':
              state->parenNesting++;
              break;
            case ')':
              state->parenNesting--;
              break;
            case '\\': /* Escape character */
              if (*(i+1) != '\0') /* Check for end-of-string */
                i++;
              break;
            case '\"': /* Quote inside comment */
              state->inQuotes = 1;
              i++;
              goto quitLoop;
            default:
              break;
          }
          i++;
        }
      quitLoop:
        i = i; /* To make compiler happy about above label */
  
      } else {
        /* Skip spaces - what we do most of the time */
        while (*i==' ' || *i=='\t' || EOLCHECK(i)) 
          i++;
        if(*i == '(') {
          state->parenNesting++;
          i++;
        }
      }
    } while (*i==' ' || *i=='\t' || EOLCHECK(i) || *i=='(' ||
             (state->parenNesting && *i));
    /* Didn't find the start of a token */
    if(!*i) 
      return(i);
    else
      state->gettingToken = 1;
      state->tokenLen = 0;
      /* fall through to next case below */
  }

  if(state->gettingToken) {
    destEnd = dest + destLen - 1;
    dest   += state->tokenLen;
  
    if(*i == '\"') {
      state->inQuotes = 1;
      i++;
    }
  
    while(*i) {
      for(p = kRFC822MustBeQuoted; *p && *p != *i; p++)
        ;
      if(*p) {
        if(!state->inQuotes) {
          /* --- Found end of a token --- */
          if(!state->tokenLen)
            /* Don't need to down case special characters */
            *dest++ = *i++;
          break;
        } else {
          /* --- We're inside a quoted string --- */
          if(*i == '\"') {
            i++;
            break; /* We're done */
          }
          if(*i == '\\')
            if(!*++i)
              break; /* Just copy the next character if not end of string */
        }
      }
      if(dest < destEnd) {
        *dest = *i;
        if(!state->dontDownCase && *dest >= 'A' && *dest <= 'Z') 
          *dest |= 0x20;
        dest++;
        state->tokenLen++;
      }
      i++;
    }
    if(*i) {
      state->gettingToken = 0;
      state->inQuotes     = 0;
      *dest               = '\0';
      return(i);
    } else {
      return(NULL);
    }
  }
  return(NULL); /* Keeps compiler happy - will never happen */
}



/* ----------------------------------------------------------------------
    Beginning of MIME type parser

  args: mType - A mime type structure to be cleared
   ---- */
static void MimeClearType( mType )
MimeTypeType *mType;
{
  MemSet ( (char *) mType,  sizeof(MimeTypeType), 0 );
  MemSet ( (char *) (mType->paramIndex), paramMax, kParamIndexUnused );
}
  

/* ----------------------------------------------------------------------
   This is the parser for content-*: lines. It does content-type,
   content-disposition and content-transfer-encoding.

  args: inBuf - the line to parse, starts after "content-"
                Note that this is NULL terminated
        hState - our mime header parsing state
        mType  - where to put the results
        isContinuation - tells us whether this is continuation of 
                  last line of input or not. Continuation may either
                  due to buffering limitations, or RFC822 continuation
                  lines - we don't care.

  Note, a complete MIME parser would be more general about the parameters
  instead of restricting parsing to a fixed set. It should also probably
  include more of the Content-xxx headers. Basically, this is where
  all the work needs to be done to make this into a more general MIME
  parser - the original goal was one that was tight and small to run
  on the PalmOS. Parameters are really the main place in MIME where
  things are not constrained in size or number.

  Note, we also mix content-disposition parameters with content-type
  parameters here.

  A full parser will also do all the wacky parameter continuation,
  character set and encoding stuff too.
   --- */

static const char *MIMEMajor[] =
{"", "unknown",  "audio", "image",  "video", "model", "multipart",
 "application", "message", "text"};

static const char *CTEs[] = 
{"7bit", "binary", "8bit", "quoted-printable", "base64", NULL};

static const char *Params[] = 
{"", "boundary", "charset", "type", "filename", "types", "format", 
 "access-type", "size", "site", "name", "directory", "mode", NULL};

/* a "1" corresponding to a param type means it is case-sensitive (do not downcase) */
static const char ParamCaseMap[] = {0, 1, 0, 0, 1, 0, 0, 
                                    0, 1, 0, 1, 1, 0 }; 

static void MimeHeaderParse( inBuf, hState, mType, isContinuation )
char            *inBuf;           /* Input line to parse */
HeaderParseType *hState;          /* State of the MIME header parser */
MimeTypeType    *mType;           /* The MIME type accumulated so far */
char             isContinuation;  /*This is a complete, or the completion*/
{
    MajorTypeKind m;
    CteKind       c;
    MimeParamKind paramType;
    char         *tempBuf = hState->tokenBuf;
   
    if ( !isContinuation ) {
        /* Start of a new header, init our lexer, and figure out
           which of the headers we care about that it is */
        MemSet ( (void *)&(hState->lexState), sizeof(Rfc822LexType), 0 );
        if ( StrBeginsI ( "type:", inBuf ) ) {
            hState->parseState = wantMajor;
            inBuf += 5; /* 5 is length of "type:" */
        } else if ( StrBeginsI ( "disposition:", inBuf ) ) {
            inBuf += 12;  /* 12 is length of "disposition:" */
            hState->parseState = wantDisp;
        } else if ( StrBeginsI ( "transfer-encoding:", inBuf ) ) {
            inBuf += 18; /* 18 is length of "transfer-encoding:" */
            hState->parseState = wantCTE;
        }
    }

    while ( *inBuf ) {
        /* First get the token */
        inBuf = MimeRfc822Token ( inBuf,
                                  &(hState->lexState),
                                  tempBuf,
                                  kMIMETokenLen );
        if ( !inBuf || !*inBuf )
          /* Don't mess with token until we got the whole thing */
          return;

        /* --- If here we have the next token to handle --- */
        switch ( hState->parseState ) {
            case wantMajor:
                for ( m = majorTypeText; m != majorTypeUnknown; m-- )
                    if ( !StrCompare(MIMEMajor[m], tempBuf) )
                        break;
                mType->majorType = m;
                hState->parseState = wantSlash;
                break;
            case wantSlash:
                if ( *tempBuf == '/' )
                    hState->parseState = wantMinor;
                break;
            case wantMinor:
                StrCopy ( mType->minorType, tempBuf );
                hState->parseState = wantSemi;
                break;
            case wantDisp:
                if ( !StrCompare(tempBuf, "inline") )
                    mType->contentDisp = dispInline;
                else if ( !StrCompare(tempBuf, "attachment") )
                    mType->contentDisp = dispAttach;
                hState->parseState = wantSemi;
                break;
            case wantSemi:
                if ( *tempBuf == ';' )
                    hState->parseState = wantParamName;
                break;
            case wantParamName:
                for ( paramType = (MimeParamKind) paramMax-1; 
                      paramType != paramNone; 
                      paramType-- )
                    if ( !StrCompare(tempBuf, Params[paramType]) ) {
                        hState->lexState.dontDownCase = ParamCaseMap[paramType];
                        break;
                    }
                hState->paramWanted = paramType; /* will be zero if not in our table */
                hState->parseState  = wantEqual;
                break;
            case wantEqual:
                if ( *tempBuf == '=' )
                    hState->parseState = wantParamValue;
                break;
            case wantParamValue:
                if ( hState->paramWanted != paramNone ) {
                    /* This is a parameter that we want */
                    StrCopy ( mType->param[mType->nextParamPos].value, tempBuf );
                    mType->param[mType->nextParamPos].name = hState->paramWanted;
                    /* Index position for the particular type */
                    mType->paramIndex[hState->paramWanted] = mType->nextParamPos;
                    if ( mType->nextParamPos != kParamMaxNum )
                        mType->nextParamPos++;
                }
                hState->parseState = wantSemi;
                hState->lexState.dontDownCase = 0;
                break;
            case wantCTE:
                for ( c = cte7bit; c != cteBadCTE; c++ )
                    if ( !StrCompare(tempBuf, CTEs[c]) )
                        break;
                mType->cte = c;
                hState->parseState = wantNothing;
                break;
            case wantNothing:
                /* Ignore the rest of the stuff */
                break;
            default:
                break;
        } /* switch hState->parseState */
    }
    return;
}


/* this relates to enums in mime.h */
static char *multTypes[] = {"", "alternative", "appledouble", "report", "related"};

/*----------------------------------------------------------------------
   Mime type basics handling. That is, stuff not specific to any
   application.

   args: s - The whole parser state
   
   Side effects:
      Sets up the cte handler in the parser state. This includes
      the output function the cte handler calls, which is the
      handler the caller has defined for this particular mime type
  --- */
static void MimeMapHandler(s)
MimeParseType *s;
{
    OutputFn     *outFnConstructor  = NULL;
    OutputFn     *outFnDestructor   = NULL;
    OutputFn     *outFn             = NULL;
    void         *outFnState        = NULL;
    unsigned char tmpNestLevel      =    0;
    MimeErrorKind mError;
    MultTypeKind  minor;

    mError                   = merrorNone;
    s->msgPartState          = parseBodyPart;
    s->multAttribs.nestLevel = s->currentBoundary + 1;
  
    if ( s->mType.majorType == majorTypeMulti ) {
        if ( s->currentBoundary == kMaxMIMENesting - 1 ) {
            mError = merrorTooDeep;
        } 
        else { /* multipart, but not too deep */
            if ( s->mType.paramIndex[paramBoundary] != kParamIndexUnused ) {
                s->currentBoundary++;
                StrCopy ( s->mimeBoundaries [ s->currentBoundary ],
                          s->mType.param [ s->mType.paramIndex[paramBoundary] ].value );
            }
            else {
                mError = merrorNoBoundary;
            }


            /* --- Got the boundary, etc: we can handle this multipart --- */
            s->msgPartState = parsePartStart;

            /* Set up the multipart attributes structure */
            tmpNestLevel = s->multAttribs.nestLevel + 1;
            s->multAttribs.attribs[tmpNestLevel].partNum = 0;
            if ( tmpNestLevel == 0 )
                s->multAttribs.attribs[tmpNestLevel-1].partNum = 1;
            for ( minor = multRelated; minor != multNuthinSpecial ; minor-- ) {
                if ( !StrCompare(s->mType.minorType, multTypes[minor]) )
                    break;
            }
            s->multAttribs.attribs[tmpNestLevel].mult = minor;
        } /* multipart, but not too deep */
    } /* multipart */
    else if ( s->mType.majorType == majorTypeMsg ) {
        s->msgPartState = parseHeaders;
    } /* message */

    if ( s->mType.cte == cteBadCTE )
        mError = merrorBadCTE;

    /* Call caller defined mapper function to find out what
       they want done with this MIME part */
    (s->mapperFun)(s->mapperFunState,
           mError,
           &(s->mType),
           &(s->multAttribs),
           &outFnConstructor,
           &outFn,
           &outFnDestructor,
           &outFnState,
           &(s->headerHandler),
           &(s->headerHandlerState));

    if ( s->mType.majorType == majorTypeMulti && 
         s->multAttribs.attribs[s->multAttribs.nestLevel+1].LookAHead == 1 ) {
        /* For MultiAlt */
        /* Get the fpos of mbox, dirty trick */
        s->multAttribs.attribs[s->multAttribs.nestLevel].offset = ftell(s->mdrop);
        s->multAttribs.attribs[s->multAttribs.nestLevel].lines = MSG_LINES;
    }

    if ( outFn == NULL || mError != merrorNone ) {
        s->cteHandlerConstructor = s->cteHandlerDestructor = NULL;
        s->outputTag = outputNone;
    } else {
        switch ( s->mType.cte ) {
            case cteBinary:
            case cte7bit:
            case cte8bit:
                s->outputTag             = outputDirect;
                s->cteHandlerConstructor = outFnConstructor;
                s->cteHandlerDestructor  = outFnDestructor;
                s->output.outHandler     = outFn;
                s->outState.oFnState     = outFnState;
                break;
            case cteQP:
                s->outputTag             = outputCTE;
                s->output.cteHandler     = MimeCteQPProcess;
                s->cteHandlerConstructor = (OutputFn*) MimeCteQPCtor;
                s->cteHandlerDestructor  = (OutputFn*) MimeCteQPDtor;
                MimeCteQPInit(outFn, outFnConstructor, outFnDestructor, outFnState, 
                      &(s->outState.cteState));
                break;
            case cteBase64:
                s->outputTag             = outputCTE;
                s->output.cteHandler     = MimeCteB64Process;
                s->cteHandlerConstructor = (OutputFn*) MimeCteB64Ctor;
                s->cteHandlerDestructor  = (OutputFn*) MimeCteB64Dtor;
                MimeCteB64Init ( outFn, outFnConstructor, outFnDestructor, 
                                 outFnState, 
                                 &(s->outState.cteState) );
                break;
            default:
                s->cteHandlerConstructor = s->cteHandlerDestructor = NULL;
                s->outputTag             = outputCTE;
                break;
        } /* switch s->mType.cte */
    } /* outFn isn't NULL and mError is merrorNone */
}





/*----------------------------------------------------------------------
  Actually do the MIME processing

  Args: State      - Stored state for the parser
        inBuf      - the buffer to parse, it's contents will be changed!
        inBufLen   - The length of the input

 Note that if input is headers (mime or 822), or CTE encoded contents
 this code expects the input to be NULL terminated. For non-encoded
 contents, this code treats the input as binary data. In any case
 all lines that are less than 80 characters must be submitted in 
 a single buffer. This tremendously simplifies MIME boundary checking
 for binary MIME.
  ---- */
static void MimeProcess( State, inBuf, inBufLen )
MimeParsePtr   State;
char          *inBuf;
short          inBufLen;
{
  char       *i, *b, hasEOL;
  int         l = 0;

  i = inBuf;

  if(i == NULL) {
    /* NULL input means flush our buffers */
    if(State->savedNewline) {
      /* flush a new line we might have (only buffering here) */
      char temp[EOLLEN+1];
      StrCopy(temp, lineEnd);
      if ( State->outputTag == outputCTE )
        (*State->output.cteHandler)(&(State->outState.cteState), temp, EOLLEN);
      else if ( State->outputTag != outputNone )
        (*State->output.outHandler)(State->outState.oFnState, temp, EOLLEN);          
    }
    /* Call output function with NULL to let them know to shutdown */
    if(State->outputTag == outputCTE && State->output.cteHandler)
      (*State->output.cteHandler)(&(State->outState.cteState), NULL, 0);
    else if(State->outputTag == outputDirect && State->output.outHandler)
      (*State->output.outHandler)(State->outState.oFnState, NULL, 0);
    if(State->cteHandlerDestructor && State->outputTag != outputNone
       && State->cteHandlerDestructor){
    (*State->cteHandlerDestructor)(State->outputTag == outputCTE ?
                       &(State->outState.cteState) :
                       (State->outState.oFnState), 0, 0);
    State->cteHandlerDestructor = 0;
    }
    State->outputTag = outputNone;
    return;
  }


  /* ---- Always check for all known boundaries --- */
  b = NULL;

  if(State->currentBoundary >= 0 && inBuf[0] == '-' && inBuf[1] == '-') {
    for(l = State->currentBoundary; l >= 0; l--) {
      for(i = inBuf+2, b = State->mimeBoundaries[l];
         *i == *b && *b;
          i++, b++)
        ;
      if(!*b)
        break;
    }
  }
  

  if(b && !*b) {
    /* -----  Found a boundary - change our major parsing state ------- */

    /* --- Inform current content handler that part is over --- */
      if(State->cteHandlerDestructor){
      if(State->outputTag == outputCTE) {
          (*State->output.cteHandler)(&(State->outState.cteState), NULL, 0);
          (*State->cteHandlerDestructor)(&(State->outState.cteState), 0, 0);
      }
      else if(State->outputTag == outputDirect) {
          (*State->output.outHandler)(State->outState.oFnState, NULL, 0);
      /* There is a tail for previous data. */
          (*State->cteHandlerDestructor)( (State->outState.oFnState), 0, 0);
          State->cteHandlerDestructor = 0;
      }
      State->outputTag = outputNone;
      }
    /* --- Newline in this case was part of the boundary ---- */
    State->savedNewline = 0;

    /* --- Nil out MIME type ---*/
    MimeClearType(&(State->mType));

    /* --- Handle bad MIME with no closing boundary ---*/
    State->currentBoundary = l;

    /* --- Count the parts in the current multipart --- */
    State->multAttribs.attribs[l+1].partNum++;

    /* --- We're looking for headers now --- */
    State->msgPartState = parseMimeHeaders;

    /* --- Check for closing boundary --- */
    l = StrLen(State->mimeBoundaries[l]);
    if ( StrLen(inBuf) >= (size_t) 4+l && inBuf[2+l] == '-' && inBuf[3+l] == '-' ) {
      /* See if we are in LookAHead at the current level. */
      if( State->multAttribs.attribs[State->multAttribs.nestLevel].LookAHead == 1 ) {
      /* Try to recreate the state */
      State->msgPartState = parsePartStart;
      fseek(State->mdrop, State->multAttribs.attribs[State->multAttribs.nestLevel - 1].offset, SEEK_SET);
      MSG_LINES = State->multAttribs.attribs[State->multAttribs.nestLevel - 1].lines;
      State->multAttribs.attribs[State->multAttribs.nestLevel].partNum = 0;
      State->multAttribs.attribs[State->multAttribs.nestLevel].LookAHead = -1;
      }
      else {
      /* Hit end of a multipart, ignore until next boundary */
      State->msgPartState          = parsePartStart;
      State->multAttribs.nestLevel = (unsigned char)State->currentBoundary;
      }
    }

  } else if((State->msgPartState == parseHeaders || /* 822 header parse or */
             State->msgPartState == parseMimeHeaders)) { /* MIME header */
    /* ------- We're parsing the message header --------- */
    /* Check for the blank separator line, tolerates space on the line */
    if(EOLCHECK(i+inBufLen-EOLLEN))
      hasEOL = 1;
    else
      hasEOL = 0;
    for(i = inBuf; *i == ' ' || *i == '\t'; i++)
        ;
    if(i == inBuf+inBufLen-EOLLEN && !State->lastHadNoEOL) {
      /* --- A blank line, the end of the headers --- */
      if(State->msgPartState == parseHeaders && State->headerHandler) 
        State->headerHandler(State->headerHandlerState,inBuf,StrLen(inBuf),0);
      /* --- Get the handler for following content set up --- */
      if(!State->isMIME)
        MimeClearType(&(State->mType));
      MimeMapHandler(State);

      if( State->cteHandlerConstructor && State->outputTag != outputNone ) {
      /* Call the header for data */
      (*State->cteHandlerConstructor)(State->outputTag == outputCTE ?
                      &(State->outState.cteState) :
                      (State->outState.oFnState), 0, 0);
      State->cteHandlerConstructor = 0;
      }
      return;
    } 

    /* --- continuation lines --- */
    if(inBuf[0] == ' ' || inBuf[0] == '\t' || State->lastHadNoEOL){
      switch(State->headerDisposition) {
        case mimeHeader:
          MimeHeaderParse(inBuf,
                        &(State->mHeaderState),
                        &(State->mType),
                          1);
          break;
        case rfc822Header:
          if(State->headerHandler) {
            rfc2047(inBuf,&(State->outState.cteState));
            State->headerHandler(State->headerHandlerState,inBuf,StrLen(inBuf),1);
          }
          break;
        default:
          break;
      }
      State->lastHadNoEOL = !hasEOL;
      return;
    }
    State->lastHadNoEOL = !hasEOL;

    /* ---- Parse the start of the content-type line ---- */
    if(StrBeginsI("content-", inBuf)) {
      MimeHeaderParse(inBuf + 8, 
                    &(State->mHeaderState),
                    &(State->mType),
                      0);
      State->headerDisposition = mimeHeader;
      return;
    }

    /* ---- Just want MIME headers ----- */
    if(State->msgPartState == parseMimeHeaders)
      return;

    /* --- Is this a MIME message? --- */
    if(StrBeginsI("mime-version", inBuf)) {
      /* small bug: ought to parse for version number */
      State->isMIME=1;
      return;
    }

    /* --- It's an rfc-822 header --- */
    if(State->headerHandler) {
      MimeCte2047Init(&(State->outState.cteState));
      rfc2047(inBuf, &(State->outState.cteState)); 
      State->headerHandler(State->headerHandlerState, inBuf, StrLen(inBuf), 0);
    }
    State->headerDisposition = rfc822Header;

  } else if(State->msgPartState == parseBodyPart) {
    /* ------- We're parsing the body ------- */
    char savedNewline = State->savedNewline;
    
    State->savedNewline = 0;
    if(inBufLen >= EOLLEN && EOLCHECK(inBuf + inBufLen - EOLLEN)) {
      /* don't pass newlines on until we know the next block of
         input is not a boundary as required by MIME */
      inBufLen -= EOLLEN;
      inBuf[inBufLen] = '\0';
      State->savedNewline = 1;
    }

    /* ---
       Here the CTE handler is called, first for the saved new-line 
       if there is one, and then for the actual content. The CTE 
       handler in turn calls the content handler that was set up
       when the CTE handler was initialized.
       --- */
    if(State->outputTag != outputNone) {
      if(savedNewline) {
        char temp[EOLLEN+1];
        StrCopy(temp, lineEnd);
        if(State->outputTag == outputCTE)
          (*State->output.cteHandler)(&(State->outState.cteState), temp, EOLLEN);
        else
          (*State->output.outHandler)(State->outState.oFnState, temp, EOLLEN);          
      }
      if(inBufLen) {
        if(State->outputTag == outputCTE)
          (*State->output.cteHandler)(&(State->outState.cteState), inBuf, inBufLen);
        else
          (*State->output.outHandler)(State->outState.oFnState, inBuf, inBufLen);
      }
    }

  } else {
    /* ------- Don't output the junk before 1st MIME part ------ */
  }
}
        




/* ======================================================================
    P U B L I C   F U N C T I O N S
   ======================================================================*/


/* ----------------------------------------------------------------------
   Initialize the MIME mangler - call this at start of every message

   Args: mapperFun      - Mime type mapping call back 
         mapperFunState - State/context for above callback
 
   Returns: dynamically allocated state/context for Mime parser
   ---- */ 
MimeParsePtr MimeInit ( mapperFun, mapperFunState, drop )
MimeTypeFn mapperFun;
void *mapperFunState; 
FILE *drop;
{
  MimeParsePtr s;
#ifdef UNIX
  s = (MimeParsePtr)malloc(sizeof(MimeParseType));
#else
  
#endif
  if ( s ) {
    /* Most of our enums and var are set up so making them 
       zero is what we want for initialization */
    MemSet ( (void *)s, sizeof(MimeParseType), 0 );
    MimeClearType ( &(s->mType) );
    s->currentBoundary    = -1;
    s->mapperFun          = mapperFun;
    s->mapperFunState     = mapperFunState;
    s->mdrop              = drop;
  }
  return ( s );
}



/* ----------------------------------------------------------------------
   Done with MIME parser - clean up

   Args: s - State/context
   ---- */ 
void MimeFinish( s )
MimeParsePtr s;
{
  /* --- Flush any input by running a blank string through --- */
  MimeProcess(s, NULL, 0);

#ifdef UNIX
  free((void *)s);
#else
  
#endif
}


/* ----------------------------------------------------------------------
    This is where input is fed into the parser
  
   Args: s        - the context/state for the parser
         inBuf    - the input buffer 
         inBufLen - length of stuff in input buffer
   --- */
void MimeInput( s, inBuf, inBufLen )
MimeParseType      *s;
const char         *inBuf;
const unsigned long inBufLen;
{
  const char *i;
  char       *iEnd;
#if (EOLLEN == 2)
  char        c, tmp;
#endif

  if(!s->gotTopHeaderHandler) {
    /* Call the mapper function to get a header handler function
       for the top level 822 headers */
      (s->mapperFun)(s->mapperFunState,
                 merrorNone,
                 NULL,
                 NULL, 
         NULL,
                 NULL,
                 NULL,
                 NULL,
                 &(s->headerHandler),
                 &(s->headerHandlerState));
      s->gotTopHeaderHandler = 1;
  }




  /* This buffers up chunks and passes them on to next layer.
     Every time a line ends, it is passed to the processor
     as a chunk. Or in other words, if a line end occurs in a
     buffer passed on to the next level of parsing it is always
     at the end of the buffer. 

     This buffering simplifies boundary detection, quoted printable
     and base64 processing substantially. In addition we add a NULL
     terminator to simply later processing of all non-binary material,
     which is most of what will be processed. 

     This code will compile for the end of line convention needed. As
     controlled in lineend.h. In the case of \r\n, we make sure it is
     never split across buffers to keep syntax parsing such as boundary
     detection, and the header/body separator easier to find.

     If some of this code looks kinda like assembly, it's because
     it is kinda like assembly and compiles smaller than the code
     that didn't. 
   */
  i     = inBuf;
  iEnd  = (char *)inBuf + inBufLen;

#if (EOLLEN == 2) /* more or less assume \r\n */
  do {
    c = s->buf[s->bufLen++] = *i++;
    tmp = -1;
    if(c == '\r' && i == iEnd) {
      s->bufLen--;
      tmp = 1;
    } else if(c == '\r' && *i == '\n') {
      s->buf[s->bufLen++] = '\n';
      tmp = 0;
      i++;
    } else if(s->bufLen = kInBufSize - 3) {
      tmp = 0;
    }
    
    if(tmp >= 0) {
      s->buf[s->bufLen] = '\0';
      MimeProcess(s, s->buf, s->bufLen);
      s->bufLen = tmp;
      if(tmp)
        s->buf[0] = '\r';
    }
  } while(i!= iEnd);

#else
  do {
    s->buf[s->bufLen++] = *i;
    if(s->bufLen == kInBufSize-2 || *i == EOLCHAR) {
      s->buf[s->bufLen] = '\0';
      MimeProcess(s, s->buf, s->bufLen);
      s->bufLen = 0;
    }
    i++;
  } while(i!= iEnd);
#endif
}

