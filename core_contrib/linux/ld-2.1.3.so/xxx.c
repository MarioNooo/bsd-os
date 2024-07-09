/*	BSDI xxx.c,v 1.1 2000/02/12 17:16:04 donn Exp	*/

#include <sys/types.h>

void *__curbrk = 0;
int errno = 0;

/*
 * Disable some GNU features (sigh).
 */
int
__profile_frequency()
{

	return (0);
}

int
__profil(unsigned short *sp, size_t a, size_t b, unsigned int c)
{

	return (-1);
}

char *
__dcgettext(const char *domain, const char *s, int category)
{

	return ((char *)s);
}
