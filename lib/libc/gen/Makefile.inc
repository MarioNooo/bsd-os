#	BSDI Makefile.inc,v 2.27 2003/06/04 15:06:13 giff Exp
#
#	@(#)Makefile.inc	8.3 (Berkeley) 4/16/94

# machine-independent gen sources
.PATH: ${.CURDIR}/${MACHINE}/gen ${.CURDIR}/gen

SRCS+=	alarm.c aout_nlist_classify.c aout_nlist_offset.c \
	aout_nlist_scan.c assert.c auth_compat.c auth_subr.c \
	authenticate.c clock.c \
	closedir.c confstr.c crypt.c ctermid.c \
	ctime.c ctime_data.c daemon.c devname.c difftime.c disklabel.c \
	elf32_nlist_classify.c elf32_nlist_offset.c elf32_nlist_scan.c \
	elf64_nlist_classify.c elf64_nlist_offset.c elf64_nlist_scan.c \
	err.c errlst.c exec.c fd_realloc.c fdnlist.c fnmatch.c \
	fstab.c ftok.c fts.c getbsize.c getcap.c getcwd.c \
	getdomainname.c gethostname.c getloadavg.c getlogin.c getlogin_r.c \
	getmntinfo.c getpagesize.c getpass.c getttyent.c getusershell.c \
	getvfsbyname.c glob.c initgroups.c isatty.c login_cap.c \
	nice.c nlist.c nlist_apply.c nlist_classify.c nlist_offset.c \
	opendir.c pause.c popen.c psignal.c pwcache.c raise.c \
	readdir.c readdir_r.c rewinddir.c scandir.c seekdir.c setdomainname.c \
	sethostname.c setmode.c siginterrupt.c siglist.c signal.c \
	sigsetops.c semaphore.c sleep.c sysconf.c sysctl.c syslog.c telldir.c \
	termios.c time.c times.c timezone.c ttyname.c ttyslot.c \
	ualarm.c uname.c unvis.c usleep.c utime.c valloc.c vis.c \
	wait.c wait3.c waitpid.c

# machine-dependent gen sources
.include "${.CURDIR}/${MACHINE}/gen/Makefile.inc"

.if (${MACHINE} == "tahoe" || ${MACHINE} == "vax")
errlst.o errlst.po:
	${CC} -S ${CFLAGS} ${.IMPSRC}
	ed - < ${.CURDIR}/${MACHINE}/:errfix errlst.s
	${AS} -o ${.TARGET} errlst.s
	rm -f errlst.s
.endif

MAN3+=	alarm.0 auth_compat.0 auth_subr.0 authenticate.0 clock.0 \
	clock_gettime.0 confstr.0 crypt.0 ctermid.0 ctime.0 daemon.0 \
	devname.0 directory.0 err.0 exec.0 fnmatch.0 frexp.0 ftok.0 fts.0 \
	getbsize.0 getcap.0 getcwd.0 getdiskbyname.0 getdomainname.0 \
	getfsent.0 gethostname.0 getloadavg.0 getmntinfo.0 getpagesize.0 \
	getpass.0 getttyent.0 getusershell.0 getvfsbyname.0 glob.0 \
	initgroups.0 isinf.0 ldexp.0 login_cap.0 modf.0 nice.0 nlist.0 \
	pause.0 popen.0 psignal.0 pwcache.0 raise.0 realpath.0 scandir.0 \
	semaphore.0 setjmp.0 setmode.0 siginterrupt.0 signal.0 sigsetops.0 \
	sleep.0 sysconf.0 sysctl.0 syslog.0 tcgetpgrp.0 tcsendbreak.0 \
	tcsetattr.0 tcsetpgrp.0 time.0 times.0 timezone.0 ttyname.0 tzset.0 \
	ualarm.0 uname.0 unvis.0 utime.0 valloc.0 vis.0

MLINKS+=auth_compat.3 auth_approve.3 auth_compat.3 auth_check.3 \
	auth_compat.3 auth_clropts.3 auth_compat.3 auth_env.3 \
	auth_compat.3 auth_response.3 auth_compat.3 auth_rmfiles.3 \
	auth_compat.3 auth_scan.3 auth_compat.3 auth_script.3 \
	auth_compat.3 auth_script_data.3 auth_compat.3 auth_setopt.3 \
	auth_compat.3 auth_value.3
MLINKS+=auth_subr.3 auth_open.3 auth_subr.3 auth_call.3 \
	auth_subr.3 auth_challenge.3 auth_subr.3 auth_check_change.3 \
	auth_subr.3 auth_check_expire.3 auth_subr.3 auth_clean.3 \
	auth_subr.3 auth_close.3 auth_subr.3 auth_clrenv.3 \
	auth_subr.3 auth_clroption.3 auth_subr.3 auth_clroptions.3 \
	auth_subr.3 auth_getitem.3 auth_subr.3 auth_getstate.3 \
	auth_subr.3 auth_getvalue.3 auth_subr.3 auth_set_va_list.3 \
	auth_subr.3 auth_setdata.3 auth_subr.3 auth_setenv.3 \
	auth_subr.3 auth_setitem.3 auth_subr.3 auth_setoption.3 \
	auth_subr.3 auth_setpwd.3 auth_subr.3 auth_setstate.3
MLINKS+=auth_subr.3 bsd_auth.3
MLINKS+=authenticate.3 auth_approval.3 authenticate.3 auth_cat.3 \
	authenticate.3 auth_checknologin.3 authenticate.3 auth_mkvalue.3 \
	authenticate.3 auth_userchallenge.3 authenticate.3 auth_usercheck.3 \
	authenticate.3 auth_userokay.3 authenticate.3 auth_userresponse.3 \
	authenticate.3 auth_verify.3
MLINKS+=crypt.3 encrypt.3 crypt.3 setkey.3
MLINKS+=clock_gettime.3 clock_settime.3
MLINKS+=clock_gettime.3 clock_getres.3
MLINKS+=ctime.3 asctime.3 ctime.3 difftime.3 ctime.3 gmtime.3 \
	ctime.3 localtime.3 ctime.3 mktime.3 \
	ctime.3 asctime_r.3 ctime.3 ctime_r.3 ctime.3 gmtime_r.3 \
	ctime.3 localtime_r.3
MLINKS+=directory.3 closedir.3 directory.3 dirfd.3 directory.3 opendir.3 \
	directory.3 readdir.3 directory.3 rewinddir.3 directory.3 seekdir.3 \
	directory.3 telldir.3 directory.3 readdir_r.3
MLINKS+=exec.3 execl.3 exec.3 execle.3 exec.3 execlp.3 exec.3 execv.3 \
	exec.3 execvp.3
MLINKS+=err.3 verr.3 err.3 errx.3 err.3 verrx.3 err.3 warn.3 err.3 vwarn.3 \
	err.3 warnx.3 err.3 vwarnx.3
MLINKS+=fts.3 fts_children.3 fts.3 fts_close.3 fts.3 fts_open.3 \
	fts.3 fts_read.3 fts.3 fts_set.3
MLINKS+=isinf.3 isnan.3
MLINKS+=getcap.3 cgetcap.3 getcap.3 cgetclose.3 getcap.3 cgetent.3 \
	getcap.3 cgetfirst.3 getcap.3 cgetmatch.3 getcap.3 cgetnext.3 \
	getcap.3 cgetnum.3 getcap.3 cgetset.3 getcap.3 cgetstr.3 \
	getcap.3 cgetustr.3
MLINKS+=getcwd.3 getwd.3
MLINKS+=getfsent.3 endfsent.3 getfsent.3 getfsfile.3 getfsent.3 getfsspec.3 \
	getfsent.3 getfstype.3 getfsent.3 setfsent.3
MLINKS+=getdomainname.3 setdomainname.3
MLINKS+=gethostname.3 sethostname.3
MLINKS+=getttyent.3 endttyent.3 getttyent.3 getttynam.3 getttyent.3 setttyent.3
MLINKS+=getusershell.3 endusershell.3 getusershell.3 setusershell.3
MLINKS+=login_cap.3 login_getclass.3 login_cap.3 login_getstyle.3 \
	login_cap.3 login_getcapbool.3 login_cap.3 login_getcapnum.3 \
	login_cap.3 login_getcapsize.3 login_cap.3 login_getcapstr.3 \
	login_cap.3 login_getcaptime.3 login_cap.3 login_close.3 \
	login_cap.3 secure_path.3 login_cap.3 setclasscontext.3 \
	login_cap.3 setusercontext.3
MLINKS+=glob.3 globfree.3
MLINKS+=popen.3 pclose.3
MLINKS+=psignal.3 sys_siglist.3
MLINKS+=pwcache.3 user_from_uid.3 pwcache.3 group_from_gid.3
MLINKS+=scandir.3 alphasort.3
MLINKS+=setjmp.3 _longjmp.3 setjmp.3 _setjmp.3 setjmp.3 longjmp.3 \
	setjmp.3 longjmperr.3 setjmp.3 longjmperror.3 \
	setjmp.3 sigsetjmp.3 setjmp.3 siglongjmp.3
MLINKS+=setmode.3 getmode.3
MLINKS+=sigsetops.3 sigemptyset.3 sigsetops.3 sigfillset.3 \
	sigsetops.3 sigaddset.3 sigsetops.3 sigdelset.3 \
	sigsetops.3 sigismember.3
MLINKS+=sleep.3 usleep.3
MLINKS+=syslog.3 closelog.3 syslog.3 openlog.3 syslog.3 setlogmask.3 \
	syslog.3 vsyslog.3
MLINKS+=tcsendbreak.3 tcdrain.3 tcsendbreak.3 tcflush.3 tcsendbreak.3 tcflow.3
MLINKS+=tcsetattr.3 tcgetattr.3 tcsetattr.3 cfsetospeed.3 \
	tcsetattr.3 cfgetospeed.3 tcsetattr.3 cfgetispeed.3 \
	tcsetattr.3 cfsetispeed.3 tcsetattr.3 cfsetspeed.3 \
	tcsetattr.3 cfmakeraw.3
MLINKS+=ttyname.3 isatty.3 ttyname.3 ttyname_r.3 ttyname.3 ttyslot.3
MLINKS+=tzset.3 tzsetwall.3
MLINKS+=vis.3 strvis.3 vis.3 strvisx.3
MLINKS+=unvis.3 strunvis.3
