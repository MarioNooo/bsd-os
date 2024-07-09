include ../../Version.mk
include ../../Config.mk

CFLAGS += -I.. -I../$(ARCH) -DNO_UNDERSCORE -DVERBOSE_DLINKER -DUSE_CACHE
CFLAGS += -fPIC -D__PIC__ #-funroll-loops

SRC2S = dlib.c

SRCS	= $(SRC1S) $(SRC2S)
OBJ1S	= $(SRC1S:.S=.o)
OBJ2S	= $(SRC2S:.c=.o)
OBJS	= $(OBJ1S) $(OBJ2S)

LIBDL = libdl.so

ELF_LDFLAGS=--shared -nostartfiles -nostdlib # using GNU ld
#ELF_LDFLAGS=-G  # with SVr4 ld

lib:: $(OBJS)
	$(CC) $(ELF_LDFLAGS) -o $(LIBDL) -Wl,-soname -Wl,libdl.so.$(VMAJOR) *.o

obj: $(OBJS)

realclean::
	$(RM) -f .depend $(LIBDL) core *.o *.a *.s *.i tmp_make foo *~

clean::
	$(RM) -f $(LIBDL) core *.o *.a *.s *.i tmp_make foo *~

depend::
	$(CC) $(CFLAGS) -M $(SRCS) | \
		sed -e 's,^[ 	]*\(.*.o\)[ 	]*:,$(STATIC_DIR)/$(SUBDIR)/\1 $(SHARED_DIR)/$(SUBDIR)/\1 $(SHARED_LITE_DIR)/$(SUBDIR)/\1 $(DEBUG_DIR)/$(SUBDIR)/\1 $(PROFILE_DIR)/$(SUBDIR)/\1 $(CHECKER_DIR)/$(SUBDIR)/\1:,' > .depend
#	$(MAKE) subdir TARGET=depend

ifneq ($(DIRS),)
lib realclean clean depend::
	@for i in $(DIRS); do \
	  echo making $@ in $$i; \
	  ($(MAKE) -C $$i $@); \
	done;
endif

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

