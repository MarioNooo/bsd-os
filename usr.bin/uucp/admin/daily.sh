#!/bin/sh

PATH=/bin:/usr/bin:/usr/sbin
export PATH

# vixie 28jan94 [original]
#
# daily.sh,v 2.1 1995/02/03 13:16:00 polk Exp

# if we had a uupoll command, this would be an ideal place to use it on
# whatever systems we want to poll once a day just to keep the pipes clear.

echo ""
/usr/bin/uusnap

exit 0
