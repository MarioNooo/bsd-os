include ../Version.mk
include ../Config.mk

CC = $(LIBC5_CC)

ALL = ld.so

all:	$(ALL)

ld.so: 	preld.o ld.so.o fixups.o fdprintf.o vsprintf.o \
	unsetenv.o strerror.o preload.o
	$(CC) -static -nostdlib -Wl,-Ttext,$(LDSO_ADDR),-T,ldso.link \
		-o $@ $^ -lc -lgcc

clean:
	rm -f $(ALL) *.o *~ core
	rm -f $(STATIC_DIR)/*.o

