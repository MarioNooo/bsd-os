#!/bin/sh

# uuxqt_hook.sh,v 2.2 1996/06/11 19:26:44 vixie Exp

# this will be exec'd by uuxqt whenever it finishes a pass through the
# incoming transaction pool.  since rmail runs sendmail with "-odq", one
# way to get incoming mail to be unbatched is to run (or exec) sendmail
# with "-q" from this file.  if you don't do this, incoming uucp mail will
# wait for the next queue run (see sendmail startup in /etc/{rc,rc.local}
# if you want to know how often that is.)

#nohup exec /usr/sbin/sendmail -q
