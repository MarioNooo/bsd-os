gethostid() {
	return 12345;
}

gethostname(char *hn, int sz) {
	bcopy("foo", hn, sz);
	return 0;
}

/* This is for libreadline - demonstrates how different functions for
   different libraries may be used. It basically overrides the beep noise
   you get when libreadline wants to warn you */

ding() {
	write(2,"\toops\n\r",6);
}
