LIB=	bn
NOPROFILE=noprofile

# Extra source files (e.g. lbnalpha.s for Alpha assembly routines)
.if ${MACHINE} == "i386"
AUXSRC= lbn80386.s
.endif

# Extra definitions (e.g. -DBNINCLUDE=lbnalpha.h)
.if ${MACHINE} == "i386"
DEFINE= -DBNINCLUDE=lbn80386.h
.endif

# If you have a machine-specific assembly file, add it to AUXSRC.
CSRCS=	bn00.c lbn00.c bn.c lbnmem.c sieve.c prime.c
CSRCS+=	bnprint.c legal.c jacobi.c germain.c
SRCS= $(CSRCS) $(AUXSRC)

GENSRCS= lbn32.c lbn64.c lbn32.h lbn64.h bn32.c bn64.c bn32.h bn64.h \
	bninit32.c bninit64.c bntest32.c bntest64.c

CLEANFILES= ${GENSRCS}

CFLAGS+= -g -Wall -W -Wshadow -Wpointer-arith
CFLAGS+= -Wmissing-prototypes -Wwrite-strings -O6 -DHAVE_CONFIG_H $(DEFINE)
CFLAGS+= -I$(.CURDIR)/obj -I$(.CURDIR)

BNLIB=	lib$(LIB).a

check: bntest
	./bntest

bntest: bntest00.o $(BNLIB)
	$(CC) $(LDFLAGS) -o $@ bntest00.o $(BNLIB) $(LIBS)

germtest: germtest.o $(BNLIB)
	$(CC) $(LDFLAGS) -o $@ germtest.o $(BNLIB) $(LIBS)

lbn80386.o: $(.CURDIR)/lbn80386.s
	$(CC) -o $@ -c $(.CURDIR)/lbn80386.s

# Extra, non-obvious dependencies.  Bnlib can be compiled in three
# word sizes, and the *00.c files #include the right .c files based
# on <limits.h>, which means that a single compilation will only use a
# subset of these files.  We put these here so that the dependencies
# are there, even if a "make depend" hasn't been done.

lbn00.o: lbn16.c lbn32.c lbn64.c lbn16.h lbn32.h lbn64.h
bn00.o: bn16.c bn32.c bn64.c bn16.h bn32.h bn64.h
bn00.o:	bninit16.c bninit32.c bninit64.c
bn00.o: lbn32.h
bntest00.o: bntest16.c bntest32.c bntest64.c lbn16.h lbn32.h lbn64.h

install:

depend: ${GENSRCS} .depend
.depend: ${CSRCS}
	${MKDEP} ${CFLAGS:M-[ID]*} ${AINC} ${.ALLSRC}
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o *:/\1.o \1.po \1.do:/' < .depend > $$TMP; \
	    mv $$TMP .depend)

.include <bsd.lib.mk>

# The 16-bit versions of the code are the master versions; all else is
# generated from them.  This fiddling about makes them unwriteable
# to discourage improper edits.

# (You didn't know that suffixes for suffix rules didn't have to begin
# with a period, did you?)
.SUFFIXES: 16.c 16.h 32.c 32.h 64.c 64.h
16.c32.c:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/64/g -e s/16/32/g $< > $@
	@chmod a-w $@

16.h32.h:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/64/g -e s/16/32/g $< > $@
	@chmod a-w $@

16.c64.c:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/128/g -e s/16/64/g $< > $@
	@chmod a-w $@

16.h64.h:
	@test ! -f $@ -o -w $@ || chmod u+w $@ && test -w $@ || rm -f $@
	sed -e s/32/128/g -e s/16/64/g $< > $@
	@chmod a-w $@
