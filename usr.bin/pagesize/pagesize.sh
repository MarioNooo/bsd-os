[ $# != 0 ] && {
	echo 'usage: pagesize'
	exit 1
}

/sbin/sysctl -n hw.pagesize
