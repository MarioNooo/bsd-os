#       BSDI bsd.submake.mk,v 2.3 1999/11/02 14:48:21 jch Exp 
#

# hand off building of targets to sub makefiles
#
# SUBMAKE specifies the list of sub makefiles
#
.if defined(SUBMAKE)
.MAIN: all

_SUBMAKEUSE: .USE .MAKE ${SUBMAKE}
.for _make in ${SUBMAKE}
	cd ${.CURDIR}; \
	${MAKE} ${MAKEFLAGS} -f ${_make} ${.TARGET:S/real//};
.endfor

#
# if we are used by the inherit makefile, these sub makefiles are generated.
# so if we are doing a cleandir, we do not bother to create them just so we 
# can do do a cleandir, so sometimes they do not exist (like just after a 
# cleandir!).
_SUBMAKECLEANUSE: .USE .MAKE
.for _make in ${SUBMAKE}
.if exists(${_make})
	cd ${.CURDIR}; \
	${MAKE} ${MAKEFLAGS} -f ${_make} ${.TARGET:S/real//};
.endif
.endfor

.if !target(all)
all: _SUBMAKEUSE
.endif

.if !target(clean)
realclean: _SUBMAKEUSE
clean: realclean
.if defined(CLEANFILES)
	rm -f ${CLEANFILES}
.endif
.endif


.if !target(cleandir)
realcleandir: _SUBMAKECLEANUSE
cleandir: realcleandir
.if defined(CLEANFILES)
	rm -f ${CLEANFILES}
.endif
.if defined(CLEANDIRFILES)
	rm -f ${CLEANDIRFILES}
.endif
.endif

.if !target(depend)
depend: _SUBMAKEUSE
.endif

.if !target(manpages)
manpages: _SUBMAKEUSE
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif
.if !target(afterinstall)
afterinstall:
.endif
install: afterinstall
afterinstall: realinstall
realinstall: beforeinstall _SUBMAKEUSE
.endif

.if !target(maninstall)
maninstall: _SUBMAKEUSE
.endif

.if !target(lint)
lint: _SUBMAKEUSE
.endif

.if !target(tags)
tags: _SUBMAKEUSE
.endif

.if !target(mansourceinstall)
mansourceinstall: _SUBMAKEUSE
.endif

OBJDIR?=/usr/obj
STRIPDIR?=/usr/src

.if !target(obj)
.if defined(NOOBJ)
obj:
.else
obj:
	@cd ${.CURDIR}; rm -rf obj; \
	here=${.CURDIR}; dest=${OBJDIR}`echo $$here | sed 's,${STRIPDIR},,'`; \
	echo "$$here -> $$dest"; ln -s $$dest obj; \
	if test -d ${OBJDIR} -a ! -d $$dest; then \
		mkdir -p $$dest; \
	else \
		true; \
	fi
.endif
.endif

.if !target(objdir)
.if defined(NOOBJ)
objdir:
.else
objdir:
	@cd ${.CURDIR}; \
	if test -h obj; then \
		here=${.CURDIR}; objlnk=`ls -l obj | sed 's/.* //'`; \
		echo "$$here -> $$objlnk"; \
		if test ! -d $$objlnk; then \
			mkdir -p $$objlnk; \
		else \
			true; \
		fi; \
	else \
		true; \
	fi
.endif
.endif

.endif	# defined(SUBMAKE)
