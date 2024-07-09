# Copyright (c) 2001 Wind River Systems, Inc. All rights reserved.
# The Wind River Systems Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#       BSDI Makefile.gnu,v 2.3 2002/04/23 09:31:15 benoitp Exp
#
# GNU Makefile: used to build and install pax in a cross development environment
#

# To install on versions prior to BSD 4.4 the following may have to be
# defined with CFLAGS +=
#
# -DNET2_STAT	Use NET2 or older stat structure. The version of the
#		stat structure is easily determined by looking at the
#		basic type of an off_t (often defined in the file:
#		/usr/include/sys/types.h). If off_t is a long (and is
#		NOT A quad) then you must define NET2_STAT.
#		This define is important, as if you do have a quad_t
#		off_t and define NET2_STAT, pax will compile but will
#		NOT RUN PROPERLY.
#
# -DNET2_FTS	Use the older NET2 fts. To identify the version,
#		examine the file: /usr/include/fts.h. If FTS_COMFOLLOW
#		is not defined then you must define NET2_FTS.
#		Pax may not compile if this not (un)defined properly.
#
# -DNET2_REGEX	Use the older regexp.h not regex.h. The regex version
#		is determined by looking at the value returned by
#		regexec() (man 3 regexec). If regexec return a 1 for
#		success (and NOT a 0 for success) you have the older
#		regex routines and must define NET2_REGEX.
#		Pax may not compile if this not (un)defined properly.

HOST_BIN	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/bin
HOST_LIB	=	${WIND_BASE}/host/${WIND_HOST_TYPE}/lib
WIND_INC	=	${WIND_BASE}/host/include

CP		=	cp -fp
CHMOD		=	chmod
CHMODFLAGS	=	0555

ifeq (${WIND_HOST_TYPE}, x86-win32)
SUFFIX		=	.exe
CFLAGS	   	+=	-I$(WIND_INC) -I$(WIND_INC)/bsdos -DNET2_STAT \
			-DBSD_TARGET -DTORNADO
UF_CFLAGS	=	-I$(WIND_INC) -I$(WIND_INC)/bsdos \
			-DBSD_TARGET -DTORNADO -DUNIXFILE
LDFLAGS		+=	-L$(HOST_LIB) -lcbsdos
UF_LDFLAGS	=	$(LDFLAGS) -lunixfile
else
SUFFIX		=
endif

SRCS		=	ar_io.c ar_subs.c buf_subs.c cache.c cpio.c \
			file_subs.c ftree.c gen_subs.c options.c pat_rep.c \
			pax.c sel_subs.c tables.c tar.c tty_subs.c
OBJS		=	$(patsubst %.c, ${WIND_HOST_TYPE}/%.o, $(SRCS))

ifeq (${WIND_HOST_TYPE}, x86-win32)
UF_OBJS 	=	$(patsubst %.c, ${WIND_HOST_TYPE}/%_uf.o, $(SRCS))
else
UF_OBJS 	=
endif

ifeq (${WIND_HOST_TYPE}, x86-win32)
PROGRAMS	=	$(HOST_BIN)/pax$(SUFFIX) $(HOST_BIN)/ufile_pax$(SUFFIX)
else
PROGRAMS	=	$(HOST_BIN)/pax$(SUFFIX)
endif

default:	$(PROGRAMS)

$(HOST_BIN)/pax$(SUFFIX): ${WIND_HOST_TYPE} $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o ${WIND_HOST_TYPE}/pax$(SUFFIX)
	$(CP) ${WIND_HOST_TYPE}/pax$(SUFFIX) $(HOST_BIN)
	$(CHMOD) $(CHMODFLAGS) $@

$(HOST_BIN)/ufile_pax$(SUFFIX): ${WIND_HOST_TYPE} $(UF_OBJS)
	$(CC) $(UF_OBJS) $(UF_LDFLAGS) \
		-o ${WIND_HOST_TYPE}/ufile_pax$(SUFFIX)
	$(CP) ${WIND_HOST_TYPE}/ufile_pax$(SUFFIX) $(HOST_BIN)
	$(CHMOD) $(CHMODFLAGS) $@

${WIND_HOST_TYPE}/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

${WIND_HOST_TYPE}/%_uf.o: %.c
	$(CC) -c $(UF_CFLAGS) -o $@ $<

${WIND_HOST_TYPE}:
	@ mkdir -p ${WIND_HOST_TYPE}

clean:
	$(RM) -rf ./${WIND_HOST_TYPE} $(HOST_BIN)/pax$(SUFFIX) \
	    $(HOST_BIN)/ufile_pax$(SUFFIX)
