# BSDI Makefile,v 2.57 2003/08/21 18:42:01 polk Exp

SUBDIR= apply apropos arch at awk banner basename bc biff bpatch \
	bsd2dos cal calendar cap_mkdb cdctl checknr \
	chflags chpass cksum cmp col colcrt colrm column comm \
	compress cput crontab ctags cut diff dirname dos2bsd du \
	env error expand false fdformat find finger fmt fold fpr \
	from fsplit fstat ftp gcore getconf gettydproxy \
	groff head hexdump hostid id indent ipcrm ipcs join jot \
	kdump ktrace lam last lastcomm ldd leave lex line locate lock \
	logger login logname look lorder lp mail make man mesg \
	mkdep mkfifo mklocale mkstr more msgs nfsstat nice \
	nm nohup pagesize passwd paste perl pppattach ppp pr printenv \
	printf quota rcs rdist renice rev rlimit rpasswd rpcgen rpcinfo \
	rs rstat ruptime rwho script sed showmount size soelim sort \
	split strings strip su systat tail talk tcopy tee tftp time \
	tip touch tr true tsort tty ttydesc ul uname unexpand \
	unifdef uniq units unvis users uucp uudecode uuencode \
	vacation vgrind vis w wall wc what whatis whereis \
	which who whois window write xargs xinstall xstr yacc yes \
	ypcat ypmatch ypwhich

# These don't build anymore, because we don't have sound support.
# (they both need <sys/soundcard.h>)
#SUBDIR+=mixer vplay

.if exists(${.CURDIR}/../domestic/Makefile)
# The following require Kerberos to compile as configured
SUBDIR+=telnet 
.endif

.if	${MACHINE} == "i386"
SUBDIR+=i386
.elif	${MACHINE} == "sparc" || ${MACHINE} == "sparc_v9"
SUBDIR+=sparc w.v8 w.v9
.elif	${MACHINE} == "powerpc"
SUBDIR+=
.endif

.include <bsd.subdir.mk>
