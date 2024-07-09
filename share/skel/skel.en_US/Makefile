#	BSDI	Makefile,v 2.12 1996/10/12 01:07:47 sanders Exp
#
# Install skel files into user's home directory
#
### These must be set on the make command line:
# HOMEDIR	/home/username
# OWNER		username
# GROUP		userprimarygroup

### These default to something but can be overridden:
# LOGIN_CLASS	userclass (default, staff, restricted, etc)
LOGIN_CLASS	?= default
# MODE		normal file mode (defaults to: 0644)
MODE		?= 0644
# PRIVATE 	private file mode [for .rhosts] (defaults to: 0600)
PRIVATE		?= 0600
# DIRMODE 	mode for directories (defaults to: 0755)
DIRMODE		?= 0755
# FORCEWRITE	force overwrite of existing files (set to 1 to force overwrite)
FORCEWRITE	?= 0

install:
	if [ "X${HOMEDIR}" = "X" -o "X${OWNER}" = "X" -o "X${GROUP}" = "X" ]; \
		then echo "Must set HOMEDIR, OWNER and GROUP"; exit 1; fi
	: make users home directory
	-mkdir ${HOMEDIR}
	: make special directories
	-mkdir -p ${HOMEDIR}/.netscape/cache
	chmod ${DIRMODE} ${HOMEDIR} ${HOMEDIR}/.netscape \
		${HOMEDIR}/.netscape/cache
	/usr/sbin/chown ${OWNER}.${GROUP} ${HOMEDIR} ${HOMEDIR}/.netscape \
		${HOMEDIR}/.netscape/cache
	: copy login class specific files, which overwrite any existing files
	find . -name 'class.${LOGIN_CLASS}*' | {			\
	    while read fname; do					\
		dest="${HOMEDIR}/$$(echo $$fname | sed -e 's#/class.${LOGIN_CLASS}#/#g')";\
		if [ ! -e $$dest -o "${FORCEWRITE}" = "1" ]; then	\
		    if [ -d $$fname ]; then				\
			if [ ! -e $$dest ]; then			\
			    rm -f $$dest; mkdir -p $$dest;		\
			    chmod ${DIRMODE} $$dest;			\
			fi;						\
		    else						\
			echo $$fname '->' $$dest;			\
			cp $$fname $$dest;				\
			chmod ${MODE} $$dest;				\
		    fi;							\
		    /usr/sbin/chown ${OWNER}.${GROUP} $$dest;		\
		fi;							\
	    done;							\
	}
	: copy standard skel files over, removing the "dot" prefix
	find . -name 'dot*' | {						\
	    while read fname; do					\
		dest="${HOMEDIR}/$$(echo $$fname | sed -e 's#/dot#/#g')";\
		if [ ! -e $$dest -o "${FORCEWRITE}" = "1" ]; then	\
		    if [ -d $$fname ]; then				\
			if [ ! -e $$dest ]; then			\
			    rm -f $$dest; mkdir -p $$dest;		\
			    chmod ${DIRMODE} $$dest;			\
			fi;						\
		    else						\
			echo $$fname '->' $$dest;			\
			cp $$fname $$dest;				\
			chmod ${MODE} $$dest;				\
		    fi;							\
		    /usr/sbin/chown ${OWNER}.${GROUP} $$dest;		\
		fi;							\
	    done;							\
	}
	: handle special cases
	chmod ${PRIVATE} ${HOMEDIR}/.rhosts
	chmod 0755 ${HOMEDIR}/.xinitrc ${HOMEDIR}/.xserverrc
	if [ ! -e ${HOMEDIR}/.xsession ]; then ln -s .xinitrc ${HOMEDIR}/.xsession; fi
