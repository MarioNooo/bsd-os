/* ----------------------------------------------------------------------
     MIME Parser -- single pass MIME parser

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.

     File: mime.h
     Version: 0.2.7   April, 2000
     Last Edited: Apr 26, 2000

  ---- */


#ifndef _MIMEINCLUDED
#define _MIMEINCLUDED

#include "config.h"
#include <stdio.h>
/* ======================================================================
     C O N F I G    S T U F F
   ====================================================================== */
/* ----------------------------------------------------------------------
   Deepest multipart nesting handled. Parts deeper are ignored and an
   error code is given. Increasing this increases stored state by about
   100 bytes/level, depending on other constants below.
   --- */
#define kMaxMIMENesting (5) 


/* ----------------------------------------------------------------------
   Set by RFC 2046. Don't change this.
   --- */
#define kMIMEBoundaryLen (76) 


/* ----------------------------------------------------------------------
   We choose largest token (e.g. parameter value) we handle as the
   boundary parameter. Could be larger if we want.
   --- */
#define kMIMETokenLen  kMIMEBoundaryLen


/* ----------------------------------------------------------------------
   Size of our internal input buffering. This must be at least 
   kMIMEBoundaryLen plus the end of line length plus one. It can be much
   larger if you can afford it, with some small gain in efficiency by
   way of fewer callbacks. 
   --- */
#define kInBufSize  (100) 




/* ======================================================================
     P U B L I C   I N T E R F A C E, Part 1
   ====================================================================== */

/* ----------------------------------------------------------------------
    Structures to store MIME type info 
    
    This structure is such that we can nil it out by clearing everything
    to zero, which makes our code smaller and faster.

    The ordering of most of the enums here is related to the order
    of string constants in the parsing code (don't change them)

    We ignore some MIME headers such as content-md5, and content description

    Parameters are handled a it oddly to drastically reduce stored state.
    We only are interested in a small number of them (e.g, boundary
    charset, Filename), and only handle a limited number per mime type.
    This is possible because most MIME types have less than some number
    (e.g. 2) parameters that we are interested in.

    The parameters from the content-disposition header are merged with
    the content-type parameters.

    Parameter values, which are potentially arbitrarily long, are truncated
    at kMIMETokenLen.

    Parameter values can be looked up quickly by indexing by MimeParamKind
    into paramIndex to get the index of the value in the param array.
    (This makes code to find a parameter of interest in the list very small)
   ---- */
typedef enum {majorTypeNone=0, majorTypeUnknown, majorTypeAudio,
              majorTypeImage, majorTypeVideo, majorTypeModel, majorTypeMulti,
              majorTypeApp, majorTypeMsg, majorTypeText}
             MajorTypeKind;

typedef enum {cte7bit=0, cteBinary, cte8bit,
              cteQP, cteBase64, cteBadCTE} CteKind;

typedef enum {paramNone=0, paramBoundary, paramCharset, paramType,
              paramFilename, paramTypes, paramFormat, paramExtType,
              paramExtSize, paramExtSite, paramExtName, paramExtDir,
              paramExtMode, paramMax} MimeParamKind;


#define kParamMaxNum        (2) /* Total number of instances */
#define kParamIndexUnused (255) /* don't change this */


typedef struct {
  MajorTypeKind   majorType;
  char            minorType[kMIMETokenLen]; 
  enum {dispNone=0, dispInline, dispAttach}
                  contentDisp;
  CteKind         cte;
  struct {
    MimeParamKind name;
    char          value[kMIMETokenLen];
  }               param[kParamMaxNum];
  unsigned char   paramIndex[paramMax];
  unsigned char   nextParamPos; /* Private - used during parsing only! */
} MimeTypeType, *MimeTypePtr;


/* ----------------------------------------------------------------------
   This structure describes the nesting of multi part types so a
   MIME type handler can know where it is to make type handling decisions
   (e.g., multipart/alternative handling).  The nestLevel element is the
   current nesting level. It is 0 in the outside 822 body, 1 for the
   elements of the first multipart, etc. It can be used to index into
   attribs to get to attributes of the current nesting level(attribs[0]
   corresponds to nest level 1, since there are no attributes for level 0).
   (Be sure nestLevel > 0 before you go attribs[nestLevel-1].) The
   attributes include the type of the multipart, and the ordinal number
   of the part being processed. The multipart types are again limited
   to a select few to save space.
  --- */
typedef enum {multNuthinSpecial=0, multAlt, multAppleII, multReport, 
	      multRelated} MultTypeKind;

typedef struct {
  unsigned char nestLevel;
  struct {
    unsigned char partNum;
    MultTypeKind  mult;
    signed char   LookAHead;     /* Look a head for all text parts in 
				    alternative  */
    unsigned long offset;        /* Preserve the offset in file when
				    look a head starts */
    unsigned long lines;         /* lines to go in message since lookahead
				    started. */
  } attribs[kMaxMIMENesting];
} MultAttribsType, *MultAttribsPtr;




/* ----------------------------------------------------------------------
   This is a prototype for a functions supplied to handle a particular
   MIME type. The user of this MIME parser defines functions for handling
   various types and pass the function to the MIME parser. The MIME parser
   then calls back the functions on buffers of data from the entity. 

   The transfer encoding is removed before being passed to this call back.
   The data passed to this call back may be binary, and is not guaranteed
   to be NULL terminated. The function may modify the buffer passed to
   it. It of course can't assume its bigger then len.

   Sample handlers are the text/enriched and text/html strippers. 

   The last call to this function for a part has buf set to NULL to indicate
   handling of the part is complete. 
  --- */
typedef void OutputFn(void *oFnState, char *buf, long len);


/* ----------------------------------------------------------------------
   This is the call back supplied by the user to the MIME parser for 
   handling non-MIME headers. The function is called with the contents
   of each header. The isContination parameter will be 1 if the buffer
   is a continuation of the previous header.  The state parameter
   is a pointer the caller initializes in the MimeInit call. 

   The blank line that separates the header from the body IS passed, and
   signals that all headers for a MIME entity have been passed, and that
   there is no continuation for the last header.

   ---*/
typedef void Rfc822Fn(void         *state,
                      char         *header,
                      long          len,
                      unsigned char isContinuation);


/* ----------------------------------------------------------------------
   Another call back supplied by the user. This one is called for each
   MIME entity the parser encounters. The function is given the mime
   type and multipart attributes. It must supply an output function
   and a context/state to be passed to the output function.

   The very first call to this function has a NULL mType and multAttribs,
   and is to get the headerFn for very top level headers. At that point no 
   MIME type is known. The outFn pointer will be NULL too, so don't
   try and pass that back.

   This is where all the real handling happens. 

   Args:  typeMapperState - The state passed to MimeInit
          mError          - An error code for current part
          mType           - Mime type of current part
          multAttribs     - Describes nesting
          outFn           - Place to return content handling function
          outFnState      - State for content handling function
          headerFn        - Header outputing function
          headerFnState   - Header outputing state
   --- */
typedef enum {merrorNone, merrorTooDeep, merrorBadCTE,
              merrorNoBoundary} MimeErrorKind;

typedef void MimeTypeFn(
  void                 *state,
  const MimeErrorKind   mError,
  const MimeTypePtr     mType,
  const MultAttribsPtr  multAttribs,
  OutputFn            **outFnConstructor,
  OutputFn            **outFn,
  OutputFn            **outFnDestructor,
  void                **outFnState,
  Rfc822Fn            **headerFn,
  void                **headerFnState);


/* ================= E N D   P U B L I C   S T U F F  ================ */



/* ======================================================================
    P R I V A T E   S T U F F 
   ====================================================================== */
/* ----------------------------------------------------------------------
   Keep track of state of rfc822 lexer.  Can initialize this structure
   by zeroing all bytes.
   ---- */
typedef struct {
  unsigned char parenNesting; /* Level of parenthesis nesting */
  unsigned char inQuotes;     /* Are we in a quoted part? */
  unsigned char dontDownCase; /* Don't make lower when copying tokens */
  unsigned char gettingToken; /* Not looking for space */
  unsigned char tokenLen;     /* Length of token we've buffered up */
} Rfc822LexType;


/* ----------------------------------------------------------------------
   Keep track of MIME header parser state
   ---- */
typedef struct {
  Rfc822LexType      lexState;
  enum {wantMajor, wantSlash, wantMinor, wantSemi, wantParamName,
        wantEqual, wantParamValue, wantDisp, wantDesc, wantCTE,
        wantNothing} parseState;
  MimeParamKind      paramWanted;
  char               tokenBuf[kMIMETokenLen+1];
} HeaderParseType;


/* ----------------------------------------------------------------------
   One structure to hold the state for all the different content-transfer
   decoders
   ---- */
typedef struct
{
  OutputFn *outFn;
  OutputFn *outFnCtor;
  OutputFn *outFnDtor;
  void     *outFnState;
  union {
  	struct {
  		char  lastWasEqual; 
  		char  is2047;
  	} QP;
  	struct {
  		unsigned long threeBytes;
  		signed char   shift;
                char          pad;
  	} B64;
  } x; 
} MimeCteType, *MimeCtePtr;

typedef void MimeCteFn(MimeCtePtr state, char *buffer, long bufferLen);


/* ----------------------------------------------------------------------
   The main structure for storing all our junk
   --- */
typedef struct {
  /* ---- Track boundaries and major MIME parser state ---- */
  signed char     currentBoundary; 
  char            mimeBoundaries[kMaxMIMENesting][kMIMEBoundaryLen];
  enum {parseHeaders=0, parseBodyPart, parsePartStart, parseMimeHeaders}
                  msgPartState; /* State for multipart and message types */

  /* ---- General header parser state ---- */
  enum {wantedHeader=0, mimeHeader, unwantedHeader, rfc822Header}
                  headerDisposition;
  unsigned char   isMIME;
  unsigned char   lastHadNoEOL;
  char            gotTopHeaderHandler;

  /* ---- MIME type of current part ---- */
  MimeTypeType    mType;

  /* ---- Track the multipart nesting that got us to where we are ---- */
  MultAttribsType multAttribs;

  /* ---- MIME header (Content-*) parser state ---- */
  HeaderParseType mHeaderState;

  /* ---- Stuff to handle CTE ---- */
  /* ---- The CTE handler state includes specific data handler ---- */
  enum {outputCTE, outputNone, outputDirect} outputTag;
  OutputFn       *cteHandlerConstructor;
  OutputFn       *cteHandlerDestructor;
  union {
    OutputFn       *outHandler; 
    MimeCteFn      *cteHandler;
  } output;
  union {
    MimeCteType     cteState; /* When we actually do CTE */
    void           *oFnState; /* When we go direct to output handler */
  } outState;

  /* --- The mime type mapper call back --- */
  MimeTypeFn     *mapperFun;
  void           *mapperFunState;
  Rfc822Fn       *headerHandler;
  void           *headerHandlerState;
  
  /* --- handle fact that trailing newline may be part of boundary --- */
  char            savedNewline;

  /* --- input blocking to make boundary detection easier --- */
  char            buf[kInBufSize];/*Buffer input once for boundary checking*/
  short           bufLen;
  FILE           *mdrop;
} MimeParseType, *MimeParsePtr;

/* ================= E N D   P R I V A T E   S T U F F  ================ */




/* ======================================================================
     P U B L I C   I N T E R F A C E, Part 2
   ====================================================================== */



/* ----------------------------------------------------------------------
   Initialize the MIME mangler - call this at start of every message

   Args: typeMapper - A function to be called for each MIME type (see above)
         typeMapperState - a pointer/context passed to each mapper call

   Returns: pointer to Mime parser state/context or NULL
   ---- */ 

MimeParsePtr MimeInit __PROTO((
  MimeTypeFn    *typeMapper,
  void          *typeMapperState,
  FILE          *mbox));



/* ----------------------------------------------------------------------
   Done with MIME parser - clean up

   Args: s - State/context

   Bug: This ought to flush any buffered input
   ---- */ 
void MimeFinish __PROTO((MimeParsePtr s));



/*----------------------------------------------------------------------
  Actually do the MIME parser

  Args: state - State/context returned from the MimeInit call
        inBuf - a buffer of input to process
        inBufLen - length of the input buffer
  
  Returns: nothing
 
  The input buffers may be any size from byte at a type to megabyte
  buffers at a time. The input may also be binary MIME. 

  Calls here result in calls to the typeMapper function, which results
  in calls to the output function supplied by the type mapper. 
  ---- */
void MimeInput __PROTO((
  MimeParsePtr  state,
  const char   *inBuf,
  unsigned long inBufLen));


#endif /* _MIMEINCLUDED */



