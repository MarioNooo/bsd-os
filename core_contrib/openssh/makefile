#	BSDI	makefile,v 1.13 2003/09/18 21:18:46 polk Exp

all:	Makefile
	make -f Makefile

MAN1=	scp.1 sftp.1 ssh-add.1 ssh-agent.1 ssh-keygen.1 ssh-keyscan.1 ssh.1
MAN5=	ssh_config.5 sshd_config.5
MAN8=	sftp-server.8 sshd.8 ssh-keysign.8 ssh-rand-helper.8

install:
	make -f Makefile install-nokeys
	@(umask 0222; for i in ${MAN1}; do \
		echo "formatting $$i" ; \
		nroff -mandoc /usr/share/core_contrib/man/man1/$$i > \
			/usr/share/core_contrib/man/cat1/$$(basename $$i .1).0 ; \
	done)
	@(umask 0222; for i in ${MAN5}; do \
		echo "formatting $$i" ; \
		nroff -mandoc /usr/share/core_contrib/man/man5/$$i > \
			/usr/share/core_contrib/man/cat5/$$(basename $$i .5).0 ; \
	done)
	@(umask 0222; for i in ${MAN8}; do \
		echo "formatting $$i" ; \
		nroff -mandoc /usr/share/core_contrib/man/man8/$$i > \
			/usr/share/core_contrib/man/cat8/$$(basename $$i .8).0 ; \
	done)

clean:
	if [ -e Makefile ]; then \
		make -f Makefile clean ; \
	fi

cleandir: clean
	rm -f Makefile

obj objdir depend mansourceinstall:

Makefile:
	CFLAGS="-DHAVE_INET_ATON=1 -DHAVE_INET_NTOA=1 -DHAVE_INET_NTOP=1 \
		-DSETEUID_BREAKS_SETUID=1 -DBROKEN_SETREUID=1 \
		-DBROKEN_SETREGID=1 -I/usr/include/krb5/gssapi" \
	./configure \
		--with-cflags=-I/usr/include/krb5 \
		--with-cppflags=-I/usr/include/krb5 \
		--prefix=/usr \
		--with-tcp-wrappers \
		--with-bsd-auth \
		--with-kerberos5=/usr \
		--libexecdir=/usr/libexec \
		--mandir=/usr/share/core_contrib/man \
		--sysconfdir=/etc
