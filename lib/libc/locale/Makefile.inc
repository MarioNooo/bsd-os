#	BSDI Makefile.inc,v 2.6 1997/12/09 21:42:32 prb Exp
#	@(#)Makefile.inc	5.1 (Berkeley) 2/18/91

# locale sources
.PATH: ${.CURDIR}/${MACHINE}/locale ${.CURDIR}/locale

SRCS+=	euc.c frune.c isctype.c lconv.c localeconv.c mbrune.c multibyte.c \
	rune.c runenone.c runetable.c setlocale.c utf8.c wchar.c

MAN3+=	ctype.0 isalnum.0 isalpha.0 isascii.0 isblank.0 iscntrl.0 isdigit.0 \
	isgraph.0 islower.0 isprint.0 ispunct.0 isspace.0 isupper.0 \
	isxdigit.0 mbrune.0 multibyte.0 rune.0 scrwidth.0 setlocale.0 \
	toascii.0 tolower.0 toupper.0
MAN3+=  fgetwc.0 fgetws.0 fputwc.0 fputws.0 ungetwc.0 wcscat.0 wcswidth.0 \
	wctype.0
MAN5+=	euc.0 utf8.0

MLINKS+=mbrune.3 mbrrune.3 mbrune.3 mbmb.3
MLINKS+=multibyte.3 mblen.3 multibyte.3 mbstowcs.3 multibyte.3 mbtowc.3 \
	multibyte.3 wcstombs.3 multibyte.3 wctomb.3
MLINKS+=rune.3 setrunelocale.3 rune.3 setinvalidrune.3 rune.3 sgetrune.3 \
	rune.3 sputrune.3 rune.3 fgetrune.3 rune.3 fungetrune.3 \
	rune.3 fputrune.3
MLINKS+=setlocale.3 localeconv.3

MLINKS+=fgetwc.3 getwc.3 fgetwc.3 getwchar.3  
MLINKS+=fputwc.3 putwc.3 fputwc.3 putwchar.3
MLINKS+=wcscat.3 wcschr.3 wcscat.3 wcscmp.3 wcscat.3 wcscpy.3 \
	wcscat.3 wcscspn.3 wcscat.3 wcsftime.3 wcscat.3 wcslen.3 \
	wcscat.3 wcsncat.3 wcscat.3 wcsncmp.3 wcscat.3 wcsncpy.3 \
	wcscat.3 wcspbrk.3 wcscat.3 wcsrchr.3 wcscat.3 wcsspn.3 \
	wcscat.3 wcswcs.3 wcscat.3 wstring.3
 
MLINKS+=wctype.3 iswalnum.3 wctype.3 iswalpha.3 wctype.3 iswcntrl.3  \
	wctype.3 iswctype.3 wctype.3 iswdigit.3 wctype.3 iswgraph.3 \
	wctype.3 iswlower.3 wctype.3 iswprint.3 wctype.3 iswpunct.3 \
	wctype.3 iswspace.3 wctype.3 iswupper.3 wctype.3 iswxdigit.3 \
	wctype.3 towlower.3 wctype.3 towupper.3 wctype.3 wcwidth.3
