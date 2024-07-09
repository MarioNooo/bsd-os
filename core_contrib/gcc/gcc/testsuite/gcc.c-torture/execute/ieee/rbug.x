# This doesn't work on d10v if doubles are not 64 bits

if { [istarget "d10v-*-*"] && ! [string-match "*-mdouble64*" $CFLAGS] } {
	set torture_execute_xfail "d10v-*-*"
}
if { [istarget "i?86-*-bsdi*"] } {
        set torture_execute_xfail "i?86-*-bsdi*"
}
return 0
