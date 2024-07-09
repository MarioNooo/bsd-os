/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI wchar.c,v 2.1 1997/10/30 22:45:09 donn Exp
 */

/*
 * Wide character routines.
 */

#include <ctype.h>
#include <errno.h>
#include <rune.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wchar.h>

#ifdef	MISSING_FUNCTIONS_STILL_MISSING
int wcscoll(const wchar_t *ws1, const wchar_t *ws2);
double wcstod(const wchar_t *nptr, wchar_t **endptr);
wchar_t *wcstok(wchar_t *ws1, const wchar_t *ws2);
long int wcstol(const wchar_t *nptr, wchar_t **endptr, int base);
unsigned long int wcstoul(const wchar_t *nptr, wchar_t **endptr, int base);
size_t wcsxfrm(wchar_t *ws1, const wchar_t *ws2, size_t n);

#include <iconv.h>

size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft,
    char **outbuf, size_t *outbytesleft);
int iconv_close(iconv_t cd);
iconv_t iconv_open(const char *tocode, const char *fromcode);

#include <monetary.h>

ssize_t strfmon(char *s, size_t maxsize, const char *format, ...);

#include <time.h>

char *strptime(const char *buf, const char *format, struct tm *tm);
 
#include <wordexp.h>

int wordexp(const char *words, wordexp_t *pwordexp, int flags);
void wordfree(wordexp_t *pwordexp);

#endif

wint_t
fgetwc(FILE *stream)
{
	rune_t rune;

	switch (rune = fgetrune(stream)) {
	case EOF:
		return (WEOF);

	default:
		if (rune == _INVALID_RUNE) {
			errno = EILSEQ;
			return (WEOF);
		}

		return ((wint_t)rune);
	}
}

wint_t
getwc(FILE *stream)
{
	return (fgetwc(stream));
}

wint_t
getwchar()
{
	return (fgetwc(stdin));
}

wchar_t *
fgetws(wchar_t *ws, int n, FILE *stream)
{
	wchar_t *rws;
	rune_t rune;

	rws = ws;

	if (n <= 0)
		return (NULL);

	while (n-- > 1) {
		if ((rune = fgetrune(stream)) == EOF)
			break;
		/*
		 * This is pretty stupid, but...
		 */
		if (rune == _INVALID_RUNE) {
			errno = EILSEQ;
			return (NULL);
		}
		*ws++ = (wchar_t)rune;
		if (rune == '\n')
			break;
			
	}
	*ws = 0;
	return (rws);
}

wint_t
fputwc(wint_t wc, FILE *stream)
{
	if (fputrune((rune_t)wc, stream) == 0)
		return (wc);
	return (WEOF);
}

wint_t
putwc(wint_t wc, FILE *stream)
{
	return (fputwc(wc, stream));
}

wint_t
putwchar(wint_t wc)
{
	return (fputwc(wc, stdout));
}

int
fputws(const wchar_t *ws, FILE *stream)
{
	int cnt;

	cnt = 0;

	while (*ws) {
		if (fputrune(*ws++, stream) == EOF)
			return (-1);
		++cnt;
	}
	return (cnt);
}

wint_t
ungetwc(wint_t wc, FILE *stream)
{
	if (wc == WEOF) {
		errno = EILSEQ;
		return (WEOF);
	}
	return (fungetrune(wc, stream) ? WEOF : wc);
}

wchar_t *
wcscat(wchar_t *ws1, const wchar_t *ws2)
{
	wchar_t *dst;

	for (dst = ws1; *dst; ++dst)
		;
	while (*dst++ = *ws2++)
		;
	return (ws1);

}

wchar_t *
wcschr(wchar_t *ws, wchar_t wc)
{
	while (*ws && *ws != wc)
		++ws;
	return (*ws == wc ? ws : NULL);
}

wchar_t *
wcsrchr(wchar_t *ws, wchar_t wc)
{
	wchar_t *wss;

	wss = NULL;
	while (*ws) {
		if (*ws == wc)
			wss = ws;
		++ws;
	}
	return (wss);
}

int
wcscmp(const wchar_t *ws1, const wchar_t *ws2)
{
	while (*ws1 && *ws1 == *ws2) {
		++ws1;
		++ws2;
	}
	return (*ws1 - *ws2);
}

int
wcsncmp(const wchar_t *ws1, const wchar_t *ws2, size_t n)
{
	while (n > 0 && *ws1 && *ws1 == *ws2) {
		++ws1;
		++ws2;
		--n;
	}
	return (n > 0 ? *ws1 - *ws2 : 0);
}

wchar_t *
wcscpy(wchar_t *ws1, const wchar_t *ws2)
{
	wchar_t *dst;

	dst = ws1;
	while (*dst++ = *ws2++)
		;
	return (ws1);
}

wchar_t *
wcsncpy(wchar_t *ws1, const wchar_t *ws2, size_t n)
{
	wchar_t *dst;

	if (n < 1)
		return (ws1);

	dst = ws1;
	while (n > 0 && (*dst = *ws2++) != 0) {
		++dst;
		--n;
	}
	while (n-- > 0)
		*dst++ = 0;
	return (ws1);
}

size_t
wcscspn(wchar_t *ws1, wchar_t *ws2)
{
	size_t cnt = 0;

	while (*ws1 && wcschr(ws2, *ws1) == NULL) {
		++ws1;
		++cnt;
	}
	return (cnt);
}

size_t
wcssspn(wchar_t *ws1, wchar_t *ws2)
{
	size_t cnt = 0;

	while (*ws1 && wcschr(ws2, *ws1) != NULL) {
		++ws1;
		++cnt;
	}
	return (cnt);
}

wchar_t *
wcspbrk(wchar_t *ws1, wchar_t *ws2)
{
	while (*ws1 && wcschr(ws2, *ws1) == NULL)
		++ws1;
	return (ws1);
}

size_t
wcsftime(wchar_t *wcs, size_t maxsize, char *format, struct tm *timeptr)
{
	size_t cnt;
	char *buf = malloc(maxsize);

	if (buf == NULL)
		return (0);		/* XXX */

	cnt = strftime(buf, maxsize, format, timeptr);
	if (cnt == 0) {
		mbstowcs(wcs, buf, maxsize);
		free (buf);
		return (0);
	}
	cnt = mbstowcs(wcs, buf, cnt);
	free (buf);
	return (cnt);
		
}

size_t
wcslen(wchar_t *ws)
{
	size_t cnt;

	cnt = 0;
	while (*ws++)
		++cnt;
	return (cnt);
}

wchar_t *
wcsncat(wchar_t *ws1, wchar_t *ws2, size_t n)
{
	wchar_t *dst;

	if (n < 1)
		return (ws1);
		
	for (dst = ws1; *dst; ++dst)
		;
	while (n > 1 && (*dst = *ws2++) != 0)
		++dst;
	*dst = 0;
	return (ws1);
}

wchar_t *
wcswcs(wchar_t *ws1, wchar_t *ws2)
{
	size_t n;

	n = wcslen(ws2);

	while (*ws1) {
		if (*ws1 == *ws2 && wcsncmp(ws1, ws2, n) == 0)
			return (ws1);
	}
	return (NULL);
}
 
int
wcswidth(wchar_t *pwcs, size_t n)
{
	int cnt;

	cnt = 0;
	while (n-- > 0 && *pwcs)  {
		cnt += scrwidth(*pwcs);
		++pwcs;
	}
	return (cnt);
}

wctype_t
wctype(char *charclass)
{
#define	WCTYPE(s, n) if (strcmp(charclass, s) == 0) return (n)
	WCTYPE("alnum", _A|_D);
	WCTYPE("alpha", _A);
	WCTYPE("blank", _B);
	WCTYPE("cntrl", _C);
	WCTYPE("digit", _D);
	WCTYPE("graph", _G);
	WCTYPE("lower", _L);
	WCTYPE("print", _R);
	WCTYPE("punct", _P);
	WCTYPE("space", _S);
	WCTYPE("upper", _U);
	WCTYPE("xdigit", _X);
	WCTYPE("phonogram", _Q);
	WCTYPE("special", _T);
	WCTYPE("ideogram", _I);
#undef	WCTYPE
	return (0);
}

int
wcwidth(wint_t wc)
{

	return (scrwidth(wc));
}

int
iswalnum(wint_t wc)
{

	return (isalnum(wc));
}


int
iswalpha(wint_t wc)
{

	return (isalpha(wc));
}


int
iswcntrl(wint_t wc)
{

	return (iscntrl(wc));
}


int
iswctype(wint_t wc, wctype_t charclass)
{

	return (__isctype(wc, charclass));
}


int
iswdigit(wint_t wc)
{

	return (isdigit(wc));
}


int
iswgraph(wint_t wc)
{

	return (isgraph(wc));
}


int
iswlower(wint_t wc)
{

	return (islower(wc));
}


int
iswprint(wint_t wc)
{

	return (isprint(wc));
}


int
iswpunct(wint_t wc)
{

	return (ispunct(wc));
}


int
iswspace(wint_t wc)
{

	return (isspace(wc));
}


int
iswupper(wint_t wc)
{

	return (isupper(wc));
}


int
iswxdigit(wint_t wc)
{

	return (isxdigit(wc));
}
