#	@(#)Makefile	8.7 (Berkeley) 8/18/96

MACROS=	-ms
ROFF=	groff
TBL=	tbl

all: vitut.ps summary.ps viapwh.ps

vitut.ps: vi.in vi.chars
	${TBL} vi.in vi.chars | ${ROFF} ${MACROS} > $@

summary.ps: vi.summary
	${TBL} vi.summary | ${ROFF}  ${MACROS} > $@

viapwh.ps: vi.apwh.ms
	${TBL} vi.apwh.ms | ${ROFF} ${MACROS} > $@

clean:
	rm -f vitut.ps summary.ps viapwh.ps
