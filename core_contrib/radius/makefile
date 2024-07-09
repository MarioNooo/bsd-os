#
# COPYRIGHT 1992, 1993, 1994, 1995, 1996, 1997, 1998
# THE REGENTS OF THE UNIVERSITY OF MICHIGAN
# ALL RIGHTS RESERVED
#
# PERMISSION IS GRANTED TO USE, COPY AND REDISTRIBUTE THIS VERSION OF THE MERIT
# BASIC AAA SERVER, SO LONG AS NO FEE IS CHARGED FOR THIS SOFTWARE, AND SO LONG
# AS THE COPYRIGHT NOTICE ABOVE, THIS GRANT OF PERMISSION, AND THE DISCLAIMER
# BELOW APPEAR IN ALL COPIES MADE; AND SO LONG AS THE NAME OF THE UNIVERSITY OF
# MICHIGAN OR MERIT NETWORK IS NOT USED IN ANY ADVERTISING OR PUBLICITY 
# PERTAINING TO THE USE OR DISTRIBUTION OF THIS SOFTWARE WITHOUT SPECIFIC, 
# WRITTEN PRIOR AUTHORIZATION. 
#
# NO RIGHTS ARE GRANTED HEREUNDER FOR ANY RECIPIENT TO MODIFY, DISASSEMBLE,
# DECOMPILE, REVERSE ENGINEER OR OTHERWISE CREATE DERIVATIVE WORKS OF THIS
# SOFTWARE.
#
# THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION FROM THE UNIVERSITY
# OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND WITHOUT WARRANTY BY THE
# UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING
# WITHOUT LIMITATION THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE.  THE REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE
# LIABLE FOR ANY DAMAGES, INCLUDING SPECIAL, INDIRECT, INCIDENTAL, OR
# CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN
# CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS BEEN OR IS HEREAFTER
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
#
# FOR FURTHER INFORMATION ABOUT THE ENHANCED MERIT AAA SERVER, SEND EMAIL TO:
# aaa.license@merit OR, VISIT THE WWW SITE:  www.merit.edu/aaa/
#
#  
#  Merit AAA server:
#
#    radiusd:   binary is multitasking server w/o Kerberos or DBM support for
#		authentication.  The client, authfile, and users (optionally)
#		files are stored in memory.  Includes RADIUS-RADIUS relay
#		authentication capability and accounting and xtacacs support.
#
#
#  Various client modules and utilities:
#
#    dnscheck - Utility to check DNS mapping.
#    radcheck - Polls a Merit AAA server to see if it's still alive.
#    radpwtst - Generates various kinds of authentication requests for
#		testing a Merit AAA server.
#

##### This is the start of the Makefile area you may need to or want to change.

# Where the program executables live.
BIN		= ./bin
# Where the man pages live.
MAN		= ./man
# Where the configuration files live.
RADDB		= ./raddb
# Some installation stuff.
DAEMON_INSDIR	= /usr/libexec
BIN_INSDIR	= /usr/sbin
MAN_INSDIR	= /usr/share/core_contrib/man
RADDB_INSDIR	= /etc/raddb
RADACCT_INSDIR	= /var/account/radius
# The server does not need to be owned by root, unless some shadow password
# scheme needs it.  You might create a user id "radius" for just this purpose.
RADOWN		= radius
RADGRP		= radius

INSTALL	  = /usr/bin/install -c

##### This is the end of the Makefile area you may need to or want to change.

SERVER	  = radiusd
RADIUS	  = $(BIN)/radiusd

DNSCHECK  = $(BIN)/dnscheck
RADCHECK  = $(BIN)/radcheck
RADPWTST  = $(BIN)/radpwtst
RADIUSC   = $(BIN)/radiusc

UTILS    = $(RADCHECK) $(RADPWTST) $(DNSCHECK)

CONFIGS	  = $(RADDB)/authfile $(RADDB)/clients $(RADDB)/conversion.pl \
	$(RADDB)/dictionary $(RADDB)/users $(RADDB)/vendors $(RADDB)/xas.fsm

all:	notice 

notice:
	@echo ""
	@echo "COPYRIGHT 1992, 1993, 1994, 1995, 1996, 1997, 1998"
	@echo "THE REGENTS OF THE UNIVERSITY OF MICHIGAN"
	@echo "ALL RIGHTS RESERVED"
	@echo ""
	@echo "PERMISSION IS GRANTED TO USE, COPY AND REDISTRIBUTE THIS VERSION OF THE MERIT"
	@echo "BASIC AAA SERVER, SO LONG AS NO FEE IS CHARGED FOR THIS SOFTWARE, AND SO LONG"
	@echo "AS THE COPYRIGHT NOTICE ABOVE, THIS GRANT OF PERMISSION, AND THE DISCLAIMER"
	@echo "BELOW APPEAR IN ALL COPIES MADE; AND SO LONG AS THE NAME OF THE UNIVERSITY OF"
	@echo "MICHIGAN OR MERIT NETWORK IS NOT USED IN ANY ADVERTISING OR PUBLICITY"
	@echo "PERTAINING TO THE USE OR DISTRIBUTION OF THIS SOFTWARE WITHOUT SPECIFIC,"
	@echo "WRITTEN PRIOR AUTHORIZATION."
	@echo ""
	@echo "NO RIGHTS ARE GRANTED HEREUNDER FOR ANY RECIPIENT TO MODIFY, DISASSEMBLE,"
	@echo "DECOMPILE, REVERSE ENGINEER OR OTHERWISE CREATE DERIVATIVE WORKS OF THIS"
	@echo "SOFTWARE."
	@echo ""
	@echo "THIS SOFTWARE IS PROVIDED AS IS, WITHOUT REPRESENTATION FROM THE UNIVERSITY"
	@echo "OF MICHIGAN AS TO ITS FITNESS FOR ANY PURPOSE, AND WITHOUT WARRANTY BY THE"
	@echo "UNIVERSITY OF MICHIGAN OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING"
	@echo "WITHOUT LIMITATION THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR"
	@echo "A PARTICULAR PURPOSE.  THE REGENTS OF THE UNIVERSITY OF MICHIGAN SHALL NOT BE"
	@echo "LIABLE FOR ANY DAMAGES, INCLUDING SPECIAL, INDIRECT, INCIDENTAL, OR"
	@echo "CONSEQUENTIAL DAMAGES, WITH RESPECT TO ANY CLAIM ARISING OUT OF OR IN"
	@echo "CONNECTION WITH THE USE OF THE SOFTWARE, EVEN IF IT HAS BEEN OR IS HEREAFTER"
	@echo "ADVISED OF THE POSSIBILITY OF SUCH DAMAGES."
	@echo ""
	@echo "FOR FURTHER INFORMATION ABOUT THE ENHANCED MERIT AAA SERVER, SEND EMAIL TO:"
	@echo "aaa.license@merit OR, VISIT THE WWW SITE:  www.merit.edu/aaa/"
	@echo ""
	@echo "Check the user changes section in the Makefile."
	@echo "Then enter one of the following:"
	@echo ""
	@echo "make server-install"
	@echo "make util-install"
	@echo "make config-install"
	@echo "make man-install"
	@echo ""


install: server-install util-install man-install
	if [ ! -d /usr/share/doc/radiusd ] ; then \
		mkdir -p /usr/share/doc/radiusd ; \
	fi
	pax -rwv -s ',.*/CVS.*,,' \
		INSTALL.basic.binary \
		README* \
		TUTORIAL \
		doc raddb /usr/share/doc/radiusd

server-install: $(RADIUS)
	-if [ ! -d $(DAEMON_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(DAEMON_INSDIR) ;\
	fi
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADIUS) \
						$(DAEMON_INSDIR)/$(SERVER)

util-install: $(UTILS) $(RADIUSC)
	-if [ ! -d $(BIN_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(BIN_INSDIR) ;\
	fi
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(DNSCHECK) \
						$(BIN_INSDIR)/dnscheck
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADCHECK) \
						$(BIN_INSDIR)/radcheck
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADPWTST) \
						$(BIN_INSDIR)/radpwtst
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADIUSC) \
						$(BIN_INSDIR)/radiusc

.SUFFIXES: .8 .7 .6 .5 .4 .3 .2 .1 .0

.PATH: man
.8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
	groff -mtty-char -Tascii -man ${.IMPSRC} > ${.TARGET}

MAN8= dnscheck.0 radcheck.0 radiusd.0 radpwtst.0 radiusc.0
MAN5= authfile.0 clients.0 dictionary.0 radius.fsm.0 users.0 vendors.0
maninstall man-install: ${MAN8} ${MAN5}
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) authfile.0 \
						$(MAN_INSDIR)/cat5/authfile.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) clients.0 \
						$(MAN_INSDIR)/cat5/clients.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) dictionary.0 \
						$(MAN_INSDIR)/cat5/dictionary.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) radius.fsm.0 \
						$(MAN_INSDIR)/cat5/radius.fsm.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) users.0 \
						$(MAN_INSDIR)/cat5/users.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) vendors.0 \
						$(MAN_INSDIR)/cat5/vendors.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) dnscheck.0 \
						$(MAN_INSDIR)/cat8/dnscheck.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) radcheck.0 \
						$(MAN_INSDIR)/cat8/radcheck.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) radiusd.0 \
						$(MAN_INSDIR)/cat8/radiusd.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) radpwtst.0 \
						$(MAN_INSDIR)/cat8/radpwtst.0
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) radiusc.0 \
						$(MAN_INSDIR)/cat8/radiusc.0

config-install: $(CONFIGS)
	-if [ ! -d $(RADDB_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(RADDB_INSDIR) ;\
	fi
	$(INSTALL) -m 660 -o $(RADOWN) -g $(RADGRP) $(RADDB)/authfile \
						$(RADDB_INSDIR)/authfile
	$(INSTALL) -m 660 -o $(RADOWN) -g $(RADGRP) $(RADDB)/clients \
						$(RADDB_INSDIR)/clients
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADDB)/conversion.pl \
						$(RADDB_INSDIR)/conversion.pl
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(RADDB)/dictionary \
						$(RADDB_INSDIR)/dictionary
	$(INSTALL) -m 660 -o $(RADOWN) -g $(RADGRP) $(RADDB)/users \
						$(RADDB_INSDIR)/users
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(RADDB)/vendors \
						$(RADDB_INSDIR)/vendors
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(RADDB)/xas.fsm \
						$(RADDB_INSDIR)/xas.fsm
	-if [ ! -d $(RADACCT_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(RADACCT_INSDIR) ;\
	fi

$(RADIUS): bin/radiusd.gz.uu
	(cd bin && uudecode radiusd.gz.uu && gunzip radiusd)

$(DNSCHECK): bin/dnscheck.gz.uu
	(cd bin && uudecode dnscheck.gz.uu && gunzip dnscheck)

$(RADCHECK): bin/radcheck.gz.uu
	(cd bin && uudecode radcheck.gz.uu && gunzip radcheck)

$(RADPWTST): bin/radpwtst.gz.uu
	(cd bin && uudecode radpwtst.gz.uu && gunzip radpwtst)


obj all depend mansourceinstall:

clean cleandir:
	rm -f *.0 *.gz bin/*.gz $(RADIUS) $(UTILS)
