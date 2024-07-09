CC		=	gcc 
TARGET		=	testj
LDFLAGS		=	
VPATH		=	..:
CDEBUGFLAGS	=	-g
#CDEBUGFLAGS	=	-g -Wall
CDEBUGDEFS	=	-D_DEBUG
CRELEASEFLAGS	=	-DUNIX
OBJS		=	mime.o mangle.o utils.o enriched.o striphtml.o \
			lineend.o etags.o
INCLUDEDIRS	= 	-I..


all: ${TARGET}

${TARGET} : ${OBJS} ${TARGET}.o
	${CC} ${OBJS} ${TARGET}.o -o ${TARGET} ${LDFLAGS}

%.o: %.c
	${CC} ${CDEBUGFLAGS} ${CRELEASEFLAGS} -c ${CDEBUGDEFS} ${INCLUDEDIRS} $< -o $@

clean:
	rm -f ${OBJS}
	rm -f ${TARGET}
	rm -f *~
