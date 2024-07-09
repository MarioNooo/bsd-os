#	BSDI	$Id$

# Determine OS version and set variables accordingly.
OSVER!=uname -r | sed 's/\..*//'
.if ${OSVER} <= 4
OPENSSLDIR=	/usr/contrib/lib/openssl
PREFIX=	/usr/contrib
MANDIR=	${PREFIX}/man
.else
OPENSSLDIR=	/etc/openssl
PREFIX=	/usr
MANDIR=	/usr/share/core_contrib/man
.endif
INCLUDEDIR=${PREFIX}/include/openssl

CLEANFILES=test/ideatest.c test/rc5test.c \
	apps/CA.pl \
	apps/der_chop \
	crypto/opensslconf.h \
	tools/c_rehash

all:	Makefile
	make -f Makefile

MAN1=	CA.pl.1 asn1parse.1 ca.1 ciphers.1 crl.1 crl2pkcs7.1 dgst.1 \
	dhparam.1 dsa.1 dsaparam.1 enc.1 gendsa.1 genrsa.1 nseq.1 \
	openssl.1 passwd.1 pkcs12.1 pkcs7.1 pkcs8.1 rand.1 req.1 \
	rsa.1 s_client.1 s_server.1 sess_id.1 smime.1 speed.1 spkac.1 \
	verify.1 version.1 x509.1
MAN3=	BN_CTX_new.3 BN_CTX_start.3 BN_add.3 BN_add_word.3 BN_bn2bin.3 \
	BN_cmp.3 BN_copy.3 BN_generate_prime.3 BN_mod_inverse.3 \
	BN_mod_mul_montgomery.3 BN_mod_mul_reciprocal.3 BN_new.3 \
	BN_num_bytes.3 BN_rand.3 BN_set_bit.3 BN_zero.3 CRYPTO_set_ex_data.3 \
	DH_generate_key.3 DH_generate_parameters.3 DH_get_ex_new_index.3 \
	DH_new.3 DH_set_method.3 DH_size.3 DSA_SIG_new.3 DSA_do_sign.3 \
	DSA_dup_DH.3 DSA_generate_key.3 DSA_generate_parameters.3 \
	DSA_get_ex_new_index.3 DSA_new.3 DSA_set_method.3 DSA_sign.3 \
	DSA_size.3 ERR_GET_LIB.3 ERR_clear_error.3 ERR_error_string.3 \
	ERR_get_error.3 ERR_load_crypto_strings.3 ERR_load_strings.3 \
	ERR_print_errors.3 ERR_put_error.3 ERR_remove_state.3 \
	EVP_DigestInit.3 EVP_EncryptInit.3 EVP_OpenInit.3 EVP_SealInit.3 \
	EVP_SignInit.3 EVP_VerifyInit.3 OPENSSL_VERSION_NUMBER.3 \
	OpenSSL_add_all_algorithms.3 RAND_add.3 RAND_bytes.3 RAND_cleanup.3 \
	RAND_egd.3 RAND_load_file.3 RAND_set_rand_method.3 RSA_blinding_on.3 \
	RSA_check_key.3 RSA_generate_key.3 RSA_get_ex_new_index.3 \
	RSA_new.3 RSA_padding_add_PKCS1_type_1.3 RSA_print.3 \
	RSA_private_encrypt.3 RSA_public_encrypt.3 RSA_set_method.3 \
	RSA_sign.3 RSA_sign_ASN1_OCTET_STRING.3 RSA_size.3 SSL_get_error.3 \
	blowfish.3 bn.3 bn_internal.3 buffer.3 crypto.3 d2i_DHparams.3 \
	d2i_RSAPublicKey.3 des.3 dh.3 dsa.3 err.3 hmac.3 lh_stats.3 \
	lhash.3 md5.3 mdc2.3 rand.3 rc4.3 ripemd.3 rsa.3 sha.3 ssl.3 \
	threads.3
MAN5=	config.5

install:
	-mkdir ${OPENSSLDIR}
.if ${OSVER} <= 4
	-ln -sf ../../man ${OPENSSLDIR}/man
	-ln -sf ../../bin ${OPENSSLDIR}/misc
.else
	-ln -sf ../..${MANDIR} ${OPENSSLDIR}/man
	-rm -f /usr/bin/openssl
.endif
	make -f Makefile install
.if ${OSVER} >= 4
	-rm -f ${OPENSSLDIR}/man
.endif
	@(umask 0222; for i in ${MAN1}; do \
		echo "formatting $$i" ; \
		nroff -mandoc ${MANDIR}/man1/$$i > \
			${MANDIR}/cat1/$$(basename $$i .1).0 ; \
	done)
	@(umask 0222; for i in ${MAN3}; do \
		echo "formatting $$i" ; \
		nroff -mandoc ${MANDIR}/man3/$$i > \
			${MANDIR}/cat3/$$(basename $$i .3).0 ; \
	done)
	@(umask 0222; for i in ${MAN5}; do \
		echo "formatting $$i" ; \
		nroff -mandoc ${MANDIR}/man5/$$i > \
			${MANDIR}/cat5/$$(basename $$i .5).0 ; \
	done)

clean:
	if [ -e Makefile ]; then \
		make -f Makefile clean ; \
	fi

cleandir: clean
	rm -f Makefile ${CLEANFILES}

obj objdir mansourceinstall:

depend:	Makefile

Makefile:
	# Add dummy files to test removed algorithms
	rm -f test/ideatest.c test/rc5test.c
	echo "int main(int argc, char *argv[]) { exit (0); }" > test/ideatest.c
	ln test/ideatest.c test/rc5test.c
	chmod u+w Makefile.ssl
.if $(MACHINE) == "powerpc"
	PERL=/usr/bin/perl \
	./config --prefix=${PREFIX} --openssldir=${OPENSSLDIR} \
		no-rc5 no-idea threads no-asm
	make -f Makefile depend
.else
	PERL=/usr/bin/perl \
	./config --prefix=${PREFIX} --openssldir=${OPENSSLDIR} \
		no-rc5 no-idea threads
	make -f Makefile depend
.endif
	mv Makefile Makefile.old
	sed -e '/^DIR/s/test//' < Makefile.old > Makefile
