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
DAEMON_INSDIR	= /usr/private/etc
BIN_INSDIR	= /usr/private/etc
MAN_INSDIR	= /usr/local/man
RADDB_INSDIR	= /usr/private/etc/raddb.new
RADACCT_INSDIR	= /usr/private/etc/radacct
# The server does not need to be owned by root, unless some shadow password
# scheme needs it.  You might create a user id "radius" for just this purpose.
RADOWN		= root
RADGRP		= bin

INSTALL	  = /bin/install
#INSTALL	  = /usr/ucb/install # Solaris, BSDi
#INSTALL	  = /usr/bin/install # Linux

##### This is the end of the Makefile area you may need to or want to change.

SERVER	  = radiusd
RADIUS	  = $(BIN)/radiusd

DNSCHECK  = $(BIN)/dnscheck
RADCHECK  = $(BIN)/radcheck
RADPWTST  = $(BIN)/radpwtst

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
	@echo "make install"
	@echo "make util-install"
	@echo "make config-install"
	@echo "make man-install"
	@echo ""


install-all: install util-install config-install man-install

install: $(RADIUS)
	-if [ ! -d $(DAEMON_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(DAEMON_INSDIR) ;\
	fi
	$(INSTALL) -m 755 -o $(RADOWN) -g $(RADGRP) $(RADIUS) \
						$(DAEMON_INSDIR)/$(SERVER)

util-install: $(UTILS)
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

man-install:
	-if [ ! -d $(MAN_INSDIR) ] ;\
	then \
		/bin/mkdir -p $(MAN_INSDIR) ;\
		/bin/mkdir -p $(MAN_INSDIR)/man5 ;\
		/bin/mkdir -p $(MAN_INSDIR)/man8 ;\
	fi
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/authfile.5 \
						$(MAN_INSDIR)/man5/authfile.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/clients.5 \
						$(MAN_INSDIR)/man5/clients.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/dictionary.5 \
						$(MAN_INSDIR)/man5/dictionary.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/radius.fsm.5 \
						$(MAN_INSDIR)/man5/radius.fsm.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/users.5 \
						$(MAN_INSDIR)/man5/users.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/vendors.5 \
						$(MAN_INSDIR)/man5/vendors.5
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/dnscheck.8 \
						$(MAN_INSDIR)/man8/dnscheck.8
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/radcheck.8 \
						$(MAN_INSDIR)/man8/radcheck.8
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/radiusd.8 \
						$(MAN_INSDIR)/man8/radiusd.8
	$(INSTALL) -m 644 -o $(RADOWN) -g $(RADGRP) $(MAN)/radpwtst.8 \
						$(MAN_INSDIR)/man8/radpwtst.8

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
