#! /usr/bin/awk -f

#
# Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI transform.awk,v 2.2 1997/11/01 04:50:16 donn Exp
#

filename == "" {
	filename = FILENAME
	init()
}

filename != FILENAME && emulation == "sco" {
	low_syscall_max = syscall_no - syscall_inc

	# iBCS2 secondary syscall table weirdness
	high_syscall_min = 296
	syscall_no = high_syscall_min
	syscall_inc = 256
	filename = FILENAME
}

/^[^_a-zA-Z]/ {
	# comments
	next
}

#
# Parse the line and submit it for processing.
#
{
	blocking = 0
	if (sub(/[ 	]*blocking[ 	]*[,;]/, "")) \
		blocking = 1
	emulated = 0
	if (sub(/[ 	]*emulated[ 	]*[,;]/, "")) \
		emulated = 1
	sub(/\)[ 	]*;?.*$/, "")
	gsub(/[ 	]+/, " ")
	match($0, /[_a-zA-Z][_a-zA-Z0-9]*\(/)
	return_type = substr($0, 1, RSTART - 1)
	sub(/[ 	]+$/, "", return_type)
	syscall = substr($0, RSTART, RLENGTH - 1)
	$0 = substr($0, RSTART + RLENGTH)
	n = split($0, argtype, /, */)
	if (n == 1 && argtype[1] == "void") \
		n = 0

	if (emulation != "bsd" || emulated) \
		if (syscall == "nosys") \
			nosys()
		else \
			process()
	syscall_no += syscall_inc
}

function init() {
	emulation = filename
	sub(/^.*\//, "", emulation)
	sub(/_.*$/, "", emulation)

	# emit #includes
	print "#include <sys/param.h>"
	print "#include <sys/ioctl.h>"
	print "#include <sys/ioctl_compat.h>"
	print "#include <sys/mount.h>"
	print "#include <sys/signal.h>"
	print "#include <sys/stat.h>"
	print "#include <sys/time.h>"
	print "#include <sys/times.h>"
	print "#include <dirent.h>"
	print "#include <err.h>"
	print "#include <errno.h>"
	print "#include <stdlib.h>"
	print "#include <time.h>"
	print "#include <utime.h>"
	print "#include \"emulate.h\""
	print "#include \"sco_ops.h\""
	print "#include \"sco.h\""
	print "#include \"sco_sig_state.h\""
	print ""

	if (emulation == "sco") {
		xin["fd_t"] = "fd_in"
		xin["filename_t"] = "filename_in"
		xout["filename_t"] = "filename_out"
		xin["open_t"] = "open_in"
		# xin["mode_t"] = "mode_in"
		xin["dev_t"] = "dev_in"
		# xin["uid_t"] = "uid_in"
		# xin["gid_t"] = "gid_in"
		xout["struct stat *"] = "stat_out"
		# xin["off_t"] = "off_in"
		xin["const struct sgttyb *"] = "sgttyb_in"
		xout["struct sgttyb *"] = "sgttyb_out"
		xout["struct statfs *"] = "statfs_out"
		xin["signal_t"] = "signal_in"
		xin["fcntl_t"] = "fcntl_in"
		xin["pathconf_t"] = "pathconf_in"
		xin["procmask_t"] = "procmask_in"
		xin["const sigset_t *"] = "sigset_in"
		xout["sigset_t *"] = "sigset_out"
		xin["sysconf_t"] = "sysconf_in"
		# xin["const gid_t *"] = "gidvec_in"
		# xout["gid_t *"] = "gidvec_out"
		print "typedef long sco_off_t;"
	} else {
		# emulation == "bsd"
		xin["filename_t"] = "filename_in"
		xout["filename_t"] = "filename_out"
		print "typedef int fd_t;"
		print "extern struct fdops bsdfileops;"
	}

	for (i in xin) {
		if (i !~ /_t$/) \
			continue
		if (i == "dev_t") \
			continue
		printf "typedef "
		if (i == "filename_t") \
			printf "char *"
		else \
			printf "int "
		print i ";"
	}
	print ""

	syscall_no = 0
	if (emulation == "sco") \
		nosys()

	syscall_no = 1
	syscall_inc = 1
}

function nosys() {
	print "int"
	print "x_" emulation "_nosys_" syscall_no "()"
	print "{"
	print ""
	print "\treturn (nosys(" syscall_no "));"
	print "}"
	print ""
	s[syscall_no] = "nosys_" syscall_no
}

function process() {
	print "/*", syscall_no ":", syscall, "*/"
	use_fd = 0

	# emit the prologue
	print return_type
	print "x_" emulation "_" syscall "(s)"
	print "\tvm_offset_t s;"
	print "{"

	if (return_type != "void") {
		printf "\t%s", return_type
		if (substr(return_type, length(return_type), 1) != "*") \
			printf " "
		print "r;"
	}

	# allocate space for native versions of data types
	for (i = 1; i <= n; ++i) {
		t = argtype[i]
		if (use_fd == 0 && t == "fd_t")
			use_fd = i
		if (xin[t] || xout[t]) {
			sub(/^const /, "", t)
			sub(/ *\*$/, "", t)
			print "\t" t, "x" i ";"
		}
	}

	print ""

	if (blocking == 0) {
		print "\tsig_state = SIG_POSTPONE;"
		print ""
	}

	# add a debugging printf()
	print "#ifdef DEBUG"
	print "\tif (debug & DEBUG_SYSCALLS)"
	printf "\t\twarnx(\"CALL %s(", syscall
	for (i = 1; i <= n; ++i) {
		printf "%%"
		if (argtype[i] == "filename_t") \
			printf "s"
		else \
			printf "#x"
		if (i < n) \
			printf ", "
	}
	printf ")\""
	for (i = 1; i <= n; ++i)
		printf ", ARG(s, %d, %s)", i, argtype[i]
	print ");"
	print "#endif /* DEBUG */"

	# convert incoming data
	for (i = 1; i <= n; ++i) {
		t = argtype[i]
		if (xin[t]) {
			if (t == "fd_t") {
				print "\tif ((x" i, "= fd_in(ARG(s,", \
				    i ", fd_t))) == -1)"
				print "\t\treturn (-1);"
			} else if (t ~ /\*$/) \
				print "\t" xin[t] "(ARG(s,", i ",", t "),",  \
					"&x" i ");"
			else \
				print "\tx" i, "=", xin[t] "(ARG(s,", \
					i ",", t "));"
		}
	}

	# emit the call to the handler, passing transformed arguments
	printf "\t"
	if (return_type != "void") \
		printf "r = "
	if (use_fd) {
		if (emulation == "bsd") \
			printf "(&bsdfileops)"
		else \
			printf "fdtab[x%d]->f_ops", use_fd
	} else \
		printf "generic"
	printf "->%s(", syscall
	for (i = 1; i <= n; ++i) {
		t = argtype[i]
		if (xin[t] || xout[t]) {
			if (t ~ /\*$/) \
				printf "&"
			printf "x%d", i
		} else \
			printf "ARG(s, %d, %s)", i, t
		if (i != n) \
			printf ", "
	}
	print ");"

	# convert outbound data
	for (i = 1; i <= n; ++i) {
		t = argtype[i]
		if (xout[t]) \
			print "\t" xout[t] "(&x" i ", ARG(s,", i ",", t "));"
	}

	# add another debugging printf()
	print "#ifdef DEBUG"
	print "\tif (debug & DEBUG_SYSCALLS)"
	printf "\t\twarnx(\"RETURN"
	if (return_type != "void") \
		printf " %%#x"
	printf "\""
	if (return_type != "void") \
		printf ", r"
	print ");"
	print "#endif /* DEBUG */"

	# return the result
	# currently no return value conversions are necessary
	if (return_type != "void") \
		print "\treturn (r);"

	print "}"
	print ""

	# save the function name for later table generation
	s[syscall_no] = syscall
}

END {
	if (emulation == "sco") \
		sco_finish()
	else
		bsd_finish()
}

function sco_finish() {
	high_syscall_max = syscall_no - syscall_inc

	# at the end, emit the syscall table(s)
	print "int (*low_syscall[])() = {"
	for (i = 0; i <= low_syscall_max; ++i) \
		print "\t(int (*)())x_" emulation "_" s[i] ","
	print "};"
	print ""

	print "int (*high_syscall[])() = {"
	for (i = high_syscall_min; i <= high_syscall_max; i += 256) \
		print "\t(int (*)())x_" emulation "_" s[i] ","
	print "};"
	print ""

	print "int low_syscall_max =", low_syscall_max ";"
	print "int high_syscall_min =", high_syscall_min ";"
	print "int high_syscall_max =", high_syscall_max ";"
}

function bsd_finish() {
	print "int (*bsd_syscall[])() = {"
	for (i = 0; i < syscall_no; ++i) {
		if (s[i] == "") \
			print "\t0,"
		else \
			print "\t(int (*)())x_" emulation "_" s[i] ","
	}
	print "};"
	print ""
	print "int bsd_syscall_max =", syscall_no - 1 ";"
}
