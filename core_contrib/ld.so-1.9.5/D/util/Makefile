include ../Version.mk
include ../Config.mk

ALL	= ldconfig ldd lddstub elf-ok

all:	$(ALL)

ldconfig: ldconfig.o readelf.o
	$(CC) -static $(CFLAGS) $(LDFLAGS) $^ -o $@

ldd:	ldd.o
	$(CC) -static $(CFLAGS) $(LDFLAGS) $^ -o $@

ldd.o:	ldd.c
	$(CC) -c $(CFLAGS) -DLDSO_ADDR=$(LDSO_ENTRY) $< -o $@

ifeq ($(DEBUG),true)
STUBFLAGS = -Wl,-dynamic-linker,../d-link/ld-linux.so
endif

lddstub: lddstub.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(STUBFLAGS) $^ -o $@

elf-ok: elf-ok.S
	$(CC) $(CFLAGS) -nostartfiles -static -s $^ -o $@

clean:
	rm -f $(ALL) *.o *~ core

