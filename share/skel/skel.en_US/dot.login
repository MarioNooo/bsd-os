# csh .login file

# Below are commands not to be done if this is a remote execution or cronjob
if ($?prompt) then
  set noglob
  eval `tset -s -m 'network:?xterm'`
  unset noglob
  #
  if ( $TERM == unknown || $TERM == network || $TERM == dialup ) then
    echo ""
    echo "term = unknown"
    set noglob
    eval `tset -Q -s -m ":?vt100"`
    unset noglob
  endif
  #
  stty crt -tostop erase '^H' kill '^U' intr '^C' status '^T'
  if ( -f ~/.login.locale ) source ~/.login.locale
  if ( -x /usr/games/fortune ) /usr/games/fortune
endif

