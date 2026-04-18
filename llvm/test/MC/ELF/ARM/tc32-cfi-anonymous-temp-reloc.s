@ RUN: llvm-mc -filetype=obj -triple tc32-unknown-none-elf %s -o %t.o
@ RUN: llvm-readelf -sr %t.o | FileCheck %s

func:
  .cfi_startproc
  tmov r0, 0
  .cfi_def_cfa_offset 8
  treti
  .cfi_endproc

@ CHECK: Relocation section '.rel.eh_frame'
@ CHECK: R_ARM_REL32            00000000   .L0
@ CHECK: Symbol table '.symtab' contains 4 entries:
@ CHECK: 2: 00000000 0 NOTYPE LOCAL DEFAULT 2 .L0
