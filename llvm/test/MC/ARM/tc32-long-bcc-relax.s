// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump --triple=tc32 -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: 01 90 c4 21   tjeq    0x138c <target>         @ imm = #0x1388
// CHECK:      0000138c <target>:
// CHECK-NEXT:     138c: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tjeq target
  .space 5000
target:
  tjex lr
