#	BSDI	Makefile,v 2.14 2000/03/08 17:12:40 jch Exp
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN5=	a.out.0 acct.0 ar.0 auxv.0 boot.define.0 cat.0 core.0 dir.0 \
	disktab.0 disklabel.0 elf.0 fs.0 fstab.0 \
	group.0 hosts.0 hosts.equiv.0 login.conf.0 networks.0 passwd.0 \
	pccard.conf.0 pfkeys.0 phones.0 printcap.0 protocols.0 ps_strings.0 \
	remote.0 services.0 shells.0 types.0 utmp.0
MLINKS+=fs.5 inode.5
MLINKS+=utmp.5 wtmp.5 utmp.5 lastlog.5
MLINKS+=dir.5 dirent.5

all: ${MAN5}

clean depend lint tags:

cleandir:
	rm -f ${MAN5}

install: maninstall afterinstall

afterinstall:
	rm -f ${DESTDIR}${MANDIR}5/rhosts.0
	ln ${DESTDIR}${MANDIR}5/hosts.equiv.0 \
		${DESTDIR}${MANDIR}5/rhosts.0

.include <bsd.prog.mk>
