#	BSDI aout.sc,v 1.3 2003/06/19 21:01:42 torek Exp

test -z "${BIG_OUTPUT_FORMAT}" && BIG_OUTPUT_FORMAT=${OUTPUT_FORMAT}
test -z "${LITTLE_OUTPUT_FORMAT}" && LITTLE_OUTPUT_FORMAT=${OUTPUT_FORMAT}
test -z "${ALIGNMENT}" && ALIGNMENT="4"

cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}", "${BIG_OUTPUT_FORMAT}",
	      "${LITTLE_OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

${RELOCATING+${LIB_SEARCH_DIRS}}
${STACKZERO+${RELOCATING+${STACKZERO}}}
${SHLIB_PATH+${RELOCATING+${SHLIB_PATH}}}
${RELOCATING+${EXECUTABLE_SYMBOLS}}
${RELOCATING+PROVIDE (__stack = 0);}
SECTIONS
{
  ${RELOCATING+. = ${TEXT_START_ADDR};}
  .text :
  {
    CREATE_OBJECT_SYMBOLS
    *(.init)
    *(.text)
    *(.fini)
    *(.note.ABI-tag)
    *(.rodata${RELOCATING+ .rodata.* .gnu.linkonce.r.*})
    /* The next six sections are for SunOS dynamic linking.  The order
       is important.  */
    *(.dynrel)
    *(.hash)
    *(.dynsym)
    *(.dynstr)
    *(.rules)
    *(.need)
    ${RELOCATING+etext = .;}
    ${RELOCATING+_etext = .;}
    ${RELOCATING+__etext = .;}
    ${PAD_TEXT+${RELOCATING+. = ${DATA_ALIGNMENT};}}
  }
  ${RELOCATING+. = ${DATA_ALIGNMENT};}
  .data :
  {
    /* The first three sections are for SunOS dynamic linking.  */
    *(.dynamic)
    *(.got)
    *(.plt)
    *(.data)
    *(.ctors)
    *(.dtors)
    *(.eh_frame)
    *(.linux-dynamic) /* For Linux dynamic linking.  */
    ${CONSTRUCTING+CONSTRUCTORS}
    ${RELOCATING+. = ALIGN(${ALIGNMENT});}
    ${RELOCATING+edata  =  .;}
    ${RELOCATING+_edata  =  .;}
    ${RELOCATING+__edata  =  .;}
  }
  .bss :
  {
    ${RELOCATING+ __bss_start = .};
    *(.bss)
    *(COMMON)
    ${RELOCATING+. = ALIGN(${ALIGNMENT});}
    ${RELOCATING+end = . };
    ${RELOCATING+_end = . };
    ${RELOCATING+__end = . };
  }
  /DISCARD/ :
  {
    *(.stab)
    *(.stabstr)
    *(.note)
    *(.comment)
  }
}
EOF
