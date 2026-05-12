// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: 40 90 01 20   tjne    0x6 <target>
// CHECK-NEXT:        4: c0 06         nop
// CHECK:      00000006 <target>:
// CHECK-NEXT:        6: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tjne target
  nop
target:
  tjex lr
