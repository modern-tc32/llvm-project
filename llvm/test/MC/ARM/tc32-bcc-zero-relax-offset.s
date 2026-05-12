// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: 8c 02         tcmp    r4, r1
// CHECK-NEXT:        2: 40 90 01 20   tjne    0x8 <target>
// CHECK-NEXT:        6: 00 a4         tmov    r4, #0x0
// CHECK:      00000008 <target>:
// CHECK-NEXT:        8: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tcmp r4, r1
  tjne target
  tmov r4, #0
target:
  tjex lr
