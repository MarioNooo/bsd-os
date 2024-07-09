#csh .cshrc file

alias h		history 25
alias j		jobs -l
alias la	ls -a
alias lf	ls -FA
alias ll	ls -lA
alias su	su -m

setenv	EDITOR	vi
setenv	EXINIT	'set autoindent'
setenv	PAGER	more
setenv	BLOCKSIZE 1k

set path = (~/bin /bin /usr/bin /sbin /usr/sbin /usr/X11/bin /usr/contrib/bin /usr/contrib/mh/bin /usr/games /usr/local/bin)

### uncomment to select an alternate timezone (/etc/localtime is default)
# setenv TZ /usr/share/zoneinfo/US/Central

### NEWS Configuration
setenv RNINIT "$HOME/.rninit"
# setenv ORGANIZATION 'Widgets, Inc.'
# setenv NNTPSERVER news

### X Window System Configuration
setenv XAPPLRESDIR "$HOME/app-defaults/Class/"
### Old-style XNLSPATH
# setenv XNLSPATH /usr/X11/lib/X11/nls

### WWW Browser Configuration
# setenv WWW_wais_GATEWAY "http://www.ncsa.uiuc.edu:8001"
setenv WWW_HOME "http://www.bsdi.com/welcome.html"

### where xbiff (et.al.) should look for mail
set mail = (/var/mail/$USER)
setenv MAILPATH /var/mail/$USER
setenv mail /var/mail/$USER

if ($?prompt) then
	# An interactive shell -- set some stuff up
	set filec
	set history = 1000
	set mail = (/var/mail/$USER)
	set mch = `hostname -s`
	set prompt = "${mch:q}: {\!} % "

	# umask sets a mask for the default file permissions
	umask 022

	# biff controls new mail notification
	biff y

	# mesg controls messages from other users
	mesg y
endif

if ( -f ~/.cshrc.locale ) source ~/.cshrc.locale
