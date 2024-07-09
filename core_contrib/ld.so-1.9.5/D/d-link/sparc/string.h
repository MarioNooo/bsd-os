#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

/* string.h: External definitions for optimized assembly string
             routines for the Linux Kernel.

   Copyright (C) 1994 David S. Miller (davem@caip.rutgers.edu)
   Adapted for linux ELF dynamic loader by Eric Youngdale(eric@aib.com).
*/


#include <linux/types.h>	/* for size_t */

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern inline size_t _dl_strlen(const char * s);
extern inline int _dl_strcmp(const char * cs,const char * ct);
extern inline int _dl_strncmp(const char * cs,const char * ct,size_t count);
extern inline char * _dl_strcpy(char * dest,const char *src);
extern inline char * _dl_strncpy(char * dest,const char *src,size_t count);
extern inline char * _dl_strcat(char * dest,const char * src);
extern inline char * _dl_strncat(char * dest,const char * src,size_t count);
extern inline char * _dl_strchr(const char * s,char c);
extern inline char * _dl_strpbrk(const char * cs,const char * ct);
extern inline size_t _dl_strspn(const char * cs, const char * ct);
extern inline char * _dl_strtok(char * s,const char * ct);
extern inline void * _dl_memset(void * s,char c,size_t count);
extern inline void * _dl_memcpy(void * to, const void * from, size_t n);
extern inline void * _dl_memmove(void * dest,const void * src, size_t n);
extern inline int _dl_memcmp(const void * cs,const void * ct,size_t count);

extern __inline__ size_t _dl_strlen(const char * s1)
{
	size_t count = 0;

	while (((const unsigned char *)s1)[count] != '\0')
		count++;
	return count;
}

extern __inline__ int _dl_strcmp(const char *s1, const char *s2)
{
	int c1, c2, result;
	size_t i;

	__asm__("#strcmp\n"
"	clr	%1\n"
"1:	ldub	[%4+%1], %2\n"
"	ldub	[%5+%1], %3\n"
"	cmp	%2, 0\n"
"	be	2f\n"
"	 subcc	%2, %3, %0\n"
"	be,a	1b\n"
"	 inc	%1\n"
"2:	#end strcmp" :
	    "=&r"(result), "=&r"(i), "=&r"(c1), "=&r"(c2) :
	    "r"(s1), "r"(s2) : "cc");
	return result;
}

extern __inline__ int _dl_strncmp(const char* str1, const char* str2, size_t strlen)
{
  register int retval=0;

  __asm__("cmp %3, 0x0\n\t"
	  "be 2f\n\t"
	  "ldub [%2], %%g3\n\t"
	  "1: ldub [%1], %%g2\n\t"
	  "sub %%g2, %%g3, %0\n\t"
	  "cmp %0, 0x0\n\t"
	  "bne 2f\n\t"
	  "add %2, 0x1, %2\n\t"
	  "cmp %%g2, 0x0\n\t"
	  "be 2f\n\t"
	  "add %1, 0x1, %1\n\t"
	  "addcc %3, -1, %3\n\t"
	  "bne,a 1b\n\t"
	  "ldub [%2], %%g3\n\t"
	  "2: " :
	  "=r" (retval), "=r" (str1), "=r" (str2), "=r" (strlen) :
	  "0" (retval), "1" (str1), "2" (str2), "3" (strlen) :
	  "%g2", "%g3");

  return retval;
}


extern __inline__ char *_dl_strcpy(char* dest, const char* source)
{
  register char *retval;

  retval = dest;
  while (*dest++ = *source++)
      ;
#if 0
  register char tmp;
  __asm__("or %%g0, %2, %0\n\t"
          "ldub [%1], %3\n\t"
          "1: stb %3, [%2]\n\t"
          "cmp %3, 0x0\n\t"
          "bne,a 1b\n\t"
          "ldub [%1], %3\n\t" :
          "=r" (retval), "=r" (source), "=r" (dest), "=r" (tmp) :
          "0" (retval), "1" (source), "2" (dest), "3" (tmp));
#endif
  return retval;
}

extern __inline__ char *_dl_strncpy(char *dest, const char *source, size_t cpylen)
{
  register char tmp;
  register char *retval;

  __asm__("or %%g0, %1, %0\n\t"
	  "1: cmp %4, 0x0\n\t"
	  "be 2f\n\t"
	  "ldub [%1], %3\n\t"
	  "add %1, 0x1, %1\n\t"
	  "stb %3, [%2]\n\t"
	  "sub %4, 0x1, %4\n\t"
	  "ba 1\n\t"
	  "add %2, 0x1, %2\n\t" :
	  "=r" (retval), "=r" (dest), "=r" (source), "=r"(tmp), "=r" (cpylen) :
	  "0" (retval), "1" (dest), "2" (source), 
	  "3" (tmp), "4" (cpylen));

  return retval;
}

extern __inline__ char *_dl_strcat(char *dest, const char *src)
{
  register char *retval;
  register char temp=0;

  __asm__("or %%g0, %1, %0\n\t"
	  "1: ldub [%1], %3\n\t"
	  "cmp %3, 0x0\n\t"
	  "bne,a 1b\n\t"
	  "add %1, 0x1, %1\n\t"
	  "2: ldub [%2], %3\n\t"
	  "stb %3, [%1]\n\t"
	  "add %1, 0x1, %1\n\t"
	  "cmp %3, 0x0\n\t"
	  "bne 2b\n\t"
	  "add %2, 0x1, %2\n\t" :
	  "=r" (retval), "=r" (dest), "=r" (src), "=r" (temp) :
	  "0" (retval), "1" (dest), "2" (src), "3" (temp));

  return retval;
}

extern __inline__ char *_dl_strncat(char *dest, const char *src, size_t len)
{
  register char *retval;
  register char temp=0;

  __asm__("or %%g0, %1, %0\n\t"
	  "1: ldub [%1], %3\n\t"
	  "cmp %3, 0x0\n\t"
	  "bne,a 1b\n\t"
	  "add %1, 0x1, %1\n\t"
	  "2: ldub [%2], %3\n\t"
	  "stb %3, [%1]\n\t"
	  "add %1, 0x1, %1\n\t"
	  "add %3, -1, %3\n\t"
	  "cmp %3, 0x0\n\t"
	  "bne 2b\n\t"
	  "add %2, 0x1, %2\n\t" :
	  "=r" (retval), "=r" (dest), "=r" (src), "=r" (len), "=r" (temp) :
	  "0" (retval), "1" (dest), "2" (src), "3" (len), "4" (temp));

  return retval;
}

extern __inline__ char *_dl_strchr(const char *src, char c)
{
  register char temp=0;
  register char *trick=0;

  __asm__("1: ldub [%0], %2\n\t"
	  "cmp %2, %1\n\t"
	  "bne,a 1b\n\t"
	  "add %0, 0x1, %0\n\t"
	  "or %%g0, %0, %3\n\t" :
	  "=r" (src), "=r" (c), "=r" (temp), "=r" (trick), "=r" (src) :
	  "0" (src), "1" (c), "2" (temp), "3" (trick), "4" (src));

  return trick;
}

extern __inline__ char *_dl_strpbrk(const char *str, const char *search)
{
	register int c, sc;
	register char *scanp;

	__asm__("#strpbrk\n"
"1:	ldub	[%0], %2\n"	/* c = *str */
"	mov	%4, %1\n"	/* scanp = search */
"2:	ldub	[%1], %3\n"	/* sc = *scanp */
"	cmp	%3, %2\n"	/* sc == c? */
"	be	3f\n"		/* yes, done */
"	 cmp	%3, 0\n"	/* sc == 0? */
"	bne,a	2b\n"		/* no, look at more from scanp++ */
"	 inc	%1\n"
"	b	1b\n"		/* restart main loop on str++ */
"	 inc	%0\n"
"3:	be,a	4f\n"		/* if sc==0, c==0, */
"	 clr	%0\n"		/* so result is NULL */
"4:	#end strpbrk" :
	    "=&r" (str), "=&r"(scanp), "=&r"(c), "=&r"(sc) :
	    "r"(search), "0"(str) : "cc");
	return (char *)str;
}

      
extern __inline__ size_t _dl_strspn(const char *s, const char *accept)
{
  register char temp1, temp2;
  register char* scratch;
  register size_t trick;

  __asm__("or %%g0, %1, %4\n\t"
	  "1: ldub [%0], %2\n\t"
	  "2: ldub [%1], %3\n\t"
	  "cmp %3, 0x0\n\t"
	  "be 3f\n\t"
	  "cmp %3, %2"
	  "bne 2b\n\t"
	  "add %1, 0x1, %1\n\t"
	  "add %0, 0x1, %0\n\t"
	  "b 1b\n\t"
	  "add %5, 0x1, %5\n\t"
	  "3: or %%g0, %0, %4\n\t" :
	  "=r" (s) :
	  "r" (accept), "r" (temp1), "r" (temp2), 
	  "r" (scratch), "r" (trick=0), "0" (s));

  return trick;

}

extern __inline__ void *_dl_memset(void *src, char c, size_t count)
{
  register char *retval;

  retval = (char *) src;
  while(count--)
  {
	  *retval++ = c;
  }
}

extern __inline__ void *_dl_memcpy(void *dest, const void *src, size_t count)
{
  register char *d = dest;
  register const char *s = src;
  int i;

  for (i = 0; i < count; i++){
      *d++ = *s++;
  }
  return dest;
#if 0
  register void *retval;
  __asm__("or %%g0, %1, %0\n\t"
	  "add %3, -1, %3\n\t"
	  "cmp %3, -1\n\t"
	  "be 2f\n\t"
	  "1: ldub [%2], %4\n\t"
	  "add %2, 0x1, %2\n\t"
	  "add %3, -1, %3\n\t"
	  "cmp %3, -1\n\t"
	  "stb %4, [%1]\n\t"
	  "bne 1b\n\t"
	  "add %1, 0x1, %1\n\t"
	  "2: " :
	  "=r" (retval), "=r" (dest), "=r" (src), "=r" (count), "=r" (tmp) :
	  "0" (retval), "1" (dest), "2" (src), "3" (count), "4" (tmp));
  return retval;
#endif
}

extern __inline__ void *_dl_memmove(void *dest, const void *src, size_t count)
{
  register void *retval;
  register char tmp;

  __asm__("or %%g0, %1, %1\n\t"
	  "add %3, -1, %3\n\t"
	  "cmp %3, -1\n\t"
	  "be 2f\n\t"
	  "1: ldub [%2], %4\n\t"
	  "add %2, 0x1, %2\n\t"
	  "add %3, -1, %3\n\t"
	  "cmp %3, -1\n\t"
	  "stb %4, [%1]\n\t"
	  "bne 1b\n\t"
	  "add %1, 0x1, %1\n\t"
	  "2: " :
	  "=r" (retval), "=r" (dest), "=r" (src), "=r" (count), "=r" (tmp) :
	  "0" (retval), "1" (dest), "2" (src), "3" (count), "4" (tmp));

  return retval;
}

/* Not really very important to be fast, it's just used to check signature */
/* of cache */
extern __inline__ int _dl_memcmp(const void *cs, const void *ct, size_t count)
{
  const char *s, *d;

  s = cs;
  d = ct;
  while (count--){
      if (*s != *d)
	  return s-d;
      s++;
      d++;
  }
  return 0;
#if 0
  register int retval;
  register unsigned long tmp1, tmp2;
  
    __asm__("or %%g0, %1, %0\n\t"
	  "cmp %3, 0x0\n\t"
	  "ble,a 3f\n\t"
	  "or %%g0, %%g0, %0\n\t"
	  "1: ldub [%1], %4\n\t"
	  "ldub [%2], %5\n\t"
	  "cmp %4, %5\n\t"
	  "be,a 2f\n\t"
	  "add %1, 0x1, %1\n\t"
	  "bgeu 3f\n\t"
	  "or %%g0, 0x1, %0\n\t"
	  "b 3f\n\t"
	  "or %%g0, -1, %0\n\t"
	  "2: add %3, -1, %3\n\t"
	  "cmp %3, 0x0\n\t"
	  "bg 1b\n\t"
	  "add %2, 0x1, %2\n\t"
	  "or %%g0, %%g0, %0\n\t"
	  "3: " :
	  "=r" (retval) :
	  "r" (cs), "r" (ct), "r" (count), "r" (tmp1=0), "r" (tmp2=0));
  return retval;
#endif
}

#endif
