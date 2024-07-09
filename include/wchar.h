/*	BSDI wchar.h,v 2.3 2000/01/09 03:40:53 prb Exp	*/

/*
 * Wide character support.
 */
#ifndef	_WCHAR_H_
#define	_WCHAR_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>
#include <machine/limits.h>
/*
 * We are not allowed to include other standard header files, but we must
 * work even if the user does not include them for us.  Blech.
 */
struct __sFILE;
struct tm;

#ifdef	_BSD_WCHAR_T_
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
typedef	_BSD_WCHAR_T_	rune_t;
#endif
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_	size_t;
#undef  _BSD_SIZE_T_
#endif

typedef	int wint_t;
typedef	int wctype_t;

#ifndef NULL
#define NULL	0
#endif

#define	WCHAR_MAX	INT_MAX
#define	WCHAR_MIN	INT_MIN

#define	WEOF	(-1)
#define	EILSEQ	101

__BEGIN_DECLS
wint_t fgetwc(struct __sFILE *stream);
wint_t getwc(struct __sFILE *stream);
wint_t getwchar();
wchar_t * fgetws(wchar_t *ws, int n, struct __sFILE *stream);
wint_t fputwc(wint_t wc, struct __sFILE *stream);
wint_t putwc(wint_t wc, struct __sFILE *stream);
wint_t putwchar(wint_t wc);
int fputws(const wchar_t *ws, struct __sFILE *stream);
wint_t ungetwc(wint_t wc, struct __sFILE *stream);
wchar_t *wcscat(wchar_t *ws1, const wchar_t *ws2);
wchar_t *wcschr(wchar_t *ws, wchar_t wc);
wchar_t * wcsrchr(wchar_t *ws, wchar_t wc);
int wcscmp(const wchar_t *ws1, const wchar_t *ws2);
int wcsncmp(const wchar_t *ws1, const wchar_t *ws2, size_t n);
wchar_t * wcscpy(wchar_t *ws1, const wchar_t *ws2);
wchar_t * wcsncpy(wchar_t *ws1, const wchar_t *ws2, size_t n);
size_t wcscspn(wchar_t *ws1, wchar_t *ws2);
size_t wcssspn(wchar_t *ws1, wchar_t *ws2);
wchar_t * wcspbrk(wchar_t *ws1, wchar_t *ws2);
size_t wcsftime(wchar_t *wcs, size_t maxsize, char *format, struct tm *timeptr);
size_t wcslen(wchar_t *ws);
wchar_t * wcsncat(wchar_t *ws1, wchar_t *ws2, size_t n);
wchar_t * wcswcs(wchar_t *ws1, wchar_t *ws2);
int wcswidth(wchar_t *pwcs, size_t n);
wctype_t wctype(char *charclass);
int wcwidth(wint_t wc);
int iswalnum(wint_t wc);
int iswalpha(wint_t wc);
int iswcntrl(wint_t wc);
int iswctype(wint_t wc, wctype_t charclass);
int iswdigit(wint_t wc);
int iswgraph(wint_t wc);
int iswlower(wint_t wc);
int iswprint(wint_t wc);
int iswpunct(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswxdigit(wint_t wc);
__END_DECLS
#endif /* !_WCHAR_H_ */
