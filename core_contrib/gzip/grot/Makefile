# Generated automatically from Makefile.in by configure.
# Makefile for gzip (GNU zip)    -*- Indented-Text -*-
# Copyright (C) 1992-1993 Jean-loup Gailly and the Free Software Foundation

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# Makefile,v 1.1.1.2 1994/01/04 20:26:01 torek Exp

#### Start of system configuration section. ####


srcdir = .
VPATH = .

CC = gcc
ASCPP = gcc -E

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = $(INSTALL) -m 555 -o bin -g bin
INSTALL_DATA = $(INSTALL) -m 444 -o bin -g bin

# Things you might add to DEFS (configure usually figures out what to do):
# -DGNU_STANDARD        Behave as gzip even if invoked as gunzip (GNU standard)
# -DDIRENT              Use <dirent.h>  for recursion (-r)
# -DSYSDIR              Use <sys/dir.h> for recursion (-r)
# -DSYSNDIR             Use <sys/ndir.h> for recursion (-r)
# -DNDIR                Use <ndir.h> for recursion (-r)
# -DSTDC_HEADERS        Use <stdlib.h>
# -DHAVE_UNISTD_H	Use <unistd.h>
# -DNO_FCNTL_H          Don't use <fcntl.h>
# -DNO_UTIME_H		Don't use <utime.h>
# -DHAVE_SYSUTIME_H	Use <sys/utime.h>
# -DNO_MEMORY_H         Don't use <memory.h>. Not needed if STDC_HEADERS.
# -DNO_STRING_H         Use strings.h, not string.h. Not needed if STDC_HEADERS
# -DRETSIGTYPE=int      Define this if signal handlers must return an int.
# -DNO_SYMLINK          OS defines S_IFLNK but does not support symbolic links
# -DNO_MULTIPLE_DOTS    System does not allow file names with multiple dots
# -DNO_UTIME		System does not support setting file modification time
# -DNO_CHOWN		System does not support setting file owner
# -DNO_DIR		System does not support readdir()
# -DPROTO		Force function prototypes even if __STDC__ not defined
# -DASMV		Use asm version match.S
# -DMSDOS		MSDOS specific
# -DOS2			OS/2 specific
# -DVAXC		Vax/VMS with Vax C compiler
# -DVMS			Vax/VMS with gcc
# -DDEBUG		Debug code
# -DDYN_ALLOC		Use dynamic allocation of large data structures
# -DMAXSEG_64K		Maximum array size is 64K (for 16 bit system)
# -DRECORD_IO           read() and write() are rounded to record sizes.
# -DNO_STDIN_FSTAT      fstat() is not available on stdin
# -DNO_FSTAT            fstat() is not available
# -DNO_SIZE_CHECK       stat() does not give a reliable file size

DEFS =  -DASMV -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1
LIBS = 

# additional assembly sources for particular systems may be required.
OBJA = match.o

SEDCMD = 1d

CFLAGS = -O
# If you want debug on by default, use: CFLAGS="-g" ./configure
LDFLAGS = $(CFLAGS)

G=g
# To install znew, zmore, etc... as gznew, gzmore... use: G=g

ZCAT=zcat
# To install zcat executable and man page as gzcat, use: ZCAT=gzcat

X=
# For OS/2 or MSDOS, use: X=.exe

O=.o
# For OS/2 or MSDOS, use: O=.obj

prefix = /usr/contrib
exec_prefix = ${prefix}

bindir = $(exec_prefix)/bin
scriptdir = $(bindir)
# scriptdir is the directory in which shell scripts should be installed
datadir = $(prefix)/lib
libdir = $(prefix)/lib
infodir = $(prefix)/info

# Extension (not including `.') for the installed manual page filenames.
manext = 1
# Where to install the manual pages.
mandir = $(prefix)/man/man$(manext)
# Use manlinks=so to use the .so method instead of hard links
manlinks = ln

alldirs = $(bindir) $(scriptdir) $(datadir) $(libdir) $(infodir) $(mandir)

#### End of system configuration section. ####

SHELL = /bin/sh

LOADLIBES = $(LIBS)

TAR = tar

SRCS = gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c crypt.c\
       lzw.c unlzw.c unpack.c unlzh.c getopt.c match.S

OBJS = gzip$O zip$O deflate$O trees$O bits$O unzip$O inflate$O util$O \
       crypt$O lzw$O unlzw$O unpack$O unlzh$O getopt$O $(OBJA)

HDRS = gzip.h lzw.h tailor.h revision.h crypt.h getopt.h

GENFILES =  README NEWS INSTALL Makefile.in configure.in configure COPYING \
  TODO THANKS ChangeLog $(SRCS) $(HDRS) zmore.in znew.in zdiff.in zgrep.in \
  zforce.in gzexe.in gzip.1 zdiff.1 zgrep.1 zmore.1 znew.1 gzexe.1 zforce.1 \
  gzip.doc algorithm.doc gzip.texi texinfo.tex gpl.texinfo gzip.info

sampleFILES =  sample/makecrc.c sample/zread.c sample/add.c sample/sub.c \
       sample/ztouch sample/zfile

msdosFILES = msdos/tailor.c msdos/match.asm msdos/gzip.prj msdos/doturboc.bat \
  msdos/Makefile.msc msdos/Makefile.bor msdos/Makefile.djg

os2FILES = os2/Makefile.os2 os2/gzip.def os2/gzip16.def

ntFILES = nt/Makefile.nt

vmsFILES = vms/Readme.vms vms/Makefile.vms vms/Makefile.gcc vms/makegzip.com \
   vms/Makefile.mms vms/vms.c vms/gzip.hlp

amigaFILES = amiga/Makefile.sasc amiga/Makefile.gcc amiga/tailor.c \
  amiga/utime.h amiga/match.a

atariFILES = atari/Makefile.st

primosFILES = primos/readme primos/primos.c primos/ci.opts \
   primos/build.cpl primos/include/errno.h primos/include/fcntl.h \
   primos/include/stdlib.h primos/include/sysStat.h primos/include/sysTypes.h

DISTFILES = $(GENFILES) $(sampleFILES) $(msdosFILES) $(os2FILES) $(ntFILES)\
   $(vmsFILES) $(amigaFILES) $(atariFILES) $(primosFILES)

SCRIPTS = $(G)zdiff $(G)zgrep $(G)zmore $(G)znew $(G)zforce gzexe

.c$O:
	$(CC) -c $(DEFS) $(CFLAGS) $<

#.PHONY: default all force test check


all:	gzip$X $(G)zdiff $(G)zgrep $(G)zmore $(G)znew $(G)zforce gzexe


obj objdir cleandir depend:
	@echo make target not supported: $@  

force:

#### Start of specific targets section. ####
#
# 'configure' works only on Unix systems. For other systems able to make
# sense of this makefile, you can define target specific entries here.
# For other systems such as MSDOS, separate Makefiles are
# provided in subdirectories.

# NeXT 2.x, 3.0, 3.1 thin. For gcc, replace -bsd with -D__STRICT_BSD__.
next:
	$(MAKE) all CFLAGS="-O -bsd -DASMV" \
	DEFS="-DNO_STDLIB_H -DNO_STRING_H -DNO_UTIME_H -DSYSDIR -DRETSIGTYPE=int"
 
# NeXT 3.1 fat (68k + 386). For gcc, replace -bsd with -D__STRICT_BSD__.
next-fat:
	$(MAKE) all OBJA=match-next.o \
	CFLAGS="-O2 -bsd -DASMV -fno-builtin -arch m68k -arch i386" \
	DEFS="-DNO_STDLIB_H -DNO_STRING_H -DNO_UTIME_H -DSYSDIR -DRETSIGTYPE=int"

match-next.o: match.S
	cat $(srcdir)/match.S > match-next.s
	$(CC) -arch m68k -arch i386 -c match-next.s
	rm -f match-next.s

# gcc with emx 0.8f kit (use by preference os2/Makefile.os2)
os2_gcc:
	$(MAKE) all CC=gcc CFLAGS="-O -DOS2" X=".exe"

# Xenix 2.3.2 for 286:
xenix_286:
	$(MAKE) all CFLAGS="-LARGE -M2l"

# Coherent (with broken /bin/sh):
coherent:
	$(MAKE) all OBJA=match.o DEFS=\
	"-DASMV -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1"

#### End of specific targets section. ####

install: installdirs installbin installman

installbin: all
	$(INSTALL_PROGRAM) gzip$X $(bindir)/gzip$X
	for f in $(SCRIPTS); do \
	  $(INSTALL_PROGRAM) $${f} $(scriptdir)/$${f}; done
	rm -f $(scriptdir)/$(G)zcmp; \
	  ln $(scriptdir)/$(G)zdiff $(scriptdir)/$(G)zcmp
	for f in gunzip$X ungzip$X $(ZCAT)$X ; do \
	  rm -f $(bindir)/$${f}; done
	@if echo $(DEFS) | grep GNU_STANDARD > /dev/null; then \
	  echo 'exec gzip -d  $${1+"$$@"}' > $(bindir)/gunzip$X; \
	  echo 'exec gzip -dc $${1+"$$@"}' > $(bindir)/$(ZCAT)$X; \
	  chmod 755 $(bindir)/gunzip$X  $(bindir)/$(ZCAT)$X; \
	else \
	  ln $(bindir)/gzip$X $(bindir)/gunzip$X; \
	  ln $(bindir)/gzip$X $(bindir)/$(ZCAT)$X; \
	fi

installman: gzip.info
	for f in gzip gunzip $(ZCAT) $(SCRIPTS) $(G)zcmp; do \
	  rm -f $(mandir)/$${f}.$(manext); done
	-cd $(srcdir); for f in gzip gzexe; do \
	  $(INSTALL_DATA) $${f}.1 $(mandir)/$${f}.$(manext); done
	-cd $(srcdir); for f in zdiff zgrep zmore znew zforce; do \
	  $(INSTALL_DATA) $${f}.1 $(mandir)/$(G)$${f}.$(manext); done
	-cd $(mandir); if test $(manlinks) = so; then \
	  echo .so man$(manext)/gzip.$(manext)      > $(ZCAT).$(manext);\
	  echo .so man$(manext)/$(G)zdiff.$(manext) > $(G)zcmp.$(manext);\
	  echo .so man$(manext)/gzip.$(manext)      > gunzip.$(manext);\
	  chmod 644 $(ZCAT).$(manext) $(G)zcmp.$(manext) gunzip.$(manext);\
	else \
	  ln gzip.$(manext)  $(ZCAT).$(manext);\
	  ln $(G)zdiff.$(manext) $(G)zcmp.$(manext);\
	  ln gzip.$(manext)  gunzip.$(manext);\
	fi
	-cd $(srcdir); for f in gzip.info* ; do $(INSTALL_DATA) $${f} \
	  $(infodir)/$${f}; done

uninstall: force
	-cd $(bindir); rm -f gzip$X gunzip$X $(ZCAT)$X
	-cd $(scriptdir); rm -f $(SCRIPTS) $(G)zcmp
	-for f in gzip gunzip $(ZCAT) $(SCRIPTS) $(G)zcmp; do \
	  rm -f $(mandir)/$${f}.$(manext); done
	-cd $(infodir); rm -f gzip.info*

# install all files and replace compress (not recommended)
install_compress: install
	-test -f $(bindir)/compress.old || \
	  mv $(bindir)/compress$X $(bindir)/compress.old
	ln $(bindir)/gzip$X $(bindir)/compress$X
	rm -f $(bindir)/uncompress$X
	ln $(bindir)/gzip$X $(bindir)/uncompress$X

# Make sure all installation directories, e.g. $(bindir) actually exist by
# making them if necessary. At most one level is created (except for man).
installdirs:
	-if test ! -d $(prefix)/man; then \
	   mkdir $(prefix)/man; fi
	-for dir in $(alldirs) ; do \
	   if test ! -d $${dir}; then \
	     mkdir $${dir}; fi; \
	done

test: check
check:	gzip$X
	./gzip -6 < $(srcdir)/texinfo.tex > _gztest.gz
	@LANG=""; export LANG; if test `wc -c < _gztest.gz` -eq 30890; then \
	   true; \
	else \
	   echo FAILED gzip test: incorrect size; \
	fi
	rm -f _gztest
	./gzip -d _gztest.gz
	@if cmp _gztest $(srcdir)/texinfo.tex; then \
	   echo gzip test OK; \
	else \
	   echo FAILED gzip test: incorrect decompress; \
	fi
	rm -f _gztest*

TAGS: $(SRCS) $(HDRS)
	cd $(srcdir); etags $(SRCS) $(HDRS)

Makefile: Makefile.in ./config.status
	./config.status

./config.status: configure
	$(srcdir)/configure --srcdir=$(srcdir) --no-create

configure: configure.in
	@echo Warning: configure is out of date
#	cd $(srcdir); autoconf

clean:
	rm -f *$O gzip$X gunzip$X ungzip$X $(ZCAT)$X add$X sub$X a.out core
	rm -f $(G)zcmp $(SCRIPTS) _gztest*
	rm -f *.aux *.cp *.cps *.dvi *.fn *.fns *.ky *.kys *.log
	rm -f *.pg *.pgs *.toc *.tp *.tps *.vr *.vrs

mostlyclean: clean

distclean: clean
	rm -f Makefile config.status

#realclean: distclean
#	rm -f TAGS gzip.info* gzip.doc

dist: $(DISTFILES) Makefile
	d=gzip-`sed -e '/VERSION/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
	            -e q revision.h` ; \
	rm -f ../$$d; \
	ln -s `pwd` ../$$d; \
	cd ..; \
	files=""; \
	for f in $(DISTFILES); do files="$$files $$d/$$f"; done; \
	GZIP=-9 $(TAR) chofz $$d/$$d.tar.gz $$files; \
	rm -f $$d

zipdist: $(DISTFILES) Makefile
	zip -u9T gzip`sed -e '/VERSION/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
			  -e s/[.]//g -e q revision.h` $(DISTFILES)

# Actual build-related targets

gzip$X:	Makefile $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	rm -f gunzip$X $(ZCAT)$X
	ln gzip$X gunzip$X
	ln gzip$X $(ZCAT)$X

gzip$O zip$O deflate$O trees$O bits$O unzip$O inflate$O: gzip.h tailor.h
util$O lzw$O unlzw$O unpack$O unlzh$O crypt$O: gzip.h tailor.h

gzip$O unlzw$O: revision.h lzw.h

bits$O unzip$O util$O zip$O: crypt.h

gzip$O getopt$O: getopt.h

match$O: match.S
	$(ASCPP) $(srcdir)/match.S > _match.s
	$(CC) -c _match.s
	mv _match$O match$O
	rm -f _match.s

$(G)zdiff: zdiff.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/zdiff.in > $@
	chmod 755 $@

$(G)zgrep: zgrep.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/zgrep.in > $@
	chmod 755 $@

$(G)zmore: zmore.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/zmore.in > $@
	chmod 755 $@

$(G)znew: znew.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/znew.in > $@
	chmod 755 $@

$(G)zforce: zforce.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/zforce.in > $@
	chmod 755 $@

gzexe: gzexe.in
	sed -e "$(SEDCMD)" -e "s|BINDIR|$(bindir)|" $(srcdir)/gzexe.in > $@
	chmod 755 $@

gzip.info: gzip.texi
	cd $(srcdir); makeinfo gzip.texi

gzip.dvi: gzip.texi
	cd $(srcdir); texi2dvi gzip.texi

gzip.doc: gzip.1
	nroff -man $(srcdir)/gzip.1 | col -b | uniq > gzip.doc

# Prevent GNU make v3 from overflowing arg limit on SysV.
.NOEXPORT:

# end of file
