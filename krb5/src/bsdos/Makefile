# build varioups binaries
# some are in library directories... others are scattered about

# commands from "krb524" directory
SUBDIR= krb524d k524init

# commands from "kadmin" directory
SUBDIR+=kadmin kadmin.local kadmind kdb5_util ktutil

# should only be needed for who had the old kerberosIV kadmind
# ported anyway -- never shipped with BSD/OS, so it shouldn't matter  
# does NOT COMPILE
# SUBDIR+=kadmind4

# these will build, but are obsoleted by the "new" kpasswd client
# they use an older (non-kadmin) protocol for password changes
# SUBDIR+=v5passwd v5passwdd

# further confusion -- the "kpasswd" in the kadmin directory isn't
# the one that gets installed -- the one from the clients directory
# is the one that gets installed..
# SUBDIR+=kadmin.kpasswd

# commands from "clients" directory
SUBDIR+=kdestroy kinit klist kpasswd kvno
# BSD/OS doesn't need ksu, due to the way BSD/OS authentication works...
# SUBDIR+= ksu

# commands from "slave" directory
SUBDIR+=kprop kpropd

# commands donated from UUNET, specific to BSD/OS
SUBDIR+=login_krb5 login_krb5-or-pwd

# commands from "appl/bsd" directory
# These commands are not backwards compatible with the command line
# options or fallback procedures for the same commands from BSD/OS
# SUBDIR+=rcp.mit rlogin.mit rlogind.mit rsh.mit rshd.mit

# BSD/OS kerberosIV commands, linked against the krb5 libraries
# and 425des libraries
# these should not be needed, but might be handy to have just in case
SUBDIR+=rcp.krb4 rlogin.krb4 rlogind.krb4 rsh.krb4 rshd.krb4

.include <bsd.subdir.mk>
