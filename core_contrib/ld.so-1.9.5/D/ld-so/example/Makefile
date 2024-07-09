include ../../Version.mk
include ../../Config.mk

ALL = foo bar.o dispinfo-bar.o readline-bar.o

all:	$(ALL)

foo:	foo.c $(LOBJS)
	$(AOUTCC) -o $@ $^

dispinfo.o: dispinfo.c
	$(AOUTCC) -c $^

bar.o:	bar.c
	$(AOUTCC) -c $^ -o a.out
	$(AOUTLD) -x -Ttext 0x50000000 -o $@ a.out -lc \
		-defsym preload_gethostname=__PLT____gethostname \
		-defsym preload_gethostid=__PLT__gethostid
	rm -f a.out

dispinfo-bar.o:	bar.c dispinfo.o
	$(AOUTCC) -c bar.c -o a.out
	$(AOUTLD) -x -Ttext 0x50000000 -o $@ a.out dispinfo.o -lc \
		-defsym preload_gethostname=__PLT____gethostname \
		-defsym preload_gethostid=__PLT__gethostid
	rm -f a.out

readline-bar.o:	bar.c
	@echo "****** This preload object may fail to build if you do not"
	@echo "******         have the readline library installed."
	@echo "****** Don't worry, just check out the other two examples."
	@echo
	$(AOUTCC) -c $^ -o a.out
	$(AOUTLD) -x -Ttext 0x50000000 -o $@ a.out dispinfo.o -lreadline -lc \
		-u __PLT__ding \
		-defsym preload_ding=__PLT__ding
	rm -f a.out

clean:
	rm -f $(ALL) *.o *~ core a.out
