#
# Copyright 1992, 1993, 1994, 1995, 1996, 1999, 2000 Rodney Ruddock
# Copyright (c) 1992, 1993
#	The Regents of the University of California.  All rights reserved.
#
# This code is software by Rodney Ruddock.
# E-mail: rodney@bond.net
# Web: http://www.bond.net/~rodney/ed.html
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#       Rodney Ruddock, rodney@bond.net
#

# by default we always ask the compiler for optimized code ("-O")
# and add the X/Open "_ALL_SOURCE" to help some systems for "historical"
# items to be picked up from header files.
CFLAGS = -O -D_ALL_SOURCE -I.

# Now choose either BSD or POSIX behavior. See the ed(1) man page for
# the differences between the two. The differences are minor. BSD behavior
# that does not conflict with POSIX is still available under POSIX.
CFLAGS+= -DBSD
#CFLAGS+= -DPOSIX

# There are 3 types of buffers that 'ed' can use. The default is to use
# MEMORY which is the fastest (and actually uses fewer resources).
# STDIO uses a temporary stdio file. DBI uses the 4.4BSD db(3) interfaces.
# The DBI was written again the 1.85 release of db(3). Since the buffer
# is recreated each time and (according to the db FAQ) later versions
# should be compatible with this code (last known up to 3.X). Be aware
# that post-1.85 version db licencing is more restrictive.
CFLAGS+= -DMEMORY
#CFLAGS+= -DSTDIO
#CFLAGS+= -DDBI

# if you use the db api's be sure to link to it. Yes, link to the
# static libc; ed's footprint is small enough.
LIBRARIES = 
#LIBRARIES = -ldb
LIBC=	/lib/libc.a

SRCS=	main.c a.c add_line.c address.c bang.c c.c d.c e.c edhup.c equal.c \
 	f.c filename.c g.c get_line.c get_pattern.c \
	i.c input_lines.c j.c k.c l.c line_number.c m.c \
	p.c q.c r.c re.c rol.c sub.c search.c t.c u.c w.c z.c tag.c
        
OBJS=	main.o a.o add_line.o address.o bang.o c.o d.o e.o edhup.o equal.o \
 	f.o filename.o g.o get_line.o get_pattern.o \
	i.o input_lines.o j.o k.o l.o line_number.o m.o \
	p.o q.o r.o re.o rol.o sub.o search.o t.o u.o w.o z.o tag.o

MAN=	ed.1
CC=	cc

all: ed

ed: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBRARIES}

clean:
	rm -f ${OBJS} core ed

cleandir: clean
	rm -f ${MAN} tags .depend

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: ${MAN}
	install -s -o bin -g bin -m 755 ed ${DESTDIR}/bin
	install -c -o bin -g bin -m 444 ed.1 ${DESTDIR}/usr/share/man/man1

bundle:
	tar cvf ed.tar README ed.1 ed.h extern.h *.c
	gzip ed.tar

lint: ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

a.o: a.c ed.h main.c input_lines.c
add_line.o: add_line.c main.c ed.h
address.o: address.c ed.h main.c
bang.o: bang.c main.c ed.h
c.o: c.c ed.h main.c a.c d.c
d.o: d.c ed.h main.c k.c
e.o: e.c main.c d.c ed.h filename.c
edhup.o: edhup.c main.c ed.h w.c
equal.o: equal.c ed.h main.c line_number.c
f.o: f.c main.c filename.c
filename.o: main.c ed.h filename.c
g.o: g.c ed.h main.c re.c get_pattern.c
get_line.o: get_line.c main.c ed.h
get_pattern.o: get_pattern.c main.c ed.h
i.o: i.c a.c main.c ed.h input_lines.c
input_lines.o: input_lines.c ed.h main.c add_line.c
j.o: j.c main.c ed.h get_line.c d.c add_line.c
k.o: k.c main.c ed.h
l.o: l.c main.c ed.h get_line.c
line_number.o: line_number.c ed.h main.c
m.o: m.c main.c ed.h address.c
main.o: main.c ed.h address.c
p.o: p.c main.c ed.h get_line.c
q.o: q.c main.c ed.h
r.o: r.c main.c ed.h input_lines.c filename.c
re.o: re.c main.c ed.h
rol.o: rol.c main.c ed.h
sub.o: sub.c main.c ed.h re.c add_line.c get_line.c k.c
search.o: search.c re.c main.c ed.h get_pattern.c
t.o: add_line.c t.c main.c ed.h get_line.c address.c
u.o: u.c main.c ed.h
w.o: w.c main.c ed.h get_line.c filename.c w.c
z.o: z.c main.c ed.h get_line.c
tag.o: main.c
