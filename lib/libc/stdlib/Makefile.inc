#	BSDI Makefile.inc,v 2.10 2003/06/02 21:30:35 torek Exp
# machine-independent stdlib sources
.PATH: ${.CURDIR}/${MACHINE}/stdlib ${.CURDIR}/stdlib

SRCS+=  abort.c atexit.c atof.c atoi.c atol.c bigint.c bsearch.c calloc.c \
	div.c dtoa.c exit.c getenv.c getopt.c getopt_data.c getsubopt.c \
	heapsort.c labs.c ldiv.c malloc.c merge.c putenv.c qsort.c \
	radixsort.c xrand.c rand48.c xrandom.c setenv.c strtod.c strtol.c \
	strtold.c strtoll.c strtoul.c strtoull.c system.c

# machine-dependent stdlib sources
.include "${.CURDIR}/${MACHINE}/stdlib/Makefile.inc"

MAN3+=	abort.0 abs.0 alloca.0 atexit.0 atof.0 atoi.0 atol.0 bsearch.0 \
	div.0 exit.0 getenv.0 getopt.0 getsubopt.0 labs.0 ldiv.0 malloc.0 \
	memory.0 qsort.0 radixsort.0 rand.0 rand48.0 random.0 strtod.0 \
	strtol.0 strtoul.0 system.0

MLINKS+=getenv.3 setenv.3 getenv.3 unsetenv.3 getenv.3 putenv.3
MLINKS+=getopt.3 optarg.3 getopt.3 opterr.3 getopt.3 optind.3 getopt.3 optopt.3
MLINKS+=getsubopt.3 suboptarg.3
MLINKS+=malloc.3 calloc.3 malloc.3 realloc.3 malloc.3 free.3
MLINKS+=qsort.3 heapsort.3 qsort.3 mergesort.3
MLINKS+=rand.3 srand.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
MLINKS+=strtol.3 strtoll.3 strtol.3 strtoq.3
MLINKS+=strtoul.3 strtoull.3 strtoul.3 strtouq.3
MLINKS+=rand48.3 drand48.3 rand48.3 erand48.3 rand48.3 jrand48.3
MLINKS+=rand48.3 lrand48.3 rand48.3 mrand48.3 rand48.3 nrand48.3
MLINKS+=rand48.3 srand48.3
