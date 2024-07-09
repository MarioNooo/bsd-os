CC=shlicc
CFLAGS+=-O2 -DPROTOTYPES=1 -I.
CFLAGS+=-DBSD44 -DANYPORT -DSHORTLINKS

BINDIR=/usr/contrib/bin
ETCDIR=/usr/contrib/libexec
MANDIR=/usr/contrib/man/cat

COMPAT=-lcompat
RPCOPTS=

MAN1=cattach.0 cdetach.0 cmkdir.0 cfssh.0 cpasswd.0 cmkkey.0
MAN8=cfsd.0 cname.0 ccat.0

OBJS= cfs.o nfsproto_xdr.o nfsproto_svr.o admproto_xdr.o admproto_svr.o \
  cfs_adm.o cfs_nfs.o cfs_fh.o cfs_des.o cfs_cipher.o adm.o ver.o mcgsbox.o \
  mcg.o safer.o cfs_bfenc.o cfs_bfsk.o
EOBJS=dhparams.o truerand.o esm_gen.o esm.o esm_cipher.o
COBJS=admproto_clnt.o cfs_des.o cfs_cipher.o cattach.o getpass.o cmkdir.o \
  cdetach.o ver.o cname.o ccat.o mcgsbox.o mcgsbox.o mcg.o shs.o cpasswd.o \
  truerand.o safer.o
OTHERS = nfsproto.h nfsproto_svr.c nfsproto_xdr.c admproto.h admproto_svr.c \
  admproto_xdr.c admproto_clnt.c

all: cfsd cattach cmkdir cdetach cname ccat cpasswd cfssh manpages
	@echo

cfsd: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o cfsd

cattach: cattach.o admproto_clnt.o admproto_xdr.o getpass.o cfs_des.o \
  cfs_cipher.o adm.o ver.o mcg.o mcgsbox.o shs.o safer.o cfs_bfenc.o cfs_bfsk.o 
	$(CC) cattach.o admproto_clnt.o admproto_xdr.o cfs_des.o \
	   cfs_cipher.o getpass.o adm.o ver.o mcg.o mcgsbox.o \
	   shs.o safer.o  cfs_bfenc.o cfs_bfsk.o $(COMPAT) $(LIBS) -o cattach

cdetach: cdetach.o admproto_clnt.o admproto_xdr.o adm.o ver.o
	$(CC) cdetach.o adm.o admproto_clnt.o admproto_xdr.o \
	   ver.o $(LIBS) -o cdetach

cmkdir: getpass.o adm.o cfs_des.o cfs_cipher.o cmkdir.o ver.o mcg.o \
   mcgsbox.o safer.o shs.o truerand.o cfs_bfenc.o cfs_bfsk.o 
	$(CC) cmkdir.o cfs_des.o cfs_cipher.o getpass.o adm.o ver.o mcg.o \
	   mcgsbox.o safer.o shs.o truerand.o  cfs_bfenc.o cfs_bfsk.o  $(COMPAT) -o cmkdir

cpasswd: getpass.o cfs_des.o cfs_cipher.o cpasswd.o ver.o mcg.o \
   mcgsbox.o safer.o shs.o truerand.o  cfs_bfenc.o cfs_bfsk.o
	$(CC) cpasswd.o cfs_des.o cfs_cipher.o getpass.o ver.o mcg.o \
	   mcgsbox.o safer.o shs.o truerand.o  cfs_bfenc.o cfs_bfsk.o $(COMPAT) -o cpasswd

cname: cname.o getpass.o cfs_des.o cfs_cipher.o cfs_adm.o cfs_fh.o \
   cfs_nfs.o ver.o mcg.o mcgsbox.o safer.o shs.o cfs_bfenc.o cfs_bfsk.o 
	$(CC) cname.o getpass.o cfs_des.o cfs_cipher.o cfs_adm.o cfs_fh.o \
	   cfs_nfs.o ver.o mcg.o mcgsbox.o safer.o shs.o cfs_bfenc.o cfs_bfsk.o\
           $(LIBS) $(COMPAT) -o cname

ccat: ccat.o getpass.o cfs_des.o cfs_cipher.o cfs_adm.o cfs_fh.o cfs_nfs.o \
   ver.o mcg.o mcgsbox.o shs.o safer.o cfs_bfenc.o cfs_bfsk.o
	$(CC) ccat.o getpass.o cfs_des.o cfs_cipher.o cfs_adm.o cfs_fh.o \
	   cfs_nfs.o ver.o mcg.o mcgsbox.o shs.o safer.o cfs_bfenc.o cfs_bfsk.o\
           $(LIBS) $(COMPAT) -o ccat

cfssh:
	ln -s ${.CURDIR}/cfssh

$(OBJS): nfsproto.h admproto.h cfs.h mcg.h safer.h shs.h

$(COBJS): nfsproto.h admproto.h cfs.h mcg.h safer.h shs.h

# truerand is a special case, no -O
truerand.o:
	$(CC) -c ${.CURDIR}/truerand.c

nfsproto_xdr.c: ${.CURDIR}/nfsproto.x
	rpcgen $(RPCOPTS) -c -o nfsproto_xdr.c ${.CURDIR}/nfsproto.x 

nfsproto_svr.c: ${.CURDIR}/nfsproto.x
	rpcgen $(RPCOPTS) -m -o nfsproto_svr.c ${.CURDIR}/nfsproto.x 

nfsproto.h: ${.CURDIR}/nfsproto.x
	rpcgen $(RPCOPTS) -h -o nfsproto.h ${.CURDIR}/nfsproto.x

admproto_xdr.c: ${.CURDIR}/admproto.x
	rpcgen $(RPCOPTS) -c -o admproto_xdr.c ${.CURDIR}/admproto.x 

admproto_svr.c: ${.CURDIR}/admproto.x
	rpcgen $(RPCOPTS) -m -o admproto_svr.c ${.CURDIR}/admproto.x 

admproto.h: ${.CURDIR}/admproto.x
	rpcgen $(RPCOPTS) -h -o admproto.h ${.CURDIR}/admproto.x

admproto_clnt.c: ${.CURDIR}/admproto.x
	rpcgen $(RPCOPTS) -l -o admproto_clnt.c ${.CURDIR}/admproto.x 

clean cleandir:
	rm -f $(OBJS) $(COBJS) $(OTHERS)
	rm -f cfsd cmkdir cattach cpassed cdetach cname ccat

install: cfsd cattach cdetach cmkdir
	install -m 0755 -c -o root cfsd ${DESTDIR}${ETCDIR}
	install -m 0755 -c -o root cattach cdetach cmkdir cpasswd cfssh \
                cname ccat ${.CURDIR}/cmkkey ${DESTDIR}${BINDIR}
	(cd ${.CURDIR}; ${MAKE} maninstall)

.include <bsd.prog.mk>
