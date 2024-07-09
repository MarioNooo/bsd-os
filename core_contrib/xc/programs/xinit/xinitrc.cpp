#! /bin/sh
# $XConsortium: xinitrc.cpp,v 1.4 91/08/22 11:41:34 rws Exp $
#	BSDI	xinitrc.cpp,v 2.15 2002/08/21 23:45:19 polk Exp
#

#undef bsdi
# (make sure bsdi isn't defined in case we're going through cpp)

### Search for CONFIGURE to find items that you might want to customize

userresources=$HOME/.Xresources
usermodmap=$HOME/.Xmodmap
sysresources=/usr/X11R6/lib/X11/xinit/.Xresources
sysmodmap=/usr/X11R6/lib/X11/xinit/.Xmodmap

HOST="`hostname 2>/dev/null`"
export HOST
HOSTNAME="`hostname -s 2>/dev/null`"
export HOSTNAME
#====== INETDISPLAY is for use by remote clients
INETDISPLAY="$HOST:`echo $DISPLAY | sed -e 's/^[^:]*:\(.*\)/\1/'`"
export INETDISPLAY

# setup some defaults -- these are mainly only used in xdm environments
if [ "X$MAILPATH" = "X" ]; then MAILPATH="/var/mail/$USER"; fi
export MAILPATH
if [ "X$WWW_HOME" = "X" ]; then WWW_HOME="http://www.bsdi.com/welcome.html"; fi
export WWW_HOME

#====== load Xresources database
# CONFIGURE: Per application configuration data is stored in the
# X resource database.  See also files in, .Xresource* and
# /usr/X11/lib/X11/app-defaults/...
[ -f $sysresources  ] && xrdb -merge -DHOME=$HOME -I$HOME -Ubsdi $sysresources
[ -f $userresources ] && xrdb -merge -DHOME=$HOME -I$HOME -Ubsdi $userresources

#====== load any X keyboard map modifications
[ -f $sysmodmap  ] && xmodmap $sysmodmap
[ -f $usermodmap ] && xmodmap $usermodmap

xsetroot -name "$HOST"

#====== If we don't have an environment specified, ask for one...
if [ ! -f $HOME/.Xenvtype ]; then
	xmessage -geometry +75+200 -buttons "GNOME:0,KDE:1,Old-Style BSDI:2" \
		"Which type of environment would you like to use?"
	case $? in
	0) echo GNOME	> $HOME/.Xenvtype ;;
	1) echo KDE	> $HOME/.Xenvtype ;;
	*) echo FVWM	> $HOME/.Xenvtype ;;
	esac
fi

#====== Read the environment file
whichenv=$(cat $HOME/.Xenvtype)
if [ X$whichenv = XGNOME ]; then
	# ====== GNOME uses gnome-session, make sure gnome, enlightenment, 
	#		and kde are in the path...
	PATH=$PATH:/usr/contrib/gnome/bin
	PATH=$PATH:/usr/contrib/gnome/enlightenment/bin
	PATH=$PATH:/usr/contrib/kde/bin
	gnome-session
elif [ X$whichenv = XKDE ]; then
	# ====== KDE uses startkde, make sure kde bin is in the path
	PATH=$PATH:/usr/contrib/kde/bin
	startkde
else

# ====== Everything from here down is the old way!

#====== start a console window
xrunclient xterm -iconic -geometry 80x12-0+0 -name "console" -ls -C

# =================================================================
# Screen Saver/DPMS Configuration
# =================================================================
# Set 10 minutes idle to screen saver activation.  With the blanking
# option, this will initiate DPMS stand-by mode in monitors that
# support it (for my monitor this is a a 92% reduction in power
# consumption).  DPMS modes are: normal -> stand-by -> suspend -> off.
# Further DPMS configuration parameters can be defined in /etc/Xaccel.ini.
# For example, DPMS parameters of one hour from stand-by to suspend
# and then 30 minutes to power off (see Xaccel(1) for more details):
#	SuspendTime = 3600;
#	OffTime = 1800;
xset s 600
xset s blank
#====== A sample screen saver (note: doesn't initiate DPMS mode)
# xrunclient beforelight
daemon xautolock -locker 'xlock -mode random -allowroot -message "Berkeley Software Design"' &
# =================================================================
# BACKGROUNDS -- pick one
# =================================================================
# CONFIGURE: which background do you like?
#====== xearth backgrounds...
XEARTHARGS="-grid -nomarkers -label -ncolors 64 -wait 900 -night 30"
#====== a nice generic mercator projection
# xrunclient xearth $XEARTHARGS -proj mercator -pos fixed,0,0
#====== mercator projection over atlantic (funky looking)
# xrunclient xearth $XEARTHARGS -proj mercator -pos fixed,30.45,-47.77
#====== globe centered over the atlantic (good view of US + western Europe)
# xrunclient xearth $XEARTHARGS -proj orth -pos fixed,30.45,-47.77 -mag 0.9
#====== globe centered over Central US
# xrunclient xearth $XEARTHARGS -proj orth -pos fixed,30.45,-97.77 -mag 0.9
#====== Misc
# xrunclient xfishtank
# xrunclient xphoon
# xv -root -quit /usr/X11/lib/X11/backgrounds/...
#====== XMAS Fun
# xrunclient xsnow -snowflakes 200 -santa 2 -notrees -nokeepsnow -slc sienna3
#====== BSDI Images
# xv -root -quit /usr/demo/clizard.gif
# xv -root -quit /var/www/docs/bsdi/bsdi-embossed.gif
xv -root -quit /var/www/docs/bsdi/doublewave.gif

# =================================================================
# XTERM's
# =================================================================
# CONFIGURE: start up local or remote xterms here
xrunclient xterm -geometry 80x24-0+0 -n "$HOSTNAME:1" -T "$HOSTNAME:1" -ls
xrunclient xterm -geometry 80x24+0-68 -n "$HOSTNAME:2" -T "$HOSTNAME:2" -ls

# =================================================================
# Sound system initialization
# =================================================================
# mixer volume 35 pcm 45 synth 0 line 0 microphone 0 cd 0 rdev microphone
# vplay $HOME/sounds/chimes.wav
xrunclient xmixer 2>/dev/null

# =================================================================
# WEB browser
# =================================================================
# CONFIGURE: if you want it started automatically
# ( cd $HOME; xrunclient netscape -iconic >/dev/null 2>&1; )
# ( cd $HOME; xrunclient mosaic -iconic >/dev/null 2>&1; )

#====== Start window manager last so that exiting the WM ends the X session
# CONFIGURE: pick a window manager and how you want to exit your X session
#exec fvwm2 -f 'Read .fvwmrc2'
# Note: fvwm1 configuration files are not compatible with fvwm2
exec fvwm
#exec fvwm2

# Note: If you want to use xexit you'll need to remove the `exec' from
# the line above that starts your WM
#exec xexit

#======= End of KDE/GNOME/Old Style if statement
fi

# The following only happens if the exec above fails
exit 127
