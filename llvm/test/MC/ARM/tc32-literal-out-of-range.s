@ RUN: not llvm-mc -filetype=obj -triple tc32-unknown-none-elf %s -o /dev/null 2>&1 | FileCheck %s

  .text
  .globl start
start:
  ldr r0, .Llit
  tj .Lafter
  .space 1024
  .balign 4
.Llit:
  .word 0x11223344
.Lafter:
  treti

@ CHECK: :[[#@LINE-9]]:11: error: invalid TC32 literal load target
