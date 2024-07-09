include ../Version.mk
include ../Config.mk

CC = $(LIBC5_CC)

ifeq ($(AOUT_SUPPORT),true)
AOUT_ALL = atesti atestf
endif

ALL = etesti etestf $(AOUT_ALL)

all: $(ALL)

etesti: testi.c
	$(CC) -Wl,-dynamic-linker,../d-link/ld-linux.so $^ -o $@

etestf: testf.c
	$(CC) -Wl,-dynamic-linker,../d-link/ld-linux.so $^ -lm -o $@

atesti: atesti.save
	ln -f $< $@

atestf: atestf.save
	ln -f $< $@

atesti.save: testi.c __load.c
	$(AOUTCC) -DLDSO_ADDR=$(LDSO_ENTRY) $^ -o $@

atestf.save: testf.c __load.c
	$(AOUTCC) -DLDSO_ADDR=$(LDSO_ENTRY) $^ -lm -o $@

clean:
	rm -f $(ALL) *.o *~ core

