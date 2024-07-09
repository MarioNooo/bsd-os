# The default PATH is set in /etc/login.conf in the daemon class
echo 'erase ^H, kill ^U, intr ^C status ^T'
stty crt erase  kill  intr  status 
export PATH
HOME=/root
export HOME
BLOCKSIZE=1k
export BLOCKSIZE
TERM=bsdos-pc
export TERM
