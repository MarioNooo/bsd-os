PROG=	apmd
MAN8=	apmd.0
BINDIR=	/usr/libexec
SCRIPTS=/usr/libdata/apmd

afterinstall:
	[ -d ${SCRIPTS} ] || mkdir ${SCRIPTS}
	[ -f ${SCRIPTS}/normal_resume ] || ( \
		install -c -g wheel -o root -m 755 ${.CURDIR}/resume \
			${SCRIPTS}/normal_resume; \
		cd ${SCRIPTS}; \
		[ -f critical_resume ] || ln normal_resume critical_resume ; \
		[ -f error_resume ] || ln normal_resume error_resume ; \
	)
	[ -f ${SCRIPTS}/user_suspend_req ] || ( \
		install -c -g wheel -o root -m 755 ${.CURDIR}/suspend \
			${SCRIPTS}/user_suspend_req; \
		cd ${SCRIPTS}; \
		[ -f suspend_req ] || ln user_suspend_req suspend_req ; \
	)

.include <bsd.prog.mk>
