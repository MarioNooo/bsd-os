/*	BSDI dlfcn.h,v 2.6 1999/08/09 15:05:08 donn Exp	*/

#ifndef _DLFCN_H_
#define	_DLFCN_H_

#include <sys/cdefs.h>

#define	RTLD_LAZY	1
#define	RTLD_NOW	2

/* Linux extensions which we currently support.  */
#define	RTLD_GLOBAL	0x100
#define	RTLD_NEXT	((void *)-1)

/* Support for dladdr(), a Solaris extension.  */
typedef struct {
        const char      *dli_fname;     /* pointer to filename of object */
        void            *dli_fbase;     /* base address of object */
        const char      *dli_sname;     /* name of symbol nearest to address */
        void            *dli_saddr;     /* address of that symbol */
} Dl_info;

__BEGIN_DECLS
int dladdr __P((void *, Dl_info *));
int dlclose __P((void *));
char *dlerror __P((void));
void *dlopen __P((const char *, int));
void *dlsym __P((void *, const char *));
__END_DECLS

#endif
