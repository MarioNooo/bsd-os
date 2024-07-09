/* ----------------------------------------------------------------------
     MIME Mangler - single pass reduction of MIME to plain text

     Laurence Lundblade <lgl@qualcomm.com>

     Copyright (c) 2003 QUALCOMM Incorporated.

     File: testjig.c - UNIX main for testing mangler on flat files
     Version: 0.2.3, Dec 1997
     Last Edited: Dec 4 21:56
   
  ---- */

#include "mime.h"
#include "mangle.h"
#include <stdio.h>


/* ----------------------------------------------------------------------
   Our output function to pass into various handlers. For example
   the text/enriched stripper needs an output handler like this. Actually
   all type handlers need something to say where the output goes.

   Args: pv  - pointer to output function's state/context
         buf - buffer of stuff to output
         len - len of data in buffer 

   Returns: nothing
   ---- */
void MyOutputFunc(void *pv, char *buf, long len)
{
   FILE *fo = (FILE *)pv;

   while (len--)
      fputc(*buf++, fo);
}




/* ----------------------------------------------------------------------
    The main has weird buffering stuff for the sake of testing. When 
    you call this, any buffering at all is acceptable from character
    at a time, to 10Mb memory mapped files.
   ---- */

/* For some reason Linux freads() bomb for large buffer sizes. e.g. 50K */

main(int argc, char **argv)
{
  char          ibuf[102];
  long          n, bufsize, line, random;
  void		*TESp;
  FILE          *fp;


  bufsize = 1024; /* The default */
  if (argc != 2) return;

  fp = fopen(argv[1],"r");
  if(!fp) return;

  TESp = textEnrichedInit(MyOutputFunc, (void *)stdout);
  while(fgets(ibuf, 100, fp) != NULL) 
	textEnrichedToHTML(TESp, ibuf, strlen(ibuf));
  fclose(fp);
}
