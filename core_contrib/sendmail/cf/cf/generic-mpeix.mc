divert(-1)
#
# Copyright (c) 2001 Sendmail, Inc. and its suppliers.
#	All rights reserved.
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the sendmail distribution.
#
#

#
#  This is a generic configuration file for HP MPE/iX.
#  It has support for local and SMTP mail only.  If you want to
#  customize it, copy it to a name appropriate for your environment
#  and do the modifications there.
#

divert(0)dnl
VERSIONID(`generic-mpeix.mc,v 1.3 2003/09/17 21:19:13 polk Exp')
OSTYPE(mpeix)dnl
DOMAIN(generic)dnl
define(`confFORWARD_PATH', `$z/.forward')dnl
MAILER(local)dnl
MAILER(smtp)dnl
