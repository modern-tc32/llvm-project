// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: 7f c0         tjeq    0x102 <ok>              @ imm = #0xfe
// CHECK:      00000102 <ok>:
// CHECK-NEXT:      102: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tjeq ok
  .space 256
ok:
  tjex lr
