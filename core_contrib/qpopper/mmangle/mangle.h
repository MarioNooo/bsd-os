/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated

     File: mangle.h -- Functions called back by mime parser to do mangling
     Version: 0.2.3, Dec 1997
     Last Edited: 9 March 2000
   
  ---- */
#ifndef _MANGLEINCLUDED
#define _MANGLEINCLUDED
#include "config.h"
#include "charmangle.h"
typedef enum { 
    TextPlain=0x0, TextHTML, TextEnriched, TextUnknown, TextNot
} TextAttrType;

typedef struct {
    unsigned long size;
    char **elem;
} StringVectorType;

typedef struct {
    TextAttrType m_text;
    TextCharSetType m_char_set;
    /* LANG, COLUMNS */
    StringVectorType *m_headers;
    char              m_bNoContent; /* Don't want content, skip MIME headers */
}MangleInfoType;

#define kMaxParts 16

typedef struct {
  long      omitMap[kMaxMIMENesting];
  TextAttrType parts[kMaxParts];
  int          partCount;
  OutputFn *outFnConstructor;
  OutputFn *outFn;
  OutputFn *outFnDestructor;
  void     *outFnState;
  char      lastWasGoodHeader;
  MangleInfoType rqInfo;
} ManglerStateType, *ManglerStatePtr;

void MangleMapper (
  void           *ourState,
  MimeErrorKind   mError,
  MimeTypePtr     mType,
  MultAttribsPtr  multAttribs,
  OutputFn      **outFnConstructor,
  OutputFn      **outFn,
  OutputFn      **outFnDestructor,
  void          **outFnState,
  Rfc822Fn      **headerFn,
  void          **headerFnState
);

void FreeMangleInfo ( MangleInfoType *infoStruct );

int FillMangleInfo (
  char           *commandLine,   /* MANGLE(ATTR=value;...) */
  char            bNoContent,
  MangleInfoType *infoStruct  
);

#endif /* MANGLEINCLUDE */
