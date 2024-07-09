#!/bin/sh
# 
# Environment Variables:
#     MAXIM_TXTBROWSER	-- use instead of lynx
#     MAXIM_XBROWSER	-- use instead of netscape
#     MAXIM_NOREMOTE	-- don't use the netscape -remote feature
#     MAXIM_URL		-- the URL to load (defaults to the Maxim interface)
#
# Flags:
#   -i  is an undocumented flag used by the startup code to
#       initially configure the system.  Users can just run
#       config_X or Xsetup directly if they want to configure X.
#   -I  is also undocumented and exists primarily for debugging purposes.
#       It runs maxim with the "initialize" URL but doesn't do any of the
#       other -i stuff.

start_maxim() { /var/www/adminweb/conf/apachectl startq; }
stop_maxim() { /var/www/adminweb/conf/apachectl stop; }
is_running() {
    [ $# -eq 0 ] && return 1;
    ### we are satisified if kill doesn't return any output or
    ### or if it is an "Operation not permitted" error.
    killout="`/bin/kill -0 $1 2>&1`"
    [ -z "$killout" ] || expr "$killout" : '.*Operation' >/dev/null
}

# we build up load_url below, this is just the starting default
# (set here for -i mode)
load_url='http://localhost:880'

if [ "X$1" = "X-i" ]; then
    ### initialization mode
    shift
    touch /root/.initialize

    # whack the path for init mode
    PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/contrib/bin:/usr/X11/bin
    export PATH

    # XXX: load /etc/netstart for now, should config lo0 and read /etc/netstart
    echo "Starting network:"
    sh /etc/netstart

    # configure the X server
    if [ -f /usr/sbin/config_X ]; then /usr/sbin/config_X; fi

    # load the serialized initialization stuff

    # BEGIN. Changes by MACNICA
    # Japanese MaxIM launches , if and only if Japanese Keyboard is selected.
    JapaneseCheck=`grep JP /etc/keyboard.map`

    if [ -n "$JapaneseCheck" ]; then
    # let Netscape launch with Japanese EUC code.
    # check character_set is defined or not.
      JapaneseCode=`grep intl.character_set \
                         /usr/libdata/maxim_init/.netscape/preferences.js`
      if [ ! -n "$JapaneseCode" ]; then
        patch /usr/libdata/maxim_init/.netscape/preferences.js \
              /usr/libdata/maxim_init/.netscape/preferences.js.mod
      fi
      if [ ! -d /root/.netscape ]; then
        cp -R /usr/libdata/maxim_init/.netscape /root
      fi

    # enables to start with Japanese MaxIM.
      cp /var/www/adminweb/interfaces/jp.cgi \
	 /var/www/adminweb/interfaces/index.cgi

    #setting the starting URL, which leads to Japanese MaxIM.
      load_url="${load_url}/jp2.html"
    else
    #This is the default case, the default URL. 
      load_url="${load_url}/View/Initialize/Step1/Standard.view"
    fi
    # END. Changes by MACNICA

    # notify user (XXX: would be nice to have a real front page for maxim)
    clear
    echo ""
    echo ""
    echo "We will now start MaxIM...  You will need to login to MaxIM"
    echo "by entering the root login and password.  Once MaxIM has"
    echo "configured your system you will be able to use your own login"
    echo "and password to access MaxIM."
    echo ""
    echo -n "Press return when ready..."
    read foo

    # start an instance of the server
    start_maxim

    # If we wrote an X configuration file then we'll do the X init thing.
    if [ -f /etc/X.config ]; then
        echo "Starting X Server..." 1>&2
	# we load the X environment from a special location
	# that starts the X browser for us.
	HOME=/usr/libdata/maxim_init
	MAXIM_URL=${load_url}
	export HOME MAXIM_URL
	xinit -- -bs -su
    else
	# we'll do our own client for non-X mode
	${MAXIM_TXTBROWSER:-"lynx"} $load_url
    fi

    # now stop the server that we started (it'll be started at boot time)
    stop_maxim

    # XXX: we reboot for now but it would be nice to do this only when needed
    echo "Rebooting System"
    reboot

    exit 0
fi

### normal start
# allow user to override the normal starting URL
load_url="${MAXIM_URL:-$load_url}"

if [ "X$1" = "X-s" ]; then
    ### start server only
    shift
    start_maxim
    exit $?
fi

if [ "X$1" = "X-S" ]; then
    ### stop server only
    shift
    stop_maxim
    exit $?
fi

if [ "X$1" = "X-I" ]; then
    # change the default URL to the Initialize mode
    shift
    load_url="${load_url}/View/Initialize/Step1/Standard.view"
fi

### Check if the server is running...
if ! is_running "`cat /var/run/adminweb.pid 2>/dev/null`"; then
    echo "MaxIM server not running; run \`maxim -s' as root to start" 1>&2
    exit 1
fi

### Start the approriate browser
if [ -z "$DISPLAY" ]; then
    exec ${MAXIM_TXTBROWSER:-"lynx"} $load_url
    exit 127
else
    browser=${MAXIM_XBROWSER:-"netscape"}
    # first, try to open the URL in an existing netscape for this user;
    # unless they set MAXIM_NOREMOTE in their environment
    visual=
    if expr "$browser" : '.*[Nn]etscape' >/dev/null \
	    && [ "X$MAXIM_NOREMOTE" = "X" ]; then
	if expr "`xdpyinfo | grep 'depth of root window'`" : '.* 4 planes' > /dev/null; then
	    visual="-visual staticgray"
	fi
	$browser -remote "openurl($load_url)" 2>/dev/null
    else
	false				# we'll run our own below
    fi

    if [ "$?" -eq "0" ]; then
	echo "NOTE: Loaded MaxIM in your existing netscape window." 1>&2
    else
        # BEGIN. Changes by MACNICA
        JapaneseCheck=`grep JP /etc/keyboard.map`
        if [ -n "$JapaneseCheck" ]; then
            if [ -f /root/.initialize ]; then
                rm /root/.initialize
                exec $browser $visual $load_url
            else
                exec $browser $visual "${load_url}/jp.html"
            fi
        else
            exec $browser $visual $load_url
        fi
        # END. Changes by MACNICA
	exit 127
    fi
fi

exit 0
