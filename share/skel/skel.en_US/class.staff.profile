#
# $HOME/.profile (works with sh, ksh and bash)
#
PATH=$HOME/bin:/bin:/usr/bin:/sbin:/usr/sbin:/usr/X11/bin:/usr/contrib/bin:/usr/contrib/mh/bin:/usr/games:/usr/local/bin
export PATH

### where to look for man pages (see also /etc/man.conf and man(1))
MANPATH=/usr/man:/usr/share/man:/usr/contrib/man:/usr/contrib/mh/man:/usr/local/man:/usr/X11/man
export MANPATH

#### default printer for lpr
PRINTER=lp
export PRINTER

### default editor for many applications
EDITOR=vi; export EDITOR

### a good alternative is: PAGER=less
PAGER=more; export PAGER
BLOCKSIZE=1k; export BLOCKSIZE

### uncomment to select an alternate timezone (/etc/localtime is default)
# TZ=/usr/share/zoneinfo/US/Central; export TZ

### NEWS Configuration
RNINIT="$HOME/.rninit"; export RNINIT
# ORGANIZATION='Widgets, Inc.'; export ORGANIZATION
NNTPSERVER=news; export NNTPSERVER

### X Window System Configuration
XAPPLRESDIR="$HOME/.Xclasses/"; export XAPPLRESDIR
### Old-style XNLSPATH
# XNLSPATH=/usr/X11/lib/X11/nls; export XNLSPATH

### WWW Browser Configuration
# WWW_wais_GATEWAY="http://www.ncsa.uiuc.edu:8001"; export WWW_wais_GATEWAY
WWW_HOME="http://www.bsdi.com/welcome.html"; export WWW_HOME

### email
MAIL="/var/mail/$USER"
MAILCHECK=30
MAILPATH="/var/mail/$USER"
export MAIL MAILCHECK MAILPATH

### shell history
HISTFILE="$HOME/.history"
HISTFILESIZE=5000
HISTSIZE=5000
FCEDIT="$EDITOR"
export HISTFILE HISTFILESIZE HISTSIZE FCEDIT

### Interactive only commands
case $- in *i*)
    eval `tset -s -m 'network:?xterm'`
    stty crt -tostop erase '^H' kill '^U' intr '^C' status '^T'
    ### biff controls new mail notification
    biff y
    ### mesg controls messages from other users
    mesg y
    if [ "X$TERM" = Xdialup -o "X$TERM" = Xunknown -o "X$TERM" = Xdumb ]; then
        TERM=`tset -Q -s -m ":?vt100"`
    fi
esac
export TERM

### Shell specific setup
case "$SHELL" in
    */bash) set -o vi; set -o notify
	    command_oriented_history=1
	    ignoreeof=1
	    if [ -w /sbin/init ]; then PSCH="#"; else PSCH="$"; fi
	    export PSCH
    	    PS1='\h:\w $PSCH ' ENV="$HOME/.shellrc"
	    export ENV
	    [ -f "$ENV" ] && . "$ENV"
            ;;
    */ksh)  set -o emacs; set -o monitor; set -o ignoreeof
	    PS1="$(/bin/hostname -s)$ "
	    ENV="$HOME/.shellrc"
	    export ENV
            ;;
    *)      set -I
	    PS1="$(/bin/hostname -s)$ "
	    ENV="$HOME/.shellrc"
	    export ENV
            ;;
esac

### umask sets a mask for the default file permissions,
### ``umask 002'' is less restrictive
umask 022

if [ -f $HOME/.profile.locale ]; then
    . $HOME/.profile.locale
fi
