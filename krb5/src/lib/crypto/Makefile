LIB=k5crypto
LDDYNMAJOR=.3
LDDYNMINOR=.0

.PATH:	${.CURDIR}/crc32 ${.CURDIR}/des ${.CURDIR}/dk \
	${.CURDIR}/enc_provider ${.CURDIR}/hash_provider \
	${.CURDIR}/keyhash_provider ${.CURDIR}/md4 \
	${.CURDIR}/md5 ${.CURDIR}/old \
	${.CURDIR}/raw ${.CURDIR}/sha1

CFLAGS+= -I${.CURDIR} -I${DESTDIR}/usr/include/krb5 \
	-I${.CURDIR}/../../util/et -I${.CURDIR}/../../include/krb5 \
	-I${.CURDIR}/../../util/profile -I${.CURDIR}/../../util/profile/obj \
	-I${.CURDIR}/../../include -I${.CURDIR}/../../include/obj \
	-I${.CURDIR}/crc32 -I${.CURDIR}/des -I${.CURDIR}/dk \
	-I${.CURDIR}/enc_provider -I${.CURDIR}/hash_provider \
	-I${.CURDIR}/keyhash_provider -I${.CURDIR}/md4 \
	-I${.CURDIR}/md5 -I${.CURDIR}/old \
	-I${.CURDIR}/raw -I${.CURDIR}/sha1

# sources in various subdirectories
SRCS =	crc32.c
SRCS+=	afsstring2key.c d3_cbc.c d3_kysched.c f_cbc.c f_cksum.c \
        f_parity.c f_sched.c f_tables.c key_sched.c string2key.c \
        weak_key.c
SRCS+=	checksum.c dk_decrypt.c dk_encrypt.c derive.c stringtokey.c
SRCS+= 	des.c des3.c
SRCS+=	hash_crc32.c hash_md4.c hash_md5.c hash_sha1.c
SRCS+=	descbc.c k5_md4des.c k5_md5des.c
SRCS+=	md4.c
SRCS+=	md5.c
SRCS+=	des_stringtokey.c old_decrypt.c old_encrypt.c
SRCS+=	raw_decrypt.c raw_encrypt.c
SRCS+=	shs.c
# sources in this directories
SRCS+=	block_size.c checksum_length.c cksumtype_to_string.c \
	cksumtypes.c coll_proof_cksum.c crypto_libinit.c decrypt.c \
	encrypt.c encrypt_length.c enctype_compare.c enctype_to_string.c \
	etypes.c hmac.c keyed_cksum.c keyed_checksum_types.c \
	make_checksum.c make_random_key.c nfold.c old_api_glue.c \
	prng.c string_to_cksumtype.c string_to_enctype.c string_to_key.c \
	valid_cksumtype.c valid_enctype.c verify_checksum.c

.include <bsd.lib.mk>
