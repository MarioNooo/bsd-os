#!/bin/sh
#
#	BSDI	chkconfig.sh,v 2.3 1995/08/01 15:45:41 bostic Exp

PATH=/bin:/usr/bin:/sbin:/usr/sbin

old=/var/db/dmesg.boot
new=/tmp/ck$$.dmesg

# Save only last configuration report in dmesg buffer.
# The strange BSDI pattern is to eliminate any garbage.
dmesg | sed -n \
    -e '/BSDI.*Kernel/ { s/.*BSDI/BSDI/ ; x ; n ; }' \
    -e 'H' -e '$ { x ; p ; }' > $new

# N.B.: "delay multiplier" is i386-specific, but the grep and awk are safe
# (are no-ops) on other architectures.
if [ -r $old ]
then
	grep -v 'delay multiplier' $old > /tmp/ck$$.1
	grep -v 'delay multiplier' $new > /tmp/ck$$.2

	cat $old $new | awk '/^delay multiplier / {
		if (oldm == 0) {
			oldm = $3;
		} else {
			newm = $3;
			if (oldm != 0) {
				pct = (newm - oldm) * 100.0 / oldm;
				if (pct < 0)
					pct = -pct;
				if (pct >= 5)
					printf("delay multiplier %d -> %d\n",
						oldm, newm);
			}
		
		}
	}' > /tmp/ck$$.3

	diff /tmp/ck$$.1 /tmp/ck$$.2 >> /tmp/ck$$.3

	if [ -s /tmp/ck$$.3 ]
	then
		(echo "Configuration changed:"; cat /tmp/ck$$.3) > /tmp/ck$$.4
		cat /tmp/ck$$.4
		logger < /tmp/ck$$.4
	fi
fi

mv $new $old
chmod 644 $old

rm -f /tmp/ck$$.*

exit 0
