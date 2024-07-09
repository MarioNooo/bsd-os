include ../Version.mk
include ../Config.mk

DIRS = $(ARCH) libdl

CFLAGS += -I. -I./$(ARCH) -DNO_UNDERSCORE -DVERBOSE_DLINKER \
	-DUSE_CACHE -fPIC -D__PIC__ #-funroll-loops

SRC2S = boot1.c hash.c readelflib1.c vsprintf.c

SRCS	= $(SRC1S) $(SRC2S)
OBJ1S	= $(SRC1S:.S=.o)
OBJ2S	= $(SRC2S:.c=.o)
OBJS	= $(OBJ1S) $(OBJ2S)

DLINKER = ld-linux.so

ELF_LDFLAGS=--shared # using GNU ld
#ELF_LDFLAGS=-G  # with SVr4 ld

lib:: $(OBJS)
	@for i in $(DIRS); do \
	  echo making $@ in $$i; \
	  ($(MAKE) -C $$i $@); \
	done;
	$(LD) -e _dl_boot $(ELF_LDFLAGS) -o $(DLINKER) \
	  -soname $(DLINKER).$(VMAJOR) *.o

obj: $(OBJS)

realclean::
	$(RM) -f .depend $(DLINKER) core *.o *.a *.s *.i tmp_make foo *~

clean::
	$(RM) -f $(DLINKER) core *.o *.a *.s *.i tmp_make foo *~

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

