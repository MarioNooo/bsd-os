include Version.mk
include Config.mk

SUBDIRS = man util d-link
ifeq ($(AOUT_SUPPORT),true)
SUBDIRS += ld-so
endif
ifeq ($(ARCH),i386)
SUBDIRS += test
endif

all:
	set -e ; for d in $(SUBDIRS) ; do $(MAKE) -C $$d ; done

install: all
	sh instldso.sh

clean:
	set -e ; for d in $(SUBDIRS) ; do $(MAKE) -C $$d $@ ; done
	-find . -name '*~' | xargs rm -f

dist:
	$(MAKE) clean
	$(MAKE) all
	find . -name '*.o' | xargs rm
	cd .. && \
	tar cvf ld.so-$(VERSION).tar ld.so-$(VERSION) && \
	gzip -v9 ld.so-$(VERSION).tar

