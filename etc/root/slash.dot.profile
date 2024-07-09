if [ -f /etc/keyboard.map ] ; then
        /sbin/setcons keymap /etc/keyboard.map ;
fi
. /root/.profile
