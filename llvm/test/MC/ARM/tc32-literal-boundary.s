@ RUN: llvm-mc -filetype=obj -triple tc32-unknown-none-elf %s -o %t.o
@ RUN: llvm-objdump -d -r --triple=tc32 %t.o | FileCheck %s

  .text
  .globl start
start:
  ldr r0, .Llit
  tj .Lafter
  .space 1016
  .balign 4
.Llit:
  .word 0x11223344
.Lafter:
  treti

@ CHECK-LABEL: <start>:
@ CHECK:       0: fe 08         tloadr	r0, [pc, #0x3f8]
@ CHECK:       2: fd 81         tj
@ CHECK:       3fc: 44 33 22 11   .word	0x11223344
@ CHECK:       400: 00 69         treti	{r15}
