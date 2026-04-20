// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump --triple=tc32 -d %t | FileCheck %s

  .syntax unified
  .thumb
  .text

start:
  tloadr   r0, table
  tloadr   r1, table + 4
  tloadrb  r2, table+8
  tloadrh  r3, table+12
  tloadrsb r4, table+16
  tloadrsh r5, table+20
  tloadr   r6, =table+24

// CHECK:      <start>:
// CHECK-NEXT: tloadr r0, [pc,
// CHECK-NEXT: tloadr r1, [pc,
// CHECK-NEXT: tloadr r2, [pc,
// CHECK-NEXT: tloadr r3, [pc,
// CHECK-NEXT: tloadr r4, [pc,
// CHECK-NEXT: tloadr r5, [pc,
// CHECK-NEXT: tloadr r6, [pc,

  .p2align 2
table:
  .word 0x11111111
  .word 0x22222222
  .word 0x33333333
  .word 0x44444444
  .word 0x55555555
  .word 0x66666666
  .word 0x77777777
