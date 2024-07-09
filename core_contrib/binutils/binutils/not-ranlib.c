/* Linked with ar.o to flag that this program is 'ar' (not 'ranlib').  */

#ifdef __bsdi__
/* XXX -- this is not technically the "right" way to do it, but
   avoids needing to change two separate files, Makefile.am and
   Makefile.in */
int is_ranlib = -1;
#else
int is_ranlib = 0;
#endif
