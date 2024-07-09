Date: Wed, 15 Apr 1998 22:33:43 -0700 (PDT)
From: Laurence Lundblade <lgl@island-resort.com>
To: lgl@java.island-resort.com
Subject: b2
X-UIDL: 042!!i@Yd94LJe98YNd9
Mime-version: 1.0
Content-Type: text/plain;charset="us-ascii"

tuvtttuuuvvv
/* ----------------------------------------------------------------------
   One structure to hold the state for all the different content-transfer
   decoders
   ---- */
typedef struct
{
  OutputFn *outFn;
  void     *outFnState;
  union {
  	struct {
  		char  lastWasEqual; 
  		char  is2047;
  	} QP;
  	struct {
  		unsigned long threeBytes;
  		char          shift;
  	} B64;
  } x; 
} MimeCteType, *MimeCtePtr;

typedef void MimeCteFn(MimeCtePtr state, char *buffer, long bufferLen);

